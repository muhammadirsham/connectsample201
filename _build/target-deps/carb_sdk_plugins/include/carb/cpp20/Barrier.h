// Copyright (c) 2019-2022, NVIDIA CORPORATION. All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto. Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.
//
// Implements a C++20-standard Barrier. From the spec:
// A barrier is a thread coordination mechanism whose lifetime consists of a sequence of barrier phases, where
// each phase allows at most an expected number of threads to block until the expected number of threads
// arrive at the barrier. A barrier is useful for managing repeated tasks that are handled by multiple
// threads.
#pragma once

#include "Atomic.h"
#include "Bit.h"

#include <utility>

namespace carb
{
namespace cpp20
{

namespace details
{

constexpr uint32_t kInvalidPhase = 0;

struct NullFunction
{
    constexpr NullFunction() noexcept = default;
    constexpr void operator()() noexcept
    {
    }
};

} // namespace details

// Handle case where Windows.h may have defined 'max'
#pragma push_macro("max")
#undef max

template <class CompletionFunction = details::NullFunction>
class barrier
{
    CARB_PREVENT_COPY_AND_MOVE(barrier);

public:
    static constexpr ptrdiff_t max() noexcept
    {
        return ptrdiff_t(INT_MAX);
    }

    constexpr explicit barrier(ptrdiff_t expected, CompletionFunction f = CompletionFunction{})
        : m_emo(InitBoth{}, std::move(f), (uint64_t(1) << kPhaseBitShift) + uint32_t(::carb_min(expected, max()))),
          m_expected(uint32_t(::carb_min(expected, max())))
    {
        CARB_ASSERT(expected >= 0 && expected <= max());
    }
    ~barrier()
    {
        // Wait for destruction until all waiters are clear
        while (m_waiters.load(std::memory_order_acquire) != 0)
            std::this_thread::yield();
    }

    class arrival_token
    {
        CARB_PREVENT_COPY(arrival_token);
        friend class barrier;
        uint32_t m_token{ details::kInvalidPhase };
        arrival_token(uint32_t token) : m_token(token)
        {
        }

    public:
        arrival_token() = default;
        arrival_token(arrival_token&& rhs) : m_token(std::exchange(rhs.m_token, details::kInvalidPhase))
        {
        }
        arrival_token& operator=(arrival_token&& rhs)
        {
            m_token = std::exchange(rhs.m_token, details::kInvalidPhase);
        }
    };

    CARB_NODISCARD arrival_token arrive(ptrdiff_t update = 1)
    {
        return arrival_token(uint32_t(_arrive(update).first >> kPhaseBitShift));
    }

    void wait(arrival_token&& arrival) const
    {
        // Precondition: arrival is associated with the phase synchronization point for the current phase or the
        // immediately preceding phase.
        CARB_CHECK(arrival.m_token != 0); // No invalid tokens
        uint64_t data = m_emo.second.load(std::memory_order_acquire);
        uint32_t phase = uint32_t(data >> kPhaseBitShift);
        CARB_CHECK((phase - arrival.m_token) <= 1, "arrival %u is not the previous or current phase %u",
                   arrival.m_token, phase);

        if (phase != arrival.m_token)
            return;

        // Register as a waiter
        m_waiters.fetch_add(1, std::memory_order_relaxed);

        do
        {
            // Wait for the phase to change
            m_emo.second.wait(data, std::memory_order_relaxed);

            // Reload after waiting
            data = m_emo.second.load(std::memory_order_acquire);
            phase = uint32_t(data >> kPhaseBitShift);
        } while (phase == arrival.m_token);

        // Unregister as a waiter
        m_waiters.fetch_sub(1, std::memory_order_release);
    }

    void arrive_and_wait()
    {
        // Two main differences over just doing arrive(wait()):
        // - We return immediately if _arrive() did the phase shift
        // - We don't CARB_CHECK that the phase is the current or preceding one since it is guaranteed
        auto result = _arrive(1);
        if (result.second)
            return;

        // Register as a waiter
        m_waiters.fetch_add(1, std::memory_order_relaxed);

        uint64_t data = result.first;
        uint32_t origPhase = uint32_t(data >> kPhaseBitShift), phase;
        do
        {
            // Wait for the phase to change
            m_emo.second.wait(data, std::memory_order_relaxed);

            // Reload after waiting
            data = m_emo.second.load(std::memory_order_acquire);
            phase = uint32_t(data >> kPhaseBitShift);
        } while (phase == origPhase);

        // Unregister as a waiter
        m_waiters.fetch_sub(1, std::memory_order_release);
    }

    void arrive_and_drop()
    {
        uint32_t prev = m_expected.fetch_sub(1, std::memory_order_relaxed);
        CARB_CHECK(prev != 0); // Precondition failure: expected count for the current barrier phase must be greater
                               // than zero.

        _arrive(1);
    }

private:
    constexpr static int kPhaseBitShift = 32;
    constexpr static uint64_t kCounterMask = 0xffffffffull;

    CARB_ALWAYS_INLINE std::pair<uint64_t, bool> _arrive(ptrdiff_t update)
    {
        CARB_CHECK(update > 0 && update <= max());

        uint64_t pre = m_emo.second.fetch_sub(uint32_t(update), std::memory_order_acq_rel);
        CARB_CHECK(ptrdiff_t(int32_t(uint32_t(pre & kCounterMask))) >= update); // Precondition check

        bool completed = false;
        if (uint32_t(pre & kCounterMask) == uint32_t(update))
        {
            // Phase is now complete
            std::atomic_thread_fence(std::memory_order_acquire);
            _completePhase(pre - uint32_t(update));
            completed = true;
        }

        return std::make_pair(pre - uint32_t(update), completed);
    }

    void _completePhase(uint64_t data)
    {
        uint32_t expected = m_expected.load(std::memory_order_relaxed);

        // Run the completion routine before releasing threads
        m_emo.first()();

        // Increment the phase and don't allow the invalid phase
        uint32_t phase = uint32_t(data >> kPhaseBitShift);
        if (++phase == details::kInvalidPhase)
            ++phase;

#if CARB_ASSERT_ENABLED
        // Should not have changed during completion function.
        uint64_t old = m_emo.second.exchange((uint64_t(phase) << kPhaseBitShift) + expected, std::memory_order_release);
        CARB_ASSERT(old == data);
#else
        m_emo.second.store((uint64_t(phase) << kPhaseBitShift) + expected, std::memory_order_release);
#endif

        // Release all waiting threads
        m_emo.second.notify_all();
    }

    // The MSB 32 bits of the atomic_uint64_t are the Phase; the other bits are the Counter
    EmptyMemberPair<CompletionFunction, cpp20::atomic_uint64_t> m_emo;
    std::atomic_uint32_t m_expected;
    mutable std::atomic_uint32_t m_waiters{ 0 };
};

#pragma pop_macro("max")

} // namespace cpp20
} // namespace carb
