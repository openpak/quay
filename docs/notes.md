# Notes: quay

Quay is planned as OpenPak's Wii U emulator, forked from OpenPak's Cemu fork
(itself built on upstream Cemu). The goal is to boot the console's own firmware,
the real Wii U Menu and system applets, and send only the network side to
OpenPak's replacement services, the way OpenPak's other firmware-first
emulators do. Independent project; upstream Cemu remains our base. This is not
an official Cemu release.

## Status

**Status:** Early (Gate 0 landed; builds and runs games as Quay)

Read this before anything else: this repository is the renamed OpenPak Cemu
fork. The user-visible rename (Gate 0 in the plan) has landed — the project
builds a `Quay` binary, the app identity, desktop entry, installer and release
titles all say Quay — but no Quay-specific emulation exists yet. Quay is now
OpenPak's only Wii U emulator; the Cemu fork
([openpak/cemu](https://github.com/openpak/cemu)) is retired. Quay builds on
Linux and boots commercial games; no system title has booted yet (Gate 1
measurement needs an owner firmware dump).

## Not affiliated

This is an independent project. It is not endorsed by the Cemu project and is
not affiliated with Nintendo. Wii U and all related names are trademarks of
their owners.

## Plan

Planned, from the base plan: Gate 0 renames the project surface from Cemu to
Quay. Gate 1 builds it on Linux, measures which real system titles load on the
current high-level kernel and where they stop, and decides whether lower-level
emulation of the console's IO processor is needed. Cemu emulates the Wii U
operating system at the library level and has no IO processor core, no boot
chain and no system menu boot today.

## Connecting to OpenPak

The OpenPak connection inherited from the Cemu fork is in this tree but has not
been run as Quay. The plan is a
first-run flow that installs your own firmware dump, sets the games folder,
signs in at https://openpak.org, then launches the real Wii U Menu. You will
supply your own console dumps, firmware and games. OpenPak provides none and
never will.

## Known limitations

- Gate 0 renamed the user-visible surface only. Internal names stay Cemu's for
  now: `src/Cemu/`, the `Cemu*` library targets and classes, `cemuLog`, the
  `CEMU_*` macros, and the `src/resource/cemu.*` file names.
- The embedded and bundle icons are still Cemu's logo. The Quay artwork under
  `packaging/` is AGPL-3.0-only and is distributed separately, never embedded
  in the binary; Quay needs its own embeddable icon set before the Cemu logo
  can go.
- The Cemu self-updater is disabled: Help → Check for updates is off and the
  update query never runs, so Quay never phones the Cemu update feed. Quay
  releases come from this repository's releases page.
- Discord Rich Presence still uses upstream's Discord application id; it needs
  an OpenPak id before it reports Quay correctly.
- No firmware boot path exists: no Wii U Menu, no Mii Maker, no firmware
  account flow.
- Cemu-era limits carry over: Cloud saves and Mods pages are read-only, and the
  device identity is synthesised until firmware flow replaces it.
- No Android or other new platforms; those are later phases.

## Contributing and support

Issues: https://github.com/openpak/quay/issues. Security reports go privately
to the OpenPak maintainers through the contact listed on https://openpak.org.

See [CONTRIBUTING.md](../CONTRIBUTING.md) and [CODING_STYLE.md](../CODING_STYLE.md).
Upstream asks that submitted code be written and understood by a human; that
policy applies here too. At this stage the most useful help is feedback on the
plan and Wii U system title knowledge. Bug reports about current behaviour
belong here; the Cemu fork is retired.

## License and acknowledgements

Mozilla Public License 2.0, see [LICENSE.txt](../LICENSE.txt). Files under
`dependencies/` and a few files in `src/` keep their own licenses. The Quay
artwork under `packaging/` carries its own AGPL-3.0-only notice and is
distributed separately from the emulator binary.

Cemu is the work of the Cemu project and its contributors
(https://github.com/cemu-project/Cemu). We preserve their work and credit them
as our upstream.

The original upstream README is kept in [upstream.md](upstream.md).
