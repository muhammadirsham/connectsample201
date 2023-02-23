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


/** This interface controls the ability to send Carbonite and Omniverse logging through the
 *  structured log system. The output is equivalent to the standard logging output, except that it
 *  is in JSON, so it will be easier for programs to consume.
 *
 *  The default state of structured log logging is off, but it can be enabled by calling
 *  @ref omni::structuredlog::IStructuredLogFromILog::enableLogging() or setting `/structuredLog/enableLogConsumer`
 *  to true with ISettings.
 */
template <>
class omni::core::Generated<omni::structuredlog::IStructuredLogFromILog_abi>
    : public omni::structuredlog::IStructuredLogFromILog_abi
{
public:
    OMNI_PLUGIN_INTERFACE("omni::structuredlog::IStructuredLogFromILog")

    /** Enable the structured log logger.
     *  @remarks Enabling this will result in all Carbonite logging (e.g. CARB_LOG_*) and all
     *           Omniverse logging (e.g. OMNI_LOG_*) going to the structured log log file.
     *           This may be useful if you want the logs to be consumed by some sort of log reader
     *           program.
     *           These log events will be sent to the default structured log system's log file if
     *           there is one; they will otherwise go to a log file named
     *           "omni.structuredlog.logging-{version}".
     *           These log events will not be sent to the collection servers.
     */
    void enableLogging() noexcept;

    /** Disables the structured log logger.
     *  @remarks After this is called, log messages will no longer be sent as structured log
     *           events to the structured log system's log file.
     */
    void disableLogging() noexcept;

    /** Get the @ref omni::structuredlog::EventId of the logging schema.
     *  @returns @ref omni::structuredlog::EventId of the logging schema.
     *
     *  @remarks This is only needed if you need to query properties of the logging schema,
     *           such as the log file name.
     *
     *  @note The logging schema will still be valid when logging is disabled.
     */
    omni::structuredlog::EventId getLoggingEventId() noexcept;
};

#endif

#ifndef OMNI_BIND_INCLUDE_INTERFACE_DECL

inline void omni::core::Generated<omni::structuredlog::IStructuredLogFromILog_abi>::enableLogging() noexcept
{
    enableLogging_abi();
}

inline void omni::core::Generated<omni::structuredlog::IStructuredLogFromILog_abi>::disableLogging() noexcept
{
    disableLogging_abi();
}

inline omni::structuredlog::EventId omni::core::Generated<omni::structuredlog::IStructuredLogFromILog_abi>::getLoggingEventId() noexcept
{
    return getLoggingEventId_abi();
}

#endif

#undef OMNI_BIND_INCLUDE_INTERFACE_DECL
#undef OMNI_BIND_INCLUDE_INTERFACE_IMPL
