// Copyright (c) 2020-2022, NVIDIA CORPORATION. All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto. Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.
//
#pragma once

#include <carb/Framework.h>
#include <carb/extras/Library.h>
#include <carb/filesystem/IFileSystem.h>
#include <carb/filesystem/FindFiles.h>
#include <carb/logging/Log.h>

#include <omni/str/Wildcard.h>

#include <cstring>

namespace carb
{

using FindPluginsOnMatchedFn = void(const char* canonical, bool reloadable, void* context);

struct FindPluginsArgs
{
    //! Search folders to look for plugins in.
    //!
    //! This may contain relative or absolute paths.  All relative paths will be resolved relative to
    //! filesystem::IFileSystem::getAppDirectoryPath(), not the current working directory. Absolute paths in the list
    //! will be searched directly.
    //!
    //! If search paths configuration is invalid (e.g. search paths count is zero), the fall
    //! back values are taken from the default plugin desc.
    //!
    //! Can be nullptr.
    const char* const* searchPaths;
    size_t searchPathCount; //!< Can be 0.

    //! Is search recursive in search folders
    bool searchRecursive;

    //! File name wildcards to select loaded files. `*` and `?` can be used, e.g. "carb.*.pl?gin"
    //!
    //! If nullptr, a reasonable default is used.
    const char* const* loadedFileWildcards;
    size_t loadedFileWildcardCount; //!< Can be 0.

    //! File name wildcards to mark loaded files as reloadable. Framework will treat them specially to allow
    //! overwriting source plugins and will monitor them for changes.
    //!
    //! Can be nullptr.
    const char* const* reloadableFileWildcards;
    size_t reloadableFileWildcardCount; //!< Can be 0.

    //! File name wildcards to select excluded files. `*` and `?` can be used.
    //!
    //! Can be nullptr.
    const char* const* excludedFileWildcards;
    size_t excludedFileWildcardCount; //!< Can be 0.

    //! Callback when a file is matched but not excluded.
    //!
    //! The canonical name is passed.
    //!
    //! reloadable is true if the file matches a pattern in reloadableFileWildcards.
    //!
    //! Must not be nullptr.
    FindPluginsOnMatchedFn* onMatched;
    void* onMatchedContext; //!< Context for onMatched.  Can be nullptr.

    //! Callback when a file is matched and excluded.
    //!
    //! The canonical name is passed.
    //!
    //! Can be nullptr.
    filesystem::FindFilesOnExcludedFn* onExcluded;
    void* onExcludedContext; //!< Context for onExcluded.  Can be nullptr.

    //! Callback when a file is not match one of the "loadedFileWildcard" patterns
    //!
    //! The canonical name is passed.
    //!
    //! Can be nullptr.
    filesystem::FindFilesOnSkippedFn* onSkipped;
    void* onSkippedContext; //!< Context for onSkipped.  Can be nullptr.

    //! Callback invoked before searching one of the given directories.
    //!
    //! Can be nullptr.
    filesystem::FindFilesOnSearchPathFn* onSearchPath;
    void* onSearchPathContext; //!< Context for onSearchPath.  Can be nullptr.

