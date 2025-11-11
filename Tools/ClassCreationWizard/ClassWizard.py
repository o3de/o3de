#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
r'''
ClassWizard.py

GUI Mode Examples:

Windows:
    PS C:\o3de> C:\o3de\python\python.cmd '.\Tools\ClassCreationWizard\ClassWizard.py' `
        --engine-path C:\o3de `
        --project-path C:\o3de\user\myproject
Linux:
    $ ~/o3de$ ./python/python.sh Tools/ClassCreationWizard/ClassWizard.py \
        --engine-path /home/yourusername/o3de \
        --project-path /home/yourusername/o3de/user/myproject

Example GUI Input:
    Component Details:
        Component Name:    Image
        Component Type:    Default
        Namespace:         myproject
        Project Directory: C:\o3de\user\myproject\Gem

    Settings:
        [X] Add to project
        [X] Default License

Non-GUI Mode Examples:
Activated using the flags: --component-name

Windows:
    PS C:\o3de> C:\o3de\python\python.cmd '.\Tools\ClassCreationWizard\ClassWizard.py' `
        --engine-path C:\o3de `
        --project-path C:\o3de\user\myproject `
        --component-name Image `
        --component-type Default `
        --namespace myproject `
        --automatic-register `
        --default-license
Linux:
    $ ~/o3de$ ./python/python.sh Tools/ClassCreationWizard/ClassWizard.py \
        --engine-path $HOME/o3de/ \
        --project-path $HOME/o3de/user/myproject \
        --component-name Image \
        --component-type Default \
        --namespace myproject \
        --automatic-register \
        --default-license

Required Arguments:
    --engine-path PATH    Path to O3DE engine root

Optional Arguments:
    --project-path   PATH Path to O3DE project (required for non-GUI)
    --component-name NAME Component name (required for non-GUI)
    --component-type TYPE Component type: Default or Editor (required for non-GUI)
    --namespace      NAME Component namespace (required for non-GUI)
    --automatic-register      Automatically add to project's Gem folder
    --default-license     Include default license
'''
import argparse
import os
import subprocess
from pathlib import Path
import sys, json
import re
import traceback
import tempfile
import shutil
import fnmatch
import tkinter as tk
import tkinter.font as tkFont
from tkinter import filedialog, ttk

#region --- Staging & Creation Functionality
COMMENT_FILE_GLOBS = ("**/*.h", "**/*.hpp", "**/*.c", "**/*.cpp", "**/*.inl")

# ----------------------------
# 1) Main staging coordinator
# ----------------------------
def _process_new_component(self):
    """Create the component using the specified settings (stage -> process -> merge)."""
    # Gather inputs
    self.clear_log()
    component_name = self.component_name.get().strip()

    if not component_name:
        self.log_message("Error: Component name is required!")
        return
    if not validate_component_name(component_name, log=self.log_message):
        return

    namespace = self.namespace.get().strip()
    if not namespace:
        self.log_message("Error: Namespace is required!")
        return
    if not validate_component_name(namespace, log=self.log_message):
        return

    # Prefer Target Path if you have it; fallback to Project Directory
    dest_dir = ""
    if hasattr(self, "target_path_var"):
        dest_dir = self.target_path_var.get().strip()
    if not dest_dir:
        dest_dir = self.project_dir_var.get().strip()

    if not os.path.isdir(dest_dir):
        self.log_message(f"Error: Destination directory {dest_dir or '<empty>'} does not exist.")
        return

    keep_license = self.default_license.get() if hasattr(self, 'default_license') else False
    strip_comments = self.remove_comments.get() if hasattr(self, 'remove_comments') else False

    
    component_type = self.component_type.get()
    component_suffix = "Component"
    if component_type == "Level":
        component_suffix = "LevelComponent"
    elif component_type == "System":
        component_suffix = "SystemComponent"
    elif component_type == "Data Asset":
        component_suffix = "Asset"

    # You said you keep this mapping elsewhere; if not, define it:
    # Example:
    # self._component_type_lookup = {
    #   "Basic": "DefaultComponent",
    #   "Editor": "DefaultEditorComponent",
    #   ...
    # }
    component_template = self._component_type_lookup[self.component_type.get()]

    self.log_message("Please wait…")
    self.root.update_idletasks()

    # ---------------------------
    # Stage: create into temp dir
    # ---------------------------
    stage_dir = Path(tempfile.mkdtemp(prefix="cw_stage_"))
    self.log_message(f"Staging to: {stage_dir}")

    ok = _create_staged_component(
        engine_path=self.engine_path,
        stage_dir=stage_dir,
        namespace=namespace,                 # pass STRINGS (not tk vars)
        component_name=component_name,       # pass STRINGS (not tk vars)
        component_template=component_template,
        keep_license=keep_license,
        log=self.log_message
    )
    if not ok:
        self.log_message("Failed to stage component.")
        try: shutil.rmtree(stage_dir, ignore_errors=True)
        except Exception: pass
        return

    # -------------------------------------
    # Process staged files based on type
    # (edits project CMake or staged bits)
    # -------------------------------------
    try:
        _process_files_by_type(self, stage_dir=stage_dir, dest_dir=Path(dest_dir), component_name=component_name, component_suffix=component_suffix, namespace=namespace)
    except Exception as e:
        self.log_message(f"Warning: post-processing failed: {e}")

    # --------------------
    # Merge stage -> dest
    # --------------------
    created, skipped = _merge_stage_into_dest(
        stage=stage_dir, dest=Path(dest_dir),
        skip_existing=True,
        strip_comments=strip_comments,
        keep_license=keep_license,
        log=self.log_message
    )
    self.log_message(f"Created {created} file(s), skipped {skipped} existing file(s).")

    # --------------------
    # Do Automatic Base Registration
    # --------------------
    automatic_register = self.automatic_register.get() if hasattr(self, "automatic_register") else False
    if automatic_register:
        success = _final_registration(
            self,
            dest_root=Path(dest_dir),            # gem root
            component_name=component_name,
            component_suffix=component_suffix,
            component_type=component_type,
            namespace=namespace,
            target_meta=None,                    # optional; will use current UI selection
            log=self.log_message
        )
        if not success:
            self.log_message("Warning: Failed to automatically register the component to the Destination.")
        else:
            self.log_message("Successfully registered component. The project may need to be rebuilt.")
    self.log_message("Full Component Creation Complete!")

    # Optionally keep stage for debugging; otherwise remove:
    try:
        shutil.rmtree(stage_dir, ignore_errors=True)
    except Exception:
        pass


# ---------------------------------------------
# 2) Call the O3DE templater to build the stage
# ---------------------------------------------
def _create_staged_component(engine_path, stage_dir, namespace, component_name,
                             component_template, keep_license=False, log=None) -> bool:
    """Creates the staged component using o3de create-from-template."""
    def log_message(msg):
        (log or print)(msg)

    try:
        # Prefer explicit template path to avoid manifest noise.
        engine_path = Path(engine_path)
        template_path = engine_path / "Templates" / component_template
        use_template_path = (template_path / "template.json").is_file()

        script_name = "o3de.bat" if sys.platform == "win32" else "o3de.sh"
        o3de_script = engine_path / "scripts" / script_name

        cmd = [str(o3de_script), "create-from-template", "-dp", str(stage_dir), "-dn", component_name]
        if use_template_path:
            cmd += ["-tp", str(template_path)]
        else:
            cmd += ["-tn", str(component_template)]

        # Template replacements
        cmd += ["-r", "${GemName}", namespace]

        # Keep restricted files inside the stage; keep license text if requested
        cmd.append("-kr")
        if keep_license:
            cmd.append("-kl")

        # Always force in the staging directory
        cmd.append("--force")

        log_message(f"Instantiating template '{component_template}' into stage…")
        result = subprocess.run(
            cmd,
            cwd=str(engine_path),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            check=True
        )

        # Show meaningful output
        if result.stdout:
            for line in result.stdout.splitlines():
                if line.strip():
                    log_message(line)
        if result.stderr:
            for line in result.stderr.splitlines():
                # Suppress info/warn noise if you like; I show everything here
                log_message(line)

        log_message(f"Successfully created staged component: {component_name}")
        return True

    except subprocess.CalledProcessError as e:
        log_message(f"Failed to create component (exit code {e.returncode})")
        if e.stdout: log_message(e.stdout)
        if e.stderr: log_message(e.stderr)
        return False
    except Exception as e:
        log_message(f"Error: {e}")
        return False


# ---------------------------------------------
# 3) Type-specific processing before the merge
# ---------------------------------------------
def _process_files_by_type(self, *, stage_dir: Path, dest_dir: Path, component_name: str, component_suffix: str, namespace: str):
    """
    Perform type-specific edits. For most cases, we want to update the
    **destination project's CMake** for the selected target, since that's
    where sources get registered.
    """
    component_type = self.component_type.get()
    self.log_message(("ProcessFilesByType " + str(component_type)))
    automatic_register = self.automatic_register.get() if hasattr(self, "automatic_register") else False
    editor_adapter = self.editor_adapter.get() if hasattr(self, "editor_adapter") else False

    if component_type == "System":
        if automatic_register:
            _register_system_component(self, dest_dir, namespace, (component_name + component_suffix), module_kind="runtime", log=self.log_message)
            if not editor_adapter:
                _register_system_component(self, dest_dir, namespace, (component_name + component_suffix), module_kind="editor", log=self.log_message)

    elif component_type == "LyShine UI":
        # Ensure the selected build target depends on LyShine (Gem::LyShine) so linking succeeds.
        self.log_message("Running LyShine UI Dependency")
        _add_gem_dependency_to_target(self, dependency="Gem::LyShine", dest_dir=dest_dir)

    elif component_type == "Data Asset":
        # Register the DataAssetSystemComponent
        if automatic_register:
            _register_file_list(self, (namespace + "DataAssetSystemComponent"), log=self.log_message)
            _register_module_description(self, dest_dir, (namespace + "DataAssetSystemComponent"), namespace, log=self.log_message)
            _register_system_component(self, dest_dir, namespace, (namespace + "DataAssetSystemComponent"), module_kind="runtime", log=self.log_message)
            if not editor_adapter:
                _register_system_component(self, dest_dir, namespace, (namespace + "DataAssetSystemComponent"), module_kind="editor", log=self.log_message)
            # Add Generic Asset Registration
            # If destination fails, then update the stage
            generic_success = _register_generic_asset(self, dest_dir, namespace, (component_name + component_suffix), self.dynamic_vars["AssetGroup"].get(), self.dynamic_vars["AssetExtension"].get(), log=self.log_message)
            if not generic_success:
                # Register at stage SysComp
                _register_generic_asset(self, stage_dir, namespace, (component_name + component_suffix), self.dynamic_vars["AssetGroup"].get(), self.dynamic_vars["AssetExtension"].get(), log=self.log_message)
            # Update the setreg
            _register_asset_setreg(self, dest_dir, stage_dir, (component_name + component_suffix), self.dynamic_vars["AssetExtension"].get(), log=self.log_message)

    # For Basic/Editor, no special pre-merge steps are required.


