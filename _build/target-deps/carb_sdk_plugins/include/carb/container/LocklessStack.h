// Copyright (c) 2020-2022, NVIDIA CORPORATION. All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto. Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.
//

//! @file
//!
//! @brief Defines the LocklessStack class.
#pragma once

#include "../Defines.h"
#include "../cpp20/Atomic.h"
#include "../thread/Mutex.h"
#include "../thread/Util.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <thread>

#if CARB_POSIX
#    include <dlfcn.h>
#endif

//! If 1, we try to do double-compare-and-swap; otherwise we do a single compare-and-swap and encode the sequence by
//! stealing some of the pointer bits. If 1 but we don't have DCAS (ARM) then we fall back to using std::mutex
//! Note that DCAS requires `-march=skylake` for GCC/x86_64. This is done below with a pragma
//! @private
#define DO_DCAS 0

#if CARB_COMPILER_MSC
#    define NO_DCAS 0
extern "C" unsigned char _InterlockedCompareExchange128(__int64 volatile* _Destination,
                                                        __int64 _ExchangeHigh,
                                                        __int64 _ExchangeLow,
                                                        __int64* _ComparandResult);
#    pragma intrinsic(_InterlockedCompareExchange128)
#elif CARB_COMPILER_GNUC
//! Indicates that double-compare-and-swap is not available on this platform.
//! @private
#    define NO_DCAS (CARB_AARCH64)
#    if DO_DCAS && !NO_DCAS
#        pragma GCC target("arch=skylake")
#    endif
#else
#    error Unsupported compiler
#endif

#if DO_DCAS && NO_DCAS
#    include <condition_variable>
#    include <mutex>
#endif

namespace carb
{
namespace container
{

template <class T>
class LocklessStackLink;
template <class T, LocklessStackLink<T> T::*U>
class LocklessStack;

#ifndef DOXYGEN_BUILD
namespace details
{
template <class T, LocklessStackLink<T> T::*U>
class LocklessStackHelpers;
template <class T, LocklessStackLink<T> T::*U>
class LocklessStackBase;
} // namespace details
#endif

/**
 * Defines the link object. Each class contained in LocklessStack must have a member of type LocklessStackLink<T>. A
 * pointer to this member is required as the second parameter for LocklessStack.
 */
template <class T>
class LocklessStackLink
{
public:
    /**
     * Default constructor.
     */
    constexpr LocklessStackLink() = default;

private:
    LocklessStackLink<T>* m_next;

    friend T;
    template <class U, LocklessStackLink<U> U::*V>
    friend class details::LocklessStackHelpers;
    template <class U, LocklessStackLink<U> U::*V>
    friend class details::LocklessStackBase;
    template <class U, LocklessStackLink<U> U::*V>
    friend class LocklessStack;
};

#ifndef DOXYGEN_BUILD
namespace details
{

#    if DO_DCAS && !NO_DCAS || !DO_DCAS
#        define LOCKLESSSTACK_HAS_SIGNALHANDLER 1 // For tests
#        if CARB_COMPILER_GNUC

#            if CARB_PLATFORM_LINUX
// Section start and stop locations for the LocklessStackDetails section automatically created by the linker.
extern "C" unsigned char __start_LocklessStackDetails[];
extern "C" unsigned char __stop_LocklessStackDetails[];
#            endif

class SignalHandler
{
public:
    SignalHandler()
    {
        ensure();
    }
    ~SignalHandler()
    {
        // Restore
        sigaction(SIGSEGV, old(), nullptr);
        // Release reference
        auto prevHandle = std::exchange(oldHandle(), nullptr);
        if (prevHandle)
        {
            dlclose(prevHandle);
        }
    }

#            if CARB_PLATFORM_LINUX
    // This function is *very specifically* always no-inline, maximum optimizations without a frame pointer, and placed
    // in a special section. This is so `Handler()` can gracefully recover from a very specific SIGSEGV in this
    // function.
    static bool __attribute__((noinline, optimize(3, "omit-frame-pointer"), section("LocklessStackDetails")))
    readNext(void** out, void* in)
    {
        // The asm for this function SHOULD be (x86_64):
        //   mov rax, qword ptr [rsi]
        //   mov qword ptr [rdi], rax
        //   mov rax, 1
        //   ret
        // And for ARMv8.1:
        //   ldr     x1, [x1]
        //   str     x1, [x0]
        //   mov     w0, 1
        //   ret
        // Very important that the first instruction is the one that can fail, and there are no stack operations.

        // This is all done very carefully because we expect that there is a very rare chance that the first operation
        // could cause SIGSEGV if another thread has already modified the stack. We handle that signal with zero cost
        // unless it happens. On Windows, this is done much simpler since SEH exists, but on Linux we have to very
        // carefully handle with some creative signal handling.
        *out = *(void**)in;
        return true;
    }
#            elif CARB_PLATFORM_MACOS
    // not that we could use __attribute__((section("__TEXT,LocklessStackDet")))
    // but mach-o doesn't have any sort of section start/end markers so there's
    // not much benefit to this. Also note that the section name can only be 16
    // characters.
    // Instead of trying to use sections, we can just create a naked function
    // (e.g. the function will contain exactly the asm we provide), so that we
    // can manually calculate our function start/end offsets.
    // Note that we can't create naked functions on Linux because our GCC version
    // is too old.
    CARB_IGNOREWARNING_GNUC_WITH_PUSH("-Wunused-parameter");
    static bool __attribute__((naked, noinline)) readNext(void** out, void* in)
    {
#                if CARB_X86_64
        asm("mov    (%rsi),%rax\n"
            "mov    %rax,(%rdi)\n"
            "mov    $0x1,%eax\n"
            "retq\n");
#                elif CARB_AARCH64
        asm("ldr     x1, [x1]\n"
            "str     x1, [x0]\n"
            "mov     w0, #0x1\n"
            "ret\n");
#                else
        CARB_UNSUPPORTED_ARCHITECTURE();
#                endif
    }
    CARB_IGNOREWARNING_GNUC_POP
#            else
    CARB_UNSUPPORTED_PLATFORM();
#            endif

#            if DO_DCAS
    using uint128_t = __int128 unsigned;
    static bool compare_exchange_strong(void volatile* destination, void* expected, const void* newval)
    {
        uint128_t val = __sync_val_compare_and_swap_16(
            destination, *reinterpret_cast<uint128_t*>(expected), *reinterpret_cast<const uint128_t*>(newval));
        if (val != *reinterpret_cast<uint128_t*>(expected))
        {
            *reinterpret_cast<uint128_t*>(expected) = val;
            return false;
        }
        return true;
    }
#            endif
    static SignalHandler& signalHandler()
    {
        static SignalHandler handler;
        return handler;
    }

