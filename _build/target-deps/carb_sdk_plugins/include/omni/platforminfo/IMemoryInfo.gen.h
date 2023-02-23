// Copyright (c) 2021-2022, NVIDIA CORPORATION. All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto. Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.
//
// --------- Warning: This is a build system generated file. ----------
//

//! @file
//!
//! @brief This file was generated by <i>omni.bind</i>.

#include <omni/core/OmniAttr.h>
#include <omni/core/Interface.h>
#include <omni/core/ResultError.h>

#include <functional>
#include <utility>
#include <type_traits>

#ifndef OMNI_BIND_INCLUDE_INTERFACE_IMPL


/** Interface to collect and retrieve information about memory installed in the system. */
template <>
class omni::core::Generated<omni::platforminfo::IMemoryInfo_abi> : public omni::platforminfo::IMemoryInfo_abi
{
public:
    OMNI_PLUGIN_INTERFACE("omni::platforminfo::IMemoryInfo")

    /** Retrieves the total installed physical RAM in the system.
     *
     *  @returns The number of bytes of physical RAM installed in the system.  This value will
     *           not change during the lifetime of the calling process.
     *
     *  @thread_safety This call is thread safe.
     */
    size_t getTotalPhysicalMemory() noexcept;

    /** Retrieves the available physical memory in the system.
     *
     *  @returns The number of bytes of physical RAM that is currently available for use by the
     *           operating system.  Note that this is not a measure of how much memory is
     *           available to the calling process, but rather for the entire system.
     *
     *  @thread_safety This call is thread safe.  However, two consecutive or concurrent calls
     *                 are unlikely to return the same value.
     */
    size_t getAvailablePhysicalMemory() noexcept;

    /** Retrieves the total page file space in the system.
     *
     *  @returns The number of bytes of page file space in the system.  The value will not
     *           change during the lifetime of the calling process.
     *
     *  @thread_safety This call is thread safe.
     */
    size_t getTotalPageFileMemory() noexcept;

    /** Retrieves the available page file space in the system.
     *
     *  @returns The number of bytes of page file space that is currently available for use
     *           by the operating system.
     *
     *  @thread_safety This call is thread safe.  However, two consecutive or concurrent calls
     *                 are unlikely to return the same value.
     */
    size_t getAvailablePageFileMemory() noexcept;

    /** Retrieves the total memory usage for the calling process.
     *
     *  @returns The number of bytes of memory used by the calling process.  This will not
     *           necessarily be the amount of the process's virtual memory space that is
     *           currently in use, but rather the amount of memory that the OS currently
     *           has wired for this process (ie: the process's working set memory).  It is
     *           possible that the process could have a lot more memory allocated, just
     *           inactive as far as the OS is concerned.
     *
     *  @thread_safety This call is thread safe.  However, two consecutive calls are unlikely
     *                 to return the same value.
     */
    size_t getProcessMemoryUsage() noexcept;

    /** Retrieves the peak memory usage of the calling process.
     *
     *  @returns The maximum number of bytes of memory used by the calling process.  This will
     *           not necessarily be the maximum amount of the process's virtual memory space that
     *           was ever allocated, but rather the maximum amount of memory that the OS ever had
     *           wired for the process (ie: the process's working set memory).  It is possible
     *           that the process could have had a lot more memory allocated, just inactive as
     *           far as the OS is concerned.
     */
    size_t getProcessPeakMemoryUsage() noexcept;
};

#endif

#ifndef OMNI_BIND_INCLUDE_INTERFACE_DECL

inline size_t omni::core::Generated<omni::platforminfo::IMemoryInfo_abi>::getTotalPhysicalMemory() noexcept
{
    return getTotalPhysicalMemory_abi();
}

inline size_t omni::core::Generated<omni::platforminfo::IMemoryInfo_abi>::getAvailablePhysicalMemory() noexcept
{
    return getAvailablePhysicalMemory_abi();
}

inline size_t omni::core::Generated<omni::platforminfo::IMemoryInfo_abi>::getTotalPageFileMemory() noexcept
{
    return getTotalPageFileMemory_abi();
}

inline size_t omni::core::Generated<omni::platforminfo::IMemoryInfo_abi>::getAvailablePageFileMemory() noexcept
{
    return getAvailablePageFileMemory_abi();
}

inline size_t omni::core::Generated<omni::platforminfo::IMemoryInfo_abi>::getProcessMemoryUsage() noexcept
{
    return getProcessMemoryUsage_abi();
}

inline size_t omni::core::Generated<omni::platforminfo::IMemoryInfo_abi>::getProcessPeakMemoryUsage() noexcept
{
    return getProcessPeakMemoryUsage_abi();
}

#endif

#undef OMNI_BIND_INCLUDE_INTERFACE_DECL
#undef OMNI_BIND_INCLUDE_INTERFACE_IMPL
