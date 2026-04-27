#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

"""
add_pass_creator_call
=====================

Wires a custom Atom RPI pass class into a gem's RenderingSystemComponent so
that Atom's PassSystem knows how to instantiate it from a .pass template.

Generated effect (idempotent)
-----------------------------
  Header:  #include "${pass_name}.h"
  Activate():
      AZ::RPI::PassSystemInterface::Get()->AddPassCreator(
          AZ::Name("${pass_name}"), &${pass_name}::Create);
  Deactivate():
      AZ::RPI::PassSystemInterface::Get()->RemovePassCreator(
          AZ::Name("${pass_name}"));

Why this command exists
-----------------------
The DataAsset template uses register_generic_asset to inject GenericAssetHandler
boilerplate into the DataAssetSystemComponent. The fullscreen post process
template needs the same trick for a different API surface (PassSystemInterface
instead of AssetManager). Doing this with replace_text would require a fragile
text marker; doing it as a dedicated command lets us:

  1. Detect already-present registrations (re-running the wizard with the same
     pass name is a no-op rather than producing duplicate AddPassCreator lines).
  2. Compute the existing indent inside Activate()/Deactivate() so injected
     lines match the surrounding code style.
  3. Skip cleanly if the user has hand-edited the system component to use a
     non-default Activate() shape -- we log and bail rather than corrupting it.

Args (template.json process_commands entry)
-------------------------------------------
  pass_name             : str, required
        The C++ class name of the pass (e.g. "MyPostProcessPass"). Becomes
        both the AZ::Name token and the &${pass_name}::Create reference.
  system_component_name : str, optional
        Override for the system component class name. Defaults to
        "${GemName}RenderingSystemComponent".

File location search order
--------------------------
Mirrors register_generic_asset:
  <gem>/Code/Source/${SCN}.cpp
  <gem>/Source/${SCN}.cpp
"""

import re
from pathlib import Path
from typing import Optional, Tuple

from command_plugin import WizardCommand, CommandContext, CommandRegistry


