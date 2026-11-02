# Deploy: quay

## Downloads

No public build yet. There are no releases for Quay.

## Building from source

Quay has not been built yet. Gate 0 has landed, so the tree builds a `Quay`
binary; [BUILD.md](../BUILD.md) applies with the Quay names:

```
git clone --recursive https://github.com/openpak/quay
cd quay
cmake -S . -B build -DCMAKE_BUILD_TYPE=release -G Ninja
cmake --build build
```

The binary lands at `bin/Quay_release`. Built and run on Linux 2026-10-27
(see [evidence](evidence.md)); limit build parallelism on smaller machines
(full `-j18` on 16 cores / 29 GB RAM OOM-kills PCH compiles — `-j6` works).
