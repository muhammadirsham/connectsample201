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
//! @brief Defines the base class for ABI-safe interfaces.
#pragma once

#include <omni/core/Assert.h>
#include <omni/core/TypeId.h>
#include <omni/core/OmniAttr.h>

#include <atomic>
#include <climits> // CHAR_BITS
#include <type_traits>

//! Main namespace for Omniverse.
namespace omni
{

//! Core functionality for Omniverse Interfaces.
namespace core
{

//! Result of an operation.
//!
//! See @ref OMNI_SUCCEEDED, @ref OMNI_FAILED, and @ref OMNI_RETURN_IF_FAILED for useful macros when dealing with these
//! result codes.
using Result OMNI_ATTR("constant, prefix=kResult") = int32_t;
constexpr Result kResultSuccess = 0; //!< Success.
constexpr Result kResultNotImplemented = 0x80004001; //!< Feature/Method was not implemented.
constexpr Result kResultNoInterface = 0x80004002; //!< Interface not implemented.
constexpr Result kResultNullPointer = 0x80004003; //!< Pointer is null
constexpr Result kResultOperationAborted = 0x80004004; //!< The operation was aborted.
constexpr Result kResultFail = 0x80004005; //!< The operation failed.
constexpr Result kResultAlreadyExists = 0x80030050; //!< Object already exists.
constexpr Result kResultNotFound = 0x80070002; //!< The item was not found.
constexpr Result kResultInvalidState = 0x80070004; //!< The system is not in a valid state to complete the operation.
constexpr Result kResultAccessDenied = 0x80070005; //!< Access denied.
constexpr Result kResultOutOfMemory = 0x8007000E; //!< System is out-of-memory.
constexpr Result kResultNotSupported = 0x80070032; //!< The operation is not supported.
constexpr Result kResultInvalidArgument = 0x80070057; //!< A supplied argument is invalid.
constexpr Result kResultVersionCheckFailure = 0x80070283; //!< Version check failure.
constexpr Result kResultVersionParseError = 0x80070309; //!< Failed to parse the version.
constexpr Result kResultInsufficientBuffer = 0x8007007A; //!< Insufficient buffer.
constexpr Result kResultTryAgain = 0x8007106B; //!< Try the operation again.
constexpr Result kResultInvalidOperation = 0x800710DD; //!< The operation is invalid.
constexpr Result kResultNoMoreItems = 0x8009002A; //!< No more items to return.
constexpr Result kResultInvalidIndex = 0x80091008; //!< Invalid index.
constexpr Result kResultNotEnoughData = 0x80290101; //!< Not enough data.
constexpr Result kResultTooMuchData = 0x80290102; //!< Too much data.
constexpr Result kResultInvalidDataType = 0x8031000B; //!< Invalid data type.
constexpr Result kResultInvalidDataSize = 0x8031000C; //!< Invalid data size.

//! Returns `true` if the given @ref omni::core::Result is not a failure code.
//!
//! `true` will be returned not only if the given result is @ref omni::core::kResultSuccess, but any other @ref
//! omni::core::Result that is not a failure code (such as warning @ref omni::core::Result codes).
#define OMNI_SUCCEEDED(x_) ((x_) >= 0)

//! Returns `true` if the given @ref omni::core::Result is a failure code.
#define OMNI_FAILED(x_) ((x_) < 0)

//! If the given @ref omni::core::Result is a failure code, calls `return result` to exit the current function.
#define OMNI_RETURN_IF_FAILED(x_)                                                                                      \
    do                                                                                                                 \
    {                                                                                                                  \
        auto result = (x_);                                                                                            \
        if (OMNI_FAILED(result))                                                                                       \
        {                                                                                                              \
            return result;                                                                                             \
        }                                                                                                              \
    } while (0)

// we assume 8-bit chars
static_assert(CHAR_BIT == 8, "non-octet char is not supported");

class IObject_abi;
class IObject;

//! Base class for all ABI-safe interfaces. Provides references counting and an ABI-safe `dynamic_cast` like mechanism.
//!
//! When defining a new interface, use the @ref Inherits template.
//!
//! When implementing one or more interfaces use the @ref omni::core::Implements template.
//!
//! See @oni_overview to understand the overall design of Omniverse Native Interfaces.
//!
//! @thread_safety All methods in this interface are thread safe.
class OMNI_ATTR("no_py") IObject_abi
{
public:
#ifndef DOXYGEN_SHOULD_SKIP_THIS
    //! Anonymous enum to store the type ID.
    enum : TypeId
    {
        kTypeId = OMNI_TYPE_ID("omni.core.IObject") //!< Uniquely identifies the @ref IObject_abi interface.
    };
#endif

protected:
    //! Returns a pointer to the interface defined by the given type id if this object implements the type id's
    //! interface.
    //!
    //! Objects can support multiple interfaces, even interfaces that are in different inheritance chains.
    //!
    //! The returned object will have @ref omni::core::IObject::acquire() called on it before it is returned, meaning it
    //! is up to the caller to call @ref omni::core::IObject::release() on the returned pointer.
    //!
    //! The returned pointer can be safely `reinterpret_cast<>` to the type id's C++ class.  For example,
    //! "omni.windowing.IWindow" can be cast to `omni::windowing::IWindow`.
    //!
    //! Do not directly use this method, rather use a wrapper function like @ref omni::core::cast() or @ref
    //! omni::core::ObjectPtr::as().
    //!
    //! @thread_safety This method is thread safe.
    virtual void* cast_abi(TypeId id) noexcept = 0;