# ---------------------------------------------------
# 4) Add a Gem dependency to the selected CMake target
# ---------------------------------------------------
def _get_selected_target_meta(self):
    """Return the selected target metadata from self._build_targets_meta."""
    chosen = ""
    if hasattr(self, "build_target_choice"):
        chosen = self.build_target_choice.get()
    metas = getattr(self, "_build_targets_meta", []) or []
    for t in metas:
        nm = t.get("name") or t.get("raw_name")
        if nm == chosen:
            return t
    return None

def _add_gem_dependency_to_target(self, dependency: str, dest_dir: Path):
    meta = _get_selected_target_meta(self)
    if not meta:
        self.log_message("Warning: No build target selected; skipping dependency injection.")
        return

    cmake_path = Path(meta["file"])
    if not cmake_path.is_file():
        self.log_message(f"Warning: CMake file not found: {cmake_path}")
        return

    text = cmake_path.read_text(encoding="utf-8")

    # 1) Pick add_target block whose NAME contains ${gem_name} or ${GemName}
    macro_pat = r'(?:o3de_add_target|ly_add_target)\s*\((?P<body>.*?)\)\s*'
    chosen = None
    for m in re.finditer(macro_pat, text, flags=re.S | re.M):
        body = m.group('body')
        nm = re.search(r'\bNAME\s+(".*?"|[^\s\)]+)', body)
        if nm and ('${gem_name}' in nm.group(1) or '${GemName}' in nm.group(1)):
            chosen = (m, body); break
    if not chosen:
        self.log_message("Warning: No add_target block with NAME ${gem_name} found; nothing changed.")
        return
    m, body = chosen

    lines = body.splitlines(keepends=False)
    def indent_of(s: str) -> str: return re.match(r'^[ \t]*', s).group(0)

    # 2) Locate BUILD_DEPENDENCIES [dep_start, dep_end) - include PRIVATE/PUBLIC/INTERFACE inside the block
    dep_start = dep_end = None
    for i, ln in enumerate(lines):
        if re.match(r'^\s*BUILD_DEPENDENCIES\b', ln):
            dep_start = i
            dep_end = len(lines)
            for j in range(i + 1, len(lines)):
                if re.match(r'^\s*[A-Z_]+\b', lines[j]) and not re.match(r'^\s*(PRIVATE|PUBLIC|INTERFACE)\b', lines[j]):
                    dep_end = j
                    break
            break

    def find_section_range(section_name: str, start: int, end: int):
        """Return (sec_start, sec_end) for header+body inside deps; else (None,None)."""
        for k in range(start + 1, end):
            if re.match(rf'^\s*{section_name}\b', lines[k]):
                sec_end = end
                for t in range(k + 1, end):
                    if re.match(r'^\s*(PUBLIC|PRIVATE|INTERFACE|[A-Z_]+)\b', lines[t]):
                        sec_end = t; break
                return k, sec_end
        return None, None

    def dependency_exists(start: int, end: int) -> bool:
        """
        True if `dependency` already appears anywhere inside BUILD_DEPENDENCIES,
        regardless of section order or inline vs. newline formatting.
        """
        dep = dependency.strip()

        for idx in range(start + 1, end):
            # Remove inline comments and trim
            line = lines[idx].split('#', 1)[0].strip()
            if not line:
                continue

            # Drop section headers if present (works for "PUBLIC Foo Bar" and "PUBLIC" on own line)
            line = re.sub(r'^(PRIVATE|PUBLIC|INTERFACE)\b', '', line).strip()
            if not line:
                continue

            # Tokenize by whitespace and check exact token match
            if dep in re.split(r'\s+', line):
                return True

        return False

    # 3) Create BUILD_DEPENDENCIES if missing
    if dep_start is None:
        # choose indent from any header; else 4 spaces
        deps_indent = next((indent_of(ln) for ln in lines if re.match(r'^\s*[A-Z_]+\b', ln)), ' ' * 4)
        priv_indent = deps_indent + ' ' * 4
        item_indent = priv_indent + ' ' * 4
        if lines and lines[-1].strip(): lines.append('')
        lines += [
            f'{deps_indent}BUILD_DEPENDENCIES',
            f'{priv_indent}PRIVATE',
            f'{item_indent}{dependency}'
        ]
    else:
        # 4) If already present (PRIVATE or PUBLIC), bail
        if dependency_exists(dep_start, dep_end):
            self.log_message(f"Dependency already present: {dependency}")
            return

        # Prefer PRIVATE section; else create PRIVATE and insert
        s_priv, e_priv = find_section_range('PRIVATE', dep_start, dep_end)
        if s_priv is not None:
            # derive item indent
            item_indent = next((indent_of(ln) for ln in lines[s_priv+1:e_priv] if ln.strip()),
                               indent_of(lines[s_priv]) + ' ' * 4)
            lines.insert(e_priv, f'{item_indent}{dependency}')
        else:
            deps_header_indent = indent_of(lines[dep_start])
            priv_indent = deps_header_indent + ' ' * 4
            item_indent = priv_indent + ' ' * 4
            insert_at = dep_start + 1
            lines.insert(insert_at, f'{priv_indent}PRIVATE')
            lines.insert(insert_at + 1, f'{item_indent}{dependency}')

    # 5) Stitch and write
    new_body = '\n'.join(lines) + '\n'
    start, end = m.span()
    new_text = text[:start] + m.group(0).replace(body, new_body) + text[end:]
    cmake_path.write_text(new_text, encoding='utf-8', newline='\n')
    self.log_message(f"Added dependency to target ${'{'}gem_name{'}'}: {dependency}")

# -----------------------------------
# 5) Merge (skip existing + scrub)
# -----------------------------------
def _merge_stage_into_dest(stage: Path, dest: Path, *,
                           skip_existing: bool = True,
                           strip_comments: bool = True,
                           keep_license: bool = False,
                           comment_globs: tuple[str, ...] = COMMENT_FILE_GLOBS,
                           log=None) -> tuple[int, int]:
    """Copy files from stage into dest. Returns (created, skipped)."""
    created = skipped = 0
    stage = Path(stage); dest = Path(dest)
    dest.mkdir(parents=True, exist_ok=True)

    def _matches(rel: Path) -> bool:
        s = str(rel).replace("\\", "/")
        return any(fnmatch.fnmatch(s, pat) for pat in comment_globs)

    for s in stage.rglob("*"):
        if s.is_dir():
            continue
        rel = s.relative_to(stage)
        d = dest / rel
        d.parent.mkdir(parents=True, exist_ok=True)

        if skip_existing and d.exists():
            skipped += 1
            if log: log(f"skip existing: {rel}")
            continue

        if strip_comments and _matches(rel):
            try:
                data = s.read_text(encoding="utf-8")
            except UnicodeDecodeError:
                shutil.copy2(s, d)
            else:
                data = _strip_c_like_comments(data, preserve_license=keep_license)
                d.write_text(data, encoding="utf-8", newline="\n")
        else:
            shutil.copy2(s, d)

        created += 1
        if log: log(f"wrote: {rel}")

    return created, skipped

# Registration methods
def _detect_setreg_target(dest_root: Path) -> Path:
    """
    If dest_root is <project>/Gem/<gem>, return <project>/Registry/AssetProcessorGemConfig.setreg.
    Otherwise return <dest_root>/Registry/AssetProcessorGemConfig.setreg (external gem case).
    """
    dest_root = Path(dest_root)
    if dest_root.parent.name.lower() == "gem" and (dest_root.parent.parent / "project.json").is_file():
        # project gem layout
        return dest_root.parent / "Registry" / "AssetProcessorGemConfig.setreg"
    # external gem code
    elif dest_root.parent.name.lower() == "code" and (dest_root.parent.parent / "gem.json").is_file():
        return dest_root.parent.parent / "Registry" / "AssetProcessorGemConfig.setreg"
    # external gem
    return dest_root / "Registry" / "AssetProcessorGemConfig.setreg"

def _extract_asset_guid(asset_name: str, stage_dir: Path, dest_root: Path) -> str | None:
    """
    Try to extract the asset's type UUID.
    Priority:
      1) Stage: Source/<AssetName>.h / .cpp with AZ_RTTI or AZ_TYPE_INFO for AssetName
      2) Dest/Stage: *DataAssetSystemComponent.cpp via AZ_COMPONENT_IMPL(..., "{GUID}")
    Returns the GUID string with braces, e.g. "{12345678-...}" or None.
    """
    guid_pat = r'"\{[0-9A-Fa-f-]{36}\}"'
    # 1) Look for AZ_RTTI / AZ_TYPE_INFO in the asset class files
    for root in (stage_dir, dest_root):
        for ext in (".h", ".hpp", ".cpp"):
            p = Path(root) / "Source" / f"{asset_name}{ext}"
            if p.is_file():
                text = p.read_text(encoding="utf-8", errors="ignore")
                # AZ_RTTI(AssetName, "{GUID}", ...)
                m = re.search(rf'AZ_RTTI\s*\(\s*{re.escape(asset_name)}\s*,\s*({guid_pat})', text)
                if m:
                    return m.group(1).strip('"')
                # AZ_TYPE_INFO(AssetName, "{GUID}")
                m = re.search(rf'AZ_TYPE_INFO\s*\(\s*{re.escape(asset_name)}\s*,\s*({guid_pat})\s*\)', text)
                if m:
                    return m.group(1).strip('"')

    # 2) Fallback: GUID from the DataAssetSystemComponent AZ_COMPONENT_IMPL
    comp_regex = re.compile(r'AZ_COMPONENT_IMPL\s*\(\s*([A-Za-z0-9_:]+)\s*,\s*"[^"]*"\s*,\s*("?\{[0-9A-Fa-f-]{36}\}"?)\s*\)')
    for root in (stage_dir, dest_root):
        for p in Path(root).rglob("*DataAssetSystemComponent.cpp"):
            try:
                t = p.read_text(encoding="utf-8", errors="ignore")
            except Exception:
                continue
            m = comp_regex.search(t)
            if m:
                # second capture may be quoted or not
                g = m.group(2)
                return g.strip('"')

    return None

