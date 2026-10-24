#include "Cemu/OpenPak/Account.h"

#include "Cemu/OpenPak/Client.h"
#include "Cemu/OpenPak/Errors.h"
#include "Cemu/OpenPak/Prefs.h"
#include "Cemu/Logging/CemuLogging.h"
#include "Cemu/ncrypto/ncrypto.h"
#include "Cafe/Account/Account.h"
#include "config/ActiveSettings.h"
#include "config/CemuConfig.h"
#include "config/NetworkSettings.h"

#include <openpak/account.h>
#include <openpak/api.h>
#include <openpak/platform.h>
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <fmt/core.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <map>
#include <mutex>
#include <random>
#include <sstream>
#include <thread>
#include <vector>

// The website session (the bearer) is the client library's account file, config/openpak_account.txt,
// shared with every OpenPak emulator. What is Wii U's alone -- the PNID, the Mii and the account.dat
// password cache the website minted -- stays in config/openpak_session.txt, keys as before.

namespace
{
	namespace Api = WebService::OpenPakApi;

	std::mutex g_mutex;
	bool g_loaded = false;
	std::string g_accountId;   // core account uuid
	uint32 g_pid = 0;
	std::string g_username;    // the NNID
	std::string g_miiName;
	std::string g_miiData;     // base64
	std::string g_passwordCache; // base64
	std::string g_country;
	std::string g_language;

	constexpr const char* kSessionFile = "openpak_session.txt";

	std::filesystem::path SessionPath() { return ActiveSettings::GetConfigPath(kSessionFile); }

	std::map<std::string, std::string> ParseKeyValueFile(const std::filesystem::path& path)
	{
		std::map<std::string, std::string> out;
		std::ifstream file(path);
		std::string line;
		while (std::getline(file, line))
		{
			const auto eq = line.find('=');
			if (eq == std::string::npos || eq == 0)
				continue;
			out.emplace(line.substr(0, eq), line.substr(eq + 1));
		}
		return out;
	}

	// Caller holds g_mutex.
	void SaveSession()
	{
		std::ofstream file(SessionPath(), std::ios::trunc);
		file << "account_id=" << g_accountId << std::endl;
		file << "username=" << g_username << std::endl;
		file << "pid=" << fmt::format("{:08x}", g_pid) << std::endl;
		file << "mii_name=" << g_miiName << std::endl;
		file << "mii_data=" << g_miiData << std::endl;
		file << "password_cache=" << g_passwordCache << std::endl;
		file << "country=" << g_country << std::endl;
		file << "language=" << g_language << std::endl;
	}

	// Caller holds g_mutex.
	void EnsureLoaded()
	{
		if (g_loaded)
			return;
		g_loaded = true;
		OpenPakClient::Init();
		const auto values = ParseKeyValueFile(SessionPath());
		auto pick = [&values](const char* key) {
			const auto it = values.find(key);
			return it != values.end() ? it->second : std::string{};
		};
		g_accountId = pick("account_id");
		g_username = pick("username");
		g_miiName = pick("mii_name");
		g_miiData = pick("mii_data");
		g_passwordCache = pick("password_cache");
		g_country = pick("country");
		g_language = pick("language");
		try
		{
			g_pid = values.count("pid") ? std::stoul(values.at("pid"), nullptr, 16) : 0;
		}
		catch (...)
		{
			g_pid = 0;
		}
		// Builds before the client library kept the bearer in this file: it moves to the library's
		// account file once, and this file keeps the rest.
		if (const std::string bearer = pick("bearer"); !bearer.empty())
		{
			if (!Common::OpenPakAccount::HasBearer())
				Common::OpenPakAccount::SaveBearerOnly(g_username, bearer);
			SaveSession();
			cemuLog_log(LogType::Force, "OpenPak: moved the stored sign-in to the shared account file");
		}
	}

	struct WiiUIdentity
	{
		std::string error; // an OpenPakError code when the identity could not be had
		nlohmann::json doc;
	};

