// Copyright (c) 2018-2022, NVIDIA CORPORATION. All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto. Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.
//

//! @file
//!
//! @brief Common types used through-out Carbonite.
#ifndef __CARB_TYPES_H__
#define __CARB_TYPES_H__ 1

#include "Interface.h"
#include "Strong.h"

#include <omni/core/OmniAttr.h>

#include <cstddef>
#include <cstdint>

namespace omni
{
namespace core
{
OMNI_DECLARE_INTERFACE(ITypeFactory) // forward declaration for entry inPluginFrameworkDesc
}
namespace log
{
class ILog; // forward declaration for entry in PluginFrameworkDesc
}
namespace structuredlog
{
class IStructuredLog;
}
} // namespace omni

//! The main Carbonite namespace.
namespace carb
{

/**
 * Defines the plugin hot reloading (auto reload) behaviour.
 *
 * @rst
 * .. deprecated:: Hot reloading support has been removed.  No replacement will be provided.
 * @endrst
 */
enum class PluginHotReload
{
    eDisabled,
    eEnabled
};

/**
 * Defines a descriptor for the plugin implementation, to be provided to the macro CARB_PLUGIN_IMPL.
 */
struct PluginImplDesc
{
    const char* name; //!< Name of the plugin (e.g. "carb.dictionary.plugin"). Must be globally unique.
    const char* description; //!< Helpful text describing the plugin.  Use for debugging/tools.
    const char* author; //!< Author (e.g. "NVIDIA").

    //! If hot reloading is supported by the plugin.
    //! @rst
    //! .. deprecated:: Hot reloading support has been removed.  No replacement will be provided.
    //! @endrst
    PluginHotReload hotReload;

    const char* build; //!< Build version of the plugin.
};

//! Defines a struct to be filled by a plugin to provide the framework with all information about it.
//! @note This struct has been superseded by PluginRegistryEntry2 but exists for historical and backwards-compatibility.
//! In the past, this struct was filled by the macro CARB_PLUGIN_IMPL.
struct PluginRegistryEntry
{
    PluginImplDesc implDesc; //!< Textual information about the plugin (name, desc, etc).

    //! Entry in an array of interfaces implemented by the plugin.
    struct Interface
    {
        InterfaceDesc desc; //!< An interface in the plugin.
        const void* ptr; //!< Pointer to the interface's `struct`.
        size_t size; //!< Size of the interface's `struct`.
    };

    Interface* interfaces; //!< Pointer to an array of interfaces implemented by the plugin.
    size_t interfaceCount; //!< Number of interfaces in the @p interfaces array.
};

//! Defines a struct to be filled by a plugin to provide the framework with all information about it.
//! This struct is automatically created and filled by the macro CARB_PLUGIN_IMPL.
struct PluginRegistryEntry2
{
    size_t sizeofThisStruct; //!< Must reflect sizeof(PluginRegistryEntry2); used as a version for this struct.
    PluginImplDesc implDesc; //!< Textual information about the plugin (name, desc, etc).

    //! Entry in an array of interfaces implemented by the plugin.
    struct Interface2
    {
        size_t sizeofThisStruct; //!< Must reflect sizeof(Interface2); used as a version for this struct.

        InterfaceDesc desc; //!< An interface in the plugin.
        size_t size; //!< Required size for the interface
        size_t align; //!< Required alignment for the interface
        void(CARB_ABI* Constructor)(void*); //!< Constructor function
        void(CARB_ABI* Destructor)(void*); //!< Destructor function

        // Internal note: This struct can be modified via the same rules for PluginRegistryEntry2 below.
    };

    Interface2* interfaces; //!< Pointer to an array of interfaces implemented by the plugin.
    size_t interfaceCount; //!< Number of interfaces in the @p interfaces array.

    // Internal note: This struct can be modified without changing the carbonite framework version, provided that new
    // members are only added to the end of the struct and existing members are not modified. The version can then be
    // determined by the sizeofThisStruct member. However, if it is modified, please add a new
    // carb.frameworktest.*.plugin (see ex2initial for an example).
};

/**
 * Defines a struct which contains all key information about a plugin loaded into memory.
 */
struct PluginDesc
{
    PluginImplDesc impl; //!< Name, description, etc.