    static void EnsureRegistered()
    {
        signalHandler().ensure();
    }

private:
    // Ensure that this SignalHandler is the first signal handler
    void ensure()
    {
        struct sigaction oldAction, newAction;
        newAction.sa_flags = SA_SIGINFO;
        sigemptyset(&newAction.sa_mask);
        newAction.sa_sigaction = &Handler;

        sigaction(SIGSEGV, &newAction, &oldAction);
        if (oldAction.sa_sigaction != &Handler)
        {
            if (!(oldAction.sa_handler == SIG_DFL || oldAction.sa_handler == SIG_IGN))
            {
                // If the old handler is a function, add a reference to the module that contains it.
                Dl_info info{};
                if (dladdr(reinterpret_cast<void*>(oldAction.sa_handler), &info))
                {
                    oldHandle() = dlopen(info.dli_fname, RTLD_NOW | RTLD_NOLOAD);
                }
            }
            *old() = oldAction;
        }
    }

    static struct sigaction* old()
    {
        static struct sigaction oldAction;
        return &oldAction;
    }

    static void*& oldHandle()
    {
        static void* oldHandleValue{};
        return oldHandleValue;
    }

    static void Handler(int sig, siginfo_t* info, void* ctx)
    {
        ucontext_t* context = reinterpret_cast<ucontext_t*>(ctx);
        // RegType is the underlying type for registers on the platform
#            if CARB_PLATFORM_LINUX && CARB_X86_64
        using RegType = greg_t;
#            elif CARB_PLATFORM_LINUX && CARB_AARCH64
        using RegType = __u64;
#            elif CARB_PLATFORM_MACOS
        using RegType = __uint64_t;
#            else
        CARB_UNSUPPORTED_PLATFORM();
#            endif

        // collect the start and the end of the segment containing readNext()
#            if CARB_PLATFORM_LINUX
        const RegType segmentStart = RegType(&__start_LocklessStackDetails);
        const RegType segmentEnd = RegType(&__stop_LocklessStackDetails);
#            elif CARB_PLATFORM_MACOS && CARB_X86_64
        const RegType segmentStart = RegType(&readNext);
        const RegType segmentEnd = segmentStart + 0xC;
#            elif CARB_PLATFORM_MACOS && CARB_AARCH64
        const RegType segmentStart = RegType(&readNext);
        const RegType segmentEnd = segmentStart + 0x10;
#            else
        CARB_UNSUPPORTED_PLATFORM();
#            endif

        // helper to validate that we are actually jumping to a return statement
#            if CARB_X86_64
        size_t returnOffset = 1;
        auto validateReturn = [](const void* addr) -> bool { return *static_cast<const uint8_t*>(addr) == 0xC3; };
#            elif CARB_AARCH64
        size_t returnOffset = sizeof(uint32_t);
        auto validateReturn = [](const void* addr) -> bool { return *static_cast<const uint32_t*>(addr) == 0xd65f03c0; };
#            else
        CARB_UNSUPPORTED_ARCHITECTURE();
#            endif

        // collect the accumulator and instruction pointer registers
#            if CARB_PLATFORM_LINUX && CARB_X86_64
        RegType& accumulator = context->uc_mcontext.gregs[REG_RAX];
        RegType& ip = context->uc_mcontext.gregs[REG_RIP];
#            elif CARB_PLATFORM_LINUX && CARB_AARCH64
        RegType& accumulator = context->uc_mcontext.regs[0];
        RegType& ip = context->uc_mcontext.pc;
#            elif CARB_PLATFORM_MACOS && CARB_X86_64
        RegType& accumulator = context->uc_mcontext->__ss.__rax;
        RegType& ip = context->uc_mcontext->__ss.__rip;
#            elif CARB_PLATFORM_MACOS && CARB_AARCH64
        RegType& accumulator = context->uc_mcontext->__ss.__x[0];
        RegType& ip = context->uc_mcontext->__ss.__pc;
#            else
        CARB_UNSUPPORTED_PLATFORM();
#            endif

        if (ip == RegType(&readNext))
        {
            // The crash happened where we expected it to, on the first instruction of readNext(). Handle gracefully.
            accumulator = 0; // Return value to false
            // Set to the last instruction of readData. ARM64 instructions are 32 bits.  It should be the return
            // instruction.
            ip = segmentEnd - returnOffset;
            CARB_FATAL_UNLESS(validateReturn(reinterpret_cast<const void*>(ip)), "Must be a return instruction");
            return;
        }
        // If this is the case, then either we crashed at a different location within the function, or there is some
        // prologue code generated by the compiler. In the last case, we can't just forward `rip` to the ret instruction
        // because any stack operations wouldn't be undone.
        CARB_FATAL_UNLESS(
            !(ip >= segmentStart && ip < segmentEnd), "SIGSEGV in expected function but not at expected location!");

        // Chain to previous handler if one exists
        struct sigaction* prev = old();
        if (CARB_UNLIKELY(prev->sa_handler == SIG_IGN)) // SIG_IGN: ignore signal
            return;
        else if (prev->sa_handler != SIG_DFL) // SIG_DFL: default signal handler
            prev->sa_sigaction(sig, info, ctx);
        else
        {
            // We need to call the default handler, which means we need to remove our handler and restore the previous
            // handler, then raise again. Since this is a SIGSEGV and we haven't handled it, returning should cause the
            // SIGSEGV from the same location.
            sigaction(SIGSEGV, prev, nullptr);
        }
    }
};
#        else
class SignalHandler
{
public:
    static constexpr void EnsureRegistered()
    {
    }

