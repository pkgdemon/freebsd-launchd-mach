# ICUExport

A utility that upgrades the ICU version for the [swift-foundation-icu](https://github.com/swiftlang/swift-foundation-icu) repository.

## Usage

`icu-export` performs a merge between the existing `swift-foundation-icu` and the upgraded `ICU` repository to extract relevant files from each. To start, first clone the two repositories:

```bash
git clone https://github.com/apple-oss-distributions/ICU.git /path/to/ICU
git clone https://github.com/swiftlang/swift-foundation-icu.git /path/to/swift-foundation-icu
```

Run the utility:

```bash
swift run icu-export -i /path/to/ICU -s /path/to/swift-foundation-icu -o /output/path
```

`icu-export` will create a directory `SwiftFoundationICU` under `/output/path` that contains the newly updated ICU.

—
### Known Limitations

Depending on the changes made to each ICU upgrade, `icu-export` may fail to apply the patches in `Sources/Patches`. In such cases, please manually apply the changes.