    //! Increments the object's reference count.
    //!
    //! Objects may have multiple reference counts (e.g. one per interface implemented).  As such, it is important that
    //! you call @ref omni::core::IObject::release() on the same pointer from which you called @ref
    //! omni::core::IObject::acquire().
    //!
    //! Do not directly use this method, rather use @ref omni::core::ObjectPtr, which will manage calling @ref
    //! omni::core::IObject::acquire() and @ref omni::core::IObject::release() for you.
    //!
    //! @thread_safety This method is thread safe.
    virtual void acquire_abi() noexcept = 0;

    //! Decrements the objects reference count.
    //!
    //! Most implementations will destroy the object if the reference count reaches 0 (though this is not a
    //! requirement).
    //!
    //! Objects may have multiple reference counts (e.g. one per interface implemented).  As such, it is important that
    //! you call @ref omni::core::IObject::release() on the same pointer from which you called @ref
    //! omni::core::IObject::acquire().
    //!
    //! Do not directly use this method, rather use @ref omni::core::ObjectPtr, which will manage calling @ref
    //! omni::core::IObject::acquire() and @ref omni::core::IObject::release() for you.
    //!
    //! @thread_safety This method is thread safe.
    virtual void release_abi() noexcept = 0;
};

} // namespace core
} // namespace omni

//!  By defining this macro before including a header generated by *omni.bind*, only the declaration of any generated
//!  boiler-plate code is included.
//!
//! @see OMNI_BIND_INCLUDE_INTERACE_IMPL
#define OMNI_BIND_INCLUDE_INTERFACE_DECL
#include "IObject.gen.h"

