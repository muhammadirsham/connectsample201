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
//! @brief DLL Boundary safe memory management functions
#pragma once

#include "Defines.h"
#include "Types.h"

#if !defined(CARB_REQUIRE_LINKED) || defined(DOXYGEN_BUILD)
//! Changes how the `carbReallocate` symbol is acquired.
//!
//! If \c CARB_REQUIRE_LINKED is defined as 1 before this file is included, then `carbReallocate` will be imported from
//! \a carb.dll or \a libcarb.so and the binary must link against the import library for the module. This can be useful
//! For applications that want to use \c carb::allocate() prior to initializing the framework.
//!
//! If not defined or defined at \c 0, `carbReallocate` is weakly-linked and the binary need not link against the import
//! library. Attempting to call \c carb::allocate() will dynamically attempt to find \c carbReallocate() in the already-
//! loaded \a carb module. If it cannot be found a warning will be thrown.
#    define CARB_REQUIRE_LINKED 0
#endif

#include "CarbWindows.h"

//! Internal function used by all other allocation functions.
//!
//! @note Using this function requires linking with carb.dll/libcarb.so if `CARB_REQUIRE_LINKED = 1` or ensuring that
//! carb.dll/libcarb.so is already loaded before it's called if `CARB_REQUIRE_LINKED = 0`.
#if CARB_REQUIRE_LINKED
CARB_DYNAMICLINK void* carbReallocate(void* p, size_t size, size_t align);
#else
CARB_DYNAMICLINK void* carbReallocate(void* p, size_t size, size_t align) CARB_ATTRIBUTE(weak);
#endif


namespace carb
{
namespace detail
{

//! Gets the @c carbReallocate function or returns `nullptr` if it could not be loaded.
inline auto getCarbReallocate() -> void* (*)(void*, size_t, size_t)
{
    // NOTE NOTE NOTE
    // If either of the fatal conditions below occurs, you have not loaded the carb.dll/libcarb.so module prior to this
    // function being called. To enforce that the binary must be linked against the import library for the module,
    // `#define CARB_REQUIRE_LINKED 1` before including this file.
    // NOTE NOTE NOTE

#if CARB_REQUIRE_LINKED || CARB_PLATFORM_LINUX || CARB_PLATFORM_MACOS
    auto impl = &carbReallocate;
    CARB_FATAL_UNLESS(impl != nullptr, "Could not find `carbReallocate` function -- make sure that libcarb.so is loaded");
    return impl;
#elif CARB_PLATFORM_WINDOWS
    static void* (*carbReallocateFn)(void*, size_t, size_t) = nullptr;
    if (!carbReallocateFn)
    {
        HMODULE hCarbDll = GetModuleHandleW(L"carb.dll");
        CARB_FATAL_UNLESS(
            hCarbDll != nullptr,
            "Could not find `carb.dll` module -- make sure that it is loaded prior to calling any memory functions");
        carbReallocateFn = (decltype(carbReallocateFn))GetProcAddress(hCarbDll, "carbReallocate");
        CARB_FATAL_UNLESS(
            carbReallocateFn != nullptr,
            "Could not find `carbReallocate` function at runtime -- #define CARB_REQUIRE_LINKED 1 before including this file");
    }
    return carbReallocateFn;
#else
    CARB_UNSUPPORTED_PLATFORM();
#endif
}
} // namespace detail

//! Allocates a block of memory.
//!
//! @note Any plugin (or the executable) may @ref allocate the memory and a different plugin (or the executable) may
//! @ref deallocate or @ref reallocate it.
//!
//! @note If carb.dll/libcarb.so is not loaded, this function will always return `nullptr`.
//!
//! @param size The size of the memory block requested, in bytes. Specifying '0' will return a valid pointer that
//! can be passed to @ref deallocate but cannot be used to store any information.
//! @param align The minimum alignment (in bytes) of the memory block requested. Must be a power of two. Values less
//!     than `sizeof(size_t)` are ignored. `0` indicates to use default system alignment (typically
//!     `2 * sizeof(void*)`).
//! @returns A non-`nullptr` memory block of @p size bytes with minimum alignment @p align. If an error occurred,
//!     or memory could not be allocated, `nullptr` is returned. The memory is not initialized.
inline void* allocate(size_t size, size_t align = 0) noexcept
{
    if (auto impl = detail::getCarbReallocate())
        return impl(nullptr, size, align);
    else
        return nullptr;
}

//! Deallocates a block of memory previously allocated with @ref allocate().
//!
//! @note Any plugin (or the executable) may @ref allocate the memory and a different plugin (or the executable) may
//! @ref deallocate or @ref reallocate it.
//!
//! @note If carb.dll/libcarb.so is not loaded, this function will silently do nothing. Since @ref allocate would have
//! returned `nullptr` in this case, this function should never be called.
//!
//! @param p The block of memory previously returned from @ref allocate() or @ref reallocate(), or `nullptr`.
inline void deallocate(void* p) noexcept
{
    if (p)
    {
        if (auto impl = detail::getCarbReallocate())
            impl(p, 0, 0);
    }
}

//! Reallocates a block of memory previously allocated with @ref allocate().
//!
//! This function changes the size of the memory block pointed to by @p p to @p size bytes with @p align alignment.
//! The contents are unchanged from the start of the memory block up to the minimum of the old size and @p size. If
//! @p size is larger than the old size, the added memory is not initialized. If @p p is `nullptr`, the call is
//! equivalent to `allocate(size, align)`; if @p size is `0` and @p p is not `nullptr`, the call is equivalent to
//! `deallocate(p)`. Unless @p p is `nullptr`, it must have been retrieved by an earlier call to @ref allocate() or
//! @ref reallocate(). If the memory region was moved in order to resize it, @p p will be freed as with `deallocate(p)`.
//!
//! @note Any plugin (or the executable) may @ref allocate the memory and a different plugin (or the executable) may
//! @ref deallocate or @ref reallocate it.
//!
//! @note If carb.dll/libcarb.so is not loaded, this function will always return @p p without side-effects.
//!
//! @param p The block of memory previously returned from @ref allocate() or @ref reallocate() if resizing is
//!     resizing is desired. If `nullptr` is passed as this parameter, the call behaves as if
//!     `allocate(size, align)` was called.
//! @param size The size of the memory block requested, in bytes. See above for further explanation.
//! @param align The minimum alignment (in bytes) of the memory block requested. Must be a power of two. Values less
//!     than `sizeof(size_t)` are ignored. Changing the alignment from a previous allocation is undefined behavior.
//!     `0` indicates to use default system alignment (typically `2 * sizeof(void*)`).
//! @returns A pointer to a block of memory of @p size bytes with minimum alignment @p align, unless an error
//!     occurs in which case `nullptr` is returned. If @p p is `nullptr` and @p size is `0` then `nullptr` is also
//!     returned.
inline void* reallocate(void* p, size_t size, size_t align = 0) noexcept
{
    if (auto impl = detail::getCarbReallocate())
        return impl(p, size, align);
    else
        return p;
}
} // namespace carb
