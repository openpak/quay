#pragma once

#include "input/api/Keyboard/KeyboardControllerProvider.h"
#include "input/api/Controller.h"

#include <utility>
#include <vector>

// Default keyboard layout for the Wii U GamePad and Pro Controller (both enums share these names).
// Key codes are what the window reports: GDK keysyms on Linux, virtual-key codes on Windows.
// ponytail: no macOS table (raw key codes there are kVK_*); bind by hand until someone needs it.
template<typename T>
std::vector<std::pair<uint64, uint64>> keyboard_default_mapping()
{
#if BOOST_OS_LINUX || BOOST_OS_BSD
	constexpr uint64 kEnter = 0xff0d, kBack = 0xff08, kLeft = 0xff51, kUp = 0xff52, kRight = 0xff53, kDown = 0xff54;
	auto ch = [](char c) -> uint64 { return (uint64)c; }; // lowercase keysym == ASCII
#elif BOOST_OS_WINDOWS
	constexpr uint64 kEnter = 0x0d, kBack = 0x08, kLeft = 0x25, kUp = 0x26, kRight = 0x27, kDown = 0x28;
	auto ch = [](char c) -> uint64 { return (uint64)(c - 'a' + 'A'); }; // VK letters are uppercase ASCII
#else
	return {};
#endif
	return {
		// face buttons: the IJKL diamond (X up, B down, Y left, A right)
		{T::kButtonId_A, ch('l')}, {T::kButtonId_B, ch('k')}, {T::kButtonId_X, ch('i')}, {T::kButtonId_Y, ch('j')},
		{T::kButtonId_L, ch('q')}, {T::kButtonId_R, ch('e')}, {T::kButtonId_ZL, ch('z')}, {T::kButtonId_ZR, ch('c')},
		{T::kButtonId_Plus, kEnter}, {T::kButtonId_Minus, kBack}, {T::kButtonId_Home, ch('o')},
		{T::kButtonId_Up, kUp}, {T::kButtonId_Down, kDown}, {T::kButtonId_Left, kLeft}, {T::kButtonId_Right, kRight},
		{T::kButtonId_StickL, ch('r')}, {T::kButtonId_StickR, ch('y')},
		{T::kButtonId_StickL_Up, ch('w')}, {T::kButtonId_StickL_Down, ch('s')}, {T::kButtonId_StickL_Left, ch('a')}, {T::kButtonId_StickL_Right, ch('d')},
		{T::kButtonId_StickR_Up, ch('t')}, {T::kButtonId_StickR_Down, ch('g')}, {T::kButtonId_StickR_Left, ch('f')}, {T::kButtonId_StickR_Right, ch('h')},
	};
}

class KeyboardController : public Controller<KeyboardControllerProvider>
{
public:
	KeyboardController();

	std::string_view api_name() const override // TODO: use constexpr virtual function with c++20
	{
		static_assert(to_string(InputAPI::Keyboard) == "Keyboard");
		return to_string(InputAPI::Keyboard);
	}
	InputAPI::Type api() const override { return InputAPI::Keyboard; }

	bool is_connected() override { return true; }

	bool has_axis() const override { return false; }

	std::string get_button_name(uint64 button) const override;

protected:
	ControllerState raw_state() override;

};
