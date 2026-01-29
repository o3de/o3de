#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import traceback
from pathlib import Path
from typing import Optional, List, Dict, Tuple, Any, Callable, Type
from abc import ABC, abstractmethod
from dataclasses import dataclass, field

from PySide6.QtCore import Qt, Signal, QTimer
from PySide6.QtWidgets import (
    QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
    QLabel, QLineEdit, QPushButton, QComboBox, QCheckBox,
    QTextEdit, QGroupBox, QFileDialog, QMessageBox, QFormLayout
)
from PySide6.QtGui import QIcon


# ============================================================================
# Constants
# ============================================================================

COMMENT_FILE_GLOBS = ("**/*.h", "**/*.hpp", "**/*.c", "**/*.cpp", "**/*.inl")

CPP_KEYWORDS = {
    'alignas', 'alignof', 'and', 'and_eq', 'asm', 'auto',
    'bitand', 'bitor', 'bool', 'break', 'case', 'catch',
    'char', 'char8_t', 'char16_t', 'char32_t', 'class', 'compl',
    'concept', 'const', 'consteval', 'constexpr', 'const_cast', 'continue',
    'co_await', 'co_return', 'co_yield', 'decltype', 'default', 'delete',
    'do', 'double', 'dynamic_cast', 'else', 'enum', 'explicit',
    'export', 'extern', 'false', 'float', 'for', 'friend',
    'goto', 'if', 'inline', 'int', 'long', 'mutable',
    'namespace', 'new', 'noexcept', 'not', 'not_eq', 'nullptr',
    'operator', 'or', 'or_eq', 'private', 'protected', 'public',
    'register', 'reinterpret_cast', 'requires', 'return', 'short', 'signed',
    'sizeof', 'static', 'static_assert', 'static_cast', 'struct', 'switch',
    'template', 'this', 'thread_local', 'throw', 'true', 'try',
    'typedef', 'typeid', 'typename', 'union', 'unsigned', 'using',
    'virtual', 'void', 'volatile', 'wchar_t', 'while', 'xor', 'xor_eq'
}

# Legacy constants removed - now dynamically loaded from template.json class_wizard blocks
# See WizardTemplateScanner for template discovery


# ============================================================================
# Command System Data Classes
# ============================================================================

@dataclass
class InputVarDef:
    """Definition of a template input variable"""
    var_name: str
    input_type: str  # "text", "toggle", "dropdown"
    title: str
    default_value: Any = ""
    description: str = ""
    required: bool = False
    options: List[str] = field(default_factory=list)  # For dropdown type


@dataclass
class CommandDef:
    """Definition of a process command from template.json"""
    command: str
    args: Dict[str, str] = field(default_factory=dict)
    condition: Optional[str] = None


@dataclass
class CopyFileDef:
    """Definition of a file to copy with optional condition"""
    file: str
    is_templated: bool = True
    condition: Optional[str] = None


@dataclass
class WizardTemplate:
    """Parsed class_wizard configuration from a template"""
    template_name: str
    template_path: Path
    display_name: str
    class_name: str
    component_suffix: str
    description: str = ""
    input_vars: List[InputVarDef] = field(default_factory=list)
    process_commands: List[CommandDef] = field(default_factory=list)
    copy_files: List[CopyFileDef] = field(default_factory=list)


# ============================================================================
# Command System Infrastructure
# ============================================================================

class CommandContext:
    """Execution context passed to all commands"""

    def __init__(self,
                 dest_root: Path,
                 namespace: str,
                 component_name: str,
                 build_target: Optional['CMakeTarget'],
                 variables: Dict[str, Any],
                 logger: Callable[[str], None],
                 engine_path: Path):
        self.dest_root = dest_root
        self.namespace = namespace
        self.component_name = component_name
        self.build_target = build_target
        self.variables = variables
        self.logger = logger
        self.engine_path = engine_path

    def log(self, message: str):
        """Log a message using the provided logger"""
        self.logger(message)


class WizardCommand(ABC):
    """Abstract base class for all wizard commands"""

    @abstractmethod
    def execute(self, ctx: CommandContext) -> bool:
        """Execute the command. Returns True on success."""
        pass

    @property
    @abstractmethod
    def name(self) -> str:
        """Return the command name for logging"""
        pass


class CommandRegistry:
    """Registry of available wizard commands"""

    _commands: Dict[str, Type[WizardCommand]] = {}

    @classmethod
    def register(cls, name: str):
        """Decorator to register a command class"""
        def decorator(command_class: Type[WizardCommand]):
            cls._commands[name] = command_class
            return command_class
        return decorator

    @classmethod
    def get(cls, name: str) -> Optional[Type[WizardCommand]]:
        """Get a command class by name"""
        return cls._commands.get(name)

    @classmethod
    def create(cls, name: str, args: Dict[str, str]) -> Optional[WizardCommand]:
        """Create a command instance by name with given arguments"""
        command_class = cls._commands.get(name)
        if command_class is None:
            return None
        return command_class(**args)

    @classmethod
    def list_commands(cls) -> List[str]:
        """List all registered command names"""
        return list(cls._commands.keys())


class VariableResolver:
    """Resolves ${variable} references in strings"""

    def __init__(self, base_vars: Dict[str, Any], input_values: Dict[str, Any]):
        self.variables = {**base_vars, **input_values}

    def resolve(self, text: str) -> str:
        """Replace all ${var} references with their values"""
        if not text or not isinstance(text, str):
            return text

        pattern = r'\$\{(\w+)\}'

        def replace(match):
            var_name = match.group(1)
            value = self.variables.get(var_name)
            if value is not None:
                return str(value)
            return match.group(0)  # Keep original if not found

        return re.sub(pattern, replace, text)

    def resolve_dict(self, d: Dict[str, str]) -> Dict[str, str]:
        """Resolve all values in a dictionary"""
        return {k: self.resolve(v) for k, v in d.items()}

    def evaluate_condition(self, condition: Optional[str]) -> bool:
        """
        Evaluate a condition expression.

        Supports:
        - None or empty: returns True
        - "var_name": returns bool(variables[var_name])
        - "!var_name": returns not bool(variables[var_name])
        - "${var} == 'value'": string comparison
        - "${var} != 'value'": string non-equality
        """
        if not condition:
            return True

        condition = condition.strip()

        # Simple negation: "!var_name"
        if condition.startswith('!'):
            var_name = condition[1:].strip()
            value = self.variables.get(var_name, False)
            return not bool(value)

        # Simple variable check: "var_name"
        if condition in self.variables:
            return bool(self.variables[condition])

        # Expression with comparison operators
        resolved = self.resolve(condition)

        # Handle == comparison
        if '==' in resolved:
            parts = resolved.split('==', 1)
            if len(parts) == 2:
                left = parts[0].strip().strip("'\"")
                right = parts[1].strip().strip("'\"")
                return left == right

        # Handle != comparison
        if '!=' in resolved:
            parts = resolved.split('!=', 1)
            if len(parts) == 2:
                left = parts[0].strip().strip("'\"")
                right = parts[1].strip().strip("'\"")
                return left != right

        # Handle > comparison (numeric)
        if '>' in resolved and '>=' not in resolved:
            parts = resolved.split('>', 1)
            if len(parts) == 2:
                try:
                    left = float(parts[0].strip())
                    right = float(parts[1].strip())
                    return left > right
                except ValueError:
                    return False

        # Handle < comparison (numeric)
        if '<' in resolved and '<=' not in resolved:
            parts = resolved.split('<', 1)
            if len(parts) == 2:
                try:
                    left = float(parts[0].strip())
                    right = float(parts[1].strip())
                    return left < right
                except ValueError:
                    return False

        # Default: treat as truthy check on resolved value
        return bool(resolved)


# ============================================================================
# Wizard Command Implementations
# ============================================================================

@CommandRegistry.register("register_file_list")
class RegisterFileListCommand(WizardCommand):
    """Add component .h/.cpp files to CMake FILES_CMAKE"""

    def __init__(self, component_name: str):
        self.component_name = component_name

    @property
    def name(self) -> str:
        return "register_file_list"

    def execute(self, ctx: CommandContext) -> bool:
        ctx.log(f"Registering files for '{self.component_name}'...")

        if not ctx.build_target:
            ctx.log("Warning: No build target selected, skipping file registration")
            return True

        rel_hdr = f"Source/{self.component_name}.h"
        rel_cpp = f"Source/{self.component_name}.cpp"

        cmake_path = ctx.build_target.file

        # If target has FILES_CMAKE includes, update those
        if ctx.build_target.files_cmake_list:
            for files_cmake in ctx.build_target.files_cmake_list:
                include_path = (cmake_path.parent / files_cmake).resolve()

                if include_path.exists():
                    self._update_files_cmake(include_path, rel_hdr, rel_cpp, ctx)
                    ctx.log(f"Updated: {files_cmake}")
                    return True

        # Otherwise append target_sources directly to CMakeLists.txt
        cmake_text = cmake_path.read_text(encoding="utf-8")

        append = (
            f"\n# Added by Class Wizard\n"
            f"target_sources({ctx.build_target.name} PRIVATE\n"
            f"    {rel_hdr}\n"
            f"    {rel_cpp}\n"
            f")\n"
        )

        if append not in cmake_text:
            cmake_text = cmake_text.rstrip() + append
            cmake_path.write_text(cmake_text, encoding="utf-8", newline="\n")
            ctx.log(f"Added target_sources block to {cmake_path.name}")

        return True

    def _update_files_cmake(self, files_cmake_path: Path, rel_hdr: str, rel_cpp: str, ctx: CommandContext):
        """Update a FILES_CMAKE include file"""
        if files_cmake_path.exists():
            text = files_cmake_path.read_text(encoding="utf-8")
        else:
            text = "set(FILES\n)\n"

        # Find set(FILES ...) block
        match = re.search(r'set\s*\(\s*FILES\b(.*?)(\))', text, flags=re.S | re.M)
        if match:
            end_pos = match.end(1)

            # Check if files already exist
            if rel_hdr not in text:
                text = text[:end_pos] + f"    {rel_hdr}\n" + text[end_pos:]

            # Re-find the match after first insertion
            match = re.search(r'set\s*\(\s*FILES\b(.*?)(\))', text, flags=re.S | re.M)
            if match:
                end_pos = match.end(1)
                if rel_cpp not in text:
                    text = text[:end_pos] + f"    {rel_cpp}\n" + text[end_pos:]
        else:
            # Create new set(FILES) block
            text = text.rstrip() + f"\nset(FILES\n    {rel_hdr}\n    {rel_cpp}\n)\n"

        files_cmake_path.write_text(text, encoding="utf-8", newline="\n")


@CommandRegistry.register("register_module_descriptor")
class RegisterModuleDescriptorCommand(WizardCommand):
    """Register component in gem module descriptor"""

    def __init__(self, component_name: str, module_kind: str = "runtime"):
        self.component_name = component_name
        self.module_kind = module_kind

    @property
    def name(self) -> str:
        return "register_module_descriptor"

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
            # Search for any Module.cpp
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

            # Compute block indent
            start = match.start()
            line_start = text.rfind('\n', 0, start)
            if line_start == -1:
                block_indent = ""
            else:
                block_indent = text[line_start + 1:start]

            # Detect indent from existing entries
            entry_indent_match = re.search(r'\n(\s*).*::CreateDescriptor\(\)', inner)
            if entry_indent_match:
                entry_indent = entry_indent_match.group(1)
            else:
                entry_indent = block_indent + "    "

            # Clean and rebuild
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


@CommandRegistry.register("register_system_component")
class RegisterSystemComponentCommand(WizardCommand):
    """Register system component in module's required components list"""

    def __init__(self, component_name: str, module_kind: str = "runtime"):
        self.component_name = component_name
        self.module_kind = module_kind

    @property
    def name(self) -> str:
        return "register_system_component"

    def execute(self, ctx: CommandContext) -> bool:
        ctx.log(f"Registering system component '{self.component_name}' in {self.module_kind} module...")

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
            ctx.log(f"Warning: Could not find {self.module_kind} module file")
            return True

        text = module_path.read_text(encoding="utf-8")
        include_line = f'#include "{self.component_name}.h"'

        # Add include
        if include_line not in text:
            lines = text.splitlines()
            last_include = 0
            for i, line in enumerate(lines):
                if line.strip().startswith("#include"):
                    last_include = i
            lines.insert(last_include + 1, include_line)
            text = "\n".join(lines) + "\n"

        # Add to GetRequiredSystemComponents
        fq_component = f"{ctx.namespace}::{self.component_name}"
        to_insert = f"azrtti_typeid<{fq_component}>()"

        if to_insert in text:
            ctx.log(f"Component already registered in {self.module_kind}")
            return True

        # Find ComponentTypeList
        pattern = r'return\s+AZ::ComponentTypeList\s*\{([^}]*)\}'
        match = re.search(pattern, text, flags=re.S)

        if match:
            list_content = match.group(1)

            # Compute indent
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


