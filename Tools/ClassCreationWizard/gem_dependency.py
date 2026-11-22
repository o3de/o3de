#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

"""
Shared logic for commands that add a dependency to a CMake target's BUILD_DEPENDENCIES.

Used by add_gem_dependency (edits an explicitly chosen target) and add_editor_gem_dependency (edits
the editor counterpart of whatever target the wizard selected). Lives beside cmake_text.py, NOT in
commands/, because the command-plugin loader execs every file in commands/ standalone, expecting a
@CommandRegistry.register() call in each one.
"""

import re
from typing import List, Optional

from cmake_text import CMakeArg, CMakeCall, call_arguments, find_calls, insert_lines_after, line_indent, starts_line
from command_plugin import CommandContext, CMakeTarget


SCOPE_KEYWORDS = ("PRIVATE", "PUBLIC", "INTERFACE")
INDENT = "    "


# ============================================================================
# Runtime -> Editor target name mapping
# ============================================================================

def editor_counterpart_name(base_name: str, namespace: str) -> Optional[str]:
    """Map a runtime target name to its editor counterpart, following O3DE's gem convention:
    the bare gem target gets ".Editor" appended, and every suffixed target gets ".Editor" inserted
    right after the gem name -- <Gem> -> <Gem>.Editor, <Gem>.API -> <Gem>.Editor.API,
    <Gem>.Private.Object -> <Gem>.Editor.Private.Object, <Gem>.Tests -> <Gem>.Editor.Tests.

    Returns None if `base_name` is already an editor target, or isn't in this namespace at all --
    the caller should treat that as "no editor counterpart to target", not an error.
    """
    if base_name == namespace:
        return f"{namespace}.Editor"

    prefix = f"{namespace}."
    if not base_name.startswith(prefix):
        return None

    rest = base_name[len(prefix):]
    if rest == "Editor" or rest.startswith("Editor."):
        return None  # base_name is already an editor target

    return f"{namespace}.Editor.{rest}"


# ============================================================================
# Finding a target's call
# ============================================================================

def find_target_call(text: str, target: CMakeTarget) -> Optional[CMakeCall]:
    """Find the o3de_add_target/ly_add_target call that declares `target` in `text`.

    Matches by the exact NAME string in the file, e.g. "${gem_name}.API" -- valid because `target`
    is always produced by scanning this same file (CMakeAnalyzer.scan_targets), so its raw_name is
    that literal string, verbatim.
    """
    for call in find_calls(text, 'o3de_add_target', 'ly_add_target'):
        name_match = re.search(r'\bNAME\s+([^\s\)]+)', call.code_body)
        if name_match and name_match.group(1).strip('"\'') == target.raw_name:
            return call
    return None


# ============================================================================
# Inserting a dependency into a target's BUILD_DEPENDENCIES
# ============================================================================

def insert_dependency(text: str, call: CMakeCall, dependency: str) -> Optional[str]:
    """Insert `dependency` into the call's BUILD_DEPENDENCIES, in the PRIVATE list -- or, for a
    CMake INTERFACE-kind target (`ly_add_target(NAME ... INTERFACE ...)`), the INTERFACE list,
    since INTERFACE targets cannot take a PRIVATE dependency at all.

    Appends to an existing list of that scope, adds one if BUILD_DEPENDENCIES exists without one,
    or adds the whole BUILD_DEPENDENCIES block if missing entirely. Returns the new text, or None
    if `dependency` is already present anywhere in the call (nothing to do).
    """
    args = call_arguments(call)
    if dependency in (arg.text.strip('"') for arg in args):
        return None

    scope = "INTERFACE" if _target_kind(args) == "INTERFACE" else "PRIVATE"

    deps_i = next((i for i, arg in enumerate(args) if arg.text == 'BUILD_DEPENDENCIES'), None)
    if deps_i is None:
        return _add_build_dependencies_block(text, call, args, scope, dependency)

    section_end = _section_end(args, deps_i)
    scope_i = _last_scope(args, deps_i, section_end, scope)
    if scope_i is None:
        return _add_scope_list(text, call, args, deps_i, section_end, scope, dependency)

    return _append_to_scope_list(text, call, args, scope_i, section_end, dependency)


def _append_to_scope_list(text: str, call: CMakeCall, args: List[CMakeArg],
                          scope_i: int, section_end: int, dependency: str) -> str:
    """Add the dependency after the last entry of an existing scope list (PRIVATE/PUBLIC/INTERFACE)."""
    scope_arg = args[scope_i]
    last = args[_scope_end(args, scope_i, section_end) - 1]

    if last is not scope_arg and starts_line(text, last.start):
        indent = line_indent(text, last.start)
    else:
        indent = line_indent(text, scope_arg.start) + INDENT

    return insert_lines_after(text, last.end, [f'{indent}{dependency}'], call.body_end)


