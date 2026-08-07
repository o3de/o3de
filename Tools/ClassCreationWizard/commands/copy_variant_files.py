#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

"""
copy_variant_files
==================

Copies a *variant subtree* of code files from a template into the gem's source
tree, picking which variant to use from a wizard input variable.

Why this exists
---------------
The wizard's existing copyFiles + ConditionalFileExcluder pipeline can drop
individual files based on a condition, but it cannot select between three
parallel sets of files with the SAME final destination paths. For templates
that ship multiple integration modes (e.g. a render pass that can attach
itself via a PostProcess Volume Component, via runtime PassTemplate injection,
or as a manual pass asset), each mode needs its own RenderingSystemComponent
shape -- and they all want to live at <gem>/Source/${GemName}RenderingSystemComponent.cpp.

This command handles that cleanly:
  * Variants live in <template>/Variants/<VariantName>/<rel_paths>
  * The wizard reads <variant_var> at generation time
  * Only the matching variant's files are copied; the others are skipped
  * Variable substitution (${Name}, ${GemName}, etc.) is applied to both
    path components and file contents, mirroring copy_asset_files
  * Files land at <gem_root>/<dest_subdir>/<rel_path>, defaulting to <gem>/Source/

Template layout convention
--------------------------
  Templates/MyTemplate/
      template.json
      Template/                                   <- staged for all variants
          Source/SharedThing.cpp
      Variants/
          PostVolume/
              Source/${GemName}RenderingSystemComponent.cpp
              Source/${Name}Settings.h
          ScreenSpaceConstant/
              Source/${GemName}RenderingSystemComponent.cpp
          ManualPassAsset/
              Source/${GemName}RenderingSystemComponent.cpp

Args (template.json process_commands entry)
-------------------------------------------
  variant_root  : str   default "Variants"
        Folder under the template root that holds the variant subdirs.
  variant_var   : str   required
        Name of the wizard input variable that names the active variant.
        E.g. "integration_mode". The value is matched (case-sensitive) to
        a subdirectory of <variant_root>.
  dest_subdir   : str   default ""
        Folder under <gem_root> to write into. Empty string writes to gem
        root. Most templates want "" so files land at <gem>/Source/...
        as if they had been listed in copyFiles.
  is_templated  : bool  default True
        Apply ${var} substitution to file contents.
  skip_existing : bool  default True
        Do not overwrite existing files. The wizard's normal merge step also
        skips existing files, so this matches that behavior.

Resolved gem root layout
------------------------
Mirrors copy_asset_files / _handle_setreg_file:
  <project>/Gem    -> <project>/Gem
  <gem>/Code       -> <gem>                       (external gem)
  <gem>/           -> <gem>                       (passed directly)
"""

import re
import shutil
from pathlib import Path
from typing import Dict, Any

from command_plugin import WizardCommand, CommandContext, CommandRegistry


@CommandRegistry.register("copy_variant_files")
class CopyVariantFilesCommand(WizardCommand):
    """Copy one of several parallel variant subtrees into the gem's source tree."""

    VAR_PATTERN = re.compile(r'\$\{(\w+)\}')

    def __init__(self,
                 variant_var: str,
                 variant_root: str = "Variants",
                 dest_subdir: str = "",
                 is_templated: Any = True,
                 skip_existing: Any = True):
        self.variant_var = variant_var
        self.variant_root = variant_root
        self.dest_subdir = dest_subdir
        self.is_templated = self._coerce_bool(is_templated, default=True)
        self.skip_existing = self._coerce_bool(skip_existing, default=True)

    @property
    def name(self) -> str:
        return "copy_variant_files"

    @property
    def description(self) -> str:
        return "Copy one variant subtree from <template>/Variants/<choice>/ into <gem>/"

    @property
    def author(self) -> str:
        return "O3DE"

    # ------------------------------------------------------------------
    # Main entry
    # ------------------------------------------------------------------

    def execute(self, ctx: CommandContext) -> bool:
        if ctx.template_path is None:
            ctx.log("copy_variant_files: ctx.template_path not set; nothing to copy")
            return True

        if not self.variant_var:
            ctx.log("copy_variant_files: variant_var is required, skipping")
            return True

        active_variant = ctx.variables.get(self.variant_var)
        if not active_variant:
            ctx.log(f"copy_variant_files: variable '{self.variant_var}' is empty, skipping")
            return True

        source_root = ctx.template_path / self.variant_root / str(active_variant)
        if not source_root.is_dir():
            ctx.log(f"copy_variant_files: variant directory not found: {source_root}")
            return True

        gem_root = self._resolve_gem_root(Path(ctx.dest_root).resolve())
        dest_root = gem_root / self.dest_subdir if self.dest_subdir else gem_root
        dest_root.mkdir(parents=True, exist_ok=True)
        ctx.log(f"copy_variant_files: variant '{active_variant}' -> {dest_root}")

        copied = skipped = 0
        for src in sorted(source_root.rglob("*")):
            if src.is_dir():
                continue

            rel_parts = [self._substitute(part, ctx.variables)
                         for part in src.relative_to(source_root).parts]
            dst = dest_root.joinpath(*rel_parts)

            if dst.exists() and self.skip_existing:
                ctx.log(f"  Skipped existing: {dst.relative_to(dest_root)}")
                skipped += 1
                continue

            dst.parent.mkdir(parents=True, exist_ok=True)

            if self.is_templated and self._is_text_file(src):
                try:
                    text = src.read_text(encoding="utf-8")
                    text = self._substitute(text, ctx.variables)
                    with open(dst, "w", encoding="utf-8", newline="\n") as fh:
                        fh.write(text)
                except UnicodeDecodeError:
                    shutil.copy2(src, dst)
            else:
                shutil.copy2(src, dst)

            ctx.log(f"  Copied: {dst.relative_to(dest_root)}")
            copied += 1

        ctx.log(f"copy_variant_files: {copied} file(s) copied, {skipped} skipped")
        return True

    # ------------------------------------------------------------------
    # Helpers (mirror copy_asset_files for consistency)
    # ------------------------------------------------------------------

    @staticmethod
    def _resolve_gem_root(dest_root: Path) -> Path:
        if dest_root.name == "Gem":
            return dest_root
        if dest_root.name == "Code" and (dest_root.parent / "gem.json").is_file():
            return dest_root.parent
        if (dest_root / "gem.json").is_file():
            return dest_root
        return dest_root

    def _substitute(self, text: str, variables: Dict[str, Any]) -> str:
        if not text or not isinstance(text, str):
            return text

        def _repl(m: re.Match) -> str:
            value = variables.get(m.group(1))
            return str(value) if value is not None else m.group(0)

        return self.VAR_PATTERN.sub(_repl, text)

    @staticmethod
    def _is_text_file(path: Path) -> bool:
        text_exts = {
            '.azsl', '.azsli', '.shader', '.pass', '.azasset',
            '.material', '.materialtype', '.json', '.setreg',
            '.xml', '.lua', '.h', '.cpp', '.inl', '.txt', '.md'
        }
        return path.suffix.lower() in text_exts

    @staticmethod
    def _coerce_bool(value: Any, default: bool) -> bool:
        if isinstance(value, bool):
            return value
        if isinstance(value, str):
            v = value.strip().lower()
            if v in ("true", "1", "yes", "on"):  return True
            if v in ("false", "0", "no", "off"): return False
        return default
