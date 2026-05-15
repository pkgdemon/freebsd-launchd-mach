//===----------------------------------------------------------------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2025 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

import Foundation
import System
import ArgumentParser
import Darwin
import RegexBuilder
import Subprocess

@available(macOS 10.15, *)
struct ICUExport : AsyncParsableCommand {

    @Option(
        name: .shortAndLong,
        completion: .file(),
        transform: { return FilePath($0) }
    )
    var icuDirectory: FilePath

    @Option(
        name: .shortAndLong,
        completion: .file(),
        transform: { return FilePath($0) }
    )
    var swiftFoundationICUDirectory: FilePath

    @Option(
        name: .shortAndLong,
        completion: .file(),
        transform: { return FilePath($0) }
    )
    var outputDirectory: FilePath

    private var skipList: Set<String> {
        return Set([".git", ".DS_Store", "icuSources", "Package.resolved", ""])
    }

    private var numberOfDataChunks = 4

    func run() async throws {
        try await self.buildICUDataFile()
        try await self.copySourceFiles()
        try await self.generatePackagedData()
        try await self.updateImportStatements()
        try processRenamingHeader()
        try await self.applyPatches()
        print("✅ Export finished")
    }

    var outputHeadersDst: FilePath {
        outputDirectory.appending("SwiftFoundationICU").appending("icuSources").appending("include").appending("_foundation_unicode")
    }
}

// MARK: - Steps
extension ICUExport {
    private func buildICUDataFile() async throws {
        print("[STEP ⏳]: Build ICU Data File")
        guard icuDirectory.appending("build").isEmpty else {
            print("[STEP ✅]: Found file")
            return
        }
        let result = try await Subprocess.run(
            .name("make"),
            arguments: ["CPPFLAGS=\"-DU_PLATFORM_IS_DARWIN_BASED=1 -DU_DISABLE_RENAMING=1\""],
            workingDirectory: icuDirectory,
            output: .standardOutput,
            error: .standardError
        )

        guard result.terminationStatus.isSuccess else {
            throw Error.commandFailed("make")
        }
        print("[STEP ✅]: Build ICU Data File")
    }

