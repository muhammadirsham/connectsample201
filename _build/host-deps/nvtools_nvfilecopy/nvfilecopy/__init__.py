import os
import sys
import shutil
from . import glob2
import json
import typing
import argparse
import platform
from string import Template
import logging
import difflib

try:
    import packmanapi
except:
    pass


version = "2.0.1"
warn_if_not_exist = False
follow_symlinks = True

logger = logging.getLogger("nvfilecopy")
logger.propagate = False

LOG_LEVEL = os.getenv("NVFILECOPY_LOGGING")
LOG_FILE = os.getenv("NVFILECOPY_LOGFILE")

if LOG_LEVEL == "DEBUG":
    if not LOG_FILE:
        print("Logging to file: %s" % LOG_FILE)
        LOG_FILE = "_builtpackages/nvfilecopy.log"
        if os.path.exists(LOG_FILE):
            os.remove(LOG_FILE)

    if not os.path.exists("_builtpackages"):
        os.makedirs("_builtpackages")

    # set up the file handle
    logger.setLevel(logging.DEBUG)
    file_handle = logging.FileHandler(LOG_FILE, "a")

    # setup the formatting
    formatter = logging.Formatter("%(asctime)s - %(name)s - %(levelname)s - %(message)s")
    file_handle.setFormatter(formatter)

    # add the handle
    logger.addHandler(file_handle)
else:
    logger.setLevel(logging.INFO)
    logger.addHandler(logging.StreamHandler())


def _merge_paths(pathA, pathB):
    diff = difflib.Differ()
    result = diff.compare(pathA, pathB)
    merged_path = ""
    for i in result:
        merged_path += i[-1]
    return merged_path


def _process_path(path, args={}):
    s = Template(path)
    new_path = s.substitute(args)
    new_path = new_path.replace("\\", os.path.sep)  # just incase windows paths are passed to linux
    new_path = new_path.replace("/", os.path.sep)  # just incase linux paths are passed to windows
    return new_path


def _destination_check(dst_path):
    if "." in os.path.split(dst_path)[1]:
        dst_path = os.path.dirname(dst_path)
    if not os.path.exists(dst_path):
        logging.debug("Making directory: %s" % dst_path)
        os.makedirs(dst_path)


def _strip_to_first_wildcard(src, dst, fi):
    # we split the string based on the first wildcard
    src_first = src.split("*", 1)[0]
    # then we split by the last slash to make sure that the wildcard wasn't in a file or directory name.
    tmp = src_first.rsplit(os.path.sep, 1)[0]
    # finally we take the file path, strip the path to the first wildcard and then add the new destination info.
    file_dst = fi[len(tmp) :]
    if file_dst.startswith(os.path.sep):
        file_dst = file_dst[1:]
    return os.path.join(dst, file_dst)


def _is_file_copy_needed(src_file_path, dst_file_path):
    # unfortunately we sometimes get 'dst_file_path' as a directory so must handle that case here
    if os.path.isdir(dst_file_path):
        dst_file_path = os.path.join(dst_file_path, os.path.basename(src_file_path))
    if os.path.exists(dst_file_path):
        src_time = int(os.path.getmtime(src_file_path))
        dst_time = int(os.path.getmtime(dst_file_path))
        # we only copy if modification time differs
        if src_time == dst_time:
            logging.debug("File copy not needed: %s => %s" % (src_file_path, dst_file_path))
            return False
    return True


def _delete_file(src_path, dst_path):
    file_path = dst_path
    if os.path.isdir(dst_path):
        file_path = os.path.join(dst_path, os.path.split(src_path)[1])
    logger.info("Removing old file: %s" % file_path)
    if os.path.exists(file_path):
        if platform.system() == "Windows":
            os.system("del /q /f %s > nul 2>&1" % file_path)
        else:
            os.system("rm -f %s > /dev/null 2>&1" % file_path)


def _do_copy_item(src_path, dst_path):
    # this is to handle the case where a file with no extension is put into a folder with the same name by glob2 - 2py3 > 2py3/2py3
    extension = os.path.splitext(src_path)
    if not extension[1] and os.path.isfile(src_path):
        filename = os.path.split(src_path)[1]
        if dst_path.endswith(filename):
            dst_path = os.path.split(dst_path)[0]
            if not os.path.exists(dst_path):
                logging.debug("Creating directory: %s" % dst_path)
                os.makedirs(dst_path)

    if _is_file_copy_needed(src_path, dst_path):
        logger.info("Copying: '%s' -> '%s'" % (src_path, dst_path))
        _destination_check(dst_path)
        try:
            if os.path.exists(dst_path) and os.path.isfile(dst_path):
                logger.debug("Old file found, removing before copying the new file")
                _delete_file(src_path, dst_path)
            if os.path.islink(src_path) and follow_symlinks is False:
                logger.debug("Src path is a link: %s" % src_path)
                logger.debug("Dst path: %s" % dst_path)
                shutil.copy2(src_path, dst_path, follow_symlinks=follow_symlinks)
            else:
                logger.debug("Src path is a file: %s" % src_path)
                logger.debug("Dst path: %s" % dst_path)
                shutil.copy2(src_path, dst_path)
        except IOError:
            logger.error("Error: unable to process %s" % src_path, exc_info=True)
            raise Exception("Error: unable to process %s" % src_path)