@CommandRegistry.register("add_pass_creator_call")
class AddPassCreatorCallCommand(WizardCommand):
    """Inject AddPassCreator/RemovePassCreator pairs into a RenderingSystemComponent."""

    def __init__(self, pass_name: str, system_component_name: str = ""):
        self.pass_name = pass_name
        self.system_component_name = system_component_name

    @property
    def name(self) -> str:
        return "add_pass_creator_call"

    @property
    def description(self) -> str:
        return "Register a custom RPI pass with PassSystemInterface from a gem's RenderingSystemComponent"

    @property
    def author(self) -> str:
        return "O3DE"

    # ------------------------------------------------------------------
    # Main entry
    # ------------------------------------------------------------------

    def execute(self, ctx: CommandContext) -> bool:
        if not self.pass_name:
            ctx.log("add_pass_creator_call: pass_name is required, skipping")
            return True

        sys_comp_name = self.system_component_name or f"{ctx.namespace}RenderingSystemComponent"
        cpp_path = self._find_system_component(ctx, sys_comp_name)

        if cpp_path is None:
            ctx.log(
                f"add_pass_creator_call: {sys_comp_name}.cpp not found. "
                "Either copy_template_files did not run, or the template's "
                "RenderingSystemComponent files were not included. Skipping."
            )
            return True

        ctx.log(f"add_pass_creator_call: registering '{self.pass_name}' in {sys_comp_name}.cpp")

        text = cpp_path.read_text(encoding="utf-8")
        original = text

        text = self._ensure_include(text, f'{self.pass_name}.h')
        text = self._ensure_pass_system_include(text)
        text = self._inject_call(
            text,
            method=f"{sys_comp_name}::Activate",
            sentinel=f'AddPassCreator(AZ::Name("{self.pass_name}")',
            line=(f'AZ::RPI::PassSystemInterface::Get()->AddPassCreator('
                  f'AZ::Name("{self.pass_name}"), &{self.pass_name}::Create);'),
            ctx=ctx,
        )
        text = self._inject_call(
            text,
            method=f"{sys_comp_name}::Deactivate",
            sentinel=f'RemovePassCreator(AZ::Name("{self.pass_name}")',
            line=(f'AZ::RPI::PassSystemInterface::Get()->RemovePassCreator('
                  f'AZ::Name("{self.pass_name}"));'),
            ctx=ctx,
        )

        if text == original:
            ctx.log("add_pass_creator_call: no changes needed (already registered).")
            return True

        # newline="\n" on open() works on all supported Pythons;
        # Path.write_text(newline=...) is 3.10+.
        with open(cpp_path, "w", encoding="utf-8", newline="\n") as fh:
            fh.write(text)
        ctx.log(f"add_pass_creator_call: updated {cpp_path}")
        return True

    # ------------------------------------------------------------------
    # File discovery
    # ------------------------------------------------------------------

    @staticmethod
    def _find_system_component(ctx: CommandContext, sys_comp_name: str) -> Optional[Path]:
        candidates = [
            ctx.dest_root / "Code" / "Source" / f"{sys_comp_name}.cpp",
            ctx.dest_root / "Source" / f"{sys_comp_name}.cpp",
            ctx.dest_root / "Code" / "Source" / "Tools" / f"{sys_comp_name}.cpp",
            ctx.dest_root / "Source" / "Tools" / f"{sys_comp_name}.cpp",
        ]
        for c in candidates:
            if c.is_file():
                return c
        return None

    # ------------------------------------------------------------------
    # Text edits
    # ------------------------------------------------------------------

    @staticmethod
    def _ensure_include(text: str, header: str) -> str:
        """Append #include "header" after the last existing #include block."""
        line = f'#include "{header}"'
        if line in text:
            return text

        lines = text.splitlines()
        last_include = -1
        for i, ln in enumerate(lines):
            if ln.lstrip().startswith("#include"):
                last_include = i

        if last_include == -1:
            return line + "\n" + text

        lines.insert(last_include + 1, line)
        return "\n".join(lines) + ("\n" if text.endswith("\n") else "")

    @staticmethod
    def _ensure_pass_system_include(text: str) -> str:
        """Add the angle-bracket Atom include needed by PassSystemInterface calls."""
        line = "#include <Atom/RPI.Public/Pass/PassSystemInterface.h>"
        if line in text:
            return text

        lines = text.splitlines()
        last_angle = -1
        for i, ln in enumerate(lines):
            stripped = ln.lstrip()
            if stripped.startswith("#include <"):
                last_angle = i

        insert_at = (last_angle + 1) if last_angle >= 0 else 0
        lines.insert(insert_at, line)
        return "\n".join(lines) + ("\n" if text.endswith("\n") else "")

    def _inject_call(self, text: str, method: str, sentinel: str,
                     line: str, ctx: CommandContext) -> str:
        """Insert `line` immediately before the closing brace of `method`'s body.

        Indent strategy:
          * Prefer the indent of an existing non-blank line inside the body.
          * Fall back to the closing brace's indent + 4 spaces.
        The closing brace stays on its own line at its original indent.
        A blank separator line is inserted ahead of the new line if and only if
        the method body already contained code, so multiple calls on the same
        method stack cleanly with one-line gaps.
        """
        if sentinel in text:
            ctx.log(f"  '{method}' already contains '{sentinel}', skipping")
            return text

        body = self._find_method_body(text, method)
        if body is None:
            ctx.log(f"  Warning: could not locate '{method}' body, skipping injection")
            return text

        body_start, body_end = body  # body_end is the offset of the closing '}'

        # The closing brace's line and its indent.
        close_line_start = text.rfind('\n', 0, body_end) + 1
        close_indent = ''
        i = close_line_start
        while i < body_end and text[i] in ' \t':
            close_indent += text[i]
            i += 1

        # Determine the indent for inserted code: copy the first non-blank inner
        # line's indent if there is one, otherwise indent one stop deeper than
        # the closing brace.
        inner = text[body_start:close_line_start]
        body_indent = None
        for inner_line in inner.splitlines():
            if inner_line.strip():
                lead = re.match(r'^[ \t]+', inner_line)
                if lead:
                    body_indent = lead.group(0)
                break
        indent = body_indent or (close_indent + '    ')

        had_content = bool(inner.strip())
        sep = "\n" if had_content else ""

        injected = f"{sep}{indent}{line}\n"
        return text[:close_line_start] + injected + text[close_line_start:]

    @staticmethod
    def _find_method_body(text: str, method: str) -> Optional[Tuple[int, int]]:
        """Return (start, end) offsets of the inside of `method`'s body braces.

        Handles the standard form:
            void Foo::Bar(...)
            {
                ...
            }
        Returns offsets of the region between the matching braces (exclusive of
        the braces themselves), or None if not found.
        """
        sig_pattern = re.compile(re.escape(method) + r'\s*\([^)]*\)\s*\{')
        m = sig_pattern.search(text)
        if not m:
            return None

        depth = 1
        i = m.end()
        body_start = i
        while i < len(text) and depth > 0:
            c = text[i]
            if c == '{':
                depth += 1
            elif c == '}':
                depth -= 1
                if depth == 0:
                    return body_start, i
            i += 1
        return None