    static bool readNext(void** out, void* in)
    {
        // We do this in a SEH block (on Windows) because it's possible (though rare) that another thread could have
        // already popped and destroyed `cur` which would cause EXCEPTION_ACCESS_VIOLATION. By handling it in an
        // exception handler, we recover cleanly and try again. On 64-bit Windows, there is zero cost unless an
        // exception is thrown, at which point the kernel will find the Exception info and Unwind Info for the function
        // that we're in.
        __try
        {
            *out = *(void**)in;
            return true;
        }
        __except (1)
        {
            return false;
        }
    }
#            if DO_DCAS
    static bool compare_exchange_strong(void volatile* destination, void* expected, const void* newval)
    {
        return !!_InterlockedCompareExchange128(
            reinterpret_cast<__int64 volatile*>(destination), reinterpret_cast<const __int64*>(newval)[1],
            reinterpret_cast<const __int64*>(newval)[0], reinterpret_cast<__int64*>(expected));
    }
#            endif
};
#        endif
#    else
#        define LOCKLESSSTACK_HAS_SIGNALHANDLER 0 // For tests
#    endif

template <class T, LocklessStackLink<T> T::*U>
class LocklessStackHelpers
{
public:
    // Access the LocklessStackLink member of `p`
    static LocklessStackLink<T>* link(T* p)
    {
        return std::addressof(p->*U);
    }

    // Converts a LocklessStackLink to the containing object
    static T* convert(LocklessStackLink<T>* p)
    {
        // We need to calculate the offset of our link member and calculate where T is.
        // Note that this doesn't work if T uses virtual inheritance
        size_t offset = (size_t) reinterpret_cast<char*>(&(((T*)0)->*U));
        return reinterpret_cast<T*>(reinterpret_cast<char*>(p) - offset);
    }
};

// Base implementations
#    if DO_DCAS && NO_DCAS
// Told to use DCAS but DCAS is not supported, so cannot be lockless. Uses a mutex internally as a baseline
// implementation.
template <class T, LocklessStackLink<T> T::*U>
class LocklessStackBase : protected LocklessStackHelpers<T, U>
{
    using Base = LocklessStackHelpers<T, U>;

public:
    bool _isEmpty() const
    {
        return !m_head;
    }

