# swift-foundation-icu (project notes)

Vendored copy of Apple's `swift-foundation-icu`, the ICU fork purpose-built
for swift-corelibs-foundation. NOT general-purpose ICU — see the upstream
[README.md](README.md) for that disclaimer.

## Provenance

- **Upstream**: [github.com/apple/swift-foundation-icu](https://github.com/apple/swift-foundation-icu)
- **Upstream HEAD**: `30f73e2` ("Update automerger to merge from release/6.4.x", PR #96)
- **ICU version**: 74.0 (per upstream README versioning matrix)
- **Vendored**: 2026-05-15
- **License**: Apache 2.0 (see `LICENSE.md`)

## Why this upstream

swift-corelibs-foundation's `libCoreFoundation` includes ICU headers via
the private namespace `<_foundation_unicode/uloc.h>` (11 files). That
namespace exists natively only in `apple/swift-foundation-icu` —
`apple-oss-distributions/ICU` (general-purpose Apple ICU) ships its
headers under the standard `<unicode/...>` prefix and would require a
symlink-alias hack at install time.

`apple/swift-foundation-icu` is a slimmer fork specifically packaged for
swift-corelibs-foundation:

- Headers shipped natively at `icuSources/include/_foundation_unicode/`
  (212 headers) — no install-time rename needed
- Apache 2.0 license
- CMake build (matches our existing libdispatch build pattern in
  `build.sh`)
- Trimmed for SwiftPM: no `test/`, `samples/`, `layoutex/`,
  `ICU.xcodeproj/` subtrees

Decision recorded in
[pkgdemon.github.io/freebsd-libicu-port-plan.html §0](https://pkgdemon.github.io/freebsd-libicu-port-plan.html#vendor-pivot).

## Repo bloat

The vendor includes 4 auto-generated hex-encoded data chunks at
`icuSources/common/icu_packaged_main_data.{0,1,2,3}.inc.h`, ~50 MB each
(200 MB total). These are pre-compiled CLDR data; the compiler emits
them as binary into `libicucore.so`'s data section. They never ship on
the installed ISO.

Trade: ~60–100 MB permanent repo growth (after git pack), 4 GitHub
50-MB push warnings (well under the 100-MB hard limit). Honors the
project's vendor-everything pattern.

## Installed footprint

| Layer | Size | Location |
|---|---|---|
| Library | ~40–50 MB | `/usr/lib/system/libicucore.so` (or whatever the CMake target produces) |
| Headers | ~5.5 MB | `/usr/include/_foundation_unicode/` (build-tools only, runtime cost zero) |

Roughly the same as macOS's `/usr/lib/libicucore.dylib`.

## Build

In the FreeBSD chroot via `build.sh`'s libicu step (lands as task ICU.3):

```sh
cmake -G Ninja /tmp/swift-foundation-icu/icuSources \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DCMAKE_INSTALL_LIBDIR=lib/system \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_FLAGS="-DU_DISABLE_RENAMING=1" \
    -DCMAKE_CXX_FLAGS="-DU_DISABLE_RENAMING=1"
ninja
ninja install
```

Anticipate 2-4 fixup commits patching `icuSources/CMakeLists.txt` for
FreeBSD-specific overrides (`MAC_OS_X_VERSION_MIN_REQUIRED` removal,
`U_HAVE_XLOCALE_H` verification). Patches recorded in this repo's git
history; flagged with `freebsd-launchd-mach patch (date):` comments per
project convention.

## Build status

🚧 Vendoring complete (task ICU.2c). Build pipeline lands in task ICU.3.