    private func copySourceFiles() async throws {
        print("[STEP ⏳]: Copy Source Files")
        let outputPath = self.outputDirectory.appending("SwiftFoundationICU")
        if FileManager.default.fileExists(atPath: outputPath.string) {
            try FileManager.default.removeItem(atPath: outputPath.string)
        }
        try FileManager.default.createDirectory(
            atPath: outputPath.string,
            withIntermediateDirectories: true
        )
        // Init git so we can apply patches later
        let gitInit = try await Subprocess.run(
            .name("git"),
            arguments: ["init"],
            workingDirectory: outputPath,
            output: .discarded
        )
        guard gitInit.terminationStatus.isSuccess else {
            throw Error.commandFailed("git init @ \(outputPath)")
        }
        // Copy over top level items such as `Package.swift`
        print("Copying metadata...")
        let topLevelContents = try FileManager.default.contentsOfDirectory(
            atPath: self.swiftFoundationICUDirectory.string)
        for item in topLevelContents {
            guard !self.skipList.contains(item) else {
                continue
            }
            let sourcePath = self.swiftFoundationICUDirectory.appending(item)
            let destPath = outputPath.appending(item)
            try FileManager.default.copyItem(
                atPath: sourcePath.string,
                toPath: destPath.string
            )
        }
        // Now start copying actual sources from ICU
        let swiftFoundationICUSrc = self.swiftFoundationICUDirectory.appending("icuSources")
        let icuSrc = self.icuDirectory.appending("icu").appending("icu4c").appending("source")
        let icuSourcesDst = outputPath.appending("icuSources")
        try self.createDirectory(icuSourcesDst)
        // Copy CMakeList.txt
        try FileManager.default.copyItem(
            atPath: swiftFoundationICUSrc.appending("CMakeLists.txt").string,
            toPath: icuSourcesDst.appending("CMakeLists.txt").string
        )
        let headersDst = icuSourcesDst
            .appending("include")
            .appending("_foundation_unicode")
        try self.createDirectory(headersDst)

        func copyICUComponent(
            _ name: String,
            copyHeaders: Bool = true,
            extraCMakeEntry: Set<String> = [],
            excludeFiles: Set<String> = [],
            excludeCMakeFile: Bool = false
        ) throws {
            print("Copying \(name) sources")
            let componentSrc = icuSrc.appending(name)
            let componentDst = icuSourcesDst.appending(name)
            print("copying from \(componentSrc) into \(componentDst)")

            do {
                try self.createDirectory(componentDst)
            } catch let e {
                throw Error.fileManagerFailed(e.localizedDescription)
            }

            let componentContents: [String]
            do {
               componentContents = try FileManager.default.contentsOfDirectory(
                    atPath: componentSrc.string
                )
            } catch let e {
                throw Error.fileManagerFailed("cannot find contents at directory: \(e)")
            }

            var allSources: Set<String> = Set()
            for item in componentContents {
                guard !self.skipList.contains(item) else {
                    continue
                }
                guard !excludeFiles.contains(item) else {
                    continue
                }
                guard item.hasSuffix(".cpp") || item.hasSuffix(".h") else {
                    continue
                }
                if item.hasSuffix(".cpp") {
                    allSources.insert(item)
                }
                let sourcePath = componentSrc.appending(item)
                let destPath = componentDst.appending(item)
                do {
                    try FileManager.default.copyItem(
                        atPath: sourcePath.string,
                        toPath: destPath.string
                    )
                } catch let e {
                    throw Error.fileManagerFailed("cannot copy \(sourcePath) to \(destPath): \(e.localizedDescription)")
                }
            }

            if copyHeaders {
                print("Copying \(name) headers")
                let componentHeadersSrc = componentSrc.appending("unicode")
                print("looking for headers at \(componentHeadersSrc)")
                let componentHeaders = try FileManager.default.contentsOfDirectory(
                    atPath: componentHeadersSrc.string
                )
                for commonHeader in componentHeaders {
                    guard commonHeader.hasSuffix(".h") else {
                        continue
                    }
                    let sourcePath = componentHeadersSrc.appending(commonHeader)
                    let destPath = headersDst.appending(commonHeader)
                    try FileManager.default.copyItem(
                        atPath: sourcePath.string,
                        toPath: destPath.string
                    )
                }
            }

            // Create CMakeLists.txt
            guard !excludeCMakeFile else {
                return
            }

            if !extraCMakeEntry.isEmpty {
                allSources = allSources.union(extraCMakeEntry)
            }
            if !excludeFiles.isEmpty {
                allSources = allSources.subtracting(excludeFiles)
            }
            let cmakeBody = allSources.sorted().joined(separator: "\n        ")
            let cmake = "\(cmakeListsTemplate)\n        \(cmakeBody))\n"
            let cmakeData = Data(cmake.utf8)
            let componentCMakeListsDst = componentDst.appending("CMakeLists.txt")
            print("Writing CMakeList.txt to \(URL(fileURLWithPath: componentCMakeListsDst.string))")
            try cmakeData.write(to: URL(fileURLWithPath: componentCMakeListsDst.string))
        }
        // Copy Common
        try copyICUComponent(
            "common",
            copyHeaders: true,
            extraCMakeEntry: ["icu_packaged_data.cpp"],
            excludeFiles: ["icuuc40shim.cpp"]
        )
        try copyICUComponent("i18n", copyHeaders: true)
        try copyICUComponent("io", copyHeaders: true)
        try copyICUComponent("stubdata", copyHeaders: false, excludeCMakeFile: true)

        // Generate module.modulemap
        let moduleMapDst = headersDst.appending("module.modulemap")
        let allPublicHeaders = try FileManager.default
            .contentsOfDirectory(atPath: headersDst.string)
            .filter { $0.hasSuffix(".h") }
            .map { "header \"\($0)\"" }
            .joined(separator: "\n    ")
        let moduleMapContent = "module _FoundationICU {\n    \(allPublicHeaders)\n\n    export *\n}\n"
        let moduelMap = Data(moduleMapContent.utf8)
        try moduelMap.write(to: URL(filePath: moduleMapDst.string), options: .atomic)
        print("Generated \(moduleMapDst)")

        print("[STEP ✅]: Copy Source Files")
    }

