#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

"""
CMake text helpers for the Class Wizard commands.

Wizard commands edit CMake files as plain text. Finding the end of a call such as set(FILES ...) or
ly_add_target(...) with a regex like "(.*?)\\)" stops at the FIRST ')', which is wrong whenever a
comment, a quoted string, or a nested call inside the block contains one. These helpers match
parentheses by depth and skip comments and strings, so they find the ')' that actually closes the
call, wherever that call sits in the file.
"""

import re
from typing import List, NamedTuple, Optional


# ============================================================================
# Types
# ============================================================================

class CMakeCall(NamedTuple):
    """Location of one CMake command call. All indices refer to the ORIGINAL text."""
    start: int        # index of the command name
    body_start: int   # index just after the opening '('
    body_end: int     # index of the matching ')' -- insert new entries here
    code_body: str    # text between the parens with comments blanked out; safe to regex against


class CMakeArg(NamedTuple):
    """One argument of a CMake call. Indices refer to the ORIGINAL text."""
    text: str
    start: int
    end: int


# ============================================================================
# Scanning primitives
# ============================================================================

_BRACKET_COMMENT_OPEN = re.compile(r'#\[(=*)\[')

# A quoted string, a lone paren, or a run of anything else that is not whitespace.
_ARGUMENT = re.compile(r'"(?:[^"\\]|\\.)*"|[()]|(?:[^\s"()\\]|\\.)+')


def _skip_string(text: str, i: int) -> int:
    """Return the index just past the quoted string that starts at text[i] (a double quote)."""
    i += 1
    while i < len(text) and text[i] != '"':
        i += 2 if text[i] == '\\' else 1
    return min(i + 1, len(text))


def _comment_end(text: str, i: int) -> int:
    """Return the index just past the comment that starts at text[i] (a '#')."""
    bracket = _BRACKET_COMMENT_OPEN.match(text, i)
    if bracket:
        closer = ']' + bracket.group(1) + ']'
        end = text.find(closer, bracket.end())
        return len(text) if end < 0 else end + len(closer)

    end = text.find('\n', i)
    return len(text) if end < 0 else end


def blank_comments(text: str) -> str:
    """Return text with every comment replaced by spaces (newlines kept), so all indices stay valid."""
    out = []
    i = 0
    while i < len(text):
        c = text[i]
        if c == '#':
            end = _comment_end(text, i)
            out.append(re.sub(r'[^\n]', ' ', text[i:end]))
            i = end
        elif c == '"':
            end = _skip_string(text, i)
            out.append(text[i:end])
            i = end
        elif c == '\\':
            out.append(text[i:i + 2])
            i += 2
        else:
            out.append(c)
            i += 1
    return ''.join(out)


def _find_closing_paren(code: str, open_idx: int) -> int:
    """Return the index of the ')' matching the '(' at code[open_idx], or -1 if it never closes.

    `code` must already have its comments blanked. Quoted strings and backslash escapes are skipped.
    """
    depth = 0
    i = open_idx
    while i < len(code):
        c = code[i]
        if c == '"':
            i = _skip_string(code, i)
            continue
        if c == '\\':
            i += 2
            continue
        if c == '(':
            depth += 1
        elif c == ')':
            depth -= 1
            if depth == 0:
                return i
        i += 1
    return -1


# ============================================================================
# Public API
# ============================================================================

def find_calls(text: str, *command_names: str) -> List[CMakeCall]:
    """Find every call to the given CMake commands, e.g. find_calls(text, 'set').

    Commented-out calls are ignored, and a ')' inside a comment or string never ends a call.
    Command names are case-insensitive, as in CMake. A call that never closes is skipped.
    """
    code = blank_comments(text)
    names = '|'.join(re.escape(name) for name in command_names)
    call_pattern = re.compile(rf'\b(?:{names})\s*\(', re.IGNORECASE)

    calls = []
    pos = 0
    while True:
        match = call_pattern.search(code, pos)
        if not match:
            return calls

        open_idx = match.end() - 1
        close_idx = _find_closing_paren(code, open_idx)
        if close_idx < 0:
            pos = match.end()
            continue

        calls.append(CMakeCall(match.start(), open_idx + 1, close_idx, code[open_idx + 1:close_idx]))
        pos = close_idx + 1


def find_files_block_end(text: str) -> Optional[int]:
    """Return the index of the ')' that closes the set(FILES ...) block, or None if there is none."""
    for call in find_calls(text, 'set'):
        if re.match(r'\s*FILES\b', call.code_body):
            return call.body_end
    return None


def call_arguments(call: CMakeCall) -> List[CMakeArg]:
    """Split a call's body into its arguments. Comments are already ignored."""
    return [CMakeArg(match.group(), call.body_start + match.start(), call.body_start + match.end())
            for match in _ARGUMENT.finditer(call.code_body)]


# ============================================================================
# Line helpers (for inserting whole lines with matching indentation)
# ============================================================================

def line_indent(text: str, index: int) -> str:
    """Return the leading whitespace of the line containing `index`."""
    line_start = text.rfind('\n', 0, index) + 1
    return re.match(r'[ \t]*', text[line_start:]).group()


def starts_line(text: str, index: int) -> bool:
    """Return True if only whitespace sits between the start of the line and `index`."""
    line_start = text.rfind('\n', 0, index) + 1
    return not text[line_start:index].strip()


def insert_lines_after(text: str, index: int, lines: List[str], closing_paren: int) -> str:
    """Insert whole lines after the line containing `index`.

    If the call's closing paren shares that line, the lines go in front of the paren instead, so
    nothing is ever inserted after the ')'.
    """
    line_end = text.find('\n', index)
    if line_end < 0:
        line_end = len(text)

    new_lines = '\n'.join(lines)
    if closing_paren <= line_end:
        return text[:closing_paren] + '\n' + new_lines + '\n' + text[closing_paren:]
    return text[:line_end] + '\n' + new_lines + text[line_end:]
