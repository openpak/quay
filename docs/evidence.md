# Evidence: quay

## What works today

- Gate 0 (user-visible Cemu → Quay rename) has landed: `Quay` project and
  binary, `Quay` app identity and user-data directories, `org.openpak.quay`
  desktop and app ids, `X-OpenPak-Client: quay/…`, the Quay installer, and
  Quay release titles. The Cemu self-updater is disabled.
- Inherited from the Cemu fork, unchanged in behaviour: the OpenPak Network
  Service choice, network profile with signed redirect ceiling, OpenPak
  sign-in with Wii U identity install, and the OpenPak menu and window. These
  are described in `OPENPAK.md`.
- Verified 2026-10-27: Quay builds on Linux (`bin/Quay_release`, 49.7 MB
  ELF), launches to the first-run assistant with Quay strings, and boots a
  commercial title (New Super Mario Bros. U, `00050000-10101e00`) to in-game
  dialogs at 60 FPS on Vulkan. Window title `Quay <hash>`, Wayland app id
  `org.openpak.quay`, fresh user data in `~/.local/share/Quay`. Screenshots in
  the 2026-10-27 session notes.
- Fixed 2026-10-27 (uncommitted): `nex_token` hosts longer than 15 chars no
  longer truncate silently — Quay resolves them to IPv4 literals (console
  ABI keeps the field at 16 bytes) or fails the token loudly, as do
  over-long passwords/tokens. Splatoon (`00050000-10176a00`, Europe) boots
  to character select under the same build.
- Fixed 2026-10-27 (uncommitted): on the OpenPak service, DNS names in
  `nex_token` responses must sit inside the platform's signed ceiling
  (checked before resolving); IP literals pass (today's rows serve them)
  and other services keep their own trust. Semantics pinned by probe
  against the built library; NSMBU regression boot green.
- Fixed 2026-10-27 (uncommitted): missing HLE exports no longer vanish —
  called-but-unimplemented imports log `Unsupported lib call: lib.func`
  once on every CPU path (recompilers used to swallow them; the
  interpreter needed a flag), and debug builds list all missing imports
  at link time. NSMBU shows 33 called vs 92 imported-never-called; the
  menu inventory greps for exactly this.
- Not verified: no system title (Wii U Menu, Mii Maker) has booted — no owner
  firmware dump exists locally yet. The OpenPak Wii U servers have never been
  run from a console or an emulated game (the launch-time profile/ceiling
  fetch does succeed: ceiling v16, wiiu profile v1), so online play is not
  promised.
