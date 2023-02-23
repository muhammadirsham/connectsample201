// Copyright (c) 2020-2022, NVIDIA CORPORATION. All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto. Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.
//
//! @file
//! @brief Provides a base class for a filter for log channel patterns.
#pragma once

#include <omni/core/IObject.h>
#include <omni/str/IReadOnlyCString.h>
#include <omni/log/ILog.h>
#include <omni/extras/OutArrayUtils.h>

namespace omni
{
//! Namespace for logging functionality.
namespace log
{

class ILogChannelFilter_abi;
class ILogChannelFilter;
class ILogChannelFilterList_abi;
class ILogChannelFilterList;
class ILogChannelFilterListUpdateConsumer_abi;
class ILogChannelFilterListUpdateConsumer;

//! Consumes (i.e. is notified) when an observed ILogChannelFilterList is updated.
//!
//! This object can be attached to multiple ILogChannelFilterList instances.
//!
//! See ILogChannelFilterList::addUpdateConsumer() to add this object to a channel filter list.
class ILogChannelFilterListUpdateConsumer_abi
    : public omni::core::Inherits<omni::core::IObject, OMNI_TYPE_ID("omni.log.ILogChannelFilterListUpdateConsumer")>
{
protected:
    //! Invoked when an observed ILogChannelFilterList is updated (e.g. filter remove/added/replaced).
    //!
    //! Is is safe to access the given ILogChannelFilterList from within this method.
    //!
    //! This method is expected to be called concurrently.
    virtual void onUpdate_abi(OMNI_ATTR("not_null") ILogChannelFilterList* list) noexcept = 0;
};

//! Read-only object to encapsulate a channel filter's pattern and effects.
//!
//! A channel filter is a pattern matcher.  If a channel's name matches the pattern, the filter can set both the
//! channel's enabled flag and/or level.
class ILogChannelFilter_abi : public omni::core::Inherits<omni::core::IObject, OMNI_TYPE_ID("omni.log.ILogChannelFilter")>
{
protected:
    //! Returns the channels pattern.  The returned memory is valid for the lifetime of this object.
    //!
    //! This method is thread safe.
    virtual OMNI_ATTR("c_str, not_null") const char* getFilter_abi() noexcept = 0;

    //! Returns the desired enabled state for this filter.
    //!
    //! All parameters must not be nullptr.
    //!
    //! If *isUsed is false after calling this method, *isEnabled and *behavior should not be used.
    //!
    //! This method is thread safe.
    virtual void getEnabled_abi(OMNI_ATTR("out, not_null") bool* isEnabled,
                                OMNI_ATTR("out, not_null") SettingBehavior* behavior,
                                OMNI_ATTR("out, not_null") bool* isUsed) noexcept = 0;

    //! Returns the desired level for this filter.
    //!
    //! All parameters must not be nullptr.
    //!
    //! If *isUsed is false after calling this method, *level and *behavior should not be used.
    //!
    //! This method is thread safe.
    virtual void getLevel_abi(OMNI_ATTR("out, not_null") Level* level,
                              OMNI_ATTR("out, not_null") SettingBehavior* behavior,
                              OMNI_ATTR("out, not_null") bool* isUsed) noexcept = 0;

