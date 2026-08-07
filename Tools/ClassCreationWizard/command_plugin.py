#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

"""
ClassWizard Command Plugin Infrastructure.

This module provides the base classes and registry for the command plugin system.
Command plugins are discovered from:
  1. Engine: ClassCreationWizard/commands/
  2. Project: <project>/ClassWizardCommands/
  3. Gems: <gem>/ClassWizardCommands/

To create a command plugin, create a .py file that:
  - Imports WizardCommand, CommandContext, CommandRegistry from this module
  - Decorates a WizardCommand subclass with @CommandRegistry.register("command_name")
  - Implements execute(ctx) and the name property
"""

import importlib.util
import json
import re
import sys
from abc import ABC, abstractmethod
from pathlib import Path
from typing import Optional, List, Dict, Any, Callable, Type


# ============================================================================
# Build System Types (used by commands that modify CMake files)
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
        """Scan a gem directory for CMake targets."""
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

        return results


# ============================================================================
# Command System Infrastructure
# ============================================================================

class CommandContext:
    """Execution context passed to all commands"""

    def __init__(self,
                 dest_root: Path,
                 namespace: str,
                 component_name: str,
                 build_target: Optional[CMakeTarget],
                 variables: Dict[str, Any],
                 logger: Callable[[str], None],
                 engine_path: Path,
                 copy_files: Optional[List] = None,
                 template_path: Optional[Path] = None,
                 stage_dir: Optional[Path] = None):
        self.dest_root = dest_root
        self.namespace = namespace
        self.component_name = component_name
        self.build_target = build_target
        self.variables = variables
        self.logger = logger
        self.engine_path = engine_path
        # List of (resolved_path: str, CopyFileDef) for active (condition-passing) files.
        # Commands use this to derive actual file paths rather than assuming Source/.
        self.copy_files: List = copy_files or []
        # Path to the source template directory (the folder that contains template.json
        # and the Template/ subfolder). Commands that need to read template-side
        # source files outside the staging pipeline -- for example asset files
        # routed to a non-Code destination -- consume this directly.
        self.template_path: Optional[Path] = template_path
        # Path to the live staging directory, valid for the duration of the
        # process_commands phase. Files marked excludeFromMerge=True in the
        # template's copyFiles list are still present here. copy_file_to and
        # copy_glob_to read from this. Cleared after process_commands finish.
        self.stage_dir: Optional[Path] = stage_dir

    def log(self, message: str):
        """Log a message using the provided logger"""
        self.logger(message)


class WizardCommand(ABC):
    """Abstract base class for all wizard commands.

    To create a command plugin:
      1. Subclass WizardCommand
      2. Decorate with @CommandRegistry.register("command_name")
      3. Implement execute() and the name property
      4. Set is_registration_command = True if this command should only run
         when automatic registration is enabled
    """

    @abstractmethod
    def execute(self, ctx: CommandContext) -> bool:
        """Execute the command. Returns True on success."""
        pass

    @property
    @abstractmethod
    def name(self) -> str:
        """Return the command name (matches template.json references)."""
        pass

    @property
    def is_registration_command(self) -> bool:
        """If True, this command only runs when automatic_register is enabled.
        Override as a class attribute in subclasses. Default False."""
        return False

    @property
    def description(self) -> str:
        """Human-readable one-liner for help/listing."""
        return ""

    @property
    def version(self) -> str:
        """Semver string for the command plugin."""
        return "1.0.0"

    @property
    def author(self) -> str:
        """Author/origin identifier."""
        return ""


