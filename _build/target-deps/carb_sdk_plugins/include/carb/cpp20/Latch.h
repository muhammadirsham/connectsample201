// Copyright (c) 2019-2022, NVIDIA CORPORATION. All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto. Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.
//
// Implements a C++20-standard Latch. From the spec:
// A latch is a thread coordination mechanism that allows any number of threads to block until an expected
// number of threads arrive at the latch (via the count_down function). The expected count is set when the
// latch is created. An individual latch is a single-use object; once the expected count has been reached, the
// latch cannot be reused.
// 1. A latch maintains an internal counter that is initialized when the latch is created. Threads can block on the
//    latch object, waiting for counter to be decremented to zero.
// 2. Concurrent invocations of the member functions of latch, other than its destructor, do not introduce data
//    races.
#pragma once

#include "Atomic.h"

#include <algorithm>
#include <thread>

namespace carb
{
namespace cpp20
{

// Handle case where Windows.h may have defined 'max'
#pragma push_macro("max")
#undef max

class latch
{
    CARB_PREVENT_COPY_AND_MOVE(latch);

public:
    static constexpr ptrdiff_t max() noexcept
    {
        return ptrdiff_t(UINT_MAX);
    }

    // Public constructor. Initializes counter with `expected`
    constexpr explicit latch(ptrdiff_t expected) noexcept : m_counter(uint32_t(::carb_min(max(), expected)))
    {
        CARB_ASSERT(expected >= 0 && expected <= max());
    }
    ~latch() noexcept
    {
        // Wait until we have no waiters
        while (m_waiters.load(std::memory_order_acquire) != 0)
            std::this_thread::yield();
    }

    // Atomically decrements the counter with `update`. If the counter is equal to zero, unblocks all threads blocked on
    // *this.
    void count_down(ptrdiff_t update = 1) noexcept
    {
        CARB_ASSERT(update >= 0);

        // `fetch_sub` returns value before operation
        uint32_t count = m_counter.fetch_sub(uint32_t(update), std::memory_order_release);
        CARB_CHECK((count - uint32_t(update)) <= count); // Malformed if we go below zero or overflow
        if ((count - uint32_t(update)) == 0)
        {
            // Wake all waiters
            m_counter.notify_all();
        }
    }

    // Returns whether the latch has completed. Allowed to return spuriously false with very low probability.
    bool try_wait() const noexcept
    {
        return m_counter.load(std::memory_order_acquire) == 0;
    }

    // If counter equals zero, returns immediately. Otherwise, blocks on *this until a call to `count_down` that
    // decrements counter to zero.
    void wait() const noexcept
    {
        uint32_t count = m_counter.load(std::memory_order_acquire);
        if (count != 0)
        {
            // Register as a waiter
            m_waiters.fetch_add(1, std::memory_order_relaxed);
            _wait(count);
        }
    }

    // Equivalent to:
    // ```
    // count_down(update);
    // wait();
    // ```
    void arrive_and_wait(ptrdiff_t update = 1) noexcept
    {
        uint32_t original = m_counter.load(std::memory_order_acquire);
        if (original == uint32_t(update))
        {
            // We're the last and won't be waiting.
#if CARB_ASSERT_ENABLED
            uint32_t updated = m_counter.exchange(0, std::memory_order_release);
            CARB_ASSERT(updated == original);
#else
            m_counter.store(0, std::memory_order_release);
#endif

            // Wake all waiters
            m_counter.notify_all();
            return;
        }

        // Speculatively register as a waiter
        m_waiters.fetch_add(1, std::memory_order_relaxed);

        original = m_counter.fetch_sub(uint32_t(update), std::memory_order_release);
        if (CARB_UNLIKELY(original == uint32_t(update)))
        {
            // Wake all waiters and unregister as a waiter
            m_counter.notify_all();
            m_waiters.fetch_sub(1, std::memory_order_release);
        }
        else
        {
            CARB_CHECK(original >= uint32_t(update)); // Malformed if we underflow
            _wait(original - uint32_t(update));
        }
    }

private:
    mutable cpp20::atomic_uint32_t m_counter;
    mutable cpp20::atomic_uint32_t m_waiters{ 0 };

    CARB_ALWAYS_INLINE void _wait(uint32_t count) const noexcept
    {
        CARB_ASSERT(count != 0);
        do
        {
            m_counter.wait(count, std::memory_order_relaxed);
            count = m_counter.load(std::memory_order_acquire);
        } while (count != 0);

        // Done waiting
        m_waiters.fetch_sub(1, std::memory_order_release);
    }
};

#pragma pop_macro("max")

} // namespace cpp20
} // namespace carb