@CommandRegistry.register("register_interface_header")
class RegisterInterfaceHeaderCommand(WizardCommand):
    """Register interface header to INTERFACE/API target"""

    def __init__(self, component_name: str):
        self.component_name = component_name

    @property
    def name(self) -> str:
        return "register_interface_header"

    def execute(self, ctx: CommandContext) -> bool:
        if not ctx.build_target:
            ctx.log("Warning: No build target selected for interface header")
            return True

        # Check if interface header exists
        interface_hdr_path = ctx.dest_root / "Include" / ctx.namespace / f"{self.component_name}Interface.h"

        if not interface_hdr_path.exists():
            ctx.log(f"No interface header found at {interface_hdr_path}, skipping")
            return True

        ctx.log(f"Registering interface header for '{self.component_name}'...")

        # Find best target for interface (INTERFACE > .API > fallback)
        cmake_dir = ctx.build_target.file.parent
        all_targets = CMakeAnalyzer.scan_targets(cmake_dir, ctx.namespace)

        interface_target = self._find_interface_target(all_targets, ctx.namespace, ctx.build_target)

        rel_hdr = f"Include/{ctx.namespace}/{self.component_name}Interface.h"

        # Register to the target
        if interface_target.kind == "o3de_add_target" and interface_target.files_cmake_list:
            for files_cmake in interface_target.files_cmake_list:
                include_path = (interface_target.file.parent / files_cmake).resolve()
                if include_path.exists():
                    self._update_interface_files_cmake(include_path, rel_hdr, ctx)
                    return True

        self._add_interface_to_target(interface_target.file, interface_target, rel_hdr, ctx)
        return True

    def _find_interface_target(self, targets: List['CMakeTarget'], gem_name: str, fallback: 'CMakeTarget') -> 'CMakeTarget':
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

    def _add_interface_to_target(self, cmake_path: Path, target: 'CMakeTarget', rel_hdr: str, ctx: CommandContext):
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

    def execute(self, ctx: CommandContext) -> bool:
        ctx.log(f"Registering GenericAssetHandler for {self.asset_name}...")

        sys_comp_name = f"{ctx.namespace}DataAssetSystemComponent"

        # Find the system component .cpp file
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


@CommandRegistry.register("register_asset_setreg")
class RegisterAssetSetregCommand(WizardCommand):
    """Update .setreg with asset config"""

    def __init__(self, asset_name: str, asset_ext: str = "mydata"):
        self.asset_name = asset_name
        self.asset_ext = asset_ext

    @property
    def name(self) -> str:
        return "register_asset_setreg"

    def execute(self, ctx: CommandContext) -> bool:
        ctx.log(f"Updating setreg for {self.asset_name}...")

        # Find setreg file
        setreg_name = f"{ctx.namespace}DataAssetRegistry.setreg"
        setreg_path = self._find_setreg(ctx, setreg_name)

        if not setreg_path:
            ctx.log(f"Warning: Could not find setreg file {setreg_name}")
            return True

        # Extract GUID
        guid = self._extract_asset_guid(ctx, self.asset_name)
        if not guid:
            guid = "{00000000-0000-0000-0000-000000000000}"
            ctx.log("Warning: Using placeholder GUID")

        # Load setreg
        try:
            data = json.loads(setreg_path.read_text(encoding="utf-8"))
        except Exception:
            data = {"Amazon": {"AssetProcessor": {"Settings": {}}}}

        # Ensure structure
        if "Amazon" not in data:
            data["Amazon"] = {}
        if "AssetProcessor" not in data["Amazon"]:
            data["Amazon"]["AssetProcessor"] = {}
        if "Settings" not in data["Amazon"]["AssetProcessor"]:
            data["Amazon"]["AssetProcessor"]["Settings"] = {}

        settings = data["Amazon"]["AssetProcessor"]["Settings"]

        # Update RC entry
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
            # Also check if dest_root is Gem folder
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
        # Pattern for GUID with or without curly braces, with or without quotes
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

            # AZ_RTTI - more permissive pattern to handle quotes and braces
            rtti_match = re.search(
                rf'AZ_RTTI\s*\(\s*{re.escape(asset_name)}\s*,\s*["\']?({guid_pattern})["\']?',
                text
            )
            if rtti_match:
                guid = rtti_match.group(1)
                # Ensure GUID has curly braces
                if not guid.startswith('{'):
                    guid = '{' + guid + '}'
                ctx.log(f"  Found GUID via AZ_RTTI: {guid}")
                return guid

            # AZ_TYPE_INFO
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

            # Fallback: Just search for any GUID near the class name
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


@CommandRegistry.register("add_gem_dependency")
class AddGemDependencyCommand(WizardCommand):
    """Add gem dependency to BUILD_DEPENDENCIES"""

    def __init__(self, dependency: str):
        self.dependency = dependency

    @property
    def name(self) -> str:
        return "add_gem_dependency"

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

        # Find target blocks
        macro_pat = r'(?:o3de_add_target|ly_add_target)\s*\((?P<body>.*?)\)\s*'

        for match in re.finditer(macro_pat, text, flags=re.S | re.M):
            body = match.group('body')

            name_match = re.search(r'\bNAME\s+([^\s\)]+)', body)
            if not name_match:
                continue

            name_in_cmake = name_match.group(1).strip('"\'')

            # Match target
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

            # Find BUILD_DEPENDENCIES
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


@CommandRegistry.register("copy_setreg")
class CopySetregCommand(WizardCommand):
    """Copy setreg file to Registry folder"""

    def __init__(self, setreg_name: str):
        self.setreg_name = setreg_name

    @property
    def name(self) -> str:
        return "copy_setreg"

    def execute(self, ctx: CommandContext) -> bool:
        ctx.log(f"Handling setreg file: {self.setreg_name}...")

        # Determine target Registry directory
        dest_root = ctx.dest_root.resolve()

        if dest_root.name == "Gem":
            project_json = dest_root.parent / "project.json"
            if project_json.is_file():
                target_dir = dest_root / "Registry"
            else:
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


@CommandRegistry.register("copy_file")
class CopyFileCommand(WizardCommand):
    """Copy a file to destination"""

    def __init__(self, source: str, dest: str):
        self.source = source
        self.dest = dest

    @property
    def name(self) -> str:
        return "copy_file"

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


@CommandRegistry.register("replace_text")
class ReplaceTextCommand(WizardCommand):
    """Replace text in a generated file. Supports literal replacement or replacement from an input variable."""

    def __init__(self, component_name: str, text_to_replace: str,
                 replacement: str = "", replacement_var: str = ""):
        self.component_name = component_name
        self.text_to_replace = text_to_replace
        self.replacement = replacement
        self.replacement_var = replacement_var

    @property
    def name(self) -> str:
        return "replace_text"

    def execute(self, ctx: CommandContext) -> bool:
        # Determine replacement value
        if self.replacement_var:
            repl_value = ctx.variables.get(self.replacement_var, "")
            if repl_value is None:
                repl_value = ""
        else:
            repl_value = self.replacement

        # Locate the target file in dest_root
        target = self._find_file(ctx.dest_root, self.component_name)
        if not target:
            ctx.log(f"Warning: File not found for replace_text: {self.component_name}")
            return True

        text = target.read_text(encoding="utf-8")
        if self.text_to_replace not in text:
            ctx.log(f"Warning: Text not found in {target.name}: {self.text_to_replace!r}")
            return True

        text = text.replace(self.text_to_replace, repl_value)
        target.write_text(text, encoding="utf-8")
        ctx.log(f"Replaced text in {target.name}")
        return True

    @staticmethod
    def _find_file(dest_root: Path, filename: str) -> Optional[Path]:
        """Search for a file in common subdirectories"""
        for subdir in ["Source", "Include", "Code/Source", "Code/Include", ""]:
            candidate = dest_root / subdir / filename if subdir else dest_root / filename
            if candidate.is_file():
                return candidate
        # Fallback: recursive search
        for match in dest_root.rglob(filename):
            if match.is_file():
                return match
        return None


# ============================================================================
# Template Scanner
# ============================================================================

class WizardTemplateScanner:
    """Discovers templates with class_wizard definitions"""

    def __init__(self, logger: Optional[Callable[[str], None]] = None):
        self.logger = logger or print

    def log(self, message: str):
        self.logger(message)

    def scan(self, engine_path: Path, project_path: Optional[Path] = None) -> List[WizardTemplate]:
        """
        Scan engine, project, and gem Templates folders for class_wizard enabled templates.

        Scan order:
            1. Engine Templates/
            2. Project Templates/
            3. Gem Templates/ (for each gem used by the project, resolved via o3de manifest)

        Returns:
            List of WizardTemplate objects
        """
        templates = []
        scanned_dirs = set()  # Avoid scanning the same directory twice

        # Scan engine templates
        engine_templates = engine_path / "Templates"
        if engine_templates.is_dir():
            scanned_dirs.add(engine_templates.resolve())
            templates.extend(self._scan_directory(engine_templates))

        # Scan project templates if provided
        if project_path:
            project_templates = project_path / "Templates"
            if project_templates.is_dir() and project_templates.resolve() not in scanned_dirs:
                scanned_dirs.add(project_templates.resolve())
                templates.extend(self._scan_directory(project_templates))

        # Scan gem templates
        if project_path:
            gem_dirs = self._resolve_project_gem_paths(project_path)
            for gem_dir in gem_dirs:
                gem_templates = gem_dir / "Templates"
                if gem_templates.is_dir() and gem_templates.resolve() not in scanned_dirs:
                    scanned_dirs.add(gem_templates.resolve())
                    templates.extend(self._scan_directory(gem_templates))

        # Sort by display name
        templates.sort(key=lambda t: t.display_name.lower())

        return templates

    def _resolve_project_gem_paths(self, project_path: Path) -> List[Path]:
        """Resolve gem directory paths for a project using the o3de manifest.

        Reads the project's gem_names from project.json, then resolves each
        gem name to an actual filesystem path via the o3de manifest's
        external_subdirectories list.
        """
        gem_paths = []

        # Read project gem names
        project_json = project_path / "project.json"
        if not project_json.is_file():
            return gem_paths

        try:
            project_data = json.loads(project_json.read_text(encoding="utf-8"))
        except Exception:
            return gem_paths

        gem_names_raw = project_data.get("gem_names", [])
        # Strip version specifiers (e.g. "GS_Cinematics==1.0.0" -> "GS_Cinematics")
        project_gem_names = set()
        for name in gem_names_raw:
            clean = name.split("==")[0].split(">=")[0].split("<=")[0].split("~=")[0].strip()
            project_gem_names.add(clean.lower())

        # Include project-local external_subdirectories (e.g. "Gem")
        for ext_sub in project_data.get("external_subdirectories", []):
            ext_path = (project_path / ext_sub).resolve()
            if ext_path.is_dir():
                gem_paths.append(ext_path)

        # Read o3de manifest
        manifest_path = Path.home() / ".o3de" / "o3de_manifest.json"
        if not manifest_path.is_file():
            return gem_paths

        try:
            manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        except Exception:
            return gem_paths

        # Check each external subdirectory in the manifest
        already_resolved = {p.resolve() for p in gem_paths}
        for ext_dir_str in manifest.get("external_subdirectories", []):
            ext_dir = Path(ext_dir_str)
            if not ext_dir.is_dir():
                continue

            # Read gem.json to get the gem's registered name
            gem_json = ext_dir / "gem.json"
            if not gem_json.is_file():
                continue

            try:
                gem_data = json.loads(gem_json.read_text(encoding="utf-8"))
            except Exception:
                continue

            gem_name = gem_data.get("gem_name", "")
            if gem_name.lower() in project_gem_names:
                resolved = ext_dir.resolve()
                if resolved not in already_resolved:
                    already_resolved.add(resolved)
                    gem_paths.append(ext_dir)

        return gem_paths

    def _scan_directory(self, templates_dir: Path) -> List[WizardTemplate]:
        """Scan a templates directory for class_wizard enabled templates"""
        templates = []

        for template_dir in templates_dir.iterdir():
            if not template_dir.is_dir():
                continue

            template_json = template_dir / "template.json"
            if not template_json.is_file():
                continue

            template = self._parse_template(template_json)
            if template:
                templates.append(template)

        return templates

    def _parse_template(self, template_json: Path) -> Optional[WizardTemplate]:
        """Parse a template.json file and extract class_wizard configuration"""
        try:
            data = json.loads(template_json.read_text(encoding="utf-8"))
        except Exception as e:
            self.log(f"Warning: Failed to parse {template_json}: {e}")
            return None

        # Check for class_wizard block
        wizard_config = data.get("class_wizard")
        if not wizard_config:
            return None

        # Parse input variables
        input_vars = []
        for var_def in wizard_config.get("input_vars", []):
            input_vars.append(InputVarDef(
                var_name=var_def.get("var_name", ""),
                input_type=var_def.get("input_type", "text"),
                title=var_def.get("title", ""),
                default_value=var_def.get("default_value", ""),
                description=var_def.get("description", ""),
                required=var_def.get("required", False),
                options=var_def.get("options", [])
            ))

        # Parse process commands
        process_commands = []
        for cmd_def in wizard_config.get("process_commands", []):
            process_commands.append(CommandDef(
                command=cmd_def.get("command", ""),
                args=cmd_def.get("args", {}),
                condition=cmd_def.get("condition")
            ))

        # Parse copyFiles with conditions
        copy_files = []
        for file_def in data.get("copyFiles", []):
            copy_files.append(CopyFileDef(
                file=file_def.get("file", ""),
                is_templated=file_def.get("isTemplated", True),
                condition=file_def.get("condition")
            ))

        return WizardTemplate(
            template_name=data.get("template_name", template_json.parent.name),
            template_path=template_json.parent,
            display_name=wizard_config.get("display_name", data.get("display_name", "")),
            class_name=wizard_config.get("class_name", ""),
            component_suffix=wizard_config.get("component_suffix", "Component"),
            description=wizard_config.get("description", data.get("summary", "")),
            input_vars=input_vars,
            process_commands=process_commands,
            copy_files=copy_files
        )


