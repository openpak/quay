#include "Cemu/OpenPak/NetworkProfile.h"
#include "Cemu/OpenPak/Client.h"

#include <openpak/network_profile.h>

#include <functional>
#include <map>
#include <mutex>
#include <string>

#include "Cemu/Logging/CemuLogging.h"
#include "config/NetworkSettings.h"

// The fetch, the validation, the signed ceiling, the cache (config/openpak_network_profile.json
// and .etag, config/openpak_network_ceiling.*) and the six-hourly re-check are the shared client
// library's (openpak::NetworkProfile). What stays here is Cemu's use of it: the profile's
// services[] by id, over the compiled-in OpenPakURLs.
namespace
{
constexpr const char* kPlatform = "wiiu";

std::mutex g_refreshMutex; // one fetch at a time (launch, the settings button, the re-check)
std::mutex g_mutex;
std::map<std::string, std::string> g_applied; // id -> url, from the profile in use
std::string g_source = "built-in";
int g_version = -1;
std::function<void()> g_onChange;

// The library has applied `result` (and filtered it through the ceiling, services by their host).
void Take(openpak::NetworkProfile::Result const& result)
{
	std::map<std::string, std::string> applied;
	for (auto const& service : result.profile.services)
	{
		if (!service.id.empty() && !service.url.empty())
			applied.emplace(service.id, service.url);
	}
	std::lock_guard lock(g_mutex);
	g_applied = std::move(applied);
	g_source = openpak::NetworkProfile::SourceName(result.source);
	g_version = result.source == openpak::NetworkProfile::Source::BuiltIn ? -1 : static_cast<int>(result.profile.version);
	if (!result.error.empty())
		cemuLog_log(LogType::Force, "network profile: {} in use ({})", g_source, result.error);
}

// Fetches and applies; the digest of what was in use before, for the change check.
std::string RefreshLocked()
{
	const std::string before = openpak::NetworkProfile::DigestInUse();
	Take(openpak::NetworkProfile::Refresh(kPlatform));
	return before;
}

void Notify()
{
	std::function<void()> notify;
	{
		std::lock_guard lock(g_mutex);
		notify = g_onChange;
	}
	cemuLog_log(LogType::Force, "network profile: the effective wiiu redirects changed");
	if (notify)
		notify();
}

} // namespace

namespace OpenPakNetworkProfile
{

void ApplyAtLaunch()
{
	OpenPakClient::Init();
	// Client rule 4: the library's re-check (six hours, and soon after a sign-in) only reports a
	// new effective set; Cemu reads its URLs per request, so it applies the set and then says so.
	openpak::NetworkProfile::SetChangeNotifier([]() {
		{
			std::lock_guard refresh(g_refreshMutex);
			RefreshLocked();
		}
		Notify();
	});
	std::lock_guard refresh(g_refreshMutex);
	Take(openpak::NetworkProfile::Fetch(kPlatform));
}

void Refresh()
{
	OpenPakClient::Init();
	std::string before;
	{
		std::lock_guard refresh(g_refreshMutex);
		before = RefreshLocked();
	}
	if (!before.empty() && before != openpak::NetworkProfile::DigestInUse())
		Notify();
}

void SetChangeListener(std::function<void()> listener)
{
	std::lock_guard lock(g_mutex);
	g_onChange = std::move(listener);
}

std::string ServiceURL(std::string_view id)
{
	{
		std::lock_guard lock(g_mutex);
		if (auto const it = g_applied.find(std::string{id}); it != g_applied.end())
			return it->second;
	}
	// The compiled-in OpenPakURLs stay the fallback: a profile that stops naming a service
	// does not empty it.
	if (id == "act") return OpenPakURLs::ACTURL;
	if (id == "ecs") return OpenPakURLs::ECSURL;
	if (id == "nus") return OpenPakURLs::NUSURL;
	if (id == "ias") return OpenPakURLs::IASURL;
	if (id == "ccsu") return OpenPakURLs::CCSUURL;
	if (id == "ccs") return OpenPakURLs::CCSURL;
	if (id == "idbe") return OpenPakURLs::IDBEURL;
	if (id == "boss") return OpenPakURLs::BOSSURL;
	if (id == "tagaya") return OpenPakURLs::TAGAYAURL;
	if (id == "olv") return OpenPakURLs::OLVURL;
	return {};
}

std::string GetSource()
{
	std::lock_guard lock(g_mutex);
	return g_source;
}

int GetVersion()
{
	std::lock_guard lock(g_mutex);
	return g_version;
}

} // namespace OpenPakNetworkProfile