def _register_asset_setreg(self,
                           dest_root: Path,
                           stage_dir: Path,
                           asset_name: str,
                           file_extension: str,
                           log=None) -> bool:
    """
    Create/update AssetProcessorGemConfig.setreg with:
      "RC <AssetName>": {
          "glob": "*.ext",
          "params": "copy",
          "productAssetType": "{GUID}"
      }

    - Uses project-level Registry if dest_root is a project gem,
      otherwise uses the gem's own Registry.
    - If project setreg exists -> update it and DELETE staged file.
      Else -> create new in project Registry and DELETE staged file if present.
    - Idempotent: replaces/updates the specific RC block by key.
    """
    def say(msg): (log or print)(msg)

    dest_root = Path(dest_root)
    stage_dir = Path(stage_dir)

    # Find GUID
    guid = _extract_asset_guid(asset_name, stage_dir, dest_root)
    if not guid:
        say(f"Error: Could not extract GUID for asset '{asset_name}'. Aborting setreg update.")
        return False

    # Build/locate target setreg path
    target_setreg = _detect_setreg_target(dest_root)
    target_setreg.parent.mkdir(parents=True, exist_ok=True)

    # Load/seed JSON
    data = {
        "Amazon": {
            "AssetProcessor": {
                "Settings": {}
            }
        }
    }
    if target_setreg.exists():
        try:
            data = json.loads(target_setreg.read_text(encoding="utf-8"))
        except Exception:
            say(f"Warning: existing setreg is invalid JSON, recreating: {target_setreg}")

    # Ensure dict path exists
    ap_settings = data.setdefault("Amazon", {}).setdefault("AssetProcessor", {}).setdefault("Settings", {})

    # Upsert the RC block
    rc_key = f"RC {asset_name}"
    ap_settings[rc_key] = {
        "glob": f"*.{file_extension.lstrip('.')}",
        "params": "copy",
        "productAssetType": guid
    }

    # Write back with LF endings
    target_setreg.write_text(json.dumps(data, indent=4) + "\n", encoding="utf-8", newline="\n")
    say(f"Updated setreg: {target_setreg}")

    # Handle staged file cleanup/move
    staged_setreg = stage_dir / "AssetProcessorGemConfig.setreg"
    if staged_setreg.exists():
        # If we successfully wrote the project setreg, remove the staged one
        try:
            staged_setreg.unlink()
            say("Removed staged AssetProcessorGemConfig.setreg after merge.")
        except Exception as e:
            say(f"Warning: could not remove staged setreg: {e}")

    return True


def _register_generic_asset(self,
                            dest_root: Path,
                            gem_namespace: str,
                            asset_name: str,
                            asset_group: str,
                            file_extension: str,
                            log=None) -> bool:
    """
    Insert GenericAssetHandler registration into <GemName>DataAssetSystemComponent:
      - In Activate(): registers handler and stores in m_assetHandlers
      - In Reflect(): calls <AssetName>::Reflect(context)
      - Ensures needed includes
    Idempotent and indentation-aware. Writes with LF endings.
    """
    def say(msg): (log or print)(msg)

    dest_root = Path(dest_root)
    class_name = f"{gem_namespace}DataAssetSystemComponent"

    # ----------------------------
    # 1) Locate the .cpp file
    # ----------------------------
    candidates = [
        dest_root / "Code" / "Source" / f"{class_name}.cpp",
        dest_root / "Source" / f"{class_name}.cpp",
    ]
    cpp_path = next((p for p in candidates if p.is_file()), None)

    if cpp_path is None:
        # best-effort search; prefer files matching *DataAssetSystemComponent.cpp
        search_roots = []
        if (dest_root / "Code" / "Source").is_dir():
            search_roots.append(dest_root / "Code" / "Source")
        if (dest_root / "Source").is_dir():
            search_roots.append(dest_root / "Source")
        if not search_roots:
            search_roots.append(dest_root)

        for base in search_roots:
            for p in base.rglob("*DataAssetSystemComponent.cpp"):
                cpp_path = p
                # prefer an exact class match if we find multiple
                if p.name == f"{class_name}.cpp":
                    break
            if cpp_path:
                break

    if not cpp_path or not cpp_path.is_file():
        say(f"Error: Could not find {class_name}.cpp under {dest_root}")
        return False

    text = cpp_path.read_text(encoding="utf-8")

    # ----------------------------
    # 2) Ensure includes
    # ----------------------------
    need_includes = []
    hdr_include = f'#include "{asset_name}.h"'
    if hdr_include not in text:
        need_includes.append(hdr_include)

    if need_includes:
        lines = text.splitlines()
        last_inc = 0
        for i, ln in enumerate(lines):
            if ln.strip().startswith("#include"):
                last_inc = i
        for inc in need_includes:
            last_inc += 1
            lines.insert(last_inc, inc)
        text = "\n".join(lines) + "\n"

    # ----------------------------
    # helpers: find function body
    # ----------------------------
    def _find_method_body(src: str, qual_name: str, method: str):
        """
        Return (body_start, body_end, indent_sample) where [body_start:body_end] is inside '{...}'.
        """
        i = src.find(f"{qual_name}::{method}")
        if i == -1:
            return None
        # find '(' then matching ')'
        n = len(src)
        j = src.find('(', i)
        if j == -1:
            return None
        # match parens
        par = 1; k = j + 1
        while k < n and par:
            if src.startswith('/*', k):
                k2 = src.find('*/', k + 2)
                if k2 == -1: break
                k = k2 + 2; continue
            if src.startswith('//', k):
                k2 = src.find('\n', k)
                if k2 == -1: break
                k = k2 + 1; continue
            ch = src[k]
            if ch == '(':
                par += 1
            elif ch == ')':
                par -= 1
            k += 1
        if par != 0:
            return None
        # skip ws/quals to '{'
        m = re.search(r'\{', src[k:])
        if not m:
            return None
        brace_open = k + m.start()
        # match braces
        depth = 0; t = brace_open; body_start = brace_open + 1
        while t < n:
            if src.startswith('/*', t):
                t2 = src.find('*/', t + 2)
                if t2 == -1: return None
                t = t2 + 2; continue
            if src.startswith('//', t):
                t2 = src.find('\n', t)
                if t2 == -1: return None
                t = t2 + 1; continue
            ch = src[t]
            if ch == '{':
                depth += 1
            elif ch == '}':
                depth -= 1
                if depth == 0:
                    body_end = t
                    # derive indent from first non-empty line in body
                    body_text = src[body_start:body_end]
                    indent = " " * 8
                    for ln in body_text.splitlines():
                        if ln.strip():
                            indent = re.match(r'\s*', ln).group(0)
                            break
                    return (body_start, body_end, indent)
            t += 1
        return None

    # ----------------------------
    # 3) Insert into Activate()
    # ----------------------------
    act_span = _find_method_body(text, class_name, "Activate")
    if not act_span:
        say(f"Error: Could not locate {class_name}::Activate()")
        return False

    a_start, a_end, a_indent = act_span
    act_body = text[a_start:a_end]

    # If already present, skip
    if re.search(rf'GenericAssetHandler\s*<\s*{re.escape(asset_name)}\s*>\s*\(', act_body):
        pass
    else:
        # Prefer inserting after the template comment if it exists
        insert_pos = 0
        m_comment = re.search(r'//\s*Register\s+Generic\s+Assets.*', act_body)
        if m_comment:
            insert_pos = m_comment.end()
            # move to end of that line
            nl = act_body.find("\n", insert_pos)
            insert_pos = nl + 1 if nl != -1 else len(act_body)

        # Build block with the body's indent
        block_lines = [
            f'{a_indent}auto* {asset_name}Handler = aznew AzFramework::GenericAssetHandler<{asset_name}>("{asset_name}", "{asset_group}", "{file_extension}");',
            f'{a_indent}{asset_name}Handler->Register();',
            f'{a_indent}m_assetHandlers.emplace_back({asset_name}Handler);',
        ]
        block_text = "\n".join(block_lines) + "\n"

        # Splice into body
        new_act_body = act_body[:insert_pos] + block_text + act_body[insert_pos:]
        text = text[:a_start] + new_act_body + text[a_end:]

    # ----------------------------
    # 4) Insert into Reflect()
    # ----------------------------
    ref_span = _find_method_body(text, class_name, "Reflect")
    if not ref_span:
        say(f"Error: Could not locate {class_name}::Reflect(...)")
        return False

    r_start, r_end, r_indent = ref_span
    ref_body = text[r_start:r_end]

    if re.search(rf'\b{re.escape(asset_name)}\s*::\s*Reflect\s*\(\s*context\s*\)\s*;', ref_body):
        pass
    else:
        # Prefer after a template comment if present
        insert_pos = 0
        m_comment = re.search(r'//\s*Reflect\s+the\s+Assets\s+Reflect\s+Methods.*', ref_body)
        if m_comment:
            insert_pos = m_comment.end()
            nl = ref_body.find("\n", insert_pos)
            insert_pos = nl + 1 if nl != -1 else len(ref_body)

        line = f"{r_indent}{asset_name}::Reflect(context);\n"
        new_ref_body = ref_body[:insert_pos] + line + ref_body[insert_pos:]
        text = text[:r_start] + new_ref_body + text[r_end:]

    # ----------------------------
    # 5) Write back (LF)
    # ----------------------------
    cpp_path.write_text(text, encoding="utf-8", newline="\n")
    say(f"Registered GenericAsset for '{asset_name}' in {cpp_path}")
    return True