# ============================================================================
# Validation Utilities
# ============================================================================

class ValidationError(Exception):
    """Custom exception for validation failures"""
    pass


def validate_component_name(name: str) -> Tuple[bool, str]:
    """
    Validate a component name according to C++ naming rules.
    
    Returns:
        Tuple of (is_valid, error_message)
    """
    if not name:
        return False, "Component name cannot be empty"
    
    invalid_chars = '*?+-,;=&%$`"\'/\\[]{}~#|<>!^@()#: \t\n\r\f\v'
    if invalid := next((c for c in invalid_chars if c in name), None):
        return False, f"Name contains invalid character: {invalid}"
    
    if not (name[0].isalpha() or name[0] == '_'):
        return False, "Name must start with a letter or underscore"
    
    if name.startswith('__'):
        return False, "Name cannot start with double underscore (reserved)"
    
    if name.startswith('_') and len(name) > 1 and name[1].isupper():
        return False, "Name cannot start with underscore followed by uppercase (reserved)"
    
    if name in CPP_KEYWORDS:
        return False, f"'{name}' is a C++ keyword"
    
    return True, ""


def validate_path(path_str: str, must_exist: bool = True) -> Path:
    """Validate and return a Path object"""
    try:
        path = Path(os.path.expanduser(path_str)).resolve()
        if must_exist and not path.exists():
            raise ValidationError(f"Path does not exist: {path}")
        if must_exist and not path.is_dir():
            raise ValidationError(f"Not a directory: {path}")
        return path
    except Exception as e:
        raise ValidationError(f"Invalid path: {e}")


def validate_engine_path(path_str: str) -> Path:
    """Validate an O3DE engine path"""
    engine_path = validate_path(path_str)
    if not (engine_path / "engine.json").exists():
        raise ValidationError(
            f"Not a valid O3DE engine directory: {engine_path}\n"
            "engine.json file not found"
        )
    return engine_path


# ============================================================================
# CMake Analysis
# ============================================================================

class CMakeTarget:
    """Represents a CMake build target with its metadata"""
    
    def __init__(self, name: str, raw_name: str, kind: str, 
                 file: Path, files_cmake_list: List[str]):
        self.name = name
        self.raw_name = raw_name
        self.kind = kind
        self.file = file
        self.files_cmake_list = files_cmake_list
    
    def __repr__(self):
        return f"CMakeTarget({self.name}, {self.kind}, {self.file.name})"


class CMakeAnalyzer:
    """Analyzes CMake files to discover build targets"""
    
    LY_MACRO_PAT = re.compile(
        r'(?:o3de_add_target|ly_add_target)\s*\((?P<body>.*?)\)\s*',
        re.S | re.M
    )
    ADD_LIB_PAT = re.compile(r'add_library\s*\(\s*(?P<name>[^\s\)]+)')
    ADD_EXE_PAT = re.compile(r'add_executable\s*\(\s*(?P<name>[^\s\)]+)')
    
    @staticmethod
    def resolve_target_name(raw_name: str, gem_name: str) -> str:
        """Resolve CMake variables in target names"""
        raw = raw_name.strip().strip('"\'')
        mapping = {
            '${GemName}': gem_name,
            '${gem_name}': gem_name,
            '${Name}': gem_name,
            '${GEM_NAME}': gem_name,
        }
        result = raw
        for var, value in mapping.items():
            result = result.replace(var, value)
        return result
    
    @staticmethod
    def extract_files_cmake_list(body: str) -> List[str]:
        """Extract FILES_CMAKE entries from a macro body"""
        files = []
        pattern = re.compile(
            r'\bFILES_CMAKE\b(?P<section>.*?)(?=^\s*[A-Z_]+\b|\Z)',
            re.S | re.M
        )
        for match in pattern.finditer(body):
            section = match.group('section')
            for tok in re.findall(r'"([^"]+)"|([^\s"\)]+)', section):
                token = tok[0] or tok[1]
                if token and token not in files:
                    files.append(token)
        return files
    
    @classmethod
    def scan_targets(cls, gem_path: Path, gem_name: str) -> List[CMakeTarget]:
        """
        Scan a gem directory for CMake targets.
        
        Returns:
            List of CMakeTarget objects found
        """
        results = []
        
        if not gem_path.is_dir():
            return results
        
        cmake_files = list(gem_path.rglob("CMakeLists.txt"))
        
        for cmake_file in cmake_files:
            try:
                text = cmake_file.read_text(encoding="utf-8")
            except Exception:
                continue
            
            # Parse o3de_add_target / ly_add_target
            for match in cls.LY_MACRO_PAT.finditer(text):
                body = match.group("body") or ""
                name_match = re.search(r'\bNAME\s+(".*?"|[^\s\)]+)', body)
                if not name_match:
                    continue
                
                raw_name = name_match.group(1).strip('"\'')
                resolved = cls.resolve_target_name(raw_name, gem_name)
                files_list = cls.extract_files_cmake_list(body)
                
                results.append(CMakeTarget(
                    name=resolved,
                    raw_name=raw_name,
                    kind="o3de_add_target",
                    file=cmake_file,
                    files_cmake_list=files_list
                ))
            
            # Parse add_library
            for match in cls.ADD_LIB_PAT.finditer(text):
                raw = match.group("name")
                resolved = cls.resolve_target_name(raw, gem_name)
                results.append(CMakeTarget(
                    name=resolved,
                    raw_name=raw,
                    kind="add_library",
                    file=cmake_file,
                    files_cmake_list=[]
                ))
            
            # Parse add_executable
            for match in cls.ADD_EXE_PAT.finditer(text):
                raw = match.group("name")
                resolved = cls.resolve_target_name(raw, gem_name)
                results.append(CMakeTarget(
                    name=resolved,
                    raw_name=raw,
                    kind="add_executable",
                    file=cmake_file,
                    files_cmake_list=[]
                ))
        
        # Deduplicate by (name, file)
        seen = set()
        unique = []
        for target in results:
            key = (target.name, str(target.file))
            if key not in seen:
                seen.add(key)
                unique.append(target)
        
        unique.sort(key=lambda t: t.name.lower())
        return unique


# ============================================================================
# Gem Discovery
# ============================================================================

class GemInfo:
    """Information about an O3DE gem"""
    
    def __init__(self, name: str, path: Path):
        self.name = name
        self.path = path
    
    def __repr__(self):
        return f"GemInfo({self.name}, {self.path})"


class GemDiscovery:
    """Discovers gems in an O3DE project"""
    
    @staticmethod
    def get_enabled_gems(engine_path: Path, project_path: Path,
                        include_dependencies: bool = True) -> List[GemInfo]:
        """
        Get list of enabled gems for a project, excluding engine-internal gems.
        
        Returns:
            List of GemInfo objects
        """
        pkg_root = engine_path / "scripts" / "o3de"
        sys.path.insert(0, str(pkg_root))
        
        try:
            from o3de import manifest
            mapping = manifest.get_project_enabled_gems(
                project_path,
                include_dependencies=include_dependencies
            ) or {}
        finally:
            if sys.path and sys.path[0] == str(pkg_root):
                sys.path.pop(0)
        
        engine_root = engine_path.resolve()
        gems = []
        
        for namespec, gem_path_str in mapping.items():
            gem_path = Path(gem_path_str).resolve()
            
            # Skip engine-internal gems
            try:
                gem_path.relative_to(engine_root)
                continue
            except ValueError:
                pass
            
            # Extract display name from gem.json
            name = namespec
            gem_json = gem_path / "gem.json"
            if gem_json.is_file():
                try:
                    data = json.loads(gem_json.read_text(encoding="utf-8"))
                    name = data.get("gem_name") or data.get("display_name") or namespec
                except Exception:
                    pass
            
            gems.append(GemInfo(name, gem_path))
        
        gems.sort(key=lambda g: g.name.lower())
        return gems


# ============================================================================
# Component Creation Engine
# ============================================================================

