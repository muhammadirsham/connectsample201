// Copyright (c) 2020-2021, NVIDIA CORPORATION. All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto. Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.
//
#pragma once

#include <omni/core/ITypeFactory.h>
#include <omni/log/WildcardLogChannelFilter.h>

#include <carb/dictionary/IDictionary.h>
#include <carb/settings/ISettings.h>

#include <cctype>

namespace omni
{
namespace log
{

//! Applies log channel filters found in settings under /log/filters.
//!
//! This function search for filters under the /log/filters path in the given settings object.
//!
//! The filters are used to create ILogChannelFilter objects which are then added to a ILogChannelFilterList.  This list
//! is applied to, and then attached to, the log returned by omniGetLogWithoutAcquire().  The net effect is that as
//! logging channels are added to the logging system, the filters will be applied.
//!
//! The patterns in the filters are wildcard patterns.  * and ? are supported.  For example:
//!
//!   /log/filters/omni.* = verbose
//!   /log/filters/carb.audio.* = disable
//!
//! The value for each pattern can be one of: verbose, info, warn, error, fatal, disable.
//!
//! If a channel matches one of the patterns, its enable flag and level are set appropriately and each setting's
//! behavior is set to eOverride.
inline void configureLogChannelFilterList(carb::settings::ISettings* settings)
{
    carb::Framework* f = carb::getFramework();
    if (!f)
    {
        OMNI_LOG_ERROR("unable to acquire carb::Framework");
        return;
    }

    if (!settings)
    {
        OMNI_LOG_ERROR("unable to acquire ISettings for log filter configuration");
        return;
    }

    std::vector<omni::core::ObjectPtr<omni::log::WildcardLogChannelFilter>> seenFilters;

    const char* kLogChannelsKey = "/log/channels";
    const carb::dictionary::Item* filtersItem = settings->getSettingsDictionary(kLogChannelsKey);
    if (filtersItem != nullptr)
    {
        auto* dictInterface = f->acquireInterface<carb::dictionary::IDictionary>();
        for (size_t i = 0, totalChildren = dictInterface->getItemChildCount(filtersItem); i < totalChildren; ++i)
        {
            const carb::dictionary::Item* levelItem = dictInterface->getItemChildByIndex(filtersItem, i);
            if (!levelItem)
            {
                OMNI_LOG_ERROR("null log filter present in the configuration");
                continue;
            }

            const char* wildcard = dictInterface->getItemName(levelItem);
            if (!wildcard)
            {
                OMNI_LOG_ERROR("log filter with no name present in settings");
                continue;
            }

            const char* levelStr = dictInterface->getStringBuffer(levelItem);
            if (!levelStr)
            {
                OMNI_LOG_ERROR("log filter '%s' does not contain a level", wildcard);
                continue;
            }

            // only check the first char
            omni::log::Level level;
            switch (std::tolower(*levelStr))
            {
                case 'v':
                    level = omni::log::Level::eVerbose;
                    break;
                case 'i':
                    level = omni::log::Level::eInfo;
                    break;
                case 'w':
                    level = omni::log::Level::eWarn;
                    break;
                case 'e':
                    level = omni::log::Level::eError;
                    break;
                case 'f':
                    level = omni::log::Level::eFatal;
                    break;
                case 'd':
                    level = omni::log::Level::eDisabled;
                    break;
                default:
                    OMNI_LOG_ERROR(
                        "unknown log level given: '%s'. valid options are: "
                        "verbose, info, warn, error, fatal, disable",
                        levelStr);
                    continue;
            }

            omni::core::ObjectPtr<omni::log::WildcardLogChannelFilter> newFilter;
            auto itr = std::find_if(seenFilters.begin(), seenFilters.end(),
                                    [wildcard](const auto& f) { return (0 == std::strcmp(f->getFilter(), wildcard)); });
            if (itr != seenFilters.end())
            {
                newFilter = *itr; // existing
            }
            else
            {
                newFilter = omni::log::WildcardLogChannelFilter::create(wildcard); // new
                seenFilters.emplace_back(newFilter);
            }

            if (omni::log::Level::eDisabled != level)
            {
                newFilter->setLevel(level, omni::log::SettingBehavior::eOverride);
                newFilter->setEnabled(true, omni::log::SettingBehavior::eOverride);
            }
            else
            {
                newFilter->setEnabled(false, omni::log::SettingBehavior::eOverride);
            }
        }
    }

    if (!seenFilters.empty())
    {
        auto filterList = omni::core::createType<omni::log::ILogChannelFilterList>();
        if (!filterList)
        {
            OMNI_LOG_ERROR("unable to create ILogChannelFilterList for ISettings");
            return;
        }

        for (auto& filter : seenFilters)
        {
            filterList->insert(uint32_t(-1), std::move(filter));
        }

        omni::log::ILog* log = omniGetLogWithoutAcquire();
        log->addChannelUpdateConsumer(filterList);
        filterList->apply(log, nullptr, 0);
    }
}

} // namespace log
} // namespace omni
