// Copyright (c) 2021-2022, NVIDIA CORPORATION. All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto. Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.
//

//! @file
//!
//! @brief Helper utilities for memory
#pragma once

#include "../Defines.h"

#if CARB_PLATFORM_LINUX
#    include <unistd.h>
#endif

namespace carb
{
namespace memory
{

// Turn off optimization for testReadable() for Visual Studio, otherwise the read will be elided and it will always
// return true.
CARB_OPTIMIZE_OFF_MSC()

/**
 * Tests if a memory word (size_t) can be read from an address without crashing.
 *
 * @note This is not a particularly efficient function and should not be depended on for performance.
 *
 * @param mem The address to attempt to read
 * @returns `true` if a value could be read successfully, `false` if attempting to read the value would cause an access
 * violation or SIGSEGV.
 */
inline bool testReadable(const void* mem)
{
#if CARB_PLATFORM_WINDOWS
    // Use SEH to catch a read failure. This is actually very fast unless an exception occurs as no setup work is needed
    // on x86_64.
    __try
    {
        size_t s = *reinterpret_cast<const size_t*>(mem);
        CARB_UNUSED(s);
        return true;
    }
    __except (1)
    {
        return false;
    }
#elif CARB_POSIX
    // The pipes trick: use the kernel to validate that the memory can be read. write() will return -1 with errno=EFAULT
    // if the memory is not readable.
    int pipes[2];
    CARB_FATAL_UNLESS(pipe(pipes) == 0, "Failed to create pipes");
    int ret = CARB_RETRY_EINTR(write(pipes[1], mem, sizeof(size_t)));
    CARB_FATAL_UNLESS(
        ret == sizeof(size_t) || errno == EFAULT, "Unexpected result from write(): {%d/%s}", errno, strerror(errno));
    close(pipes[0]);
    close(pipes[1]);
    return ret == sizeof(size_t);
#else
    CARB_UNSUPPORTED_PLATFORM();
#endif
}
CARB_OPTIMIZE_ON_MSC()

} // namespace memory
} // namespace carb