    private func generatePackagedData() async throws {
        let dataFileDirectory: FilePath = self.icuDirectory
            .appending("build")
            .appending("icuhost")
            .appending("data")
            .appending("out")
        let destinationDirectory: FilePath = self.outputDirectory
            .appending("SwiftFoundationICU")
            .appending("icuSources")
            .appending("common")
        let contents = try FileManager.default.contentsOfDirectory(
            atPath: dataFileDirectory.string
        )
        let dataFile = contents.first { $0.hasSuffix(".dat") }
        guard let dataFile = dataFile else {
            throw Error.dataFileMissing
        }
        let dataFilePath = dataFileDirectory.appending(dataFile)
        print("Found data file: \(dataFilePath)")
        let data = try Data(contentsOf: URL(filePath: dataFilePath.string))
        let chunkSize = data.count / self.numberOfDataChunks
        var chunkContent = ""
        var index = 0
        var chunkIndex = 0
        chunkContent.reserveCapacity(chunkSize * 6)
        for byte: UInt8 in data {
            if index != 0 && index % 32 == 0 {
                // Add new line
                chunkContent += "\\\n    "
            }
            chunkContent += "0x\(String(format: "%02X", byte)), "
            index += 1

            if index >= chunkSize {
                // Write the current chunk to disk
                let chunkFileName = destinationDirectory
                    .appending(dataFileNameTemplate
                        .replacingOccurrences(
                            of: dataFileIndexMark,
                            with: "\(chunkIndex)"
                        )
                    )
                // Remove the trailing ", "
                print("About to remove trailing")
                chunkContent.removeLast(2)
                let fileContent = dataFileTemplate
                    .replacingOccurrences(of: dataFileIndexMark, with: "\(chunkIndex)")
                    .replacingOccurrences(of: dataFileContentMark, with: chunkContent)
                try fileContent.write(
                    to: URL(filePath: chunkFileName.string),
                    atomically: true,
                    encoding: .utf8
                )
                print("Generated \(chunkFileName)")

                chunkContent = ""
                chunkIndex += 1
                index = 0
            }
        }
        // Now copy over the reset of data file from SwiftFoundationICU
        // Timezone
        let swiftFoundationICUSrc = self.swiftFoundationICUDirectory
            .appending("icuSources")
            .appending("common")
        for timezoneIndex in 0 ..< self.numberOfDataChunks {
            let dataFileName = timezoneFileNameTemplate
                .replacingOccurrences(of: dataFileIndexMark, with: "\(timezoneIndex)")
            try FileManager.default.copyItem(
                atPath: swiftFoundationICUSrc.appending(dataFileName).string,
                toPath: destinationDirectory.appending(dataFileName).string
            )
            print("Copied \(dataFileName)")
        }
        // Copy icu_packaged_data.h
        try FileManager.default.copyItem(
            atPath: swiftFoundationICUSrc.appending("icu_packaged_data.h").string,
            toPath: destinationDirectory.appending("icu_packaged_data.h").string
        )
        print("Copied icu_packaged_data.h")
        // Copy icu_packaged_data.cpp
        try FileManager.default.copyItem(
            atPath: swiftFoundationICUSrc.appending("icu_packaged_data.cpp").string,
            toPath: destinationDirectory.appending("icu_packaged_data.cpp").string
        )
        print("Copied icu_packaged_data.cpp")
    }