def _register_system_component(self,
                               dest_root: Path,
                               namespace: str,
                               system_component_class: str,
                               *,
                               module_kind: str = "runtime",   # "runtime" or "editor"
                               log=None) -> bool:
    """
    Insert a SystemComponent into the selected module (runtime or editor):
      - Adds #include "<system_component_class>.h" if missing
      - Adds azrtti_typeid<Namespace::SystemComponent>() to GetRequiredSystemComponents()
    Assumptions:
      - The module .cpp exists (e.g., <Namespace>Module.cpp or <Namespace>EditorModule.cpp)
      - GetRequiredSystemComponents() exists and returns AZ::ComponentTypeList{ ... }
    If the function isn't found, we log a clear message and return False (safer than guessing).
    """
    def say(msg): (log or print)(msg)

    dest_root = Path(dest_root)

    # ----------------------------
    # 1) Locate the module file
    # ----------------------------
    # Preferred exact names:
    exact = f"{namespace}EditorModule.cpp" if module_kind == "editor" else f"{namespace}Module.cpp"
    candidates = [
        dest_root / "Code" / "Source" / "Tools" / exact,
        dest_root / "Code" / "Source" / exact,
        dest_root / "Source" / exact,
        dest_root / "Source" / "Tools" / exact,
        dest_root / "Code" / "Source" / f"{namespace}ModuleInterface.cpp",
        dest_root / "Source" / f"{namespace}ModuleInterface.cpp"
    ]
    module_path = next((p for p in candidates if p.is_file()), None)

    # If not found, do a best-effort search
    if module_path is None:
        pattern = "*EditorModule.cpp" if module_kind == "editor" else "*Module.cpp"
        # Prefer Code/Source
        code_src = dest_root / "Code" / "Source"
        search_dirs = ([code_src] if code_src.is_dir() else []) + [dest_root]
        for base in search_dirs:
            for p in base.rglob(pattern):
                # If multiple, prefer the one containing the namespace in the filename
                if module_path is None:
                    module_path = p
                elif exact and p.name == exact:
                    module_path = p
                    break
            if module_path:
                break

    if not module_path or not module_path.is_file():
        say(f"Error: Could not locate {'Editor' if module_kind=='editor' else 'Runtime'} module .cpp under {dest_root}")
        return False

    text = module_path.read_text(encoding="utf-8")

    # ----------------------------
    # 2) Ensure the include line
    # ----------------------------
    hdr_include = f'#include "{system_component_class}.h"'
    if hdr_include not in text:
        lines = text.splitlines()
        last_inc = 0
        for i, ln in enumerate(lines):
            if ln.strip().startswith("#include"):
                last_inc = i
        lines.insert(last_inc + 1, hdr_include)
        text = "\n".join(lines) + "\n"

    # ---------------------------------------------------------
    # 3) Insert into GetRequiredSystemComponents() initializer
    # ---------------------------------------------------------
    # Normalize a fully-qualified type for azrtti_typeid
    fq_component = f"{namespace}::{system_component_class}" if "::" not in system_component_class else system_component_class
    to_insert = f"azrtti_typeid<{fq_component}>()"

    # Find the function body: return AZ::ComponentTypeList{ ... };
    # We capture the list body between '{' and '};'
    func_pat = (
        r'GetRequiredSystemComponents\s*\(\s*\)\s*'
        r'(?:\s*(?:const|override|noexcept|final))*\s*'   # <- allow qualifiers in any order/amount
        r'\{'
        r'(?:(?!\}).)*?'                                   # lazy up to return
        r'return\s+AZ::ComponentTypeList\s*\{\s*'
        r'(?P<body>.*?)'
        r'\s*\}\s*;'
        r'(?:(?!\}).)*?'
        r'\}'
    )
    m_func = re.search(func_pat, text, flags=re.S)
    if not m_func:
        say("Error: Could not find GetRequiredSystemComponents() body; refusing to guess.")
        say("       Please ensure your module defines the method and returns AZ::ComponentTypeList{ ... }.")
        return False

    body = m_func.group('body')

    # If already present, we are done with the list
    if re.search(rf'\bazrtti_typeid\s*<\s*{re.escape(fq_component)}\s*>\s*\(\s*\)', body):
        list_done = True
        new_body = body
    else:
        list_done = False
        # Determine indent from the FIRST existing azrtti_typeid<> line, if any
        first_indent = None
        existing_lines = body.splitlines()
        for ln in existing_lines:
            if "azrtti_typeid" in ln:
                first_indent = re.match(r'\s*', ln).group(0)
                break
        if first_indent is None:
            # Fallback: indent relative to the "return AZ::ComponentTypeList{" line.
            # Try to find that line's indent
            # Scan backwards from function start to the 'return' line for indent (best-effort)
            # Simpler: use 12 spaces as a reasonable default (matches O3DE templates well)
            first_indent = " " * 12

        # Ensure the previous meaningful line ends with a comma
        trimmed = [ln for ln in existing_lines if ln.strip()]
        if trimmed:
            last_line = trimmed[-1]
            if not last_line.strip().endswith(','):
                # Replace last occurrence of that line in body with a comma-terminated version
                # (operate on the raw body to preserve other whitespace)
                last_line_escaped = re.escape(last_line)
                body = re.sub(last_line_escaped + r'\s*$', last_line.rstrip() + ',', body, count=1, flags=re.M)

        # Append our new entry with matching indent and ensure a trailing comma
        insertion_line = f"{first_indent}{to_insert},"
        # Place just before the closing '};' of the return list => we're working inside body only
        if body and not body.endswith("\n"):
            body += "\n"
        new_body = body + insertion_line + "\n"

    # Rebuild the function with the modified body
    if not list_done:
        start, end = m_func.span('body')
        text = text[:start] + new_body + text[end:]

    # ----------------------------
    # 4) Write back with LF endings
    # ----------------------------
    module_path.write_text(text, encoding="utf-8", newline="\n")

    kind_label = "Editor" if module_kind == "editor" else "Runtime"
    say(f"Registered {fq_component} into {kind_label} module: {module_path}")
    return True

def _register_file_list(self,
                        component_name: str,
                        target_meta: dict | None = None,
                        log=None):
    def say(msg): (log or print)(msg)

    if target_meta is None:
        target_meta = _get_selected_target_meta(self)
    if not target_meta:
        say("Error: No build target selected/found; cannot register files.")
        return False

    cmake_path: Path = Path(target_meta["file"])
    target_name = target_meta.get("name") or target_meta.get("raw_name")
    files_list = target_meta.get("files_cmake_list") or []
    if not cmake_path.is_file():
        say(f"Error: CMake file for target not found: {cmake_path}")
        return False

    say(f"Registering in target '{target_name}' ({cmake_path})")

    rel_hdr = f"Source/{component_name}.h"
    rel_cpp = f"Source/{component_name}.cpp"

    cmake_text = cmake_path.read_text(encoding="utf-8")

    # Find the macro block for this target (handles ${gem_name})
    macro_pat = r'(?:o3de_add_target|ly_add_target)\s*\((?P<body>.*?)\)\s*'
    blocks = list(re.finditer(macro_pat, cmake_text, flags=re.S | re.M))

    def _cmake_name_matches(raw: str, resolved: str) -> bool:
        raw = raw.strip('"\'')
        if raw == resolved:
            return True
        # Resolve common gem name variables for comparison
        for var in ("${GemName}", "${gem_name}", "${GEM_NAME}"):
            if var in raw:
                # Permit suffix forms like ${gem_name}.Private.Object
                candidate = raw.replace(var, resolved.split('.', 1)[0])
                if candidate == resolved:
                    return True
        # Loose match: allow "${gem_name}.*" to match resolved suffix
        if "${" in raw and "." in resolved:
            suffix = "." + ".".join(resolved.split(".")[1:])
            if raw.endswith(suffix):
                return True
        return False

    chosen_block = None
    for m in blocks:
        body = m.group("body") or ""
        nm = re.search(r'\bNAME\s+(".*?"|[^\s\)]+)', body)
        if not nm:
            continue
        raw = nm.group(1).strip('"\'')
        if _cmake_name_matches(raw, target_name):
            chosen_block = m
            break
    if not chosen_block and blocks:
        chosen_block = blocks[0]

    def _update_one_files_cmake(include_rel: str) -> bool:
        """Append rel_cpp/rel_hdr into the files cmake; create it if missing."""
        include_path = (cmake_path.parent / include_rel).resolve()
        include_path.parent.mkdir(parents=True, exist_ok=True)

        if include_path.exists():
            txt = include_path.read_text(encoding="utf-8")
        else:
            txt = "set(FILES\n)\n"

        mfs = re.search(r'set\s*\(\s*FILES\b(.*?)(\))', txt, flags=re.S | re.M)
        if mfs:
            end = mfs.end(1)  # before ')'
            def ensure(txt0, rel):
                line = f"    {rel}\n"
                return txt0 if re.search(rf'^\s*{re.escape(rel)}\s*$', txt0, flags=re.M) else txt0[:end] + line + txt0[end:]
            txt2 = ensure(txt, rel_hdr)
            # re-compute end for second insert (txt length changed)
            mfs2 = re.search(r'set\s*\(\s*FILES\b(.*?)(\))', txt2, flags=re.S | re.M)
            end2 = mfs2.end(1) if mfs2 else len(txt2)
            txt3 = ensure(txt2, rel_cpp) if not re.search(rf'^\s*{re.escape(rel_cpp)}\s*$', txt2, flags=re.M) else txt2
            include_path.write_text(txt3, encoding="utf-8", newline="\n")
            return True
        else:
            txt = txt.rstrip() + f"\nset(FILES\n    {rel_hdr}\n    {rel_cpp}\n)\n"
            include_path.write_text(txt, encoding="utf-8", newline="\n")
            return True

    # Prefer a non-platform FILES_CMAKE include (no ${pal_dir}), else choose one that exists/has set(FILES)
    chosen_include = None
    if files_list:
        non_pal = [p for p in files_list if "pal_dir" not in p and "PAL_PLATFORM" not in p]
        candidates = non_pal or files_list

        # try existing files that look real
        for rel in candidates:
            p = (cmake_path.parent / rel).resolve()
            if p.exists():
                chosen_include = rel
                break
        if not chosen_include:
            chosen_include = candidates[0]

    files_added = False
    if chosen_include:
        if _update_one_files_cmake(chosen_include):
            (log or print)(f"Updated files list: {chosen_include}")
            files_added = True
    else:
        # No FILES_CMAKE in this target: append target_sources fallback
        if chosen_block:
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
                say(f"Appended target_sources() block for {target_name}")
                files_added = True
        else:
            # Plain CMake
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
                say(f"Appended target_sources() block for {target_name}")
                files_added = True

    if not files_added:
        say("Warning: could not determine a files list to update.")
    return files_added

