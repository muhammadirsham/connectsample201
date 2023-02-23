__copyright__ = "Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved."
__license__ = """
NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
"""

import io
import typing
import unittest

from omni.bind import diagnostics
from omni.bind.diagnostics import (
    Color,
    DiagnosticException,
    DiagnosticLevel,
    DiagnosticMessage,
    DiagnosticReporter,
    TextDiagnosticReporter,
    get_source_location,
)


class SourceLocation(typing.NamedTuple):
    file: str
    line: int
    column: int


class Cursor(typing.NamedTuple):
    location: SourceLocation


class RaiseOnRepr(object):
    """
    An instance that raisees an exception when its ``__repr`` or ``__str__`` is called. This is
    useful for testing that ``DiagnosticReporter`` discards message format arguments without
    attempting to put them in ``str.format``.
    """

    def __repr__(self) -> str:
        raise NotImplementedError("__repr__ cannot be called")

    def __str__(self) -> str:
        raise NotImplementedError("__str__ cannot be called")


class DiagnosticsLevelTest(unittest.TestCase):
    def test_parse(self):
        cases = (
            ("IGNORED", DiagnosticLevel.IGNORED),
            ("verbose", DiagnosticLevel.VERBOSE),
            ("VeRbOsE", DiagnosticLevel.VERBOSE),
        )

        for source, expected in cases:
            with self.subTest(source):
                actual = DiagnosticLevel.parse(source)
                self.assertEqual(expected, actual)
                self.assertEqual(source.lower(), str(actual))

        self.assertRaises(ValueError, DiagnosticLevel.parse, "")
        self.assertRaises(ValueError, DiagnosticLevel.parse, "notAValue")


class SourceLocationTest(unittest.TestCase):
    def test_basic(self):
        loc = SourceLocation("foo/bar.hpp", 60, 8)
        self.assertEqual(str(diagnostics.get_source_location(loc)), "foo/bar.hpp:60:8")
        self.assertEqual(loc, get_source_location(loc))
        self.assertEqual(loc, get_source_location(Cursor(loc)))
        self.assertIsNone(get_source_location(None))
        self.assertRaises(TypeError, get_source_location, 5)

    def test_get_source_location_identity(self):
        orig = diagnostics.SourceLocation("file.cpp", 0, 0)
        self.assertEqual(orig, diagnostics.get_source_location(orig))
        self.assertEqual(str(orig), "file.cpp")
        self.assertEqual(str(diagnostics.SourceLocation("bar.cpp", 9, 0)), "bar.cpp:9")


class DiagnosticMessageTest(unittest.TestCase):
    def test_basic(self):
        self.assertEqual("warning: No source", str(DiagnosticMessage(DiagnosticLevel.WARNING, None, "No source")))

        msg = DiagnosticMessage(DiagnosticLevel.NOTE, SourceLocation("file.cpp", 40, 8), "A simple message")
        self.assertEqual("file.cpp:40:8: note: A simple message", str(msg))

    def test_print_multiline(self):
        msg = DiagnosticMessage(
            DiagnosticLevel.VERBOSE, SourceLocation("path/somewhere.hpp", 94, 55), "A multiple\nline\nmessage"
        )
        expected = """path/somewhere.hpp:94:55: verbose: A multiple
line
message"""
        self.assertEqual(expected, str(msg))

        with self.subTest("indentation"):
            expected = """\
            path/somewhere.hpp:94:55: verbose: A multiple
            line
            message"""
            sbuf = io.StringIO()
            msg.print_into(sbuf, indent=3)
            self.assertEqual(expected, sbuf.getvalue())

    def test_print_nested(self):
        msg = DiagnosticMessage(
            DiagnosticLevel.ERROR,
            None,
            "Found ambiguous operator<<, let me tell you about them",
            (
                DiagnosticMessage(
                    DiagnosticLevel.VERBOSE, SourceLocation("snark.c", 100, 8), "Best sit down\nThis could take a bit"
                ),
                DiagnosticMessage(DiagnosticLevel.NOTE, SourceLocation("foo.hpp", 40, 4), "Oh wow operator<<"),
                DiagnosticMessage(
                    DiagnosticLevel.WARNING, None, "Using operator<< for printing was maybe not a good idea"
                ),
            ),
        )
        expected_simple = """error: Found ambiguous operator<<, let me tell you about them (3 related messages)"""
        expected = f"""{Color.BOLD}{Color.ERROR}error: Found ambiguous operator<<, let me tell you about them{Color.END} (3 related messages)

    {Color.BOLD}snark.c:100:8: verbose: Best sit down{Color.END}
    This could take a bit

    {Color.BOLD}foo.hpp:40:4: note: Oh wow operator<<{Color.END}

    {Color.BOLD}{Color.WARNING}warning: Using operator<< for printing was maybe not a good idea{Color.END}"""
        self.assertEqual(expected_simple, str(msg))

        sbuf = io.StringIO()
        msg.print_into(sbuf, decorate=True)
        self.assertEqual(expected, sbuf.getvalue())

    def test_from_clang(self):
        # There are no Python bindings to directly create a Clang Diagnostic, so compile a small
        # file that will cause a fatal error
        from clang import cindex

        source_code = """#include <some/nonsense/that/does/not/exist.hpp>"""
        tu = cindex.TranslationUnit.from_source(
            "source.cpp", args=["--std=c++17"], unsaved_files=[("source.cpp", source_code)]
        )
        self.assertEqual(1, len(tu.diagnostics), list(tu.diagnostics))
        clang_diag = tu.diagnostics[0]
        our_diag = DiagnosticMessage.from_clang(clang_diag)
        self.assertEqual(clang_diag.severity, cindex.Diagnostic.Fatal)
        self.assertEqual(our_diag.level, DiagnosticLevel.FATAL)
        self.assertEqual(str(clang_diag.location.file), "source.cpp")
        self.assertEqual(our_diag.source.file, "source.cpp")
        self.assertEqual(clang_diag.location.line, 1)
        self.assertEqual(our_diag.source.line, 1)
        self.assertRegex(clang_diag.format(0), "some/nonsense/that/does/not/exist.hpp")
        self.assertRegex(our_diag.message, "some/nonsense/that/does/not/exist.hpp")


