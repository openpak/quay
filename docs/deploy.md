# Deploy: quay

## Downloads

No public build yet. There are no releases for Quay.

## Building from source

Quay has not been built. The tree still builds as Cemu, so upstream's
[BUILD.md](../BUILD.md) applies and nothing OpenPak-specific changes it:

```
git clone --recursive https://github.com/openpak/quay
cd quay
cmake -S . -B build -DCMAKE_BUILD_TYPE=release -G Ninja
cmake --build build
```

Expect the binary and window to say Cemu until Gate 0 lands. Commands above
were not run for this README.