class ComponentCreator:
    """Handles the component creation workflow"""

    def __init__(self, engine_path: Path, logger=None):
        self.engine_path = engine_path
        self.logger = logger or print
        self.template_scanner = WizardTemplateScanner(logger=self.log)

    def log(self, message: str):
        """Log a message"""
        self.logger(message)

    def create_component(self, config: Dict[str, Any]) -> bool:
        """Create a component with the given configuration.

        Requires 'wizard_template' (WizardTemplate) in config for command-driven processing.
        All template-specific behavior is defined in the template's class_wizard block.
        """
        wizard_template = config.get('wizard_template')

        if not wizard_template or not isinstance(wizard_template, WizardTemplate):
            raise ValueError(
                "wizard_template is required in config. "
                "Ensure template.json contains a 'class_wizard' configuration block."
            )

        return self._create_component_command_driven(config, wizard_template)

    def _create_component_command_driven(self, config: Dict[str, Any],
                                          template: WizardTemplate) -> bool:
        """Create component using command-driven processing"""
        self.log("Starting component creation (command-driven)...")
        self.log(f"Using template: {template.display_name}")

        stage_dir = Path(tempfile.mkdtemp(prefix="cw_stage_"))
        self.log(f"Staging to: {stage_dir}")

        try:
            # Build variable resolver with base vars and user input
            resolver = VariableResolver(
                base_vars={
                    'Name': config['component_name'],
                    'GemName': config['namespace'],
                    'ComponentSuffix': template.component_suffix,
                },
                input_values=config.get('dynamic_fields', {})
            )

            # Stage the component using o3de create-from-template
            if not self._create_staged_component(
                stage_dir=stage_dir,
                namespace=config['namespace'],
                component_name=config['component_name'],
                component_template=template.template_name,
                keep_license=config.get('keep_license', False),
                template_path=template.template_path
            ):
                self.log("Failed to stage component")
                return False

            # Filter copyFiles based on conditions
            files_to_skip = set()
            for copy_def in template.copy_files:
                if copy_def.condition and not resolver.evaluate_condition(copy_def.condition):
                    # Resolve the file path and mark for skipping
                    resolved_file = resolver.resolve(copy_def.file)
                    files_to_skip.add(resolved_file)
                    self.log(f"Skipping file (condition not met): {resolved_file}")

            # Remove skipped files from stage
            interface_skipped = False
            for skip_file in files_to_skip:
                skip_path = stage_dir / skip_file
                if 'Interface' in skip_file:
                    interface_skipped = True
                if skip_path.exists():
                    skip_path.unlink()
                    self.log(f"Removed from stage: {skip_file}")

            # Clean interface references from remaining staged files
            if interface_skipped:
                self._clean_interface_references(stage_dir, config['component_name'])

            # Handle setreg file if present
            setreg_path = None
            for copy_def in template.copy_files:
                if '.setreg' in copy_def.file:
                    setreg_path = self._handle_setreg_file(
                        stage_dir,
                        config['dest_dir'],
                        config['namespace']
                    )
                    break

            # Merge stage to destination
            created, skipped = self._merge_stage_into_dest(
                stage=stage_dir,
                dest=config['dest_dir'],
                skip_existing=True,
                strip_comments=config.get('strip_comments', True),
                keep_license=config.get('keep_license', False)
            )

            self.log(f"Created {created} file(s), skipped {skipped} existing file(s)")

            # Execute process commands
            # Registration commands only run with automatic_register;
            # all other commands (replace_text, add_gem_dependency, etc.) always run.
            REGISTRATION_COMMANDS = {
                'register_file_list', 'register_module_descriptor',
                'register_system_component', 'register_interface_header',
            }
            auto_register = config.get('automatic_register', False)

            if template.process_commands:
                self.log("Executing process commands...")

                ctx = CommandContext(
                    dest_root=config['dest_dir'],
                    namespace=config['namespace'],
                    component_name=config['component_name'],
                    build_target=config.get('build_target'),
                    variables=resolver.variables,
                    logger=self.log,
                    engine_path=self.engine_path
                )

                for cmd_def in template.process_commands:
                    # Skip registration commands when automatic_register is off
                    if cmd_def.command in REGISTRATION_COMMANDS and not auto_register:
                        continue

                    # Check condition
                    if not resolver.evaluate_condition(cmd_def.condition):
                        self.log(f"Skipping command '{cmd_def.command}' (condition not met)")
                        continue

                    # Resolve command arguments
                    resolved_args = resolver.resolve_dict(cmd_def.args)

                    # Create and execute command
                    command = CommandRegistry.create(cmd_def.command, resolved_args)
                    if command:
                        self.log(f"Executing: {cmd_def.command}")
                        command.execute(ctx)
                    else:
                        self.log(f"Warning: Unknown command '{cmd_def.command}'")

            self.log("Component creation complete!")
            return True

        except Exception as e:
            self.log(f"Error: {e}")
            traceback.print_exc()
            return False
        finally:
            try:
                shutil.rmtree(stage_dir, ignore_errors=True)
            except Exception:
                pass

    def _create_staged_component(self, stage_dir: Path, namespace: str,
                                component_name: str, component_template: str,
                                keep_license: bool,
                                template_path: Optional[Path] = None) -> bool:
        """Create the staged component using o3de create-from-template"""
        try:
            # Use the provided template_path (from WizardTemplate), or fall back to engine
            if template_path and (template_path / "template.json").is_file():
                use_template_path = True
            else:
                template_path = self.engine_path / "Templates" / component_template
                use_template_path = (template_path / "template.json").is_file()
            
            script_name = "o3de.bat" if sys.platform == "win32" else "o3de.sh"
            o3de_script = self.engine_path / "scripts" / script_name
            
            cmd = [
                str(o3de_script),
                "create-from-template",
                "-dp", str(stage_dir),
                "-dn", component_name
            ]
            
            if use_template_path:
                cmd += ["-tp", str(template_path)]
            else:
                cmd += ["-tn", component_template]
            
            cmd += ["-r", "${GemName}", namespace]
            cmd.append("-kr")
            
            if keep_license:
                cmd.append("-kl")
            
            cmd.append("--force")
            
            self.log(f"Instantiating template '{component_template}'...")
            
            result = subprocess.run(
                cmd,
                cwd=str(self.engine_path),
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                check=True
            )
            
            if result.stdout:
                for line in result.stdout.splitlines():
                    if line.strip():
                        self.log(line)
            
            self.log(f"Successfully created staged component: {component_name}")
            return True
            
        except subprocess.CalledProcessError as e:
            self.log(f"Failed to create component (exit code {e.returncode})")
            if e.stdout:
                self.log(e.stdout)
            if e.stderr:
                self.log(e.stderr)
            return False
        except Exception as e:
            self.log(f"Error: {e}")
            return False

    def _clean_interface_references(self, stage_dir: Path, component_name: str):
        """Remove interface-related references from staged .h and .cpp files
        when the interface file has been skipped."""
        self.log("Cleaning interface references from staged files...")

        for source_file in stage_dir.rglob("*"):
            if not source_file.is_file() or source_file.suffix not in ('.h', '.cpp'):
                continue

            text = source_file.read_text(encoding="utf-8")
            original = text

            # Remove #include line for the interface header
            text = re.sub(
                r'^#include\s+[<"].*?Interface\.h[>"]\s*\n',
                '', text, flags=re.MULTILINE
            )

            # Remove RequestBus::Handler inheritance (", public ...RequestBus::Handler")
            text = re.sub(
                r'\s*,\s*public\s+\S+RequestBus::Handler',
                '', text
            )

            # Remove RequestBus::Handler::BusConnect(...) lines
            text = re.sub(
                r'^\s*\S+RequestBus::Handler::BusConnect\(.*?\);\s*\n',
                '', text, flags=re.MULTILINE
            )

            # Remove RequestBus::Handler::BusDisconnect(...) lines
            text = re.sub(
                r'^\s*\S+RequestBus::Handler::BusDisconnect\(.*?\);\s*\n',
                '', text, flags=re.MULTILINE
            )

            if text != original:
                source_file.write_text(text, encoding="utf-8")
                self.log(f"Cleaned interface references from: {source_file.name}")

    def _merge_stage_into_dest(self, stage: Path, dest: Path,
                            skip_existing: bool, strip_comments: bool,
                            keep_license: bool) -> Tuple[int, int]:
        """Merge staged files into destination"""
        created = skipped = 0
        dest.mkdir(parents=True, exist_ok=True)
        
        for src_file in stage.rglob("*"):
            if src_file.is_dir():
                continue
            
            # Skip setreg files - they're handled separately
            if src_file.suffix == '.setreg':
                self.log(f"Skipping setreg in merge: {src_file.name}")
                continue
            
            rel = src_file.relative_to(stage)
            dst_file = dest / rel
            dst_file.parent.mkdir(parents=True, exist_ok=True)
            
            if skip_existing and dst_file.exists():
                skipped += 1
                self.log(f"Skipped existing: {rel}")
                continue
            
            if strip_comments and self._should_strip_comments(rel):
                try:
                    data = src_file.read_text(encoding="utf-8")
                    data = self._strip_c_comments(data, keep_license)
                    dst_file.write_text(data, encoding="utf-8", newline="\n")
                except UnicodeDecodeError:
                    shutil.copy2(src_file, dst_file)
            else:
                shutil.copy2(src_file, dst_file)
            
            created += 1
            self.log(f"Created: {rel}")
        
        return created, skipped
    
    def _handle_setreg_file(self, stage_dir: Path, dest_root: Path, namespace: str):
        """Move setreg file from stage to proper Registry location"""
        # The template creates: ${GemName}DataAssetRegistry.setreg
        # After template substitution it becomes: {namespace}DataAssetRegistry.setreg
        expected_name = f"{namespace}DataAssetRegistry.setreg"
        
        self.log(f"Looking for setreg file: {expected_name}")
        
        # Search for the file in stage
        staged_setreg = None
        for setreg_file in stage_dir.rglob("*.setreg"):
            if setreg_file.name == expected_name:
                staged_setreg = setreg_file
                break
        
        if not staged_setreg:
            self.log(f"Warning: Could not find {expected_name} in stage directory")
            setreg_candidates = list(stage_dir.rglob("*DataAssetRegistry.setreg"))
            if setreg_candidates:
                staged_setreg = setreg_candidates[0]
                self.log(f"Found alternative setreg: {staged_setreg.name}")
            else:
                self.log("No setreg file found in stage")
                return None
        
        self.log(f"Found staged setreg: {staged_setreg}")
        
        # Determine target Registry directory
        dest_root = Path(dest_root).resolve()
        
        # Check if project gem (dest_root is <project>/Gem)
        if dest_root.name == "Gem":
            project_json = dest_root.parent / "project.json"
            if project_json.is_file():
                # Project gem - Registry goes INSIDE Gem folder
                target_dir = dest_root / "Registry"
                self.log(f"Detected project gem, Registry at: {target_dir}")
            else:
                # Gem folder but not a project gem
                target_dir = dest_root / "Registry"
                self.log(f"Gem folder (non-project), Registry at: {target_dir}")
        # Check if external gem code folder (<gem>/Code)
        elif dest_root.name == "Code":
            gem_json = dest_root.parent / "gem.json"
            if gem_json.is_file():
                # External gem - Registry at gem root level
                target_dir = dest_root.parent / "Registry"
                self.log(f"Detected external gem code folder, Registry at: {target_dir}")
            else:
                # Code folder but not a gem
                target_dir = dest_root / "Registry"
                self.log(f"Code folder (non-gem), Registry at: {target_dir}")
        else:
            # Fallback - assume dest_root is gem root
            gem_json = dest_root / "gem.json"
            if gem_json.is_file():
                target_dir = dest_root / "Registry"
                self.log(f"Detected gem root, Registry at: {target_dir}")
            else:
                target_dir = dest_root / "Registry"
                self.log(f"Fallback, Registry at: {target_dir}")
        
        # Create Registry directory
        target_dir.mkdir(parents=True, exist_ok=True)
        self.log(f"Created/verified Registry directory: {target_dir}")
        
        # Target path with original filename
        target_setreg = target_dir / expected_name
        
        self.log(f"Target setreg path: {target_setreg}")
        
        # Copy the file to target location
        try:
            if target_setreg.exists():
                self.log(f"Setreg already exists, will be updated: {target_setreg}")
            else:
                self.log(f"Copying setreg from stage to Registry...")
                shutil.copy2(staged_setreg, target_setreg)
                self.log(f"Successfully copied setreg to: {target_setreg}")
            
            return target_setreg
        except Exception as e:
            self.log(f"Error handling setreg: {e}")
            import traceback
            self.log(traceback.format_exc())
            return None
    
    @staticmethod
    def _should_strip_comments(rel_path: Path) -> bool:
        """Check if file should have comments stripped"""
        import fnmatch
        path_str = str(rel_path).replace("\\", "/")
        return any(fnmatch.fnmatch(path_str, pat) for pat in COMMENT_FILE_GLOBS)
    
    @staticmethod
    def _strip_c_comments(text: str, preserve_license: bool) -> str:
        """Strip C/C++ style comments from text, preserving license blocks"""
        if not text:
            return text
        
        # Protect license blocks - detect the actual O3DE license format
        protected = []
        def protect(m):
            idx = len(protected)
            protected.append(m.group(0))
            return f"__CW_LIC_{idx}__"
        
        # Protect O3DE license blocks (at start of file, contains "Copyright" and "SPDX-License-Identifier")
        # This matches the standard O3DE license header format
        license_pattern = r'/\*\s*\n\s*\*\s*Copyright\s+\(c\).*?SPDX-License-Identifier:.*?\*/'
        text = re.sub(license_pattern, protect, text, flags=re.S | re.I)
        
        # Remove remaining block comments /* ... */
        text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
        
        # Remove line comments //...
        text = re.sub(r"//.*", "", text)
        
        # Remove trailing whitespace from each line
        text = re.sub(r"[ \t]+$", "", text, flags=re.M)
        
        # Remove lines that are now empty (only whitespace)
        lines = text.splitlines()
        cleaned_lines = []
        prev_blank = False
        
        for line in lines:
            # Check if line is empty or only whitespace
            if not line.strip():
                # Only add one blank line max (don't stack multiple blanks)
                if not prev_blank:
                    cleaned_lines.append("")
                    prev_blank = True
            else:
                cleaned_lines.append(line)
                prev_blank = False
        
        text = "\n".join(cleaned_lines)
        
        # Restore protected license blocks
        for i, block in enumerate(protected):
            text = text.replace(f"__CW_LIC_{i}__", block)
        
        return text
    
    def _add_gem_dependency(self, dest_dir: Path, target: Optional[CMakeTarget],
                        dependency: str):
        """Add a gem dependency to a CMake target"""
        if not target:
            self.log("Warning: No build target selected")
            return
        
        cmake_path = target.file
        if not cmake_path.is_file():
            self.log(f"Warning: CMake file not found: {cmake_path}")
            return
        
        self.log(f"Adding dependency to target: {target.name}")
        
        text = cmake_path.read_text(encoding="utf-8")
        
        # Find ALL target blocks
        macro_pat = r'(?:o3de_add_target|ly_add_target)\s*\((?P<body>.*?)\)\s*'
        
        for match in re.finditer(macro_pat, text, flags=re.S | re.M):
            body = match.group('body')
            
            # Check if this is the target we want by matching NAME
            name_match = re.search(r'\bNAME\s+([^\s\)]+)', body)
            if not name_match:
                continue
            
            # Extract name (could be quoted or with variables)
            name_in_cmake = name_match.group(1).strip('"\'')
            
            # Match if it contains the target's raw_name pattern
            # (handles ${gem_name}.Private.Object matching MyGem.Private.Object)
            is_match = False
            if name_in_cmake == target.raw_name:
                is_match = True
            elif '${' in name_in_cmake and '.' in target.name:
                # Check if suffix matches (e.g., .Private.Object)
                if '.' in name_in_cmake:
                    cmake_suffix = name_in_cmake.split('.', 1)[1]
                    target_suffix = target.name.split('.', 1)[1] if '.' in target.name else ''
                    if cmake_suffix == target_suffix:
                        is_match = True
            
            if not is_match:
                continue
            
            # Found the right target!
            self.log(f"Found target block with NAME {name_in_cmake}")
            
            # Check if dependency already exists in this target
            if dependency in body:
                self.log(f"Dependency already present: {dependency}")
                return
            
            # Parse lines
            lines = body.splitlines()
            
            # Find BUILD_DEPENDENCIES
            deps_idx = None
            for i, line in enumerate(lines):
                if re.match(r'^\s*BUILD_DEPENDENCIES\b', line):
                    deps_idx = i
                    break
            
            # Get base indent
            base_indent = '    '
            for line in lines:
                if line.strip() and not line.strip().startswith('#'):
                    base_indent = re.match(r'(\s*)', line).group(1)
                    break
            
            if deps_idx is None:
                # Add BUILD_DEPENDENCIES at end
                lines.append('')
                lines.append(f'{base_indent}BUILD_DEPENDENCIES')
                lines.append(f'{base_indent}    PRIVATE')
                lines.append(f'{base_indent}        {dependency}')
            else:
                # Find PRIVATE section within BUILD_DEPENDENCIES
                private_idx = None
                deps_end = len(lines)
                
                for i in range(deps_idx + 1, len(lines)):
                    if re.match(r'^\s*PRIVATE\b', lines[i]):
                        private_idx = i
                    # Stop at next major section
                    if re.match(r'^\s*[A-Z_]+\b', lines[i]) and not re.match(r'^\s*(PRIVATE|PUBLIC|INTERFACE)\b', lines[i]):
                        deps_end = i
                        break
                
                if private_idx is None:
                    # Add PRIVATE section
                    indent = re.match(r'(\s*)', lines[deps_idx]).group(1)
                    lines.insert(deps_idx + 1, f'{indent}    PRIVATE')
                    lines.insert(deps_idx + 2, f'{indent}        {dependency}')
                else:
                    # Add to PRIVATE - insert right after PRIVATE line
                    indent = re.match(r'(\s*)', lines[private_idx]).group(1)
                    lines.insert(private_idx + 1, f'{indent}    {dependency}')
            
            # Rebuild
            new_body = '\n'.join(lines) + '\n'
            new_text = text[:match.start()] + match.group(0).replace(body, new_body) + text[match.end():]
            
            cmake_path.write_text(new_text, encoding='utf-8', newline='\n')
            self.log(f"Added dependency {dependency} to {target.name}")
            return
        
        self.log(f"Warning: Could not find target block for {target.name}")
        
    # Legacy _register_component method removed - now handled by command system
    # All registration logic is in the individual Command classes


    def _register_file_list(self, dest_root: Path, component_name: str, 
                           target: CMakeTarget):
        """Add component files to CMake file list"""
        self.log(f"Registering files in target '{target.name}'...")
        
        cmake_path = target.file
        rel_hdr = f"Source/{component_name}.h"
        rel_cpp = f"Source/{component_name}.cpp"
        
        # If target has FILES_CMAKE includes, update those
        if target.files_cmake_list:
            for files_cmake in target.files_cmake_list:
                include_path = (cmake_path.parent / files_cmake).resolve()
                
                if include_path.exists():
                    self._update_files_cmake(include_path, rel_hdr, rel_cpp)
                    self.log(f"Updated: {files_cmake}")
                    return
        
        # Otherwise append target_sources directly to CMakeLists.txt
        cmake_text = cmake_path.read_text(encoding="utf-8")
        
        append = (
            f"\n# Added by Class Wizard\n"
            f"target_sources({target.name} PRIVATE\n"
            f"    {rel_hdr}\n"
            f"    {rel_cpp}\n"
            f")\n"
        )
        
        if append not in cmake_text:
            cmake_text = cmake_text.rstrip() + append
            cmake_path.write_text(cmake_text, encoding="utf-8", newline="\n")
            self.log(f"Added target_sources block to {cmake_path.name}")
    
    def _update_files_cmake(self, files_cmake_path: Path, rel_hdr: str, rel_cpp: str):
        """Update a FILES_CMAKE include file"""
        if files_cmake_path.exists():
            text = files_cmake_path.read_text(encoding="utf-8")
        else:
            text = "set(FILES\n)\n"
        
        # Find set(FILES ...) block
        match = re.search(r'set\s*\(\s*FILES\b(.*?)(\))', text, flags=re.S | re.M)
        if match:
            end_pos = match.end(1)
            
            # Check if files already exist
            if rel_hdr not in text:
                text = text[:end_pos] + f"    {rel_hdr}\n" + text[end_pos:]
            
            # Re-find the match after first insertion
            match = re.search(r'set\s*\(\s*FILES\b(.*?)(\))', text, flags=re.S | re.M)
            if match:
                end_pos = match.end(1)
                if rel_cpp not in text:
                    text = text[:end_pos] + f"    {rel_cpp}\n" + text[end_pos:]
        else:
            # Create new set(FILES) block
            text = text.rstrip() + f"\nset(FILES\n    {rel_hdr}\n    {rel_cpp}\n)\n"
        
        files_cmake_path.write_text(text, encoding="utf-8", newline="\n")

    def _find_interface_target(self, targets: List[CMakeTarget], gem_name: str, fallback_target: CMakeTarget) -> CMakeTarget:
        """Find the best target for interface headers (INTERFACE > .API > fallback)"""
        
        # Priority 1: INTERFACE target
        for target in targets:
            if 'INTERFACE' in target.name.upper():
                self.log(f"Found INTERFACE target: {target.name}")
                return target
        
        # Priority 2: .API target
        for target in targets:
            if target.name.endswith('.API') or '.API.' in target.name:
                self.log(f"Found API target: {target.name}")
                return target
        
        # Priority 3: Fallback to provided target
        self.log(f"Using fallback target: {fallback_target.name}")
        return fallback_target
    
    def _register_interface_header(self, dest_root: Path, namespace: str, 
                                component_name: str, interface_target: CMakeTarget):
        """Register interface header file to INTERFACE/API target"""
        self.log(f"Registering interface header in target '{interface_target.name}'...")
        
        cmake_path = interface_target.file
        # Correct relative path from CMakeLists.txt location
        rel_hdr = f"Include/{namespace}/{component_name}Interface.h"
        
        # Check if this is an o3de_add_target
        if interface_target.kind == "o3de_add_target":
            # If target has FILES_CMAKE includes, update those
            if interface_target.files_cmake_list:
                for files_cmake in interface_target.files_cmake_list:
                    include_path = (cmake_path.parent / files_cmake).resolve()
                    
                    if include_path.exists():
                        self._update_interface_files_cmake(include_path, rel_hdr)
                        self.log(f"Updated interface header in: {files_cmake}")
                        return
            
            # Otherwise, add to target directly
            self._add_interface_to_target(cmake_path, interface_target, rel_hdr)
        else:
            # For plain CMake targets, just add via target_sources
            self._add_interface_to_target(cmake_path, interface_target, rel_hdr)

    def _update_interface_files_cmake(self, files_cmake_path: Path, rel_hdr: str):
        """Update a FILES_CMAKE file with interface header"""
        if files_cmake_path.exists():
            text = files_cmake_path.read_text(encoding="utf-8")
        else:
            text = "set(FILES\n)\n"
        
        # Check if already present
        if rel_hdr in text:
            self.log(f"Interface header already in {files_cmake_path.name}")
            return
        
        # Find set(FILES ...) block
        match = re.search(r'set\s*\(\s*FILES\b(.*?)(\))', text, flags=re.S | re.M)
        if match:
            end_pos = match.end(1)
            text = text[:end_pos] + f"    {rel_hdr}\n" + text[end_pos:]
        else:
            # Create new block
            text = text.rstrip() + f"\nset(FILES\n    {rel_hdr}\n)\n"
        
        files_cmake_path.write_text(text, encoding="utf-8", newline="\n")
        self.log(f"Added interface header to {files_cmake_path.name}")

    def _add_interface_to_target(self, cmake_path: Path, target: CMakeTarget, rel_hdr: str):
        """Add interface header directly to target using target_sources"""
        text = cmake_path.read_text(encoding="utf-8")
        
        # Check if already added
        if rel_hdr in text:
            self.log(f"Interface header already in {cmake_path.name}")
            return
        
        # Look for existing target_sources for this target with INTERFACE
        pattern = rf'target_sources\s*\(\s*{re.escape(target.name)}\s+INTERFACE\s+([^)]*)\)'
        match = re.search(pattern, text, flags=re.S)
        
        if match:
            # Add to existing INTERFACE block
            content = match.group(1)
            # Find indent
            indent_match = re.search(r'\n(\s+)', content)
            indent = indent_match.group(1) if indent_match else '    '
            
            # Add before closing paren
            new_content = content.rstrip() + f'\n{indent}{rel_hdr}\n'
            new_text = text[:match.start(1)] + new_content + text[match.end(1):]
            text = new_text
        else:
            # Create new target_sources block
            append = (
                f"\n# Interface header\n"
                f"target_sources({target.name} INTERFACE\n"
                f"    {rel_hdr}\n"
                f")\n"
            )
            text = text.rstrip() + append
        
        cmake_path.write_text(text, encoding="utf-8", newline="\n")
        self.log(f"Added interface header to {target.name}")
    
    def _register_module_descriptor(self, dest_root: Path, component_name: str, 
                                    namespace: str, module_kind: str):
        """Register component in gem module descriptor"""
        self.log("Registering in module descriptor...")
        
        # Find module file
        suffix = "EditorModule" if module_kind == "editor" else "Module"
        candidates = [
            dest_root / "Code" / "Source" / "Tools" / f"{namespace}{suffix}.cpp",
            dest_root / "Code" / "Source" / f"{namespace}{suffix}.cpp",
            dest_root / "Source" / f"{namespace}{suffix}.cpp",
            dest_root / "Code" / "Source" / f"{namespace}{suffix}Interface.cpp",
            dest_root / "Source" / f"{namespace}{suffix}Interface.cpp",
            dest_root / "Source" / "Tools" / f"{namespace}{suffix}.cpp",
        ]
        
        module_path = None
        for candidate in candidates:
            if candidate.is_file():
                module_path = candidate
                break
        
        if not module_path:
            # Search for any Module.cpp
            for pattern in ["Code/Source/*Module.cpp", "Source/*Module.cpp"]:
                matches = list(dest_root.glob(pattern))
                if matches:
                    module_path = matches[0]
                    break
        
        if not module_path:
            self.log("Warning: Could not find module file")
            return
        
        text = module_path.read_text(encoding="utf-8")
        include_line = f'#include "{component_name}.h"'
        
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
        descriptor_line = f"{component_name}::CreateDescriptor()"
        
        if descriptor_line in text:
            self.log(f"Descriptor already present for {component_name}")
            module_path.write_text(text, encoding="utf-8", newline="\n")
            return
        
        # Find m_descriptors.insert block
        pattern = r'(m_descriptors\.insert\s*\(\s*m_descriptors\.end\s*\(\s*\)\s*,\s*\{)([^}]*)(\}\s*\)\s*;)'
        match = re.search(pattern, text, flags=re.S)

        if match:
            prefix = match.group(1)   # 'm_descriptors.insert(...{'
            inner  = match.group(2)   # contents between '{' and '}'
            suffix = match.group(3)   # '});'

            # ----- NEW: compute block indent (indent of the m_descriptors line) -----
            start = match.start()  # index of 'm' in m_descriptors
            line_start = text.rfind('\n', 0, start)
            if line_start == -1:
                block_indent = ""
            else:
                block_indent = text[line_start + 1:start]
            # ------------------------------------------------------------------------

            # Detect indent from an existing descriptor line if possible
            entry_indent_match = re.search(r'\n(\s*).*::CreateDescriptor\(\)', inner)
            if entry_indent_match:
                entry_indent = entry_indent_match.group(1)
            else:
                # Fallback: entries indented one level more than the block
                entry_indent = block_indent + "    "

            # Split into lines and remove empty/whitespace-only lines
            lines = inner.split('\n')
            cleaned = [line for line in lines if line.strip()]

            # Ensure last existing descriptor has a trailing comma
            if cleaned:
                last = cleaned[-1].rstrip()
                if not last.endswith(','):
                    cleaned[-1] = last + ','

            # Add new descriptor with consistent indent
            cleaned.append(f"{entry_indent}{descriptor_line},")

            # Rebuild inner block:
            #   newline
            #   entries
            #   newline + block_indent  (so the closing '});' aligns with the insert)
            new_inner = '\n' + '\n'.join(cleaned) + '\n' + block_indent

            new_block = prefix + new_inner + suffix
            text = text[:match.start()] + new_block + text[match.end():]

            self.log(f"Added descriptor for {component_name}")
        else:
            self.log("Warning: Could not find m_descriptors.insert block")
        
        module_path.write_text(text, encoding="utf-8", newline="\n")
        self.log(f"Updated module: {module_path.name}")


    
    def _register_system_component(self, dest_root: Path, namespace: str,
                                component_name: str, module_kind: str):
        """Register a system component in runtime or editor module"""
        self.log(f"Registering system component in {module_kind} module...")
        
        # Find module file
        suffix = "EditorModule" if module_kind == "editor" else "Module"
        candidates = [
            dest_root / "Code" / "Source" / "Tools" / f"{namespace}{suffix}.cpp",
            dest_root / "Code" / "Source" / f"{namespace}{suffix}.cpp",
            dest_root / "Source" / f"{namespace}{suffix}.cpp",
            dest_root / "Code" / "Source" / f"{namespace}{suffix}Interface.cpp",
            dest_root / "Source" / f"{namespace}{suffix}Interface.cpp",
            dest_root / "Source" / "Tools" / f"{namespace}{suffix}.cpp",
        ]
        
        module_path = None
        for candidate in candidates:
            if candidate.is_file():
                module_path = candidate
                break
        
        if not module_path:
            self.log(f"Warning: Could not find {module_kind} module file")
            return
        
        text = module_path.read_text(encoding="utf-8")
        include_line = f'#include "{component_name}.h"'
        
        # Add include
        if include_line not in text:
            lines = text.splitlines()
            last_include = 0
            for i, line in enumerate(lines):
                if line.strip().startswith("#include"):
                    last_include = i
            lines.insert(last_include + 1, include_line)
            text = "\n".join(lines) + "\n"
        
        # Add to GetRequiredSystemComponents
        fq_component = f"{namespace}::{component_name}"
        to_insert = f"azrtti_typeid<{fq_component}>()"
        
        # Check if already present
        if to_insert in text:
            self.log(f"Component already registered in {module_kind}")
            return
        
        # Find the ComponentTypeList initialization
        pattern = r'return\s+AZ::ComponentTypeList\s*\{([^}]*)\}'
        match = re.search(pattern, text, flags=re.S)
        
        if match:
            list_content = match.group(1)

            # ----- NEW: compute block indent (indent of the 'return AZ::ComponentTypeList' line) -----
            start = match.start()  # index of 'r' in return
            line_start = text.rfind('\n', 0, start)
            if line_start == -1:
                block_indent = ""
            else:
                block_indent = text[line_start + 1:start]
            # -----------------------------------------------------------------------------------------

            # Find indent from existing entries
            entry_indent_match = re.search(r'\n(\s+)azrtti_typeid', list_content)
            if entry_indent_match:
                entry_indent = entry_indent_match.group(1)
            else:
                # Default: entries indented one level more than the block
                entry_indent = block_indent + "    "
            
            # Split into lines and clean up
            lines = list_content.split('\n')
            cleaned = [line for line in lines if line.strip()]
            
            # Ensure last entry has comma
            if cleaned and not cleaned[-1].rstrip().endswith(','):
                cleaned[-1] = cleaned[-1].rstrip() + ','
            
            # Add new entry with consistent indent
            cleaned.append(f'{entry_indent}{to_insert},')
            
            # Rebuild with proper formatting:
            #   newline
            #   entries
            #   newline + block_indent  (so the closing '};' aligns with the return line)
            new_content = '\n' + '\n'.join(cleaned) + '\n' + block_indent
            
            # Replace just the contents between '{' and '}'
            new_text = text[:match.start(1)] + new_content + text[match.end(1):]
            
            module_path.write_text(new_text, encoding="utf-8", newline="\n")
            self.log(f"Registered in {module_kind} module: {module_path.name}")
        else:
            self.log(f"Warning: Could not find GetRequiredSystemComponents in {module_kind} module")


    def _register_generic_asset(self, dest_root: Path, namespace: str, asset_name: str, 
                            asset_ext: str = "mydata", asset_group: str = "Other"):
        """Register GenericAssetHandler in DataAssetSystemComponent"""
        self.log(f"Registering GenericAssetHandler for {asset_name}...")
        
        sys_comp_name = f"{namespace}DataAssetSystemComponent"
        
        # Find the system component .cpp file
        candidates = [
            dest_root / "Code" / "Source" / f"{sys_comp_name}.cpp",
            dest_root / "Source" / f"{sys_comp_name}.cpp",
        ]
        
        cpp_path = None
        for candidate in candidates:
            if candidate.is_file():
                cpp_path = candidate
                break
        
        if not cpp_path:
            self.log(f"Warning: Could not find {sys_comp_name}.cpp")
            return
        
        text = cpp_path.read_text(encoding="utf-8")
        
        # Add include for the asset if not present
        include_line = f'#include "{asset_name}.h"'
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
            
            # Check if already registered
            handler_check = f'{asset_name}Handler'
            if handler_check in body:
                self.log(f"Handler already registered for {asset_name}")
            else:
                # Detect indent - look for existing "auto*" lines or "m_assetHandlers" lines
                indent_match = re.search(r'\n(\s+)(?:auto\*|m_assetHandlers)', body)
                if not indent_match:
                    # Fallback to any line with content
                    indent_match = re.search(r'\n(\s+)\S', body)
                
                if indent_match:
                    base_indent = indent_match.group(1)
                else:
                    base_indent = '        '  # 8 spaces default
                
                # Check if there's already handler registration content
                has_handlers = 'Handler' in body or 'm_assetHandlers' in body
                
                # Build registration block with newline prefix if there are existing handlers
                suffix = '\n\n' if has_handlers else '\n'
                
                registration_block = (
                    f'{base_indent}// Register {asset_name}\n'
                    f'{base_indent}auto* {asset_name}Handler = aznew AzFramework::GenericAssetHandler<{asset_name}>("{asset_name}", "{asset_group}", "{asset_ext}");\n'
                    f'{base_indent}{asset_name}Handler->Register();\n'
                    f'{base_indent}m_assetHandlers.emplace_back({asset_name}Handler);{suffix}'
                )
                
                # Insert at the very beginning (after opening brace and initial whitespace)
                lines = body.split('\n')
                insert_line = 0
                for i, line in enumerate(lines):
                    if line.strip():  # First non-empty line
                        insert_line = i
                        break
                
                # Reconstruct body with insertion
                if insert_line == 0 and not lines[0].strip():
                    # Empty function, insert after first newline
                    new_body = lines[0] + '\n' + registration_block + '\n'.join(lines[1:])
                else:
                    new_body = '\n'.join(lines[:insert_line]) + '\n' + registration_block + '\n'.join(lines[insert_line:])
                
                text = text[:match.start(1)] + new_body + text[match.end(1):]
                
                self.log(f"Added GenericAssetHandler registration for {asset_name}")
        
        # Find Reflect() method
        reflect_pattern = r'void\s+' + re.escape(sys_comp_name) + r'::Reflect\s*\([^)]*\)\s*\{([^}]*)\}'
        match = re.search(reflect_pattern, text, flags=re.S)
        
        if match:
            body = match.group(1)
            
            # Check if already reflected
            reflect_call = f'{asset_name}::Reflect(context);'
            if reflect_call in body:
                self.log(f"Reflect already present for {asset_name}")
            else:
                # Detect indent - look for existing Reflect calls
                indent_match = re.search(r'\n(\s+)\w+::Reflect', body)
                if not indent_match:
                    indent_match = re.search(r'\n(\s+)\S', body)
                
                if indent_match:
                    base_indent = indent_match.group(1)
                else:
                    base_indent = '        '
                
                # Build reflect call (no newline prefix - keep tight)
                reflect_line = f'{base_indent}{reflect_call}\n'
                
                # Insert at beginning
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
                
                self.log(f"Added Reflect call for {asset_name}")
        
        cpp_path.write_text(text, encoding="utf-8", newline="\n")
        self.log(f"Updated {sys_comp_name}.cpp")

    def _register_asset_setreg(self, setreg_path: Optional[Path], asset_name: str, asset_ext: str, dest_root: Path):
        """Update AssetProcessorGemConfig.setreg with asset info"""
        
        if not setreg_path:
            self.log("Warning: No setreg path provided, skipping setreg update")
            return
        
        if not setreg_path.exists():
            self.log(f"Warning: Setreg file does not exist: {setreg_path}")
            return
        
        self.log(f"Updating setreg for {asset_name} at {setreg_path}...")
        
        # Extract GUID from the asset
        guid = self._extract_asset_guid(dest_root, asset_name)
        if not guid:
            guid = "{00000000-0000-0000-0000-000000000000}"
            self.log("Warning: Using placeholder GUID")
        else:
            self.log(f"Using GUID: {guid}")
        
        # Load setreg data
        try:
            data = json.loads(setreg_path.read_text(encoding="utf-8"))
        except Exception as e:
            self.log(f"Could not load existing setreg, creating new: {e}")
            data = {"Amazon": {"AssetProcessor": {"Settings": {}}}}
        
        # Ensure structure exists
        if "Amazon" not in data:
            data["Amazon"] = {}
        if "AssetProcessor" not in data["Amazon"]:
            data["Amazon"]["AssetProcessor"] = {}
        if "Settings" not in data["Amazon"]["AssetProcessor"]:
            data["Amazon"]["AssetProcessor"]["Settings"] = {}
        
        settings = data["Amazon"]["AssetProcessor"]["Settings"]
        
        # Update or add RC entry
        rc_key = f"RC {asset_name}"
        settings[rc_key] = {
            "glob": f"*.{asset_ext}",
            "params": "copy",
            "productAssetType": guid
        }
        
        # Write back
        setreg_path.write_text(json.dumps(data, indent=4) + "\n", encoding="utf-8", newline="\n")
        self.log(f"Successfully updated setreg: {setreg_path}")

    def _extract_asset_guid(self, dest_root: Path, asset_name: str) -> Optional[str]:
        """Extract the asset's type UUID from AZ_RTTI or AZ_TYPE_INFO"""
        self.log(f"Extracting GUID for {asset_name}...")
        
        guid_pattern = r'\{[0-9A-Fa-f]{8}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{12}\}'
        
        # Look in the asset's .h and .cpp files
        candidates = [
            dest_root / "Source" / f"{asset_name}.h",
            dest_root / "Source" / f"{asset_name}.cpp",
            dest_root / "Code" / "Source" / f"{asset_name}.h",
            dest_root / "Code" / "Source" / f"{asset_name}.cpp",
        ]
        
        for candidate in candidates:
            if not candidate.is_file():
                continue
            
            try:
                text = candidate.read_text(encoding="utf-8", errors="ignore")
            except:
                continue
            
            # Look for AZ_RTTI(AssetName, "{GUID}", ...)
            rtti_match = re.search(
                rf'AZ_RTTI\s*\(\s*{re.escape(asset_name)}\s*,\s*"?({guid_pattern})"?',
                text
            )
            if rtti_match:
                guid = rtti_match.group(1)
                self.log(f"Found GUID in AZ_RTTI: {guid}")
                return guid
            
            # Look for AZ_TYPE_INFO(AssetName, "{GUID}")
            type_info_match = re.search(
                rf'AZ_TYPE_INFO\s*\(\s*{re.escape(asset_name)}\s*,\s*"?({guid_pattern})"?',
                text
            )
            if type_info_match:
                guid = type_info_match.group(1)
                self.log(f"Found GUID in AZ_TYPE_INFO: {guid}")
                return guid
        
        # Fallback: check DataAssetSystemComponent for AZ_COMPONENT_IMPL
        namespace = asset_name.replace("Asset", "").replace("Data", "")  # Best guess
        sys_comp_candidates = [
            dest_root / "Source" / f"{namespace}DataAssetSystemComponent.cpp",
            dest_root / "Code" / "Source" / f"{namespace}DataAssetSystemComponent.cpp",
        ]
        
        for candidate in sys_comp_candidates:
            if not candidate.is_file():
                continue
            
            try:
                text = candidate.read_text(encoding="utf-8", errors="ignore")
            except:
                continue
            
            # Look for AZ_COMPONENT_IMPL(..., "{GUID}")
            comp_match = re.search(
                rf'AZ_COMPONENT_IMPL\s*\([^,]+,\s*"[^"]*",\s*"?({guid_pattern})"?',
                text
            )
            if comp_match:
                guid = comp_match.group(1)
                self.log(f"Found GUID in AZ_COMPONENT_IMPL: {guid}")
                return guid
        
        self.log("Warning: Could not extract GUID from asset")
        return None


