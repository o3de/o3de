#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

import re
from pathlib import Path
from typing import List, Tuple, Optional, Any, Callable


class ConditionalFileExcluder:
    """Handles conditional file exclusion and reference cleanup during staging.

    Called as an automatic staging step before process_commands run.
    Iterates CopyFileDef entries, deletes files whose conditions evaluate to
    false, then scrubs references to those files from the remaining staged files.

    Cleanup hint values (from CopyFileDef.cleanup_hint):
        "interface"  -- strip EBus includes, handler inheritance, and
                        BusConnect/BusDisconnect calls
        "editor"     -- strip #include lines referencing the excluded file's header
        None         -- delete the file only, no reference scrubbing
    """

    def __init__(self, logger: Optional[Callable[[str], None]] = None):
        self.logger = logger or (lambda msg: None)

    def log(self, msg: str):
        self.logger(msg)

    # ---------------------------------------------------------------
    # Main entry point
    # ---------------------------------------------------------------

    def process(self, stage_dir: Path, copy_files: List[Any], resolver: Any) -> None:
        """Evaluate conditions, remove excluded files, and clean up references.

        Args:
            stage_dir:   Staging directory containing generated files
            copy_files:  List of CopyFileDef instances from WizardTemplate
            resolver:    VariableResolver with current variable context
        """
        # Phase 1: determine which files to skip and collect cleanup data
        files_to_skip = set()
        interface_data: List[Tuple[str, List[str]]] = []  # (filename, bus_list)
        editor_filenames: List[str] = []

        for copy_def in copy_files:
            if not copy_def.condition or resolver.evaluate_condition(copy_def.condition):
                continue

            resolved_file = resolver.resolve(copy_def.file)
            files_to_skip.add(resolved_file)
            self.log(f"Skipping file (condition not met): {resolved_file}")

            stage_file = stage_dir / resolved_file
            hint = copy_def.cleanup_hint

            if hint == "interface" and stage_file.exists():
                buses = self._extract_bus_names(stage_file)
                interface_data.append((stage_file.name, buses))
                self.log(f"Interface '{stage_file.name}' excluded -- buses: {buses}")

            elif hint == "editor" and stage_file.exists():
                editor_filenames.append(stage_file.name)
                self.log(f"Editor file '{stage_file.name}' excluded")

        # Phase 2: delete skipped files from staging, then prune empty directories
        for skip_file in files_to_skip:
            skip_path = stage_dir / skip_file
            if skip_path.exists():
                skip_path.unlink()
                self.log(f"Removed from stage: {skip_file}")

        self._prune_empty_dirs(stage_dir)

        # Phase 3: scrub references in remaining staged files
        if interface_data:
            self._clean_interface_references(stage_dir, interface_data)

        if editor_filenames:
            self._clean_editor_references(stage_dir, editor_filenames)

    # ---------------------------------------------------------------
    # Bus name extraction
    # ---------------------------------------------------------------

    def _extract_bus_names(self, interface_file: Path) -> List[str]:
        """Extract all EBus type names declared in an interface header.

        Matches:
            using FooBus = AZ::EBus<...>;
            typedef AZ::EBus<...> BarBus;
        """
        text = interface_file.read_text(encoding="utf-8")
        buses = []
        buses.extend(re.findall(r'using\s+(\w+)\s*=\s*AZ::EBus<', text))
        buses.extend(re.findall(r'typedef\s+AZ::EBus<[^>]+>\s+(\w+)\s*;', text))
        return buses

    # ---------------------------------------------------------------
    # Reference cleanup
    # ---------------------------------------------------------------

    def _clean_interface_references(self, stage_dir: Path,
                                     interface_data: List[Tuple[str, List[str]]]) -> None:
        """Remove EBus-related references from remaining staged files.

        Removes:
            - #include lines for the excluded interface header
            - public BusName::Handler base class entries
            - BusName::Handler::BusConnect(...) and BusDisconnect(...) calls
        """
        interface_filenames = [name for name, _ in interface_data]
        all_buses: List[str] = []
        for _, buses in interface_data:
            all_buses.extend(buses)

        self.log(f"Cleaning interface references: {interface_filenames} (buses: {all_buses})")

        for source_file in stage_dir.rglob("*"):
            if not source_file.is_file() or source_file.suffix not in ('.h', '.cpp'):
                continue

            text = source_file.read_text(encoding="utf-8")
            original = text

            for name in interface_filenames:
                base = Path(name).stem
                text = re.sub(
                    rf'^#include\s+[<"].*?{re.escape(base)}\.h[>"]\s*\n',
                    '', text, flags=re.MULTILINE
                )

            for bus in all_buses:
                text = re.sub(
                    rf'\s*,\s*public\s+(?:\w+::)*{re.escape(bus)}::Handler',
                    '', text
                )
                text = re.sub(
                    rf'^\s*(?:\w+::)*{re.escape(bus)}::Handler::BusConnect\(.*?\);\s*\n',
                    '', text, flags=re.MULTILINE
                )
                text = re.sub(
                    rf'^\s*(?:\w+::)*{re.escape(bus)}::Handler::BusDisconnect\(.*?\);\s*\n',
                    '', text, flags=re.MULTILINE
                )

            if text != original:
                source_file.write_text(text, encoding="utf-8")
                self.log(f"Cleaned interface references from: {source_file.name}")

    def _clean_editor_references(self, stage_dir: Path,
                                  editor_filenames: List[str]) -> None:
        """Remove #include lines referencing excluded editor component files."""
        self.log(f"Cleaning editor file references: {editor_filenames}")

        for source_file in stage_dir.rglob("*"):
            if not source_file.is_file() or source_file.suffix not in ('.h', '.cpp'):
                continue

            text = source_file.read_text(encoding="utf-8")
            original = text

            for name in editor_filenames:
                base = Path(name).stem
                text = re.sub(
                    rf'^#include\s+[<"].*?{re.escape(base)}\.h[>"]\s*\n',
                    '', text, flags=re.MULTILINE
                )

            if text != original:
                source_file.write_text(text, encoding="utf-8")
                self.log(f"Cleaned editor references from: {source_file.name}")

    # ---------------------------------------------------------------
    # Staging cleanup
    # ---------------------------------------------------------------

    def _prune_empty_dirs(self, stage_dir: Path) -> None:
        """Remove empty subdirectories left behind after file exclusion.

        Walks bottom-up so nested empty directories are pruned in a single pass.
        The stage root itself is never removed.
        """
        for dirpath in sorted(stage_dir.rglob("*"), reverse=True):
            if dirpath == stage_dir or not dirpath.is_dir():
                continue
            try:
                dirpath.rmdir()  # only succeeds if directory is empty
                self.log(f"Pruned empty staging dir: {dirpath.relative_to(stage_dir)}")
            except OSError:
                pass  # not empty -- leave it alone
