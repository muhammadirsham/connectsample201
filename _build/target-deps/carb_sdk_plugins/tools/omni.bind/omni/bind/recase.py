__copyright__ = "Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved."
__license__ = """
NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
"""

import re


def to_snake_case(s):
    """Converts pascalCase or CamelCase to snake_case:

    pascalCase -> pascal_case
    CamelCase  -> camel_case
    """
    return s[0].lower() + re.sub("([A-Z])", lambda p: f"_{p.group(1).lower()}", s[1:])
