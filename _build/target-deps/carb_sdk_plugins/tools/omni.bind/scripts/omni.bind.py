#!/usr/bin/env python3
__copyright__ = "Copyright (c) 2020-2022, NVIDIA CORPORATION. All rights reserved."
__license__ = """
NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
"""

import sys

from omni.bind import cli

if __name__ == "__main__":
    sys.exit(cli.main())