    bool _push(T* first, T* last)
    {
        std::lock_guard<Lock> g(m_lock);
        Base::link(last)->m_next = m_head;
        bool const wasEmpty = !m_head;
        m_head = Base::link(first);
        return wasEmpty;
    }

    T* _popOne()
    {
        std::unique_lock<Lock> g(m_lock);
        auto cur = m_head;
        if (!cur)
        {
            return nullptr;
        }
        m_head = cur->m_next;
        return Base::convert(cur);
    }

    T* _popAll()
    {
        std::lock_guard<Lock> g(m_lock);
        LocklessStackLink<T>* head = m_head;
        m_head = nullptr;
        return head ? Base::convert(head) : nullptr;
    }

    void _wait()
    {
        std::unique_lock<Lock> g(m_lock);
        m_cv.wait(g, [this] { return m_head != nullptr; });
    }

    template <class Rep, class Period>
    bool _waitFor(const std::chrono::duration<Rep, Period>& dur)
    {
        std::unique_lock<Lock> g(m_lock);
        return m_cv.wait_for(g, dur, [this] { return m_head != nullptr; });
    }

    template <class Clock, class Duration>
    bool _waitUntil(const std::chrono::time_point<Clock, Duration>& tp)
    {
        std::unique_lock<Lock> g(m_lock);
        return m_cv.wait_until(g, tp, [this] { return m_head != nullptr; });
    }

    void _notifyOne()
    {
        m_cv.notify_one();
    }

    void _notifyAll()
    {
        m_cv.notify_all();
    }

private:
    // Cannot be lockless if we don't have DCAS
    // Testing reveals that mutex is significantly faster than spinlock in highly-contended cases.
    using Lock = carb::thread::mutex;
    Lock m_lock;
    LocklessStackLink<T>* m_head{ nullptr };
    std::condition_variable_any m_cv;
};
#    elif DO_DCAS
// DCAS (Double Compare-And-Swap) implementation. Must be 16-byte aligned on 64-bit hardware.
template <class T, LocklessStackLink<T> T::*U>
class alignas(16) LocklessStackBase : protected LocklessStackHelpers<T, U>
{
    using Base = LocklessStackHelpers<T, U>;

public:
    constexpr LocklessStackBase()
    {
        SignalHandler::EnsureRegistered();
    }
    bool _isEmpty() const
    {
        return !m_impl.head;
    }

    bool _push(T* first, T* last)
    {
        Impl expected, temp;
        memcpy(&expected, const_cast<Impl*>(&m_impl), sizeof(m_impl)); // relaxed
        do
        {
            memcpy(&temp, &expected, sizeof(Impl));
            Base::link(last)->m_next = temp.head;
            temp.alignment += size_t(0x10001); // increment both count and sequence
            temp.head = Base::link(first);
        } while (CARB_UNLIKELY(!SignalHandler::compare_exchange_strong(&m_impl, &expected, &temp)));
        return !expected.head;
    }

    T* _popOne()
    {
        Impl expected, temp;
        memcpy(&expected, const_cast<Impl*>(&m_impl), sizeof(m_impl)); // Relaxed copy
        LocklessStackLink<T>* cur;
        for (;;)
        {
            memcpy(&temp, &expected, sizeof(m_impl));
            cur = temp.head;
            if (!cur)
            {
                return nullptr;
            }

            // Attempt to read the next value
            if (!details::SignalHandler::readNext((void**)&temp.head, cur))
            {
                // Another thread must have changed `cur`, so reload and try again.
                memcpy(&expected, const_cast<Impl*>(&m_impl), sizeof(m_impl)); // Relaxed copy
                continue;
            }

            --temp.alignment;
            if (CARB_LIKELY(details::SignalHandler::compare_exchange_strong(&m_impl, &expected, &temp)))
            {
                return Base::convert(cur);
            }
        }
    }

    T* _popAll()
    {
        // There is no 128-bit atomic exchange, so we must keep trying until we succeed.
        Impl empty, expected;
        memcpy(&expected, const_cast<Impl*>(&m_impl), sizeof(Impl)); // relaxed
        for (;;)
        {
            if (!expected.head)
            {
                return nullptr;
            }
            empty.sequence = expected.sequence; // Keep the same sequence, because the count will be moving to null
            if (CARB_LIKELY(details::SignalHandler::compare_exchange_strong(&m_impl, &expected, &empty)))
            {
                return Base::convert(expected.head);
            }
        }
    }

    void _wait()
    {
        AtomicRef(head()).wait(nullptr, std::memory_order_relaxed);
    }

