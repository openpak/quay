#include "Cemu/OpenPak/Social.h"
#include "Cemu/OpenPak/Account.h"
#include "Cemu/OpenPak/Client.h"
#include "Cemu/OpenPak/Errors.h"

#include <openpak/api.h>
#include <fmt/core.h>

#include <algorithm>
#include <cctype>
#include <ctime>
#include <mutex>
#include <string>
#include <vector>

// The window's data, through the shared client library (WebService::OpenPakApi): same website
// endpoints, same shapes. What is left here is the translation into the UI's types and its error
// codes.
namespace
{
	namespace Api = WebService::OpenPakApi;

	std::string ToUpper(std::string s)
	{
		std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::toupper(c); });
		return s;
	}

	// RFC3339 UTC from unix seconds, as the UI's LocalTime reads it; empty when unknown.
	std::string Rfc3339(int64_t seconds)
	{
		if (seconds <= 0)
			return {};
		const std::time_t t = (std::time_t)seconds;
		std::tm tm{};
#if BOOST_OS_WINDOWS
		gmtime_s(&tm, &t);
#else
		gmtime_r(&t, &tm);
#endif
		return fmt::format("{:04}-{:02}-{:02}T{:02}:{:02}:{:02}Z", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec);
	}

	OpenPakSocial::Friend FromLibrary(const Api::AccountFriend& entry)
	{
		OpenPakSocial::Friend f;
		f.account_id = entry.account_id;
		f.display_name = entry.display_name;
		f.online = entry.online;
		if (f.online)
		{
			f.title_id = ToUpper(entry.title_id);
			f.console = entry.console_namespace;
			f.since = Rfc3339(entry.online_since);
		}
		return f;
	}

	Api::AccountFriend Request(const std::string& account_id)
	{
		Api::AccountFriend request;
		request.account_id = account_id;
		return request;
	}

	std::mutex s_catalogueMutex;
	std::map<std::string, OpenPakSocial::CatalogueTitle> s_catalogue;
} // namespace

namespace OpenPakSocial
{
	std::pair<FriendList, RequestList> GetFriendsAndRequests()
	{
		OpenPakClient::Init();
		std::pair<FriendList, RequestList> out;
		const auto graph = Api::GetAccountFriends();
		if (!graph.ok)
		{
			out.first.error = out.second.error = OpenPakError::FromLibrary(graph.error);
			return out;
		}
		for (const auto& entry : graph.friends)
			out.first.friends.push_back(FromLibrary(entry));
		for (const auto& entry : graph.requests)
			(entry.incoming ? out.second.incoming : out.second.outgoing).push_back(FromLibrary(entry));
		out.first.ok = out.second.ok = true;
		return out;
	}

	InvitationList GetInvitations()
	{
		OpenPakClient::Init();
		InvitationList out;
		if (!OpenPakAccount::IsSignedIn())
		{
			out.error = OpenPakError::NotSignedIn;
			return out;
		}
		for (const auto& inv : Api::GetInvitations())
			out.invitations.push_back({inv.id, inv.from, ToUpper(inv.title_id), inv.expires_at});
		out.ok = true;
		return out;
	}

	std::string AcceptFriend(const std::string& account_id)
	{
		OpenPakClient::Init();
		return OpenPakError::FromLibrary(Api::AcceptFriendRequest(Request(account_id)));
	}

	// The website API has no separate decline: removing a pending request is the decline (the
	// core ends a pending friendship in either direction).
	std::string DeclineFriend(const std::string& account_id)
	{
		OpenPakClient::Init();
		return OpenPakError::FromLibrary(Api::DeclineFriendRequest(Request(account_id)));
	}

	std::string RemoveFriend(const std::string& account_id)
	{
		OpenPakClient::Init();
		return OpenPakError::FromLibrary(Api::RemoveAccountFriend(account_id));
	}

	std::string BlockFriend(const std::string& account_id)
	{
		OpenPakClient::Init();
		return OpenPakError::FromLibrary(Api::BlockAccount(account_id));
	}

