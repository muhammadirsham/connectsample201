"""
A collection of utility classes not meant for use outside of ``omni.bind``.
"""

__copyright__ = "Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved."
__license__ = """
NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
"""

import typing

_T = typing.TypeVar("_T")


class Lazy(typing.Generic[_T]):
    """
    Lazily evaluates a loader function and caches the result. This is useful for caching the result
    of an expensive function that might not be accessed.

    For example, consider an annoying to compute value that has a 5% and 10% chance to be printed::

        from random import random

        annoying = Lazy(lambda: sum((x // 711 for x in range(100200) if x % 101 == 63)))
        if random() < 0.05:
            print(f"Annoying value 5%: {annoying.get()}")
        if random() < 0.10:
            print(f"Annoying value 10%: {annoying.get()}")

    In most cases (85.5% of the time), the value of ``annoying`` will not be printed, so the
    complex comprehension will never be evaluated. If only one of the ``if`` branches is hit, then
    the value will be computed. In the case where both are hit (0.5% of the time), the cached value
    from the first evaluation will be reused in the second call to ``annoying.get()``.

    See Also
    --------
    :ref:`lazy`
    """

    __slots__ = ("_load", "_value")

    def __init__(self, load: typing.Callable[[], _T]) -> None:
        """
        Parameters
        ----------
        load : ``() -> T``
            The loader function to call to get the underlying value. Normally, this function will
            be called once.
        """
        if not callable(load):
            raise TypeError("Load function must be callable")

        self._load = load

    def get(self) -> _T:
        """
        Get the value. If this is the first time this function is called, the loader function will
        be called and the successful result will be saved. Subsequent calls to this function return
        the cached value.

        Raises
        ------
        Any exception that the ``load`` function can raise can be raised by this function. When an
        exception is raised, the next call to this function will call ``load`` again. In other
        words, exceptions thrown from load functions are not cached.
        """
        if not hasattr(self, "_value"):
            self._value = self._load()  # pylint: disable=attribute-defined-outside-init
        return self._value

    def set(self, value: _T) -> None:
        """
        Explicitly set the value to be returned from :ref:`get` to ``value``, bypassing the loader
        function.
        """
        self._value = value  # pylint: disable=attribute-defined-outside-init

    def clear(self) -> None:
        """
        Clear the cached value, if there is one. This is useful when using ``Lazy`` as a cheap
        cache and you have just invalidated it through an operation.
        """
        if hasattr(self, "_value"):
            del self._value


def lazy(load: typing.Callable[[], _T]) -> typing.Callable[[], _T]:
    """
    Similar to :ref:`Lazy`, but evaluated with ``var()`` instead of ``var.get()``. Prefer using
    ``lazy`` over ``Lazy`` if you only ever need to get values.

    The ``Lazy`` usage example with ``lazy`` instead::

        from random import random

        annoying = lazy(lambda: sum((x // 711 for x in range(100200) if x % 101 == 64)))
        if random() < 0.05:
            print(f"Annoying value 5%: {annoying()}")
        if random() < 0.10:
            print(f"Annoying value 10%: {annoying()}")
    """
    return Lazy(load).get