def _register_module_description(self, 
                        dest_root: Path,
                        component_name: str,
                        namespace: str,
                        log=None):
    # ------------------------------------------------
    # 2) Update Gem Module: include + descriptor line
    # ------------------------------------------------
    def say(msg): (log or print)(msg)
    # Try common locations; prefer Code/Source first
    candidates = [
        dest_root / "Code" / "Source" / f"{namespace}Module.cpp",
        dest_root / "Source" / f"{namespace}Module.cpp",
        dest_root / "Code" / "Source" / f"{namespace}ModuleInterface.cpp",
        dest_root / "Source" / f"{namespace}ModuleInterface.cpp",
    ]
    module_path = None
    for p in candidates:
        if p.is_file():
            module_path = p
            break
    if not module_path:
        # Fallback: search
        for p in (dest_root / "Code").rglob("*Module.cpp") if (dest_root / "Code").exists() else dest_root.rglob("*Module.cpp"):
            # Prefer files whose name matches namespace
            if p.name == f"{namespace}Module.cpp":
                module_path = p
                break
        if not module_path:
            # As a last chance, pick the first Module.cpp under Code/Source
            for p in (dest_root / "Code" / "Source").glob("*Module.cpp"):
                module_path = p
                break

    if not module_path or not module_path.exists():
        say(f"Error: Module file not found under {dest_root}.")
        say(" Hint: ensure the destination is the Gem root (contains Code/Source and a Module.cpp).")
        return False

    mod_text = module_path.read_text(encoding="utf-8")
    include_line = f'#include "{component_name}.h"'

    # Insert include after last #include if not present
    if include_line not in mod_text:
        lines = mod_text.splitlines()
        last_inc = 0
        for i, line in enumerate(lines):
            if line.strip().startswith("#include"):
                last_inc = i
        lines.insert(last_inc + 1, include_line)
        mod_text = "\n".join(lines) + "\n"

    # ------------------------------------------------
    # Insert descriptor inside m_descriptors.insert({ ... });
    # - one extra TAB deeper than existing entries
    # - ensure LF before closing "});"
    # ------------------------------------------------
    descriptor_line = f"{component_name}::CreateDescriptor()"

    pat = (
        r'(m_descriptors\.insert\s*\(\s*m_descriptors\.end\s*\(\s*\)\s*,\s*\{\s*)'
        r'(?P<inner>.*?)'
        r'(\s*\}\s*\)\s*;)'
    )
    m = re.search(pat, mod_text, flags=re.S)
    if m:
        pre  = m.group(1)
        inner = m.group('inner')
        post = m.group(3)  # contains "});" (possibly with surrounding whitespace)

        # Work on inner as lines (LF discipline)
        inner_lines = inner.splitlines()

        # Trim trailing blank lines from inner
        while inner_lines and not inner_lines[-1].strip():
            inner_lines.pop()

        # Determine base indent from the last non-empty inner line (or a default)
        base_indent = " " * 12
        if inner_lines:
            last_sig = inner_lines[-1]
            base_indent = re.match(r'\s*', last_sig).group(0)

            # Ensure the previous line ends with a comma
            if not last_sig.strip().endswith(','):
                inner_lines[-1] = last_sig.rstrip() + ','

        # Our new descriptor line: one TAB more nested than existing lines
        new_indent = base_indent + '\t'
        inner_lines.append(f"{new_indent}{descriptor_line},")

        # Recompose inner with LF and ensure a final LF before closing
        adjusted_inner = "\n".join(inner_lines)

        # Force LF before the closing "});"
        new_block = pre + adjusted_inner + "\n" + post.lstrip()

        # Splice back into the file
        mod_text = mod_text[:m.start()] + new_block + mod_text[m.end():]
    else:
        # If your module uses a different pattern, we need a sample to tailor the regex.
        say("Warning: Could not find m_descriptors.insert({...}); descriptor not added.")
    

    module_path.write_text(mod_text, encoding="utf-8", newline="\n")
    say(f"Updated Module: {module_path}")

def _final_registration(self,
                        dest_root: Path,
                        component_name: str,
                        component_suffix: str,
                        component_type: str,
                        namespace: str,
                        target_meta: dict | None = None,
                        log=None) -> bool:
    """
    Integrate the new component into the selected target's files list, and into the Gem Module.
    - dest_root: gem root (the folder that contains Code/, Source/, CMakeLists.txt etc.)
    - component_name: e.g. "MyThing"
    - namespace: gem namespace (used to locate Module.cpp if possible)
    - target_meta: optional dict from _scan_cmake_targets; if None, uses current UI selection.
    """
    def say(msg): (log or print)(msg)

    try:
        composite_name = (component_name + component_suffix)
        
        _register_file_list(self, composite_name, target_meta, log=self.log_message)

        if not component_type == "Data Asset":
            _register_module_description(self, dest_root, composite_name, namespace, log=self.log_message)

        return True

    except Exception as e:
        say(f"Error during final registration: {e}")
        return False

# -----------------------------------
# 6) Comment scrubbing helper
# -----------------------------------
def _strip_c_like_comments(text: str, *, preserve_license: bool = True) -> str:
    """Preserve {BEGIN_LICENSE}...{END_LICENSE}, then strip /*...*/ and //... comments."""
    if not text:
        return text
    protected = []
    if preserve_license:
        def _protect(m):
            idx = len(protected)
            protected.append(m.group(0))
            return f"__CW_LIC_{idx}__"
        text = re.sub(r"\{BEGIN_LICENSE\}.*?\{END_LICENSE\}", _protect, text, flags=re.S)
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    text = re.sub(r"//.*", "", text)
    text = re.sub(r"[ \t]+\r?\n", "\n", text)
    if preserve_license:
        for i, block in enumerate(protected):
            text = text.replace(f"__CW_LIC_{i}__", block)
    return text
#endregion

#region --- CMake target scanning functionality
_CMAKE_FN_PATTERNS = (
    r'(?P<all>^\s*(?:o3de_add_target|ly_add_target)\s*\((?P<body>.*?)\)\s*)',  # O3DE macros
    r'(?P<all>^\s*add_library\s*\(\s*(?P<name>[A-Za-z0-9_.+\-]+)\b.*?\)\s*)',   # add_library(<name> ...)
    r'(?P<all>^\s*add_executable\s*\(\s*(?P<name>[A-Za-z0-9_.+\-]+)\b.*?\)\s*)' # add_executable(<name> ...)
)

def _resolve_target_name(raw: str, gem_name: str) -> str:
    """Best-effort: resolve ${GemName}, ${gem_name}, ${Name} expansions to the gem name."""
    if not raw:
        return raw
    # strip surrounding quotes
    if (raw.startswith('"') and raw.endswith('"')) or (raw.startswith("'") and raw.endswith("'")):
        raw = raw[1:-1]
    # Basic variable substitution
    mapping = {
        '${GemName}': gem_name,
        '${gem_name}': gem_name,
        '${Name}': gem_name,
    }
    out = raw
    for k, v in mapping.items():
        out = out.replace(k, v)
    return out

def _scan_cmake_targets(gem_path: Path, gem_name: str):
    """
    Returns list of dicts:
      { name, raw_name, kind, file, files_cmake_list }
    - Scans gem_path/**/CMakeLists.txt (project or gem)
    - Understands o3de_add_target/ly_add_target, add_library, add_executable
    - For o3de/ly targets, extracts *per-target* FILES_CMAKE includes (all of them)
    """
    results = []
    code_dir = Path(gem_path)
    if not code_dir.is_dir():
        return results

    cmake_files = list(code_dir.rglob("CMakeLists.txt"))
    if not cmake_files:
        return results

    # Patterns
    ly_macro_pat = r'(?:o3de_add_target|ly_add_target)\s*\((?P<body>.*?)\)\s*'
    add_lib_pat  = r'add_library\s*\(\s*(?P<name>[^\s\)]+)'
    add_exe_pat  = r'add_executable\s*\(\s*(?P<name>[^\s\)]+)'

    def _resolve_target_name(raw_name: str, gem: str) -> str:
        raw = raw_name.strip().strip('"\'')
        if not gem:
            return raw
        # Best-effort variable substitution for display/matching only
        return (raw
                .replace("${GemName}", gem)
                .replace("${gem_name}", gem)
                .replace("${GEM_NAME}", gem))

    def _extract_files_cmake_list(body: str) -> list[str]:
        """
        Find all FILES_CMAKE blocks in this macro body and return token list
        (tokens can be quoted or unquoted; we keep raw strings like '${pal_dir}/foo.cmake')
        """
        files = []
        # Capture the span after 'FILES_CMAKE' up to the next ALLCAPS header or end
        for m in re.finditer(r'\bFILES_CMAKE\b(?P<section>.*?)(?=^\s*[A-Z_]+\b|\Z)', body, flags=re.S | re.M):
            section = m.group('section')
            # tokens can be on multiple lines
            for tok in re.findall(r'"([^"]+)"|([^\s"\)]+)', section):
                token = tok[0] or tok[1]
                if token and token not in files:
                    files.append(token)
        return files

    for cmake in cmake_files:
        try:
            text = cmake.read_text(encoding="utf-8")
        except Exception:
            continue

        # 1) ly/o3de macro targets (preferred in O3DE gems)
        for mm in re.finditer(ly_macro_pat, text, flags=re.S | re.M):
            body = mm.group("body") or ""
            nm = re.search(r'\bNAME\s+(".*?"|[^\s\)]+)', body)
            if not nm:
                continue
            raw_name = nm.group(1).strip('"\'')
            resolved = _resolve_target_name(raw_name, gem_name)
            files_cmake_list = _extract_files_cmake_list(body)  # <-- per-target list
            results.append({
                "name": resolved,
                "raw_name": raw_name,
                "kind": "o3de_add_target",
                "file": cmake,
                "files_cmake_list": files_cmake_list
            })

        # 2) Plain CMake targets as fallback
        for mm in re.finditer(add_lib_pat, text):
            raw = mm.group("name")
            resolved = _resolve_target_name(raw, gem_name)
            results.append({
                "name": resolved,
                "raw_name": raw,
                "kind": "add_library",
                "file": cmake,
                "files_cmake_list": []
            })
        for mm in re.finditer(add_exe_pat, text):
            raw = mm.group("name")
            resolved = _resolve_target_name(raw, gem_name)
            results.append({
                "name": resolved,
                "raw_name": raw,
                "kind": "add_executable",
                "file": cmake,
                "files_cmake_list": []
            })

    # de-dupe by (name, file)
    uniq = {}
    for r in results:
        key = (r["name"], str(r["file"]))
        uniq[key] = r
    results = list(uniq.values())
    results.sort(key=lambda r: (r["name"] or "").lower())
    return results