	std::string SendFriendRequest(const std::string& friend_code)
	{
		OpenPakClient::Init();
		return OpenPakError::FromLibrary(Api::SendFriendRequest(friend_code));
	}

	Profile GetProfile()
	{
		OpenPakClient::Init();
		Profile out;
		const auto profile = Api::GetProfile();
		if (!profile.ok)
		{
			out.error = OpenPakError::FromLibrary(profile.error);
			return out;
		}
		out.ok = true;
		out.account_id = profile.account_id;
		out.display_name = profile.name;
		out.friend_code = profile.friend_code;
		out.linked_platforms = profile.linked_platforms;
		return out;
	}

	std::optional<std::vector<uint8_t>> GetAvatar(const std::string& account_id)
	{
		OpenPakClient::Init();
		return Api::GetAccountAvatar(account_id);
	}

	std::map<std::string, CatalogueTitle> GetCatalogueIfLoaded()
	{
		std::lock_guard lock(s_catalogueMutex);
		return s_catalogue;
	}

	std::map<std::string, CatalogueTitle> GetCatalogue()
	{
		{
			std::lock_guard lock(s_catalogueMutex);
			if (!s_catalogue.empty())
				return s_catalogue;
		}
		OpenPakClient::Init();
		std::map<std::string, CatalogueTitle> out;
		for (const auto& [id, title] : Api::Catalogue())
			out[id] = {title.name, title.console};
		std::lock_guard lock(s_catalogueMutex);
		s_catalogue = out;
		return out;
	}

	CloudSaves GetCloudSaves()
	{
		OpenPakClient::Init();
		CloudSaves out;
		const auto saves = Api::GetCloudSaves();
		if (!saves.ok)
		{
			out.error = OpenPakError::FromLibrary(saves.error);
			return out;
		}
		out.ok = true;
		out.used = saves.allowance_used;
		out.allowance = saves.allowance;
		for (const auto& item : saves.saves)
		{
			CloudSave save{item.platform, ToUpper(item.title_id), item.name, {}};
			for (const auto& v : item.versions) // newest first already
				save.versions.push_back({v.number, v.conflict, v.size, v.device, v.saved_at});
			out.saves.push_back(std::move(save));
		}
		return out;
	}

	ModList GetMods(const std::string& title_id)
	{
		OpenPakClient::Init();
		ModList out;
		out.ok = true;
		for (const auto& mod : Api::GetMods(title_id))
			out.mods.push_back({mod.id, mod.name, mod.version, mod.author, mod.licence, mod.summary});
		return out;
	}

	std::vector<std::string> GetFavouriteModIds()
	{
		OpenPakClient::Init();
		return Api::GetFavouriteModIds();
	}

	std::string SetModFavourite(const std::string& mod_id, bool favourite)
	{
		OpenPakClient::Init();
		if (!OpenPakAccount::IsSignedIn())
			return OpenPakError::NotSignedIn;
		if (Api::SetModFavourite(mod_id, favourite))
			return {};
		return OpenPakAccount::IsSignedIn() ? OpenPakError::Unreachable : OpenPakError::Expired;
	}

	Players GetPlayers()
	{
		OpenPakClient::Init();
		const auto status = Api::GetStatus();
		Players out;
		out.ok = status.ok;
		out.players_online = status.players_online;
		out.titles = status.titles;
		out.networks = status.networks;
		return out;
	}

	ServiceStatus GetServiceStatus()
	{
		OpenPakClient::Init();
		const auto status = Api::GetServiceStatus();
		ServiceStatus out;
		out.ok = status.ok;
		out.url = status.url;
		out.headline = status.headline;
		out.summary = status.sub;
		out.state = status.state;
		for (const auto& s : status.services)
			out.services.push_back({s.name, s.up, s.uptime, s.latency});
		return out;
	}

	std::optional<int> PingWebsite()
	{
		OpenPakClient::Init();
		return Api::PingBackend();
	}

	std::optional<int> PingUrl(const std::string& url)
	{
		OpenPakClient::Init();
		return Api::PingUrl(url);
	}
} // namespace OpenPakSocial
