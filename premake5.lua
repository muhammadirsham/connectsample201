newoption {
    trigger     = "platform-host",
    description = "(Optional) Specify host platform for cross-compilation"
}

-- remove /JMC parameter for visual studio
require('vstudio')
premake.override(premake.vstudio.vc2010.elements, "clCompile", function(oldfn, cfg)
    local calls = oldfn(cfg)
    table.insert(calls, function(cfg)
        premake.vstudio.vc2010.element("SupportJustMyCode", nil, "false")
    end)
    return calls
end)

function dofile_arg(filename, ...)
    local f = assert(loadfile(filename))
    return f(...)
end

-- Wrapper funcion around path.getabsolute() which makes drive letter lowercase on windows.
-- Otherwise drive letter can alter the case depending on environment and cause solution to reload.
function get_abs_path(p)
    p = path.getabsolute(p)
    if os.target() == "windows" then
        p = p:gsub("^%a:", function(c) return c:lower() end)
    end
    return p
end

function copy_to_targetdir(filePath)
    local filePathAbs = get_abs_path(filePath)
    postbuildcommands {"{COPY} " .. filePathAbs .. " %{cfg.targetdir}/"}
end

function get_prebuild_files()
    return { os.matchfiles("premake5.lua"), os.matchfiles("deps/*"), os.matchfiles("tools/buildscripts/*.*") }
end

function split(instr, sep)
    local substrings = {}; i = 1
    for str in string.gmatch(instr, "([^"..sep.."]+)") do
        substrings[i] = str
        i = i + 1
    end
    return substrings
end

local hostDepsDir = "_build/host-deps"
local targetDepsDir = "_build/target-deps"
local currentAbsPath = get_abs_path(".");

local usdDir = targetDepsDir.."/usd"
local usdLibDir = usdDir.."/%{cfg.buildcfg}/lib"
local usdLibDirRelease = usdDir.."/release/lib"

local pyver = "37"
local boostVer = "1_68"

if pyver == "39" or pyver == "310" then
    boostVer = "1_70"
end

-- This bit will find the correct boost version
-- It can also live in a folder inside USD, so look for that as well
local boostInSubFolder = false
local boostMatches = os.matchdirs(usdDir.."/release/include/boost-*")
for k,v in ipairs(boostMatches) do
    print(k)
    print(v)
    boostVer = v:match("boost%-(.*)")
    boostInSubFolder = true
end

-- premake5.lua
workspace "Samples"

    configurations { "debug", "release" }
    platforms { "x86_64" }
    architecture "x86_64"

    local targetName = _ACTION
    local workspaceDir = "_compiler/"..targetName

    -- common dir name to store platform specific files
    local platform = "%{cfg.system}-%{cfg.platform}"

    local targetDependencyPlatform = "%{cfg.system}-%{cfg.platform}";
    local hostDependencyPlatform = _OPTIONS["platform-host"] or targetDependencyPlatform;

    --local externalsDir = targetDepsDir..""
    local targetDir = "_build/"..platform.."/%{cfg.buildcfg}"

    -- adding dependencies
    filter { "system:linux" }
        linkoptions { '-Wl,--disable-new-dtags -Wl,-rpath,../../../_build/target-deps/usd/%{cfg.buildcfg}/lib:../../../_build/target-deps/omni_client_library/%{cfg.buildcfg}:../../../_build/target-deps/python/lib:' }
        includedirs { 
            targetDepsDir.."/python/include/python3.7m"
         }
         libdirs {
            targetDepsDir.."/python/lib"
         }
    filter { "system:windows" }
        includedirs { 
            targetDepsDir.."/python/include"
         }
         libdirs {
            targetDepsDir.."/python/libs"
         }
    
    -- not platform specific
    filter {}
        includedirs {
            usdDir.."/%{cfg.buildcfg}/include",
            targetDepsDir.."/usd_ext_physics/%{cfg.buildcfg}/include",
            targetDepsDir.."/omni_client_library/include",
            targetDepsDir.."/omni_usd_resolver/include",
            targetDepsDir.."/tinytoml/include",
            targetDepsDir.."/nlohmann-json/include",
            "source/omniUtilsLib/include"
        }

        if boostInSubFolder then 
            includedirs { usdDir.."/%{cfg.buildcfg}/include/boost-"..boostVer }
        end

        libdirs {
            usdLibDir,
            targetDepsDir.."/usd_ext_physics/%{cfg.buildcfg}/lib",
            targetDepsDir.."/omni_client_library/%{cfg.buildcfg}",
            targetDepsDir.."/omni_usd_resolver/%{cfg.buildcfg}",
            targetDepsDir
        }

    filter { "system:windows" }

        -- Use the toolset here because this USD version is built with vc141, don't automatically pick based on our compiler
        defines { "BOOST_ALL_DYN_LINK", "BOOST_LIB_TOOLSET=\"vc141\"" }
        -- Work around https://github.com/intel/tbb/issues/154
        defines { "TBB_USE_DEBUG=%{cfg.buildcfg == 'debug' and 1 or 0}" }
        defines { "_CRT_SECURE_NO_WARNINGS" }
    filter {}

    filter { "configurations:debug" }
        defines { "DEBUG", "NOMINMAX" }
        optimize "Off"
        runtime "Debug"
    filter { "configurations:release" }
        defines { "NDEBUG", "NOMINMAX" }
        optimize "On"
        runtime "Release"
    filter {}

    location (workspaceDir)
    targetdir (targetDir)

    -- symbolspath ("_build/"..targetName.."/symbols/%{cfg_buildcfg}/%{prj.name}.pdb")
    objdir ("_build/intermediate/"..platform.."/%{prj.name}")
    symbols "On"
    exceptionhandling "On"
    rtti "On"
    staticruntime "Off"
    cppdialect "C++17"

    filter { "system:windows" }
        -- add .editorconfig to all projects so that VS 2017 automatically picks it up
        files {".editorconfig"}
        editandcontinue "Off"
        local paths = {}
        -- This is where 64-bit cl.exe is located
        paths.clExeDir = "_build/host-deps/vc/bin/HostX64/x64"
        -- This is where 64-bit rc.exe is located
        paths.rcExeDir = "_build/host-deps/winsdk/bin/x64"
        -- This is where STL includes are located for your toolchain
        paths.msvcInclude = "_build/host-deps/vc/include"
        -- This is where the 64-bit vcruntime is located for your toolchain
        paths.msvcLibs = "_build/host-deps/vc/lib/onecore/x64"
        -- This is where Windows SDK includes are located
        winSdkInclude = "_build/host-deps/winsdk/include"
        -- This is where Windows SDK 64-bit libs are located
        winSdkLibs = "_build/host-deps/winsdk/lib"
        paths.sdkInclude = { winSdkInclude.."/winrt", winSdkInclude.."/um", winSdkInclude.."/ucrt", winSdkInclude.."/shared" }
        paths.sdkLibs = { winSdkLibs.."/ucrt/x64", winSdkLibs.."/um/x64" }
        bindirs { paths.clExeDir, paths.rcExeDir }
        systemversion "10.0.17763.0"
        syslibdirs { paths.msvcLibs, paths.sdkLibs }
        sysincludedirs { paths.msvcInclude, paths.sdkInclude }
        -- all of our source strings and executable strings are utf8
        buildoptions {"/utf-8 /wd4244 /wd4305 /wd4267 -D_SCL_SECURE_NO_WARNINGS"}
    filter { "system:linux" }
        buildoptions {"-D_GLIBCXX_USE_CXX11_ABI=0 -Wno-deprecated-declarations -Wno-deprecated -Wno-unused-variable -pthread -lstdc++fs"}
    filter {}