# ============================================================================
# PySide6 GUI Application
# ============================================================================

class ClassWizardWindow(QMainWindow):
    """Main window for the O3DE Class Creation Wizard"""

    def __init__(self, engine_path: Path, project_path: Optional[Path] = None):
        super().__init__()

        self.engine_path = engine_path
        self.project_path = project_path

        self.setWindowTitle("O3DE Class Creation Wizard")
        self.setMinimumSize(600, 700)
        self.resize(650, 750)

        # Initialize data
        self.gems: List[GemInfo] = []
        self.targets: List[CMakeTarget] = []
        self.selected_gem: Optional[GemInfo] = None
        self.selected_target: Optional[CMakeTarget] = None

        # Template scanner for command-driven system
        self.template_scanner = WizardTemplateScanner(logger=lambda msg: None)  # Silent during init
        self.available_templates: List[WizardTemplate] = []

        # Setup UI
        self._setup_ui()
        self._apply_styles()

        # Load initial data
        if project_path:
            self._load_project_data()

        # Center window
        self._center_window()
    
    def _setup_ui(self):
        """Setup the user interface"""
        central = QWidget()
        self.setCentralWidget(central)
        
        layout = QVBoxLayout(central)
        layout.setSpacing(10)
        layout.setContentsMargins(15, 15, 15, 15)
        
        # Destination section
        layout.addWidget(self._create_destination_section())
        
        # Component details section
        layout.addWidget(self._create_component_section())
        
        # Settings section
        layout.addWidget(self._create_settings_section())
        
        # Log section
        layout.addWidget(self._create_log_section())
        
        # Buttons
        layout.addWidget(self._create_button_section())
    
    def _create_destination_section(self) -> QGroupBox:
        """Create the destination selection section"""
        group = QGroupBox("Destination")
        layout = QFormLayout()
        
        # Target combo
        self.target_combo = QComboBox()
        self.target_combo.currentTextChanged.connect(self._on_target_changed)
        self._add_form_row(layout, "Target:", self.target_combo)
        
        # Target path with browse
        path_layout = QHBoxLayout()
        self.target_path_edit = QLineEdit()
        self.browse_btn = QPushButton("...")
        self.browse_btn.setMaximumWidth(40)
        self.browse_btn.clicked.connect(self._browse_destination)
        path_layout.addWidget(self.target_path_edit)
        path_layout.addWidget(self.browse_btn)
        self._add_form_row(layout, "Path:", path_layout)
        
        # Package (build target)
        self.package_combo = QComboBox()
        self._add_form_row(layout, "Package:", self.package_combo)
        
        # Namespace
        self.namespace_edit = QLineEdit()
        if self.project_path:
            self.namespace_edit.setText(self.project_path.stem)
        self._add_form_row(layout, "Namespace:", self.namespace_edit)
        
        group.setLayout(layout)
        return group
    
    def _create_component_section(self) -> QGroupBox:
        """Create the component details section"""
        group = QGroupBox("Component Details")
        layout = QFormLayout()
        
        # Component name
        self.component_name_edit = QLineEdit()
        self.component_name_edit.setPlaceholderText("Enter component name...")
        self._add_form_row(layout, "Name:", self.component_name_edit)
        
        # Component type
        self.component_type_combo = QComboBox()
        self.component_type_combo.addItems([
            "Basic", "System", "Level", "LyShine UI", "Data Asset"
        ])
        self.component_type_combo.currentTextChanged.connect(self._on_component_type_changed)
        self._add_form_row(layout, "Type:", self.component_type_combo)
        
        # Dynamic fields will be added directly to this layout
        self.dynamic_layout = layout
        self.dynamic_widgets = []  # Track dynamic widgets for cleanup

        group.setLayout(layout)
        return group
    
    def _create_settings_section(self) -> QGroupBox:
        """Create the settings section"""
        group = QGroupBox("Settings")
        layout = QVBoxLayout()
        
        self.auto_register_check = QCheckBox("Register Automatically")
        self.auto_register_check.setChecked(True)
        self.auto_register_check.setToolTip(
            "Automatically add component to CMake and module files"
        )
        layout.addWidget(self.auto_register_check)
        
        self.remove_comments_check = QCheckBox("Remove Comments")
        self.remove_comments_check.setChecked(False)
        self.remove_comments_check.setToolTip("Strip comments from generated files")
        layout.addWidget(self.remove_comments_check)
        
        self.editor_adapter_check = QCheckBox("Add Editor Adapter")
        self.editor_adapter_check.setToolTip("Create editor-specific component variant")
        layout.addWidget(self.editor_adapter_check)
        
        self.default_license_check = QCheckBox("Default License")
        self.default_license_check.setToolTip("Include license header in source files")
        layout.addWidget(self.default_license_check)
        
        group.setLayout(layout)
        return group
    
    def _create_log_section(self) -> QGroupBox:
        """Create the log output section"""
        group = QGroupBox("Log")
        layout = QVBoxLayout()
        
        self.log_text = QTextEdit()
        self.log_text.setReadOnly(True)
        self.log_text.setMinimumHeight(100)
        layout.addWidget(self.log_text)
        
        group.setLayout(layout)
        return group
    
    def _create_button_section(self) -> QWidget:
        """Create the button section"""
        widget = QWidget()
        layout = QHBoxLayout()
        layout.addStretch()
        
        self.create_btn = QPushButton("Create")
        self.create_btn.clicked.connect(self._on_create)
        layout.addWidget(self.create_btn)
        
        self.cancel_btn = QPushButton("Cancel")
        self.cancel_btn.clicked.connect(self.close)
        layout.addWidget(self.cancel_btn)
        
        widget.setLayout(layout)
        return widget
    
    def _add_form_row(self, layout, text, field_widget):
        label = QLabel(text)
        # Tag this as a form label so we can style it
        label.setProperty("formLabel", True)
        layout.addRow(label, field_widget)
    
    def _apply_styles(self):
        """Apply stylesheet to the window"""
        self.setStyleSheet("""
            QMainWindow {
                background-color: #2b2b2b;
            }
            QGroupBox {
                color: #cccccc;
                border: 1px solid #555555;
                border-radius: 4px;
                margin-top: 8px;
                padding-top: 8px;
                font-weight: bold;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                left: 10px;
                padding: 0 5px;
            }
            QLabel {
                color: #cccccc;
            }
            QLabel[formLabel="true"] {
                font-size: 9.5pt;
                min-width: 90px;
                max-width: 90px;
            }
            QLineEdit, QComboBox, QTextEdit {
                background-color: #3c3c3c;
                color: #cccccc;
                border: 1px solid #555555;
                border-radius: 3px;
                padding: 5px;
                font-size: 9.5pt;
            }
            QLineEdit:focus, QComboBox:focus {
                border: 1px solid #0078d4;
            }
            QComboBox::drop-down {
                border: none;
                width: 20px;
            }
            QComboBox::down-arrow {
                image: none;
                border-left: 4px solid transparent;
                border-right: 4px solid transparent;
                border-top: 6px solid #cccccc;
                margin-right: 5px;
            }
            QPushButton {
                background-color: #0078d4;
                color: white;
                border: none;
                border-radius: 3px;
                padding: 8px 20px;
                font-weight: bold;
            }
            QPushButton:hover {
                background-color: #1084d8;
            }
            QPushButton:pressed {
                background-color: #006cc1;
            }
            QPushButton:disabled {
                background-color: #555555;
                color: #888888;
            }
            QCheckBox {
                color: #cccccc;
                spacing: 8px;
                font-size: 9.5pt;
            }
            QCheckBox::indicator {
                width: 18px;
                height: 18px;
                border: 1px solid #555555;
                border-radius: 3px;
                background-color: #3c3c3c;
            }
            QCheckBox::indicator:checked {
                background-color: #0078d4;
                border-color: #0078d4;
            }
            QTextEdit {
                font-family: 'Courier New', monospace;
                font-size: 9pt;
            }
        """)
    
    def _load_project_data(self):
        """Load gems, targets, and wizard templates for the current project"""
        try:
            self.log("Loading project data...")

            # Discover gems
            self.gems = GemDiscovery.get_enabled_gems(
                self.engine_path,
                self.project_path,
                include_dependencies=True
            )

            # Populate target combo
            self.target_combo.clear()
            self.target_combo.addItem("Project")

            for gem in self.gems:
                self.target_combo.addItem(gem.name)

            # Select first item
            if self.target_combo.count() > 0:
                self.target_combo.setCurrentIndex(0)

            self.log(f"Loaded {len(self.gems)} gems")

            # Scan for wizard-enabled templates
            self.template_scanner = WizardTemplateScanner(logger=self.log)
            self.available_templates = self.template_scanner.scan(
                self.engine_path,
                self.project_path
            )

            # Populate component type combo with discovered templates
            self._populate_component_types()

            self.log(f"Found {len(self.available_templates)} wizard-enabled templates")

        except Exception as e:
            self.log(f"Error loading project data: {e}")
            QMessageBox.critical(self, "Error", f"Failed to load project data:\n{e}")

    def _populate_component_types(self):
        """Populate component type combo from discovered templates"""
        self.component_type_combo.clear()

        if self.available_templates:
            # Use wizard-enabled templates
            for template in self.available_templates:
                self.component_type_combo.addItem(
                    template.display_name,
                    userData=template
                )
            self.log("Using command-driven templates")
        else:
            # Fallback to legacy hardcoded types
            self.component_type_combo.addItems([
                "Basic", "System", "Level", "LyShine UI", "Data Asset"
            ])
            self.log("Using legacy template types (no class_wizard blocks found)")
    
    def _on_target_changed(self, target_name: str):
        """Handle target selection change"""
        try:
            if target_name == "Project":
                # Project gem
                self.selected_gem = None
                gem_path = self.project_path / "Gem"
                cmake_path = gem_path
                gem_name = self.project_path.stem if self.project_path else ""
                
                # Read gem.json if available
                gem_json = gem_path / "gem.json"
                if gem_json.is_file():
                    try:
                        data = json.loads(gem_json.read_text(encoding="utf-8"))
                        gem_name = data.get("gem_name") or data.get("display_name") or gem_name
                    except Exception:
                        pass
                
                self.target_path_edit.setText(str(gem_path))
            else:
                # External gem
                self.selected_gem = next((g for g in self.gems if g.name == target_name), None)
                if not self.selected_gem:
                    return
                
                gem_path = self.selected_gem.path
                cmake_path = gem_path / "Code"
                gem_name = self.selected_gem.name
                
                self.target_path_edit.setText(str(cmake_path))
            
            # Update namespace
            self.namespace_edit.setText(gem_name)
            
            # Scan for build targets
            self.targets = CMakeAnalyzer.scan_targets(cmake_path, gem_name)
            
            # Populate package combo
            self.package_combo.clear()
            if self.targets:
                target_names = []
                seen = set()
                for target in self.targets:
                    if target.name not in seen:
                        seen.add(target.name)
                        target_names.append(target.name)
                
                self.package_combo.addItems(target_names)
                
                # Select preferred target
                preferred = self._find_preferred_target(target_names, gem_name)
                if preferred:
                    idx = self.package_combo.findText(preferred)
                    if idx >= 0:
                        self.package_combo.setCurrentIndex(idx)
                
                self.log(f"Found {len(self.targets)} build targets")
            else:
                self.package_combo.addItem("<no targets found>")
                self.log("Warning: No CMake targets found")
        
        except Exception as e:
            self.log(f"Error changing target: {e}")
    
    def _find_preferred_target(self, names: List[str], gem_name: str) -> Optional[str]:
        """Find the preferred build target from available names"""
        preferences = [
            f"{gem_name}.Private.Object",
            gem_name,
            f"{gem_name}.API",
            f"{gem_name}.Static",
        ]
        
        for pref in preferences:
            if pref in names:
                return pref
        
        return names[0] if names else None
    
    def _on_component_type_changed(self, comp_type: str):
        """Handle component type selection change"""
        # Clear previous dynamic fields
        for widget in self.dynamic_widgets:
            # Remove row from layout (removes both label and field)
            self.dynamic_layout.removeRow(widget)
        self.dynamic_widgets.clear()

        # Check if we have a WizardTemplate for this type
        template = self.component_type_combo.currentData()

        if template and isinstance(template, WizardTemplate):
            # Build dynamic fields from template.input_vars
            for input_def in template.input_vars:
                widget = self._create_input_widget(input_def)
                if widget:
                    self._add_form_row(self.dynamic_layout, f"{input_def.title}:", widget)
                    self.dynamic_widgets.append(widget)
        else:
            # Legacy fallback: hardcoded fields for Data Asset
            if comp_type == "Data Asset":
                ext_edit = QLineEdit("mydata")
                ext_edit.setToolTip("File extension for the asset (without dot)")
                ext_edit.setProperty("var_name", "file_extension")
                self._add_form_row(self.dynamic_layout, "File Extension:", ext_edit)
                self.dynamic_widgets.append(ext_edit)

                group_edit = QLineEdit("DataAssets")
                group_edit.setToolTip("Asset group name for organization")
                group_edit.setProperty("var_name", "asset_group")
                self._add_form_row(self.dynamic_layout, "Group:", group_edit)
                self.dynamic_widgets.append(group_edit)

    def _create_input_widget(self, input_def: InputVarDef) -> Optional[QWidget]:
        """Create a widget for a template input variable"""
        widget = None

        if input_def.input_type == "toggle":
            widget = QCheckBox()
            widget.setChecked(bool(input_def.default_value))
            if input_def.description:
                widget.setToolTip(input_def.description)

        elif input_def.input_type == "text":
            widget = QLineEdit(str(input_def.default_value) if input_def.default_value else "")
            if input_def.description:
                widget.setToolTip(input_def.description)

        elif input_def.input_type == "dropdown":
            widget = QComboBox()
            widget.addItems(input_def.options)
            if input_def.default_value and input_def.default_value in input_def.options:
                widget.setCurrentText(str(input_def.default_value))
            if input_def.description:
                widget.setToolTip(input_def.description)

        if widget:
            widget.setProperty("var_name", input_def.var_name)

        return widget
    
    def _browse_destination(self):
        """Browse for destination directory"""
        current = self.target_path_edit.text()
        directory = QFileDialog.getExistingDirectory(
            self,
            "Select Destination Directory",
            current if current else str(self.project_path)
        )
        
        if directory:
            self.target_path_edit.setText(directory)
            self.log(f"Destination set to: {directory}")
    
    def _on_create(self):
        """Handle create button click"""
        try:
            # Validate inputs
            component_name = self.component_name_edit.text().strip()
            if not component_name:
                QMessageBox.warning(self, "Validation Error", "Component name is required")
                return

            is_valid, error = validate_component_name(component_name)
            if not is_valid:
                QMessageBox.warning(self, "Validation Error", f"Invalid component name:\n{error}")
                return

            namespace = self.namespace_edit.text().strip()
            if not namespace:
                QMessageBox.warning(self, "Validation Error", "Namespace is required")
                return

            is_valid, error = validate_component_name(namespace)
            if not is_valid:
                QMessageBox.warning(self, "Validation Error", f"Invalid namespace:\n{error}")
                return

            dest_dir = self.target_path_edit.text().strip()
            if not dest_dir or not Path(dest_dir).is_dir():
                QMessageBox.warning(self, "Validation Error", "Invalid destination directory")
                return

            # Get selected target
            selected_target = None
            package_name = self.package_combo.currentText()
            if package_name and package_name != "<no targets found>":
                selected_target = next(
                    (t for t in self.targets if t.name == package_name),
                    None
                )

            # Gather dynamic fields from widgets using var_name property
            dynamic_fields = self._collect_dynamic_fields()

            # Check if using command-driven template or legacy mode
            wizard_template = self.component_type_combo.currentData()
            component_type = self.component_type_combo.currentText()

            # Build configuration
            config = {
                'component_name': component_name,
                'namespace': namespace,
                'dest_dir': Path(dest_dir),
                'keep_license': self.default_license_check.isChecked(),
                'strip_comments': self.remove_comments_check.isChecked(),
                'automatic_register': self.auto_register_check.isChecked(),
                'build_target': selected_target,
                'dynamic_fields': dynamic_fields,
            }

            if not wizard_template or not isinstance(wizard_template, WizardTemplate):
                raise ValueError(
                    f"No wizard template found for '{component_type}'. "
                    "Ensure the template has a 'class_wizard' block in its template.json."
                )

            # Command-driven mode (required)
            config['wizard_template'] = wizard_template
            config['component_template'] = wizard_template.template_name

            # Clear log and create component
            self.log_text.clear()
            self.log("Creating component...")

            # Disable UI during creation
            self.create_btn.setEnabled(False)
            self.cancel_btn.setEnabled(False)

            # Create component
            creator = ComponentCreator(self.engine_path, logger=self.log)
            success = creator.create_component(config)

            # Re-enable UI
            self.create_btn.setEnabled(True)
            self.cancel_btn.setEnabled(True)

            if success:
                QMessageBox.information(
                    self,
                    "Success",
                    f"Component '{component_name}' created successfully!"
                )
            else:
                QMessageBox.warning(
                    self,
                    "Creation Failed",
                    "Component creation failed. Check the log for details."
                )

        except Exception as e:
            self.log(f"Error: {e}")
            traceback.print_exc()
            QMessageBox.critical(self, "Error", f"An error occurred:\n{e}")

            self.create_btn.setEnabled(True)
            self.cancel_btn.setEnabled(True)

    def _collect_dynamic_fields(self) -> Dict[str, Any]:
        """Collect values from dynamic input fields using var_name property"""
        dynamic_fields = {}

        for i in range(self.dynamic_layout.rowCount()):
            field_item = self.dynamic_layout.itemAt(i, QFormLayout.FieldRole)

            if field_item:
                field = field_item.widget()
                if not field:
                    continue

                var_name = field.property("var_name")
                if not var_name:
                    # Fallback to label text for legacy compatibility
                    label_item = self.dynamic_layout.itemAt(i, QFormLayout.LabelRole)
                    if label_item and label_item.widget():
                        label = label_item.widget()
                        if isinstance(label, QLabel):
                            var_name = label.text().rstrip(':').replace(' ', '').lower()

                if var_name:
                    if isinstance(field, QLineEdit):
                        dynamic_fields[var_name] = field.text()
                    elif isinstance(field, QCheckBox):
                        dynamic_fields[var_name] = field.isChecked()
                    elif isinstance(field, QComboBox):
                        dynamic_fields[var_name] = field.currentText()

        return dynamic_fields

    def log(self, message: str):
        """Append a message to the log"""
        self.log_text.append(message)
        # Auto-scroll to bottom
        scrollbar = self.log_text.verticalScrollBar()
        scrollbar.setValue(scrollbar.maximum())
    
    def _center_window(self):
        """Center the window on screen"""
        QTimer.singleShot(0, self._do_center)
    
    def _do_center(self):
        """Actually center the window"""
        screen = QApplication.primaryScreen().geometry()
        window = self.frameGeometry()
        center = screen.center()
        window.moveCenter(center)
        self.move(window.topLeft())