namespace omni
{
namespace core
{

//! @copydoc omni::core::IObject_abi
class IObject : public omni::core::Generated<omni::core::IObject_abi>
{
};

//! Helper template for interface inheritance.
//!
//! Using this template defines compile time information used by @ref omni::core::Implements.
//!
//! Expected usage:
//!
//! @code{.cpp}
//!
//!   class IMyInterface : public omni::core::Inherits<omni::core::IObject, OMNI_TYPE_ID("IMyInterface")
//!   { /* ... */ };
//!
//! @endcode
template <typename BASE, TypeId TYPEID>
class Inherits : public BASE
{
public:
#ifndef DOXYGEN_BUILD
    //! Anonymous enum to store the type ID.
    enum : TypeId
    {
        kTypeId = TYPEID //!< The unique interface type of this object.
    };
    using BaseType = BASE; //!< Useful for @ref omni::core::Implements.
#endif
};

#ifndef DOXYGEN_BUILD
namespace details
{
//! Helper type used by @ref ObjectPtr.
class BorrowPtrType
{
public:
    explicit BorrowPtrType() noexcept = default;
};
} // namespace details
#endif

//! Used to create an @ref ObjectPtr that increments an objects reference count.
//!
//! @code{.cpp}
//!
//!  IMyType* raw = /* ... */;
//!  auto smart = ObjectPtr<IMyType>(myType, kBorrow);
//!
//! @endcode
//!
//! @ref ObjectPtr's rarely "borrow" raw pointers, rather they usually "steal" them (see @ref kSteal).
//!
//! See @ref omni::core::borrow().
constexpr details::BorrowPtrType kBorrow{};

#ifndef DOXYGEN_BUILD
namespace details
{
//! Helper type used by ObjectPtr.
class StealPtrType
{
public:
    explicit StealPtrType() noexcept = default;
};
} // namespace details
#endif

//! Used to create an @ref ObjectPtr that does not increments an objects reference count.  The @ref ObjectPtr does
//! decrement the reference count of the raw pointer upon the @ref ObjectPtr's destruction.
//!
//! @code
//!
//!  auto smart = ObjectPtr<IMyType>(createMyType, kSteal);
//!
//! @endcode
//!
//! Stealing a raw pointer is quite common when a function returns a raw interface pointer.
//!
//! See @ref omni::core::steal().
constexpr details::StealPtrType kSteal{};

//! Smart pointer wrapper around interface pointers.
//!
//! This object manages the mundane details of managing the given objects reference count.
//!
//! There is no implicit raw pointer to @ref ObjectPtr conversion.  Such a conversion is ambiguous, as it is unclear if
//! the object's reference count should be immediately incremented. Rather, use @ref omni::core::steal() and @ref
//! omni::core::borrow() to create an @ref ObjectPtr.
//!
//! Use @ref get() to return the raw object pointer.  The pointer will still be managed by this wrapper.
//!
//! Use @ref detach() to return and stop managing the raw pointer.  When calling @ref detach(), @ref
//! omni::core::IObject::release() will not be called.
//!
//! Use @ref release() to decrement the raw pointer's reference count and stop managing the raw pointer.
//!
//! @rst
//!
//! .. warning::
//!     ``ObjectPtr::release()`` does not have the same meaning as ``std::unique_ptr::release()``.
//!     ``std::unique_ptr::release()`` is equivalent to ``ObjectPtr::detach()``.
//!
//! @endrst
//!
//! Use @ref as() to cast the pointer to another interface.
//!
//! Unless otherwise stated, the managed pointer can be `nullptr`.
//!
//! @thread_safety All methods are thread safe.
template <typename T>
class ObjectPtr
{
public:
    //! Allow implicit conversion from `nullptr` to an @ref ObjectPtr.
    constexpr ObjectPtr(std::nullptr_t = nullptr) noexcept
    {
    }

    //! Never call this method as it will terminate the application.
    //!
    //! This method is currently needed to satisfy `PyBind`.  Note, `PyBind` will never actually call this method but
    //! will fail with compiler errors if it is not present.
    //!
    //! OM-18948: Update PyBind to not require a raw pointer to "holder" constructor.
    explicit ObjectPtr(T* other) : m_ptr(other)
    {
        // if this assertion hits, something is amiss in our bindings (or PyBind was updated)
        OMNI_FATAL_UNLESS(false, "look at what you've done PyBind!");
    }

    //! Start managing the given raw pointer.  @ref omni::core::IObject::acquire() will be called on the pointer.
    //!
    //! Prefer using @ref omni::core::borrow() over this constructor.
    ObjectPtr(T* other, details::BorrowPtrType) noexcept : m_ptr(other)
    {
        addRef();
    }

    //! Start managing the given raw pointer.  @ref omni::core::IObject::acquire() will *not* be called on the pointer.
    //!
    //! Prefer using @ref omni::core::steal() over this constructor.
    constexpr ObjectPtr(T* other, details::StealPtrType) noexcept : m_ptr(other)
    {
    }

    //! Copy constructor.
    ObjectPtr(const ObjectPtr& other) noexcept : m_ptr(other.m_ptr)
    {
        addRef();
    }

    //! Copy constructor.
    template <typename U>
    ObjectPtr(const ObjectPtr<U>& other) noexcept : m_ptr(other.m_ptr)
    {
        addRef();
    }

    //! Move constructor.
    template <typename U>
    ObjectPtr(ObjectPtr<U>&& other) noexcept : m_ptr(std::exchange(other.m_ptr, {}))
    {
    }

    //! Destructor.  Calls @ref release() on the managed pointer.
    ~ObjectPtr() noexcept
    {
        releaseRef();
    }

