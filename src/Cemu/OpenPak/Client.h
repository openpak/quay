#pragma once

// OpenPak: the shared client library (github.com/openpak/openpak-client, dependencies/openpak-client)
// does the website API, the account file, the network profile and the signed ceiling for every
// OpenPak emulator. Quay keeps what is Wii U's alone: the identity it writes into the emulated
// console (Account), the synthesised device identity (DeviceIdentity) and its wx UI.
namespace OpenPakClient
{
	// Sets the library up once: Quay's config directory (the account file, the cached profile and
	// ceiling), the client name for X-OpenPak-Client, the log sink, the Website setting and the
	// device name. Idempotent; every OpenPak entry point calls it before its first request.
	void Init();
}