class CommandRegistry:
    """Registry of available wizard commands.

    Commands self-register via the @CommandRegistry.register decorator.
    First registration wins -- duplicate names are logged but not overwritten.
    """

    _commands: Dict[str, Type[WizardCommand]] = {}
    _sources: Dict[str, str] = {}
    _logger: Optional[Callable[[str], None]] = None

    @classmethod
    def set_logger(cls, logger: Callable[[str], None]):
        cls._logger = logger

    @classmethod
    def register(cls, name: str):
        """Decorator to register a command class"""
        def decorator(command_class: Type[WizardCommand]):
            if name in cls._commands:
                if cls._logger:
                    existing = cls._sources.get(name, "unknown")
                    cls._logger(f"Warning: Command '{name}' already registered by '{existing}', ignoring duplicate")
                return command_class
            cls._commands[name] = command_class
            return command_class
        return decorator

    @classmethod
    def set_source(cls, name: str, source: str):
        """Record the source/origin of a registered command"""
        cls._sources[name] = source

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

    @classmethod
    def is_registration_command(cls, name: str) -> bool:
        """Query whether a command requires automatic_register to run"""
        cmd_class = cls._commands.get(name)
        if cmd_class is None:
            return False
        return getattr(cmd_class, 'is_registration_command', False)

    @classmethod
    def get_source(cls, name: str) -> str:
        """Get the origin of a registered command"""
        return cls._sources.get(name, "unknown")


# ============================================================================
# Plugin Loader
# ============================================================================

class CommandPluginLoader:
    """Discovers and loads command plugins from engine/project/gem directories.

    Load order (first registration wins):
      1. Engine: ClassCreationWizard/commands/
      2. Project: <project>/ClassWizardCommands/
      3. Gems: <gem>/ClassWizardCommands/ (alphabetical by gem name)
    """

    COMMAND_DIR_NAME = "ClassWizardCommands"
    ENGINE_COMMANDS_DIR = "commands"

    def __init__(self, logger: Optional[Callable[[str], None]] = None):
        self.logger = logger or print
        self._loaded_modules: Dict[str, Any] = {}

    def log(self, message: str):
        self.logger(message)

    def discover_and_load(self, engine_tool_path: Path,
                          project_path: Optional[Path] = None,
                          gem_paths: Optional[List[Path]] = None):
        """Load command plugins in priority order."""
        CommandRegistry.set_logger(self.logger)
        scanned: set = set()

        # 1. Engine built-in commands
        engine_cmds = engine_tool_path / self.ENGINE_COMMANDS_DIR
        if engine_cmds.is_dir():
            self._load_from_dir(engine_cmds, namespace="engine", scanned=scanned)

        # 2. Project commands
        if project_path:
            proj_cmds = project_path / self.COMMAND_DIR_NAME
            if proj_cmds.is_dir() and proj_cmds.resolve() not in scanned:
                self._load_from_dir(proj_cmds, namespace="project", scanned=scanned)

        # 3. Gem commands
        for gem_dir in sorted(gem_paths or [], key=lambda p: p.name):
            gem_cmds = gem_dir / self.COMMAND_DIR_NAME
            if gem_cmds.is_dir() and gem_cmds.resolve() not in scanned:
                self._load_from_dir(gem_cmds, namespace=gem_dir.name, scanned=scanned)

    def _load_from_dir(self, directory: Path, namespace: str, scanned: set):
        """Load all .py command files from a directory"""
        scanned.add(directory.resolve())
        count = 0
        for py_file in sorted(directory.glob("*.py")):
            if py_file.name.startswith("_"):
                continue
            if self._load_module(py_file, namespace):
                count += 1
        if count > 0:
            self.log(f"Loaded {count} command(s) from {namespace}: {directory}")

    def _load_module(self, py_file: Path, namespace: str) -> bool:
        """Load a single command plugin module"""
        module_name = f"cw_cmd_{namespace}_{py_file.stem}"
        if module_name in self._loaded_modules:
            return False

        try:
            spec = importlib.util.spec_from_file_location(module_name, py_file)
            if spec is None or spec.loader is None:
                self.log(f"Warning: Could not create spec for {py_file}")
                return False

            module = importlib.util.module_from_spec(spec)
            sys.modules[module_name] = module

            # Track commands before loading to detect new registrations
            before = set(CommandRegistry.list_commands())
            spec.loader.exec_module(module)
            after = set(CommandRegistry.list_commands())

            new_commands = after - before
            for cmd_name in new_commands:
                CommandRegistry.set_source(cmd_name, namespace)

            self._loaded_modules[module_name] = module
            return True
        except Exception as e:
            self.log(f"Warning: Failed to load command plugin {py_file.name}: {e}")
            return False