    //! Assignment operator.
    ObjectPtr& operator=(const ObjectPtr& other) noexcept
    {
        copyRef(other.m_ptr);
        return *this;
    }

    //! Move operator.
    ObjectPtr& operator=(ObjectPtr&& other) noexcept
    {
        if (this != &other)
        {
            releaseRef();
            m_ptr = std::exchange(other.m_ptr, {});
        }

        return *this;
    }

    //! Assignment operator.
    template <typename U>
    ObjectPtr& operator=(const ObjectPtr<U>& other) noexcept
    {
        copyRef(other.m_ptr);
        return *this;
    }

    //! Move operator.
    template <typename U>
    ObjectPtr& operator=(ObjectPtr<U>&& other) noexcept
    {
        releaseRef();
        m_ptr = std::exchange(other.m_ptr, {});
        return *this;
    }

    //! Returns true if the managed pointer is not `nullptr`.
    explicit operator bool() const noexcept
    {
        return m_ptr != nullptr;
    }

    //! The managed pointer must not be `nullptr`.
    T* operator->() const noexcept
    {
        return m_ptr;
    }

    //! The managed pointer must not be `nullptr`.
    T& operator*() const noexcept
    {
        return *m_ptr;
    }

    //! Returns the raw pointer.  The pointer is still managed by this wrapper.
    //!
    //! This method is useful when having to pass raw pointers to ABI functions.
    T* get() const noexcept
    {
        return m_ptr;
    }

    //! Returns a pointer to the managed pointer (which must be `nullptr`).
    //!
    //! The managed pointer must be `nullptr`, otherwise the function results in undefined behavior.
    //!
    //! Useful when having to manage pointers output via a function argument list.
    //!
    //! @code{.cpp}
    //!
    //!  void createMyType(MyType** out);
    //!
    //!  // ...
    //!
    //!  ObjectPtr<MyType> ptr;
    //!  createMyType(ptr.put());
    //!
    //! @endcode
    //!
    //! Such methods are rare.
    T** put() noexcept
    {
        OMNI_ASSERT(m_ptr == nullptr);
        return &m_ptr;
    }

    //! Manage the given pointer.  @ref omni::core::IObject::acquire() is not called on the pointer.
    //!
    //! See @ref borrow() for a method that does call @ref omni::core::IObject::acquire().
    void steal(T* value) noexcept
    {
        releaseRef();
        *put() = value;
    }

    //! Returns the managed pointer and no longer manages the pointer.
    //!
    //! @ref omni::core::IObject::release() is *not* called on the pointer.  Use this method to stop managing the
    //! pointer.
    T* detach() noexcept
    {
        return std::exchange(m_ptr, {});
    }

    //! Manage the given pointer.  @ref omni::core::IObject::acquire() is called on the pointer.
    //!
    //! See @ref steal() for a method that does not call @ref omni::core::IObject::acquire().
    void borrow(T* value) noexcept
    {
        releaseRef();
        *put() = value;
        addRef();
    }

    //! Cast the managed pointer to a new interface type (@p To).
    //!
    //! `nullptr` is returned if the pointer does not implement the interface.
    template <typename To>
    ObjectPtr<To> as() const noexcept
    {
        if (!m_ptr)
        {
            return nullptr; // dynamic_cast allows a nullptr, so we do as well
        }
        else
        {
            return ObjectPtr<To>(reinterpret_cast<To*>(m_ptr->cast(To::kTypeId)), kSteal);
        }
    }

    //! Cast the managed pointer to the type of the given @ref ObjectPtr (e.g. @p To).
    //!
    //! `nullptr` is written to @p to if the pointer does not implement the interface.
    template <typename To>
    void as(ObjectPtr<To>& to) const noexcept
    {
        if (!m_ptr)
        {
            to.steal(nullptr); // dynamic_cast allows a nullptr, so we do as well
        }
        else
        {
            to.steal(reinterpret_cast<To*>(m_ptr->cast(To::kTypeId)));
        }
    }

    //! Calls @ref release() on the managed pointer and sets the internal pointer to `nullptr`.
    //!
    //! @rst
    //!
    //! .. warning::
    //!     ``ObjectPtr::release()`` does not have the same meaning as ``std::unique_ptr::release()``.
    //!     ``std::unique_ptr::release()`` is equivalent to ``ObjectPtr::detach()``.
    //!
    //! @endrst
    void release() noexcept
    {
        releaseRef();
    }