# ============================================================================
# Command Line Interface
# ============================================================================

def discover_cli_templates(engine_path: Path, project_path: Optional[Path] = None) -> List[WizardTemplate]:
    """Discover all wizard-enabled templates for CLI"""
    scanner = WizardTemplateScanner()
    return scanner.scan(engine_path, project_path)


def list_templates(templates: List[WizardTemplate]) -> None:
    """Print available templates to stdout"""
    print("\nAvailable Templates:")
    print("-" * 60)
    for template in templates:
        print(f"  {template.class_name:<25} {template.display_name}")
        if template.description:
            print(f"    {template.description}")
    print()


def print_template_help(template: WizardTemplate) -> None:
    """Print detailed help for a specific template"""
    print(f"\nTemplate: {template.display_name}")
    print(f"  Class Name: {template.class_name}")
    if template.description:
        print(f"  Description: {template.description}")
    print(f"  Component Suffix: {template.component_suffix}")

    if template.input_vars:
        print(f"\n  Template-Specific Arguments:")
        for var in template.input_vars:
            arg_name = f"--{var.var_name.replace('_', '-')}"
            default_str = f" (default: {var.default_value})" if var.default_value is not None else ""
            required_str = " [REQUIRED]" if var.required else ""
            print(f"    {arg_name:<25} {var.title}{default_str}{required_str}")
            if var.description:
                print(f"      {var.description}")

    if template.process_commands:
        print(f"\n  Processing Commands ({len(template.process_commands)} commands):")
        for cmd in template.process_commands:
            condition_str = f" [if {cmd.condition}]" if cmd.condition else ""
            # Format command arguments
            if cmd.args:
                args_str = ", ".join(f"{k}={v}" for k, v in cmd.args.items())
                print(f"    - {cmd.command}({args_str}){condition_str}")
            else:
                print(f"    - {cmd.command}{condition_str}")
    print()