function sample(projectName, sourceFolder)
    project(projectName)
    kind "ConsoleApp"
    filter { "system:windows", "configurations:release" }
        optimize "Size"
    filter { "system:windows", "configurations:debug" }
        optimize "Off"
    intrinsics "off"
    inlining "Explicit"
    flags { "NoManifest", "NoIncrementalLink", "NoPCH" }

    local usdlibs = {
        "ar",
        "arch",
        "gf",
        "js",
        "kind",
        "pcp",
        "plug",
        "sdf",
        "tf",
        "trace",
        "usd",
        "usdGeom",
        "usdLux",
        "usdPhysics",
        "usdShade",
        "usdUtils",
        "vt",
        "work"
    }

    filter { "system:windows", "configurations:debug" }
        links { "omniclient", "omni_usd_resolver", "omniUtilsLib" }
        if pyver ~= "0" then
            links { "python"..pyver, "boost_python"..pyver.."-vc141-mt-gd-x64-"..boostVer }
        end
    filter { "system:windows", "configurations:release" }
        links { "omniclient", "omni_usd_resolver", "omniUtilsLib" }
        if pyver ~= "0" then
            links { "python"..pyver, "boost_python"..pyver.."-vc141-mt-x64-"..boostVer }
        end
    filter { "system:linux" }
        links { "omniclient", "omni_usd_resolver", "pthread", "stdc++fs", "omniUtilsLib" }
        if pyver ~= "0" then
            links { "python3.7m", "boost_python"..pyver}
        end
    filter {}

        if os.target() == "windows" then
            if next(os.matchfiles(usdLibDirRelease.."/usd_ms.dll")) then
                links { "usd_ms" }
        -- prefixed monolithic build with "lib"
        elseif next(os.matchfiles(usdLibDirRelease.."/libusd_ms.dll")) then
                links { "libusd_ms" }
            elseif next(os.matchfiles(usdLibDirRelease.."/usd_usd.dll")) then
                for _, lib in ipairs(usdlibs) do
                    links { "usd_"..lib }
                end
            elseif next(os.matchfiles(usdLibDirRelease.."/nv_usd.dll")) then
                for _, lib in ipairs(usdlibs) do
                    links { "nv_"..lib }
                end
            else
                links(usdlibs)
            end
        elseif os.target() == "linux" then
            if next(os.matchfiles(usdLibDirRelease.."/libusd_ms.so")) then
                links { "usd_ms" }
            elseif next(os.matchfiles(usdLibDirRelease.."/libusd_usd.so")) then
                for _, lib in ipairs(usdlibs) do
                    links { "usd_"..lib }
                end
            else
                links(usdlibs)
            end
        end

    location (workspaceDir.."/%{prj.name}")
    files { "source/"..sourceFolder.."/**.*" }
    filter { "system:windows" }
        links { "shlwapi" }
    filter {}
end

function sample_lib(projectName, sourceFolder)
    project(projectName)
    kind "StaticLib"
    filter { "system:windows", "configurations:release" }
        optimize "Size"
    filter { "system:windows", "configurations:debug" }
        optimize "Off"
    intrinsics "off"
    inlining "Explicit"
    flags { "NoManifest", "NoIncrementalLink", "NoPCH" }
    filter {}
    location (workspaceDir.."/%{prj.name}")
    files { "source/"..sourceFolder.."/**.*" }
    filter {}
end


sample_lib("omniUtilsLib", "omniUtilsLib")
sample("HelloWorld", "helloWorld")
sample("LiveSession", "liveSession")
sample("omnicli", "omnicli")
sample("omniUsdaWatcher", "omniUsdaWatcher")
sample("omniSimpleSensor", "omniSimpleSensor")
sample("omniSensorThread", "omniSensorThread")

