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
//! @brief Helpers related reporting errors from @ref omni::core::Result.
#pragma once

#include <carb/extras/Debugging.h>
#include <omni/core/Assert.h>
#include <omni/core/IObject.h>

#include <stdexcept>
#include <string>

namespace omni
{
namespace core
{

//! Given a @ref Result code, returns a human readable interpretation of the
//! code.
//!
//! A valid pointer is always returned.  The returned pointer is valid for the
//! lifetime of the module from which the function was called.
inline const char* resultToString(Result result)
{
    switch (result)
    {
        case kResultSuccess:
            return "The operation was successful";
        case kResultNotImplemented:
            return "Not implemented";
        case kResultNoInterface:
            return "Interface not implemented";
        case kResultNullPointer:
            return "NULL pointer";
        case kResultOperationAborted:
            return "The operation is was aborted";
        case kResultFail:
            return "The operation failed";
        case kResultAlreadyExists:
            return "Already exists";
        case kResultNotFound:
            return "Not found";
        case kResultInvalidState:
            return "Invalid state";
        case kResultAccessDenied:
            return "Access denied";
        case kResultOutOfMemory:
            return "Out of memory";
        case kResultNotSupported:
            return "Not supported";
        case kResultInvalidArgument:
            return "Invalid argument";
        case kResultVersionCheckFailure:
            return "Version check failure";
        case kResultVersionParseError:
            return "Version parse error";
        case kResultInsufficientBuffer:
            return "Insufficient buffer";
        case kResultTryAgain:
            return "Try again";
        case kResultInvalidOperation:
            return "Invalid operation";
        case kResultNoMoreItems:
            return "No more items";
        case kResultInvalidIndex:
            return "Invalid index";
        case kResultNotEnoughData:
            return "Not enough data";
        case kResultTooMuchData:
            return "Too much data";
        case kResultInvalidDataType:
            return "Invalid data type";
        case kResultInvalidDataSize:
            return "Invalid data size";
        default:
            return "The operation produced an unknown error";
    }
}

//! Exception object that encapsulates a @ref Result along with a customizable
//! message.
class ResultError : public std::exception
{
public:
    //! Constructor.
    ResultError(Result result) : m_result{ result }
    {
    }

    //! Constructor with custom messages.
    ResultError(Result result, std::string msg) : m_result{ result }, m_msg(std::move(msg))
    {
    }

    //! Returns a human readable description of the error.
    virtual const char* what() const noexcept override
    {
        if (m_msg.empty())
        {
            return resultToString(m_result);
        }
        else
        {
            return m_msg.c_str();
        }
    }

    //! Return the result code that describes the error.
    Result getResult() const noexcept
    {
        return m_result;
    }

private:
    Result m_result;
    std::string m_msg;
};

} // namespace core
} // namespace omni

#if CARB_DEBUG && !defined(DOXYGEN_BUILD)
#    define OMNI_RETURN_ERROR(e_)                                                                                      \
        carb::extras::debuggerBreak();                                                                                 \
        return e_;
#else
//! Helper macro used to return a @ref omni::core::Result.  When in debug mode
//! and attached to a debugger, this macro will cause a debugger break.  Useful
//! for determining the origin of an error.
#    define OMNI_RETURN_ERROR(e_) return e_
#endif

//! Helper macro to catch exceptions and return them as @ref omni::core::Result
//! codes. Useful when writing ABI code.
#define OMNI_CATCH_ABI_EXCEPTION()                                                                                     \
    catch (const omni::core::ResultError& e_)                                                                          \
    {                                                                                                                  \
        OMNI_RETURN_ERROR(e_.getResult());                                                                             \
    }                                                                                                                  \
    catch (...)                                                                                                        \
    {                                                                                                                  \
        OMNI_RETURN_ERROR(omni::core::kResultFail);                                                                    \
    }

//! Helper macro to convert a @ref omni::core::Result to a @ref
//! omni::core::ResultError exception. Useful when authoring API code.  Used
//! heavily by *omni.bind*.
#define OMNI_THROW_IF_FAILED(expr_)                                                                                    \
    do                                                                                                                 \
    {                                                                                                                  \
        omni::core::Result result_ = (expr_);                                                                          \
        if (OMNI_FAILED(result_))                                                                                      \
        {                                                                                                              \
            throw omni::core::ResultError{ result_ };                                                                  \
        }                                                                                                              \
    } while (0)

//! Helper macro to return an appropriate @ref omni::core::Result when the given
//! argument is @c nullptr.  Useful when authoring ABI code.
//!
//! Note, use of this macro should be rare since *omni.bind* will check for @c
//! nullptr arguments in the generated API code.
#define OMNI_RETURN_IF_ARG_NULL(expr_)                                                                                 \
    do                                                                                                                 \
    {                                                                                                                  \
        if (nullptr == expr_)                                                                                          \
        {                                                                                                              \
            OMNI_RETURN_ERROR(omni::core::kResultInvalidArgument);                                                     \
        }                                                                                                              \
    } while (0)

//! Helper macro to throw a @ref omni::core::ResultError exception if a function argument is
//! @c nullptr.  Used heavily by *omni.bind*.
#define OMNI_THROW_IF_ARG_NULL(ptr_)                                                                                   \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!ptr_)                                                                                                     \
        {                                                                                                              \
            auto constexpr const msg_ = __FILE__ ":" CARB_STRINGIFY(__LINE__) /*": "  CARB_PRETTY_FUNCTION*/           \
                ": argument '" #ptr_ "' must not be nullptr";                                                          \
            throw omni::core::ResultError(omni::core::kResultInvalidArgument, msg_);                                   \
        }                                                                                                              \
    } while (0)
