import os
import sys
import copy
import glob
import platform
import argparse

# External modules
import packmanapi

def has_options_arg(options, arg):
    return hasattr(options, arg) and getattr(options, arg) is not None


def get_caller_filename():
    import inspect
    stackframes = inspect.stack()
    for frame_idx in range(len(stackframes)):
        frame_filename = stackframes[frame_idx][0].f_code.co_filename
        if frame_filename != __file__:
            return frame_filename


def get_repo_paths():
    script_filename = get_caller_filename()
    repo_folders = {}
    repo_folders["root"] = os.path.abspath(os.path.join(os.path.dirname(script_filename), "..", ".."))
    repo_folders["build"] = os.path.join(repo_folders["root"], "_build")
    repo_folders["compiler"] = os.path.join(repo_folders["root"], "_compiler")
    repo_folders["installer"] = os.path.join(repo_folders["root"], "_installer")
    repo_folders["builtpackages"] = os.path.join(repo_folders["root"], "_builtpackages")
    repo_folders["signedpackages"] = os.path.join(repo_folders["root"], "_signedpackages")
    repo_folders["unsignedpackages"] = os.path.join(repo_folders["root"], "_unsignedpackages")
    repo_folders["host_deps"] = os.path.join(repo_folders["build"], "host-deps")
    repo_folders["target_deps"] = os.path.join(repo_folders["build"], "target-deps")
    repo_folders["deps_xml_folder"] = os.path.join(repo_folders["root"], "deps")
    repo_folders["host_deps_xml"] = os.path.join(repo_folders["deps_xml_folder"], "host-deps.packman.xml")
    repo_folders["target_deps_xml"] = os.path.join(repo_folders["deps_xml_folder"], "target-deps.packman.xml")
    return repo_folders


def validate_platform(platform_kind, in_platform, supported_platforms):
    if in_platform not in supported_platforms:
        print("Unsupported %s platform: %s, only %s are supported" % (platform_kind, in_platform, supported_platforms))
        sys.exit(1)
    return in_platform


def get_and_validate_host_platform(supported_platforms):
    arch = platform.machine()
    if arch == "AMD64":
        arch = "x86_64"
    platform_host = platform.system().lower() + "-" + arch

    validate_platform("host", platform_host, supported_platforms)

    return platform_host


def fetch_deps(deps_dict, in_platform, host_deps_path):
    dependency_folders = {}
    for dep_name, dep_attrib in deps_dict.items():
        # Check platforms
        if "platforms" in dep_attrib:
            if in_platform not in dep_attrib["platforms"]:
                continue

        # Determine where to link the package to
        package_link_path = None
        if "link_path_host" in dep_attrib:
            package_link_path = os.path.join(
                host_deps_path, dep_attrib["link_path_host"])

        # First, check if source path linking is specified, if it is - just link the
        # source and do not fetch/install package
        if "source_path_host" in dep_attrib:
            if package_link_path is None:
                package_link_path = os.path.join(
                    host_deps_path, dep_name)
            dependency_folders[dep_name] = dep_attrib["source_path_host"]
            packmanapi.link(package_link_path, dep_attrib["source_path_host"])
            continue
        print("Installing %s v. %s to %s" % (dep_name, dep_attrib["version"], package_link_path))
        dep_folder = packmanapi.install(
            dep_name, package_version=dep_attrib["version"], link_path=package_link_path)
        dependency_folders.update(dep_folder)
    return dependency_folders


def run_script(script_to_run, global_name="__main__"):
    script_globals = dict.copy(globals())
    script_globals["__file__"] = script_to_run
    script_globals["__name__"] = global_name
    with open(script_to_run, "rb") as file_script_to_run:
        exec(compile(file_script_to_run.read(), script_to_run, 'exec'), script_globals)


def run_script_with_args(script_to_run, args, global_name="__main__"):
    saved_args = copy.deepcopy(sys.argv)
    sys.argv = [script_to_run]
    sys.argv.extend(copy.deepcopy(args))
    sys.path.append(os.path.dirname(script_to_run))
    run_script(script_to_run, global_name)
    sys.argv = copy.deepcopy(saved_args)


def run_script_with_sys_args(script_to_run, global_name="__main__"):
    run_script_with_args(script_to_run, sys.argv[1:], global_name)


def run_script_with_custom_args(script_to_run, args, global_name="__main__"):
    if not isinstance(args, list):
        import shlex
        parsed_args = shlex.split(args)
    else:
        parsed_args = args
    run_script_with_args(script_to_run, parsed_args, global_name)


def are_paths_equal(path1, path2):
    n_path1 = os.path.normpath(os.path.abspath(path1))
    n_path2 = os.path.normpath(os.path.abspath(path2))

    if os.name == 'nt':
        n_path1 = os.path.normcase(n_path1)
        n_path2 = os.path.normcase(n_path2)

    return n_path1 == n_path2