	// GET /api/v1/me/wiiu: the account's Wii U identity, minted server-side on first sight.
	WiiUIdentity FetchWiiUIdentity(const std::string& bearer)
	{
		WiiUIdentity out;
		httplib::Client client(Api::BaseUrl());
		Api::ApplySystemCa(client);
		client.set_connection_timeout(15);
		client.set_read_timeout(15);
		httplib::Headers headers{{"Authorization", "Bearer " + bearer}};
		if (std::string header = openpak::Platform::ClientHeader(); !header.empty())
			headers.emplace("X-OpenPak-Client", std::move(header));
		const auto result = client.Get("/api/v1/me/wiiu", headers);
		if (!result)
		{
			cemuLog_log(LogType::Force, "OpenPak: could not reach the website: {}", httplib::to_string(result.error()));
			out.error = OpenPakError::Unreachable;
			return out;
		}
		nlohmann::json doc = nlohmann::json::parse(result->body, nullptr, false);
		if (result->status != 200)
		{
			if (doc.is_object() && doc.contains("error") && doc["error"].is_string() && !doc["error"].get<std::string>().empty())
				out.error = OpenPakError::Server(doc["error"].get<std::string>());
			else
			{
				cemuLog_log(LogType::Force, "OpenPak: the account request failed (HTTP {})", result->status);
				out.error = OpenPakError::Unreachable;
			}
			return out;
		}
		if (!doc.is_object())
		{
			cemuLog_log(LogType::Force, "OpenPak: the identity response was not understood");
			out.error = OpenPakError::Unreachable;
			return out;
		}
		out.doc = std::move(doc);
		return out;
	}

	std::string JsonString(const nlohmann::json& doc, const char* field)
	{
		const auto it = doc.find(field);
		return it != doc.end() && it->is_string() ? it->get<std::string>() : std::string{};
	}

	uint32 JsonUint(const nlohmann::json& doc, const char* field)
	{
		const auto it = doc.find(field);
		return it != doc.end() && it->is_number_unsigned() ? (uint32)it->get<uint64>() : 0;
	}

	std::string ToHex(const uint8* data, size_t len)
	{
		std::string out;
		out.reserve(len * 2);
		char buf[8];
		for (size_t i = 0; i < len; ++i)
		{
			snprintf(buf, sizeof(buf), "%02x", data[i]);
			out += buf;
		}
		return out;
	}

	// NNAS country index from an ISO name, via Cemu's own table.
	uint32 CountryIndex(const std::string& countryName)
	{
		if (!countryName.empty())
		{
			for (sint32 i = 1; i < 512; ++i)
			{
				if (std::string_view(NCrypto::GetCountryAsString(i)) == countryName)
					return (uint32)i;
			}
		}
		return 49; // US, the network profile's default
	}

	std::wstring MiiNameFromUtf8(const std::string& utf8)
	{
		std::wstring out;
		// the server truncates to ten runes; widen the same width account.dat stores
		size_t i = 0;
		while (i < utf8.size() && out.size() < 10)
		{
			// decode one UTF-8 rune
			const auto c = (unsigned char)utf8[i];
			size_t len = 1;
			if (c >= 0xF0) len = 4;
			else if (c >= 0xE0) len = 3;
			else if (c >= 0xC0) len = 2;
			if (i + len > utf8.size())
				break;
			uint32 cp = c & (0xFF >> (len + 1));
			for (size_t k = 1; k < len; ++k)
				cp = (cp << 6) | ((unsigned char)utf8[i + k] & 0x3F);
			out.push_back((wchar_t)cp);
			i += len;
		}
		return out;
	}
} // namespace

