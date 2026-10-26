#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

import re
from pathlib import Path

from command_plugin import WizardCommand, CommandContext, CommandRegistry


@CommandRegistry.register("register_generic_asset")
class RegisterGenericAssetCommand(WizardCommand):
    """Register asset in DataAssetSystemComponent"""

    def __init__(self, asset_name: str, asset_ext: str = "mydata", asset_group: str = "Other"):
        self.asset_name = asset_name
        self.asset_ext = asset_ext
        self.asset_group = asset_group

    @property
    def name(self) -> str:
        return "register_generic_asset"

    @property
    def description(self) -> str:
        return "Register GenericAssetHandler in DataAssetSystemComponent"

    def execute(self, ctx: CommandContext) -> bool:
        ctx.log(f"Registering GenericAssetHandler for {self.asset_name}...")

        sys_comp_name = f"{ctx.namespace}DataAssetSystemComponent"

        candidates = [
            ctx.dest_root / "Code" / "Source" / f"{sys_comp_name}.cpp",
            ctx.dest_root / "Source" / f"{sys_comp_name}.cpp",
        ]

        cpp_path = None
        for candidate in candidates:
            if candidate.is_file():
                cpp_path = candidate
                break

        if not cpp_path:
            ctx.log(f"Warning: Could not find {sys_comp_name}.cpp")
            return True

        text = cpp_path.read_text(encoding="utf-8")

        # Add include for the asset
        include_line = f'#include "{self.asset_name}.h"'
        if include_line not in text:
            lines = text.splitlines()
            last_include = 0
            for i, line in enumerate(lines):
                if line.strip().startswith("#include") and '"' in line:
                    last_include = i
            lines.insert(last_include + 1, include_line)
            text = "\n".join(lines) + "\n"

        # Find Activate() method
        activate_pattern = r'void\s+' + re.escape(sys_comp_name) + r'::Activate\s*\([^)]*\)\s*\{([^}]*)\}'
        match = re.search(activate_pattern, text, flags=re.S)

        if match:
            body = match.group(1)
            handler_check = f'{self.asset_name}Handler'

            if handler_check not in body:
                indent_match = re.search(r'\n(\s+)(?:auto\*|m_assetHandlers)', body)
                if not indent_match:
                    indent_match = re.search(r'\n(\s+)\S', body)

                base_indent = indent_match.group(1) if indent_match else '        '
                has_handlers = 'Handler' in body or 'm_assetHandlers' in body
                suffix = '\n\n' if has_handlers else '\n'

                registration_block = (
                    f'{base_indent}// Register {self.asset_name}\n'
                    f'{base_indent}auto* {self.asset_name}Handler = aznew AzFramework::GenericAssetHandler<{self.asset_name}>("{self.asset_name}", "{self.asset_group}", "{self.asset_ext}");\n'
                    f'{base_indent}{self.asset_name}Handler->SetAutoBuildAssetToCache(true);\n'
                    f'{base_indent}{self.asset_name}Handler->Register();\n'
                    f'{base_indent}m_assetHandlers.emplace_back({self.asset_name}Handler);{suffix}'
                )

                lines = body.split('\n')
                insert_line = 0
                for i, line in enumerate(lines):
                    if line.strip():
                        insert_line = i
                        break

                if insert_line == 0 and not lines[0].strip():
                    new_body = lines[0] + '\n' + registration_block + '\n'.join(lines[1:])
                else:
                    new_body = '\n'.join(lines[:insert_line]) + '\n' + registration_block + '\n'.join(lines[insert_line:])

                text = text[:match.start(1)] + new_body + text[match.end(1):]
                ctx.log(f"Added GenericAssetHandler registration for {self.asset_name}")

        # Find Reflect() method
        reflect_pattern = r'void\s+' + re.escape(sys_comp_name) + r'::Reflect\s*\([^)]*\)\s*\{([^}]*)\}'
        match = re.search(reflect_pattern, text, flags=re.S)

        if match:
            body = match.group(1)
            reflect_call = f'{self.asset_name}::Reflect(context);'

            if reflect_call not in body:
                indent_match = re.search(r'\n(\s+)\w+::Reflect', body)
                if not indent_match:
                    indent_match = re.search(r'\n(\s+)\S', body)

                base_indent = indent_match.group(1) if indent_match else '        '
                reflect_line = f'{base_indent}{reflect_call}\n'

                lines = body.split('\n')
                insert_line = 0
                for i, line in enumerate(lines):
                    if line.strip():
                        insert_line = i
                        break

                if insert_line == 0 and not lines[0].strip():
                    new_body = lines[0] + '\n' + reflect_line + '\n'.join(lines[1:])
                else:
                    new_body = '\n'.join(lines[:insert_line]) + '\n' + reflect_line + '\n'.join(lines[insert_line:])

                text = text[:match.start(1)] + new_body + text[match.end(1):]
                ctx.log(f"Added Reflect call for {self.asset_name}")

        cpp_path.write_text(text, encoding="utf-8", newline="\n")
        ctx.log(f"Updated {sys_comp_name}.cpp")
        return True
