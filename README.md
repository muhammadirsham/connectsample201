# Connect Samples for the Omniverse Client Library

Build your own NVIDIA Omniverse Connector by following our samples that use Pixar USD and Omniverse Client Library APIs:

- Omni CLI - A command line utility to manage files on a Nucleus server.
- HelloWorld (C++ and Python) - A sample program that shows how to connect to an Omniverse Nucleus server, create a USD stage, create a polygonal box, bind a material, add a light, save data to .usd file, create and edit a .live layer, and send/receive messages over a channel on Nucleus. This sample is provided in both C++ and Python to demonstrate the Omniverse APIs for each language.
- LiveSession (C++ and Python) - A sample program that demonstrates how to create, join, merge, and participate in live sessions. This sample is provided in both C++ and Python to demonstrate the Omniverse APIs for each language.
- OmniUsdaWatcher (C++) - A live USD watcher that outputs a constantly updating USDA file on disk.
- OmniSimpleSensor (C++) - A C++ program that demonstrates how to connect external input (e.g sensor data) to a USD layer in Nucleus.

## Using the prebuilt package from the Omniverse Launcher

If the Connect Sample was downloaded from the Omniverse Launcher then the sample programs are already built and can be run with the relevant `run_*.bat` or `run_*.sh` commands. They all accept commandline arguments are are best experienced from an interactive terminal.

If you are interested in building these samples yourself, proceed to the [`How to build`](#how-to-build) section to download all of the build dependencies and build the samples.

## How to build

### Linux
This project requires "make" and "g++".

- Open a terminal.
- To obtain "make" type ```sudo apt install make``` (Ubuntu/Debian), or ```yum install make``` (CentOS/RHEL).  
- For "g++" type ```sudo apt install g++``` (Ubuntu/Debian), or ```yum install gcc-c++``` (CentOS/RHEL).

Use the provided build script to download all other dependencies (e.g USD), create the Makefiles, and compile the code.

```bash
./build.sh
```

Use any of the `run_*.sh` scripts (e.g. `./run_hello_world.sh`) to execute each program with a pre-configured environment.

> Tip: If you prefer to manage the environment yourself, add `<samplesRoot>/_build/linux64-x86_64/release` to your `LD_LIBRARY_PATH`.

For commandline argument help, use `--help`
```bash
./run_hello_world.sh --help
```

> Note : For omnicli, use `./omnicli.sh help` instead.

### Windows
#### Building
Use the provided build script to download all dependencies (e.g USD), create the projects, and compile the code.  
```bash
.\build.bat
```

Use any of the `run_*.bat` scripts (e.g. `.\run_hello_world.bat`) to execute each program with a pre-configured environment.

For commandline argument help, use `--help`
```bash
.\run_hello_world.bat --help
```

> Note : For omnicli, use `.\omnicli.bat help` instead.

#### Changing the  MSVC Compiler [Advanced]

When `prebuild.bat` is executed (by itself or through `build.bat`) a version of the Microsoft Visual Studio Compiler and the Windows 10 SDK are downloaded and referenced by the generated Visual Studio projects.  If a user wants the projects to use an installed version of Visual Studio 2019 then run [configure_win_buildtools.bat](configure_win_buildtools.bat) first.  This script _should_ find `vswhere.exe` and then use it to discover which MSVC compiler to use.  If it does not you can edit the [deps/host-deps.packman.xml](deps/host-deps.packman.xml) file's `buildtools` `source path` to the folder on your PC which contains the `VC` subfolder (normally something like `C:/Program Files (x86)/Microsoft Visual Studio/2019/BuildTools`).  Note, the build scripts are configured to tell premake to generate VS 2019 project files.  Some plumbing is required to support other Visual Studio versions.

#### Changing the Windows 10 SDK [Advanced]
If a user wants the generated projects to use a different Windows 10 SDK than what is downloaded then the [configure_win_buildtools.bat](configure_win_buildtools.bat) file will automatically search for one using the `winsdk.bat` Microsoft tool and insert the correct paths into the [deps/host-deps.packman.xml](deps/host-deps.packman.xml) file.  If the `winsdk.bat` file doesn't do what is required, edit these fields in the [deps/host-deps.packman.xml](deps/host-deps.packman.xml) file:
* `winsdk`
* `winsdk_bin`
* `winsdk_include`
* `winsdk_lib`

Note that an installed Windows SDK will have the actual version in all of these paths, for example `include/10.0.17763.0` rather than just `include`


## Issues with Self-Signed Certs
If the scripts from the Connect Sample fail due to self-signed cert issues, a possible workaround would be to do this:

Install python-certifi-win32 which allows the windows certificate store to be used for TLS/SSL requests:

```bash
%PM_PYTHON% -m pip install python-certifi-win32 --trusted-host pypi.org --trusted-host files.pythonhosted.org
```

Note the %PM_PYTHON% is an environment variable set by the build script.



## Documentation and learning resources for USD and Omniverse

[USD Docs - Creating Your First USD Stage](https://graphics.pixar.com/usd/docs/Hello-World---Creating-Your-First-USD-Stage.html)

[Pixar USD API Docs](https://graphics.pixar.com/usd/docs/api/index.html)

[Pixar USD User Docs](https://graphics.pixar.com/usd/release/index.html)

[NVDIDA USD Docs](https://developer.nvidia.com/usd)

[Omniverse Client Library API Docs](https://omniverse-docs.s3-website-us-east-1.amazonaws.com/client_library)

[Omniverse USD Resolver API Docs](http://omniverse-docs.s3-website-us-east-1.amazonaws.com/usd_resolver)