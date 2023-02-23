// Copyright (c) 2020-2022, NVIDIA CORPORATION. All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto. Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.
//
//! @file
//! @brief Provides a helper class to store wildcard patterns for log channels.
#pragma once

#include <omni/log/ILogChannelFilter.h>
#include <omni/str/Wildcard.h>
#include <carb/Defines.h>

namespace omni
{
//! Namespace for logging functionality.
namespace log
{

//! ILogChannelFilter implementation that supports pattern matching via wildcards (e.g. * and ?).
class WildcardLogChannelFilter : public omni::core::Implements<ILogChannelFilter>
{
public:
    //! Creates a filter with the given pattern.
    //!
    //! @param[in] wildcard     The wildcard pattern that this object will represent.  This may
    //!                         not be `nullptr`.
    //! @returns The newly constructed object containing the given wildcard pattern.
    static omni::core::ObjectPtr<WildcardLogChannelFilter> create(const char* wildcard)
    {
        OMNI_ASSERT(wildcard, "WildcardLogChannelFilter: the given string must not be nullptr");
        return { new WildcardLogChannelFilter{ wildcard }, omni::core::kSteal };
    }

    //! Tells the filter to set the enabled settings when a channel matches.
    //!
    //! If this method is not called, the filter will not set any enabled settings.
    //!
    //! This method is not thread safe and should only be called before the filter is added to a ILogChannelFilterList.
    void setEnabled(bool enabled, SettingBehavior behavior) noexcept
    {
        m_enabled = enabled;
        m_enabledBehavior = behavior;
        m_enabledUsed = true;
    }

    //! Tells the filter to set the level settings when a channel matches.
    //!
    //! If this method is not called, the filter will not set any level settings.
    //!
    //! This method is not thread safe and should only be called before the filter is added to a ILogChannelFilterList.
    void setLevel(Level level, SettingBehavior behavior) noexcept
    {
        m_level = level;
        m_levelBehavior = behavior;
        m_levelUsed = true;
    }

protected:
    //! @copydoc omni::log::ILogChannelFilter_abi::getFilter_abi()
    const char* getFilter_abi() noexcept override
    {
        return m_wildcard.c_str();
    }

    //! @copydoc omni::log::ILogChannelFilter_abi::getEnabled_abi()
    virtual void getEnabled_abi(bool* enabled, SettingBehavior* behavior, bool* isUsed) noexcept override
    {
        *enabled = m_enabled;
        *behavior = m_enabledBehavior;
        *isUsed = m_enabledUsed;
    }

    //! @copydoc omni::log::ILogChannelFilter_abi::getLevel_abi()
    virtual void getLevel_abi(Level* level, SettingBehavior* behavior, bool* isUsed) noexcept override
    {
        *level = m_level;
        *behavior = m_levelBehavior;
        *isUsed = m_levelUsed;
    }

    //! @copydoc omni::log::ILogChannelFilter_abi::isMatch_abi()
    virtual bool isMatch_abi(const char* channel) noexcept override
    {
        return omni::str::matchWildcard(channel, m_wildcard.c_str());
    }

private:
    WildcardLogChannelFilter(std::string wildcard) : m_wildcard{ std::move(wildcard) }
    {
    }

    CARB_PREVENT_COPY_AND_MOVE(WildcardLogChannelFilter);

    std::string m_wildcard;

    bool m_enabled = false;
    SettingBehavior m_enabledBehavior = SettingBehavior::eInherit;

    Level m_level = Level::eWarn;
    SettingBehavior m_levelBehavior = SettingBehavior::eInherit;

    bool m_enabledUsed = false;
    bool m_levelUsed = false;
};

} // namespace log
} // namespace omni