    //! IFileSystem object to use to walk the file system.
    //!
    //! If nullptr, tryAcquireInterface<IFileSystem> is called.
    filesystem::IFileSystem* fs;
};

//! Find plugins in a given list of search paths.
//!
//! See FindPluginsArgs for argument documentation.
//!
//! When finding plugins, the following assumptions are made:
//!
//! * The file's extension is ignored.
//! * On Linux, the "lib" prefix is ignored.
//! * ${MY_ENV_VAR} in a search path is replaced with the corresponding env var.
//!
//! Returns true if the file system was searched, false otherwise (i.e. bad args).
bool findPlugins(FindPluginsArgs& args);

namespace details
{

inline bool caseInsensitiveEndsWith(const char* str, const char* tail)
{
    const size_t strLen = std::strlen(str);
    const size_t tailLen = std::strlen(tail);

    // String should be at least as long as tail
    if (strLen < tailLen)
    {
        return false;
    }

    // Compare with tail, character by character
    for (size_t i = 0; i < tailLen; ++i)
    {
        // Tail is assumed to already be lowercase
        if (tail[tailLen - i - 1] != std::tolower(str[strLen - i - 1]))
        {
            return false;
        }
    }
    return true;
}

} // namespace details

inline bool findPlugins(FindPluginsArgs& inArgs)
{
    filesystem::FindFilesArgs args = {};

    args.searchPaths = inArgs.searchPaths;
    args.searchPathsCount = uint32_t(inArgs.searchPathCount);

    PluginLoadingDesc defaultPluginDesc = PluginLoadingDesc::getDefault();
    if (!args.searchPaths || (0 == args.searchPathsCount))
    {
        // If search path count it not specified, fall back to the default desc search paths
        args.searchPaths = defaultPluginDesc.searchPaths;
        args.searchPathsCount = uint32_t(defaultPluginDesc.searchPathCount);
    }

    args.matchWildcards = inArgs.loadedFileWildcards;
    args.matchWildcardsCount = uint32_t(inArgs.loadedFileWildcardCount);

    args.excludeWildcards = inArgs.excludedFileWildcards;
    args.excludeWildcardsCount = uint32_t(inArgs.excludedFileWildcardCount);

#if CARB_PLATFORM_LINUX || CARB_PLATFORM_MACOS
    constexpr const char* const kIgnorePrefixes[] = { "lib" };
    constexpr uint32_t kIgnorePrefixesCount = 1;
#elif CARB_PLATFORM_WINDOWS
    constexpr const char* const* kIgnorePrefixes = nullptr;
    constexpr uint32_t kIgnorePrefixesCount = 0;
#else
    CARB_UNSUPPORTED_PLATFORM();
#endif

    args.ignorePrefixes = kIgnorePrefixes;
    args.ignorePrefixesCount = kIgnorePrefixesCount;

    args.fs = inArgs.fs;

    // to avoid the expensive filename canonicalization and pattern matching, we do a quick check to make sure the
    // extension is for a plugin
    args.onFilterNonCanonical = [](const char* path, void*) {
        if (details::caseInsensitiveEndsWith(path, carb::extras::getDefaultLibraryExtension()))
        {
            return filesystem::WalkAction::eContinue; // could be a plugin (i.e. correct .ext)
        }
        else
        {
            return filesystem::WalkAction::eSkip; // not a plug .ext.  skip
        }
    };

    args.onMatched = [](const char* canonical, void* context) {
        auto inArgs = static_cast<FindPluginsArgs*>(context);

        bool reloadable = false;
        if (inArgs->reloadableFileWildcards && inArgs->reloadableFileWildcardCount)
        {
            extras::Path path(canonical);
            auto stemBuffer = path.getStem();
            const char* stem = stemBuffer.getStringBuffer();

            reloadable = omni::str::matchWildcards(
                stem, inArgs->reloadableFileWildcards, uint32_t(inArgs->reloadableFileWildcardCount));
#if CARB_PLATFORM_LINUX || CARB_PLATFORM_MACOS
            if (!reloadable)
            {
                if (extras::startsWith(stem, "lib"))
                {
                    stem += 3;
                    reloadable = omni::str::matchWildcards(
                        stem, inArgs->reloadableFileWildcards, uint32_t(inArgs->reloadableFileWildcardCount));
                }
            }
#endif
        }

        inArgs->onMatched(canonical, reloadable, inArgs->onMatchedContext);
    };
    args.onMatchedContext = const_cast<FindPluginsArgs*>(&inArgs);

    args.onExcluded = inArgs.onExcluded;
    args.onExcludedContext = inArgs.onExcludedContext;
    if (!args.onExcluded)
    {
        args.onExcluded = [](const char* canonical, void*) {
            CARB_LOG_VERBOSE("Excluding potential plugin file: %s.", canonical);
        };
    }

    args.onSkipped = inArgs.onSkipped;
    args.onSkippedContext = inArgs.onSkippedContext;

    args.onSearchPath = inArgs.onSearchPath;
    args.onSearchPathContext = inArgs.onSearchPathContext;
    if (!args.onSearchPath)
    {
        args.onSearchPath = [](const char* path, void* context) {
            auto inArgs = static_cast<FindPluginsArgs*>(context);
            CARB_LOG_VERBOSE("Searching plugins %sin folder: %s", (inArgs->searchRecursive ? "recursively " : ""), path);
        };
        args.onSearchPathContext = const_cast<FindPluginsArgs*>(&inArgs);
    }

    args.flags = (filesystem::kFindFilesFlagMatchStem | filesystem::kFindFilesFlagReplaceEnvironmentVariables);
    if (inArgs.searchRecursive)
    {
        args.flags |= filesystem::kFindFilesFlagRecursive;
    }

    return filesystem::findFiles(args);
}

} // namespace carb
