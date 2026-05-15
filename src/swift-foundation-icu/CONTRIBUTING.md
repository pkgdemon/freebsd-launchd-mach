# Contributing

The C++ code in this repository mirror a specific version of the [upstream Apple ICU repository](https://github.com/apple-oss-distributions/ICU). We do not accept changes to those files in this repository.

The Swift code is intended for use only by [Swift Foundation](https://github.com/swift/swift-foundation). The Swift API here is not intended to be a full Swift wrapper for ICU. Foundation is the preferred API to use for clients that wish to take advantage of ICU's functionality, as that API is available on all platforms including Darwin. Therefore, most pull requests should be targeted at Foundation and not at this repository.


## Bug reports

We are using [GitHub Issues](https://github.com/apple/swift-foundation-icu/issues) for tracking bugs, feature requests, and other work.
