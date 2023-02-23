-- ****** Add extra options for this tool here ******
newoption {
    trigger     = "fail-on-write",
    description = "(Optional) fail code generation steps if they need to write a file."
}
-- **************************************************


omni_structuredlog_carb_path = nil

-- Sets up the path to the Carbonite SDK where the structured log script can be found.
-- This must be called in each 'premake5.lua' script that is independently built.
-- If this is not called, an error will be generated.
--
-- Args:
--      carb_path: the full path to the Carbonite SDK.  This may be a relative or
--                 absolute path.  This may not be nil.
function setup_omni_structuredlog(carb_path)
    if carb_path == "." or carb_path == "./" then
        carb_path = os.getcwd()
    else
        carb_path = path.join(os.getcwd(), carb_path)
    end

    -- make sure the path ends in a separator so we can consistently use it later.
    if string.sub(carb_path, -1) ~= "/" then
        carb_path = carb_path.."/"
    end

    omni_structuredlog_carb_path = carb_path
    print("using the Carbonite SDK path '"..omni_structuredlog_carb_path.."'")
end

-- Differentiates an array from an object.  This is done by testing whether it has
-- numbered key elements versus key/value pairs when it is detected as a table.
-- Since lua sees all tables, arrays, dictionaries, and objects as simply "table",
-- there are several tests to try to differentiate a table treated as an array versus
-- anything else.
--
-- Args:
--      var: The variable to test for being an array.  This may be any type of variable
--           including a string, boolean, etc.  In these cases, the variable will just
--           be rejected immediately because it is not a table.
--
-- Returns:
--      Returns `true` if the variable is an array.  Returns `false` otherwise.
function is_table_an_array(var)
    -- not a table -> can't possibly be an array => fail.
    if not type(var) == "table" then
        return false
    end

    -- the table has numbered keys -> most likely being used as an array => succeed.
    --   Note that this is still not a perfect test.  The table could just be using
    --   multi-typed keys for storage.  However, since this use case is likely to be
    --   extremely rare, we'll just pretend it doesn't exist.
    if #var > 0 then
        return true
    end

    -- anything with multiple key/value pairs is being treated as an object not an
    -- array => fail.
    for key, value in pairs(var) do
        return false
    end

    -- assume all other cases are an array.  This will mostly catch the case of an empty
    -- array.
    return true
end


-- Generates the build script for a single schema.  This takes as an argument an object
-- that contains the schema name and its output file(s) and settings.  This will return
-- the script for the specified schema as a string.
--
-- Args:
--      settings: A table of constant settings to be used in generating the build script.
--                The caller is expected to generate this object.  This must at least
--                contain the `script` value specifying the full path to the platform
--                specific shell script to call out to.
--      args: The arguments object specifying the schema to be processed and its outputs.
--
-- Returns:
--      A string containing the full shell command to execute when building the schema.
function generate_schema_script(settings, args)
    -- declare all these variables as local so they don't prevent type changes from other
    -- same named variables in other functions here (because lua defaults all variables
    -- to global instead of local).
    local py_file_arg = ""
    local namespace_arg = ""
    local baked_arg = ""
    local bake_to_arg = ""
    local in_file = ""
    local out_file = ""
    local extra_options = ""

    if _OPTIONS["fail-on-write"] then
        extra_options = " --fail-on-write"
    end

    if type(args.pybind_output) == "string" then
        local py_file = path.join(os.getcwd(), args.pybind_output)
        py_file_arg = " --py-binding=\""..py_file.."\""
    end

    if type(args.namespace) == "string" then
        namespace_arg = " --namespace=\""..args.namespace.."\""
    end

    if type(args.baked) == "boolean" and args.baked then
        baked_arg = " --baked"
    end

    if type(args.bake_to) == "string" then
        bake_to_arg = " --bake-to=\""..path.join(os.getcwd(), args.bake_to).."\""
    end

    in_file = path.join(os.getcwd(), args.schema)
    out_file = path.join(os.getcwd(), args.cpp_output)

    return settings.script.." \""..in_file.."\" --cpp=\""..out_file.."\""..py_file_arg..namespace_arg..
           baked_arg..bake_to_arg..' --repo-root="'..os.getcwd()..'"'..extra_options
end

