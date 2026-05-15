// swift-tools-version: 5.7
// The swift-tools-version declares the minimum version of Swift required to build this package.

import PackageDescription

var buildSettings: [CXXSetting] = [
  .define("DEBUG", to: "1", .when(configuration: .debug)),
  .define("U_ATTRIBUTE_DEPRECATED", to: ""),
  .define("U_SHOW_CPLUSPLUS_API", to: "1"),
  .define("U_SHOW_INTERNAL_API", to: "1"),
  .define("U_STATIC_IMPLEMENTATION"),
  .define("U_TIMEZONE", to: "_timezone", .when(platforms: [.windows])),
  .define(
    "U_TIMEZONE", to: "timezone",
    .when(platforms: [
      .iOS,
      .macOS,
      .tvOS,
      .watchOS,
      .macCatalyst,
      .driverKit,
      .android,
      .linux,
    ])),
  .define("U_TIMEZONE_PACKAGE", to: "\"icutz44l\""),
  .define("U_HAVE_TZSET", to: "0", .when(platforms: [.wasi])),
  .define("U_HAVE_TZNAME", to: "0", .when(platforms: [.wasi])),
  .define("U_HAVE_TIMEZONE", to: "0", .when(platforms: [.wasi])),
  .define("FORTIFY_SOURCE", to: "2"),
  .define("STD_INSPIRED"),
  .define("MAC_OS_X_VERSION_MIN_REQUIRED", to: "101500"),
  .define("_WASI_EMULATED_SIGNAL", .when(platforms: [.wasi])),
  .define("_WASI_EMULATED_MMAN", .when(platforms: [.wasi])),
  .define("U_HAVE_STRTOD_L", to: "1"),
  .define("U_HAVE_XLOCALE_H", to: "1"),
  .define("U_HAVE_STRING_VIEW", to: "1"),
  .define(
    "U_DISABLE_RENAMING", to: "1",
    .when(platforms: [
      .macOS,
      .iOS,
      .tvOS,
      .watchOS,
      .visionOS,
    ])),
  .define(
    "U_DISABLE_RENAMING", to: "0",
    .when(platforms: [
      .linux,
      .windows,
      .android,
      .wasi,
    ])),
  .define("U_COMBINED_IMPLEMENTATION"),
  .define("U_COMMON_IMPLEMENTATION"),
  .define("U_I18N_IMPLEMENTATION"),
  .define("U_IO_IMPLEMENTATION"),
  .define("HAVE_DLFCN_H", to: "0", .when(platforms: [.wasi])),
  .define("HAVE_DLOPEN", to: "0", .when(platforms: [.wasi])),
  .define("U_ENABLE_DYLOAD", to: "0", .when(platforms: [.wasi])),

  // Where data are stored
  .define("ICU_DATA_DIR", to: "\"/usr/share/icu/\""),
  .define("USE_PACKAGE_DATA", to: "1"),
  .define("APPLE_ICU_CHANGES", to: "1"),
  .define("UCHAR_TYPE", to: "char16_t"),
  .define("U_PLATFORM_IS_DARWIN_BASED", to: "0"),

  .headerSearchPath("common"),
  .headerSearchPath("io"),
  .headerSearchPath("i18n"),
  .define("SWIFT_PACKAGE", to: "1", .when(platforms: [.linux])),
]

#if os(Windows)
  buildSettings.append(contentsOf: [
    .define("_CRT_SECURE_NO_DEPRECATE"),
    .define("U_PLATFORM_USES_ONLY_WIN32_API"),
  ])
#endif

let linkerSettings: [LinkerSetting] = [
  .linkedLibrary("wasi-emulated-signal", .when(platforms: [.wasi])),
  .linkedLibrary("wasi-emulated-mman", .when(platforms: [.wasi])),
]

let package = Package(
  name: "FoundationICU",
  products: [
    .library(
      name: "_FoundationICU",
      targets: [
        "_FoundationICU"
      ]
    )
  ],
  targets: [
    .target(
      name: "_FoundationICU",
      path: "icuSources",
      exclude: ["stubdata"],
      cxxSettings: buildSettings,
      linkerSettings: linkerSettings
    )
  ],
  cxxLanguageStandard: .cxx17
)