class DiagnosticReporterTest(unittest.TestCase):
    def test_basic_use(self):
        reporter = TextDiagnosticReporter(output=io.StringIO())
        self.assertEqual(DiagnosticLevel.NOTE, reporter.diagnostic_log_level)
        self.assertEqual(DiagnosticLevel.ERROR, reporter.diagnostic_error_level)
        self.assertListEqual([], reporter.diagnostics)
        self.assertLess(reporter.worst_diagnostic_level, DiagnosticLevel.VERBOSE)
        self.assertFalse(reporter.colors_enabled)

        expected_diags = []
        self.assertIsNone(reporter.verbose(SourceLocation("file.cpp", 50, 8), "Ignore me {}", RaiseOnRepr()))
        self.assertListEqual(expected_diags, reporter.diagnostics)
        with reporter.nest_verbose(None, "Also ignored") as sub_reporter:
            self.assertEqual(reporter.diagnostic_error_level, sub_reporter.diagnostic_error_level)
        self.assertListEqual(expected_diags, reporter.diagnostics)

        reporter.diagnostic_log_level = DiagnosticLevel.VERBOSE
        reporter.diagnostic_error_level = DiagnosticLevel.WARNING
        diag = reporter.verbose(None, "No longer ignored {}", 10)
        self.assertIsNotNone(diag)
        expected_diags.append(diag)
        self.assertListEqual(expected_diags, reporter.diagnostics)
        with reporter.nest(DiagnosticLevel.IGNORED, None, "Still ignored") as sub_reporter:
            self.assertEqual(sub_reporter.diagnostic_log_level, DiagnosticLevel.VERBOSE)
            self.assertEqual(sub_reporter.diagnostic_error_level, DiagnosticLevel.WARNING)

            del reporter.diagnostic_log_level
            self.assertEqual(sub_reporter.diagnostic_log_level, DiagnosticLevel.NOTE)
            self.assertEqual(reporter.diagnostic_log_level, DiagnosticLevel.NOTE)

            sub_reporter.diagnostic_error_level = DiagnosticLevel.NOTE
            del reporter.diagnostic_error_level
            self.assertEqual(sub_reporter.diagnostic_error_level, DiagnosticLevel.NOTE)
            self.assertEqual(reporter.diagnostic_error_level, DiagnosticLevel.ERROR)
        self.assertListEqual(expected_diags, reporter.diagnostics)

    def test_context(self):
        sbuf = io.StringIO()
        try:
            with TextDiagnosticReporter(sbuf) as reporter:
                reporter.warning(None, "Just a warning message")
                reporter.error(None, "Wololo (the one that will be thrown)")
                reporter.error(None, "Second error is still captured")
        except DiagnosticException as ex:
            self.assertRegex(ex.diagnostic.message, "Wololo")

        sout = sbuf.getvalue()
        self.assertRegex(sout, "Just a warning message")
        self.assertRegex(sout, "Second error is still captured")

    def test_context_success(self):
        with DiagnosticReporter() as reporter:
            reporter.warning(None, "Not an error")

    def test_context_exit_with_exception(self):
        sbuf = io.StringIO()
        try:
            with TextDiagnosticReporter(sbuf) as reporter:
                reporter.warning(None, "Just a warning message")
                with reporter.nest_error(None, "It's getting worse") as sub_reporter:
                    sub_reporter.note(None, "Still reported")
                    raise NotImplementedError("Something horrible happened")
        except NotImplementedError as ex:
            self.assertRegex(str(ex), "Something horrible happened")

        sout = sbuf.getvalue()
        self.assertRegex(sout, "Just a warning message")
        self.assertRegex(sout, "It's getting worse")
        self.assertRegex(sout, "Still reported")
        self.assertRegex(sout, "Something horrible happened")

    def test_report_nested(self):
        reporter = TextDiagnosticReporter(output=io.StringIO())
        with reporter.nest_error(None, "Oh no") as sub_reporter:
            self.assertEqual(DiagnosticLevel.ERROR, sub_reporter.root_level)
            sub_diags = []
            for value in range(5):
                diag = sub_reporter.note(None, "Bad value {value}", value=value)  # <-- not worse
                self.assertIsNotNone(diag)
                sub_diags.append(diag)
        sub_diags = tuple(sub_diags)

        self.assertEqual(1, len(reporter.diagnostics))
        diag = reporter.diagnostics[0]
        self.assertEqual(DiagnosticLevel.ERROR, diag.level)
        self.assertTupleEqual(sub_diags, diag.nested)

    def test_report_nested_escalation_through_message(self):
        reporter = TextDiagnosticReporter(output=io.StringIO())
        with reporter.nest_warning(None, "Oh no") as sub_reporter:
            self.assertEqual(DiagnosticLevel.WARNING, sub_reporter.root_level)
            sub_diags = []
            for value in range(5):
                diag = sub_reporter.fatal(None, "Bad value {value}", value=value)  # <-- cause
                self.assertIsNotNone(diag)
                sub_diags.append(diag)
        sub_diags = tuple(sub_diags)

        self.assertEqual(1, len(reporter.diagnostics))
        diag = reporter.diagnostics[0]
        self.assertEqual(DiagnosticLevel.FATAL, diag.level)
        self.assertTupleEqual(sub_diags, diag.nested)

    def test_report_nested_escalation_through_root_level(self):
        reporter = TextDiagnosticReporter(output=io.StringIO())
        with reporter.nest_note(None, "Nothing bad") as sub_reporter:
            self.assertEqual(DiagnosticLevel.NOTE, sub_reporter.root_level)
            sub_diags = []
            for value in range(5):
                diag = sub_reporter.warning(None, "Bad value {value}", value=value)
                self.assertIsNotNone(diag)
                sub_diags.append(diag)
            sub_reporter.root_level = DiagnosticLevel.FATAL  # <-- cause
        sub_diags = tuple(sub_diags)

        self.assertEqual(1, len(reporter.diagnostics))
        diag = reporter.diagnostics[0]
        self.assertEqual(DiagnosticLevel.FATAL, diag.level)
        self.assertTupleEqual(sub_diags, diag.nested)

    def test_report_nested_descalation_through_root_level(self):
        reporter = TextDiagnosticReporter(output=io.StringIO())
        with reporter.nest_fatal(None, "Looks really bad") as sub_reporter:
            self.assertEqual(DiagnosticLevel.FATAL, sub_reporter.root_level)
            sub_diags = []
            for value in range(5):
                diag = sub_reporter.error(None, "Bad value {value}", value=value)  # <-- cause
                self.assertIsNotNone(diag)
                sub_diags.append(diag)
            sub_reporter.root_level = DiagnosticLevel.NOTE  # <-- not so bad
        sub_diags = tuple(sub_diags)

        self.assertEqual(1, len(reporter.diagnostics))
        diag = reporter.diagnostics[0]
        self.assertEqual(DiagnosticLevel.ERROR, diag.level)
        self.assertTupleEqual(sub_diags, diag.nested)

    def test_raises_on_bad_source(self):
        reporter = diagnostics.default_reporter()
        self.assertRaises(TypeError, reporter.error, "not a full source", "message")
        self.assertRaises(TypeError, reporter.nest_error, "not a full source", "message")