    //! Calls @ref release() on the managed pointer and sets the internal pointer to @p value
    //!
    //! Equivalent to `std::unique_ptr::reset()`.
    //!
    //! @param value The new value to assign to `*this`, defaults to `nullptr`.
    void reset(T* value = nullptr) noexcept
    {
        if (value)
        {
            const_cast<std::remove_const_t<T>*>(value)->acquire();
        }

        T* oldval = std::exchange(m_ptr, value);

        if (oldval)
        {
            oldval->release();
        }
    }

private:
    void copyRef(T* other) noexcept
    {
        if (m_ptr != other)
        {
            releaseRef();
            m_ptr = other;
            addRef();
        }
    }

    void addRef() const noexcept
    {
        if (m_ptr)
        {
            const_cast<std::remove_const_t<T>*>(m_ptr)->acquire();
        }
    }

    void releaseRef() noexcept
    {
        if (m_ptr)
        {
            std::exchange(m_ptr, {})->release();
        }
    }

    template <typename U>
    friend class ObjectPtr;

    T* m_ptr{};
};

// Breathe/Sphinx is unable to handle these overloads and produces warnings.  Since we don't like warnings, remove these
// overloads from the docs until Breathe/Sphinx is updated.
#ifndef DOXYGEN_SHOULD_SKIP_THIS

//! @ref ObjectPtr less than operator.
template <typename T>
inline bool operator<(const ObjectPtr<T>& left, const ObjectPtr<T>& right) noexcept
{
    return (left.get() < right.get());
}

//! @ref ObjectPtr equality operator.
template <typename T>
inline bool operator==(const ObjectPtr<T>& left, const ObjectPtr<T>& right) noexcept
{
    return (left.get() == right.get());
}

//! @ref ObjectPtr equality operator (with raw pointer).
template <typename T>
inline bool operator==(const ObjectPtr<T>& left, const T* right) noexcept
{
    return (left.get() == right);
}

//! @ref ObjectPtr equality operator (with raw pointer).
template <typename T>
inline bool operator==(const T* left, const ObjectPtr<T>& right) noexcept
{
    return (left == right.get());
}

//! @ref ObjectPtr equality operator (with nullptr).
template <typename T>
inline bool operator==(const ObjectPtr<T>& left, std::nullptr_t) noexcept
{
    return (left.get() == nullptr);
}

//! @ref ObjectPtr equality operator (with nullptr).
template <typename T>
inline bool operator==(std::nullptr_t, const ObjectPtr<T>& right) noexcept
{
    return (right.get() == nullptr);
}

//! @ref ObjectPtr inequality operator.
template <typename T>
inline bool operator!=(const ObjectPtr<T>& left, const ObjectPtr<T>& right) noexcept
{
    return (left.get() != right.get());
}

//! @ref ObjectPtr inequality operator (with raw pointer).
template <typename T>
inline bool operator!=(const ObjectPtr<T>& left, const T* right) noexcept
{
    return (left.get() != right);
}

//! @ref ObjectPtr inequality operator (with raw pointer).
template <typename T>
inline bool operator!=(const T* left, const ObjectPtr<T>& right) noexcept
{
    return (left != right.get());
}

//! @ref ObjectPtr inequality operator (with nullptr).
template <typename T>
inline bool operator!=(const ObjectPtr<T>& left, std::nullptr_t) noexcept
{
    return (left.get() != nullptr);
}

//! @ref ObjectPtr inequality operator (with nullptr).
template <typename T>
inline bool operator!=(std::nullptr_t, const ObjectPtr<T>& right) noexcept
{
    return (right.get() != nullptr);
}

#endif // DOXYGEN_SHOULD_SKIP_THIS

//! Returns an @ref ObjectPtr managing the given pointer. @ref omni::core::IObject::acquire() is **not** called on the
//! pointer.
//!
//! `nullptr` is accepted.
template <typename T>
inline ObjectPtr<T> steal(T* ptr) noexcept
{
    return ObjectPtr<T>(ptr, kSteal);
}

//! Returns an @ref ObjectPtr managing the given pointer. @ref omni::core::IObject::acquire() is called on the
//! pointer.
//!
//! `nullptr` is accepted.
template <typename T>
inline ObjectPtr<T> borrow(T* ptr) noexcept
{
    return ObjectPtr<T>(ptr, kBorrow);
}

//! Casts the given pointer to the given interface (e.g. T).
//!
//! `nullptr` is accepted.
//!
//! @returns A valid pointer is returned if the given pointer implements the given interface.  Otherwise, `nullptr` is
//! returned.
template <typename T, typename U>
inline ObjectPtr<T> cast(U* ptr) noexcept
{
    static_assert(std::is_base_of<IObject, T>::value, "cast can only be used with classes that derive from IObject");
    if (ptr)
    {
        return ObjectPtr<T>{ reinterpret_cast<T*>(ptr->cast(T::kTypeId)), kSteal };
    }
    else
    {
        return { nullptr };
    }
}

#ifndef DOXYGEN_BUILD
namespace details
{
template <typename T>
inline void* cast(T* obj, TypeId id) noexcept; // forward declaration
} // namespace details
#endif

//! Helper template for implementing the cast function for one or more interfaces.
//!
//! Implementations of interfaces (usually hidden in <i>.cpp</i> files) are well served to use this template.
//!
//! This template provides the following useful feature:
//!
//! - It provides a @ref omni::core::IObject_abi::cast_abi() implementation that supports mutiple inheritance.
//!
//! Using @ref omni::core::ImplementsCast is recommended in cases where you want the default implementation
//! of the cast function, but want to override the behavior of @ref omni::core::IObject_abi::acquire_abi() and @ref
//! omni::core::IObject_abi::release_abi(). If the default implementation of cast, acquire, and release functions is
//! desired, then using @ref Implements is recommended.
//!
//! A possible usage implementing your own acquire/release semantics:
//!
//! @code{.cpp}
//!
//!   class MyIFooAndIBarImpl : public omni::core::ImplementsCast<IFoo, IBar>
//!   {
//!   public:
//!       //! See @ref omni::core::IObject::acquire.
//!       inline void acquire() noexcept
//!       {
//!           // note: this implementation is needed to disambiguate which `cast` to call when using multiple
//!           // inheritance. it has zero-overhead.
//!           static_cast<IFoo*>(this)->acquire();
//!       }
//!
//!       //! See @ref omni::core::IObject::release.
//!       inline void release() noexcept
//!       {
//!           // note: this implementation is needed to disambiguate which `cast` to call when using multiple
//!           // inheritance. it has zero-overhead.
//!           static_cast<IFoo*>(this)->release();
//!       }
//!   protected:
//!       std::atomic<uint32_t> m_refCount{ 1 }; //!< Reference count.
//!
//!       //! @copydoc IObject_abi::acquire_abi
//!       virtual void acquire_abi() noexcept override
//!       {
//!           uint32_t count = m_refCount.fetch_add(1, std::memory_order_relaxed) + 1;
//!           CARB_LOG_INFO("Increased count to %u", count);
//!       }
//!
//!       //! @copydoc IObject_abi::release_abi
//!       virtual void release_abi() noexcept override
//!       {
//!           uint32_t count = m_refCount.fetch_sub(1, std::memory_order_release) - 1;
//!           CARB_LOG_INFO("Reduced count to %u", count);
//!           if (0 == count)
//!           {
//!               std::atomic_thread_fence(std::memory_order_acquire);
//!               delete this;
//!           }
//!       }
//!
//!       /* ... */
//!   };
//!
//! @endcode
template <typename T, typename... Rest>
struct ImplementsCast : public T, public Rest...
{
public:
    //! See @ref omni::core::IObject::cast.
    inline void* cast(omni::core::TypeId id) noexcept
    {
        // note: this implementation is needed to disambiguate which `cast` to call when using multiple inheritance. it
        // has zero-overhead.
        return static_cast<T*>(this)->cast(id);
    }

private:
    // given a type id, castImpl() check if the type id matches T's typeid.  if not, T's parent class type id is
    // checked. if T's parent class type id does not match, the grandparent class's type id is check.  this continues
    // until IObject's type id is checked.
    //
    // if no type id in T's inheritance chain match, the next interface in Rest is checked.
    //
    // it's expected the compiler can optimize away the recursion
    template <typename U, typename... Args>
    inline void* castImpl(TypeId id) noexcept
    {
        // details::cast will march down the inheritance chain
        void* obj = omni::core::details::cast<U>(this, id);
        if (nullptr == obj)
        {
            // check the next class (inheritance chain) provide in the inheritance list
            return castImpl<Args...>(id);
        }

        return obj;
    }