    template <class Rep, class Period>
    bool _waitFor(const std::chrono::duration<Rep, Period>& dur)
    {
        return AtomicRef(head()).wait_for(nullptr, dur, std::memory_order_relaxed);
    }

    template <class Clock, class Duration>
    bool _waitUntil(const std::chrono::time_point<Clock, Duration>& tp)
    {
        return AtomicRef(head()).wait_until(nullptr, tp, std::memory_order_relaxed);
    }

    void _notifyOne()
    {
        AtomicRef(head()).notify_one();
    }

    void _notifyAll()
    {
        AtomicRef(head()).notify_all();
    }

private:
    using AtomicRef = cpp20::atomic_ref<LocklessStackLink<T>*>;
    union alignas(16) Impl
    {
        struct
        {
            size_t alignment;
            size_t region;
        };
        struct
        {
            size_t count : 16; // Since it's limited to 16 bits, this is not a full count.
            size_t sequence : (8 * sizeof(size_t) - 16);
            LocklessStackLink<T>* head;
        };
        constexpr Impl() : alignment(0), region(0)
        {
        }
    };

    Impl volatile m_impl{};

    LocklessStackLink<T>*& head()
    {
        return const_cast<Impl&>(m_impl).head;
    }
};
#    else
// Preferred implementation: doesn't require DCAS, but relies upon the fact that aligned pointers on modern OSes don't
// use at least 10 bits of the 64-bit space, so it uses those bits as a sequence number to ensure uniqueness between
// different threads competing to pop.
template <class T, LocklessStackLink<T> T::*U>
class LocklessStackBase : protected LocklessStackHelpers<T, U>
{
    using Base = LocklessStackHelpers<T, U>;

public:
    constexpr LocklessStackBase()
    {
        SignalHandler::EnsureRegistered();
    }
    bool _isEmpty() const
    {
        return !decode(m_head.load(std::memory_order_acquire));
    }

    bool _push(T* first, T* last)
    {
        // All OS bits should either be zero or one, and it needs to be 8-byte-aligned.
        LocklessStackLink<T>* lnk = Base::link(first);
        CARB_ASSERT((size_t(lnk) & kCPUMask) == 0 || (size_t(lnk) & kCPUMask) == kCPUMask, "Unexpected OS bits set");
        CARB_ASSERT((size_t(lnk) & ((1 << 3) - 1)) == 0, "Pointer not aligned properly");

        uint16_t seq;
        uint64_t expected = m_head.load(std::memory_order_acquire), temp;
        decltype(lnk) next;
        do
        {
            next = decode(expected, seq);
            Base::link(last)->m_next = next;
            temp = encode(lnk, seq + 1); // Increase sequence
        } while (CARB_UNLIKELY(
            !m_head.compare_exchange_strong(expected, temp, std::memory_order_release, std::memory_order_relaxed)));
        return !next;
    }

    T* _popOne()
    {
        uint64_t expected = m_head.load(std::memory_order_acquire);
        LocklessStackLink<T>* cur;
        uint16_t seq;

        bool isNull = false;

        this_thread::spinWaitWithBackoff([&] {
            cur = decode(expected, seq);
            if (!cur)
            {
                // End attempts because the stack is empty
                isNull = true;
                return true;
            }

            // Attempt to read the next value
            LocklessStackLink<T>* newhead;
            if (!details::SignalHandler::readNext((void**)&newhead, cur))
            {
                // Another thread changed `cur`, so reload and try again.
                expected = m_head.load(std::memory_order_acquire);
                return false;
            }

            // Only push needs to increase `seq`
            uint64_t temp = encode(newhead, seq);
            return m_head.compare_exchange_strong(expected, temp, std::memory_order_release, std::memory_order_relaxed);
        });

        return isNull ? nullptr : Base::convert(cur);
    }

    T* _popAll()
    {
        uint16_t seq;
        uint64_t expected = m_head.load(std::memory_order_acquire), temp;
        for (;;)
        {
            LocklessStackLink<T>* head = decode(expected, seq);
            if (!head)
            {
                return nullptr;
            }

            // Keep the same sequence since only push() needs to increment the sequence
            temp = encode(nullptr, seq);
            if (CARB_LIKELY(
                    m_head.compare_exchange_weak(expected, temp, std::memory_order_release, std::memory_order_relaxed)))
            {
                return Base::convert(head);
            }
        }
    }

    void _wait()
    {
        uint64_t head = m_head.load(std::memory_order_acquire);
        while (!decode(head))
        {
            m_head.wait(head, std::memory_order_relaxed);
            head = m_head.load(std::memory_order_acquire);
        }
    }

    template <class Rep, class Period>
    bool _waitFor(const std::chrono::duration<Rep, Period>& dur)
    {
        return _waitUntil(std::chrono::steady_clock::now() + dur);
    }

