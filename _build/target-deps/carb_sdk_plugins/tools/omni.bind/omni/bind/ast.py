__copyright__ = "Copyright (c) 2020-2022, NVIDIA CORPORATION. All rights reserved."
__license__ = """
NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
"""

from pathlib import Path

import clang.cindex

from .diagnostics import DiagnosticReporter


def traverse(node: clang.cindex.Cursor, reporter: DiagnosticReporter, space: str = "", dive: bool = False) -> None:
    if node.location.file and (Path(node.location.file.name) == Path("include/omni/core/TypeId.h")):
        return

    text = node.spelling or node.displayname
    text = node.displayname or node.spelling
    try:
        kind = str(node.kind)[str(node.kind).index(".") + 1 :]
    except:
        kind = "unknown"

    print(
        "{}{} {} template:{} usr:{} ref:{} pointee:{} type:{}".format(
            space,
            kind,
            text,
            clang.cindex.conf.lib.clang_getSpecializedCursorTemplate(node),
            node.get_usr(),
            node.type.get_ref_qualifier(),
            node.type.get_pointee().spelling,
            node.type.kind,
        )
    )
    if clang.cindex.CursorKind.TYPE_REF == node.kind:
        ref = node.referenced
        if node != ref:
            print(
                "{} ref:{} {} template:{} usr:{} ref:{} pointee:{} type:{}".format(
                    space,
                    ref.kind,
                    ref.displayname,
                    clang.cindex.conf.lib.clang_getSpecializedCursorTemplate(ref),
                    ref.get_usr(),
                    ref.type.get_ref_qualifier(),
                    ref.type.get_pointee().spelling,
                    ref.type.kind,
                )
            )

    for child in node.get_children():
        try:
            traverse(child, reporter, space + "  ", dive)
        except (ValueError, AttributeError):
            reporter.error(None, f"{kind} {text}")


def print_ast(tu: clang.cindex.TranslationUnit, reporter: DiagnosticReporter) -> None:
    traverse(tu.cursor, reporter)
