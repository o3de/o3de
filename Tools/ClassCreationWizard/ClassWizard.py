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
import traceback
from pathlib import Path
import tempfile
import shutil
import fnmatch
import tkinter as tk
import tkinter.font as tkFont
from tkinter import filedialog, ttk

# --- ClassWizard staging helpers ---
def _import_engine_template(engine_path: Path):
    """Load O3DE's engine_template module from <engine>/scripts/o3de without spawning a process."""
    pkg_root = Path(engine_path) / "scripts" / "o3de"
    sys.path.insert(0, str(pkg_root))
    try:
        from o3de import engine_template  # type: ignore
        return engine_template
    finally:
        if sys.path and sys.path[0] == str(pkg_root):
            sys.path.pop(0)

def _create_to_stage(*, engine_path: Path, template_name: str, destination_name: str,
                     replacements: list[str] | None, keep_license_text: bool) -> Path:
    """
    Call O3DE's create_from_template into a temporary staging folder.
    Returns the staging folder path.
    """
    stage = Path(tempfile.mkdtemp(prefix="cw_stage_"))
    et = _import_engine_template(engine_path)
    rc = et.create_from_template(
        destination_path=stage,
        template_name=template_name,
        destination_name=destination_name,
        replace=replacements or [],
        keep_restricted_in_instance=True,
        keep_license_text=keep_license_text,
        no_register=True,
        force=True,
    )
    if rc != 0:
        shutil.rmtree(stage, ignore_errors=True)
        raise RuntimeError(f"create_from_template failed with rc={rc}")
    return stage

COMMENT_FILE_GLOBS = ("**/*.h", "**/*.hpp", "**/*.c", "**/*.cpp", "**/*.inl")

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

