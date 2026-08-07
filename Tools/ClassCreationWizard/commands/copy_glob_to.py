#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

"""
copy_glob_to
============

Glob-based companion to copy_file_to. Copies every staged file matching a
glob pattern into a destination directory under one of the named anchors,
preserving the relative path structure beneath the matched root.

Use this when an entire subtree of staged files has the same destination
anchor and dest_subdir, e.g. "everything under TemplateAssets/Shaders/ goes
under <gem>/Assets/Shaders/". For one-off file copies use copy_file_to
directly.

Args
----
    source_glob   : str, required
        Glob pattern relative to the staging dir. Standard Path.glob syntax;
        use ** for recursive matches. Examples:
            "TemplateAssets/Shaders/**/*"
            "Variants/PostVolume/Source/*"
    dest_anchor   : str, default "dest_root"
        Same anchor set as copy_file_to: dest_root | gem_root | gem_assets |
        gem_registry | engine_root.
    dest_subdir   : str, default ""
        Path under the anchor that becomes the new root of the matched tree.
    strip_prefix  : str, default ""
        Optional path prefix to strip from each matched file's relative path
        before joining onto dest_subdir. Useful when the staging tree has a
        scaffold prefix (TemplateAssets/Shaders/Foo.azsl) you don't want to
        carry into the destination (Assets/Shaders/Foo.azsl).
    is_templated  : bool, default True
        Apply ${var} substitution to file CONTENTS for text-typed extensions.
    skip_existing : bool, default True
        Don't overwrite files that already exist at the destination.
    create_dirs   : bool, default True
        mkdir -p destination subdirectories.
"""

import re
import shutil
from pathlib import Path
from typing import Any, Dict, Optional

from command_plugin import WizardCommand, CommandContext, CommandRegistry


@CommandRegistry.register("copy_glob_to")
class CopyGlobToCommand(WizardCommand):
    """Glob multiple staged files into a destination directory under a named anchor."""

    VAR_PATTERN = re.compile(r'\$\{(\w+)\}')

    TEXT_EXTS = {
        '.azsl', '.azsli', '.shader', '.pass', '.azasset',
        '.material', '.materialtype', '.json', '.setreg',
        '.xml', '.lua', '.h', '.hpp', '.cpp', '.inl',
        '.txt', '.md', '.cmake'
    }

    def __init__(self,
                 source_glob: str,
                 dest_anchor: str = "dest_root",
                 dest_subdir: str = "",
                 strip_prefix: str = "",
                 is_templated: Any = True,
                 skip_existing: Any = True,
                 create_dirs: Any = True):
        self.source_glob = source_glob
        self.dest_anchor = dest_anchor
        self.dest_subdir = dest_subdir
        self.strip_prefix = strip_prefix.replace("\\", "/").strip("/")
        self.is_templated = self._coerce_bool(is_templated, default=True)
        self.skip_existing = self._coerce_bool(skip_existing, default=True)
        self.create_dirs = self._coerce_bool(create_dirs, default=True)

    @property
    def name(self) -> str:
        return "copy_glob_to"

    @property
    def description(self) -> str:
        return "Glob staged files into an anchor-rooted destination directory"

    @property
    def author(self) -> str:
        return "O3DE"

    # ------------------------------------------------------------------
    # Main entry
    # ------------------------------------------------------------------

    def execute(self, ctx: CommandContext) -> bool:
        if not self.source_glob:
            ctx.log("copy_glob_to: 'source_glob' is required")
            return True

        if ctx.stage_dir is None:
            ctx.log("copy_glob_to: ctx.stage_dir is not set; nothing to copy")
            return True

        anchor_root = self._resolve_anchor(ctx)
        if anchor_root is None:
            ctx.log(f"copy_glob_to: unknown anchor '{self.dest_anchor}'")
            return True

        dest_root = anchor_root / self.dest_subdir if self.dest_subdir else anchor_root

        copied = skipped = 0
        for src in sorted(Path(ctx.stage_dir).glob(self.source_glob)):
            if src.is_dir():
                continue

            rel_posix = str(src.relative_to(ctx.stage_dir)).replace("\\", "/")
            if self.strip_prefix and rel_posix.startswith(self.strip_prefix + "/"):
                rel_posix = rel_posix[len(self.strip_prefix) + 1:]
            elif self.strip_prefix and rel_posix == self.strip_prefix:
                continue  # strip_prefix matched the file itself, nothing left

            dst = (dest_root / rel_posix).resolve()

            if dst.exists() and self.skip_existing:
                ctx.log(f"  Skipped existing: {rel_posix}")
                skipped += 1
                continue

            if self.create_dirs:
                dst.parent.mkdir(parents=True, exist_ok=True)
            elif not dst.parent.is_dir():
                ctx.log(f"  Parent missing and create_dirs=false: {dst.parent}")
                continue

            if self.is_templated and src.suffix.lower() in self.TEXT_EXTS:
                try:
                    text = src.read_text(encoding="utf-8")
                    text = self._substitute(text, ctx.variables)
                    with open(dst, "w", encoding="utf-8", newline="\n") as fh:
                        fh.write(text)
                except UnicodeDecodeError:
                    shutil.copy2(src, dst)
            else:
                shutil.copy2(src, dst)

            ctx.log(f"  Copied: {rel_posix} -> {dst}")
            copied += 1

        ctx.log(f"copy_glob_to: {copied} file(s) copied, {skipped} skipped")
        return True

    # ------------------------------------------------------------------
    # Anchors (mirror copy_file_to)
    # ------------------------------------------------------------------

    def _resolve_anchor(self, ctx: CommandContext) -> Optional[Path]:
        anchor = (self.dest_anchor or "dest_root").strip().lower()
        dest_root = Path(ctx.dest_root).resolve()

        if anchor == "dest_root":
            return dest_root

        gem_root = self._gem_root_from_dest(dest_root)
        if anchor == "gem_root":      return gem_root
        if anchor == "gem_assets":    return gem_root / "Assets"
        if anchor == "gem_registry":  return gem_root / "Registry"
        if anchor == "engine_root":   return Path(ctx.engine_path).resolve()
        return None

    @staticmethod
    def _gem_root_from_dest(dest_root: Path) -> Path:
        if dest_root.name == "Gem":
            return dest_root
        if dest_root.name == "Code" and (dest_root.parent / "gem.json").is_file():
            return dest_root.parent
        if (dest_root / "gem.json").is_file():
            return dest_root
        return dest_root

    # ------------------------------------------------------------------
    # Helpers
    # ------------------------------------------------------------------

    def _substitute(self, text: str, variables: Dict[str, Any]) -> str:
        if not text or not isinstance(text, str):
            return text

        def _repl(m: re.Match) -> str:
            value = variables.get(m.group(1))
            return str(value) if value is not None else m.group(0)

        return self.VAR_PATTERN.sub(_repl, text)

    @staticmethod
    def _coerce_bool(value: Any, default: bool) -> bool:
        if isinstance(value, bool):
            return value
        if isinstance(value, str):
            v = value.strip().lower()
            if v in ("true", "1", "yes", "on"):  return True
            if v in ("false", "0", "no", "off"): return False
        return default
