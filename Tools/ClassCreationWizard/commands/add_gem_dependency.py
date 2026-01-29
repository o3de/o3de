#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

import re
from pathlib import Path

from command_plugin import WizardCommand, CommandContext, CommandRegistry


@CommandRegistry.register("add_gem_dependency")
class AddGemDependencyCommand(WizardCommand):
    """Add gem dependency to BUILD_DEPENDENCIES"""

    def __init__(self, dependency: str):
        self.dependency = dependency

    @property
    def name(self) -> str:
        return "add_gem_dependency"

    @property
    def description(self) -> str:
        return "Add gem dependency to BUILD_DEPENDENCIES in CMake"

    def execute(self, ctx: CommandContext) -> bool:
        # Guard: don't add a gem as a dependency of itself
        dep_name = self.dependency
        if dep_name.startswith("Gem::"):
            dep_name = dep_name[len("Gem::"):]
        dep_gem = dep_name.split(".")[0]  # Strip .API, .Private, etc.
        if dep_gem == ctx.namespace:
            ctx.log(f"Skipping self-dependency: {self.dependency} (target gem is {ctx.namespace})")
            return True

        if not ctx.build_target:
            ctx.log("Warning: No build target selected")
            return True

        ctx.log(f"Adding dependency '{self.dependency}' to target '{ctx.build_target.name}'...")

        cmake_path = ctx.build_target.file
        if not cmake_path.is_file():
            ctx.log(f"Warning: CMake file not found: {cmake_path}")
            return True

        text = cmake_path.read_text(encoding="utf-8")

        macro_pat = r'(?:o3de_add_target|ly_add_target)\s*\((?P<body>.*?)\)\s*'

        for match in re.finditer(macro_pat, text, flags=re.S | re.M):
            body = match.group('body')

            name_match = re.search(r'\bNAME\s+([^\s\)]+)', body)
            if not name_match:
                continue

            name_in_cmake = name_match.group(1).strip('"\'')

            is_match = False
            if name_in_cmake == ctx.build_target.raw_name:
                is_match = True
            elif '${' in name_in_cmake and '.' in ctx.build_target.name:
                if '.' in name_in_cmake:
                    cmake_suffix = name_in_cmake.split('.', 1)[1]
                    target_suffix = ctx.build_target.name.split('.', 1)[1] if '.' in ctx.build_target.name else ''
                    if cmake_suffix == target_suffix:
                        is_match = True

            if not is_match:
                continue

            if self.dependency in body:
                ctx.log(f"Dependency already present: {self.dependency}")
                return True

            lines = body.splitlines()

            deps_idx = None
            for i, line in enumerate(lines):
                if re.match(r'^\s*BUILD_DEPENDENCIES\b', line):
                    deps_idx = i
                    break

            base_indent = '    '
            for line in lines:
                if line.strip() and not line.strip().startswith('#'):
                    base_indent = re.match(r'(\s*)', line).group(1)
                    break

            if deps_idx is None:
                lines.append('')
                lines.append(f'{base_indent}BUILD_DEPENDENCIES')
                lines.append(f'{base_indent}    PRIVATE')
                lines.append(f'{base_indent}        {self.dependency}')
            else:
                private_idx = None
                for i in range(deps_idx + 1, len(lines)):
                    if re.match(r'^\s*PRIVATE\b', lines[i]):
                        private_idx = i
                    if re.match(r'^\s*[A-Z_]+\b', lines[i]) and not re.match(r'^\s*(PRIVATE|PUBLIC|INTERFACE)\b', lines[i]):
                        break

                if private_idx is None:
                    indent = re.match(r'(\s*)', lines[deps_idx]).group(1)
                    lines.insert(deps_idx + 1, f'{indent}    PRIVATE')
                    lines.insert(deps_idx + 2, f'{indent}        {self.dependency}')
                else:
                    indent = re.match(r'(\s*)', lines[private_idx]).group(1)
                    lines.insert(private_idx + 1, f'{indent}    {self.dependency}')

            new_body = '\n'.join(lines) + '\n'
            new_text = text[:match.start()] + match.group(0).replace(body, new_body) + text[match.end():]

            cmake_path.write_text(new_text, encoding='utf-8', newline='\n')
            ctx.log(f"Added dependency {self.dependency} to {ctx.build_target.name}")
            return True

        ctx.log(f"Warning: Could not find target block for {ctx.build_target.name}")
        return True
