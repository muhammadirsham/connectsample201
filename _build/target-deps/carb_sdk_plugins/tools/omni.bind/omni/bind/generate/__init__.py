__copyright__ = "Copyright (c) 2020-2022, NVIDIA CORPORATION. All rights reserved."
__license__ = """
NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
"""

import difflib
import importlib
import importlib.util
import os
import re
import shutil
import sys
import typing
from datetime import date

from omni.bind._util import lazy
from omni.bind.diagnostics import DiagnosticReporter


class CompareResult(typing.NamedTuple):
    """See :ref:`compare_files`."""

    are_same: bool
    """If the two compared files are the same, this is ``True``."""

    differences: str = ""
    """A string describing the differences between the two files."""

    def __bool__(self) -> bool:
        """Equivalent to ``self.are_same``."""
        return self.are_same


def compare_files(current_filename: str, generated_filename: str) -> CompareResult:
    """
    Compare the contents of ``current_filename`` and ``generated_filename`` for differences. If the
    files are different, the returned value will have :ref:`CompareResult.are_same` as ``False``
    and a description of the differences will be stored in :ref:`CompareResult.differences`. If the
    files are the same, then ``are_same`` will be ``True`` and the ``differences`` string will be
    empty.
    """

    with open(generated_filename, "r") as new_file:
        with open(current_filename, "r") as old_file:

            def read_lines(f: typing.TextIO):
                # Skip first line if it contains the copyright comment. This behavior should vary
                # depending on the source language, but for now we're only generating C++ code. It
                # makes the most sense to assocate this with the generator, since it would know
                # where it keeps the copyright info. this should be fixed when adding other
                # generators.
                lines = f.readlines()
                if lines and lines[0].startswith("// Copyright"):
                    lines = lines[1:]
                return lines

            old_lines = read_lines(old_file)
            gen_lines = read_lines(new_file)

            diff_result = list(
                difflib.context_diff(old_lines, gen_lines, f"saved {current_filename}", f"generated {current_filename}")
            )
            return CompareResult(len(diff_result) == 0, str.join("", diff_result))


class CodeGenerator:
    """Base class for generating bindings."""

    def __init__(
        self,
        matches,
        hierarchy,
        out_filename,
        *,
        skip_write_if_same,
        fail_on_write,
        reporter: DiagnosticReporter,
        format_module=None,
        repo_root=None,
    ):
        self._matches = matches
        self._interfaces = matches.interfaces
        self._enums = matches.enums
        self._flags = matches.flags
        self._constant_groups = matches._constant_groups
        self._hierarchy = hierarchy
        self._skip_write_if_same = skip_write_if_same
        self._fail_on_write = fail_on_write
        self._reporter = reporter
        self._format_module = format_module
        self._repo_root = repo_root

        try:
            self._copyright_year = date.today().year
            with open(out_filename, "r") as f:
                first = f.readline()
                if "copyright" in first.lower():
                    m = re.match(r".*?(\d{4})(?:\w?-\w?(\d{4}))?.*?", first)
                    if m:
                        existing_year = int(m.groups()[0])
                        if self._copyright_year != existing_year:
                            self._copyright_year = f"{existing_year}-{self._copyright_year}"
        except:
            pass

        if "-" == out_filename:
            self._out_filename = None
            self._tmp_filename = None
            self._out_file = sys.stdout
        else:
            self._out_filename = out_filename
            self._tmp_filename = f"{out_filename}.~nv"

            make_dirs(self._tmp_filename, self.reporter)
            self._out_file = open(self._tmp_filename, "w")

    @classmethod
    def generate(
        cls,
        matches,
        hierarchy,
        out_file,
        *,
        reporter: DiagnosticReporter,
        skip_write_if_same=True,
        fail_on_write=False,
        format_module=None,
        repo_root=None,
    ):
        cls(
            matches,
            hierarchy,
            out_file,
            reporter=reporter,
            skip_write_if_same=skip_write_if_same,
            fail_on_write=fail_on_write,
            format_module=format_module,
            repo_root=repo_root,
        )

    def _write(self, msg):
        self._out_file.write(msg)

    def _compare_files(self) -> CompareResult:
        if not os.path.exists(self._out_filename):
            return CompareResult(False, "Previous file did not exist. Was it not checked in?")
        if self._out_filename is None or self._tmp_filename is None:
            raise RuntimeError("Attempt to compare files, but output is to stdout")
        return compare_files(self._out_filename, self._tmp_filename)

    def _commit(self):
        from pathlib import Path

        if not self._tmp_filename:
            return

        self._out_file.write("\n")
        self._out_file.close()

        if self._format_module:
            spec = importlib.util.spec_from_file_location("format.module", self._format_module)
            assert spec, "Failed to import ModuleSpec, but no error was raised"
            format_module = importlib.util.module_from_spec(spec)
            assert spec.loader
            spec.loader.exec_module(format_module)
            format_module.format_cpp(self._tmp_filename, self._out_filename, self._repo_root)

        compare = lazy(self._compare_files)
        if self._skip_write_if_same and os.path.exists(self._out_filename) and compare().are_same:
            # if we got this far, a dependency has changed but the generated output (ignoring the copyright line) is the
            # same.
            #
            # we need to update the timestamp on the output file to avoid rebuilding this file on the next build.
            #
            # we could write the file, but doing so would write the possibly new copyright date, which we don't want
            # unless the code actually changed.
            #
            # in order to update the dependency info and avoid writing an unwanted updated copyright, we skip the file
            # write and simply touch the file.
            Path(self._out_filename).touch()
            os.remove(self._tmp_filename)
            self.reporter.verbose(None, "skipped writing: {}", self._out_filename)
        else:
            self.reporter.note(None, "Generated: {}", self._out_filename)

            if self._fail_on_write:
                error_message = (
                    f"    A code change was detected in the generated file '{self._out_filename}' but\n"
                    "    the '--fail-on-write' option was used.  This likely means that code changes\n"
                    "    occurred during a CI build step because a full local build was not performed\n"
                    "    before creating an MR.  This can occur when the Carbonite SDK or repoman versions\n"
                    "    are updated and a tool change caused some code to be regenerated.\n"
                    "\n"
                    "    To remedy this, please run a full local build after updating the Carbonite SDK,\n"
                    "    then commit all modified .gen.h files to your MR.  This will happen from time to\n"
                    "    time when changes are made to the tools or code generation bugs need to be fixed.\n"
                    "    Note that a change only to a copyright year at the top of the file won't be considered\n"
                    "    a code change that will fail this operation.\n"
                ) + compare().differences
                os.remove(self._tmp_filename)
                self.reporter.error(None, error_message)

            shutil.move(self._tmp_filename, self._out_filename)

    @property
    def reporter(self) -> DiagnosticReporter:
        return self._reporter