    template <class Clock, class Duration>
    bool _waitUntil(const std::chrono::time_point<Clock, Duration>& tp)
    {
        uint64_t head = m_head.load(std::memory_order_acquire);
        while (!decode(head))
        {
            if (!m_head.wait_until(head, tp, std::memory_order_relaxed))
            {
                return false;
            }
            head = m_head.load(std::memory_order_acquire);
        }
        return true;
    }

    void _notifyOne()
    {
        m_head.notify_one();
    }

    void _notifyAll()
    {
        m_head.notify_all();
    }

private:
    // On 64-bit architectures, we make use of the fact that CPUs only use a certain number of address bits.
    // Intel CPUs require that these 8 to 16 most-significant-bits match (all 1s or 0s). Since 8 appears to be the
    // lowest common denominator, we steal 7 bits (to save the value of one of the bits so that they can match) for a
    // sequence number. The sequence is important as it causes the resulting stored value to change even if the stack is
    // pushing and popping the same value.
    //
    // Pointer compression drops the `m` and `z` bits from the pointer. `m` are expected to be consistent (all 1 or 0)
    // and match the most-significant `P` bit. `z` are expected to be zeros:
    //
    // 63 ------------------------------ BITS ------------------------------ 0
    // mmmmmmmP PPPPPPPP PPPPPPPP PPPPPPPP PPPPPPPP PPPPPPPP PPPPPPPP PPPPPzzz
    //
    // `m_head` is encoded as the shifted compressed pointer bits `P` with sequence bits `s`:
    // 63 ------------------------------ BITS ------------------------------ 0
    // PPPPPPPP PPPPPPPP PPPPPPPP PPPPPPPP PPPPPPPP PPPPPPPP PPPPPPss ssssssss

    static_assert(sizeof(size_t) == 8, "64-bit only");
    constexpr const static size_t kCPUBits = 7; // MSBs that are limited by CPU hardware and must match the 56th bit
    constexpr const static size_t kCPUMask = ((size_t(1) << (kCPUBits + 1)) - 1) << (63 - kCPUBits);
    constexpr const static size_t kSeqBits = kCPUBits + 3; // We also use the lowest 3 bits as part of the sequence
    constexpr const static size_t kSeqMask = (size_t(1) << kSeqBits) - 1;
    cpp20::atomic_uint64_t m_head{ 0 };

    static LocklessStackLink<T>* decode(size_t val)
    {
        // Clear the `s` sequence bits and shift as a signed value to sign-extend so that the `m` bits are filled in to
        // match the most-significant `P` bit.
        return reinterpret_cast<LocklessStackLink<T>*>(ptrdiff_t(val & ~kSeqMask) >> kCPUBits);
    }
    static LocklessStackLink<T>* decode(size_t val, uint16_t& seq)
    {
        seq = val & kSeqMask;
        return decode(val);
    }
    static size_t encode(LocklessStackLink<T>* p, uint16_t seq)
    {
        // Shift the pointer value, dropping the most significant `m` bits and write the sequence number over the `z`
        // and space created in the least-significant area.
        return ((reinterpret_cast<size_t>(p) << kCPUBits) & ~kSeqMask) + (seq & uint16_t(kSeqMask));
    }
};
#    endif

} // namespace details
#endif

/**
 * @brief Implements a lockless stack: a LIFO container that is thread-safe yet requires no kernel involvement.
 *
 * LocklessStack is designed to be easy-to-use. For a class `Foo` that you want to be contained in a LocklessStack, it
 * must have a member of type LocklessStackLink<Foo>. This member is what the LocklessStack will use for tracking data.
 *
 * Pushing to LocklessStack is simply done through LocklessStack::push(), which is entirely thread-safe. LocklessStack
 * ensures last-in-first-out (LIFO) for each producer pushing to LocklessStack. Multiple producers may be pushing to
 * LocklessStack simultaneously, so their items can become mingled, but each producer's pushed items will be strongly
 * ordered.
 *
 * Popping is done through LocklessStack::pop(), which is also entirely thread-safe. Multiple threads may all attempt to
 * pop from the same LocklessStack simultaneously.
 *
 * Simple example:
 * ```cpp
 * class Foo
 * {
 * public:
 *     LocklessStackLink<Foo> m_link;
 * };
 *
 * LocklessStack<Foo, &Foo::m_link> stack;
 * stack.push(new Foo);
 * Foo* p = stack.pop();
 * delete p;
 * ```
 *
 * @thread_safety LocklessStack is entirely thread-safe except where declared otherwise. No allocation happens with a
 * LocklessStack; instead the caller is responsible for construction/destruction of contained objects.
 *
 * @tparam T The type to contain.
 * @tparam U A pointer-to-member of a LocklessStackLink member within T (see above example).
 */
template <class T, LocklessStackLink<T> T::*U>
class LocklessStack final : protected details::LocklessStackBase<T, U>
{
    using Base = details::LocklessStackBase<T, U>;

public:
    /**
     * Constructor.
     */
    constexpr LocklessStack() = default;

