__copyright__ = "Copyright (c) 2020-2022, NVIDIA CORPORATION. All rights reserved."
__license__ = """
NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
#include <omni/python/PyString.h>
"""


def _get_dependencies():
    from pathlib import Path

    """
    Gets the list of .py files needed by this module.

    This allows the dependency checker to mark output as dirty if the omni.bind module changes.
    """
    deps = []
    cwd = Path.cwd()
    f = Path(__file__)
    if not f.is_absolute():
        f = cwd / f

    root = f.parent / ".." / ".."

    deps = []
    for path in root.rglob("*.py"):
        deps.append(str(path.relative_to(cwd)))

    return deps
