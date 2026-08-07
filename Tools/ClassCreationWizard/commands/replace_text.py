#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

from pathlib import Path
from typing import Optional

from command_plugin import WizardCommand, CommandContext, CommandRegistry


@CommandRegistry.register("replace_text")
class ReplaceTextCommand(WizardCommand):
    """Replace text in a generated file. Supports literal replacement or replacement from an input variable."""

    def __init__(self, component_name: str, text_to_replace: str,
                 replacement: str = "", replacement_var: str = ""):
        self.component_name = component_name
        self.text_to_replace = text_to_replace
        self.replacement = replacement
        self.replacement_var = replacement_var

    @property
    def name(self) -> str:
        return "replace_text"

    @property
    def description(self) -> str:
        return "Find and replace text in a generated file"

    def execute(self, ctx: CommandContext) -> bool:
        # Determine replacement value
        if self.replacement_var:
            repl_value = ctx.variables.get(self.replacement_var, "")
            if repl_value is None:
                repl_value = ""
        else:
            repl_value = self.replacement

        # Coerce to string; booleans become JSON-compatible lowercase (true/false)
        if isinstance(repl_value, bool):
            repl_value = "true" if repl_value else "false"
        elif not isinstance(repl_value, str):
            repl_value = str(repl_value)

        # Locate the target file in dest_root
        target = self._find_file(ctx.dest_root, self.component_name)
        if not target:
            ctx.log(f"Warning: File not found for replace_text: {self.component_name}")
            return True

        text = target.read_text(encoding="utf-8")
        if self.text_to_replace not in text:
            ctx.log(f"Warning: Text not found in {target.name}: {self.text_to_replace!r}")
            return True

        text = text.replace(self.text_to_replace, repl_value)
        target.write_text(text, encoding="utf-8")
        ctx.log(f"Replaced text in {target.name}")
        return True

    @staticmethod
    def _find_file(dest_root: Path, filename: str) -> Optional[Path]:
        """Search for a file in common subdirectories"""
        for subdir in ["Source", "Include", "Code/Source", "Code/Include", ""]:
            candidate = dest_root / subdir / filename if subdir else dest_root / filename
            if candidate.is_file():
                return candidate
        # Fallback: recursive search
        for match in dest_root.rglob(filename):
            if match.is_file():
                return match
        return None