def _add_scope_list(text: str, call: CMakeCall, args: List[CMakeArg],
                    deps_i: int, section_end: int, scope: str, dependency: str) -> str:
    """Add a new scope list (holding the dependency) at the end of BUILD_DEPENDENCIES."""
    last = args[section_end - 1]  # BUILD_DEPENDENCIES itself when the section is empty
    first_scope = next((arg for arg in args[deps_i + 1:section_end] if arg.text in SCOPE_KEYWORDS), None)

    if first_scope and starts_line(text, first_scope.start):
        scope_indent = line_indent(text, first_scope.start)
    else:
        scope_indent = line_indent(text, args[deps_i].start) + INDENT

    lines = [f'{scope_indent}{scope}', f'{scope_indent}{INDENT}{dependency}']
    return insert_lines_after(text, last.end, lines, call.body_end)


def _add_build_dependencies_block(text: str, call: CMakeCall, args: List[CMakeArg],
                                  scope: str, dependency: str) -> str:
    """Add a whole BUILD_DEPENDENCIES section (holding the dependency) at the end of the target."""
    first = args[0] if args else None
    base = line_indent(text, first.start) if first and starts_line(text, first.start) else INDENT
    last_end = args[-1].end if args else call.body_start

    lines = ['', f'{base}BUILD_DEPENDENCIES', f'{base}{INDENT}{scope}', f'{base}{INDENT}{INDENT}{dependency}']
    return insert_lines_after(text, last_end, lines, call.body_end)


# ============================================================================
# Reading, editing and writing back the target's CMake file
# ============================================================================

def apply_to_target(ctx: CommandContext, target: CMakeTarget, dependency: str) -> bool:
    """Add `dependency` to `target`'s BUILD_DEPENDENCIES, logging through ctx and writing the file.

    Shared by add_gem_dependency and add_editor_gem_dependency once each has resolved its target.
    Always returns True (registration commands never hard-fail the wizard run); ctx.log carries the
    warning when the target's CMake file or its call can't be found.
    """
    cmake_path = target.file
    if not cmake_path.is_file():
        ctx.log(f"Warning: CMake file not found: {cmake_path}")
        return True

    text = cmake_path.read_text(encoding="utf-8")
    call = find_target_call(text, target)
    if call is None:
        ctx.log(f"Warning: Could not find target block for {target.name}")
        return True

    new_text = insert_dependency(text, call, dependency)
    if new_text is None:
        ctx.log(f"Dependency already present: {dependency}")
        return True

    cmake_path.write_text(new_text, encoding='utf-8', newline='\n')
    ctx.log(f"Added dependency {dependency} to {target.name}")
    return True


# ============================================================================
# Argument scanning
# ============================================================================

def _is_keyword(arg: CMakeArg) -> bool:
    """Keywords are ALL_CAPS words (NAME, BUILD_DEPENDENCIES, PRIVATE...). Targets like AZ::AzCore are not."""
    return re.fullmatch(r'[A-Z][A-Z0-9_]*', arg.text) is not None


def _target_kind(args: List[CMakeArg]) -> Optional[str]:
    """Return the target type keyword right after NAME's value (STATIC, MODULE, SHARED, INTERFACE,
    EXECUTABLE, GEM_MODULE, ...), or None if the call has no NAME (malformed) or nothing follows it.
    """
    name_i = next((i for i, arg in enumerate(args) if arg.text == 'NAME'), None)
    if name_i is None or name_i + 2 >= len(args):
        return None
    kind = args[name_i + 2]
    return kind.text if _is_keyword(kind) else None


def _section_end(args: List[CMakeArg], section_i: int) -> int:
    """Return the index just past a section's arguments.

    A section runs until the next keyword that is not a PRIVATE/PUBLIC/INTERFACE scope.
    """
    i = section_i + 1
    while i < len(args) and not (_is_keyword(args[i]) and args[i].text not in SCOPE_KEYWORDS):
        i += 1
    return i


def _scope_end(args: List[CMakeArg], scope_i: int, section_end: int) -> int:
    """Return the index just past a scope list's entries (it ends at the next scope keyword)."""
    i = scope_i + 1
    while i < section_end and args[i].text not in SCOPE_KEYWORDS:
        i += 1
    return i


def _last_scope(args: List[CMakeArg], deps_i: int, section_end: int, scope: str) -> Optional[int]:
    """Return the index of the last `scope` keyword inside the BUILD_DEPENDENCIES section, if any."""
    indexes = [i for i in range(deps_i + 1, section_end) if args[i].text == scope]
    return indexes[-1] if indexes else None