namespace OpenPakAccount
{
	SignInResult SignIn(std::string_view email, std::string_view password, std::string_view deviceName)
	{
		SignInResult out;
		{
			std::lock_guard lock(g_mutex);
			EnsureLoaded();
		}
		if (!deviceName.empty())
		{
			// how this machine appears in the account's device list (UX spec §3.3)
			Api::SetDeviceName(std::string(deviceName));
			Api::SetSaveDevice(std::string(deviceName));
		}
		const auto login = Api::SignInAccountOnly(std::string(email), std::string(password));
		if (!login.ok)
		{
			out.error = OpenPakError::FromLibrary(login.error);
			return out;
		}

		const auto identity = FetchWiiUIdentity(login.bearer);
		if (!identity.error.empty())
		{
			out.error = identity.error;
			return out;
		}
		const auto& idDoc = identity.doc;
		const uint32 pid = JsonUint(idDoc, "pid");
		const std::string username = JsonString(idDoc, "username");
		const std::string miiData = JsonString(idDoc, "mii_data");
		const std::string passwordCache = JsonString(idDoc, "password_cache");
		if (pid == 0 || username.empty() || miiData.empty() || passwordCache.empty())
		{
			cemuLog_log(LogType::Force, "OpenPak: the account has no Wii U identity; is the Wii U adapter online?");
			out.error = OpenPakError::NoIdentity;
			return out;
		}

		{
			std::lock_guard lock(g_mutex);
			g_accountId = JsonString(idDoc, "account_id");
			g_pid = pid;
			g_username = username;
			g_miiName = JsonString(idDoc, "mii_name");
			g_miiData = miiData;
			g_passwordCache = passwordCache;
			g_country = JsonString(idDoc, "country");
			g_language = JsonString(idDoc, "language");
			SaveSession();
			Common::OpenPakAccount::SaveBearerOnly(username, login.bearer);
		}
		out.ok = true;
		return out;
	}

	void SignOut()
	{
		std::string bearer;
		{
			std::lock_guard lock(g_mutex);
			EnsureLoaded();
			bearer = Common::OpenPakAccount::GetBearer();
			Common::OpenPakAccount::Clear();
			g_accountId.clear();
			g_pid = 0;
			g_username.clear();
			g_miiName.clear();
			g_miiData.clear();
			g_passwordCache.clear();
			g_country.clear();
			g_language.clear();
			SaveSession();
		}
		// End the website session too: best-effort, in the background, so presence ends now
		// without the UI waiting on the network.
		if (!bearer.empty())
			std::thread([bearer]() { Api::RevokeToken(bearer); }).detach();
	}

	bool DropIfRejected()
	{
		if (!IsSignedIn())
			return false;
		// GET /api/v1/me: the library signs out on a 401, and only on a 401.
		Api::GetProfile();
		if (Common::OpenPakAccount::HasBearer())
			return false;
		cemuLog_log(LogType::Force, "OpenPak: the stored sign-in was refused; signed out");
		return true;
	}

	std::string GetMiiName()
	{
		std::lock_guard lock(g_mutex);
		EnsureLoaded();
		return g_miiName;
	}

	uint32_t GetPid()
	{
		std::lock_guard lock(g_mutex);
		EnsureLoaded();
		return g_pid;
	}

	uint32_t AppliedSlot()
	{
		const uint32 slot = OpenPakPrefs::Slot();
		if (slot == 0)
			return 0;
		for (const auto& account : Account::GetAccounts())
		{
			if (account.GetPersistentId() == slot)
				return slot;
		}
		return 0;
	}

	bool IsSignedIn()
	{
		{
			std::lock_guard lock(g_mutex);
			EnsureLoaded();
		}
		return Common::OpenPakAccount::HasBearer();
	}

	std::string GetUsername()
	{
		std::lock_guard lock(g_mutex);
		EnsureLoaded();
		return g_username;
	}

	std::string GetBearer()
	{
		{
			std::lock_guard lock(g_mutex);
			EnsureLoaded();
		}
		return Common::OpenPakAccount::GetBearer();
	}