def build_dynamic_parser(templates: List[WizardTemplate]) -> argparse.ArgumentParser:
    """Build argument parser with dynamic template choices"""
    template_choices = [t.class_name for t in templates]

    parser = argparse.ArgumentParser(
        description="O3DE Class Creation Wizard - Command-driven component generation",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Launch GUI mode
  python ClassWizard.py --engine-path D:\\O3DE

  # List available templates
  python ClassWizard.py --engine-path D:\\O3DE --list-templates

  # Get help for a specific template
  python ClassWizard.py --engine-path D:\\O3DE --template-help data_asset

  # Create a basic component (CLI mode)
  python ClassWizard.py --engine-path D:\\O3DE --project-path D:\\MyProject \\
    --template default_component --component-name MyComponent --namespace MyGem \\
    --automatic-register

  # Create a data asset with custom settings
  python ClassWizard.py --engine-path D:\\O3DE --project-path D:\\MyProject \\
    --template data_asset --component-name MyData --namespace MyGem \\
    --file-extension mydat --asset-group "Custom Assets" --automatic-register
"""
    )

    # Required arguments
    parser.add_argument(
        "--engine-path",
        required=True,
        type=str,
        help="Path to O3DE engine root"
    )

    # Project path (required for CLI mode)
    parser.add_argument(
        "--project-path",
        type=str,
        help="Path to O3DE project (required for CLI mode)"
    )

    # Template discovery and help
    parser.add_argument(
        "--list-templates",
        action="store_true",
        help="List all available wizard-enabled templates and exit"
    )

    parser.add_argument(
        "--template-help",
        type=str,
        metavar="TEMPLATE",
        help="Show detailed help for a specific template and exit"
    )

    # CLI mode arguments
    parser.add_argument(
        "--template",
        type=str,
        choices=template_choices if template_choices else None,
        help="Template to use for component generation (enables CLI mode)"
    )

    parser.add_argument(
        "--component-name",
        type=str,
        help="Component name"
    )

    parser.add_argument(
        "--namespace",
        type=str,
        help="Component namespace (gem name)"
    )

    parser.add_argument(
        "--target-path",
        type=str,
        help="Destination path (defaults to project/Gem)"
    )

    parser.add_argument(
        "--automatic-register",
        action="store_true",
        help="Automatically register component in CMake and modules"
    )

    parser.add_argument(
        "--default-license",
        action="store_true",
        help="Include default license header in generated files"
    )

    parser.add_argument(
        "--keep-comments",
        action="store_true",
        help="Keep comments in generated files (default: strip comments)"
    )

    return parser


def add_template_specific_args(parser: argparse.ArgumentParser, template: WizardTemplate) -> None:
    """Add template-specific arguments from input_vars"""
    if not template.input_vars:
        return

    for var in template.input_vars:
        arg_name = f"--{var.var_name.replace('_', '-')}"

        # Check if argument already exists (avoid duplicates)
        existing_args = [a.option_strings for a in parser._actions if hasattr(a, 'option_strings')]
        if any(arg_name in opts for opts in existing_args):
            continue

        if var.input_type == "toggle":
            parser.add_argument(
                arg_name,
                action="store_true",
                default=var.default_value if var.default_value is not None else False,
                help=f"{var.title}" + (f": {var.description}" if var.description else "")
            )
        elif var.input_type == "dropdown" and var.options:
            parser.add_argument(
                arg_name,
                type=str,
                choices=var.options,
                default=var.default_value,
                help=f"{var.title}" + (f": {var.description}" if var.description else "")
            )
        else:
            # text input
            parser.add_argument(
                arg_name,
                type=str,
                default=str(var.default_value) if var.default_value is not None else None,
                required=var.required and var.default_value is None,
                help=f"{var.title}" + (f": {var.description}" if var.description else "")
            )


def create_component_cli(args, template: WizardTemplate, engine_path: Path, project_path: Path) -> bool:
    """Create component from command line arguments using dynamic template"""
    try:
        # Validate required arguments
        if not args.component_name:
            print("Error: --component-name is required in CLI mode")
            return False

        is_valid, error = validate_component_name(args.component_name)
        if not is_valid:
            print(f"Error: Invalid component name: {error}")
            return False

        if not args.namespace:
            print("Error: --namespace is required in CLI mode")
            return False

        is_valid, error = validate_component_name(args.namespace)
        if not is_valid:
            print(f"Error: Invalid namespace: {error}")
            return False

        # Determine destination
        if args.target_path:
            dest_dir = Path(args.target_path)
        else:
            dest_dir = project_path / "Gem"

        if not dest_dir.is_dir():
            print(f"Error: Destination directory does not exist: {dest_dir}")
            return False

        # Collect dynamic fields from template-specific arguments
        dynamic_fields = {}
        for var in template.input_vars:
            arg_name = var.var_name.replace('-', '_')
            if hasattr(args, arg_name):
                value = getattr(args, arg_name)
                if value is not None:
                    dynamic_fields[var.var_name] = value
                elif var.default_value is not None:
                    dynamic_fields[var.var_name] = var.default_value

        # Build configuration
        config = {
            'component_name': args.component_name,
            'namespace': args.namespace,
            'component_type': template.display_name,
            'component_template': template.template_name,
            'wizard_template': template,  # Pass full template for command-driven processing
            'dest_dir': dest_dir,
            'keep_license': args.default_license,
            'strip_comments': not args.keep_comments,
            'automatic_register': args.automatic_register,
            'build_target': None,
            'dynamic_fields': dynamic_fields,
        }

        # Create component
        creator = ComponentCreator(engine_path, logger=print)
        return creator.create_component(config)

    except Exception as e:
        print(f"Error: {e}")
        traceback.print_exc()
        return False


def main():
    """Main entry point with dynamic template discovery"""
    # First, do a minimal parse to get engine path for template discovery
    pre_parser = argparse.ArgumentParser(add_help=False)
    pre_parser.add_argument("--engine-path", type=str)
    pre_parser.add_argument("--project-path", type=str)
    pre_args, _ = pre_parser.parse_known_args()

    # Validate engine path early
    engine_path = None
    project_path = None

    if pre_args.engine_path:
        try:
            engine_path = validate_engine_path(pre_args.engine_path)
        except ValidationError as e:
            print(f"Error: {e}")
            return 1

    if pre_args.project_path:
        try:
            project_path = validate_path(pre_args.project_path)
        except ValidationError as e:
            print(f"Error: {e}")
            return 1

    # Discover templates if engine path is valid
    templates = []
    if engine_path:
        templates = discover_cli_templates(engine_path, project_path)

    # Build the main parser with discovered templates
    parser = build_dynamic_parser(templates)

    # Check for template-specific help first
    if "--template-help" in sys.argv:
        idx = sys.argv.index("--template-help")
        if idx + 1 < len(sys.argv):
            template_name = sys.argv[idx + 1]
            template = next((t for t in templates if t.class_name == template_name), None)
            if template:
                print_template_help(template)
                return 0
            else:
                print(f"Error: Unknown template '{template_name}'")
                print(f"Available templates: {', '.join(t.class_name for t in templates)}")
                return 1

    # Check for template selection and add template-specific args
    if "--template" in sys.argv:
        idx = sys.argv.index("--template")
        if idx + 1 < len(sys.argv):
            template_name = sys.argv[idx + 1]
            template = next((t for t in templates if t.class_name == template_name), None)
            if template:
                add_template_specific_args(parser, template)

    # Parse all arguments
    args = parser.parse_args()

    # Handle --list-templates
    if args.list_templates:
        if not templates:
            print("No wizard-enabled templates found.")
            print("Ensure template.json files contain 'class_wizard' configuration blocks.")
        else:
            list_templates(templates)
        return 0

    # Handle --template-help (if not already handled above)
    if args.template_help:
        template = next((t for t in templates if t.class_name == args.template_help), None)
        if template:
            print_template_help(template)
            return 0
        else:
            print(f"Error: Unknown template '{args.template_help}'")
            return 1

    # CLI mode vs GUI mode
    if args.template:
        # CLI mode - template specified
        if not project_path:
            print("Error: --project-path is required in CLI mode")
            return 1

        template = next((t for t in templates if t.class_name == args.template), None)
        if not template:
            print(f"Error: Unknown template '{args.template}'")
            return 1

        success = create_component_cli(args, template, engine_path, project_path)
        return 0 if success else 1
    else:
        # GUI mode - check if QApplication already exists
        app = QApplication.instance()
        if app is None:
            # Create new QApplication (standalone mode)
            app = QApplication(sys.argv)
            app.setStyle('Fusion')

            # Set application icon if available
            icon_path = engine_path / "Assets" / "Editor" / "UI" / "Icons" / "Editor Settings Manager.png"
            if icon_path.exists():
                app.setWindowIcon(QIcon(str(icon_path)))

            window = ClassWizardWindow(engine_path, project_path)
            window.show()

            return app.exec()
        else:
            # Use existing QApplication (inside O3DE Editor)
            # Use setAttribute to ensure window is deleted when closed
            window = ClassWizardWindow(engine_path, project_path)
            window.setAttribute(Qt.WA_DeleteOnClose, True)
            window.show()

            # Keep window alive - store reference
            if not hasattr(app, '_o3de_wizard_windows'):
                app._o3de_wizard_windows = []
            app._o3de_wizard_windows.append(window)

            # Clean up reference when window closes
            def cleanup():
                if window in app._o3de_wizard_windows:
                    app._o3de_wizard_windows.remove(window)

            window.destroyed.connect(cleanup)

            return 0

if __name__ == "__main__":
    try:
        result = main()
        # Don't call sys.exit when running inside O3DE Editor
        # The editor manages the lifecycle
        if QApplication.instance() and QApplication.instance() != QApplication.instance():
            sys.exit(result)
    except SystemExit:
        pass  # Ignore SystemExit when inside O3DE
    except Exception as e:
        print(f"Error: {e}")
        traceback.print_exc()