-- Generate code for a JSON structured log schema
-- Args:
--     args.schema: The input JSON schema.
--     args.cpp_output: The generated C++ header.
--     args.pybind_output: (optional) The generated pybind11 bindings for the C++ header.
--     args.namespace: (optional) The namespace for the generated code.
--                     This defaults to ``omni::structuredlog``.
--     args.baked: (optional) Boolean flag to indicate whether a simplified schema (false)
--                 or a full JSON schema (true) is being specified.
--     args.bake_to: (optional) The name of the file to write the baked schema to.  This is
--                   only used when the ``args.baked`` flag is not used.
function omni_structuredlog_schema(args)
    local check_failure = ""
    local settings = {}

    local carb_path = omni_structuredlog_carb_path

    if carb_path == nil or not os.isfile(carb_path.."tools/omni.structuredlog/structuredlog.py") then
        error("the path to the Carbonite repo was not set with setup_omni_structuredlog() before"..
              "calling omni_structuredlog_schema() in your 'premake5.lua'script.")
    end

    -- store the arguments table for this project so we can disable the 'fast up-to-date check'
    -- option in MSVC (just for this project).  This object isn't used for anything else however.
    -- Note that this will implicitly set the `omnistructuredlogfiles` property on the project
    -- object that this function was called in the context of (ie: the `project` specification
    -- that contains this call to `omni_structuredlog_schema()`).
    omnistructuredlogfiles(args)

    if not type(args) == "table" then
        error("expected a table of arguments passed to omni_structuredlog_schema().  Expected at"..
              "least a 'schema' argument.")
    end

    -- build a table of constant paths and commands to pass to generate_schema_script().
    if os.target() == "windows" then
        check_failure = "if %errorlevel% neq 0 ( exit /b %errorlevel% )"
        settings.script = "call "..path.join(os.getcwd(), "%PM_python_PATH%\\python -s -S -u \""..carb_path.."tools/omni.structuredlog/structuredlog.py\"")
    else
        settings.script = path.join(os.getcwd(), "\"$$PM_python_PATH/python\" -s -S -u \""..carb_path.."tools/omni.structuredlog/structuredlog.py\"")
    end

    -- declare the various values we'll need to collect the prebuild script.
    local dummy_file = path.join(os.getcwd(), carb_path.."tools/omni.structuredlog/Dummy.cpp")
    local prebuild_script = {}
    local packman_path = os.getenv("PM_INSTALL_PATH")

    -- !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    -- !!! NOTE: when adding a new package's path envvar to `PYTHONPATH` below, please  !!!
    -- !!!       make sure to ALSO ADD THOSE PACKAGES TO THIS TOOL'S 'deps.packman.xml' !!!
    -- !!!       file.  Failing to do so will break the build under MSVC.               !!!
    -- !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    if os.target() == "windows" then
        table.insert(prebuild_script, "@echo off")
        table.insert(prebuild_script, "setlocal")
        table.insert(prebuild_script, "")
        table.insert(prebuild_script, "set SCRIPTDIR="..carb_path.."tools/omni.structuredlog")
        table.insert(prebuild_script, "call "..packman_path.."/packman pull %SCRIPTDIR%/deps.packman.xml -p windows-x86_64")
        table.insert(prebuild_script, check_failure)
        table.insert(prebuild_script, "")
        table.insert(prebuild_script, "set PYTHONPATH=%PM_jsonref_PATH%/python;%PM_repo_man_PATH%;%PM_repo_format_PATH%;%PM_MODULE_DIR%;%PYTHONPATH%")
        table.insert(prebuild_script, "")
    else
        -- Note: under gmake2, the make tool will try to immediately evaluate all '$' tagged
        --       values.  If no matching shell symbol is found, it will simply strip the '$'
        --       and the character immediately following it.  This is typically unexpected
        --       behaviour.  If the '$' symbol is intended to make it to a shell to be evaluated
        --       later, it must be escaped with another '$'.
        --
        --       Also, since we will later be turning this script into a single line command
        --       separated by semicolons, there may not be any blank lines present in it.  These
        --       will not be accepted by the shell for evaluation.
        table.insert(prebuild_script, "set -e")
        table.insert(prebuild_script, "export SCRIPTDIR="..carb_path.."tools/omni.structuredlog")
        table.insert(prebuild_script, "source \""..packman_path.."/packman\" pull \"$$SCRIPTDIR/deps.packman.xml\" -p \"linux-$$(arch)\"")
        table.insert(prebuild_script, "set -u")
        table.insert(prebuild_script, "export LD_LIBRARY_PATH=$$PM_python_PATH/lib")
        table.insert(prebuild_script, "export PYTHONPATH=$$PM_jsonref_PATH/python:$${PM_repo_man_PATH}:$${PM_repo_format_PATH}:$${PM_MODULE_DIR}:$$PYTHONPATH")
    end


    -- the specified arguments are an object => just process the single object into a script.
    if not is_table_an_array(args) then
        table.insert(prebuild_script, generate_schema_script(settings, args))

        if check_failure ~= "" then
            table.insert(prebuild_script, check_failure)
        end

    -- the specified arguments are an array => walk the full array and collect each object in
    --   it into a single multi-command script.
    else
        for key, value in pairs(args) do
            table.insert(prebuild_script, generate_schema_script(settings, value))

            if check_failure ~= "" then
                table.insert(prebuild_script, check_failure)
            end
        end
    end

    if os.target() == "windows" then
        table.insert(prebuild_script, "exit /b 0")
    else
        -- Note: under gmake2 on linux, prebuild command scripts are run such that each command
        --       runs as its own subshell.  This means that no environment variables will persist
        --       across commands.  Since running a python app requires PYTHONHOME and PYTOHNPATH
        --       to be set in order for it to work properly, we can't have those variables being
        --       exported to a different shell than the one that runs python.  Thus the only way
        --       to handle this properly is to concatenate all the commands into a single one
        --       line command.
        --       bash is run explicitly because `source` will no work if POSIX sh is used.
        local command = "/usr/bin/env bash -c '"
        for key, value in pairs(prebuild_script) do
            command = command..value.."; \\\n"
        end

        -- strip off the trailing "; \\\n" from the command.  Note that the substring we want to
        -- chop off is only 4 characters long, but we subtract 5 from the string's length here
        -- because the end index for the substring is inclusive.
        command = string.sub(command, 1, #string - 5)
        command = command.."'"
        prebuild_script = {}
        table.insert(prebuild_script, command)
    end

    -- add a single prebuild command to the project.  Note that this is a limitation of MSVC
    -- projects under premake - only a single command will be added to the project's prebuild
    -- script.  If any other `prebuildcommands()` calls are made for the project, they will
    -- be silently ignored.  Linux doesn't have this issue however.  It will happily accept
    -- and properly process multiple `prebuildcommands()` scripts for a single project.  This
    -- seems to be a bug in premake's MSVC project generation itself.
    prebuildcommands{ prebuild_script }

    -- dummy stuff to get this to even build
    kind "StaticLib"
    files { dummy_file }

    buildcommands { "echo building %{prj.name}" }
    buildoutputs { "_______________________________________" }
end


-- Sets up this tool.  This is intended to be called once in the premake project.  This installs
-- a project specific data object that is used to disable MSVC's 'fast up-to-date' check for
-- projects that use the structured log schema tool.  This is necessary to ensure the schema
-- rebuild step is not skipped unnecessarily.
local function setup_project_bindings()
    if os.target() == "windows" then
        local function disableFastUpToDateCheck(prj)
            if prj.omnistructuredlogfiles ~= nil then
                -- forces prebuild rule to always run (which we need since it is a dependency checker)
                --   https://stackoverflow.com/questions/28916414/visual-studio-add-pre-build-event-that-always-runs-c-project
                premake.push('<PropertyGroup>')
                premake.x('<DisableFastUpToDateCheck>true</DisableFastUpToDateCheck>')
                premake.pop('</PropertyGroup>')
            end
        end

        -- This command adds a custom callback hook to all 'onProject' operations.  When the
        -- projects are generated, this will call into the local helper function above to check
        -- if the project made use of `omni_structuredlog_schema()`.  If it did, the fast
        -- build check will be disabled so we can perform our own custom build checks as needed.
        -- Projects that do not make use of `omni_structuredlog_schema()` will be unmodified.
        premake.override(premake.vstudio.vc2010.elements, "project", function(base, prj)
            local calls = base(prj)
            table.insertafter(calls, premake.vstudio.vc2010.outputPropertiesGroup, disableFastUpToDateCheck)
            return calls
        end)
    end

    -- This registers a custom table object for each project object generated in this script.
    -- The object is expected to be a table, but for some strange language syntax reason, It
    -- is treated as though it's a function.  When something calls this object from within
    -- a project object's context (ie: within a `project` specification), it will implicitly
    -- set it on that project's object so that it can be queried later.
    premake.api.register {
        name = "omnistructuredlogfiles",
        scope = "project",
        kind = "table",
        tokens = true,
    }
end

if premake['omni_structuredlog'] == nil then
    setup_project_bindings()
    premake['omni_structuredlog'] = true
end
