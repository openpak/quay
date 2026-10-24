#include "Cemu/OpenPak/Client.h"

#include "Cemu/OpenPak/Prefs.h"
#include "Cemu/Logging/CemuLogging.h"
#include "Common/version.h"
#include "config/ActiveSettings.h"

#include <openpak/api.h>
#include <openpak/log.h>
#include <openpak/platform.h>

#include <mutex>

namespace OpenPakClient
{
	void Init()
	{
		static std::once_flag once;
		std::call_once(once, []() {
			openpak::SetLogSink([](openpak::LogLevel level, const std::string& message) {
				if (level == openpak::LogLevel::Trace || level == openpak::LogLevel::Debug)
					return;
				cemuLog_log(LogType::Force, "OpenPak: {}", message);
			});
			openpak::Platform::SetDirectories(ActiveSettings::GetConfigPath(), ActiveSettings::GetCachePath("openpak"));
			openpak::Platform::SetClient("quay", BUILD_VERSION_STRING);
			// The library reads the website once, before its first request.
			OpenPakPrefs::ExportWebsite();
			WebService::OpenPakApi::SetSavesPlatform("wiiu");
			if (const std::string device = OpenPakPrefs::DeviceName(); !device.empty())
			{
				WebService::OpenPakApi::SetDeviceName(device);
				WebService::OpenPakApi::SetSaveDevice(device);
			}
		});
	}
}
