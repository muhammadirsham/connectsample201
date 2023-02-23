"""
Diagnostics
===========

Tools for reporting messages to the user.
"""

__copyright__ = "Copyright (c) 2020-2022, NVIDIA CORPORATION. All rights reserved."
__license__ = """
NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
"""

import sys
import types
import typing
from enum import IntEnum

if typing.TYPE_CHECKING:
    from clang.cindex import Diagnostic as _ClangDiagnostic  # pragma: no cover
else:
    _ClangDiagnostic = typing.Any

T = typing.TypeVar("T")


class Color:
    """A list of terminal color constants."""

    BOLD = "\033[1m"
    WARNING = "\033[93m"
    ERROR = "\033[91m"
    END = "\033[0m"

    @classmethod
    def for_level(cls, level: "DiagnosticLevel") -> typing.Optional[str]:
        """
        Get a message color for the given ``level``, if that message should be colored. Returns
        ``None`` if there is no associated color.
        """
        if level == DiagnosticLevel.WARNING:
            return cls.WARNING
        elif level >= DiagnosticLevel.ERROR:
            return cls.ERROR
        else:
            return None


class DiagnosticLevel(IntEnum):
    """
    Represents the severity of a given diagnostic message. They are ordered so that less-than means
    less severe (e.g.: ``DiagnosticLevel.NOTE < DiagnosticLevel.ERROR``).
    """

    IGNORED = 0
    VERBOSE = 1
    NOTE = 2
    WARNING = 3
    ERROR = 4
    FATAL = 5

    def __str__(self) -> str:
        return self.name.lower()

    @classmethod
    def parse(cls, source: str) -> "DiagnosticLevel":
        """
        Parse the ``source`` string as its :ref:`DiagnosticLevel` enumeration value. The casing of
        the input is ignored.

        Raises
        ------
        ValueError
            If the ``source`` is not a known value for :ref:`DiagnosticLevel`.
        """

        try:
            return cls[source.upper()]
        except KeyError:
            raise ValueError(f'No DiagnosticLevel named "{source}"') from None


if sys.version_info >= (3, 8):
    from typing import Protocol as _Protocol
else:
    # typing.Protocol was added in 3.8, but we can fall back to just using `object` for 3.7
    _Protocol = object  # pragma: no cover


class SourceLocation(typing.NamedTuple):
    """A location in source code."""

    file: str
    """Name of the file this is from or empty string if no file."""

    line: int
    """The line number, starting with 1. A value of 0 refers to a whole file."""

    column: int
    """The column number, starting with 1. A value of 0 refers to a whole line."""

    def __str__(self) -> str:
        out = str(self.file) if self.file else "<NO-FILE>"
        if self.line != 0:
            out += f":{self.line}"
            if self.column != 0:
                out += f":{self.column}"
        return out


class IsSourceLocation(_Protocol):
    """This protocol refers to an instance with ``file``, ``line``, and ``column`` members."""

    @property
    def file(self) -> str:
        """Name of the file this is from or empty string if no file."""
        ...  # pragma: no cover

    @property
    def line(self) -> int:
        """The line number, starting with 1. A value of 0 refers to a whole file."""
        ...  # pragma: no cover

    @property
    def column(self) -> int:
        """The column number, starting with 1. A value of 0 refers to a whole line."""
        ...  # pragma: no cover


def _is_source_location(source: typing.Any) -> bool:
    """Checks if ``source`` adheres to the ``SourceLocation`` protocol."""

    field_types = {"file": object, "line": int, "column": int}
    return all((isinstance(getattr(source, attrname, None), typ) for attrname, typ in field_types.items()))


class HasSourceLocation(_Protocol):
    """
    This protocol refers to an instance which has a ``location`` member property which returns
    something which :ref:`IsSourceLocation`.
    """

    @property
    def location(self) -> IsSourceLocation:
        """The location."""
        ...  # pragma: no cover


def _has_source_location(source: typing.Any) -> bool:
    """Checks if ``source`` adheres to the ``HasSourceLocation`` protocol."""

    return _is_source_location(getattr(source, "location", None))


if sys.version_info >= (3, 8):
    Source = typing.Union[SourceLocation, IsSourceLocation, HasSourceLocation]
    """
    A source for a message can be a source location or be an object with a property ``.location``
    which is a source location.
    """
else:
    Source = typing.Any  # pragma: no cover