	std::string ApplyIdentity()
	{
		std::string username, miiName, miiData, passwordCache, country;
		uint32 pid = 0;
		{
			std::lock_guard lock(g_mutex);
			EnsureLoaded();
			username = g_username;
			pid = g_pid;
			miiName = g_miiName;
			miiData = g_miiData;
			passwordCache = g_passwordCache;
			country = g_country;
		}
		if (pid == 0 || miiData.empty() || passwordCache.empty())
			return "no OpenPak identity to apply; sign in first";

		auto miiBytes = NCrypto::base64Decode(miiData);
		auto cacheBytes = NCrypto::base64Decode(passwordCache);
		if (miiBytes.size() != 96 || cacheBytes.size() != 32)
			return "the stored identity is malformed; sign in again";

		// UX spec §5.10: write into the Mii account OpenPak used before — the remembered slot,
		// else one already holding this PID (an earlier build made a new one per sign-in) — and
		// take a new slot only when there is none.
		Account::RefreshAccounts();
		uint32 persistentId = 0;
		std::array<uint8, 16> uuid{};
		bool haveUuid = false;
		const uint32 remembered = OpenPakPrefs::Slot();
		for (const auto& account : Account::GetAccounts())
		{
			if (remembered != 0 && account.GetPersistentId() == remembered)
			{
				persistentId = remembered;
				uuid = account.GetUuid();
				haveUuid = true;
				break;
			}
		}
		if (persistentId == 0)
		{
			for (const auto& account : Account::GetAccounts())
			{
				if (account.GetPrincipalId() == pid)
				{
					persistentId = account.GetPersistentId();
					uuid = account.GetUuid();
					haveUuid = true;
					break;
				}
			}
		}
		if (persistentId == 0)
		{
			if (!Account::HasFreeAccountSlots())
				return "every Mii account slot is taken; delete one in General settings > Account";
			persistentId = Account::GetNextPersistentId();
		}

		// account.dat in the exact shape Account::ParseFile reads.
		const auto accountDir = ActiveSettings::GetMlcPath(fmt::format(L"usr/save/system/act/{:08x}", persistentId));
		std::error_code ec;
		std::filesystem::create_directories(accountDir, ec);
		if (ec)
			return fmt::format("cannot create the account directory: {}", ec.message());

		if (!haveUuid)
		{
			std::mt19937 rng(std::random_device{}());
			std::uniform_int_distribution<int> dist(0, 255);
			for (auto& b : uuid)
				b = (uint8)dist(rng);
		}

		const std::wstring miiNameWide = MiiNameFromUtf8(miiName);
		std::ostringstream file;
		file << "AccountInstance_20120705" << std::endl;
		file << fmt::format("PersistentId={:08x}", persistentId) << std::endl;
		file << fmt::format("TransferableIdBase={:x}", (0x2000004ULL << 32) | ((uint64)uuid[12] << 24) | ((uint64)uuid[13] << 16) | ((uint64)uuid[14] << 8) | (uint64)uuid[15]) << std::endl;
		file << "Uuid=" << ToHex(uuid.data(), uuid.size()) << std::endl;
		file << "MiiData=" << ToHex(miiBytes.data(), miiBytes.size()) << std::endl;
		file << "MiiName=";
		for (wchar_t c : miiNameWide)
			file << fmt::format("{:04x}", (uint16)c);
		file << std::endl;
		// The NNAS login sends AccountId as the user_id; the adapter resolves it
		// by username, so this is the minted NNID and never the core uuid.
		file << "AccountId=" << username << std::endl;
		file << fmt::format("BirthYear={:x}", 1990) << std::endl;
		file << fmt::format("BirthMonth={:x}", 1) << std::endl;
		file << fmt::format("BirthDay={:x}", 1) << std::endl;
		file << fmt::format("Gender={:x}", 0) << std::endl;
		file << "EmailAddress=" << std::endl;
		file << fmt::format("Country={:x}", CountryIndex(country)) << std::endl;
		file << fmt::format("SimpleAddressId={:x}", 0) << std::endl;
		file << fmt::format("PrincipalId={:x}", pid) << std::endl;
		file << fmt::format("IsPasswordCacheEnabled={:x}", 1) << std::endl;
		file << "AccountPasswordCache=" << ToHex(cacheBytes.data(), cacheBytes.size()) << std::endl;

		{
			std::ofstream out(accountDir / "account.dat", std::ios::trunc);
			if (!out.is_open())
				return "cannot write account.dat";
			out << file.str();
		}

		Account::UpdatePersisidDat();
		OpenPakPrefs::SetSlot(persistentId);

		// Select the account and point it at the OpenPak service.
		GetConfig().account.m_persistent_id = persistentId;
		GetConfig().SetAccountSelectedService(persistentId, NetworkService::OpenPak);
		GetConfigHandle().Save();
		Account::RefreshAccounts();

		cemuLog_log(LogType::Force,
			"OpenPak: applied the identity of {} (PID {:x}) as account {:08x}; the OpenPak service is selected",
			miiName.empty() ? username : miiName, pid, persistentId);
		return {};
	}
} // namespace OpenPakAccount