    /**
     * Destructor.
     *
     * Asserts that isEmpty() returns true.
     */
    ~LocklessStack()
    {
        // Ensure the stack is empty
        CARB_ASSERT(isEmpty());
    }

    /**
     * Indicates whether the stack is empty.
     *
     * @warning Another thread may have modified the LocklessStack before this function returns.
     *
     * @returns `true` if the stack appears empty; `false` if items appear to exist in the stack.
     */
    bool isEmpty() const
    {
        return Base::_isEmpty();
    }

    /**
     * Pushes an item onto the stack.
     *
     * @param p The item to push onto the stack.
     *
     * @return `true` if the stack was previously empty prior to push; `false` otherwise. Note that this is atomically
     * correct as opposed to checking isEmpty() before push().
     */
    bool push(T* p)
    {
        return Base::_push(p, p);
    }

    /**
     * Pushes a contiguous block of entries from [ @p begin, @p end) onto the stack.
     *
     * @note All of the entries are guaranteed to remain strongly ordered and will not be interspersed with entries from
     * other threads. @p begin will be popped from the stack first.
     *
     * @param begin An <a href="https://en.cppreference.com/w/cpp/named_req/InputIterator">InputIterator</a> of the
     * first item to push. `*begin` must resolve to a `T&`.
     * @param end An off-the-end InputIterator after the last item to push.
     * @returns `true` if the stack was empty prior to push; `false` otherwise. Note that this is atomically correct
     * as opposed to calling isEmpty() before push().
     */
#ifndef DOXYGEN_BUILD
    template <class InputItRef,
              std::enable_if_t<std::is_convertible<decltype(std::declval<InputItRef&>()++, *std::declval<InputItRef&>()), T&>::value,
                               bool> = false>
#else
    template <class InputItRef>
#endif
    bool push(InputItRef begin, InputItRef end)
    {
        if (begin == end)
        {
            return false;
        }

        // Walk the list and have them point to each other
        InputItRef last = begin;
        InputItRef iter = begin;
        for (iter++; iter != end; last = iter++)
        {
            Base::link(std::addressof(*last))->m_next = Base::link(std::addressof(*iter));
        }

        return Base::_push(std::addressof(*begin), std::addressof(*last));
    }

    /**
     * Pushes a block of pointers-to-entries from [ @p begin, @p end) onto the stack.
     *
     * @note All of the entries are guaranteed to remain strongly ordered and will not be interspersed with entries from
     * other threads. @p begin will be popped from the stack first.
     *
     * @param begin An <a href="https://en.cppreference.com/w/cpp/named_req/InputIterator">InputIterator</a> of the
     * first item to push. `*begin` must resolve to a `T*`.
     * @param end An off-the-end InputIterator after the last item to push.
     * @returns `true` if the stack was empty prior to push; `false` otherwise. Note that this is atomically correct
     * as opposed to calling isEmpty() before push().
     */
#ifndef DOXYGEN_BUILD
    template <class InputItPtr,
              std::enable_if_t<std::is_convertible<decltype(std::declval<InputItPtr&>()++, *std::declval<InputItPtr&>()), T*>::value,
                               bool> = true>
#else
    template <class InputItPtr>
#endif
    bool push(InputItPtr begin, InputItPtr end)
    {
        if (begin == end)
        {
            return false;
        }

        // Walk the list and have them point to each other
        InputItPtr last = begin;
        InputItPtr iter = begin;
        for (iter++; iter != end; last = iter++)
        {
            Base::link(*last)->m_next = Base::link(*iter);
        }

        return Base::_push(*begin, *last);
    }

    /**
     * Pops an item from the top of the stack if available.
     *
     * @return An item popped from the stack. If the stack was empty, then `nullptr` is returned.
     */
    T* pop()
    {
        return Base::_popOne();
    }

    /**
     * Empties the stack.
     *
     * @note To perform an action on each item as it is popped, use forEach() instead.
     */
    void popAll()
    {
        Base::_popAll();
    }

    /**
     * Pops all available items from the stack calling a functionish object on each.
     *
     * First, pops all available items from `*this` and then calls @p f on each.
     *
     * @note As the pop is the first thing that happens, any new entries that get pushed while the function is executing
     * will NOT be popped and will remain in the stack when this function returns.
     *
     * @param f A functionish object that accepts a `T*` parameter. Called for each item that was popped from the stack.
     */
    template <class Func>
    void forEach(Func&& f)
    {
        T* p = Base::_popAll();
        LocklessStackLink<T>* h = p ? Base::link(p) : nullptr;
        while (h)
        {
            p = Base::convert(h);
            h = h->m_next;
            f(p);
        }
    }

