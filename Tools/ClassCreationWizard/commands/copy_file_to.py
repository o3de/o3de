#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

"""
copy_file_to
============

General-purpose "copy one staged file to any destination, creating directories
as needed" command. The single primitive intended to replace the family of
specialised copiers (copy_file, copy_setreg, copy_asset_files, copy_variant_files)
in NEW templates while leaving those legacy commands registered for older ones.

Source / destination model
--------------------------
SOURCE
    Always a path inside the live staging directory (CommandContext.stage_dir).
    The path is the same path the template authored in copyFiles `file:` --
    e.g. `Variants/PostVolume/Source/${Name}Pass.h`. The o3de CLI has already
    applied ${var} substitution to both path components and contents during
    staging. Files staged with `excludeFromMerge: true` in the template's
    copyFiles entry are still present here; files without that flag have been
    bulk-merged to dest_root by the time process_commands run, but a copy
    from staging still works (the original copy is intact in stage_dir).

DESTINATION
    A path under one of the named anchors. Anchors:

      dest_root      -- (default) write under CommandContext.dest_root.
                        For an external gem this is <gem>/Code/. Match for
                        files that belong inside the build target's source
                        tree, the same place the legacy bulk merge writes to.

      gem_root       -- write under the gem root (parent of Code/, sibling of
                        Registry/, Assets/, etc.). Match for assets, level data,
                        anything outside the C++ build tree.

      gem_assets     -- write under <gem_root>/Assets/. Convenience alias for
                        gem_root with a fixed Assets/ prefix; same as setting
                        anchor=gem_root + destination="Assets/<rest>".

      gem_registry   -- write under <gem_root>/Registry/. Convenience alias.

      engine_root    -- write under CommandContext.engine_path. Use sparingly;
                        modifying engine source from a template is unusual.

Args
----
    source        : str, required
        Path inside the staging dir.
    destination   : str, required
        Path under the resolved anchor.
    anchor        : str, default "dest_root"
        One of: dest_root, gem_root, gem_assets, gem_registry, engine_root.
    is_templated  : bool, default True
        Apply ${var} substitution to the file CONTENTS as well. Path
        components are always substituted before the copy fires (the wizard
        resolves ${var} in process_commands args at command-create time).
        Useful for the few files o3de's stager doesn't touch (.azsl, .pass,
        anything whose extension is outside its substitution allowlist).
    skip_existing : bool, default True
        If the destination already exists, do not overwrite. Re-runs of the
        wizard pick up new files only.
    create_dirs   : bool, default True
        mkdir -p the destination's parent if it doesn't exist. Set to false
        only when you want a missing directory to surface as an error.
"""

import re
import shutil
from pathlib import Path
from typing import Any, Dict, Optional

from command_plugin import WizardCommand, CommandContext, CommandRegistry


@CommandRegistry.register("copy_file_to")
class CopyFileToCommand(WizardCommand):
    """Copy one staged file to any destination under a named gem/engine anchor."""

    VAR_PATTERN = re.compile(r'\$\{(\w+)\}')

    # Set of file extensions for which is_templated=True applies. Anything
    # outside this set is copied byte-for-byte even when is_templated is True
    # (binary assets shouldn't be UTF-8 round-tripped).
    TEXT_EXTS = {
        '.azsl', '.azsli', '.shader', '.pass', '.azasset',
        '.material', '.materialtype', '.json', '.setreg',
        '.xml', '.lua', '.h', '.hpp', '.cpp', '.inl',
        '.txt', '.md', '.cmake'
    }

    def __init__(self,
                 source: str,
                 destination: str,
                 anchor: str = "dest_root",
                 is_templated: Any = True,
                 skip_existing: Any = True,
                 create_dirs: Any = True):
        self.source = source
        self.destination = destination
        self.anchor = anchor
        self.is_templated = self._coerce_bool(is_templated, default=True)
        self.skip_existing = self._coerce_bool(skip_existing, default=True)
        self.create_dirs = self._coerce_bool(create_dirs, default=True)

    @property
    def name(self) -> str:
        return "copy_file_to"

    @property
    def description(self) -> str:
        return "Copy one staged file to any anchor-rooted destination"

    @property
    def author(self) -> str:
        return "O3DE"

    # ------------------------------------------------------------------
    # Main entry
    # ------------------------------------------------------------------

    def execute(self, ctx: CommandContext) -> bool:
        if not self.source or not self.destination:
            ctx.log("copy_file_to: 'source' and 'destination' are required")
            return True

        if ctx.stage_dir is None:
            ctx.log("copy_file_to: ctx.stage_dir is not set; nothing to copy")
            return True

        src = (ctx.stage_dir / self.source).resolve()
        if not src.is_file():
            ctx.log(f"copy_file_to: source not found in stage: {self.source}")
            return True

        anchor_root = self._resolve_anchor(ctx)
        if anchor_root is None:
            ctx.log(f"copy_file_to: unknown anchor '{self.anchor}'; "
                    "valid: dest_root, gem_root, gem_assets, gem_registry, engine_root")
            return True

        dst = (anchor_root / self.destination).resolve()

        if dst.exists() and self.skip_existing:
            ctx.log(f"copy_file_to: skipped existing -> {dst}")
            return True

        if self.create_dirs:
            dst.parent.mkdir(parents=True, exist_ok=True)
        elif not dst.parent.is_dir():
            ctx.log(f"copy_file_to: destination parent missing and create_dirs=false: {dst.parent}")
            return True

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

        ctx.log(f"copy_file_to: {self.source} -> {dst}")
        return True

    # ------------------------------------------------------------------
    # Anchors
    # ------------------------------------------------------------------

    def _resolve_anchor(self, ctx: CommandContext) -> Optional[Path]:
        anchor = (self.anchor or "dest_root").strip().lower()
        dest_root = Path(ctx.dest_root).resolve()

        if anchor == "dest_root":
            return dest_root

        gem_root = self._gem_root_from_dest(dest_root)

        if anchor == "gem_root":
            return gem_root
        if anchor == "gem_assets":
            return gem_root / "Assets"
        if anchor == "gem_registry":
            return gem_root / "Registry"
        if anchor == "engine_root":
            return Path(ctx.engine_path).resolve()
        return None

    @staticmethod
    def _gem_root_from_dest(dest_root: Path) -> Path:
        """Same gem-root resolution as _handle_setreg_file in ClassWizard.py."""
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
