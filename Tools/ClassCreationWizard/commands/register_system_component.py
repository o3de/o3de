#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

import re
from pathlib import Path

from command_plugin import WizardCommand, CommandContext, CommandRegistry


@CommandRegistry.register("register_system_component")
class RegisterSystemComponentCommand(WizardCommand):
    """Register system component in module's required components list"""

    is_registration_command = True

    def __init__(self, component_name: str, module_kind: str = "runtime"):
        self.component_name = component_name
        self.module_kind = module_kind

    @property
    def name(self) -> str:
        return "register_system_component"

    @property
    def description(self) -> str:
        return "Add to GetRequiredSystemComponents() list"

    def execute(self, ctx: CommandContext) -> bool:
        ctx.log(f"Registering system component '{self.component_name}' in {self.module_kind} module...")

        suffix = "EditorModule" if self.module_kind == "editor" else "Module"
        candidates = [
            ctx.dest_root / "Code" / "Source" / "Tools" / f"{ctx.namespace}{suffix}.cpp",
            ctx.dest_root / "Code" / "Source" / f"{ctx.namespace}{suffix}.cpp",
            ctx.dest_root / "Source" / f"{ctx.namespace}{suffix}.cpp",
            ctx.dest_root / "Code" / "Source" / f"{ctx.namespace}{suffix}Interface.cpp",
            ctx.dest_root / "Source" / f"{ctx.namespace}{suffix}Interface.cpp",
            ctx.dest_root / "Source" / "Tools" / f"{ctx.namespace}{suffix}.cpp",
        ]

        module_path = None
        for candidate in candidates:
            if candidate.is_file():
                module_path = candidate
                break

        if not module_path:
            ctx.log(f"Warning: Could not find {self.module_kind} module file")
            return True

        text = module_path.read_text(encoding="utf-8")
        include_line = f'#include "{self.component_name}.h"'

        if include_line not in text:
            lines = text.splitlines()
            last_include = 0
            for i, line in enumerate(lines):
                if line.strip().startswith("#include"):
                    last_include = i
            lines.insert(last_include + 1, include_line)
            text = "\n".join(lines) + "\n"

        fq_component = f"{ctx.namespace}::{self.component_name}"
        to_insert = f"azrtti_typeid<{fq_component}>()"

        if to_insert in text:
            ctx.log(f"Component already registered in {self.module_kind}")
            return True

        pattern = r'return\s+AZ::ComponentTypeList\s*\{([^}]*)\}'
        match = re.search(pattern, text, flags=re.S)

        if match:
            list_content = match.group(1)

            start = match.start()
            line_start = text.rfind('\n', 0, start)
            if line_start == -1:
                block_indent = ""
            else:
                block_indent = text[line_start + 1:start]

            entry_indent_match = re.search(r'\n(\s+)azrtti_typeid', list_content)
            if entry_indent_match:
                entry_indent = entry_indent_match.group(1)
            else:
                entry_indent = block_indent + "    "

            lines = list_content.split('\n')
            cleaned = [line for line in lines if line.strip()]

            if cleaned and not cleaned[-1].rstrip().endswith(','):
                cleaned[-1] = cleaned[-1].rstrip() + ','

            cleaned.append(f'{entry_indent}{to_insert},')

            new_content = '\n' + '\n'.join(cleaned) + '\n' + block_indent
            new_text = text[:match.start(1)] + new_content + text[match.end(1):]

            module_path.write_text(new_text, encoding="utf-8", newline="\n")
            ctx.log(f"Registered in {self.module_kind} module: {module_path.name}")
        else:
            ctx.log(f"Warning: Could not find GetRequiredSystemComponents in {self.module_kind} module")

        return True