def get_source_location(source: typing.Optional[Source]) -> typing.Optional[SourceLocation]:
    """
    Create an instance from the ``SourceLocation``-like ``source`` or an object with a property
    ``.location`` which is ``SourceLocation``-like. If ``source`` is already a ``SourceLocation``,
    it is returned as-is. Returns ``None`` if ``source`` is ``None`` or if ``source.location`` is
    ``None``.

    Raises
    ------
    TypeError
        If ``source`` is not ``None`` but does not have a ``SourceLocation``-like protocol.
    """

    if source is None:
        return None
    elif isinstance(source, SourceLocation):
        return source
    elif _is_source_location(source):
        # _is_source_location returns True iff these accesses will be valid, but MyPy does not
        # understand that.
        return SourceLocation(str(source.file), source.line, source.column)  # type: ignore
    elif _has_source_location(source):
        # Same as above
        return get_source_location(source.location)  # type: ignore
    else:
        raise TypeError(f"The `source` parameter must be a Source or None (actual: {source}")


class DiagnosticMessage(typing.NamedTuple):
    """
    Represents a message with 0 or more possible :ref:`nested` related issues.
    """

    level: DiagnosticLevel
    """The level this message was reported at."""

    source: typing.Optional[SourceLocation]
    """What is the origin of this message?"""

    message: str
    """A user-readable message."""

    nested: typing.Sequence["DiagnosticMessage"] = tuple()
    """
    If this message has associated sub-messages, they are in this list. This is useful in cases like
    ambiguous function overloads, where the call site is the error, but the ``nested`` property is
    filled with notes on the overloads that matched.
    """

    @classmethod
    def from_clang(cls, src: _ClangDiagnostic) -> "DiagnosticMessage":
        """Convert ``clang.cindex.Diagnostic`` message ``src`` to an internal representation."""
        message = src.format(0)
        return cls(
            level=DiagnosticLevel(src.severity + 1),  # Clang's levels are 1 lower
            source=get_source_location(src),
            message=message[message.find(":") + 1 :].strip(),  # Remove redundant "<level>: " intro
            nested=tuple(map(cls.from_clang, src.children)),
        )

    def __str__(self) -> str:
        import io

        out = io.StringIO()
        self.print_into(out, print_nested=False, decorate=False)
        return out.getvalue()

    def print_into(
        self, out: typing.TextIO, indent: int = 0, print_nested: bool = True, decorate: typing.Optional[bool] = None
    ) -> None:
        """
        Print the contents of this message into ``out``.

        Parameters
        ----------
        out : TextIO
            The output buffer to write into.

        indent : int
            The indentation level to write message contents into. This is used for printing nested
            messages.

        print_nested : bool
            Should :ref:`nested` messages be printed? If ``True``, the sub-messages will be printed
            with their ``indent`` increased by 1.

        decorate : bool | None
            If ``True``, text decorations for coloring and bolding will be used. If ``None``, the
            value will be automatically determined based on if ``out`` is a TTY.
        """
        indent_text = "    " * indent
        decorate = out.isatty() if decorate is None else decorate

        out.write(indent_text)
        if decorate:
            out.write(Color.BOLD)
            color_code = Color.for_level(self.level)
            if color_code:
                out.write(color_code)

        if self.source is not None:
            out.write(f"{self.source.file}:{self.source.line}:{self.source.column}: ")
        out.write(str(self.level))

        out.write(": ")
        first = True
        for line in self.message.splitlines():
            if not first:
                out.write("\n")
                out.write(indent_text)
            out.write(line)
            if first:
                first = False
                if decorate:
                    out.write(Color.END)

        if len(self.nested) > 0:
            out.write(f" ({len(self.nested)} related" f" message{'' if len(self.nested) == 1 else 's'})")

            if print_nested:
                for sub in self.nested:
                    out.write("\n\n")
                    sub.print_into(out, indent + 1, decorate=decorate)


class DiagnosticException(Exception):
    """Represents an error detected by a diagnostic reporter."""

    def __init__(self, message: DiagnosticMessage) -> None:
        super().__init__(str(message))
        self._diagnostic = message

    @property
    def diagnostic(self) -> DiagnosticMessage:
        """
        The diagnostic which caused this exception. The stringified version of this is the message
        of the exception.
        """
        return self._diagnostic