    private func updateImportStatements() async throws {
        print("[STEP ⏳]: Update import statements")
        func isPathDirectory(_ path: FilePath) -> Bool {
            return path.string.withCString {
                var fileInfo = stat()
                guard stat($0, &fileInfo) == 0 else {
                    return false
                }
                let isDir = (fileInfo.st_mode & S_IFMT) == S_IFDIR
                return isDir
            }
        }

        func recursiveIterate(at path: FilePath, work: (FilePath) throws -> Void) throws {
            let contents = try FileManager.default.contentsOfDirectory(atPath: path.string)
            for item in contents {
                let fullFilePath = path.appending(item)
                if isPathDirectory(fullFilePath) {
                    try recursiveIterate(at: fullFilePath, work: work)
                } else {
                    try work(fullFilePath)
                }
            }
        }

        // Iterate through all .h and .cpp files and change
        // #include "unicode/header.h" to
        // #include <_foundation_unicode/header.h>
        let sourceDirectory = self.outputDirectory
            .appending("SwiftFoundationICU")
            .appending("icuSources")
        try recursiveIterate(at: sourceDirectory) {
            guard $0.string.hasSuffix(".h") || $0.string.hasSuffix(".cpp") else {
                return
            }
            guard !$0.string.contains(".inc.") else {
                return
            }
            let contents = try Data(contentsOf: URL(filePath: $0.string))
            let string = String(data: contents, encoding: .utf8)!
            let regex = Regex {
                "\"unicode/"
                Capture(OneOrMore(.anyNonNewline))
                "\""
            }
            var result = string.replacing(regex) { match in
                return "<_foundation_unicode/\(match.output.1)>"
            }
            let regexAngleBracket = Regex {
                "<unicode/"
                Capture(OneOrMore(.anyNonNewline))
                ">"
            }
            result = result.replacing(regexAngleBracket) { match in
                return "<_foundation_unicode/\(match.output.1)>"
            }
            // Write back to disk
            try Data(result.utf8).write(to: URL(filePath: $0.string), options: .atomic)
        }
        print("[STEP ✅]: Update import statements")
    }

    private func applyPatches() async throws {
        print("[STEP ⏳]: Applying patches")
        let workingDirectory = self.outputDirectory
            .appending("SwiftFoundationICU")
        let putilPatch = Bundle.module.path(
            forResource: "putil", ofType: "patch"
        )!
        let namespacePatch = Bundle.module.path(
            forResource: "namespace", ofType: "patch"
        )!
        // Before applying, commit everything we have so far
        print("Commiting all current changes before applying...")
        let addAll = try await Subprocess.run(
            .name("git"),
            arguments: ["add", "*"],
            workingDirectory: workingDirectory,
            output: .standardOutput,
            error: .standardError
        )
        guard addAll.terminationStatus.isSuccess else {
            throw Error.commandFailed("git add * @ \(workingDirectory)")
        }
        let commit = try await Subprocess.run(
            .name("git"),
            arguments: ["commit", "-m", "Initial import"],
            workingDirectory: workingDirectory,
            input: .standardInput,
            output: .standardOutput,
            error: .standardError
        )
        guard commit.terminationStatus.isSuccess else {
            throw Error.commandFailed("git commit @ \(workingDirectory)")
        }

        print("Applying patch: \(putilPatch)")
        let putilResult = try await Subprocess.run(
            .name("git"),
            arguments: ["am", "--empty=drop", putilPatch],
            workingDirectory: workingDirectory,
            output: .standardOutput,
            error: .standardError
        )
        guard putilResult.terminationStatus.isSuccess else {
            throw Error.commandFailed("git am --empty=drop \(putilPatch)")
        }

        print("Applying patch: \(namespacePatch)")
        let namespaceResult = try await Subprocess.run(
            .name("git"),
            arguments: ["am", "--empty=drop", namespacePatch],
            workingDirectory: workingDirectory,
            output: .standardOutput,
            error: .standardError
        )
        guard namespaceResult.terminationStatus.isSuccess else {
            throw Error.commandFailed("git am --empty=drop \(namespacePatch)")
        }
        print("[STEP ✅]: Applying patches")
    }

