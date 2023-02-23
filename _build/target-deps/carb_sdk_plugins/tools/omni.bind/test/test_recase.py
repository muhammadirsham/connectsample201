__copyright__ = "Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved."
__license__ = """
NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
"""

"""
Omni.Bind Recase Tests
######################

Tests for the ``omni.bind.recase`` module.
"""

import unittest


class ConventionConversion(unittest.TestCase):
    """Unit tests for code convention conversion routines.

    .. note::
        This is just a placeholder test for now so that a unit test is run.
    """

    def test_snake_case(self):
        from omni.bind.recase import to_snake_case

        CASES = (("camelCase", "camel_case"), ("PascalCase", "pascal_case"))

        for source, expected in CASES:
            with self.subTest(f'source="{source}"'):
                self.assertEqual(expected, to_snake_case(source))