class DiagnosticReporter:
    """Used to track and report errors.

    The base class simply collects
    """

    def __init__(
        self,
        diagnostic_log_level: typing.Optional[DiagnosticLevel] = None,
        diagnostic_error_level: typing.Optional[DiagnosticLevel] = None,
        parent: typing.Optional["DiagnosticReporter"] = None,
    ):
        """
        Parameters
        ----------
        diagnostic_log_level : DiagnosticLevel
            Diagnostics below this level will not be emitted.

        diagnostic_error_level : DiagnosticLevel
            Diagnostics at or above this level will be reported as errors.

        parent : DiagnosticReporter | None
            If this reporter is nested, this is the
        """
        self._diagnostics: typing.List[DiagnosticMessage] = []

        self._worst_diagnostic_level = DiagnosticLevel.IGNORED
        self._diagnostic_log_level = diagnostic_log_level
        self._diagnostic_error_level = diagnostic_error_level
        self._parent = parent

    @property
    def diagnostics(self) -> typing.Sequence[DiagnosticMessage]:
        """Gets the list of diagnostic messages collected so far."""
        return self._diagnostics[:]

    @property
    def worst_diagnostic_level(self) -> DiagnosticLevel:
        """Get the worst-reported diagnostic level collected so far."""
        return self._worst_diagnostic_level

    def _value_for(self, selector: typing.Callable[["DiagnosticReporter"], typing.Optional[T]], default: T) -> T:
        val = selector(self)
        if val is not None:
            return val
        elif self.parent is not None:
            return self.parent._value_for(selector, default)
        else:
            return default

    @property
    def diagnostic_log_level(self) -> DiagnosticLevel:
        """Diagnostics below this level will be discarded."""
        return self._value_for(lambda x: x._diagnostic_log_level, DiagnosticLevel.NOTE)

    @diagnostic_log_level.setter
    def diagnostic_log_level(self, value: DiagnosticLevel) -> None:
        self._diagnostic_log_level = value

    @diagnostic_log_level.deleter
    def diagnostic_log_level(self) -> None:
        """Deleting the ignore level resets it to the default behavior."""
        self._diagnostic_log_level = None

    @property
    def diagnostic_error_level(self) -> DiagnosticLevel:
        """Diagnostics at or above this level should be considered as errors."""
        return self._value_for(lambda x: x._diagnostic_error_level, DiagnosticLevel.ERROR)

    @diagnostic_error_level.setter
    def diagnostic_error_level(self, value: DiagnosticLevel) -> None:
        self._diagnostic_error_level = value

    @diagnostic_error_level.deleter
    def diagnostic_error_level(self) -> None:
        self._diagnostic_error_level = None

    @property
    def parent(self) -> typing.Optional["DiagnosticReporter"]:
        """If this instance is a sub-reporter, this is the parent it reports to."""
        return self._parent

    @property
    def valid(self) -> bool:
        """
        If the :ref:`worst_diagnostic_level` is less bad than the :ref:`diagnostic_error_level`,
        then this collector is still considered "valid."

        :see:`validate`
        """
        return self.worst_diagnostic_level < self.diagnostic_error_level

    def validate(self) -> None:
        """
        If the :ref:`worst_diagnostic_level` is at least as bad as :ref:`diagnostic_error_level`,
        (this collector is not :ref:`valid`) then a :ref:`DiagnosticException` will be thrown with
        the cause.
        """
        if not self.valid:
            diagnostic = next(
                iter(
                    (
                        diag  # pragma: no cover -- intentionally not reaching end
                        for diag in self.diagnostics
                        if diag.level == self.worst_diagnostic_level
                    )
                )
            )
            raise DiagnosticException(diagnostic)

    def __enter__(self) -> "DiagnosticReporter":
        return self

    def __exit__(
        self,
        exc_type: typing.Optional[typing.Type[BaseException]],
        exc_val: typing.Optional[BaseException],
        exc_trace: typing.Optional[types.TracebackType],
    ) -> None:
        """
        Exiting context will automatically call :ref:`validate` if an exception is not being
        thrown, which might cause a :ref:`DiagnosticException` to be thrown. If an unhandled
        exception is raised, it is automatically logged as a fatal diagnostic, but will pass
        through.
        """
        if exc_type is None:
            self.validate()
        else:
            self.fatal(None, "Got {} exception during processing: {}", exc_type.__name__, exc_val)

    def emit_message(self, msg: DiagnosticMessage) -> typing.Optional[DiagnosticMessage]:
        """
        Emit the ``msg`` diagnostic if it is not ignored.

        Returns
        -------
        The emitted message (which is exactly ``msg``) if this reporter accepted it. If it was not
        accepted, then ``None`` is returned.
        """
        if not isinstance(msg, DiagnosticMessage):  # pragma: no cover
            raise TypeError(f"Expected `DiagnosticMessage`, not {type(msg).__name__}")

        if msg.level < self.diagnostic_log_level:
            return None

        self._diagnostics.append(msg)
        if msg.level > self._worst_diagnostic_level:
            self._worst_diagnostic_level = msg.level
        return msg

    def emit(
        self, level: DiagnosticLevel, source: typing.Optional[Source], message: str, *format_args, **format_kwargs
    ) -> typing.Optional[DiagnosticMessage]:
        """
        Emit a diagnostic message. The exact action depends on the configuration of the collector
        and the value of the ``level`` parameter. Basic behavior is to add the message to the
        collection if ``level > self.diagnostic_log_level`` and discard it otherwise.

        Returns
        -------
        If the message was emitted, this returns the emitted message structure. If it was ignored,
        then ``None`` is returned.
        """
        if level < self.diagnostic_log_level:
            return None

        return self.emit_message(
            DiagnosticMessage(level, get_source_location(source), str.format(message, *format_args, **format_kwargs))
        )

    def verbose(
        self, source: typing.Optional[Source], message: str, *format_args, **format_kwargs
    ) -> typing.Optional[DiagnosticMessage]:
        """Call :ref:`emit` with :ref:`DiagnosticLevel.VERBOSE`."""
        return self.emit(DiagnosticLevel.VERBOSE, source, message, *format_args, **format_kwargs)

    def note(
        self, source: typing.Optional[Source], message: str, *format_args, **format_kwargs
    ) -> typing.Optional[DiagnosticMessage]:
        """Call :ref:`emit` with :ref:`DiagnosticLevel.NOTE`."""
        return self.emit(DiagnosticLevel.NOTE, source, message, *format_args, **format_kwargs)

    def warning(
        self, source: typing.Optional[Source], message: str, *format_args, **format_kwargs
    ) -> typing.Optional[DiagnosticMessage]:
        """Call :ref:`emit` with :ref:`DiagnosticLevel.WARNING`."""
        return self.emit(DiagnosticLevel.WARNING, source, message, *format_args, **format_kwargs)

    def error(
        self, source: typing.Optional[Source], message: str, *format_args, **format_kwargs
    ) -> typing.Optional[DiagnosticMessage]:
        """Call :ref:`emit` with :ref:`DiagnosticLevel.ERROR`."""
        return self.emit(DiagnosticLevel.ERROR, source, message, *format_args, **format_kwargs)

    def fatal(
        self, source: typing.Optional[Source], message: str, *format_args, **format_kwargs
    ) -> typing.Optional[DiagnosticMessage]:
        """Call :ref:`emit` with :ref:`DiagnosticLevel.FATAL`."""
        return self.emit(DiagnosticLevel.FATAL, source, message, *format_args, **format_kwargs)

    def nest_message(self, msg: DiagnosticMessage) -> "DiagnosticReportBlock":
        """
        See :ref:`nest`.

        Note
        ----
        The message which will end up in the stream is based on this message, but will not be the
        same thing. The property :ref:`DiagnosticMessage.nested` will be filled with the messages
        added to the :ref:`DiagnosticReportBlock` returned from this function. The property
        :ref:`DiagnosticMessage.level` will be replaced with the most severe sub-message or left
        at the original level if all sub-messages are less severe.
        """
        return DiagnosticReportBlock(parent=self, initial_message=msg)

    def nest(
        self, level: DiagnosticLevel, source: typing.Optional[Source], message: str, *format_args, **format_kwargs
    ) -> "DiagnosticReportBlock":
        """
        Begin a nested diagnostic message.

        Parameters
        ----------
        The arguments behave identically to ``emit`` with the exception of ``level``.

        level : DiagnosticLevel
            This notes the original level of the message, which is usually the level the reported
            message will have. However, if a sub-message has a more severe level, the top-level
            message will have its severity escalated to the worst value.

        Example
        -------
        This is useful for reporting extra information::

            overloads = resolve_stuff()
            if len(overloads) > 1:
                # Start a block with the error message
                with diagnostics.nest_error(None, "Found {} overloads", len(overloads)) as sub:
                    for overload in overloads:
                        # Extra diagnostic notes added to the original error
                        sub.note(overload, "Valid overload {}", overload)
        """
        return self.nest_message(
            DiagnosticMessage(level, get_source_location(source), str.format(message, *format_args, **format_kwargs))
        )

    def nest_verbose(
        self, source: typing.Optional[Source], message: str, *format_args, **format_kwargs
    ) -> "DiagnosticReportBlock":
        """Call :ref:`nest` with :ref:`DiagnosticLevel.VERBOSE`."""
        return self.nest(DiagnosticLevel.VERBOSE, source, message, *format_args, **format_kwargs)

    def nest_note(
        self, source: typing.Optional[Source], message: str, *format_args, **format_kwargs
    ) -> "DiagnosticReportBlock":
        """Call :ref:`nest` with :ref:`DiagnosticLevel.NOTE`."""
        return self.nest(DiagnosticLevel.NOTE, source, message, *format_args, **format_kwargs)

    def nest_warning(
        self, source: typing.Optional[Source], message: str, *format_args, **format_kwargs
    ) -> "DiagnosticReportBlock":
        """Call :ref:`nest` with :ref:`DiagnosticLevel.WARNING`."""
        return self.nest(DiagnosticLevel.WARNING, source, message, *format_args, **format_kwargs)

    def nest_error(
        self, source: typing.Optional[Source], message: str, *format_args, **format_kwargs
    ) -> "DiagnosticReportBlock":
        """Call :ref:`nest` with :ref:`DiagnosticLevel.ERROR`."""
        return self.nest(DiagnosticLevel.ERROR, source, message, *format_args, **format_kwargs)

    def nest_fatal(
        self, source: typing.Optional[Source], message: str, *format_args, **format_kwargs
    ) -> "DiagnosticReportBlock":
        """Call :ref:`nest` with :ref:`DiagnosticLevel.FATAL`."""
        return self.nest(DiagnosticLevel.FATAL, source, message, *format_args, **format_kwargs)