    /**
     * Pushes an item onto the stack and notifies a waiting listener.
     *
     * Equivalent to doing `auto b = push(p); notifyOne(); return b;`.
     *
     * @see push(), notifyOne()
     *
     * @param p The item to push.
     * @returns `true` if the stack was empty prior to push; `false` otherwise. Note that this is atomically correct
     * as opposed to calling isEmpty() before push().
     */
    bool pushNotify(T* p)
    {
        bool b = push(p);
        notifyOne();
        return b;
    }

    /**
     * Blocks the calling thread until an item is available and returns it.
     *
     * Requires the item to be pushed with pushNotify(), notifyOne() or notifyAll().
     *
     * @see pop(), wait()
     *
     * @returns The first item popped from the stack.
     */
    T* popWait()
    {
        T* p = pop();
        while (!p)
        {
            wait();
            p = pop();
        }
        return p;
    }

    /**
     * Blocks the calling thread until an item is available and returns it or a timeout elapses.
     *
     * Requires the item to be pushed with pushNotify(), notifyOne() or notifyAll().
     *
     * @see pop(), waitFor()
     *
     * @param dur The duration to wait for an item to become available.
     * @returns the first item removed from the stack or `nullptr` if the timeout period elapses.
     */
    template <class Rep, class Period>
    T* popWaitFor(const std::chrono::duration<Rep, Period>& dur)
    {
        return popWaitUntil(std::chrono::steady_clock::now() + dur);
    }

    /**
     * Blocks the calling thread until an item is available and returns it or the clock reaches a time point.
     *
     * Requires the item to be pushed with pushNotify(), notifyOne() or notifyAll().
     *
     * @see pop(), waitUntil()
     *
     * @param tp The time to wait until for an item to become available.
     * @returns the first item removed from the stack or `nullptr` if the timeout period elapses.
     */
    template <class Clock, class Duration>
    T* popWaitUntil(const std::chrono::time_point<Clock, Duration>& tp)
    {
        T* p = pop();
        while (!p)
        {
            if (!waitUntil(tp))
            {
                return pop();
            }
            p = pop();
        }
        return p;
    }

    /**
     * Waits until the stack is non-empty.
     *
     * @note Requires notification that the stack is non-empty, such as from pushNotify(), notifyOne() or notifyAll().
     *
     * @note Though wait() returns, another thread may have popped the available item making the stack empty again. Use
     * popWait() if it is desired to ensure that the current thread can obtain an item.
     */
    void wait()
    {
        Base::_wait();
    }

    /**
     * Waits until the stack is non-empty or a specified duration has passed.
     *
     * @note Though waitFor() returns `true`, another thread may have popped the available item making the stack empty
     * again. Use popWaitFor() if it is desired to ensure that the current thread can obtain an item.
     *
     * @note Requires notification that the stack is non-empty, such as from pushNotify(), notifyOne() or notifyAll().
     *
     * @param dur The duration to wait for an item to become available.
     * @returns `true` if an item appears to be available; `false` if the timeout elapses.
     */
    template <class Rep, class Period>
    bool waitFor(const std::chrono::duration<Rep, Period>& dur)
    {
        return Base::_waitFor(dur);
    }

    /**
     * Waits until the stack is non-empty or a specific time is reached.
     *
     * @note Though waitUntil() returns `true`, another thread may have popped the available item making the stack empty
     * again. Use popWaitUntil() if it is desired to ensure that the current thread can obtain an item.
     *
     * @note Requires notification that the stack is non-empty, such as from pushNotify(), notifyOne() or notifyAll().
     *
     * @param tp The time to wait until for an item to become available.
     * @returns `true` if an item appears to be available; `false` if the time is reached.
     */
    template <class Clock, class Duration>
    bool waitUntil(const std::chrono::time_point<Clock, Duration>& tp)
    {
        return Base::_waitUntil(tp);
    }

    /**
     * Notifies a single waiting thread.
     *
     * Notifies a single thread waiting in wait(), waitFor(), waitUntil(), popWait(), popWaitFor(), or popWaitUntil() to
     * wake and check the stack.
     */
    void notifyOne()
    {
        Base::_notifyOne();
    }

    /**
     * Notifies all waiting threads.
     *
     * Notifies all threads waiting in wait(), waitFor(), waitUntil(), popWait(), popWaitFor(), or popWaitUntil() to
     * wake and check the stack.
     */
    void notifyAll()
    {
        Base::_notifyAll();
    }
};

} // namespace container
} // namespace carb

// undef things we no longer need
#undef NO_DCAS
#undef DO_DCAS