    const InterfaceDesc* interfaces; //!< Array of interfaces implemented by the plugin.
    size_t interfaceCount; //!< Number of interfaces implemented by the plugin.

    const InterfaceDesc* dependencies; //!< Array of interfaces on which the plugin depends.
    size_t dependencyCount; //!< Number of interfaces on which the plugin depends.

    const char* libPath; //!< File from which the plugin was loaded.
};

//! Lets clients of a plugin know both just before and just after that the plugin is being reloaded.
enum class PluginReloadState
{
    eBefore, //!< The plugin is about to be reloaded.
    eAfter //!< The plugin has been reloaded.
};

//! Pass to each plugin's @ref OnPluginRegisterExFn during load.  Allows the plugin to grab global Carbonite state such
//! as the @ref carb::Framework singleton.
struct PluginFrameworkDesc
{
    struct Framework* framework; //!< Owning carb::Framework.  Never `nullptr`.
    omni::core::ITypeFactory* omniTypeFactory; //!< omni::core::ITypeFactory singleton.  May be `nullptr`.
    omni::log::ILog* omniLog; //!< omni::log::ILog singleton.  May be `nullptr`.
    omni::structuredlog::IStructuredLog* omniStructuredLog; //!< omni::structuredlog::IStructuredLog singleton.  May be
                                                            //!< `nullptr`.