#endregion

#region --- Getting Gems Functionality
def get_enabled_gems(engine_path: Path, project_path: Path, include_dependencies: bool = True):
        """
        Returns list[(gem_name, gem_path: Path)] for the project.
        Filters out engine-internal gems by comparing paths to engine_path.
        """
        pkg_root = Path(engine_path) / "scripts" / "o3de"   # contains the 'o3de' package
        sys.path.insert(0, str(pkg_root))
        try:
            from o3de import manifest  # provided by <engine>/scripts/o3de/o3de/manifest.py
            mapping = manifest.get_project_enabled_gems(
                Path(project_path),
                include_dependencies=include_dependencies
            ) or {}
        finally:
            sys.path.pop(0)

        # Normalize & filter
        engine_root = Path(engine_path).resolve()
        out = []
        for namespec, p in mapping.items():
            gp = Path(p).resolve()
            # exclude gems that live inside the engine folder (engine-default gems)
            try:
                gp.relative_to(engine_root)
                continue
            except ValueError:
                pass
            # derive a display name (prefer gem.json -> gem_name/display_name)
            name = namespec
            gj = gp / "gem.json"
            if gj.is_file():
                try:
                    data = json.loads(gj.read_text(encoding="utf-8"))
                    name = data.get("gem_name") or data.get("display_name") or namespec
                except Exception:
                    pass
            out.append((name, gp))
        out.sort(key=lambda x: x[0].lower())
        return out
#endregion

LOCK_FILE = Path(__file__).parent / ".lock"

def check_instance() -> bool:
    """Check if another instance is running"""
    if LOCK_FILE.exists():
       print("Another instance may already be running.")
       return False
    LOCK_FILE.touch()
    return True

def remove_lock():
    """Remove the lock file on exit"""
    try:
        if LOCK_FILE.exists():
            LOCK_FILE.unlink()
    except:
        pass

def validate_path(path: str) -> Path:
    """Validates path"""
    try:
        _path = Path(os.path.expanduser(path)).resolve()
        if not _path.exists():
            raise argparse.ArgumentTypeError(f"Path does not exist: {_path.name}")
        if not _path.is_dir():
            raise argparse.ArgumentTypeError(f"Not a directory: {_path.name}")
        return _path
    except Exception as e:
        raise argparse.ArgumentTypeError(f"Invalid path: {str(e)}")

def validate_engine_path(path: str) -> Path:
    """Validates an O3DE engine path"""
    engine_path = validate_path(path)
    if not (engine_path / "engine.json").exists():
        raise argparse.ArgumentTypeError(
            f" Not a valid O3DE engine directory: {engine_path}\n"
            "  Hint: engine.json file not found.\n"
            "  Make sure you're pointing to the root of an O3DE engine directory.\n"
            "  Example: --engine-path C:\\o3de"
        )
    return engine_path

def validate_component_name(component_name, log=None) -> bool:
    """Validate the component name"""
    KEYWORDS = {
    'alignas',       'alignof',      'and',          'and_eq',       'asm',          'auto',
    'bitand',        'bitor',        'bool',         'break',        'case',         'catch',
    'char',          'char8_t',      'char16_t',     'char32_t',     'class',        'compl',
    'concept',       'const',        'consteval',    'constexpr',    'const_cast',   'continue',
    'co_await',      'co_return',    'co_yield',     'decltype',     'default',      'delete',
    'do',            'double',       'dynamic_cast', 'else',         'enum',         'explicit',
    'export',        'extern',       'false',        'float',        'for',          'friend',
    'goto',          'if',           'inline',       'int',          'long',         'mutable',
    'namespace',     'new',          'noexcept',     'not',          'not_eq',       'nullptr',
    'operator',      'or',           'or_eq',        'private',      'protected',    'public',
    'register',      'reinterpret_cast','requires',  'return',       'short',        'signed',
    'sizeof',        'static',       'static_assert','static_cast',  'struct',       'switch',
    'template',      'this',         'thread_local', 'throw',        'true',         'try',
    'typedef',       'typeid',       'typename',     'union',        'unsigned',     'using',
    'virtual',       'void',         'volatile',     'wchar_t',      'while',        'xor',
    'xor_eq'
    }

    if not component_name:
        if log:
            log("Error: The name cannot be empty.")
        return False
    if (invalid := next((c for c in '*?+-,;=&%$`"\'/\\[]{}~#|<>!^@()#: \t\n\r\f\v' if c in component_name), None)):
        log and log(f"The name contains invalid character: {invalid}");
        return False
    if (
        not (component_name[0].isalpha() or component_name[0] == '_')
        or component_name.startswith('__')
        or (component_name.startswith('_') and len(component_name) > 1 and component_name[1].isupper())
    ):
        if log:
            log("Error: The name must start with a letter or single underscore.")
        return False
    if component_name in KEYWORDS:
        if log:
            log(f"Error: '{component_name}' is a C++ keyword. Please choose a different name.")
        return False
    return True

class Tooltip:
    """Tooltip class for displaying text when hovering over a widget."""
    def __init__(self, widget, text, delay=515):
        self.widget = widget
        self.text = text
        self.tooltip = None
        self.delay = delay
        self.id = None
        self.widget.bind("<Enter>", self.start_timer)
        self.widget.bind("<Leave>", self.hide)

    def show(self, event=None):
        """Display the tooltip near the mouse pointer when hovering over the widget."""
        if self.tooltip:
            return
        x, y, _, _ = self.widget.bbox("insert")
        x += self.widget.winfo_rootx() + 25
        y += self.widget.winfo_rooty() + 25

        self.tooltip = tk.Toplevel(self.widget)
        self.tooltip.wm_overrideredirect(True)
        self.tooltip.wm_geometry(f"+{x}+{y}")

        label = ttk.Label(
            self.tooltip,
            text=self.text,
            background="#ffffe0",
            foreground="black",
            relief="solid",
            borderwidth=0,
            padding=5,
            wraplength=380)
        label.pack()

    def start_timer(self, event=None):
        self.id = self.widget.after(self.delay, self.show)

    def hide(self, event=None):
        """Hide the tooltip when the mouse leaves the widget."""
        if self.id:
            self.widget.after_cancel(self.id)
            self.id = None
        if self.tooltip:
            self.tooltip.destroy()
            self.tooltip = None

class NewComponentWindow:
    """GUI window for creating a new component in an O3DE project.

    This class allows users to define settings such as the component name, type,
    namespace, and project location. It supports automatic integration with a project's Gem folder.
    """
    def __init__(self, root, engine_path, project_path):
        self.root = root
        self.engine_path = engine_path
        self.project_path = project_path
        self.namespace = tk.StringVar(value=project_path.stem if project_path else "")

        self.root.title("Add C++ Component")
        self.root.minsize(300, 480) if sys.platform == "win32" else self.root.minsize(300, 500)
        self.root.geometry("500x480") if sys.platform == "win32" else self.root.geometry("500x500")
        self.root.protocol("WM_DELETE_WINDOW", self.close_window)
        self.root.columnconfigure(1, weight=1)

        # default style to all ttk widgets
        style = ttk.Style()
        style.theme_use('clam')

        self.root.configure(bg="#444444")
        self.root.option_add("*TEntry.Font", ("TkDefaultFont", 10))
        self.root.option_add("*TCombobox.Font", ("TkDefaultFont", 10))
        default_font = tkFont.nametofont("TkDefaultFont")
        default_font.configure(family="Sans Serif", size=10)
        style.configure('.', background='#444444', foreground='#8C8C8C')
        style.configure("C.TLabelframe", background="#444444", bordercolor="#4E4E4E", borderwidth=1, relief="solid")
        style.configure("C.TButton", background="#444444", bordercolor="#4E4E4E", borderwidth=1, relief="solid")

        # Main container
        main_frame = ttk.Frame(root, padding="10")
        main_frame.pack(fill=tk.BOTH, expand=True)

