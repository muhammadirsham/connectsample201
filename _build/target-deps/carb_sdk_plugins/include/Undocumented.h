// Copyright (c) 2021-2022, NVIDIA CORPORATION. All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto. Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.
//

//! \file Undocumented.h
//!
//! \brief Place-holder documentation for core types whose documentation we haven't had a chance to clean-up.
#pragma once

#ifndef DOXYGEN_BUILD
#    error Do not include this header.  It's sole purpose is to reduce documentation build warnings.
#endif

namespace omni
{
namespace core
{
//! Undocumented.
class ITypeFactory
{
};
} // namespace core

} // namespace omni

namespace carb
{

//! Undocumented.
namespace l10n
{

//! Undocumented.
void registerLocalizationForClient();
//! Undocumented.
void deregisterLocalizationForClient();

} // namespace l10n

//! Undocumented.
namespace logging
{
//! Undocumented.
struct ILogging
{
};

//! Undocumented.
void registerLoggingForClient();

//! Undocumented.
void deregisterLoggingForClient();
} // namespace logging

//! Undocumented.
namespace settings
{
//! Undocumented.
struct ISettings
{
};
} // namespace settings

//! Undocumented.
namespace dictionary
{
//! Undocumented.
enum class ItemType
{
    eBool, //!< Undocumented.
    eInt, //!< Undocumented.
    eFloat, //!< Undocumented.
    eString, //!< Undocumented.
    eDictionary, //!< Undocumented.
    eCount //!< Undocumented.
};

//! Undocumented.
enum class UpdateAction
{
    eOverwrite, //!< Undocumented.
    eKeep, //!< Undocumented.
    eReplaceSubtree //!< Undocumented.
};

//! Undocumented.
void (*OnUpdateItemFn)();

//! Undocumented.
struct IDictionary
{
    //! Undocumented.
    void getItemChildByIndex();
    //! Undocumented.
    void destroyItem();
    //! Undocumented.
    void update();
};
} // namespace dictionary

//! Undocumented.
void acquireFrameworkForBindings(const char*);


} // namespace carb

//! Undocumented.
carb::logging::ILogging* g_carbLogging;

//! Undocumented.
void* g_carbLogFn;

//! Undocumented.
#define OMNI_PYTHON_GLOBALS
