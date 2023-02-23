# Omniverse USD Resolver

This is a USD plugin that allows for working with files in Omniverse

## Documentation

The latest documentation can be found at http://omniverse-docs.s3-website-us-east-1.amazonaws.com/usd_resolver/

## Getting

You can get the latest build from Packman.
There are seperate packages for each usd flavor, python version, and platform.

They are all named:
omni_usd_resolver.{usd_flavor}.{python_flavor}.{platform}

usd_flavor is one of:
* nv-20_08
* nv-21_11
* pxr-20_08

python_flavor is one of:
* nopy
* py27
* py37
* py39

platform is one of:
* windows-x86_64
* linux-x86_64
* linux-aarch64

All packages use the same versioning scheme:
`{major}.{minor}.{patch}`

#### USD & Client Library

The package includes `redist.packman.xml` which point to the versions of USD and the Omniverse Client Library
that this plugin was built against. You can include it in your own packman.xml file like this:

```
<project toolsVersion="5.0">
  <import path="../_build/target-deps/omni_usd_resolver/deps/redist.packman.xml">
  </import>
  <dependency name="usd_debug" linkPath="../_build/target-deps/usd/debug">
  </dependency>
  <dependency name="usd_release" linkPath="../_build/target-deps/usd/release">
  </dependency>
  <dependency name="omni_client_library" linkPath="../_build/target-deps/omni_client_library">
  </dependency>
</project>
```

NOTE: This must be in packman.xml file that is pulled _after_ pulling this package.
You can't put it in the same packman.xml as the one that pulls this package.

This requires packman 6.4 or later.

## Initializing

You must either copy the omni_usd_resolver plugin to the default USD plugin location, or register
the plugin location at application startup using `PXR_NS::PlugRegistry::GetInstance().RegisterPlugins`.

Be sure to package both the library (.dll or .so) and the "plugInfo.json" file. Be sure to keep the
folder structure the same for the "plugInfo.json" file. It should look like this:

- omni_usd_resolver.dll or omni_usd_resolver.so
- usd/omniverse/resources/plugInfo.json

If you use `RegisterPlugins`, provide it the path to the "resources" folder.
Otherwise, you can copy the entire 'debug' or 'release' folders into the standard USD folder structure.

## Live Mode

In order to send/receive updates you must:
1. `#include <OmniClient.h>` (from client library)
2. Create or open a ".live" file on an Omniverse server
3. Call `omniClientLiveProcess();` periodically

For "frame based" applications, you can safely just call `omniClientLiveProcess` inside your main loop.

For event based applications, you can register a callback function using `omniClientLiveSetQueuedCallback` to receive
a notification that an update is queued and ready to be processed.

In either case, make sure that nothing (ie: no other thread) is using the USD library when you call `omniClientLiveProcess`
because it will modify the layers and that is not thread safe.