class DiagnosticReportBlock(DiagnosticReporter):
    """See :ref:`DiagnosticReporter.nest`."""

    def __init__(self, parent: DiagnosticReporter, initial_message: DiagnosticMessage, **kwargs) -> None:
        if parent is None:
            raise ValueError("DiagnosticReportBlock requires parent reporter")  # pragma: no cover

        super().__init__(parent=parent, **kwargs)
        self._initial_mesasge = initial_message

    def __enter__(self) -> "DiagnosticReportBlock":
        return self

    def __exit__(
        self,
        exc_type: typing.Optional[typing.Type[BaseException]],
        exc_val: typing.Optional[BaseException],
        exc_trace: typing.Optional[types.TracebackType],
    ) -> None:
        if exc_val is not None:
            super().__exit__(exc_type, exc_val, exc_trace)

        msg = self._initial_mesasge._replace(
            level=max(self.worst_diagnostic_level, self.root_level), nested=tuple(self.diagnostics)
        )
        assert self.parent is not None, "parent must have been set in `__init__`"
        self.parent.emit_message(msg)

    @property
    def root_level(self) -> DiagnosticLevel:
        """Get the level of the root message this report block will create."""
        return self._initial_mesasge.level

    @root_level.setter
    def root_level(self, value: DiagnosticLevel):
        self._initial_mesasge = self._initial_mesasge._replace(level=value)