    //! Reserved space for future fields.  If a new field is added, subtract 1 from this array.
    //!
    //! The fields above must never be removed though newer implementations of carb.dll may decide to populate them with
    //! nullptr.
    //!
    //! When a newer plugin is loaded by an older carb.dll, these fields will be nullptr.  It is up to the newer plugin
    //! (really CARB_PLUGIN_IMPL_WITH_INIT()) to handle this.
    void* Reserved[28];
};

static_assert(sizeof(PluginFrameworkDesc) == (sizeof(void*) * 32),
              "sizeof(PluginFrameworkDesc) is unexpected. did you add a new field improperly?"); // contact ncournia for
                                                                                                 // questions

/**
 * Defines a shared object handle.
 */
struct CARB_ALIGN_AS(8) SharedHandle
{
    union
    {
        void* handlePointer; ///< A user-defined pointer.
        void* handleWin32; ///< A Windows/NT HANDLE. Defined as void* instead of "HANDLE" to avoid requiring windows.h.
        int handleFd; ///< A file descriptor (FD), POSIX handle.
    };
};

//! @defgroup CarbonitePluginExports Functions exported from Carbonite plugins. Use @ref CARB_PLUGIN_IMPL to have
//! reasonable default implementations of these function implemented for you in your plugin.

//! Required.  Returns the plugin's required @ref carb::Framework version.
//
//! Use @ref CARB_PLUGIN_IMPL to have this function generated for your plugin.
//!
//! Most users will not have a need to define this function, as it is defined by default via @ref CARB_PLUGIN_IMPL.
//!
//! @ingroup CarbonitePluginExports
//!
//! @see carbGetFrameworkVersion
typedef Version(CARB_ABI* GetFrameworkVersionFn)();

//! Either this or OnPluginRegisterExFn or OnPluginRegisterEx2Fn are required.  Populates the given @ref
//! carb::PluginRegistryEntry with the plugin's information.
//!
//! Prefer using @ref OnPluginRegisterExFn instead of this function.
//!
//! Most users will not have a need to define this function, as it is defined by default via @ref CARB_PLUGIN_IMPL.
//!
//! @ingroup CarbonitePluginExports
//!
//! @see carbOnPluginRegister
typedef void(CARB_ABI* OnPluginRegisterFn)(Framework* framework, PluginRegistryEntry* outEntry);

//! Either this or OnPluginRegisterFn or OnPluginRegisterEx2 are required.  Populates the given @ref
//! carb::PluginRegistryEntry with the plugin's information.
//!
//! Use @ref CARB_PLUGIN_IMPL to have this function generated for your plugin.
//!
//! @ingroup CarbonitePluginExports
//!
//! @see carbOnPluginRegisterEx
typedef void(CARB_ABI* OnPluginRegisterExFn)(PluginFrameworkDesc* framework, PluginRegistryEntry* outEntry);

//! Either this or OnPluginRegisterEx2Fn or OnPluginRegisterFn are required.  Populates the given
//! carb::PluginRegistryEntry2 with the plugin's information.
//!
//! Use @ref CARB_PLUGIN_IMPL to have this function generated for your plugin.
//!
//! @ingroup CarbonitePluginExports
//!
//! @see carbOnPluginRegisterEx2
typedef void(CARB_ABI* OnPluginRegisterEx2Fn)(PluginFrameworkDesc* framework, PluginRegistryEntry2* outEntry);

//! Optional. Called after @ref OnPluginRegisterExFn.
//!
//! Most users will not have a need to define this function, as it is defined by default via @ref CARB_PLUGIN_IMPL.
//!
//! @ingroup CarbonitePluginExports
//!
//! @see carbOnPluginPreStartup
typedef void(CARB_ABI* OnPluginPreStartupFn)();

//! Optional. Called after @ref OnPluginPreStartupFn.
//!
//! Prefer using @ref OnPluginStartupExFn instead of this function since @ref OnPluginStartupExFn return a value that
//! will cause the plugin be unloaded.
//!
//! @ingroup CarbonitePluginExports
//!
//! @see carbOnPluginStartup
typedef void(CARB_ABI* OnPluginStartupFn)();

//! Optional. Called after @ref OnPluginPreStartupFn.
//!
//! This is the main user defined function for running startup code in your plugin.
//!
//! @returns Returns `true` if the startup was successful.  If `false` is returned, the plugin will be immediately
//! unloaded (only @ref OnPluginPostShutdownFn is called).
//!
//! @ingroup CarbonitePluginExports
//!
//! @see carbOnPluginStartupEx
typedef bool(CARB_ABI* OnPluginStartupExFn)();

//! Optional. Called after @ref OnPluginStartupExFn.
//!
//! Called when the @ref carb::Framework is unloading the plugin. If the framework is released with
//! carb::quickReleaseFrameworkAndTerminate() and OnPluginQuickShutdownFn is available for plugin, this function is not
//! called.
//!
//! This is the main user defined function for running shutdown code in your plugin.
//!
//! @ingroup CarbonitePluginExports
//!
//! @see carbOnPluginShutdown
typedef void(CARB_ABI* OnPluginShutdownFn)();

//! Optional. Called if provided in lieu of OnPluginShutdownFn when the @ref carb::quickReleaseFrameworkAndTerminate()
//! is performing a quick shutdown.
//!
//! This function should save any state necessary, and close and flush any I/O, returning as quickly as possible. This
//! function is not called if the plugin is unloaded normally or through carb::releaseFramework().
//!
//! @note If carb::quickReleaseFrameworkAndTerminate() is called, OnPluginQuickShutdownFn is called if it is available.
//! If the function does not exist, OnPluginShutdownFn is called instead. OnPluginPostShutdownFn is always called.
//!
//! @ingroup CarbonitePluginExports
//!
//! @see carbOnPluginQuickShutdown
typedef void(CARB_ABI* OnPluginQuickShutdownFn)();

//! Optional. Called after @ref OnPluginShutdownFn.
//!
//! Called when the @ref carb::Framework is unloading the plugin.
//!
//! Most users will not have a need to define this function, as it is defined by default via @ref CARB_PLUGIN_IMPL.
//!
//! @ingroup CarbonitePluginExports
//!
//! @see carbOnPluginPostShutdown
typedef void(CARB_ABI* OnPluginPostShutdownFn)();

//! Optional. Returns a static list of interfaces this plugin depends upon.
//!
//! Use @ref CARB_PLUGIN_IMPL_DEPS to have this function generated for you.
//!
//! @ingroup CarbonitePluginExports
//!
//! @see carbGetPluginDeps
typedef void(CARB_ABI* GetPluginDepsFn)(InterfaceDesc** interfaceDesc, size_t* count);

//! Optional.
//!
//! @ingroup CarbonitePluginExports
//!
//! @see carbOnReloadDependency
typedef void(CARB_ABI* OnReloadDependencyFn)(PluginReloadState reloadState, void* pluginInterface, PluginImplDesc desc);


//! Two component `float` vector.
struct Float2
{
    float x; //!< x-component
    float y; //!< y-component
};

//! Three component `float` vector.
struct Float3
{
    float x; //!< x-component
    float y; //!< y-component
    float z; //!< z-component
};

//! Four component `float` vector.
struct Float4
{
    float x; //!< x-component
    float y; //!< y-component
    float z; //!< z-component
    float w; //!< w-component
};

//! Two component `double` vector.
struct Double2
{
    double x; //!< x-component
    double y; //!< y-component
};

//! Three component `double` vector.
struct Double3
{
    double x; //!< x-component
    double y; //!< y-component
    double z; //!< z-component
};

//! Four component `double` vector.
struct Double4
{
    double x; //!< x-component
    double y; //!< y-component
    double z; //!< z-component
    double w; //!< w-component
};

//! RGBA color with templated data type.
template <typename T>
struct Color
{
    T r; //!< Red
    T g; //!< Green
    T b; //!< Blue
    T a; //!< Alpha (transparency)
};

//! RGB `float` color.
struct ColorRgb
{
    float r; //!< Red
    float g; //!< Green
    float b; //!< Blue
};

//! RGBA `float` color.
struct ColorRgba
{
    float r; //!< Red
    float g; //!< Green
    float b; //!< Blue
    float a; //!< Alpha (transparency)
};

//! RGB `double` color.
struct ColorRgbDouble
{
    double r; //!< Red
    double g; //!< Green
    double b; //!< Blue
};


//! RGBA `double` color.
struct ColorRgbaDouble
{
    double r; //!< Red
    double g; //!< Green
    double b; //!< Blue
    double a; //!< Alpha (transparency)
};

//! Two component `int32_t` vector.
struct Int2
{
    int32_t x; //!< x-component
    int32_t y; //!< y-component
};

//! Three component `int32_t` vector.
struct Int3
{
    int32_t x; //!< x-component
    int32_t y; //!< y-component
    int32_t z; //!< z-component
};

//! Four component `int32_t` vector.
struct Int4
{
    int32_t x; //!< x-component
    int32_t y; //!< y-component
    int32_t z; //!< z-component
    int32_t w; //!< w-component
};

//! Two component `uint32_t` vector.
struct Uint2
{
    uint32_t x; //!< x-component
    uint32_t y; //!< y-component
};


//! Three component `uint32_t` vector.
struct Uint3
{
    uint32_t x; //!< x-component
    uint32_t y; //!< y-component
    uint32_t z; //!< z-component
};

//! Four component `uint32_t` vector.
struct Uint4
{
    uint32_t x; //!< x-component
    uint32_t y; //!< y-component
    uint32_t z; //!< z-component
    uint32_t w; //!< w-component
};

//! A representation that can combine four character codes into a single 32-bit value for quick comparison.
//! @see CARB_MAKE_FOURCC
using FourCC = uint32_t;

//! A macro for producing a carb::FourCC value from four characters.
#define CARB_MAKE_FOURCC(a, b, c, d)                                                                                   \
    ((FourCC)(uint8_t)(a) | ((FourCC)(uint8_t)(b) << 8) | ((FourCC)(uint8_t)(c) << 16) | ((FourCC)(uint8_t)(d) << 24))

/**
 * Timeout constant
 */
constexpr uint32_t kTimeoutInfinite = CARB_UINT32_MAX;

//! A handle type for \ref Framework::addLoadHook() and \ref Framework::removeLoadHook()
CARB_STRONGTYPE(LoadHookHandle, size_t);

//! A value indicating an invalid load hook handle.
constexpr LoadHookHandle kInvalidLoadHook{};

//! An enum that describes a binding registration for \ref carb::Framework::registerScriptBinding().
enum class BindingType : uint32_t
{
    Owner, //!< The given client owns a script language; any interfaces acquired within the script language will be
           //!< considered as dependencies of the script language.
    Binding, //!< The given client is a binding for the given script language. Any interfaces acquired by the binding
             //!< will be considered as dependencies of all owners of the script language.
};

} // namespace carb


// these types used to be in this file but didn't really belong.  we continue to include these type in this file for
// backward-compat.
#include <carb/RenderingTypes.h>

#endif
