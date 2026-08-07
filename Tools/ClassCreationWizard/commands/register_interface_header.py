#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

import re
from pathlib import Path
from typing import List

from command_plugin import WizardCommand, CommandContext, CommandRegistry, CMakeTarget, CMakeAnalyzer


@CommandRegistry.register("register_interface_header")
class RegisterInterfaceHeaderCommand(WizardCommand):
    """Register interface header to INTERFACE/API target"""

    is_registration_command = True

    def __init__(self, component_name: str):
        self.component_name = component_name

    @property
    def name(self) -> str:
        return "register_interface_header"

    @property
    def description(self) -> str:
        return "Register interface header to INTERFACE/API target"

    def execute(self, ctx: CommandContext) -> bool:
        if not ctx.build_target:
            ctx.log("Warning: No build target selected for interface header")
            return True

        interface_hdr_path = ctx.dest_root / "Include" / ctx.namespace / f"{self.component_name}Interface.h"

        if not interface_hdr_path.exists():
            ctx.log(f"No interface header found at {interface_hdr_path}, skipping")
            return True

        ctx.log(f"Registering interface header for '{self.component_name}'...")

        cmake_dir = ctx.build_target.file.parent
        all_targets = CMakeAnalyzer.scan_targets(cmake_dir, ctx.namespace)

        interface_target = self._find_interface_target(all_targets, ctx.namespace, ctx.build_target)

        rel_hdr = f"Include/{ctx.namespace}/{self.component_name}Interface.h"

        if interface_target.kind == "o3de_add_target" and interface_target.files_cmake_list:
            for files_cmake in interface_target.files_cmake_list:
                include_path = (interface_target.file.parent / files_cmake).resolve()
                if include_path.exists():
                    self._update_interface_files_cmake(include_path, rel_hdr, ctx)
                    return True

        self._add_interface_to_target(interface_target.file, interface_target, rel_hdr, ctx)
        return True

    def _find_interface_target(self, targets: List[CMakeTarget], gem_name: str, fallback: CMakeTarget) -> CMakeTarget:
        """Find best target for interface headers"""
        for target in targets:
            if 'INTERFACE' in target.name.upper():
                return target

        for target in targets:
            if target.name.endswith('.API') or '.API.' in target.name:
                return target

        return fallback

    def _update_interface_files_cmake(self, files_cmake_path: Path, rel_hdr: str, ctx: CommandContext):
        """Add interface header to FILES_CMAKE file"""
        if files_cmake_path.exists():
            text = files_cmake_path.read_text(encoding="utf-8")
        else:
            text = "set(FILES\n)\n"

        if rel_hdr in text:
            ctx.log(f"Interface header already in {files_cmake_path.name}")
            return

        match = re.search(r'set\s*\(\s*FILES\b(.*?)(\))', text, flags=re.S | re.M)
        if match:
            end_pos = match.end(1)
            text = text[:end_pos] + f"    {rel_hdr}\n" + text[end_pos:]
        else:
            text = text.rstrip() + f"\nset(FILES\n    {rel_hdr}\n)\n"

        files_cmake_path.write_text(text, encoding="utf-8", newline="\n")
        ctx.log(f"Added interface header to {files_cmake_path.name}")

    def _add_interface_to_target(self, cmake_path: Path, target: CMakeTarget, rel_hdr: str, ctx: CommandContext):
        """Add interface header via target_sources"""
        text = cmake_path.read_text(encoding="utf-8")

        if rel_hdr in text:
            ctx.log(f"Interface header already in {cmake_path.name}")
            return

        pattern = rf'target_sources\s*\(\s*{re.escape(target.name)}\s+INTERFACE\s+([^)]*)\)'
        match = re.search(pattern, text, flags=re.S)

        if match:
            content = match.group(1)
            indent_match = re.search(r'\n(\s+)', content)
            indent = indent_match.group(1) if indent_match else '    '
            new_content = content.rstrip() + f'\n{indent}{rel_hdr}\n'
            new_text = text[:match.start(1)] + new_content + text[match.end(1):]
            text = new_text
        else:
            append = (
                f"\n# Interface header\n"
                f"target_sources({target.name} INTERFACE\n"
                f"    {rel_hdr}\n"
                f")\n"
            )
            text = text.rstrip() + append

        cmake_path.write_text(text, encoding="utf-8", newline="\n")
        ctx.log(f"Added interface header to {target.name}")
