import os
import sys
import platform
import argparse
import subprocess
import shutil
import multiprocessing

import packmanapi

TOOL_VERSION = "0.3.3"
SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))


def run_executable(args):
    try:
        p = subprocess.Popen(args, stdout=sys.stdout, stderr=subprocess.STDOUT)
        returncode = p.wait()
        if returncode != 0:
            print("Error running: %s" % args)
            sys.exit(returncode)
        return returncode
    except subprocess.CalledProcessError as e:
        print("Error running: %s" % args)
        print("Code '%s', message '%s'" % (e.returncode, e.output))
        sys.exit(1)


def run_command():
    parser = argparse.ArgumentParser()
    parser.add_argument('-g', '--generateprojects-only',
                        action='store_true', dest='generate_only', required=False)
    parser.add_argument('-j', '--jobs', dest='jobs',
                        required=False, default=-1)
    parser.add_argument('-q', '--quiet', action='store_true',
                        dest='quiet', required=False)
    parser.add_argument('-x', '--rebuild', action='store_true',
                        dest='rebuild', required=False)
    parser.add_argument('-d', '--debug-only',
                        action='store_true', dest='debug_only', required=False)
    parser.add_argument('-r', '--release-only',
                        action='store_true', dest='release_only', required=False)
    parser.add_argument('-p', '--platform-target',
                        dest='platform_target', required=True)
    parser.add_argument('--deps-host',
                        dest='deps_host', action='append', required=False)
    parser.add_argument('--deps-target',
                        dest='deps_target', action='append', required=False)
    parser.add_argument('--root',
                        dest='root', required=False)
    parser.add_argument('--premake-tool',
                        dest='premake_tool', required=True)
    parser.add_argument('--premake-file',
                        dest='premake_file', required=False, default="premake5.lua")
    parser.add_argument('--sln',
                        dest='sln', required=False)
    parser.add_argument('--msbuild-tool',
                        dest='msbuild_tool', required=False)
    parser.add_argument('--vs-version',
                        dest='vs_version', required=False, default="vs2017")

    options, leftover_args = parser.parse_known_args()

    if options.root:
        repo_root = options.root
    else:
        repo_root = os.getcwd()

    arch = platform.machine()
    if arch == "AMD64":
        arch = "x86_64"
    platform_host = platform.system().lower() + "-" + arch

    # Validate platform host early
    supported_platforms_host = "windows-x86_64 linux-x86_64 linux-aarch64"
    if platform_host not in supported_platforms_host:
        print("Unsupported host platform: %s" % (platform_host))
        sys.exit(1)

    if platform_host == "windows-x86_64":
        if not options.sln:
            print("To build using MSBuild on Windows, you need to specify the solution path")
            sys.exit(1)
        if not options.msbuild_tool:
            print("To build using MSBuild on Windows, you need to specify the MSBuild path")
            sys.exit(1)

    # Buildscript
    # TODO: improve logic that determines if rebuild should be triggered
    generate_projects = True
    build = True
    build_release = True
    build_debug = True

    if options.generate_only:
        build = False

    if options.release_only:
        build_debug = False

    if options.debug_only:
        build_release = False

    # Dependencies will be fetched from the "deps" folder files
    if options.deps_target:
        for deps_target in options.deps_target:
            packmanapi.pull(os.path.join(repo_root, deps_target), platform=options.platform_target)
    if options.deps_host:
        for deps_host in options.deps_host:
            packmanapi.pull(os.path.join(repo_root, deps_host), platform=platform_host)

    if generate_projects:
        premake_file = os.path.realpath(os.path.join(repo_root, options.premake_file))
        # Configure premake to run according to host platform
        if platform_host == "windows-x86_64":
            premake_args = [os.path.join(repo_root, options.premake_tool), "--verbose",
                            "--file=%s" % (premake_file), options.vs_version, "--platform-host=%s" % (platform_host)]
        elif platform_host in ["linux-x86_64", "linux-aarch64"]:
            premake_args = [os.path.join(repo_root, options.premake_tool), "--verbose",
                            "--file=%s" % (premake_file), "gmake2", "--platform-host=%s" % (platform_host),
                            "--os=linux"]
        run_executable(premake_args)

    if build:
        if platform_host == "windows-x86_64":
            # MSB8003 - WindowsSDKDir variable from the registry.
            # MSB8005 - The property 'NMakeCleanCommandLine' doesn't exist.
            msbuild_tool = os.path.join(repo_root, options.msbuild_tool)
            solution = os.path.join(repo_root, options.sln)
            ignore_warnings = "/NoWarn:MSB8003,MSB8005"
            verbosity = "/verbosity:minimal"
            if options.quiet:
                verbosity = "/verbosity:quiet"
            max_cpu_count = "/m"
            if options.jobs != -1:
                max_cpu_count = "/m:%s" % (options.jobs)
            if build_release:
                print("*** Building Release x64 ***")
                msbuild_args = [msbuild_tool, ignore_warnings, solution,
                                "/p:Configuration=release;Platform=x64", verbosity, max_cpu_count]
                if options.rebuild:
                    msbuild_args.append("/t:Rebuild")
                run_executable(msbuild_args)
            if build_debug:
                print("*** Building Debug x64 ***")
                msbuild_args = [msbuild_tool, ignore_warnings, solution,
                                "/p:Configuration=debug;Platform=x64", verbosity, max_cpu_count]
                if options.rebuild:
                    msbuild_args.append("/t:Rebuild")
                run_executable(msbuild_args)
        elif platform_host in ["linux-x86_64", "linux-aarch64"]:
            make_tool = "make"
            directory = "--directory=%s" % (
                os.path.join(repo_root, "_compiler/gmake2"))
            stop = "--stop"
            verbosity = "verbose=1"
            if options.quiet:
                verbosity = "verbose=0"
            if options.jobs != -1:
                jobs = "-j%s" % (options.jobs)
            else:
                num_cpus = multiprocessing.cpu_count()
                jobs = "-j%s" % num_cpus
            make_platform_target = options.platform_target.replace(
                "linux-", "")
            if build_release:
                make_args = [make_tool, directory, stop, "config=release_%s" % (
                    make_platform_target), verbosity, jobs]
                run_executable(make_args)
            if build_debug:
                make_args = [make_tool, directory, stop, "config=debug_%s" % (
                    make_platform_target), verbosity, jobs]
                run_executable(make_args)


def test():
    pass


if __name__ == "__main__":
    run_command()
elif __name__ == "__nvtool_test__":
    test()