    // this terminates walking across the types in the variadic template
    template <int = 0>
    inline void* castImpl(TypeId) noexcept
    {
        return nullptr;
    }

protected:
    virtual ~ImplementsCast() noexcept = default;

    //! @copydoc omni::core::IObject_abi::cast_abi
    virtual void* cast_abi(TypeId id) noexcept override
    {
        return castImpl<T, Rest...>(id);
    }
};


//! Helper template for implementing one or more interfaces.
//!
//! Implementations of interfaces (usually hidden in <i>.cpp</i> files) are well served to use this template.
//!
//! This template provides two useful features:
//!
//! - It provides a reference count and reasonable implementations of @ref omni::core::IObject_abi::acquire_abi() and
//!   @ref omni::core::IObject_abi::release_abi().
//!
//! - It provides a @ref omni::core::IObject_abi::cast_abi() implementation that supports mutiple inheritance.
//!
//! Using @ref omni::core::Implements is recommended in most cases when implementing one or more interfaces.
//!
//! Expected usage:
//!
//! @code{.cpp}
//!
//!   class MyIFooAndIBarImpl : public omni::core::Implements<IFoo, IBar>
//!   { /* ... */ };
//!
//! @endcode
template <typename T, typename... Rest>
struct Implements : public ImplementsCast<T, Rest...>
{
public:
    //! See @ref omni::core::IObject::acquire.
    inline void acquire() noexcept
    {
        // note: this implementation is needed to disambiguate which `cast` to call when using multiple inheritance. it
        // has zero-overhead.
        static_cast<T*>(this)->acquire();
    }

