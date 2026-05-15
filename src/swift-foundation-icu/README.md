# Foundation ICU

This version of the [ICU4C](https://icu.unicode.org/) project contains customized extensions for use by the [Foundation package](https://github.com/apple/swift-foundation). It is automatically extracted from [Apple OSS Distribution's ICU](https://github.com/apple-oss-distributions/ICU) to add Swift Package Manager support. Improvements to ICU's core functionality should be proposed to the upstream ICU4C library and not to this package.

- Sources: https://github.com/apple/swift-foundation-icu
- API Docs: https://unicode-org.github.io/icu/userguide/icu4c/

## Versioning

See the following version matrix:

| `FoundationICU` version | `ICU` version |
| --- | --- |
| `0.0.2` and below | `70.1` |
| `0.0.3` ~ `0.0.9`  | `72.1` |
| `0.0.10` and above | `74.0` |


## Adding FoundationICU as a Dependency

> :warning: This package is intended to be a dependency for the [Foundation package](https://github.com/apple/swift-foundation). It is not useful as a "general purpose" ICU4C library because all files irrelevant to the SwiftPM build are removed. The package is considered a private implementation detail of Foundation, and its API surface and structure is likely to change between major versions.

To use the `FoundationICU` library in a SwiftPM project, add the following lines to the dependencies in your `Package.swift` file:

```swift
.package(url: "https://github.com/apple/swift-foundation-icu", from: "0.0.3"),
```

Include `"FoundationICU"` as a dependency for your target:

```swift
.target(name: "<target>", dependencies: [
    .product(name: "FoundationICU", package: "swift-foundation-icu"),
]),
```

Finally, add `import FoundationICU` to your source code. You should now be able to directly use/extend ICU types:

```swift
import FoundationICU

extension UCalendarAttribute {
    static let lenient = UCAL_LENIENT
    static let firstDayOfWeek = UCAL_FIRST_DAY_OF_WEEK
    static let minimalDaysInFirstWeek = UCAL_MINIMAL_DAYS_IN_FIRST_WEEK
}
```

## Future Improvements

- **Data file handling**: currently, the data file is embedded into the binary itself as `[uint8_t]` (see `icu_packaged_data.h`). In the future, we would like to check in the source files instead and build the data as a shared library to avoid the need to maintain and load a separate data file.
