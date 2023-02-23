__copyright__ = "Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved."
__license__ = """
NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
"""

import unittest

from omni.bind import _util


class Counter:
    def __init__(self) -> None:
        self.value = 0

    def next(self) -> int:
        self.value += 1
        return self.value


class LazyTest(unittest.TestCase):
    def test_regular_usage(self):
        counter = Counter()

        var = _util.Lazy(counter.next)
        self.assertEqual(0, counter.value)
        self.assertEqual(1, var.get())
        self.assertEqual(1, var.get())

        var.clear()
        self.assertEqual(2, var.get())
        self.assertEqual(2, var.get())

        var.set(-9)
        self.assertEqual(-9, var.get())
        self.assertEqual(2, counter.value)  # did not access counter.next function again

        var.clear()
        var.clear()  # clearing multiple times should not throw

        self.assertEqual(3, var.get())
        self.assertEqual(3, var.get())

    def test_function_usage(self):
        counter = Counter()

        var = _util.lazy(counter.next)
        self.assertEqual(0, counter.value)
        self.assertEqual(1, var())
        self.assertEqual(1, var())
        self.assertEqual(1, counter.value)

    def test_ctor_checks_callable(self):
        with self.assertRaises(TypeError):
            _util.Lazy(5)
