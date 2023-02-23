-- ****** Add extra options for this tool here ******
newoption {
    trigger     = "fail-on-write",
    description = "(Optional) fail code generation steps if they need to write a file."
}
-- **************************************************


local function relpath(p, prj)
    if (path.hasdeferredjoin(p)) then
        p = path.resolvedeferredjoin(p)
    end
    p = path.translate(path.getrelative(prj.location, p))
    return p
end

local function starts_with(s, pre)
    return (string.sub(s, 1, #pre) == pre)
end

local function get_dep(p, prj)
    if (path.hasdeferredjoin(p)) then
        p = path.resolvedeferredjoin(p)
    end

    p = path.getrelative(prj.location, p)

    while starts_with(p, "../") or starts_with(p, "..\\") do
        p = string.sub(p, 4)
    end

    return p
end

local function resolve_flags(prj, flags)
    local out = ""
    for k, v in pairs(flags) do
        if k == "file" then
            out = out.." \""..relpath(v, prj).."\""
            dep = get_dep(v, prj)
            out = out.." -M \""..dep..".dep\""
        else
            out = out.." --"..k.." \""..relpath(v, prj).."\""
        end
    end

    for _, v in ipairs(prj.sysincludedirs) do
        out = out.." --isystem \""..relpath(v, prj).."\""
    end

    for _, v in ipairs(prj.includedirs) do
        out = out.." -I \""..relpath(v, prj).."\""
    end

    return out
end

local function bind_on_windows(prj)
    local omnibindpath = relpath(prj.omnibindpath, prj)
    local packmanpath = prj.packmanpath
    local extra_options = ""
    if nil == packmanpath then
        packmanpath = '%PM_INSTALL_PATH%'
    end

    local out = ""
    local platform = "windows-x86_64"

    if _OPTIONS["fail-on-write"] then
        extra_options = " --fail-on-write"
    end

    out = out.."@ECHO OFF\n"
    out = out.."setlocal EnableDelayedExpansion\n"
    out = out.."set LF=^\n"
    out = out.."\n"
    out = out.."\n"
    out = out.."REM The two empty lines above are required here\n"
    out = out.."\n"
    out = out.."pushd "..path.translate(prj.location).."\n"
    out = out.."call \""..packmanpath.."\\packman\" pull \""..omnibindpath.."\\deps.packman.xml\" -p windows-x86_64".."\n"
    out = out.."if \"%PYTHONPATH%\" == \"\" set \"PYTHONPATH=%PM_MODULE_DIR%;%PYTHONPATH%\"\n"
    out = out.."set PYTHONPATH="..omnibindpath..";%PM_clang_llvm_libs_PATH%/_build/"..platform.."/clang-libs/python;%PM_repo_format_PATH%;%PM_repo_man_PATH%;%PYTHONPATH%\n"
    out = out.."set PYTHONHOME=\n"
    out = out.."set status=0\n"

    cmd = "\"%PM_PYTHON_PATH%\\python.exe\" -s -S -u \""..omnibindpath.."\\scripts\\omni.bind.py\" "
        .." --skip-write-if-same"
        .." --clang \"%PM_clang_llvm_libs_PATH%/_build/"..platform.."/clang-libs/bin/libclang.dll\""
        .." --isystem \"%PM_clang_llvm_libs_PATH%/_build/"..platform.."/clang-libs/lib/clang/12.0.0/include\""
        .." --format-module \""..omnibindpath.."\\scripts\\formatter.py\""
        .." --std "..prj.cppdialect:lower()
        .." --repo-root \""..os.getcwd().."\""
        ..extra_options

    for _, flags in ipairs(prj.omnibindfiles) do
        out = out.."\n"
        out = out.."set \"py_output=\"\n"
        out = out.."for /F \"delims=\" %%f in ('\""  -- yes this ridiculousness with the extra double quote is necessary here.
        out = out..cmd..resolve_flags(prj, flags).." %*"
        out = out.."\"') do (\n"    -- and the double quote is also necessary here.  Without it, the 'python.exe' path itself cannot be double quoted.
        out = out.."    if defined py_output set \"py_output=!py_output!!LF!\"\n"
        out = out.."    set \"py_output=Py: !py_output!%%f\"\n"
        out = out..")\n"
        out = out.."echo !py_output!\n"
        out = out.."set /a status = %status% + %errorlevel%\n"
    end

    out = out.."\n"
    out = out.."popd\n"
    out = out.."exit /b %status%\n"

    script = prj.location.."/omni.bind.bat"

    local f = io.open(script, 'w')
    if f then
        ln = f:write(out)
        f:close()
    end
end

local function bind_on_posix(prj)
    local omnibindpath = relpath(prj.omnibindpath, prj)
    local packmanpath = prj.packmanpath
    local extra_options = ""
    if nil == packmanpath then
        packmanpath = '${PM_INSTALL_PATH}'
    else
        packmanpath = path.getabsolute(packmanpath)
    end

    local out = ''
    local platform = 'linux-'..os.outputof('uname -m')
    local lib_ext = "so"
    if os.outputof('uname') == "Darwin" then
        platform = "macos-universal"
        lib_ext = "dylib"
    end

    local usingLinbuild = os.getenv('LINBUILD_EMBEDDED')

    if _OPTIONS["fail-on-write"] then
        extra_options = " --fail-on-write"
    end

    out = out..'#!/usr/bin/env bash\n'
    -- uncomment for debugging
    --out = out..'set -x\n'
    out = out..'set -o errexit\n'
    --out = out..'set -o xtrace\n'

    out = out..'SCRIPT_DIR="'..omnibindpath..'"\n'
    out = out..'source "'..packmanpath..'/packman" pull "${SCRIPT_DIR:?}/deps.packman.xml" -p '..platform..'\n'
    out = out..'export PYTHONPATH="${SCRIPT_DIR:?}:${PM_clang_llvm_libs_PATH:?}/_build/'..platform..'/clang-libs/python:${PM_repo_format_PATH:?}:${PM_repo_man_PATH:?}:${PYTHONPATH}"\n'
    out = out..'unset PYTHONHOME\n'
    out = out..'export LD_LIBRARY_PATH="${PM_python_PATH:?}/lib:${PM_clang_llvm_libs_PATH:?}/lib"\n'

    cmd = '"${PM_python_PATH:?}/bin/python3" -s -S -u "'..omnibindpath..'/scripts/omni.bind.py"'
        .." --skip-write-if-same"
        ..' --clang="${PM_clang_llvm_libs_PATH:?}/_build/'..platform..'/clang-libs/bin/libclang.'..lib_ext..'"'
        ..' --isystem "${PM_clang_llvm_libs_PATH:?}/_build/'..platform..'/clang-libs/lib/clang/12.0.0/include"'
        ..' --format-module "'..omnibindpath..'/scripts/formatter.py"'
        ..' --std '..prj.cppdialect:lower()
        ..' --repo-root "'..os.getcwd()..'"'
        ..extra_options

    -- this is kinda hack to setup the toolchain search paths
    -- it would probably be better to ask the compiler for its search paths
    if usingLinbuild then
        if cfg.platform == 'x86_64' then
            cmd = cmd
                ..' --isystem /opt/rh/devtoolset-7/root/usr/include/c++/7/x86_64-redhat-linux'
                ..' --isystem /opt/rh/devtoolset-7/root/usr/include/c++/7'
        else
            cmd = cmd
                ..' --isystem /usr/include/c++/7'
        end
    elseif cfg.system == 'linux' then
        cmd = cmd
            ..' --isystem /usr/include/c++/7'
    elseif cfg.system == 'macosx' then
        -- Use `xcode-select` to get the toolchain location
        local handle = io.popen("xcode-select -p")
        local toolchainroot = handle:read("*a")
        handle:close()
        -- Strip the newline
        toolchainroot = string.gsub(toolchainroot, "\n", "")

        -- The folder structure is different between default XCode installations
        -- and installations of the command line tools. Because of course it is.
        -- And since lua has no built-in way to see if a folder exists, we
        -- have to use a trick to try to rename the folder to itself and check
        -- for an error condition... ugh.
        -- Examples of the clang include folder that we need here:
        -- Command Line Tools with clang 13.1.6:
        --   /Library/Developer/CommandLineTools/usr/lib/clang/13.1.6/include
        -- Xcode via App Store with clang 14.0.0:
        --   /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/lib/clang/14.0.0/include
        check = toolchainroot.."/Toolchains"
        local ok, err, code = os.rename(check, check)
        if not ok then
            if code == 13 then
                -- Code 13 is permission denied, but the file/folder exists
                toolchainroot = toolchainroot.."/Toolchains/XcodeDefault.xctoolchain"
            end
        end

        -- If there's an `SDKROOT` in the environment, use that.
        local sdkroot = os.getenv("SDKROOT")
        if nil == sdkroot then
            -- Otherwise, get it from `xcrun`
            handle = io.popen("xcrun --show-sdk-path")
            sdkroot = handle:read("*a")
            handle:close()
            -- Strip the newline
            sdkroot = string.gsub(sdkroot, "\n", "")
        end

        -- Now figure out the `clang` version
        -- Example output: "Apple clang version 13.1.6 (clang-1316.0.21.2.5)\n ... plus more lines"
        handle = io.popen("clang --version")
        local clangversion = handle:read("*a")
        handle:close()
        -- Parse the semver out of the string
        clangversion = string.match(clangversion, "%d[%d.,]*")

        cmd = cmd
            .." --isystem \""..sdkroot.."/usr/include/c++/v1\""
            .." --isystem \""..sdkroot.."/usr/local/include\""
            .." --isystem \""..toolchainroot.."/usr/lib/clang/"..clangversion.."/include\""
            .." --isystem \""..sdkroot.."/usr/include\""
            .." --isystem \""..toolchainroot.."/usr/include\""
    else
        error("unknown platform: '"..cfg.system.."'")
    end

    for _, flags in ipairs(prj.omnibindfiles) do
        out = out..cmd..resolve_flags(prj, flags)..' "${@}"\n'
    end

    script = prj.location..'/omni.bind.sh'

    local f = io.open(script, 'w')
    if f then
        ln = f:write(out)
        f:close()
        os.chmod(script, 0755)
    end
end

local function on_project(prj)
    if nil == prj.omnibindfiles then
        return
    end
    if os.target() == "windows" then
        bind_on_windows(prj)
    else
        bind_on_posix(prj)
    end
end

local function setup_omni_bind()
    local action = premake.action.current()
    if os.target() == "windows" then
        local function disableFastUpToDateCheck(prj)
            -- forces prebuild rule to always run (which we need since it is a dependency checker)
            --   https://stackoverflow.com/questions/28916414/visual-studio-add-pre-build-event-that-always-runs-c-project
            if nil ~= prj.omnibindfiles then
                premake.push('<PropertyGroup>')
                premake.x('<DisableFastUpToDateCheck>true</DisableFastUpToDateCheck>')
                premake.pop('</PropertyGroup>')
            end
        end

        premake.override(premake.vstudio.vc2010.elements, "project", function(base, prj)
            local calls = base(prj)
            table.insertafter(calls, premake.vstudio.vc2010.outputPropertiesGroup, disableFastUpToDateCheck)
            return calls
        end)
    end

    -- the onProject callback is invoked whenever commands need to be generated
    -- for a project within a workspace.  note, this is not called during
    -- any of the clean actions.
    local onProject = action.onProject;
    action.onProject = function(prj)
        if nil == prj.omnibindpath then
            prj.omnibindpath = "tools/omni.bind"
        end

        if nil == prj.packmanpath then
            prj.packmanpath = os.getenv('PM_INSTALL_PATH')
        end

        cfg = premake.project.getfirstconfig(prj)
        on_project(prj)
        cfg = nil

        if nil ~= onProject then
            onProject(prj)
        end
    end

    premake.api.register {
        name = "omnibindfiles",
        scope = "project",
        kind = "table",
        tokens = true,
    }

    premake.api.register {
        name = "omnibindpath",
        scope = "project",
        kind = "path",
        tokens = true,
    }

    premake.api.register {
        name = "packmanpath",
        scope = "project",
        kind = "path",
        tokens = true,
    }
end

local function resolvefiles(tbl)
    local out = {}
    for _, t in ipairs(tbl) do
        local tmp = {}
        for k, v in pairs(t) do
            tmp[k] = path.deferredjoin(os.getcwd(), v)
        end
        table.insert(out, tmp)
    end
    return out
end

function omnibind(tbl)
    kind "StaticLib"
    omnibindfiles(resolvefiles(tbl))
    files { "%{prj.omnibindpath}/cpp/Dummy.cpp" }
    if os.target() == "windows" then
        prebuildcommands{ path.translate("%{prj.location}/omni.bind.bat") }
    else
        prebuildcommands{ path.translate("%{prj.location}/omni.bind.sh") }
    end
end

if premake['omnibind'] == nil then
    setup_omni_bind()
    premake['omnibind'] = true
end
