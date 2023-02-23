// Copyright (c) 2020-2022, NVIDIA CORPORATION. All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto. Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.
//
#pragma once

#include <omni/core/IObject.h>
#include <omni/log/ILog.h>
#include <carb/IObject.h>
#include <carb/BindingsPythonUtils.h>

#include <pybind11/pybind11.h>
#include <pybind11/functional.h>

#include <sstream>
#include <vector>

namespace py = pybind11;

// tell pybind the smart pointer it should use to manage or interfaces
PYBIND11_DECLARE_HOLDER_TYPE(T, omni::core::ObjectPtr<T>, true);

// See comment block for DISABLE_PYBIND11_DYNAMIC_CAST for an explanation.
namespace pybind11
{
template <typename itype>
struct polymorphic_type_hook<itype, detail::enable_if_t<std::is_base_of<omni::core::IObject, itype>::value>>
{
    static const void* get(const itype* src, const std::type_info*&)
    {
        return src;
    }
};
} // namespace pybind11

namespace omni
{
namespace python
{

//! Specialize this class to define hand-written bindings for T.
template <typename T>
struct PyBind
{
    template <typename S>
    static S& bind(S& s)
    {
        return s;
    }
};

//! Given a pointer, pokes around in pybind's internals to see if there's already a PyObject managing the pointer.  This
//! can be used to avoid data copies.
inline bool hasPyObject(const void* p)
{
    auto& instances = py::detail::get_internals().registered_instances;
    return (instances.end() != instances.find(p));
}

//! Converts a C value to Python.  Makes a copy if needed.
//!
//! The default template (this template) handles:
//!  - pointer to primitive type (int, float, etc.)
//!  - pointer to interface
//!  - primitive value (int, float, etc.)
template <typename T,
          bool isInterface = std::is_base_of<omni::core::IObject, typename std::remove_pointer<T>::type>::value,
          bool isStruct = std::is_class<typename std::remove_pointer<T>::type>::value>
class ValueToPython
{
public:
    explicit ValueToPython(T orig) : m_orig{ orig }
    {
    }
    T get()
    {
        return m_orig;
    }

private:
    T m_orig;
};

//! Specialization of ValueToPython
//!
//! This specialization handles:
//!
//!  - pointer to struct
template <typename T>
class ValueToPython<T, /*interface*/ false, /*struct*/ true>
{
public:
    using Type = typename std::remove_pointer<T>::type;

    explicit ValueToPython(Type* orig)
    {
        m_orig = orig;
        if (!hasPyObject(orig))
        {
            m_copy.reset(new Type{ *orig });
        }
    }

    Type* getData()
    {
        if (m_copy)
        {
            return m_copy.get();
        }
        else
        {
            return m_orig;
        }
    }

private:
    Type* m_orig;
    std::unique_ptr<Type> m_copy;
};

template <typename T,
          bool isPointer = std::is_pointer<T>::value,
          bool isInterface = std::is_base_of<omni::core::IObject, typename std::remove_pointer<T>::type>::value,
          bool isStruct = std::is_class<typename std::remove_pointer<T>::type>::value>
struct PyCopy
{
    // code generator shouldn't get here
};

// primitive value
template <typename T>
struct PyCopy<T, false, false, false>
{
    static py::object toPython(T cObj)
    {
        return py::cast(cObj);
    }

    static T fromPython(py::object pyObj)
    {
        return pyObj.cast<T>();
    }
};

// pointer to struct or primitive
template <typename T, bool isStruct>
struct PyCopy<T, /*isPointer=*/true, false, isStruct>
{
    /*
    static py::object toPython(T cObj)
    {
        return py::cast(cObj);
    }
    */

    static void fromPython(T out, py::object pyObj)
    {
        *out = *(pyObj.cast<T>());
    }
};

// struct
template <typename T>
struct PyCopy<T, false, false, true>
{
    static py::object toPython(const T& cObj)
    {
        return py::cast(cObj);
    }

    static const T& fromPython(py::object pyObj)
    {
        return *pyObj.cast<T*>();
    }
};

template <typename T,
          bool elementIsPointer = std::is_pointer<typename std::remove_pointer<T>::type>::value,
          bool elementIsInterfacePointer =
              std::is_base_of<omni::core::IObject, typename std::remove_pointer<typename std::remove_pointer<T>::type>::type>::value,
          bool elementIsStruct = std::is_class<typename std::remove_pointer<T>::type>::value>
struct PyArrayCopy
{
    using ItemType = typename std::remove_const<typename std::remove_pointer<T>::type>::type;