def copy_files(src_path, dst_path, recurse_dir_links=False, args={}):
    """Copy files using wildcards.

    Args:
        src_path (str): Src path mask.
        dst_path (str): Destination path mask.
        recurse_dir_links (bool, optional): If True follow folder links when copying (). Defaults to False.
        args (Dict, optional): Substitution args.

    Examples:
        Copy all python files from one folder to another:

        .. code-block:: python

            nvfilecopy.copy_files("folder1/*.py", "folder2")

        Copy whole folder:

        .. code-block:: python

            nvfilecopy.copy_files("folder1/**", "folder2")

        Substitution:

        .. code-block:: python
        
            nvfilecopy.copy_files("folder1/{config}/**", "folder2", args={"config": "debug"})
    """
    src_path = _process_path(src_path, args=args)
    dst_path = _process_path(dst_path, args=args)
    if os.path.isdir(src_path):
        logger.debug("Src path is a dir, adding '**'")
        logger.debug("Src Path: %s" % src_path)
        src_path = os.path.join(src_path, "**")
    if "*" in src_path:
        logger.debug("Wildcard found in src_path: %s" % src_path)
        for fi in glob2.glob(src_path, recursive=True):
            src = os.path.join(os.getcwd(), fi)
            logger.debug("Src: %s" % src)
            logger.debug("dst_path: %s" % dst_path)
            logger.debug("fi: %s" % fi)
            if os.path.isdir(src):
                logger.debug("Src is a directory: %s" % src)
                if os.path.islink(src):
                    logger.debug("Src is a symlink: %s" % src)
                    if recurse_dir_links:
                        # dst_tmp = os.path.join(dst_path, os.path.split(src_path)[0], os.path.split(src)[1])
                        dst_tmp = os.path.join(_merge_paths(dst_path, fi))
                        logger.debug("dst_tmp: %s" % dst_tmp)
                        # we have to check if the original search filter had something like bala/bla/*.py and use it
                        sf = os.path.split(src_path)[1]
                        if "*" in sf:
                            src = os.path.join(src, sf)
                        copy_files(src, dst_tmp, recurse_dir_links=recurse_dir_links)
                continue
            dst = _strip_to_first_wildcard(src_path, dst_path, fi)
            # we do a split here because we want to check that the directory structure exists, not the file.
            _do_copy_item(src, dst)
    else:
        # lets assume its a file.
        if os.path.exists(src_path):
            _do_copy_item(src_path, dst_path)
        else:
            if warn_if_not_exist:
                logger.warning("Warning: Path does not exist: %s" % src_path)
            else:
                logger.error("Error: Path does not exist: %s" % src_path, exc_info=True)
                raise RuntimeError("Path does not exist: %s" % src_path)


# This needs to go.
def copy_item(src_path, dst_path, file_exclusions=[], recurse_dir_links=False, args={}):
    copy_files(src_path, dst_path, recurse_dir_links=recurse_dir_links, args=args)


def link_folders(src_path, dst_path, args={}):
    """Create folder link pointing to `src_path` named `dst_path`.

    Args:
        src_path (str): Source folder link points to.
        dst_path (str): New folder path to become a link. Should not exist.
        args (Dict, optional): Substitution args.
    """
    src_path = _process_path(src_path, args=args)
    dst_path = _process_path(dst_path, args=args)
    src_path = os.path.abspath(src_path)
    dst_path = os.path.abspath(dst_path)
    logger.info(f"Creating a link {src_path} -> {dst_path}")
    packmanapi.link(dst_path, src_path)


def copy_files_using_json_config(json_file: str, key: str = None, args: typing.Dict = {}):
    """Copy files using json config.

    Use json keys as source, values as destinations.

    Args:
        json_file (str): Json file path.
        key (str, optional): Json root key to use. Defaults to None.
        args (Dict, optional): Substitution args.
    """
    with open(json_file) as f:
        data = json.load(f)

    if key is not None:
        data = data.get(key)
    for src in data:
        copy_files(src, data.get(src), args=args)


def link_folders_using_json_config(json_file: str, key: str = None, args: typing.Dict = {}):
    """Create folder link using json config.

    Use json keys what link should point to and json values as folder link names to be created.

    Args:
        json_file (str): Json file path.
        key (str, optional): Json root key to use. Defaults to None.
        args (Dict, optional): Substitution args.
    """
    with open(json_file) as f:
        data = json.load(f)

    if key is not None:
        data = data.get(key)
    for src in data:
        link_folders(src, data.get(src), args=args)
