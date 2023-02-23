// Copyright (c) 2018-2022, NVIDIA CORPORATION. All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto. Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.
//
#pragma once

#include <cinttypes>
#include <cstdint>
#include <cstdio>

namespace carb
{

/**
 * Defines a major and minor version.
 */
struct Version
{
    uint32_t major;
    uint32_t minor;
};

constexpr bool operator<(const Version& lhs, const Version& rhs)
{
    if (lhs.major == rhs.major)
    {
        return lhs.minor < rhs.minor;
    }
    return lhs.major < rhs.major;
}

constexpr bool operator==(const Version& lhs, const Version& rhs)
{
    return lhs.major == rhs.major && lhs.minor == rhs.minor;
}

inline bool isVersionSemanticallyCompatible(const char* name, const Version& minimum, const Version& candidate)
{
    if (minimum.major != candidate.major)
    {
        return false;
    }
    else if (minimum.major == 0)
    {
        // Need to special case when major is equal but zero, then any difference in minor makes them
        // incompatible. See http://semver.org for details.
        // the case of version 0.x (major of 0), we are only going to "warn" the user of possible
        // incompatability when a user asks for 0.x and we have an implementation 0.y (where y > x).
        // see https://nvidia-omniverse.atlassian.net/browse/CC-249
        if (minimum.minor > candidate.minor)
        {
            return false;
        }
        else if (minimum.minor < candidate.minor)
        {
            // using CARB_LOG maybe pointless, as logging may not be set up yet.
            fprintf(stderr,
                    "Warning: Possible version incompatability. Attempting to load %s with version v%" PRIu32
                    ".%" PRIu32 " against v%" PRIu32 ".%" PRIu32 ".\n",
                    name, candidate.major, candidate.minor, minimum.major, minimum.minor);
        }
    }
    else if (minimum.minor > candidate.minor)
    {
        return false;
    }
    return true;
}

} // namespace carb
