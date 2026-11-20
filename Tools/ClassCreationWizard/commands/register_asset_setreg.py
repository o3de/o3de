#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

import json
import re
from pathlib import Path
from typing import Optional

from command_plugin import WizardCommand, CommandContext, CommandRegistry


@CommandRegistry.register("register_asset_setreg")
class RegisterAssetSetregCommand(WizardCommand):
    """Update .setreg with asset config"""

    def __init__(self, asset_name: str, asset_ext: str = "mydata"):
        self.asset_name = asset_name
        self.asset_ext = asset_ext

    @property
    def name(self) -> str:
        return "register_asset_setreg"

    @property
    def description(self) -> str:
        return "Update .setreg with asset processor configuration"

    def execute(self, ctx: CommandContext) -> bool:
        ctx.log(f"Updating setreg for {self.asset_name}...")

        setreg_name = f"{ctx.namespace}DataAssetRegistry.setreg"
        setreg_path = self._find_setreg(ctx, setreg_name)

        if not setreg_path:
            ctx.log(f"Warning: Could not find setreg file {setreg_name}")
            return True

        guid = self._extract_asset_guid(ctx, self.asset_name)
        if not guid:
            guid = "{00000000-0000-0000-0000-000000000000}"
            ctx.log("Warning: Using placeholder GUID")

        try:
            data = json.loads(setreg_path.read_text(encoding="utf-8"))
        except Exception:
            data = {"Amazon": {"AssetProcessor": {"Settings": {}}}}

        if "Amazon" not in data:
            data["Amazon"] = {}
        if "AssetProcessor" not in data["Amazon"]:
            data["Amazon"]["AssetProcessor"] = {}
        if "Settings" not in data["Amazon"]["AssetProcessor"]:
            data["Amazon"]["AssetProcessor"]["Settings"] = {}

        settings = data["Amazon"]["AssetProcessor"]["Settings"]

        rc_key = f"RC {self.asset_name}"
        settings[rc_key] = {
            "glob": f"*.{self.asset_ext}",
            "params": "copy",
            "productAssetType": guid
        }

        setreg_path.write_text(json.dumps(data, indent=4) + "\n", encoding="utf-8", newline="\n")
        ctx.log(f"Updated setreg: {setreg_path}")
        return True

    def _find_setreg(self, ctx: CommandContext, setreg_name: str) -> Optional[Path]:
        """Find the setreg file"""
        candidates = [
            ctx.dest_root / "Registry" / setreg_name,
            ctx.dest_root.parent / "Registry" / setreg_name,
            ctx.dest_root / setreg_name,
            ctx.dest_root / "Gem" / "Registry" / setreg_name,
        ]

        ctx.log(f"Looking for setreg file: {setreg_name}")
        for candidate in candidates:
            ctx.log(f"  Checking: {candidate}")
            if candidate.is_file():
                ctx.log(f"  Found: {candidate}")
                return candidate

        ctx.log(f"  Setreg not found in any expected location")
        return None

    def _extract_asset_guid(self, ctx: CommandContext, asset_name: str) -> Optional[str]:
        """Extract UUID from asset class"""
        guid_pattern = r'\{?[0-9A-Fa-f]{8}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{12}\}?'

        candidates = [
            ctx.dest_root / "Source" / f"{asset_name}.h",
            ctx.dest_root / "Source" / f"{asset_name}.cpp",
            ctx.dest_root / "Code" / "Source" / f"{asset_name}.h",
            ctx.dest_root / "Code" / "Source" / f"{asset_name}.cpp",
        ]

        ctx.log(f"Searching for GUID in asset files for '{asset_name}'...")

        for candidate in candidates:
            if not candidate.is_file():
                ctx.log(f"  Not found: {candidate}")
                continue

            ctx.log(f"  Checking: {candidate}")

            try:
                text = candidate.read_text(encoding="utf-8", errors="ignore")
            except Exception as e:
                ctx.log(f"  Error reading: {e}")
                continue

            rtti_match = re.search(
                rf'AZ_RTTI\s*\(\s*{re.escape(asset_name)}\s*,\s*["\']?({guid_pattern})["\']?',
                text
            )
            if rtti_match:
                guid = rtti_match.group(1)
                if not guid.startswith('{'):
                    guid = '{' + guid + '}'
                ctx.log(f"  Found GUID via AZ_RTTI: {guid}")
                return guid

            type_info_match = re.search(
                rf'AZ_TYPE_INFO\s*\(\s*{re.escape(asset_name)}\s*,\s*["\']?({guid_pattern})["\']?',
                text
            )
            if type_info_match:
                guid = type_info_match.group(1)
                if not guid.startswith('{'):
                    guid = '{' + guid + '}'
                ctx.log(f"  Found GUID via AZ_TYPE_INFO: {guid}")
                return guid

            class_pattern = rf'class\s+{re.escape(asset_name)}\b.*?({guid_pattern})'
            class_match = re.search(class_pattern, text, re.DOTALL)
            if class_match:
                guid = class_match.group(1)
                if not guid.startswith('{'):
                    guid = '{' + guid + '}'
                ctx.log(f"  Found GUID via class search: {guid}")
                return guid

        ctx.log(f"  No GUID found for {asset_name}")
        return None