    //! See @ref omni::core::IObject::release.
    inline void release() noexcept
    {
        // note: this implementation is needed to disambiguate which `cast` to call when using multiple inheritance. it
        // has zero-overhead.
        static_cast<T*>(this)->release();
    }

protected:
    std::atomic<uint32_t> m_refCount{ 1 }; //!< Reference count.

    virtual ~Implements() noexcept = default;

    //! @copydoc omni::core::IObject_abi::acquire_abi
    virtual void acquire_abi() noexcept override
    {
        m_refCount.fetch_add(1, std::memory_order_relaxed);
    }

    //! @copydoc omni::core::IObject_abi::release_abi
    virtual void release_abi() noexcept override
    {
        if (0 == m_refCount.fetch_sub(1, std::memory_order_release) - 1)
        {
            std::atomic_thread_fence(std::memory_order_acquire);
            delete this;
        }
    }
};

#ifndef DOXYGEN_BUILD
namespace details
{
//! Given a type, this function walks the inheritance chain for the type, checking if the id of the type matches the
//! given id.
//!
//! Implementation detail.  Do not call.
template <typename T>
inline void* cast(T* obj, TypeId id) noexcept
{
    if (T::kTypeId == id)
    {
        obj->acquire(); // match! since we return an interface pointer, acquire() must be called.
        return obj;
    }
    else
    {
        return cast<typename T::BaseType>(obj, id); // call cast again, but with the parent type
    }
}

//! Specialization of `cast<T>(T*, TypeId)` for @ref omni::core::IObject. @ref omni::core::IObject always terminates the
//! recursive template since it does not have a base class.
//!
//! Implementation detail.  Do not call.
template <>
inline void* cast<IObject>(IObject* obj, TypeId id) noexcept
{
    if (IObject::kTypeId == id)
    {
        obj->acquire();
        return obj;
    }
    else
    {
        return nullptr;
    }
}
} // namespace details
#endif

} // namespace core
} // namespace omni

//!  By defining this macro before including a header generated by *omni.bind*, only the implementations of any
//!  generated boiler-plate code is included.
//!
//! @see OMNI_BIND_INCLUDE_INTERACE_DECL
#define OMNI_BIND_INCLUDE_INTERFACE_IMPL
#include "IObject.gen.h"