def make_dirs(filename, reporter: DiagnosticReporter):
    import pathlib

    path = pathlib.Path(filename)
    if not path.parent.exists():
        reporter.verbose(None, "making: {}", path.parent)
        os.makedirs(str(path.parent))


def write_includes(out_filename, translation_unit, reporter: DiagnosticReporter, extra_deps=[]):
    from pathlib import Path

    from omni.bind import _get_dependencies

    includes = {}
    for inc in translation_unit.get_includes():
        path = Path(str(inc.include))
        path = path.resolve()
        if path.exists() and (not str(path).endswith(".gen.h")):
            includes[str(path)] = True

    deps = _get_dependencies()
    for dep in deps:
        includes[dep] = True
    if 0 == len(deps):
        raise Exception("omni.bind dependency search failed.  did omni.bind.py move?")

    make_dirs(out_filename, reporter)
    with open(out_filename, "w") as f:
        f.write(f"{translation_unit.spelling}\n")
        for key, _ in includes.items():
            f.write(f"{key}\n")


def is_output_up_to_date(outputs, includes_filename, reporter: DiagnosticReporter) -> bool:
    if not os.path.exists(includes_filename):
        reporter.warning(None, "out-of-date: {} does not exist!", includes_filename)
        return False

    output_timestamps = []
    for out in outputs:
        if not os.path.exists(out):
            reporter.note(None, "out-of-date: output {} does not exist", out)
            return False
        output_timestamps.append((out, os.path.getmtime(out)))

    includes = []
    with open(includes_filename, "r") as f:
        for _, ln in enumerate(f):
            ln = ln.strip()
            if len(ln) > 0:
                includes.append(ln)

    for inc in includes:
        if not os.path.exists(inc):
            reporter.warning(None, "out-of-date: input {} does not exist", inc)
            return False

        included_time = os.path.getmtime(inc)
        for out in output_timestamps:
            output_time = out[1]
            reporter.verbose(None, "checking {} with {}", inc, out)
            if included_time > output_time:
                reporter.note(None, "out-of-date: input {} is newer than {}", inc, out[0])
                return False

    return True