#region MainWindow --- Destination Section (Targeting) ---
        target_frame = ttk.LabelFrame(main_frame, text=" Destination ", padding="10", style="C.TLabelframe")
        target_frame.pack(fill=tk.X, pady=5)

        choices = ["Project"]
        lookup  = {"Project": project_path}

        # 1) ALWAYS create the StringVar first, and attach it to the correct master
        self.target_choice = tk.StringVar(master=self.root, value="Project")

        # 2) Seed choices + lookup
        choices = ["Project"]
        self._target_lookup = {"Project": (project_path / "Gem")}

        # 4) Try to import and query the o3de manifest directly
        try:
            pkg_root = Path(engine_path) / "scripts" / "o3de"
            sys.path.insert(0, str(pkg_root))
            from o3de import manifest

            mapping = manifest.get_project_enabled_gems(Path(project_path), include_dependencies=True) or {}
        finally:
            if sys.path and sys.path[0] == str(pkg_root):
                sys.path.pop(0)

        # 5) Build choices from mapping (filter engine gems if you want)
        engine_root = Path(engine_path).resolve()
        for namespec, p in (mapping.items() if 'mapping' in locals() else []):
            gp = Path(p).resolve()
            try:
                gp.relative_to(engine_root)   # inside engine? skip
                continue
            except ValueError:
                pass

            name = namespec
            gj = gp / "gem.json"
            data = json.loads(gj.read_text(encoding="utf-8"))
            name = data.get("gem_name") or data.get("display_name") or name

            choices.append(name)
            self._target_lookup[name] = gp

        if len(choices) > 1:
            choices = ["Project"] + sorted(choices[1:], key=str.casefold)

        ttk.Label(target_frame, text="Target:").grid(row=0, column=0, sticky="e", padx=5, pady=5)

        # 6) Create the combobox using the StringVar you just made
        self.target_combo = ttk.Combobox(
            target_frame,
            values=choices,
            textvariable=self.target_choice,   # <-- THIS MUST BE self.target_choice
            state="readonly",
            width=28
        )
        self.target_combo.grid(row=0, column=1, sticky="ew", padx=5, pady=5)
        target_frame.columnconfigure(1, weight=1)

        # 7) Initialize selection explicitly
        self.target_combo.current(0)          # selects "Project"
        # OR: self.target_choice.set("Project")

        # Destination path display (Project -> project_root/Gem)
        ttk.Label(target_frame, text="Target Path:").grid(row=1, column=0, sticky="e", padx=5, pady=5)
        _initial_sel = self.target_combo.get()
        _base_path = self._target_lookup.get(_initial_sel, project_path)
        _dest_path = _base_path
        self.target_path_var = tk.StringVar(value=str(_dest_path))
        self.target_path_entry = ttk.Entry(target_frame, textvariable=self.target_path_var, state="normal")
        self.target_path_entry.grid(row=1, column=1, sticky="ew", padx=5, pady=5)

        # Browse Button
        self.browse_btn = ttk.Button(
            target_frame,
            text="...",
            width=3,
            command=self.browse_project_dir,
            style="C.TButton")
        self.browse_btn.grid(row=1, column=2, sticky="e", padx=5, pady=5)
        Tooltip(self.browse_btn, "Browse for a different project's Gem folder or destination directory.")

        def _on_target_changed(event=None):
            sel = self.target_choice.get()
            base_path = self._target_lookup.get(sel, project_path)                
            
            # Determine gem_path and gem_name
            if sel == "Project":
                self.target_path_var.set(str(base_path))
                # For "Project", we expect you're targeting <project>/Gem; read gem.json there
                gem_path = Path(project_path) / "Gem"
                cmake_path = gem_path
            else:
                self.target_path_var.set(self._target_lookup.get(sel, project_path) / "Code")
                gem_path = Path(self._target_lookup.get(sel, project_path))
                cmake_path = gem_path / "Code"

            gem_name = sel
            gj = gem_path / "gem.json"
            if gj.is_file():
                try:
                    import json
                    data = json.loads(gj.read_text(encoding="utf-8"))
                    gem_name = data.get("gem_name") or data.get("display_name") or sel
                except Exception:
                    pass

            # Scan targets
            targets = _scan_cmake_targets(cmake_path, gem_name)
            self._build_targets_meta = targets

            # Build a display list (just names), dedupe names for the combobox
            names = []
            seen = set()
            for t in targets:
                nm = t["name"] or t["raw_name"]
                if nm and nm not in seen:
                    seen.add(nm)
                    names.append(nm)

            # If nothing found, give a helpful placeholder
            if not names:
                names = ["<no CMake targets found>"]
                self.build_target_choice.set(names[0])
            else:
                # Choose a sensible default (prefer something with 'Static' or the plain gem name)
                preferred = None
                for cand in (f"{gem_name}.Private.Object", gem_name, "{gem_name}.API", f"{gem_name}.Editor", f"{gem_name}.Tools"):
                    if cand in names:
                        preferred = cand
                        break
                self.build_target_choice.set(preferred or names[0])

            self.build_target_combo["values"] = names

            # Set namespace
            self.namespace.set(str(gem_name))

            # Debug logging (optional)
            try:
                self.log_message(f"Found {len(targets)} targets under {gem_path}/Code")
                for t in targets:
                    self.log_message(f" - {t['name']}  [{t['kind']}]  ({t['file']})")
            except Exception:
                pass


        self.target_combo.bind("<<ComboboxSelected>>", _on_target_changed)

        # --- Build Target dropdown ---
        ttk.Label(target_frame, text="Package:").grid(row=2, column=0, sticky="e", padx=5, pady=5)
        self.build_target_choice = tk.StringVar(master=self.root, value="")
        self.build_target_combo = ttk.Combobox(
            target_frame,
            values=[],
            textvariable=self.build_target_choice,
            state="readonly",
            width=28
        )
        self.build_target_combo.grid(row=2, column=1, sticky="ew", padx=5, pady=5)
        target_frame.columnconfigure(1, weight=1)

        # Store meta for later (which CMakeLists this target came from)
        self._build_targets_meta = []   # list of dicts from _scan_cmake_targets
        
        # Row 2: Namespace
        ttk.Label(target_frame, text="Namespace:").grid(
            row=3, column=0, sticky="e", padx=5, pady=5)

        self.namespace_entry = ttk.Entry(
            target_frame,
            textvariable=self.namespace)
        self.namespace_entry.grid(row=3, column=1, sticky="ew", padx=5, pady=5)
        Tooltip(self.namespace_entry, "Enter the C++ namespace for your component.\nThis is usually your project name.")

#endregion

#region MainWindow --- Component Details Section ---
        details_frame = ttk.LabelFrame(main_frame, text=" Component Details ", padding="10", style="C.TLabelframe")
        details_frame.pack(fill=tk.X, pady=5)

        # Configure grid for alignment
        for i in range(3):
            details_frame.columnconfigure(i, weight=1 if i == 1 else 0)

        # Row 0: Component Name
        ttk.Label(details_frame, text="Component Name:").grid(
            row=0, column=0, sticky="e", padx=5, pady=5)

        self.component_name = ttk.Entry(details_frame)
        self.component_name.grid(row=0, column=1, columnspan=1, sticky="ew", padx=5, pady=5)
        Tooltip(self.component_name,  text="Enter the base name of your C++ component. \nThe template appends the word 'Component'.")

        # Row 1: Component Type
        ttk.Label(details_frame, text="Component Type:").grid(
            row=1, column=0, sticky="e", padx=5, pady=5)

        def on_component_select(event):
            """Component type selection"""
            ctype = self.component_type.get()

            # Rebuild the dynamic section
            self._render_dynamic_details(ctype)

        self.component_type = ttk.Combobox(
            details_frame,
            values=["Basic", "System", "Level", "LyShine UI", "Data Asset"],
            state="readonly",
            width=18)
        self.component_type.current(0)
        self.component_type.grid(row=1, column=1, sticky="ew", padx=5, pady=5)
        self.component_type.bind("<<ComboboxSelected>>", on_component_select)
        Tooltip(self.component_type, "Select component type: 'Basic' for runtime")
        self._component_type_lookup = {"Basic": "DefaultComponent", "System": "SystemComponent", "Level": "LevelComponent", "LyShine UI": "LyShineComponent", "Data Asset": "DataAsset"}

        # Dynamic sub-area that we will rebuild per component type
        self.details_dynamic_frame = ttk.Frame(details_frame)
        self.details_dynamic_frame.grid_columnconfigure(1, minsize=0, weight=1)  # allow entry to expand
        self.details_dynamic_frame.grid(row=2, column=0, columnspan=2, sticky="ew", padx=5, pady=5)

        # storage for dynamic tkinter variables & widgets
        self.dynamic_vars: dict[str, tk.Variable] = {}
        self.dynamic_widgets: dict[str, tk.Widget] = {}

        # Empty cell for alignment
        ttk.Frame(details_frame, width=10).grid(row=2, column=2)
#endregion
        

#region MainWindow --- Settings Section ---
        settings_frame = ttk.LabelFrame(main_frame, text=" Settings ", padding="10", style="C.TLabelframe")
        settings_frame.pack(fill=tk.X, pady=5)

        # Checkboxes
        self.automatic_register = tk.BooleanVar(value=True)
        cmake_cb = ttk.Checkbutton(
            settings_frame,
            text="Register Automatically",
            variable=self.automatic_register,
            onvalue=True,
            offvalue=False)
        cmake_cb.pack(anchor="w", pady=2)
        Tooltip(cmake_cb, "Automatically add this component to the Gem's private CMake source files.")
        
        self.remove_comments = tk.BooleanVar(value=True)
        comment_cb = ttk.Checkbutton(
            settings_frame,
            text="Remove Comments",
            variable=self.remove_comments,
            onvalue=True,
            offvalue=False)
        comment_cb.pack(anchor="w", pady=2)
        Tooltip(comment_cb, "Strip comments from generated files.")
        
        self.editor_adapter = tk.BooleanVar(value=False)
        editor_cb = ttk.Checkbutton(
            settings_frame,
            text="Add Editor Adapter",
            variable=self.editor_adapter,
            onvalue=True,
            offvalue=False)
        editor_cb.pack(anchor="w", pady=2)
        Tooltip(editor_cb, "Add Editor equivalent to your component!")

        self.default_license = tk.BooleanVar(value=False)
        license_cb = ttk.Checkbutton(
            settings_frame,
            text="Default License",
            variable=self.default_license,
            onvalue=True,
            offvalue=False)
        license_cb.pack(anchor="w", pady=2)
        Tooltip(license_cb, "Include the default license header in the source files.")
#endregion

#region MainWindow --- Log Section ---
        log_frame = ttk.LabelFrame(main_frame, text=" Log ", padding="10", style="C.TLabelframe")
        log_frame.pack(fill=tk.BOTH, expand=True, pady=5)

        self.log_text = tk.Text(log_frame, height=3, state="disabled", bg="#444444", fg="#c4c4c4",
                                relief="flat", bd=0, highlightthickness=0,
                                highlightbackground="#444444", highlightcolor="#444444")
        self.log_text.grid(row=0, column=0, sticky="nsew")

        # Make the text fill inside the label frame
        log_frame.grid_rowconfigure(0, weight=1)
        log_frame.grid_columnconfigure(0, weight=1)
#endregion

