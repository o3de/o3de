#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

import re
from pathlib import Path
from typing import Optional

from command_plugin import WizardCommand, CommandContext, CommandRegistry


@CommandRegistry.register("register_module_descriptor")
class RegisterModuleDescriptorCommand(WizardCommand):
    """Register component in gem module descriptor"""

    is_registration_command = True

    def __init__(self, component_name: str, module_kind: str = "runtime"):
        self.component_name = component_name
        self.module_kind = module_kind

    @property
    def name(self) -> str:
        return "register_module_descriptor"

    @property
    def description(self) -> str:
        return "Add CreateDescriptor() to gem module interface"

    def execute(self, ctx: CommandContext) -> bool:
        ctx.log(f"Registering '{self.component_name}' in {self.module_kind} module descriptor...")

        # Find module file
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
            for pattern in ["Code/Source/*Module.cpp", "Source/*Module.cpp"]:
                matches = list(ctx.dest_root.glob(pattern))
                if matches:
                    module_path = matches[0]
                    break

        if not module_path:
            ctx.log(f"Warning: Could not find {self.module_kind} module file")
            return True

        text = module_path.read_text(encoding="utf-8")
        include_line = f'#include "{self.component_name}.h"'

        # Add include if not present
        if include_line not in text:
            lines = text.splitlines()
            last_include = 0
            for i, line in enumerate(lines):
                if line.strip().startswith("#include"):
                    last_include = i
            lines.insert(last_include + 1, include_line)
            text = "\n".join(lines) + "\n"

        # Check if descriptor already exists
        descriptor_line = f"{self.component_name}::CreateDescriptor()"

        if descriptor_line in text:
            ctx.log(f"Descriptor already present for {self.component_name}")
            module_path.write_text(text, encoding="utf-8", newline="\n")
            return True

        # Find m_descriptors.insert block
        pattern = r'(m_descriptors\.insert\s*\(\s*m_descriptors\.end\s*\(\s*\)\s*,\s*\{)([^}]*)(\}\s*\)\s*;)'
        match = re.search(pattern, text, flags=re.S)

        if match:
            prefix = match.group(1)
            inner = match.group(2)
            suffix_str = match.group(3)

            start = match.start()
            line_start = text.rfind('\n', 0, start)
            if line_start == -1:
                block_indent = ""
            else:
                block_indent = text[line_start + 1:start]

            entry_indent_match = re.search(r'\n(\s*).*::CreateDescriptor\(\)', inner)
            if entry_indent_match:
                entry_indent = entry_indent_match.group(1)
            else:
                entry_indent = block_indent + "    "

            lines = inner.split('\n')
            cleaned = [line for line in lines if line.strip()]

            if cleaned and not cleaned[-1].rstrip().endswith(','):
                cleaned[-1] = cleaned[-1].rstrip() + ','

            cleaned.append(f"{entry_indent}{descriptor_line},")

            new_inner = '\n' + '\n'.join(cleaned) + '\n' + block_indent
            new_block = prefix + new_inner + suffix_str
            text = text[:match.start()] + new_block + text[match.end():]

            ctx.log(f"Added descriptor for {self.component_name}")
        else:
            ctx.log("Warning: Could not find m_descriptors.insert block")

        module_path.write_text(text, encoding="utf-8", newline="\n")
        ctx.log(f"Updated module: {module_path.name}")
        return True
