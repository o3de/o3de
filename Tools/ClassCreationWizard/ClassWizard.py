"""
O3DE Class Creation Wizard - PySide6 Edition

A robust, class-based tool for creating C++ components in Open 3D Engine projects.
Supports multiple component types with automatic registration and intelligent
target detection.

Copyright (c) Contributors to the Open 3D Engine Project.
SPDX-License-Identifier: Apache-2.0 OR MIT
"""

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
from typing import Optional, List, Dict, Tuple, Any

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

COMPONENT_TYPE_TEMPLATES = {
    "Basic": "DefaultComponent",
    "System": "SystemComponent",
    "Level": "LevelComponent",
    "LyShine UI": "LyShineComponent",
    "Data Asset": "DataAsset"
}

COMPONENT_TYPE_SUFFIXES = {
    "Basic": "Component",
    "System": "SystemComponent",
    "Level": "LevelComponent",
    "LyShine UI": "Component",
    "Data Asset": "Asset"
}


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
    
    def log(self, message: str):
        """Log a message"""
        self.logger(message)
    
    def create_component(self, config: Dict[str, Any]) -> bool:
        """Create a component with the given configuration."""
        self.log("Starting component creation...")
        
        stage_dir = Path(tempfile.mkdtemp(prefix="cw_stage_"))
        self.log(f"Staging to: {stage_dir}")
        
        try:
            # Stage the component
            if not self._create_staged_component(
                stage_dir=stage_dir,
                namespace=config['namespace'],
                component_name=config['component_name'],
                component_template=config['component_template'],
                keep_license=config.get('keep_license', False)
            ):
                self.log("Failed to stage component")
                return False
            
            # Process files based on type
            self._process_files_by_type(
                stage_dir=stage_dir,
                config=config
            )
            
            setreg_path = None
            if config['component_type'] == "Data Asset":
                self.log("=== Processing Data Asset setreg ===")
                setreg_path = self._handle_setreg_file(
                    stage_dir, 
                    config['dest_dir'], 
                    config['namespace']
                )
                if setreg_path:
                    self.log(f"Setreg will be updated at: {setreg_path}")
                else:
                    self.log("WARNING: Setreg handling failed!")
            
            # Merge into destination
            created, skipped = self._merge_stage_into_dest(
                stage=stage_dir,
                dest=config['dest_dir'],
                skip_existing=True,
                strip_comments=config.get('strip_comments', True),
                keep_license=config.get('keep_license', False)
            )
            
            self.log(f"Created {created} file(s), skipped {skipped} existing file(s)")
            
            # Automatic registration
            if config.get('automatic_register', False):
                suffix = COMPONENT_TYPE_SUFFIXES.get(config['component_type'], 'Component')
                self._register_component(
                    dest_root=config['dest_dir'],
                    component_name=config['component_name'] + suffix,
                    component_type=config['component_type'],
                    namespace=config['namespace'],
                    target=config.get('build_target'),
                    config=config,
                    setreg_path=setreg_path
                )
            
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
                                keep_license: bool) -> bool:
        """Create the staged component using o3de create-from-template"""
        try:
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
    
    def _process_files_by_type(self, stage_dir: Path, config: Dict[str, Any]):
        """Perform type-specific processing"""
        component_type = config['component_type']
        
        if component_type == "System":
            pass  # Handled in registration
        
        elif component_type == "LyShine UI":
            self._add_gem_dependency(
                config['dest_dir'],
                config.get('build_target'),
                "Gem::LyShine"
            )
        
        elif component_type == "Data Asset":
            pass  # Handled in registration
    
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
        
    def _register_component(self, dest_root: Path, component_name: str,
                        component_type: str, namespace: str,
                        target: Optional[CMakeTarget],
                        config: Dict[str, Any],
                        setreg_path: Optional[Path] = None):
        """Register the component in the gem"""
        self.log("Registering component...")
        
        try:
            # Register in CMake files
            if target:
                self._register_file_list(dest_root, component_name, target)
            
            # Register in module descriptor (for Basic, Level, LyShine UI)
            if component_type in ["Basic", "Level", "LyShine UI"]:
                self._register_module_descriptor(dest_root, component_name, namespace, "runtime")
            
            # Register system component (for System type)
            if component_type == "System":
                self._register_module_descriptor(dest_root, component_name, namespace, "runtime")
                self._register_system_component(dest_root, namespace, component_name, "runtime")
                self._register_system_component(dest_root, namespace, component_name, "editor")
            
            # For Data Asset: register the DataAssetSystemComponent
            if component_type == "Data Asset":
                sys_comp_name = f"{namespace}DataAssetSystemComponent"
                
                # Get dynamic fields from config (passed from GUI)
                dynamic_fields = config.get('dynamic_fields', {})
                asset_ext = dynamic_fields.get('FileExtension', 'mydata')
                asset_group = dynamic_fields.get('Group', 'Other')
                
                self.log(f"Asset extension: {asset_ext}, Asset group: {asset_group}")
                
                # Register system component files
                if target:
                    self._register_file_list(dest_root, sys_comp_name, target)
                
                # Register in module descriptor
                self._register_module_descriptor(dest_root, sys_comp_name, namespace, "runtime")
                
                # Register as system component in both modules
                self._register_system_component(dest_root, namespace, sys_comp_name, "runtime")
                self._register_system_component(dest_root, namespace, sys_comp_name, "editor")
                
                # Register the asset itself in the system component
                self._register_generic_asset(dest_root, namespace, component_name, asset_ext, asset_group)
                
                # Update the setreg file (if it exists)
                if setreg_path:
                    self._register_asset_setreg(setreg_path, component_name, asset_ext, dest_root)
            
            self.log("Registration complete!")
            
        except Exception as e:
            self.log(f"Warning: Registration failed: {e}")
            import traceback
            self.log(traceback.format_exc())
    
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
        pattern = r'(m_descriptors\.insert\s*\(\s*m_descriptors\.end\s*\(\s*\)\s*,\s*\{)\s*([^}]*?)(\s*\}\s*\)\s*;)'
        
        match = re.search(pattern, text, flags=re.S)
        if match:
            prefix = match.group(1)
            inner = match.group(2)
            suffix = match.group(3)
            
            # Split into lines
            inner_lines = inner.split('\n')
            
            # Find existing descriptors to detect indent
            descriptor_indent = None
            for line in inner_lines:
                if '::CreateDescriptor()' in line:
                    descriptor_indent = re.match(r'(\s*)', line).group(1)
                    break
            
            if not descriptor_indent:
                # Default indent - 4 spaces more than the opening brace line
                base_match = re.match(r'(\s*)', prefix)
                base_indent = base_match.group(1) if base_match else ''
                descriptor_indent = base_indent + '    '
            
            # Remove trailing whitespace/empty lines
            while inner_lines and not inner_lines[-1].strip():
                inner_lines.pop()
            
            # Ensure last descriptor has a comma
            if inner_lines and inner_lines[-1].strip():
                last_line = inner_lines[-1]
                if not last_line.rstrip().endswith(','):
                    inner_lines[-1] = last_line.rstrip() + ','
            
            # Add new descriptor with consistent indent
            inner_lines.append(f'{descriptor_indent}{descriptor_line},')
            
            # Rebuild
            new_inner = '\n'.join(inner_lines)
            new_block = prefix + '\n' + new_inner + '\n' + suffix
            
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
            
            # Find indent from existing entries
            indent_match = re.search(r'\n(\s+)azrtti_typeid', list_content)
            if indent_match:
                indent = indent_match.group(1)
            else:
                # Default: reasonable indent
                indent = "                "  # 16 spaces
            
            # Split into lines and clean up
            lines = list_content.split('\n')
            cleaned = [line for line in lines if line.strip()]
            
            # Ensure last entry has comma
            if cleaned and not cleaned[-1].rstrip().endswith(','):
                cleaned[-1] = cleaned[-1].rstrip() + ','
            
            # Add new entry with consistent indent
            cleaned.append(f'{indent}{to_insert},')
            
            # Rebuild with proper formatting
            new_content = '\n' + '\n'.join(cleaned) + '\n'
            
            # Replace
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
        layout.addRow("Target:", self.target_combo)
        
        # Target path with browse
        path_layout = QHBoxLayout()
        self.target_path_edit = QLineEdit()
        self.browse_btn = QPushButton("...")
        self.browse_btn.setMaximumWidth(40)
        self.browse_btn.clicked.connect(self._browse_destination)
        path_layout.addWidget(self.target_path_edit)
        path_layout.addWidget(self.browse_btn)
        layout.addRow("Path:", path_layout)
        
        # Package (build target)
        self.package_combo = QComboBox()
        layout.addRow("Package:", self.package_combo)
        
        # Namespace
        self.namespace_edit = QLineEdit()
        if self.project_path:
            self.namespace_edit.setText(self.project_path.stem)
        layout.addRow("Namespace:", self.namespace_edit)
        
        group.setLayout(layout)
        return group
    
    def _create_component_section(self) -> QGroupBox:
        """Create the component details section"""
        group = QGroupBox("Component Details")
        layout = QFormLayout()
        
        # Component name
        self.component_name_edit = QLineEdit()
        self.component_name_edit.setPlaceholderText("Enter component name...")
        layout.addRow("Name:", self.component_name_edit)
        
        # Component type
        self.component_type_combo = QComboBox()
        self.component_type_combo.addItems([
            "Basic", "System", "Level", "LyShine UI", "Data Asset"
        ])
        self.component_type_combo.currentTextChanged.connect(self._on_component_type_changed)
        layout.addRow("Type:", self.component_type_combo)
        
        # Dynamic fields container
        self.dynamic_layout = QFormLayout()
        layout.addRow(self.dynamic_layout)
        
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
        self.remove_comments_check.setChecked(True)
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
            QLineEdit, QComboBox, QTextEdit {
                background-color: #3c3c3c;
                color: #cccccc;
                border: 1px solid #555555;
                border-radius: 3px;
                padding: 5px;
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
        """Load gems and targets for the current project"""
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
            
        except Exception as e:
            self.log(f"Error loading project data: {e}")
            QMessageBox.critical(self, "Error", f"Failed to load project data:\n{e}")
    
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
        # Clear dynamic fields
        while self.dynamic_layout.count():
            item = self.dynamic_layout.takeAt(0)
            if item.widget():
                item.widget().deleteLater()
        
        # Add type-specific fields
        if comp_type == "Data Asset":
            ext_edit = QLineEdit("mydata")
            ext_edit.setToolTip("File extension for the asset (without dot)")
            self.dynamic_layout.addRow("File Extension:", ext_edit)
            
            group_edit = QLineEdit("DataAssets")
            group_edit.setToolTip("Asset group name for organization")
            self.dynamic_layout.addRow("Group:", group_edit)
    
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
            
            # Gather dynamic fields
            dynamic_fields = {}
            for i in range(self.dynamic_layout.rowCount()):
                label_item = self.dynamic_layout.itemAt(i, QFormLayout.LabelRole)
                field_item = self.dynamic_layout.itemAt(i, QFormLayout.FieldRole)
                
                if label_item and field_item:
                    label = label_item.widget()
                    field = field_item.widget()
                    
                    if isinstance(label, QLabel) and isinstance(field, QLineEdit):
                        key = label.text().rstrip(':').replace(' ', '')
                        dynamic_fields[key] = field.text()
            
            # Build configuration
            component_type = self.component_type_combo.currentText()
            config = {
                'component_name': component_name,
                'namespace': namespace,
                'component_type': component_type,
                'component_template': COMPONENT_TYPE_TEMPLATES[component_type],
                'dest_dir': Path(dest_dir),
                'keep_license': self.default_license_check.isChecked(),
                'strip_comments': self.remove_comments_check.isChecked(),
                'automatic_register': self.auto_register_check.isChecked(),
                'build_target': selected_target,
                'dynamic_fields': dynamic_fields,
            }
            
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

def create_component_cli(args):
    """Create component from command line arguments"""
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
        
        if not args.component_type:
            print("Error: --component-type is required in CLI mode")
            return False
        
        if args.component_type not in COMPONENT_TYPE_TEMPLATES:
            print(f"Error: Invalid component type. Must be one of: {', '.join(COMPONENT_TYPE_TEMPLATES.keys())}")
            return False
        
        # Determine destination
        if args.target_path:
            dest_dir = Path(args.target_path)
        else:
            dest_dir = args.project_path / "Gem"
        
        if not dest_dir.is_dir():
            print(f"Error: Destination directory does not exist: {dest_dir}")
            return False
        
        # Build configuration
        config = {
            'component_name': args.component_name,
            'namespace': args.namespace,
            'component_type': args.component_type,
            'component_template': COMPONENT_TYPE_TEMPLATES[args.component_type],
            'dest_dir': dest_dir,
            'keep_license': args.default_license,
            'strip_comments': not args.keep_comments,
            'automatic_register': args.automatic_register,
            'build_target': None,
            'dynamic_fields': {},
        }
        
        # Create component
        creator = ComponentCreator(args.engine_path, logger=print)
        return creator.create_component(config)
    
    except Exception as e:
        print(f"Error: {e}")
        traceback.print_exc()
        return False


def main():
    """Main entry point"""
    parser = argparse.ArgumentParser(
        description="O3DE Class Creation Wizard",
        formatter_class=argparse.RawDescriptionHelpFormatter
    )
    
    # Required arguments
    parser.add_argument(
        "--engine-path",
        required=True,
        type=str,
        help="Path to O3DE engine root"
    )
    
    # GUI mode arguments
    parser.add_argument(
        "--project-path",
        type=str,
        help="Path to O3DE project (required for non-GUI mode)"
    )
    
    # CLI mode arguments
    parser.add_argument(
        "--component-name",
        type=str,
        help="Component name (enables CLI mode)"
    )
    
    parser.add_argument(
        "--component-type",
        type=str,
        choices=list(COMPONENT_TYPE_TEMPLATES.keys()),
        help="Component type"
    )
    
    parser.add_argument(
        "--namespace",
        type=str,
        help="Component namespace"
    )
    
    parser.add_argument(
        "--target-path",
        type=str,
        help="Destination path (defaults to project/Gem)"
    )
    
    parser.add_argument(
        "--automatic-register",
        action="store_true",
        help="Automatically register component"
    )
    
    parser.add_argument(
        "--default-license",
        action="store_true",
        help="Include default license header"
    )
    
    parser.add_argument(
        "--keep-comments",
        action="store_true",
        help="Keep comments in generated files"
    )
    
    args = parser.parse_args()
    
    # Validate engine path
    try:
        engine_path = validate_engine_path(args.engine_path)
    except ValidationError as e:
        print(f"Error: {e}")
        return 1
    
    # Validate project path if provided
    project_path = None
    if args.project_path:
        try:
            project_path = validate_path(args.project_path)
        except ValidationError as e:
            print(f"Error: {e}")
            return 1
    
    # CLI mode vs GUI mode
    if args.component_name:
        # CLI mode
        if not project_path:
            print("Error: --project-path is required in CLI mode")
            return 1
        
        args.engine_path = engine_path
        args.project_path = project_path
        success = create_component_cli(args)
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