    //! Given a channel name, returns if the channel name matches the filter's pattern.
    //!
    //! The matching algorithm used is implementation specific (e.g. regex, glob, etc).
    //!
    //! This method is thread safe.
    virtual bool isMatch_abi(OMNI_ATTR("c_str, not_null") const char* channel) noexcept = 0;
};

//! A list of channel filters that will be applied as channels are added to the log.
//!
//! This object is a list of ILogChannelFilter objects.  Filters can be added, removed, and replaced.
//!
//! This object can be attached to one or more ILog objects.  As channels are added to the ILog, this object will apply
//! its filters, thereby setting the new channel's state.
//!
//! If multiple filters match a channel, only the first filter's settings will be applied.
//!
//! This API is thread safe.
class ILogChannelFilterList_abi
    : public omni::core::Inherits<ILogChannelUpdateConsumer, OMNI_TYPE_ID("omni.log.ILogChannelFilterList")>
{
protected:
    //! Inserts the given filter at the given index.
    //!
    //! Existing filters at and after the given index are moved to the next slot in the list.
    //!
    //! If index is equal to or greater than the number of existing filters, the given filter is added to the end of the
    //! filter list.
    //!
    //! The given filter must not be nullptr.
    //!
    //! This method is thread safe.
    virtual void insert_abi(uint32_t index, OMNI_ATTR("not_null") ILogChannelFilter* filter) noexcept = 0;

    //! Replace the filter at the given index with a new filter.
    //!
    //! oldFilter is the current filter at index while newFilter is the filter to replace the old filter.
    //!
    //! If oldFilter is not currently at index, it is assumed another thread has modified the list.  In this case, this
    //! function does nothing and returns kResultInvalidState.
    //!
    //! oldFilter and newFilter must not be nullptr.
    //!
    //! Returns kResultSuccess if the filter was successfully replaced.
    //!
    //! This method is thread safe.
    virtual omni::core::Result replace_abi(uint32_t index,
                                           OMNI_ATTR("not_null") ILogChannelFilter* oldFilter,
                                           OMNI_ATTR("not_null") ILogChannelFilter* newFilter) noexcept = 0;

    //! Removes the filter at the given index.
    //!
    //! If filter is not currently at index, it is assumed another thread has modified the list.  In this case, this
    //! function does nothing and returns kResultInvalidState.
    //!
    //! filter must not be nullptr.
    //!
    //! Returns kResultSuccess if the filter was successfully replaced.
    //!
    //! This method is thread safe.
    virtual omni::core::Result remove_abi(uint32_t index, OMNI_ATTR("not_null") ILogChannelFilter* filter) noexcept = 0;

    //! Returns the list of filters.
    //!
    //! This method operates in two modes: query mode or get mode.
    //!
    //! outCount must not be nullptr in both modes.
    //!
    //! Query mode is enabled when out is nullptr.  When in this mode, *outCount will be populated with the number of
    //! filters and kResultSuccess is returned.
    //!
    //! Get mode is enabled when out is not nullptr.  Upon entering the function, *outCount stores the number of entries
    //! in out.  If *outCount is less than the number of filters, *outCount is updated with the number of filters and
    //! kResultInsufficientBuffer is returned.  If *outCount is greater or equal to the number of filters, *outCount is
    //! updated with the number of filters, out array is populated, and kResultSuccess is returned.
    //!
    //! If the out array is populated, each written entry in the array will have had acquire() called on it.
    //!
    //! Upon entering the function, the pointers in the out array must be nullptr.
    //!
    //! Return values are as above.  Additional error codes may be returned (e.g. kResultInvalidArgument if outCount is
    //! nullptr).
    //!
    //! This method is thread safe.
    virtual OMNI_ATTR("no_py, no_api") omni::core::Result getFilters_abi( // disable omni.bind until OM-21202
        OMNI_ATTR("out, count=*outCount, *not_null") ILogChannelFilter** out,
        OMNI_ATTR("in, out, not_null") uint32_t* outCount) noexcept = 0;

    //! Applies each filter to the given list of channels.
    //!
    //! log must not be nullptr.
    //!
    //! channels is an array of channel names in the given log.  This object will apply all filters to each channel.
    //!
    //! If the channels array is nullptr, this method will call ILog::getChannelNames() to get the list of channels.
    //!
    //! This method is thread safe.
    virtual OMNI_ATTR("no_py") void apply_abi( // disable omni.bind until OM-21202
        OMNI_ATTR("not_null") ILog* log,
        OMNI_ATTR("in, count=channelsCount, *not_null") omni::str::IReadOnlyCString* const* channels,
        uint32_t channelsCount) noexcept = 0;

    //! Adds an update consumer to this list.
    //!
    //! This added consumer will be notified each time an filter is added, removed, or replaced in this list.
    //!
    //! The given consumer may be notified in parallel.
    //!
    //! Notifications may be consolidated (i.e. multiple items in the list were updated).
    //!
    //! The given consumer may be notified spuriously.
    //!
    //! consumer must not be nullptr.
    //!
    //! This method is thread safe.
    virtual void OMNI_ATTR("consumer=onUpdate_abi")
        addUpdateConsumer_abi(OMNI_ATTR("not_null") ILogChannelFilterListUpdateConsumer* consumer) noexcept = 0;

    //! Removes the given consumer from the internal consumer list.
    //!
    //! This method silently accepts nullptr.
    //!
    //! This method silently accepts consumers that have not been registered with this object.
    //!
    //! Calling ILog::removeOnMessageConsumer() from ILogOnMessageConsumer::onMessage_abi() will lead to undefined
    //! behavior.
    //!
    //! This method is thread safe.
    virtual void removeUpdateConsumer_abi(ILogChannelFilterListUpdateConsumer* consumer) noexcept = 0;

    //! Returns the list of update consumers.
    //!
    //! This method operates in two modes: query mode or get mode.
    //!
    //! outCount must not be nullptr in both modes.
    //!
    //! Query mode is enabled when out is nullptr.  When in this mode, *outCount will be populated with the number of
    //! consumers and kResultSuccess is returned.
    //!
    //! Get mode is enabled when out is not nullptr.  Upon entering the function, *outCount stores the number of entries
    //! in out.  If *outCount is less than the number of consumers, *outCount is updated with the number of consumers
    //! and kResultInsufficientBuffer is returned.  If *outCount is greater or equal to the number of consumers,
    //! *outCount is updated with the number of consumers, out array is populated, and kResultSuccess is returned.
    //!
    //! If the out array is populated, each written entry in the array will have had acquire() called on it.
    //!
    //! Upon entering the function, the pointers in the out array must be nullptr.
    //!
    //! Return values are as above.  Additional error codes may be returned (e.g. kResultInvalidArgument if outCount is
    //! nullptr).
    //!
    //! This method is thread safe.
    virtual OMNI_ATTR("no_py, no_api") omni::core::Result getUpdateConsumers_abi( // disable omni.bind until OM-21202
        OMNI_ATTR("out, count=*outCount, *not_null") ILogChannelFilterListUpdateConsumer** out,
        OMNI_ATTR("in, out, not_null") uint32_t* outCount) noexcept = 0;

    //! Removes all filters from this list.
    //!
    //! This method is thread safe.
    virtual void clear_abi() noexcept = 0;
};

} // namespace log
} // namespace omni

