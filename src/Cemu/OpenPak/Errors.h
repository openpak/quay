#pragma once

#include <string>

#include <openpak/api.h>

// OpenPak: the errors the account and social calls return. They are codes, not sentences: the
// UI turns each into its error.* string (emulators/prds/openpak-ux-spec.md §3.12, §7.2), shows a
// server's own sentence only behind "OpenPak said: {0}", and keeps HTTP codes and transport
// texts in the log. Anything that is not a code is an internal English message.
namespace OpenPakError
{
	inline const std::string Credentials = "@credentials";
	inline const std::string RateLimited = "@rate_limited";
	inline const std::string Unreachable = "@unreachable"; // transport failure or no usable answer
	inline const std::string Expired = "@expired";         // the stored token was refused (401)
	inline const std::string NotSignedIn = "@not_signed_in";
	inline const std::string NoIdentity = "@no_identity";  // the account has no Wii U identity
	inline const std::string ServerPrefix = "@server:";    // followed by the server's sentence

	inline std::string Server(const std::string& sentence) { return ServerPrefix + sentence; }
	inline bool IsCode(const std::string& error) { return !error.empty() && error[0] == '@'; }

	// The client library's sentences for the failures the UI words itself, as codes; anything
	// else it answers is the website's own sentence (or a fallback naming the HTTP status).
	inline std::string FromLibrary(const std::string& error)
	{
		namespace E = WebService::OpenPakApi::Errors;
		if (error.empty())
			return {};
		if (error == E::Credentials)
			return Credentials;
		if (error == E::RateLimited)
			return RateLimited;
		if (error == E::Unreachable)
			return Unreachable;
		if (error == E::SessionExpired)
			return Expired;
		if (error == E::NotSignedIn)
			return NotSignedIn;
		return Server(error);
	}
}
