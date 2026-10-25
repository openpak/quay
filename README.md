# Quay Emulator

OpenPak's Quay Emulator.

| | |
| --- | --- |
| Console | Wii U |
| Kind | Emulator |
| Based on | [Cemu](https://github.com/cemu-project/Cemu) (MPL-2.0 as upstream) |
| Licence | MPL-2.0 (as upstream), see [LICENSE.txt](LICENSE.txt) |

## Build

```
git clone --recursive https://github.com/openpak/quay
cd quay
cmake -S . -B build -DCMAKE_BUILD_TYPE=release -G Ninja
cmake --build build
```

See [deploy](docs/deploy.md) for the full build notes.

## Docs

- [Protocol](docs/protocol.md)
- [Evidence](docs/evidence.md)
- [Deploy](docs/deploy.md)
- [Notes](docs/notes.md)
- [Upstream README](docs/upstream.md)

## Licence

Mozilla Public License 2.0, see [LICENSE.txt](LICENSE.txt).
