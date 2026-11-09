# Changelog — Quay (OpenPak fork)

Only OpenPak's changes are listed; everything else is upstream. `git log` stays the source of
truth. Tags are `vX.Y.Z`; `v*.*.*` publishes a GitHub Release (`nightly` is a rolling pre-release).

## v0.4.6 — 2026-11-09

- Input: the keyboard now has a default layout for the Wii U GamePad and the Pro Controller (`keyboard_default_mapping`): WASD left stick, TFGH right stick, arrow keys D-pad, IJKL as X/B/Y/A (I up, K down, J left, L right), Q/E L/R, Z/C ZL/ZR, Return Plus, Backspace Minus, O Home, R/Y stick clicks (Linux and Windows key codes; none on macOS). Upstream's `set_default_mapping` had no Keyboard case, so a keyboard added to a controller stayed unbound and games ignored it
- A saved keyboard controller with no mappings at all gets that layout when the profile loads, so a profile created by opening the Input settings and never mapping anything now works

## v0.4.5 — 2026-11-07

- nn_act: `Finalize` implemented (drops the Initialize reference count; it was an unsupported call returning 0)
- nn_ec: the `ShoppingCatalog`, `Query` and `DateTime` constructors and the first two destructors return their this pointer instead of 0, so a title that builds them no longer dereferences null
- Minecraft: Wii U Edition needs the Mii resource files (`FFLResMiddle.dat` and `FFLResHigh.dat` of system title 0005001b10056000) under `mlc01/sys/title/0005001b/10056000/content`. Without them its FFL init fails and the game dereferences null right after init (the crash seen after `nn_act.Finalize`)

## v0.4.4 — 2026-11-07

- OpenPak sign-in: account.dat MiiName is written as the 11 UTF-16 units Account::ParseFile requires, and ASCII is decoded with 7 bits; before, every signed-in OpenPak account failed to load and Quay silently ran the offline first account (no NEX traffic), with a garbled Mii name. Accounts signed in with an earlier build: sign in again (or fix the MiiName line in account.dat)
- imgui: advertise a gamepad to ImGui (`ImGuiBackendFlags_HasGamepad`) and clear the legacy NavInputs each frame; ImGui 1.89 ignored controller navigation without the flag, so the software keyboard could not be used with a controller

## v0.4.3 — 2026-11-04

- Cemu rebranded to Quay across CI, build, packaging, GUI strings and docs; the AppImage and
  macOS packaging steps now use the renamed `org.openpak.quay.*` and `quay.macos.entitlements`
  files (the rebrand left both pointing at the old names, so the nightly packaging failed)
- napi_act: the nex_token host resolves to IPv4; over-long host, password or token refused
- Unsupported HLE calls logged once per export on every CPU path
- openpak-client bumped to 056de92 (shared library; Account.h became WiiUAccount.h)
- Nightly builds that run only when the source changed; upstream sync of 2026-10-24

## v0.4.0 to v0.4.2 — 2026-09-23 to 2026-09-26

- Network Service choice OpenPak next to Nintendo, Pretendo and Custom; network profile fetch
  at launch; Wii U family UX
