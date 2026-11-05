# Changelog — Quay (OpenPak fork)

Only OpenPak's changes are listed; everything else is upstream. `git log` stays the source of
truth. Tags are `vX.Y.Z`; `v*.*.*` publishes a GitHub Release (`nightly` is a rolling pre-release).

## Unreleased

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
