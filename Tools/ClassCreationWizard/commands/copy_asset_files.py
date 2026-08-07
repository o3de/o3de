#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

"""
copy_asset_files
================

Copies a subtree of asset files (shaders, .pass, .azasset, materials, textures
and similar) from a template's source directory directly into the destination
gem's Assets/ folder, bypassing the o3de create-from-template staging pipeline.

Why this exists
---------------
The standard staging path runs every file through `o3de create-from-template`,
which lands files at <dest_root>/<rel_path>. For an external gem, dest_root is
typically <gem>/Code, so a file at Template/Passes/Foo.pass would land at
<gem>/Code/Passes/Foo.pass. Asset files belong at <gem>/Assets/Passes/Foo.pass
(the AssetProcessor scan root), not under Code/.

Rather than introducing a per-CopyFileDef destination override (which would
ripple through the merge pipeline, the conditional excluder, and every existing
template), this command takes ownership of asset files for templates that opt
in. Asset files live under a sibling folder in the template (default name:
TemplateAssets/) that the standard staging never sees, and this command copies
them with ${Name}/${GemName} substitution applied to both path components and
file contents.

Template layout convention
--------------------------
  Templates/MyTemplate/
      template.json
      Template/                     <- consumed by staging  (Code/Source/...)
          Source/${Name}Component.cpp
      TemplateAssets/               <- consumed by THIS command (Assets/...)
          Passes/${Name}.pass
          Shaders/${Name}.azsl
          Shaders/${Name}.shader

Args (template.json process_commands entry)
-------------------------------------------
  source_subdir : str   default "TemplateAssets"
        Folder under the template root to walk.
  dest_subdir   : str   default "Assets"
        Folder under <gem_root> to write into.
  is_templated  : bool  default True
        If True, ${Name}/${GemName}/etc. are substituted in file *contents*.
        Path components are always substituted regardless.
  skip_existing : bool  default True
        If True, do not overwrite existing destination files. Existing assets
        usually contain user edits.

Resolved gem root layout
------------------------
Mirrors _handle_setreg_file in ClassWizard.py:
  <project>/Gem    -> <project>/Gem/Assets
  <gem>/Code       -> <gem>/Assets               (external gem)
  <gem>/           -> <gem>/Assets               (passed directly)
"""

import re
import shutil
from pathlib import Path
from typing import Dict, Any, Optional

from command_plugin import WizardCommand, CommandContext, CommandRegistry


@CommandRegistry.register("copy_asset_files")
class CopyAssetFilesCommand(WizardCommand):
    """Copy a template's asset subtree into <gem>/Assets/ with var substitution."""

    # Pattern shared with VariableResolver in ClassWizard.py.
    VAR_PATTERN = re.compile(r'\$\{(\w+)\}')

    def __init__(self,
                 source_subdir: str = "TemplateAssets",
                 dest_subdir: str = "Assets",
                 is_templated: Any = True,
                 skip_existing: Any = True):
        self.source_subdir = source_subdir
        self.dest_subdir = dest_subdir
        self.is_templated = self._coerce_bool(is_templated, default=True)
        self.skip_existing = self._coerce_bool(skip_existing, default=True)

    @property
    def name(self) -> str:
        return "copy_asset_files"

    @property
    def description(self) -> str:
        return "Copy template asset files (shaders, passes, azassets) into <gem>/Assets/"

    @property
    def author(self) -> str:
        return "O3DE"

    # ------------------------------------------------------------------
    # Main entry
    # ------------------------------------------------------------------

    def execute(self, ctx: CommandContext) -> bool:
        if ctx.template_path is None:
            ctx.log("copy_asset_files: ctx.template_path not set; nothing to copy")
            return True

        source_root = ctx.template_path / self.source_subdir
        if not source_root.is_dir():
            ctx.log(f"copy_asset_files: source dir not found, skipping: {source_root}")
            return True

        dest_root = self._resolve_gem_assets_dir(Path(ctx.dest_root).resolve()) / self.dest_subdir_tail()
        dest_root.mkdir(parents=True, exist_ok=True)
        ctx.log(f"copy_asset_files: copying {source_root} -> {dest_root}")

        copied = skipped = 0
        for src in sorted(source_root.rglob("*")):
            if src.is_dir():
                continue

            rel_parts = [self._substitute(part, ctx.variables) for part in src.relative_to(source_root).parts]
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
                    # newline="\n" on open() works on all supported Pythons;
                    # Path.write_text(newline=...) is 3.10+.
                    with open(dst, "w", encoding="utf-8", newline="\n") as fh:
                        fh.write(text)
                except UnicodeDecodeError:
                    shutil.copy2(src, dst)
            else:
                shutil.copy2(src, dst)

            ctx.log(f"  Copied: {dst.relative_to(dest_root)}")
            copied += 1

        ctx.log(f"copy_asset_files: {copied} file(s) copied, {skipped} skipped")
        return True

    # ------------------------------------------------------------------
    # Helpers
    # ------------------------------------------------------------------

    def dest_subdir_tail(self) -> str:
        """Return the dest_subdir as the leaf path under gem root.

        Empty string if the user wants files placed directly at gem root.
        """
        return self.dest_subdir or ""

    @staticmethod
    def _resolve_gem_assets_dir(dest_root: Path) -> Path:
        """Same gem-root resolution rule as the setreg handler.

        We return the gem root, NOT the Assets subdir; the caller appends
        dest_subdir on top. This keeps copy_asset_files decoupled from the
        directory name (Assets, GameAssets, Materials, ...).
        """
        if dest_root.name == "Gem":
            return dest_root
        if dest_root.name == "Code" and (dest_root.parent / "gem.json").is_file():
            return dest_root.parent
        if (dest_root / "gem.json").is_file():
            return dest_root
        return dest_root

    def _substitute(self, text: str, variables: Dict[str, Any]) -> str:
        """Replace ${var} tokens. Mirrors VariableResolver.resolve()."""
        if not text or not isinstance(text, str):
            return text

        def _repl(m: re.Match) -> str:
            value = variables.get(m.group(1))
            return str(value) if value is not None else m.group(0)

        return self.VAR_PATTERN.sub(_repl, text)

    @staticmethod
    def _is_text_file(path: Path) -> bool:
        """Decide if substitution should be applied to the file contents.

        Asset files we expect: .azsl .azsli .shader .pass .azasset .material
        .materialtype .lua .json .setreg .xml .luac (textual). Binary assets
        (textures, models) are copied byte-for-byte.
        """
        text_exts = {
            '.azsl', '.azsli', '.shader', '.pass', '.azasset',
            '.material', '.materialtype', '.json', '.setreg',
            '.xml', '.lua', '.h', '.cpp', '.inl', '.txt', '.md'
        }
        return path.suffix.lower() in text_exts

    @staticmethod
    def _coerce_bool(value: Any, default: bool) -> bool:
        """Accept True/False or "true"/"false" strings from template.json args."""
        if isinstance(value, bool):
            return value
        if isinstance(value, str):
            v = value.strip().lower()
            if v in ("true", "1", "yes", "on"):
                return True
            if v in ("false", "0", "no", "off"):
                return False
        return default
