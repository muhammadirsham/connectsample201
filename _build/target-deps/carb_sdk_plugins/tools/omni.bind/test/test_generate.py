__copyright__ = "Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved."
__license__ = """
NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
"""

import tempfile
import unittest

from omni.bind import generate


class FileComparisonTest(unittest.TestCase):
    def test_same_contents(self):
        with tempfile.NamedTemporaryFile() as a, tempfile.NamedTemporaryFile() as b:
            for f in (a, b):
                f.writelines((f"{x}\n".encode("UTF-8") for x in range(25)))
                f.flush()

            result = generate.compare_files(a.name, b.name)
            self.assertTrue(result, result.differences)

    def test_same_contents_different_copyright(self):
        with tempfile.NamedTemporaryFile() as a, tempfile.NamedTemporaryFile() as b:
            for f, firstline in ((a, "1984"), (b, "2001")):
                f.write(f"// Copyright (c) {firstline}\n".encode("UTF-8"))
                f.writelines((f"{x}\n".encode("UTF-8") for x in range(25)))
                f.flush()

            result = generate.compare_files(a.name, b.name)
            self.assertTrue(result, result.differences)

    def test_same_contents_different_first_line_non_copyright(self):
        with tempfile.NamedTemporaryFile() as a, tempfile.NamedTemporaryFile() as b:
            for f, firstline in ((a, "1984"), (b, "2001")):
                f.write(f"{firstline}\n".encode("UTF-8"))
                f.writelines((f"{x}\n".encode("UTF-8") for x in range(25)))
                f.flush()

            result = generate.compare_files(a.name, b.name)
            self.assertFalse(result, result.differences)

            self.assertRegex(result.differences, r"(.){3} 1,4 \1{4}\n! 1984")
            self.assertRegex(result.differences, r"(.){3} 1,4 \1{4}\n! 2001")

    def test_different_contents(self):
        with tempfile.NamedTemporaryFile() as a, tempfile.NamedTemporaryFile() as b:
            for f, multiplier in ((a, 1), (b, 2)):
                f.writelines(f"{x * multiplier}\n".encode("UTF-8") for x in range(25))
                f.flush()

            result = generate.compare_files(a.name, b.name)
            self.assertFalse(result, result.differences)
