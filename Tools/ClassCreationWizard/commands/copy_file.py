#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

import shutil
from pathlib import Path

from command_plugin import WizardCommand, CommandContext, CommandRegistry


@CommandRegistry.register("copy_file")
class CopyFileCommand(WizardCommand):
    """Copy a file to destination"""

    def __init__(self, source: str, dest: str):
        self.source = source
        self.dest = dest

    @property
    def name(self) -> str:
        return "copy_file"

    @property
    def description(self) -> str:
        return "Copy a file to destination path"

    def execute(self, ctx: CommandContext) -> bool:
        source_path = ctx.dest_root / self.source
        dest_path = ctx.dest_root / self.dest

        if not source_path.exists():
            ctx.log(f"Warning: Source file not found: {source_path}")
            return True

        dest_path.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source_path, dest_path)
        ctx.log(f"Copied {self.source} to {self.dest}")
        return True