    static py::object toPython(T in, uint32_t count)
    {
        py::tuple out(count);
        for (uint32_t i = 0; i < count; ++i)
        {
            out[i] = PyCopy<ItemType>::toPython(in[i]);
        }

        return out;
    }

    static void fromPython(T out, uint32_t count, py::sequence in)
    {
        if (count != uint32_t(in.size()))
        {
            std::ostringstream msg;
            msg << "expected " << count << " elements in the sequence, python returned " << int32_t(in.size());
            throw std::runtime_error(msg.str());
        }

        T dst = out;
        for (const auto& elem : in)
        {
            PyCopy<T>::fromPython(dst, elem);
            ++dst;
        }
    }
};

template <typename T, bool elementIsInterfacePointer, bool elementIsStruct>
struct PyArrayCopy<T, /*elementIsPointer=*/true, elementIsInterfacePointer, elementIsStruct>
{
    // TODO: handle array of pointers
};

template <>
struct PyArrayCopy<const char* const*, /*elementIsPointer=*/true, /*elementIsInterfacePointer=*/false, /*elementIsStruct=*/false>
{
    static py::object toPython(const char* const* in, uint32_t count)
    {
        py::tuple out(count);
        for (uint32_t i = 0; i < count; ++i)
        {
            out[i] = py::str(in[i]);
        }

        return out;
    }
};

template <typename T>
class ArrayToPython
{
public:
    using ItemType = typename std::remove_const<typename std::remove_pointer<T>::type>::type;

    ArrayToPython(T src, uint32_t sz) : m_tuple{ PyArrayCopy<T>::toPython(src, sz) }
    {
    }

    py::tuple& getPyObject()
    {
        return m_tuple;
    }

private:
    py::tuple m_tuple;
};

template <typename T>
class ArrayFromPython
{
public:
    using ItemType = typename std::remove_const<typename std::remove_pointer<T>::type>::type;

    ArrayFromPython(py::sequence seq)
    {
        m_data.reserve(seq.size());
        for (auto pyObj : seq)
        {
            m_data.emplace_back(PyCopy<ItemType>::fromPython(pyObj));
        }
    }

    T getData()
    {
        return m_data.data();
    }

    uint32_t getCount() const
    {
        return uint32_t(m_data.size());
    }

    py::tuple createPyObject()
    {
        py::tuple out{ m_data.size() };
        for (size_t i = 0; i < m_data.size(); ++i)
        {
            out[i] = PyCopy<ItemType>::toPython(m_data[i]);
        }

        return out;
    }

private:
    std::vector<ItemType> m_data;
};

template <typename T,
          bool isInterface = std::is_base_of<omni::core::IObject, typename std::remove_pointer<T>::type>::value,
          bool isStruct = std::is_class<typename std::remove_pointer<T>::type>::value>
class PointerFromPython
{
    // code generator should never instantiate this version
};

// inout pointer to struct
template <typename T>
class PointerFromPython<T, /*interface=*/false, /*struct=*/true>
{
public:
    using Type = typename std::remove_const<typename std::remove_pointer<T>::type>::type;

    PointerFromPython() : m_copy{ new Type }
    {
    }
    explicit PointerFromPython(T orig) : m_copy{ new Type{ *orig } }
    {
    }

    T getData()
    {
        return m_copy.get();
    }

    py::object createPyObject()
    {
        return py::cast(std::move(m_copy.release()), py::return_value_policy::take_ownership);
    }

private:
    std::unique_ptr<Type> m_copy;
};

// inout pointer to primitive type
template <typename T>
class PointerFromPython<T, /*interface=*/false, /*struct=*/false>
{
public:
    using Type = typename std::remove_const<typename std::remove_pointer<T>::type>::type;

    PointerFromPython() = default;
    explicit PointerFromPython(T orig) : m_copy{ *orig }
    {
    }

    T getData()
    {
        return &m_copy;
    }

    py::object createPyObject()
    {
        return py::cast(m_copy);
    }

private:
    Type m_copy;
};

inline void throwIfNone(const py::object& pyObj)
{
    if (pyObj.is_none())
    {
        throw std::runtime_error("python object must not be None");
    }
}

} // namespace python
} // namespace omni

#define OMNI_PYTHON_GLOBALS(name_, desc_) CARB_BINDINGS_EX(name_, desc_)
