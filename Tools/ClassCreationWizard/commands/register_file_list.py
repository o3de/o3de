#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

import re
from pathlib import Path
from typing import Optional, Tuple

from command_plugin import WizardCommand, CommandContext, CommandRegistry


@CommandRegistry.register("register_file_list")
class RegisterFileListCommand(WizardCommand):
    """Add component .h/.cpp files to CMake FILES_CMAKE"""

    is_registration_command = True

    def __init__(self, component_name: str):
        self.component_name = component_name

    @property
    def name(self) -> str:
        return "register_file_list"

    @property
    def description(self) -> str:
        return "Add component .h/.cpp files to CMake FILES_CMAKE"

    # ---------------------------------------------------------------
    # Path resolution
    # ---------------------------------------------------------------

    def _resolve_file_paths(self, ctx: CommandContext) -> Tuple[str, str, bool]:
        """Look up the actual .h/.cpp paths and is_editor flag from ctx.copy_files.

        ctx.copy_files holds (resolved_path, CopyFileDef) for all condition-passing
        files. We match on stem == component_name and use those paths verbatim rather
        than assuming Source/.

        Returns:
            (rel_hdr, rel_cpp, is_editor)
        """
        hdr, cpp = None, None
        is_editor = False

        for resolved_path, copy_def in ctx.copy_files:
            p = Path(resolved_path)
            if p.stem == self.component_name:
                if p.suffix == '.h':
                    hdr = resolved_path.replace('\\', '/')
                    is_editor = getattr(copy_def, 'is_editor', False)
                elif p.suffix == '.cpp':
                    cpp = resolved_path.replace('\\', '/')
            if hdr and cpp:
                break

        # Fall back to plain Source/ when no copy_files context is available
        if not hdr:
            hdr = f"Source/{self.component_name}.h"
        if not cpp:
            cpp = f"Source/{self.component_name}.cpp"

        return hdr, cpp, is_editor

    # ---------------------------------------------------------------
    # Main execute
    # ---------------------------------------------------------------

    def execute(self, ctx: CommandContext) -> bool:
        rel_hdr, rel_cpp, is_editor = self._resolve_file_paths(ctx)
        ctx.log(f"Registering files for '{self.component_name}' ({rel_hdr}, {rel_cpp})...")

        if is_editor:
            return self._register_editor_files(ctx, rel_hdr, rel_cpp)

        return self._register_runtime_files(ctx, rel_hdr, rel_cpp)

    # ---------------------------------------------------------------
    # Runtime cmake registration
    # ---------------------------------------------------------------

    def _register_runtime_files(self, ctx: CommandContext, rel_hdr: str, rel_cpp: str) -> bool:
        """Register files to the runtime build target."""
        if not ctx.build_target:
            ctx.log("Warning: No build target selected, skipping file registration")
            return True

        cmake_path = ctx.build_target.file

        if ctx.build_target.files_cmake_list:
            for files_cmake in ctx.build_target.files_cmake_list:
                include_path = (cmake_path.parent / files_cmake).resolve()
                if include_path.exists():
                    self._update_files_cmake(include_path, rel_hdr, rel_cpp, ctx)
                    ctx.log(f"Updated: {files_cmake}")
                    return True

        # Fall back: append target_sources directly to CMakeLists.txt
        self._append_target_sources(cmake_path, ctx.build_target.name, rel_hdr, rel_cpp, ctx)
        return True

    # ---------------------------------------------------------------
    # Editor cmake registration
    # ---------------------------------------------------------------

    def _register_editor_files(self, ctx: CommandContext, rel_hdr: str, rel_cpp: str) -> bool:
        """Register files to the editor module cmake target.

        Search order:
          1. CMakeLists.txt directory (Code/) -- preferred: *editor*private*files.cmake,
             then any *editor*files*.cmake, then any *editor*.cmake
          2. Source/Tools/ -- .cmake files with "files" or "editor" in the name
          3. Source/Tools/CMakeLists.txt -- append target_sources fallback
          4. Runtime build target (ctx.build_target) -- last resort

        This mirrors O3DE gem convention where editor cmake lists live at
        the same level as CMakeLists.txt, e.g. gs_core_editor_private_files.cmake.
        """
        # --- Pass 1: scan the CMakeLists.txt directory (Code/) ---
        search_dirs = []
        if ctx.build_target:
            search_dirs.append(ctx.build_target.file.parent)
        for code_rel in ("Code", ""):
            d = ctx.dest_root / code_rel if code_rel else ctx.dest_root
            if d.is_dir() and d not in search_dirs:
                search_dirs.append(d)

        for search_dir in search_dirs:
            cmake_file = self._find_editor_files_cmake(search_dir)
            if cmake_file:
                self._update_files_cmake(cmake_file, rel_hdr, rel_cpp, ctx)
                ctx.log(f"Updated editor files cmake: {cmake_file.name}")
                return True

        # --- Pass 2: Source/Tools/ directory cmake ---
        tools_candidates = [
            ctx.dest_root / "Code" / "Source" / "Tools",
            ctx.dest_root / "Source" / "Tools",
        ]
        for tools_dir in tools_candidates:
            if not tools_dir.is_dir():
                continue
            for cmake_file in sorted(tools_dir.glob("*.cmake")):
                name_lower = cmake_file.name.lower()
                if "files" in name_lower or "editor" in name_lower:
                    self._update_files_cmake(cmake_file, rel_hdr, rel_cpp, ctx)
                    ctx.log(f"Updated editor files cmake: {cmake_file.name}")
                    return True
            cmake_lists = tools_dir / "CMakeLists.txt"
            if cmake_lists.exists():
                target_name = self._detect_editor_target_name(cmake_lists, ctx.namespace)
                self._append_target_sources(cmake_lists, target_name, rel_hdr, rel_cpp, ctx)
                return True

        # --- Pass 3: fall back to runtime build target ---
        if ctx.build_target:
            ctx.log("Warning: No editor cmake target found, adding to runtime build target")
            return self._register_runtime_files(ctx, rel_hdr, rel_cpp)

        ctx.log("Warning: Could not find any cmake target, skipping file registration")
        return True

    def _find_editor_files_cmake(self, directory: Path) -> Optional[Path]:
        """Return the best editor files cmake in a directory.

        Priority:
          1. *editor*private*files*.cmake  (e.g. gs_core_editor_private_files.cmake)
          2. *editor*files*.cmake          (any editor files list)
          3. *editor*.cmake                (any editor cmake)
        """
        if not directory.is_dir():
            return None

        candidates = list(directory.glob("*.cmake"))

        for cmake_file in sorted(candidates):
            n = cmake_file.name.lower()
            if "editor" in n and "private" in n and "files" in n:
                return cmake_file

        for cmake_file in sorted(candidates):
            n = cmake_file.name.lower()
            if "editor" in n and "files" in n:
                return cmake_file

        for cmake_file in sorted(candidates):
            n = cmake_file.name.lower()
            if "editor" in n and "api" not in n:
                return cmake_file

        return None

    def _detect_editor_target_name(self, cmake_lists: Path, namespace: str) -> str:
        """Extract the target NAME from a CMakeLists.txt; falls back to namespace.Editor."""
        try:
            text = cmake_lists.read_text(encoding="utf-8")
            match = re.search(r'\bNAME\s+([^\s\)]+)', text)
            if match:
                raw = match.group(1).strip('"\'')
                raw = raw.replace('${GemName}', namespace).replace('${gem_name}', namespace)
                return raw
        except Exception:
            pass
        return f"{namespace}.Editor"

    # ---------------------------------------------------------------
    # Shared cmake helpers
    # ---------------------------------------------------------------

    def _append_target_sources(self, cmake_path: Path, target_name: str,
                                rel_hdr: str, rel_cpp: str, ctx: CommandContext):
        """Append a target_sources() block to a CMakeLists.txt."""
        cmake_text = cmake_path.read_text(encoding="utf-8")
        append = (
            f"\n# Added by Class Wizard\n"
            f"target_sources({target_name} PRIVATE\n"
            f"    {rel_hdr}\n"
            f"    {rel_cpp}\n"
            f")\n"
        )
        if append not in cmake_text:
            cmake_text = cmake_text.rstrip() + append
            cmake_path.write_text(cmake_text, encoding="utf-8", newline="\n")
            ctx.log(f"Added target_sources block to {cmake_path.name}")

    def _update_files_cmake(self, files_cmake_path: Path, rel_hdr: str, rel_cpp: str,
                             ctx: CommandContext):
        """Insert header and cpp into a FILES_CMAKE set(FILES ...) block."""
        if files_cmake_path.exists():
            text = files_cmake_path.read_text(encoding="utf-8")
        else:
            text = "set(FILES\n)\n"

        match = re.search(r'set\s*\(\s*FILES\b(.*?)(\))', text, flags=re.S | re.M)
        if match:
            end_pos = match.end(1)
            if rel_hdr not in text:
                text = text[:end_pos] + f"    {rel_hdr}\n" + text[end_pos:]

            match = re.search(r'set\s*\(\s*FILES\b(.*?)(\))', text, flags=re.S | re.M)
            if match:
                end_pos = match.end(1)
                if rel_cpp not in text:
                    text = text[:end_pos] + f"    {rel_cpp}\n" + text[end_pos:]
        else:
            text = text.rstrip() + f"\nset(FILES\n    {rel_hdr}\n    {rel_cpp}\n)\n"

        files_cmake_path.write_text(text, encoding="utf-8", newline="\n")
