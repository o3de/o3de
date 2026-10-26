#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

from pathlib import Path

from command_plugin import WizardCommand, CommandContext, CommandRegistry


@CommandRegistry.register("copy_setreg")
class CopySetregCommand(WizardCommand):
    """Copy setreg file to Registry folder"""

    def __init__(self, setreg_name: str):
        self.setreg_name = setreg_name

    @property
    def name(self) -> str:
        return "copy_setreg"

    @property
    def description(self) -> str:
        return "Copy setreg file to Registry folder"

    def execute(self, ctx: CommandContext) -> bool:
        ctx.log(f"Handling setreg file: {self.setreg_name}...")

        dest_root = ctx.dest_root.resolve()

        if dest_root.name == "Gem":
            target_dir = dest_root / "Registry"
        elif dest_root.name == "Code":
            gem_json = dest_root.parent / "gem.json"
            if gem_json.is_file():
                target_dir = dest_root.parent / "Registry"
            else:
                target_dir = dest_root / "Registry"
        else:
            target_dir = dest_root / "Registry"

        target_dir.mkdir(parents=True, exist_ok=True)
        ctx.log(f"Registry directory: {target_dir}")

        return True