#define OMNI_BIND_INCLUDE_INTERFACE_DECL
#include "ILogChannelFilter.gen.h"

//! \copydoc omni::log::ILogChannelFilter_abi
class omni::log::ILogChannelFilter : public omni::core::Generated<omni::log::ILogChannelFilter_abi>
{
};

// clang-format off
//! \copydoc omni::log::ILogChannelFilterList_abi
class omni::log::ILogChannelFilterList : public omni::core::Generated<omni::log::ILogChannelFilterList_abi>
{
public:
    //! Adds a filter to the end of the filter list.
    //!
    //! This method is thread safe.
    void append(omni::core::ObjectParam<omni::log::ILogChannelFilter> filter) noexcept
    {
        insert_abi(uint32_t(-1), filter.get());
    }

    //! Returns the list of filters.
    //!
    //! This method is thread safe.
    std::vector<omni::core::ObjectPtr<omni::log::ILogChannelFilter>> getFilters() noexcept
    {
        std::vector<omni::core::ObjectPtr<omni::log::ILogChannelFilter>> vec;
        auto result = omni::extras::getOutArray<omni::log::ILogChannelFilter*>(
            [this](omni::log::ILogChannelFilter** out, uint32_t* outCount) // get func
            {
                std::memset(out, 0, sizeof(omni::log::ILogChannelFilter*) * *outCount); // incoming ptrs must be nullptr
                return this->getFilters_abi(out, outCount);
            },
            [&vec](omni::log::ILogChannelFilter** in, uint32_t inCount) // fill func
            {
                vec.reserve(inCount);
                for (uint32_t i = 0; i < inCount; ++i)
                {
                    vec.emplace_back(in[i], omni::core::kSteal);
                }
            });

        if (OMNI_FAILED(result))
        {
            OMNI_LOG_ERROR("unable to retrieve filter list: 0x%08X", result);
        }

        return vec;
    }
};
// clang-format on

//! \copydoc omni::log::ILogChannelFilterListUpdateConsumer_abi
class omni::log::ILogChannelFilterListUpdateConsumer
    : public omni::core::Generated<omni::log::ILogChannelFilterListUpdateConsumer_abi>
{
};

#define OMNI_BIND_INCLUDE_INTERFACE_IMPL
#include "ILogChannelFilter.gen.h"