class TextDiagnosticReporter(DiagnosticReporter):
    """
    A :ref:`DiagnosticReporter` which prints messages to some form of ``TextIO`` (e.g.:
    ``sys.stderr`` or ``io.StringIO``).
    """

    def __init__(
        self, output: typing.Optional[typing.TextIO] = None, enable_colors: typing.Optional[bool] = None, **kwargs
    ):
        super().__init__(**kwargs)
        self._output = sys.stderr if output is None else output
        self._colors_enabled = self._output.isatty() if enable_colors is None else enable_colors

    @property
    def output(self) -> typing.TextIO:
        """The output stream for this reporter."""
        return self._output

    @property
    def colors_enabled(self) -> bool:
        """Get if colors are enabled for this reporter."""
        return self._colors_enabled

    def emit_message(self, msg: DiagnosticMessage) -> typing.Optional[DiagnosticMessage]:
        """Extends :ref:`DiagnosticReporter.emit_message` by printing ``msg`` to the output."""

        msg2 = super().emit_message(msg)

        if msg2 is not None:
            msg2.print_into(self.output, decorate=self.colors_enabled)
            self.output.write("\n")

        return msg2


def default_reporter() -> DiagnosticReporter:
    """Create a :ref:`DiagnosticReporter` with the default configuration."""
    return TextDiagnosticReporter()