    // Process urename.h to expand renaming macro: https://github.com/swiftlang/swift-foundation-icu/pull/63
    private func processRenamingHeader() throws {
        print("[STEP ⏳]: Process renaming header")

        let renameHeaderPath = outputHeadersDst.appending("urename.h")

        let contents = try String(contentsOfFile: renameHeaderPath.string, encoding: .utf8)

        let renameDataStart = "/* C exports renaming data */"
        let parts = contents.components(separatedBy: renameDataStart)
        guard parts.count == 2 else {
            fatalError()
        }

        let partsContainingRenamingDefines = parts[1]
        let r = try Regex(#"#define\s+(\w+)\s+U_ICU_ENTRY_POINT_RENAME\(\1\)"#)
        let matches = partsContainingRenamingDefines.matches(of: r)
        print("found \(matches.count) matches")

        // First section: expanded symbol defines for when symbol renaming is enabled
        var output = parts[0] + renameDataStart + "\n\n#if APPLE_ICU_CHANGES\n#if !U_DISABLE_RENAMING\n"
        for match in matches {
            guard let matchRange = match[1].range else {
                fatalError("range not found for \(match)")
            }
            let symbol = partsContainingRenamingDefines[matchRange]
            output += "#define \(symbol) swift_\(symbol)\n"
        }
        output += "#endif\n#else\n"

        // Second section: the original defines
        for match in matches {
            guard let matchRange = match[1].range else {
                fatalError("range not found for \(match)")
            }
            let symbol = partsContainingRenamingDefines[matchRange]
            output += "#define \(symbol) U_ICU_ENTRY_POINT_RENAME(\(symbol))\n"
        }
        output += "#endif\n"

        // Final part: everything else
        guard let lastSymbolDef = partsContainingRenamingDefines.ranges(of: r).last else {
            fatalError("unexpected error")
        }
        let trailingParts = partsContainingRenamingDefines[lastSymbolDef.upperBound...]
        output += trailingParts

        // Replace the file
        let replacedHeader =  outputHeadersDst.appending("urename_new.h")
        try output.write(toFile: replacedHeader.string, atomically: true, encoding: .utf8)

        print("[STEP ✅]: Process renaming header")
    }
}

// MARK: - Helpers
extension ICUExport {
    private func createDirectory(_ path: FilePath) throws {
        print("creating directory at \(path)")
        try FileManager.default.createDirectory(
            atPath: path.string,
            withIntermediateDirectories: true
        )
    }
}


// MARK: - Templates
private let cmakeListsTemplate =
"""
##===----------------------------------------------------------------------===##
##
## This source file is part of the SwiftFoundation open source project
##
## Copyright (c) 2024 Apple Inc. and the SwiftFoundation project authors
## Licensed under Apache License v2.0
##
## See LICENSE.txt for license information
## See CONTRIBUTORS.md for the list of SwiftFoundation project authors
##
## SPDX-License-Identifier: Apache-2.0
##
##===----------------------------------------------------------------------===##

target_include_directories(_FoundationICU PRIVATE .)
target_sources(_FoundationICU
    PRIVATE
"""

private let dataFileIndexMark = "<%DATA_FILE_INDEX_MARK%>"
private let dataFileContentMark = "<%DATA_FILE_CONTENT_MARK%>"
private let dataFileNameTemplate = "icu_packaged_main_data.\(dataFileIndexMark).inc.h"
private let dataFileTemplate =
"""
//===----------------------------------------------------------------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2023 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

// This file is automatically generated. Don't touch this file directly.

#define __ICU_PACKAGED_MAIN_DATA_CHUNK_\(dataFileIndexMark) \(dataFileContentMark)
"""

private let timezoneFileNameTemplate = "icu_packaged_timezone_data.\(dataFileIndexMark).inc.h"

// MARK: - Misc
extension ICUExport {
    enum Error: Swift.Error {
        case commandFailed(String)
        case dataFileMissing
        case fileManagerFailed(String)
        case malformedFileContent(FilePath, String)
    }
}

extension AsyncParsableCommand {
    public static func main2(_ arguments: [String]?) async {
        await main(arguments)
    }
}

await ICUExport.main2(nil)