#region MainWindow --- Button Section ---
        button_frame = ttk.Frame(main_frame)
        button_frame.pack(fill=tk.X, pady=5)

        cancel_btn = ttk.Button(
            button_frame,
            text="Cancel",
            command=self.on_cancel,
            style="C.TButton")
        cancel_btn.pack(side="right")
        Tooltip(cancel_btn, "Close this window without creating a component.")

        ok_btn = ttk.Button(
            button_frame,
            text="Create",
            command=self.on_ok,
            style="C.TButton")
        ok_btn.pack(side="right", padx=5)
        Tooltip(ok_btn, "Create the component using the specified settings.")
#endregion

        # Call events once to set things at start
        # Target project change
        self.target_combo.event_generate("<<ComboboxSelected>>")

        # Component type change
        self.component_type.event_generate("<<ComboboxSelected>>")


        #self.root.after_idle(_autosize_and_center)
        
        self._resize_to_fit_content()
        self._center_main_window()
        # Window Init Complete

#region --- Custom Template Input Handling
    def _field_specs_for_type(self, comp_type: str) -> list[dict]:
        """
        Return a list of field specs to render for comp_type.
        Each spec: {"key","label","kind","default","values","tooltip"}
        kind: "entry" | "checkbox" | "combo"
        """
        base = []  # e.g. nothing for "Basic", "Level", "System", "LyShine"

        if comp_type == "Data Asset":
            return base + [
                {"key": "AssetExtension", "label": "File Extension", "kind": "entry",
                "default": "mydata", "tooltip": "Extension for assets (without dot)."},
                {"key": "AssetGroup", "label": "Group", "kind": "entry",
                "default": "DataAssets", "tooltip": "The group for the asset in the [New] menu."}
            ]

        # Default / Basic
        return base
    
    def _autosize_window(self):
        # Let Tk compute required size, then adopt it
        self.root.update_idletasks()
        req_w = self.root.winfo_reqwidth()
        req_h = self.root.winfo_reqheight()

        # Option A: fully adopt requested size
        self.root.geometry(f"{req_w}x{req_h}")
        self.root.minsize(req_w, req_h)  # avoid clipping after theme/scale changes

        # Option B: keep current width, only grow/shrink height:
        # cur_w = self.root.winfo_width()
        # self.root.geometry(f"{cur_w}x{req_h}")

    def _render_dynamic_details(self, comp_type: str):
        # 1) destroy previous widgets
        for child in self.details_dynamic_frame.winfo_children():
            child.destroy()
        self.dynamic_vars.clear()
        self.dynamic_widgets.clear()

        # 2) render new fields
        specs = self._field_specs_for_type(comp_type)

        if not specs:
            # hide the frame completely and let the parent reclaim space
            self.details_dynamic_frame.grid_remove()
            self._autosize_window()
            return

        # ensure the frame is visible (restores previous grid placement)
        self.details_dynamic_frame.grid()

        row = 0
        for spec in specs:
            key = spec["key"]; label = spec["label"]; kind = spec.get("kind", "entry")
            default = spec.get("default")
            tooltip = spec.get("tooltip", "")

            ttk.Label(self.details_dynamic_frame, text=label).grid(row=row, column=0, sticky="w", padx=5, pady=5)

            if kind == "entry":
                var = tk.StringVar(master=self.root, value=str(default) if default is not None else "")
                ent = ttk.Entry(self.details_dynamic_frame, textvariable=var)
                ent.grid(row=row, column=1, sticky="w", padx=5, pady=3)
                self.dynamic_vars[key] = var
                self.dynamic_widgets[key] = ent
                try: Tooltip(ent, tooltip)
                except Exception: pass

            elif kind == "checkbox":
                var = tk.BooleanVar(master=self.root, value=bool(default))
                cb = ttk.Checkbutton(self.details_dynamic_frame, text="", variable=var)
                # put checkbox on the right, label already on the left
                cb.grid(row=row, column=1, sticky="w", padx=5, pady=3)
                self.dynamic_vars[key] = var
                self.dynamic_widgets[key] = cb
                try: Tooltip(cb, tooltip)
                except Exception: pass

            elif kind == "combo":
                var = tk.StringVar(master=self.root, value=str(default) if default else "")
                values = spec.get("values", [])
                cmb = ttk.Combobox(self.details_dynamic_frame, state="readonly",
                                values=values, textvariable=var, width=28)
                cmb.grid(row=row, column=1, sticky="ew", padx=5, pady=3)
                self.dynamic_vars[key] = var
                self.dynamic_widgets[key] = cmb
                try: Tooltip(cmb, tooltip)
                except Exception: pass

            row += 1

        # 3) let the window resize to fit
        self._resize_to_fit_content()
        # also autosize the dynamic box.

    #endregion

    # Resize to fit content, but DO NOT move the window
    def _resize_to_fit_content(self, *, grow_only=False, set_minsize=True, pad=(0, 0)):
        """
        Resize the toplevel to its requested size (or only grow to it),
        without changing the current position.

        grow_only=True -> never shrink, only expand to fit new content.
        set_minsize=True -> lock current size as the minimum to avoid clipping.
        pad=(extra_w, extra_h) -> add a few pixels if you want breathing room.
        """
        self.root.update_idletasks()

        req_w = self.root.winfo_reqwidth()  + int(pad[0])
        req_h = self.root.winfo_reqheight() + int(pad[1])

        cur_w = self.root.winfo_width()
        cur_h = self.root.winfo_height()
        # If the window isn't mapped yet, width/height can be 1 - fall back to requested
        if cur_w <= 1 or cur_h <= 1:
            cur_w, cur_h = req_w, req_h

        new_w = max(cur_w, req_w) if grow_only else req_w
        new_h = max(cur_h, req_h) if grow_only else req_h

        if set_minsize:
            self.root.minsize(new_w, new_h)

        # IMPORTANT: no "+x+y" -> keeps current position
        self.root.geometry(f"{new_w}x{new_h}")

    # Center on screen, but DO NOT change the size
    def _center_main_window(self, *, clamp_to_screen=True):
        """
        Center the window using its current size.
        Does not resize - only changes position.
        """
        self.root.update_idletasks()

        w = self.root.winfo_width()
        h = self.root.winfo_height()
        if w <= 1 or h <= 1:
            w, h = self.root.winfo_reqwidth(), self.root.winfo_reqheight()

        sw = self.root.winfo_screenwidth()
        sh = self.root.winfo_screenheight()

        x = (sw - w) // 2
        y = (sh - h) // 2

        if clamp_to_screen:
            x = max(0, min(x, sw - w))
            y = max(0, min(y, sh - h))

        # IMPORTANT: "+x+y" with no "WxH" -> moves only
        self.root.geometry(f"+{x}+{y}")



#region Original Functionality Methods
    def close_window(self):
        """Centralized for all close operations"""
        remove_lock()
        self.root.destroy()

    def log_message(self, message):
        """ Append a message to the log frame"""
        self.log_text.config(state="normal")
        self.log_text.insert("end", message + "\n")
        self.log_text.see("end")
        self.log_text.config(state="disabled")

    def clear_log(self):
        """Clear all content from the log"""
        self.log_text.config(state="normal")
        self.log_text.delete("1.0", "end")
        self.log_text.config(state="disabled")

    def browse_project_dir(self):
        """Open directory dialog to select project path"""
        selected_path = filedialog.askdirectory(
            title="Select Project Directory",
            initialdir=self.target_path_var.get())
        if selected_path:
            self.target_path_var.set(selected_path)
            self.clear_log()
            self.log_message(f"Project directory: {selected_path}")

    def on_cancel(self):
        """Close the window"""
        self.close_window()

    def on_ok(self):
        """Create the component using the specified settings"""
        self.clear_log()
        _process_new_component(self)

#endregion

def main():
    """
    Supports both GUI and command-line modes.

    Parses command line arguments, validates paths, and initiates either:
        GUI mode:      Interactive Tkinter interface for creating components
        Non-GUI mode:  Automated component creation using command-line arguments
    """

    # Check if an instance of the application is already running
    if not check_instance():
        sys.exit(1)

    try:
        # Command line arguments for the script
        # GUI mode
        parser = argparse.ArgumentParser()
        parser.add_argument("--engine-path", required=True, type=validate_engine_path, help="Path to O3DE engine")
        parser.add_argument("--project-path", nargs='?', default=None, type=validate_path, help="Path to O3DE project")
        # Non-GUI mode
        parser.add_argument("--component-name", help="Component name")
        parser.add_argument("--component-type", choices=["Default", "Editor"], help="Default or Editor")
        parser.add_argument("--namespace", help="Namespace")
        parser.add_argument("--default-license", action="store_true", help="Include default license")
        parser.add_argument("--automatic-register", action="store_true", help="Add to project's Gem folder")

        args, unknown = parser.parse_known_args()
        if unknown:
            print(f"Please check your input for typos or unquoted special characters!")
            sys.exit(1)

        engine_path  = args.engine_path
        project_path = args.project_path

        if args.component_name:
            if not args.project_path:
                print("Error: --project-path is required in non-GUI mode.")
                sys.exit(1)
            if not args.component_type:
                print("Error: --component-type is required in non-GUI mode.")
                sys.exit(1)
            if not validate_component_name(args.component_name, log=print):
                print(f"Error: --component-name is required in non-GUI mode. Please provide valid --component-name argument.")
                sys.exit(1)
            if not validate_component_name(args.namespace, log=print):
                print("Error: --namespace is required in non-GUI mode. Please provide valid --namespace argument.")
                sys.exit(1)
            success = create_component(
                    engine_path=engine_path,
                    project_dir=project_path,
                    namespace=args.namespace,
                    component_name=args.component_name,
                    component_type=args.component_type,
                    automatic_register=args.automatic_register,
                    default_license=args.default_license,
                    log=print
                )
            sys.exit(0 if success else 1)
        else:
            # Initialize the main Tkinter window
            root = tk.Tk()

            # Window icon setup (PNG format)
            icon_path = Path(engine_path).joinpath("Assets", "Editor", "UI", "Icons", "Editor Settings Manager.png")
            if not icon_path.exists():
                 print(f"Icon not found at: {icon_path}")
            else:
                img = tk.PhotoImage(file=icon_path)
                root.iconphoto(True, img)

            # Create and run the main application window
            app = NewComponentWindow(root, engine_path, project_path)
            root.mainloop()
    except Exception:
        traceback.print_exc()
        sys.exit(1)
    finally:
        # Remove lock file before exiting
        remove_lock()

if __name__ == "__main__":
    main()
