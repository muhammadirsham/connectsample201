__copyright__ = "Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved."
__license__ = """
NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
"""

"""This module contains bindings to C++ carb::eventdispatcher::IEventDispatcher interface.
"""

# This module depends on other modules. VSCode python language server scrapes
# modules in an isolated environment (ignoring PYTHONPATH set). `import` fails and for that we have separate code
# path to explicitly add it's folder to sys.path and import again.
try:
    import carb
except:
    import os
    import sys

    sys.path.append(os.path.abspath(os.path.join(os.path.dirname(os.path.realpath(__file__)), "..", "..", "..")))
    import carb

from functools import lru_cache

import carb.variant

from ._eventdispatcher import *


@lru_cache()
def get_eventdispatcher_interface() -> IEventDispatcher:
    """Returns cached :class:`carb.eventdispatcher.IEventDispatcher` interface"""
    return acquire_eventdispatcher_interface()


def get_eventdispatcher() -> IEventDispatcher:
    """Returns cached :class:`carb.eventdispatcher.IEventDispatcher` interface (shorthand)."""
    return get_eventdispatcher_interface()
