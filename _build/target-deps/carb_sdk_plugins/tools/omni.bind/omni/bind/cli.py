__copyright__ = "Copyright (c) 2020-2022, NVIDIA CORPORATION. All rights reserved."
__license__ = """
NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
"""

import argparse
import os
import pathlib
import sys

import clang.cindex

from . import ast, extract
from .diagnostics import DiagnosticException, DiagnosticLevel, DiagnosticMessage, TextDiagnosticReporter
from .generate import cpp, is_output_up_to_date, write_includes


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("input", help="Input interface header containing the ABI definition")
    parser.add_argument("--api", type=str, help="Output header for generated API wrapper")
    parser.add_argument("--py", type=str, help="Output header for generated Python bindings")
    parser.add_argument("--clang", type=str, help="Location of clang .dll/.so")
    parser.add_argument("-v", "--verbose", action="store_true", help="Write diagnostic data to stdout")
    parser.add_argument("--debug", action="store_true", help="Write full stack trace on error")
    parser.add_argument("-I", action="append", type=str, default=[], help="Include directory")
    parser.add_argument("--ast", action="store_true", help="Print AST and Exit")
    parser.add_argument("-M", type=str, help="Write #include'd files to given file")
    parser.add_argument(
        "--skip-write-if-same", action="store_true", help="Don't write the output if it's the same as the file on disk."
    )
    parser.add_argument("--isystem", action="append", type=str, default=[], help="clang system path")
    parser.add_argument("--format-module", type=str, help="Python script to use to format files after generation.")
    parser.add_argument("--repo-root", type=str, help="The root of the repo, where repo.toml can be found.")
    parser.add_argument("--std", type=str, default="c++14", help="C++ language standard.")
    parser.add_argument(
        "--fail-on-write", action="store_true", help="Fail the operation if a write to a file needs to occur."
    )
    args = parser.parse_args()

    try:
        in_filename = args.input
        api_filename = args.api
        py_filename = args.py

        log_level = DiagnosticLevel.NOTE if not args.verbose else DiagnosticLevel.VERBOSE
        reporter = TextDiagnosticReporter(diagnostic_log_level=log_level)

        generated_files = []
        if api_filename:
            generated_files.append(api_filename)
        if py_filename:
            generated_files.append(py_filename)
        if args.M:
            generated_files.append(args.M)

        if (not args.ast) and args.M and is_output_up_to_date(generated_files, args.M, reporter):
            # nothing to do
            print(f"{in_filename}: All outputs up-to-date")
            return 0

        clang_dll = args.clang
        if not args.clang:
            if os.name == "nt":
                clang_dll = "c:/Program Files/LLVM/bin/libclang.dll"
            else:
                # clang_dll = '/usr/lib/llvm-6.0/lib/libclang.so'
                clang_dll = "/usr/lib/x86_64-linux-gnu/libclang-6.0.so.1"

        clang_args = [
            "-x",
            "c++",
            "-std=" + args.std,
            "-DOMNI_BIND",  # enable OMNI_ATTR macro
            "-Wno-ignored-pragma-intrinsic",  # ignore "'_addcarry_u64' is not a recognized builtin"
            "-Wno-pragma-once-outside-header",  # reflection tool does this?
        ]
        for inc in args.isystem:
            clang_args.append("-isystem")
            clang_args.append(str(pathlib.Path(inc)))
        for inc in args.I:
            clang_args.append(f"-I{str(pathlib.Path(inc))}")

        if args.verbose:
            clang_args.append("-v")

        # make sure the api output exists. we do this because it's likely the input .h file is already trying to include
        # it.  if the file doesn't exist, index.parse() will fail, so here we create an empty file.
        if api_filename and (not pathlib.Path(api_filename).is_file()):
            with open(api_filename, "w") as f:
                pass

        clang.cindex.Config.set_library_file(clang_dll)
        index = clang.cindex.Index.create()
        translation_unit = index.parse(
            in_filename,
            clang_args,
            options=clang.cindex.TranslationUnit.PARSE_INCOMPLETE
            | clang.cindex.TranslationUnit.PARSE_SKIP_FUNCTION_BODIES,
        )  # faster?

        for d in translation_unit.diagnostics:
            reporter.emit_message(DiagnosticMessage.from_clang(d))

        if args.ast:
            with reporter:
                ast.print_ast(translation_unit, reporter)
                return 0

        # generate the class hierarchy
        hierarchy = extract.Hierarchy(index, translation_unit, reporter)

        # traverse the entire ast looking for interfaces in the specified file
        matches = extract.InterfaceMatcher(translation_unit.cursor, hierarchy, in_filename, reporter)

        generated_files = []
        if api_filename:
            cpp.CppGenerator.generate(
                matches,
                hierarchy,
                api_filename,
                reporter=reporter,
                skip_write_if_same=args.skip_write_if_same,
                fail_on_write=args.fail_on_write,
                format_module=args.format_module,
                repo_root=args.repo_root,
            )
        if py_filename:
            cpp.PyGenerator.generate(
                matches,
                hierarchy,
                py_filename,
                reporter=reporter,
                skip_write_if_same=args.skip_write_if_same,
                fail_on_write=args.fail_on_write,
                format_module=args.format_module,
                repo_root=args.repo_root,
            )

        if args.M and len(args.M) > 0:
            write_includes(args.M, translation_unit, reporter)

        return 0 if reporter.valid else 1
    except clang.cindex.TranslationUnitLoadError as e:
        reporter.fatal(None, "{}", e)
        reporter.fatal(None, "Check if the input file, '{}' exists and is readable.", in_filename)
        return 1
    except DiagnosticException as e:
        if args.debug:
            raise
        else:
            # DiagnosticExceptions have already been reported, so just return 1
            return 1
    except Exception as e:
        if args.debug:
            raise
        else:
            reporter.fatal(None, "{}", e)
            return 1