def _merge_stage_into_dest(stage: Path, dest: Path, *,
                           skip_existing: bool = True,
                           strip_comments: bool = True,
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
                data = _strip_c_like_comments(data, preserve_license=True)
                d.write_text(data, encoding="utf-8", newline="\\n")
        else:
            shutil.copy2(s, d)
        created += 1
        if log: log(f"wrote: {rel}")
    return created, skipped

def create_default_component_staged(*,
    engine_path: Path,
    dest_dir: Path,
    namespace: str,
    component_name: str,
    template_name: str = "DefaultComponent",
    strip_comments: bool = True,
    keep_license_text: bool = False,
    automatic_register: bool = False,
    log=None
) -> bool:
    """Stage -> merge pipeline for Default/Editor component templates."""
    def _log(msg): log(msg) if log else print(msg)
    replacements = ["${GemName}", namespace]
    stage = None
    try:
        _log(f"Staging template '{template_name}' for {component_name}…")
        stage = _create_to_stage(
            engine_path=Path(engine_path),
            template_name=template_name,
            destination_name=component_name,
            replacements=replacements,
            keep_license_text=keep_license_text,
        )
        _log("Merging into destination… (skipping existing files)")
        created, skipped = _merge_stage_into_dest(
            stage=stage, dest=Path(dest_dir),
            skip_existing=True, strip_comments=strip_comments, log=_log
        )
        _log(f"Created {created} file(s), skipped {skipped} existing file(s).")
        # Hook for future auto-registration if desired
        return True
    except Exception as e:
        _log(f"Error: {e}")
        return False
    finally:
        if stage:
            try: shutil.rmtree(stage, ignore_errors=True)
            except Exception: pass


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

def add_component_to_project(component_path: Path, component_name: str, namespace: str, log=None) -> bool:
    """Automatically integrates the component into the project's Gem folder."""
    def log_message(message):
        if log:
            log(message)
        else:
            print(message)

    try:
        log_message(f"Adding component to the project...")

        # Update {namespace}Module.cpp
        module_path = component_path / "Source" / f"{namespace}Module.cpp"
        if not module_path.exists():
            log_message(f"Error: Module file not found at {module_path}")
            log_message(" Hint: Make sure the Project Directory points to a valid Gem directory, not the root of a project. Usually it's where *_files.cmake resides.")
            return False

        with open(module_path, 'r', encoding='utf-8') as f:
            lines = f.read().splitlines()

        include_line = f'#include "{component_name}Component.h"'
        descriptor_line = f'{component_name}Component::CreateDescriptor()'

        # Insert include if not present
        if not any(include_line in line for line in lines):
            last_include_idx = max(i for i, line in enumerate(lines) if line.strip().startswith('#include'))
            lines.insert(last_include_idx + 1, include_line)

        # Insert descriptor if not present
        descriptor_inserted = any(descriptor_line in line for line in lines)
        if not descriptor_inserted:
            for i, line in enumerate(lines):
                if 'm_descriptors.insert' in line:
                    insert_start = i
                    break
            else:
                insert_start = -1
            if insert_start != -1:
                # Find the line with closing "});"
                for j in range(insert_start, len(lines)):
                    if '});' in lines[j]:
                        descriptor_end = j
                        break
                else:
                    descriptor_end = -1
                if descriptor_end != -1:
                    # Determine indentation
                    for k in range(insert_start, descriptor_end):
                        if 'CreateDescriptor()' in lines[k]:
                            indent = lines[k][:len(lines[k]) - len(lines[k].lstrip())]
                            break
                    else:
                        indent = ' ' * 16
                    # Insert new descriptor before closing
                    prev_line_idx = descriptor_end - 1
                    if not lines[prev_line_idx].strip().endswith(','):
                        lines[prev_line_idx] = lines[prev_line_idx].rstrip() + ','
                    # Insert new descriptor line
                    lines.insert(descriptor_end, f'{indent}{descriptor_line},')

        # Write the generated content to the module_path with UTF-8 encoding
        with open(module_path, 'w', encoding='utf-8', newline='\n') as f:
            f.write('\n'.join(lines) + '\n')

        # Update {namespace}_files.cmake
        project_files_path = component_path / f"{namespace.lower()}_files.cmake"
        if not project_files_path.exists():
            log_message(f"Error: Could not find {project_files_path}")
            return False

        with open(project_files_path, 'r', encoding='utf-8') as f:
            content = f.read()

        # Add .h/.cpp files if not present
        new_files = [
            f'    Source/{component_name}Component.cpp\n',
            f'    Source/{component_name}Component.h\n'
        ]
        files_section_start = content.find('set(FILES')
        if files_section_start != -1:
            files_section_end = content.find(')', files_section_start)
            if files_section_end != -1:
                for new_file in new_files:
                    if new_file not in content:
                        content = content[:files_section_end] + new_file + content[files_section_end:]

        # Write the generated content to the project_files_path with UTF-8 encoding
        with open(project_files_path, 'w', encoding='utf-8') as f:
            f.write(content)
        return True
    except Exception as e:
        log_message(f"Error adding component: {str(e)}")
        return False

def create_default_component(engine_path, project_dir, namespace, component_name,
                            automatic_register=False, default_license=False, log=None) -> bool:
    """Creates a new default component with the specified parameters."""
    def log_message(message):
        if log:
            log(message)
        else:
            print(message)

    try:
        script_name = "o3de.bat" if sys.platform == "win32" else "o3de.sh"
        o3de_script = Path(engine_path) / "scripts" / script_name
        cmd = [
            str(o3de_script),
            "create-from-template",
            "-dp", str(project_dir),
            "-dn", component_name,
            "-tn", "DefaultComponent",
            "-r", "${GemName}", namespace
        ]
        if default_license:
            cmd.append("--keep-license-text")
        cmd.append("--force")

        log(f"Creating component: {component_name}...")
        result = subprocess.run(
            cmd,
            cwd=engine_path,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            check=True
        )

        if result.stdout:
            for line in result.stderr.splitlines():
                if not line or line.replace('.', '').isdigit():
                    continue
                if '[INFO]' not in line and '[WARNING]' not in line:
                    log_message("" + line)
        if result.stderr:
            for line in result.stderr.splitlines():
                if '[INFO]' not in line and '[WARNING]' not in line:
                    log_message("" + line)

        log_message(f"Successfully created component: {component_name}")

        if automatic_register:
            success = add_component_to_project(Path(project_dir), component_name, namespace, log)
            if not success:
                log_message("Warning: Failed to automatically add the component to the project.")
            else:
                log_message("Successfully added component. The project may need to be rebuilt.")

        return True

    except subprocess.CalledProcessError as e:
        log_message(f"Failed to create component (exit code {e.returncode})")
        if e.stderr:
            log_message("" + e.stderr)
        return False
    except Exception as e:
        log_message(f"Error: {str(e)}")
        return False

def create_editor_component(engine_path, project_dir, namespace, component_name,
                            automatic_register=False, default_license=False, log=None) -> bool:
    """Creates an editor component with the specified parameters."""
    if log:
        log("Error: Editor component is not yet implemented. Please use 'Default' component type.")
    return False

def create_component(engine_path, project_dir, namespace, component_name,
                    component_type="Default", automatic_register=False, default_license=False, log=None)-> bool:
    """Creates a new O3DE component of the specified type."""
    if component_type == "Default":
        status = create_default_component(
                    engine_path=engine_path,
                    project_dir=project_dir,
                    namespace=namespace,
                    component_name=component_name,
                    automatic_register=automatic_register,
                    default_license=default_license,
                    log=print
                )
    elif component_type == "Editor":
        status = create_editor_component(
                    engine_path=engine_path,
                    project_dir=project_path,
                    namespace=namespace,
                    component_name=component_name,
                    automatic_register=automatic_register,
                    default_license=default_license,
                    log=print
                )
    return status

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

        # --- Targeting ---
        target_frame = ttk.LabelFrame(main_frame, text=" Destination ", padding="10", style="C.TLabelframe")
        target_frame.pack(fill=tk.X, pady=5)

        # small debug helper that prints and also uses the wizard's log area if available
        def _dbg(msg):
            try:
                self.log_message(str(msg))
            except Exception:
                pass
            print(f"[ClassWizard] {msg}")

        choices = ["Project"]
        lookup  = {"Project": project_path}

        _dbg(f"engine_path={engine_path}")
        _dbg(f"project_path={project_path}")
        _dbg(f"project.json exists? {(Path(project_path)/'project.json').exists()}")

        # 1) ALWAYS create the StringVar first, and attach it to the correct master
        self.target_choice = tk.StringVar(master=self.root, value="Project")

        # 2) Seed choices + lookup
        choices = ["Project"]
        self._target_lookup = {"Project": (project_path / "Gem")}

        # 3) (Optional) debug helper
        def _dbg(msg):
            try:
                self.log_message(str(msg))
            except Exception:
                pass
            print(f"[ClassWizard] {msg}")

        # 4) Try to import and query the o3de manifest directly
        try:
            pkg_root = Path(engine_path) / "scripts" / "o3de"
            sys.path.insert(0, str(pkg_root))
            from o3de import manifest
            _dbg(f"manifest from: {manifest.__file__}")

            mapping = manifest.get_project_enabled_gems(Path(project_path), include_dependencies=True) or {}
            _dbg(f"enabled gems returned: {len(mapping)}")
        finally:
            if sys.path and sys.path[0] == str(pkg_root):
                sys.path.pop(0)

        # 5) Build choices from mapping (filter engine gems if you want)
        engine_root = Path(engine_path).resolve()
        for namespec, p in (mapping.items() if 'mapping' in locals() else []):
            gp = Path(p).resolve()
            try:
                gp.relative_to(engine_root)   # inside engine? skip
                _dbg(f"filtered engine gem: {namespec}")
                continue
            except ValueError:
                pass

            name = namespec
            gj = gp / "gem.json"
            if gj.is_file():
                try:
                    data = json.loads(gj.read_text(encoding="utf-8"))
                    name = data.get("gem_name") or data.get("display_name") or name
                except Exception as ge:
                    _dbg(f"warn gem.json read: {ge!r}")

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


        def _on_target_changed(event=None):
            sel = self.target_choice.get()
            base_path = self._target_lookup.get(sel, project_path)
            self.target_path_var.set(str(base_path))
            # feed the destination to Project Directory field used by create-from-template
            try:
                self.project_dir_var.set(str(base_path))
            except Exception:
                pass

        self.target_combo.bind("<<ComboboxSelected>>", _on_target_changed)

        # Component Details
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
            if self.component_type.get() == "System":
                self.component_type.set("Simple")
                self.clear_log()
                self.log_message("Info: System Component needs a Template.")
            if self.component_type.get() == "Level":
                self.component_type.set("Simple")
                self.clear_log()
                self.log_message("Info: Level Component needs a Template.")
            if self.component_type.get() == "Lyshine UI":
                self.component_type.set("Simple")
                self.clear_log()
                self.log_message("Info: Ui component needs Template.")
            if self.component_type.get() == "Data Asset":
                self.component_type.set("Simple")
                self.clear_log()
                self.log_message("Info: Data Asset needs features and template.")

        self.component_type = ttk.Combobox(
            details_frame,
            values=["Simple", "System", "Level", "Lyshine UI", "Data Asset"],
            state="readonly",
            width=18)
        self.component_type.current(0)
        self.component_type.grid(row=1, column=1, sticky="ew", padx=5, pady=5)
        self.component_type.bind("<<ComboboxSelected>>", on_component_select)
        Tooltip(self.component_type, "Select component type: 'Simple' for runtime")

        # Row 2: Namespace
        ttk.Label(details_frame, text="Namespace:").grid(
            row=2, column=0, sticky="e", padx=5, pady=5)

        self.namespace_entry = ttk.Entry(
            details_frame,
            textvariable=self.namespace)
        self.namespace_entry.grid(row=2, column=1, sticky="ew", padx=5, pady=5)
        Tooltip(self.namespace_entry, "Enter the C++ namespace for your component.\nThis is usually your project name.")

        # Empty cell for alignment
        ttk.Frame(details_frame, width=10).grid(row=2, column=2)

        # Row 3: Project Directory
        ttk.Label(details_frame, text="Project Directory:").grid(
            row=3, column=0, sticky="e", padx=5, pady=5)

        self.project_dir_var = tk.StringVar(value=str(project_path))
        self.project_dir_entry = ttk.Entry(
            details_frame,
            textvariable=self.project_dir_var)
        self.project_dir_entry.grid(row=3, column=1, sticky="ew", padx=5, pady=5)
        Tooltip(
            self.project_dir_entry,
            "Specifies the destination directory where the component will be created. "
            "To automatically add the component to the project, this must point to a valid Gem folder within the project. "
            "In that case, 'Add to project' must be checked.")

        # Browse Button
        self.browse_btn = ttk.Button(
            details_frame,
            text="...",
            width=3,
            command=self.browse_project_dir,
            style="C.TButton")
        self.browse_btn.grid(row=3, column=2, sticky="e", padx=5, pady=5)
        Tooltip(self.browse_btn, "Browse for a different project's Gem folder or destination directory.")

        # Settings Section
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

        # Log Section
        log_frame = ttk.LabelFrame(main_frame, text=" Log ", padding="10", style="C.TLabelframe")
        log_frame.pack(fill=tk.BOTH, expand=True, pady=5)

        self.log_text = tk.Text(log_frame, height=3, state="disabled", bg="#444444", fg="#c4c4c4", relief="flat", bd=0,
            highlightthickness=0, highlightbackground="#444444", highlightcolor="#444444")
        self.log_text.pack(fill=tk.BOTH, expand=True)

        # Button Frame
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

        def _autosize_and_center():
            self.root.update_idletasks()
            w, h = self.root.winfo_reqwidth(), self.root.winfo_reqheight()
            x = (self.root.winfo_screenwidth()  - w) // 2
            y = (self.root.winfo_screenheight() - h) // 2
            self.root.minsize(w, h)
            self.root.geometry(f"{w}x{h}+{x}+{y}")

        self.root.after_idle(_autosize_and_center)

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
            initialdir=self.project_dir_var.get())
        if selected_path:
            self.project_dir_var.set(selected_path)
            self.clear_log()
            self.log_message(f"Project directory: {selected_path}")

    def on_cancel(self):
        """Close the window"""
        self.close_window()

    def on_ok(self):
        """Create the component using the specified settings"""
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
        component_type = self.component_type.get()

        # Destination path: prefer Target Path if available, else fallback
        try:
            dest_dir = self.target_path_var.get().strip()
        except Exception:
            dest_dir = ""

        if not dest_dir:
            try:
                dest_dir = self.project_dir_var.get().strip()
            except Exception:
                dest_dir = ""

        if not dest_dir or not os.path.isdir(dest_dir):
            self.log_message(f"Error: Target path {dest_dir or '<empty>'} does not exist.")
            return

        automatic_register = self.automatic_register.get() if hasattr(self, 'automatic_register') else False
        strip_comments = True
        keep_license = self.default_license.get() if hasattr(self, 'default_license') else False

        self.log_message("Please wait...")
        self.root.update_idletasks()

        # Map component type to template name
        template_map = {
            "Default": "DefaultComponent",
            "Editor": "DefaultEditorComponent"
        }
        template_name = template_map.get(component_type, "DefaultComponent")

        ok = create_default_component_staged(
            engine_path=self.engine_path,
            dest_dir=Path(dest_dir),
            namespace=namespace,
            component_name=component_name,
            template_name=template_name,
            strip_comments=strip_comments,
            keep_license_text=keep_license,
            automatic_register=automatic_register,
            log=self.log_message
        )
        if ok:
            self.log_message("Done.")
        else:
            self.log_message("Failed.")

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
