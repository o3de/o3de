#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

import argparse
import contextlib
import io
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import traceback
from pathlib import Path
from typing import Optional, List, Dict, Tuple, Any, Callable
from dataclasses import dataclass, field

# Command plugin infrastructure base classes, registry, and plugin loader
from command_plugin import (
    CommandContext, WizardCommand, CommandRegistry, CommandPluginLoader,
    CMakeTarget, CMakeAnalyzer
)
from commands.exclude_conditional_files import ConditionalFileExcluder


# ============================================================================
# PySide6 Bootstrap
# ============================================================================
# O3DE ships PySide6 as a C++ runtime dependency of the QtForPython gem: the
# native libraries are copied next to Editor.exe, but PySide6 is never installed
# into the o3de Python venv (python/requirements.txt does not list it). The
# Editor's embedded interpreter can therefore "import PySide6" -- it is
# bootstrapped by Gems/QtForPython/Editor/Scripts/bootstrap.py -- but a
# standalone tool launched through python/python.cmd cannot.
#
# To run standalone we locate the exact Qt-matched pyside6 and qt 3rdParty
# packages the engine downloaded and place the PySide6 Python package plus the
# native DLL folders on the path BEFORE importing PySide6. This reuses the
# shipped, ABI-correct build: no PyPI wheel, no venv mutation, no dependence on
# the host machine's Python. The whole step is skipped when PySide6 already
# imports (e.g. a developer running under a system Python that provides it).


def _pyside6_engine_root() -> Optional[Path]:
    """Engine tree to resolve 3rdParty packages from: an explicit --engine-path
    if one was passed, otherwise the engine this script lives in."""
    argv = sys.argv
    for i, arg in enumerate(argv):
        value = None
        if arg == "--engine-path" and i + 1 < len(argv):
            value = argv[i + 1]
        elif arg.startswith("--engine-path="):
            value = arg.split("=", 1)[1]
        if value and (Path(value) / "engine.json").exists():
            return Path(value).resolve()
    # <engine>/Tools/ClassCreationWizard/ClassWizard.py -> <engine>
    fallback = Path(__file__).resolve().parents[2]
    return fallback if (fallback / "engine.json").exists() else None


def _pyside6_packages_folder(engine_root: Path) -> Path:
    """The o3de 3rdParty 'packages' folder (shared across engines). Resolved via
    the o3de manifest API when available, else the default ~/.o3de location."""
    scripts = engine_root / "scripts" / "o3de"
    sys.path.insert(0, str(scripts))
    try:
        from o3de import manifest
        return Path(manifest.get_o3de_third_party_folder()) / "packages"
    except Exception:
        return Path.home() / ".o3de" / "3rdParty" / "packages"
    finally:
        if sys.path and sys.path[0] == str(scripts):
            sys.path.pop(0)


def _pyside6_newest_package(packages: Path, prefix: str, subpath: str) -> Optional[Path]:
    """Newest downloaded package matching '<prefix>*' whose <subpath> exists.
    'Newest' == highest folder name, which orders revisions correctly
    (e.g. pyside6-6.10.2-py3.10-rev4 sorts above ...-rev1)."""
    matches = sorted((p for p in packages.glob(prefix + "*") if p.is_dir()), reverse=True)
    for pkg in matches:
        target = pkg / subpath
        if target.exists():
            return target
    return None


def _pyside6_add_dll_dir(path: Path) -> None:
    """Make a native DLL folder discoverable to the dynamic loader."""
    if not path.is_dir():
        return
    if hasattr(os, "add_dll_directory"):        # Windows, Python 3.8+
        try:
            os.add_dll_directory(str(path))
        except OSError:
            pass
    os.environ["PATH"] = str(path) + os.pathsep + os.environ.get("PATH", "")


def _bootstrap_pyside6() -> None:
    """Put O3DE's shipped PySide6 on the path so a standalone launch can import
    it. Safe to call when PySide6 is partially available."""
    engine_root = _pyside6_engine_root()
    if engine_root is None:
        return
    packages = _pyside6_packages_folder(engine_root)

    # 1. Put the engine's own complete PySide6 package first on sys.path. We match
    #    the package to the RUNNING interpreter's Python tag (e.g. "py3.10") so a
    #    non-matching interpreter's own PySide6 (e.g. a system wheel under 3.13) is
    #    never shadowed by an ABI-incompatible build. Prepending unconditionally --
    #    rather than only when PySide6 seems missing -- is deliberate: an engine
    #    venv can carry a BROKEN/partial PySide6 + shiboken6 (e.g. the native
    #    shiboken6.Shiboken .pyd absent -> "No module named 'shiboken6.Shiboken'"),
    #    and the engine's complete 3rdParty copy must win over that.
    py_tag = f"py{sys.version_info.major}.{sys.version_info.minor}"
    site_packages = _pyside6_newest_package(packages, f"pyside6-*-{py_tag}-", "pyside6/lib/site-packages")
    if site_packages is None or not (site_packages / "PySide6").is_dir():
        return  # No interpreter-matched engine package; leave any existing PySide6 alone.
    sys.path.insert(0, str(site_packages))

    # 2. Make PySide6's native Qt6 dependencies discoverable. This is required
    #    even when the PySide6 package itself is already importable: the Qt6 DLLs
    #    it links against are not on a standalone interpreter's DLL search path,
    #    so 'import PySide6.QtCore' fails with "DLL load failed" without it. The
    #    pyside6 and qt 3rdParty package bin folders are build-independent; the
    #    engine's built bin/<config> is added too when present as an exact match.
    _pyside6_add_dll_dir(site_packages.parent.parent / "bin")           # <pyside6-pkg>/pyside6/bin
    qt_bin = _pyside6_newest_package(packages, "qt-", "qt/bin")
    if qt_bin is not None:
        _pyside6_add_dll_dir(qt_bin)
    for built_bin in engine_root.glob("build/*/bin/*"):
        if (built_bin / "Qt6Core.dll").exists() or (built_bin / "libQt6Core.so.6").exists():
            _pyside6_add_dll_dir(built_bin)

    # 3. Point Qt at its platform plugins. The o3de pyside6 package does not
    #    bundle a 'platforms/' folder -- it links against the engine's Qt, whose
    #    plugins live in the qt 3rdParty package. Without QT_PLUGIN_PATH, Qt fails
    #    to load the platform plugin (qwindows / qoffscreen) and the GUI aborts on
    #    startup ("could not find or load the Qt platform plugin"). Mirrors the
    #    editor's Gems/QtForPython/Editor/Scripts/bootstrap.py. Do not clobber a
    #    value the caller already set (e.g. when embedded in the Editor).
    if not os.environ.get("QT_PLUGIN_PATH"):
        qt_plugins = _pyside6_newest_package(packages, "qt-", "qt/plugins")
        if qt_plugins is not None:
            os.environ["QT_PLUGIN_PATH"] = str(qt_plugins)


# Try the NATURAL import first -- exactly as any O3DE-tied Python app would, using
# only whatever the engine's Python environment provides. If that works, we touch
# nothing. If it fails, engage the wizard's own PySide6 bootstrap so the tool runs
# anywhere (terminal, VS, Rider, an extension, ...).
#
# The bootstrap is EXPECTED for a standalone launcher: a .pyd's dependent DLLs are
# not resolved from PATH since Python 3.8, so a bare interpreter genuinely cannot
# load Qt without an in-process os.add_dll_directory -- which only code inside the
# process (this bootstrap) can do. So that case stays silent. Only the O3DE editor's
# EMBEDDED interpreter is expected to hand us a fully working PySide6; a failure
# there is unexpected and worth flagging, since it points at a broken engine Python.
# The probe is expected to fail (and print its own diagnostic) on a bare launcher,
# so capture its stderr; we only re-surface it in the unexpected (editor) case.
_pyside_probe_stderr = io.StringIO()
try:
    with contextlib.redirect_stderr(_pyside_probe_stderr):
        import PySide6.QtCore  # noqa: F401  -- natural probe; reused by the imports below
except BaseException as _natural_pyside_err:
    try:
        import azlmbr  # noqa: F401  -- importable only in the editor's embedded interpreter
        _in_embedded_editor = True
    except Exception:
        _in_embedded_editor = False

    if isinstance(_natural_pyside_err, ModuleNotFoundError):
        _pyside_reason = "PySide6/shiboken6 native module missing from the venv"
    elif isinstance(_natural_pyside_err, ImportError):
        _pyside_reason = "Qt6 DLLs not on the interpreter's search path"
    else:
        _pyside_reason = type(_natural_pyside_err).__name__

    # Drop any partially-initialized modules so the retry resolves from our path.
    for _m in [m for m in list(sys.modules)
               if m in ("PySide6", "shiboken6") or m.startswith(("PySide6.", "shiboken6."))]:
        del sys.modules[_m]

    try:
        _bootstrap_pyside6()
    except Exception:
        pass  # Fall through: the import below raises a clear, actionable error.

    if _in_embedded_editor:
        # Unexpected: the editor's embedded interpreter should already provide
        # PySide6. Falling back here suggests a broken engine Python environment.
        print(
            f"[ClassWizard] Unexpected: PySide6 was not importable from the O3DE "
            f"editor's Python environment ({_pyside_reason}); fell back to the "
            f"wizard's own bootstrap. This usually means the engine's Python setup "
            f"is broken.",
            file=sys.stderr,
        )
        _probe_diag = _pyside_probe_stderr.getvalue().strip()
        if _probe_diag:
            print(_probe_diag, file=sys.stderr)

try:
    from PySide6.QtCore import Qt, Signal, QTimer, QSettings
    from PySide6.QtWidgets import (
        QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
        QLabel, QLineEdit, QPushButton, QComboBox, QCheckBox,
        QSpinBox, QDoubleSpinBox,
        QTextEdit, QGroupBox, QFileDialog, QMessageBox, QFormLayout
    )
    from PySide6.QtGui import QIcon
except ImportError as pyside_import_error:
    raise ImportError(
        "PySide6 could not be imported. O3DE ships PySide6 as a runtime "
        "dependency of the QtForPython gem (copied next to Editor.exe), not in "
        "the Python venv, so the standalone ClassWizard bootstraps it from the "
        "engine's 3rdParty packages. Ensure --engine-path points at an engine "
        "whose 'pyside6' and 'qt' 3rdParty packages have been downloaded (they "
        "are fetched during the engine's CMake configure)."
    ) from pyside_import_error


# ============================================================================
# Utilities
# ============================================================================

@contextlib.contextmanager
def o3de_manifest_api(engine_path: Path):
    """Context manager that temporarily adds the o3de scripts to sys.path
    and yields the o3de.manifest module.

    Usage:
        with o3de_manifest_api(engine_path) as manifest:
            gems = manifest.get_project_enabled_gems(project_path)
    """
    pkg_root = engine_path / "scripts" / "o3de"
    sys.path.insert(0, str(pkg_root))
    try:
        from o3de import manifest
        yield manifest
    finally:
        if sys.path and sys.path[0] == str(pkg_root):
            sys.path.pop(0)


# ============================================================================
# Widgets
# ============================================================================

class BoundedComboBox(QComboBox):
    """QComboBox whose dropdown popup is bounded to a maximum pixel height.

    Fusion style calculates QComboBoxPrivateContainer size independently of the
    view's maximumHeight, so two steps are required:
      1. Pre-constrain the view -- Qt then positions the popup at the correct
         screen location (no stale-coordinate problem).
      2. Post-resize the container with resize() only (no setMaximumHeight, which
         would persist and confuse future show calls) to clamp the scrollbar and
         background. Re-anchor with move() only for the above-placement case.
    """
    MAX_POPUP_HEIGHT = 400

    def showPopup(self):
        # Step 1: pre-constrain the view so Qt positions the popup at the correct
        # screen location. Qt respects this when choosing popup geometry.
        self.view().setMaximumHeight(self.MAX_POPUP_HEIGHT)
        super().showPopup()

        # Step 2: clamp the container that Fusion sizes independently of the view,
        # and force the popup to always open downward below the combo.
        popup = self.view().window()
        if popup is self:
            return

        combo_bottom = self.mapToGlobal(self.rect().bottomLeft())
        clamped_height = min(popup.height(), self.MAX_POPUP_HEIGHT)
        popup.resize(popup.width(), clamped_height)
        popup.move(combo_bottom)


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

# Color/radius literals are $ClassWizard* theme tokens, substituted at apply time by
# apply_theme_tokens() from Themes/ClassWizard/themeProperties.json (see THEMING section).
# Every token referenced here MUST exist in that file (and in _BUILTIN_THEME_TOKENS) so a
# missing/sparse theme can only change values, never leave a literal "$..." in the QSS.
WINDOW_STYLESHEET = """
    QMainWindow {
        background-color: $ClassWizardWindowBackgroundColor;
    }
    QGroupBox {
        color: $ClassWizardTextColor;
        border: 1px solid $ClassWizardBorderColor;
        border-radius: $ClassWizardGroupBoxRadius;
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
        color: $ClassWizardTextColor;
    }
    QLabel[formLabel="true"] {
        font-size: 9.5pt;
        min-width: 110px;
        max-width: 110px;
    }
    QLineEdit, QComboBox, QTextEdit {
        background-color: $ClassWizardInputBackgroundColor;
        color: $ClassWizardTextColor;
        border: 1px solid $ClassWizardBorderColor;
        border-radius: $ClassWizardControlRadius;
        padding: 5px;
        font-size: 9.5pt;
    }
    QLineEdit:focus, QComboBox:focus {
        border: 1px solid $ClassWizardAccentColor;
    }
    QComboBox::drop-down {
        border: none;
        width: 20px;
    }
    QComboBox::down-arrow {
        image: url(COMBO_ARROW_URL);
        width: 10px;
        height: 10px;
    }
    QComboBox::down-arrow:on {
        image: url(COMBO_ARROW_UP_URL);
        width: 10px;
        height: 10px;
    }
    QPushButton {
        background-color: $ClassWizardAccentColor;
        color: $ClassWizardOnAccentTextColor;
        border: none;
        border-radius: $ClassWizardControlRadius;
        padding: 8px 20px;
        font-weight: bold;
    }
    QPushButton:hover {
        background-color: $ClassWizardAccentHoverColor;
    }
    QPushButton:pressed {
        background-color: $ClassWizardAccentPressedColor;
    }
    QPushButton:disabled {
        background-color: $ClassWizardDisabledBackgroundColor;
        color: $ClassWizardDisabledTextColor;
    }
    QCheckBox {
        color: $ClassWizardTextColor;
        spacing: 8px;
        font-size: 9.5pt;
    }
    QCheckBox::indicator {
        width: 18px;
        height: 18px;
        border: 1px solid $ClassWizardBorderColor;
        border-radius: $ClassWizardControlRadius;
        background-color: $ClassWizardInputBackgroundColor;
    }
    QCheckBox::indicator:checked {
        background-color: $ClassWizardAccentColor;
        border-color: $ClassWizardAccentColor;
    }
    QTextEdit {
        font-family: 'Courier New', monospace;
        font-size: 9pt;
    }
"""

# Status label inline styles (font/padding shared; color varies by state)
_STATUS_STYLE_BASE = "QLabel {{ font-size: 12pt; font-weight: bold; padding: 3px; color: {color}; }}"
_STATUS_COLOR_PENDING  = "#808080"   # grey - placeholder / not yet run
_STATUS_COLOR_ACTIVE   = "palette(text)"  # theme text - filled summary


# ============================================================================
# Theming
# ----------------------------------------------------------------------------
# The wizard's look is driven by named $ClassWizard* tokens substituted into
# WINDOW_STYLESHEET. Token values come from a theme file in the same format the
# O3DE editor theming system uses (AzQtComponents/Themes/<Name>/themeProperties
# .json), so the editor StyleManager / Theme Editor can read and author this
# file. Resolution order, lowest to highest precedence:
#   1. _BUILTIN_THEME_TOKENS   -- in-code mirror; last-resort safety net so the
#                                 tool never renders a literal "$..." even if the
#                                 JSON is missing (raw-deployment guarantee).
#   2. DEFAULT_THEME_FILE      -- the shipped, editable default (canonical).
#   3. selected engine theme   -- the ClassWizard group of the theme the editor
#                                 has selected (read from disk via --engine-path).
#   4. editor working overrides-- the editor's unsaved-but-Applied edits, persisted
#                                 in shared QSettings. Highest precedence so that
#                                 "Apply in editor -> relaunch wizard" reflects the
#                                 live edits. Cleared by the editor on theme switch.
# Layers 3-4 are no-ops when the wizard runs standalone, so it stays native.
# ============================================================================

THEME_TOKEN_PREFIX = "$"
DEFAULT_THEME_FILE  = Path(__file__).resolve().parent / "Themes" / "ClassWizard" / "themeProperties.json"

# Shared QSettings the O3DE editor writes its active theme into. PySide6's QSettings reads the
# same native store the C++ editor uses (keyed by org/app), so this is the cross-process,
# cross-language handshake. Matches AtomToolsApplication.cpp (Material Editor etc. follow the
# editor theme the same way). Keep these three constants identical to the editor's.
EDITOR_QSETTINGS_ORG   = "O3DE"
EDITOR_QSETTINGS_APP   = "O3DE Editor"
EDITOR_THEME_KEY       = "Settings/EditorTheme"   # value is a theme folder name, e.g. "O3DE_Dark"

# Working overrides: the editor's Applied-but-unsaved theme edits, persisted by the Theme Editor.
# Keys mirror the editor's ThemeWorkingOverrides.h contract. The base key records which theme the
# overrides sit on, so the wizard ignores them if a different theme is now selected.
EDITOR_WORKING_OVERRIDES_GROUP    = "ThemeWorkingOverrides"
EDITOR_WORKING_OVERRIDES_BASE_KEY = "__baseTheme"

# Engine-relative location of the theme folders the editor theming system ships.
ENGINE_THEMES_RELPATH  = Path("Code") / "Framework" / "AzQtComponents" / "AzQtComponents" / "Themes"
# Subtree of an engine theme that themes THIS tool. Engine themes opt in by adding this group.
WIZARD_THEME_GROUP     = "ClassWizard"

# Mirror of Themes/ClassWizard/themeProperties.json -> theme_properties.ClassWizard.
# KEEP IN SYNC with that file. Used only when the JSON cannot be read.
_BUILTIN_THEME_TOKENS = {
    "ClassWizardWindowBackgroundColor": "#393a3c",
    "ClassWizardInputBackgroundColor":  "#303030",
    "ClassWizardBorderColor":           "#4E4E4E",
    "ClassWizardTextColor":             "#cccccc",
    "ClassWizardDisabledTextColor":     "#999999",
    "ClassWizardOnAccentTextColor":     "#ffffff",
    "ClassWizardAccentColor":           "#D9822E",
    "ClassWizardAccentHoverColor":      "#e09854",
    "ClassWizardAccentPressedColor":    "#b26b26",
    "ClassWizardDisabledBackgroundColor": "#555555",
    "ClassWizardControlRadius":         "3px",
    "ClassWizardGroupBoxRadius":        "4px",
}


def _flatten_theme_properties(props, prefix=""):
    """Flatten a nested theme_properties tree into {TokenName: value} by key
    concatenation, matching the engine StylesheetPreprocessor rule
    ({"Radius": {"Input": "4px"}} -> "RadiusInput")."""
    flat = {}
    for key, value in props.items():
        name = prefix + key
        if isinstance(value, dict):
            flat.update(_flatten_theme_properties(value, name))
        else:
            flat[name] = str(value)
    return flat


def _load_theme_file(path):
    """Return flattened tokens from a themeProperties.json, or {} on any failure."""
    try:
        with open(path, "r", encoding="utf-8") as f:
            data = json.load(f)
        return _flatten_theme_properties(data.get("theme_properties", {}))
    except Exception:
        return {}


def _read_editor_selected_theme():
    """Return the theme folder name the editor has selected (e.g. "O3DE_Dark"),
    or "" if no editor preference is stored. Reads the same QSettings store the
    C++ editor writes -- this is how the wizard 'follows' the editor's choice."""
    try:
        settings = QSettings(EDITOR_QSETTINGS_ORG, EDITOR_QSETTINGS_APP)
        return str(settings.value(EDITOR_THEME_KEY, "") or "")
    except Exception:
        return ""


def _load_engine_theme_overlay(engine_path):
    """Resolve the editor's active theme and return its flattened ClassWizard.*
    tokens to overlay the shipped defaults. Returns {} (a no-op) whenever the
    wizard runs standalone, the engine path is unknown, the theme isn't on disk,
    or the selected theme defines no ClassWizard group -- so a missing or sparse
    theme can only change values, never break the look."""
    if not engine_path:
        return {}
    # Fall back to the editor's default theme when no selection is stored yet, so launching with
    # --engine-path alone still themes the wizard (the editor writes the key once settings are saved).
    theme_name = _read_editor_selected_theme() or "O3DE_Original"
    theme_file = Path(engine_path) / ENGINE_THEMES_RELPATH / theme_name / "themeProperties.json"
    try:
        with open(theme_file, "r", encoding="utf-8") as f:
            data = json.load(f)
    except Exception:
        return {}
    props = data.get("theme_properties", {})
    tokens = {}
    # Hand-authored themes nest the tokens under a "ClassWizard" object; the Theme Editor's
    # "Save As" writes them flat ("ClassWizardAccentColor": ...). Accept both forms.
    nested = props.get(WIZARD_THEME_GROUP, {})
    if isinstance(nested, dict):
        tokens.update(_flatten_theme_properties(nested, WIZARD_THEME_GROUP))
    for key, value in props.items():
        if key.startswith(WIZARD_THEME_GROUP) and not isinstance(value, dict):
            tokens[key] = str(value)
    return tokens


def _load_editor_working_overrides():
    """Return the editor's Applied-but-unsaved ClassWizard.* edits from shared
    QSettings, or {} if none / standalone. Only honored when the overrides' base
    theme matches the editor's currently selected theme (the editor clears them on
    a theme switch, but this guards against any stale cross-theme bleed)."""
    try:
        settings = QSettings(EDITOR_QSETTINGS_ORG, EDITOR_QSETTINGS_APP)
        selected = str(settings.value(EDITOR_THEME_KEY, "") or "")
        settings.beginGroup(EDITOR_WORKING_OVERRIDES_GROUP)
        try:
            base = str(settings.value(EDITOR_WORKING_OVERRIDES_BASE_KEY, "") or "")
            if not base or (selected and base != selected):
                return {}
            return {key: str(settings.value(key))
                    for key in settings.allKeys()
                    if key.startswith(WIZARD_THEME_GROUP)}
        finally:
            settings.endGroup()
    except Exception:
        return {}


def resolve_theme_tokens(engine_path=None):
    """Resolve the active {TokenName: value} map. Builtins, shipped default file,
    the editor's selected theme, then the editor's live Applied edits -- each layer
    overrides the last. engine_path enables the theme-file overlay; the working
    overrides come from QSettings. Omit engine_path / run with no editor to stay
    standalone (the upper layers are no-ops)."""
    tokens = dict(_BUILTIN_THEME_TOKENS)
    tokens.update(_load_theme_file(DEFAULT_THEME_FILE))
    tokens.update(_load_engine_theme_overlay(engine_path))
    tokens.update(_load_editor_working_overrides())
    return tokens


def apply_theme_tokens(qss, tokens):
    """Substitute every $TokenName in a stylesheet with its theme value. Longest
    names first so no token name is a prefix-collision of another."""
    if not tokens:
        return qss
    pattern = re.compile(
        "|".join(re.escape(THEME_TOKEN_PREFIX + name)
                 for name in sorted(tokens, key=len, reverse=True)))
    return pattern.sub(lambda m: tokens[m.group(0)[len(THEME_TOKEN_PREFIX):]], qss)


# ============================================================================
# Command System Data Classes
# ============================================================================

@dataclass
class InputVarDef:
    """Definition of a template input variable"""
    var_name: str
    input_type: str  # "text", "toggle", "dropdown", "int", "float"
    title: str
    default_value: Any = ""
    description: str = ""
    required: bool = False
    options: List[str] = field(default_factory=list)  # For dropdown type
    show_if: Optional[str] = None  # Project condition key, e.g. "hasEditor"
    min_value: Optional[float] = None  # For int/float types
    max_value: Optional[float] = None  # For int/float types


@dataclass
class CommandDef:
    """Definition of a process command from template.json"""
    command: str
    args: Dict[str, str] = field(default_factory=dict)
    condition: Optional[str] = None


@dataclass
class ScopedCommandsDef:
    """Conditional dispatch block in a process_commands list.

    Authored as:
      { "scope": "${integration_mode}",
        "branches": {
          "VariantA": [ ...command entries... ],
          "VariantB": [ ...command entries... ]
        },
        "default": [ ...command entries... ]   # optional
      }

    At execution time the wizard resolves the scope string against the user's
    input values, picks the matching branch (or the default block, or nothing
    if neither resolves), and inlines the branch's commands in place. Branches
    may themselves contain nested ScopedCommandsDef entries -- expansion is
    recursive.

    Scoping is OPTIONAL. A process_commands list with no scope blocks behaves
    exactly as before. Existing per-command `condition:` fields keep working
    inside branches.
    """
    scope: str
    branches: Dict[str, List[Any]] = field(default_factory=dict)
    default: List[Any] = field(default_factory=list)
    # Optional outer condition -- if false, the entire scope block is skipped.
    condition: Optional[str] = None


@dataclass
class CopyFileDef:
    """Definition of a file to copy with optional condition"""
    file: str
    is_templated: bool = True
    is_interface: bool = False
    condition: Optional[str] = None
    cleanup_hint: Optional[str] = None  # "interface" or "editor" -- how to scrub refs when excluded
    is_editor: bool = False             # file belongs to the editor module cmake target
    is_test: bool = False               # file belongs to the test cmake target
    # When True, the file is staged (so the o3de CLI runs ${var} substitution
    # on it) but is NOT bulk-merged into dest_root. A process_commands entry
    # -- typically copy_file_to -- is expected to consume it from the staging
    # dir and write it to a non-default destination. Defaults to False so
    # every legacy template behaves identically to today.
    exclude_from_merge: bool = False


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
# Command System Infrastructure (imported from command_plugin.py)
# ============================================================================
# CommandContext, WizardCommand, CommandRegistry, CommandPluginLoader,
# CMakeTarget, CMakeAnalyzer are imported at the top of this file.


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
# Wizard Command Implementations (loaded dynamically from commands/ directory)
# ============================================================================
# Commands are now individual .py files in the commands/ directory.
# They self-register via @CommandRegistry.register() when loaded by CommandPluginLoader.
# See command_plugin.py for the plugin infrastructure.




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
            gem_dirs = self._resolve_project_gem_paths(project_path, engine_path)
            for gem_dir in gem_dirs:
                gem_templates = gem_dir / "Templates"
                if gem_templates.is_dir() and gem_templates.resolve() not in scanned_dirs:
                    scanned_dirs.add(gem_templates.resolve())
                    templates.extend(self._scan_directory(gem_templates))

        # Sort by display name
        templates.sort(key=lambda t: t.display_name.lower())

        return templates

    def _resolve_project_gem_paths(self, project_path: Path,
                                   engine_path: Optional[Path] = None) -> List[Path]:
        """Resolve gem directory paths for a project using the o3de manifest API.

        Uses manifest.get_project_enabled_gems() when engine_path is available.
        Falls back to manual project.json + o3de_manifest.json parsing otherwise.
        """
        # --- Primary path: use o3de manifest API ---
        if engine_path:
            try:
                with o3de_manifest_api(engine_path) as manifest:
                    mapping = manifest.get_project_enabled_gems(project_path) or {}
                    return [Path(p) for p in mapping.values() if Path(p).is_dir()]
            except Exception as e:
                self.log(f"Warning: manifest API unavailable ({e}), falling back to manual resolution")

        # --- Fallback: manual resolution via project.json + o3de_manifest.json ---
        gem_paths = []

        project_json = project_path / "project.json"
        if not project_json.is_file():
            return gem_paths

        try:
            project_data = json.loads(project_json.read_text(encoding="utf-8"))
        except Exception:
            return gem_paths

        gem_names_raw = project_data.get("gem_names", [])
        project_gem_names = set()
        for name in gem_names_raw:
            clean = name.split("==")[0].split(">=")[0].split("<=")[0].split("~=")[0].strip()
            project_gem_names.add(clean.lower())

        for ext_sub in project_data.get("external_subdirectories", []):
            ext_path = (project_path / ext_sub).resolve()
            if ext_path.is_dir():
                gem_paths.append(ext_path)

        manifest_path = Path.home() / ".o3de" / "o3de_manifest.json"
        if not manifest_path.is_file():
            return gem_paths

        try:
            mdata = json.loads(manifest_path.read_text(encoding="utf-8"))
        except Exception:
            return gem_paths

        already_resolved = {p.resolve() for p in gem_paths}
        for ext_dir_str in mdata.get("external_subdirectories", []):
            ext_dir = Path(ext_dir_str)
            if not ext_dir.is_dir():
                continue
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
                options=var_def.get("options", []),
                show_if=var_def.get("show_if"),
                min_value=var_def.get("min_value"),
                max_value=var_def.get("max_value")
            ))

        # Parse process commands. Each entry is either:
        #   * a regular command  -> CommandDef
        #   * a scope/branches block (class_wizard-only) -> ScopedCommandsDef
        # See _parse_command_entry for the recursion handling nested branches.
        process_commands = [
            self._parse_command_entry(entry)
            for entry in wizard_config.get("process_commands", [])
        ]
        process_commands = [c for c in process_commands if c is not None]

        # Parse copyFiles with conditions
        copy_files = []
        for file_def in data.get("copyFiles", []):
            is_interface = file_def.get("isInterface", False)
            # cleanup_hint defaults to "interface" when isInterface is true for backward compatibility
            default_hint = "interface" if is_interface else None
            copy_files.append(CopyFileDef(
                file=file_def.get("file", ""),
                is_templated=file_def.get("isTemplated", True),
                is_interface=is_interface,
                condition=file_def.get("condition"),
                cleanup_hint=file_def.get("cleanup_hint", default_hint),
                is_editor=file_def.get("isEditor", False),
                is_test=file_def.get("isTest", False),
                exclude_from_merge=file_def.get("excludeFromMerge", False)
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

    @classmethod
    def _parse_command_entry(cls, entry: Dict[str, Any]):
        """Parse a single process_commands entry.

        Returns CommandDef for a regular command, ScopedCommandsDef for a
        scope/branches dispatch block, or None if the entry is unparseable.
        Branch lists are recursively parsed -- a branch may contain nested
        scope blocks.
        """
        if not isinstance(entry, dict):
            return None

        # Scope/branches dispatch block.
        if "scope" in entry and "branches" in entry:
            raw_branches = entry.get("branches", {}) or {}
            branches: Dict[str, List[Any]] = {}
            for branch_name, branch_cmds in raw_branches.items():
                if not isinstance(branch_cmds, list):
                    continue
                branches[str(branch_name)] = [
                    cls._parse_command_entry(sub) for sub in branch_cmds
                ]
                branches[str(branch_name)] = [
                    c for c in branches[str(branch_name)] if c is not None
                ]
            raw_default = entry.get("default", []) or []
            default_cmds: List[Any] = []
            if isinstance(raw_default, list):
                default_cmds = [cls._parse_command_entry(sub) for sub in raw_default]
                default_cmds = [c for c in default_cmds if c is not None]
            return ScopedCommandsDef(
                scope=str(entry.get("scope", "")),
                branches=branches,
                default=default_cmds,
                condition=entry.get("condition"),
            )

        # Regular command.
        return CommandDef(
            command=entry.get("command", ""),
            args=entry.get("args", {}),
            condition=entry.get("condition"),
        )

    @staticmethod
    def expand_scoped_commands(commands: List[Any], resolver: 'VariableResolver') -> List['CommandDef']:
        """Flatten any ScopedCommandsDef entries into concrete CommandDef sequences.

        - Resolves the scope's ${var} expression against the resolver.
        - Picks the matching branch by case-sensitive name match.
        - Falls back to the `default` block if the scope value matches no branch.
        - Skips the entire block if the optional outer `condition` is false.
        - Recurses into nested scope blocks inside the chosen branch.

        Templates with no scope blocks pass through unchanged. Existing per-command
        `condition:` semantics are preserved -- this expander only handles scope
        dispatch; condition evaluation still happens later in the executor loop.
        """
        out: List[CommandDef] = []
        for item in commands:
            if isinstance(item, ScopedCommandsDef):
                if not resolver.evaluate_condition(item.condition):
                    continue
                resolved_value = resolver.resolve(item.scope)
                branch = item.branches.get(resolved_value)
                if branch is None:
                    branch = item.default
                if branch:
                    out.extend(WizardTemplateScanner.expand_scoped_commands(branch, resolver))
            elif isinstance(item, CommandDef):
                out.append(item)
            # Anything else (None, malformed entries) is silently dropped.
        return out


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


def detect_engine_path_fallback() -> Optional[Path]:
    """Best-effort engine path when the launch command did not supply one.

    1. azlmbr.paths.engroot -- available when embedded in the O3DE Editor.
    2. The engine tree this script lives in: <engine>/Tools/ClassCreationWizard.
    """
    try:
        import azlmbr.paths
        root = azlmbr.paths.engroot
        if root and (Path(root) / "engine.json").exists():
            return Path(root).resolve()
    except Exception:
        pass

    candidate = Path(__file__).resolve().parents[2]
    if (candidate / "engine.json").exists():
        return candidate
    return None


def detect_project_path_fallback() -> Optional[Path]:
    """Best-effort project path when the launch command did not supply one.

    Uses azlmbr.paths.projectroot, available when embedded in the O3DE Editor.
    This covers the case where AZ::Utils::GetProjectPath() returned empty and
    the wizard was launched with an empty --project-path argument.
    """
    try:
        import azlmbr.paths
        root = azlmbr.paths.projectroot
        if root and Path(root).is_dir():
            return Path(root).resolve()
    except Exception:
        pass
    return None


# ============================================================================
# CMake Analysis (imported from command_plugin.py)
# ============================================================================
# CMakeTarget and CMakeAnalyzer are imported at the top of this file.


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
        with o3de_manifest_api(engine_path) as manifest:
            mapping = manifest.get_project_enabled_gems(
                project_path,
                include_dependencies=include_dependencies
            ) or {}

            # Build an allowlist of user-registered gem roots using the manifest API.
            # Covers: ~/.o3de/o3de_manifest.json external_subdirectories and
            #         project.json external_subdirectories.
            # This allowlist lets us skip engine-bundled gems in pre-built engine setups
            # where those gems live outside engine_root.
            user_gem_roots: set = set()
            for ext in (manifest.get_manifest_external_subdirectories() or []):
                if not ext:
                    continue
                resolved = Path(ext).resolve()
                if resolved.is_dir():
                    user_gem_roots.add(os.path.normcase(str(resolved)))
            for ext in (manifest.get_project_external_subdirectories(project_path) or []):
                if not ext:
                    continue
                resolved = Path(ext).resolve()
                if resolved.is_dir():
                    user_gem_roots.add(os.path.normcase(str(resolved)))

        engine_root = engine_path.resolve()

        gems = []

        for namespec, gem_path_str in mapping.items():
            # The manifest can map an enabled gem to a None/empty path when the
            # "project" is actually the engine root or a relocated prebuilt
            # engine. Skip those rather than letting Path(None) raise
            # "expected str, bytes or os.PathLike object, not NoneType".
            if not gem_path_str:
                continue
            gem_path = Path(gem_path_str).resolve()

            # Filter 1: skip gems that live inside the engine directory
            try:
                gem_path.relative_to(engine_root)
                continue
            except ValueError:
                pass

            # Filter 2: if user_gem_roots is populated, only include gems registered there.
            # Engine-bundled gems that live outside engine_root (pre-built engine scenario)
            # will not appear in the manifest and are skipped here.
            if user_gem_roots and os.path.normcase(str(gem_path)) not in user_gem_roots:
                continue

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

            # Exclude conditional files and scrub references from staging area
            ConditionalFileExcluder(logger=self.log).process(
                stage_dir, template.copy_files, resolver
            )

            # Handle setreg files if present. Any .setreg listed in copyFiles
            # (e.g. DataAssetRegistry, PostProcessRegistry, custom) is routed
            # to <gem>/Registry/ rather than going through the source merge.
            for copy_def in template.copy_files:
                if '.setreg' in copy_def.file and resolver.evaluate_condition(copy_def.condition):
                    self._handle_setreg_file(
                        stage_dir,
                        config['dest_dir'],
                        config['namespace']
                    )
                    break

            # Build the merge-exclusion set: any copyFiles entry with
            # excludeFromMerge=True (after condition evaluation) is staged but
            # NOT bulk-copied into dest. Such files are expected to be handled
            # by an explicit copy_file_to / copy_glob_to command in
            # process_commands. Legacy templates that never set the flag get
            # the empty set and behave identically to before.
            merge_excluded = {
                resolver.resolve(cf.file).replace("\\", "/")
                for cf in template.copy_files
                if cf.exclude_from_merge and resolver.evaluate_condition(cf.condition)
            }

            # Merge stage to destination
            created, skipped = self._merge_stage_into_dest(
                stage=stage_dir,
                dest=config['dest_dir'],
                skip_existing=True,
                strip_comments=config.get('strip_comments', True),
                keep_license=config.get('keep_license', False),
                merge_excluded=merge_excluded
            )

            self.log(f"Created {created} file(s), skipped {skipped} existing file(s)")

            # Execute process commands
            # Registration commands only run with automatic_register;
            # all other commands (replace_text, add_gem_dependency, etc.) always run.
            # Commands self-declare via is_registration_command property.
            auto_register = config.get('automatic_register', False)

            if template.process_commands:
                self.log("Executing process commands...")

                # Flatten any scope/branches blocks into a concrete CommandDef
                # sequence based on the user's input values. Templates that
                # don't use scope blocks pass through unchanged. See
                # WizardTemplateScanner.expand_scoped_commands for full rules.
                expanded_commands = WizardTemplateScanner.expand_scoped_commands(
                    template.process_commands, resolver
                )

                # Build resolved copy_files list for active (condition-passing) files only.
                # Commands use this to look up actual file paths rather than assuming Source/.
                resolved_copy_files = [
                    (resolver.resolve(cf.file), cf)
                    for cf in template.copy_files
                    if resolver.evaluate_condition(cf.condition)
                ]

                ctx = CommandContext(
                    dest_root=config['dest_dir'],
                    namespace=config['namespace'],
                    component_name=config['component_name'],
                    build_target=config.get('build_target'),
                    variables=resolver.variables,
                    logger=self.log,
                    engine_path=self.engine_path,
                    copy_files=resolved_copy_files,
                    template_path=template.template_path,
                    stage_dir=stage_dir
                )

                for cmd_def in expanded_commands:
                    # Skip registration commands when automatic_register is off
                    if CommandRegistry.is_registration_command(cmd_def.command) and not auto_register:
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

    def _merge_stage_into_dest(self, stage: Path, dest: Path,
                            skip_existing: bool, strip_comments: bool,
                            keep_license: bool,
                            merge_excluded: Optional[set] = None) -> Tuple[int, int]:
        """Merge staged files into destination.

        Files in `merge_excluded` (relative POSIX paths inside `stage`) are
        left untouched in staging for later consumption by an explicit
        copy_file_to / copy_glob_to command. The staging dir is wiped at the
        end of the run regardless, so anything not consumed disappears.
        """
        created = skipped = 0
        dest.mkdir(parents=True, exist_ok=True)
        merge_excluded = merge_excluded or set()

        for src_file in stage.rglob("*"):
            if src_file.is_dir():
                continue

            # Skip setreg files - they're handled separately
            if src_file.suffix == '.setreg':
                self.log(f"Skipping setreg in merge: {src_file.name}")
                continue

            rel = src_file.relative_to(stage)
            rel_posix = str(rel).replace("\\", "/")
            if rel_posix in merge_excluded:
                self.log(f"Skipping merge (handled by explicit copy command): {rel_posix}")
                continue

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
        """Move every .setreg produced by the staging step into <gem>/Registry/.

        Templates that ship a setreg (DataAsset's DataAssetRegistry, the
        FullscreenPostProcess template's PostProcessRegistry, or any custom
        registry a future template adds) place it at the template root. The
        generic merge step in _merge_stage_into_dest skips .setreg files, so
        this routine owns the move.

        Each staged .setreg is moved to <gem_root>/Registry/<original_name>.
        Existing destination files are NOT overwritten (the matching
        register_*_setreg command is responsible for in-place updates).
        """
        target_dir = self._resolve_registry_dir(Path(dest_root).resolve())
        target_dir.mkdir(parents=True, exist_ok=True)
        self.log(f"Routing setreg files to: {target_dir}")

        moved = []
        for staged_setreg in stage_dir.rglob("*.setreg"):
            if not staged_setreg.is_file():
                continue
            target_setreg = target_dir / staged_setreg.name
            try:
                if target_setreg.exists():
                    self.log(f"Setreg already exists (left in place): {target_setreg.name}")
                else:
                    shutil.copy2(staged_setreg, target_setreg)
                    self.log(f"Copied setreg: {staged_setreg.name} -> {target_setreg}")
                moved.append(target_setreg)
            except Exception as e:
                self.log(f"Error handling setreg {staged_setreg.name}: {e}")

        if not moved:
            self.log("No setreg files found in stage")
            return None
        return moved[0]

    @staticmethod
    def _resolve_registry_dir(dest_root: Path) -> Path:
        """Pick the gem's canonical Registry directory based on dest_root layout.

        - <project>/Gem  -> <project>/Gem/Registry
        - <gem>/Code     -> <gem>/Registry            (external gems)
        - <gem>/         -> <gem>/Registry            (gem-root passed directly)
        - anything else  -> <dest>/Registry           (fallback)
        """
        if dest_root.name == "Gem":
            return dest_root / "Registry"
        if dest_root.name == "Code" and (dest_root.parent / "gem.json").is_file():
            return dest_root.parent / "Registry"
        if (dest_root / "gem.json").is_file():
            return dest_root / "Registry"
        return dest_root / "Registry"
    
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


# ============================================================================
# PySide6 GUI Application
# ============================================================================

class ClassWizardWindow(QMainWindow):
    """Main window for the O3DE Class Creation Wizard"""

    # Design size for the window. Re-asserted after show() in _do_center because
    # embedded hosts (the O3DE Editor's QApplication) ignore the pre-show resize.
    DEFAULT_WIDTH = 650
    DEFAULT_HEIGHT = 900

    def __init__(self, engine_path: Path, project_path: Optional[Path] = None):
        super().__init__()

        self.engine_path = engine_path
        self.project_path = project_path

        self.setWindowTitle("O3DE Class Creation Wizard")
        self.setMinimumSize(600, 800)
        self.resize(self.DEFAULT_WIDTH, self.DEFAULT_HEIGHT)

        # Initialize data
        self.gems: List[GemInfo] = []
        self.targets: List[CMakeTarget] = []
        self.selected_gem: Optional[GemInfo] = None
        self.selected_target: Optional[CMakeTarget] = None
        self.project_conditions: Dict[str, bool] = {}  # e.g. {"hasEditor": True}

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

        # Creation status section
        layout.addWidget(self._create_status_section())

        # Buttons
        layout.addWidget(self._create_button_section())
    
    def _create_destination_section(self) -> QGroupBox:
        """Create the destination selection section"""
        group = QGroupBox("Destination")
        layout = QFormLayout()
        
        # Target combo
        self.target_combo = BoundedComboBox()
        self.target_combo.currentTextChanged.connect(self._on_target_changed)
        self.target_combo.currentTextChanged.connect(self._update_creation_status)
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
        self.package_combo = BoundedComboBox()
        self._add_form_row(layout, "Package:", self.package_combo)
        
        # Namespace
        self.namespace_edit = QLineEdit()
        if self.project_path:
            self.namespace_edit.setText(self.project_path.stem)
        self.namespace_edit.textChanged.connect(self._update_creation_status)
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
        self.component_name_edit.textChanged.connect(self._update_creation_status)
        self._add_form_row(layout, "Name:", self.component_name_edit)

        # Component type
        self.component_type_combo = BoundedComboBox()
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

    def _create_status_section(self) -> QWidget:
        """Create the creation status display section"""
        widget = QWidget()
        layout = QVBoxLayout()
        layout.setContentsMargins(10, 2, 10, 2)

        self.creation_status_label = QLabel("Creating...")
        self.creation_status_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.creation_status_label.setStyleSheet(
            _STATUS_STYLE_BASE.format(color=_STATUS_COLOR_PENDING)
        )
        self.creation_status_label.setWordWrap(True)
        layout.addWidget(self.creation_status_label)

        widget.setLayout(layout)
        return widget

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
        icons_dir = Path(__file__).resolve().parent / "icons"
        stylesheet = apply_theme_tokens(WINDOW_STYLESHEET, resolve_theme_tokens(self.engine_path)) \
            .replace("COMBO_ARROW_URL",    (icons_dir / "combo_arrow.svg").as_posix()) \
            .replace("COMBO_ARROW_UP_URL", (icons_dir / "combo_arrow_up.svg").as_posix())
        self.setStyleSheet(stylesheet)
    
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

            # Load command plugins (only if not already loaded by main())
            if not CommandRegistry.list_commands():
                tool_dir = Path(__file__).resolve().parent
                plugin_loader = CommandPluginLoader(logger=self.log)
                gem_paths = [gem.path for gem in self.gems if hasattr(gem, 'path') and gem.path]
                plugin_loader.discover_and_load(tool_dir, self.project_path, gem_paths)
            else:
                self.log(f"Command plugins already loaded ({len(CommandRegistry.list_commands())} commands)")

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
            # Pin "Basic Component" first, sort the rest alphabetically
            def _sort_key(t):
                return (0, "") if t.display_name == "Basic Component" else (1, t.display_name.lower())

            for template in sorted(self.available_templates, key=_sort_key):
                self.component_type_combo.addItem(template.display_name, userData=template)

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

            # Detect project conditions for show_if filtering on dynamic inputs
            self.project_conditions = self._detect_project_conditions(gem_path)

            # Refresh dynamic fields -- some may now appear/disappear based on conditions
            self._on_component_type_changed(self.component_type_combo.currentText())

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

    def _detect_project_conditions(self, gem_path: Path) -> Dict[str, bool]:
        """Detect project-level feature conditions from gem directory structure.

        Results are used to show or hide template dynamic inputs that carry a
        'show_if' key.  Add new conditions here as the project grows.

        Conditions:
            hasEditor   -- gem has an Editor source directory or editor cmake files
            hasTesting  -- gem has a Tests directory or test cmake files
        """
        conditions: Dict[str, bool] = {
            'hasEditor': False,
            'hasTesting': False,
        }

        if not gem_path or not gem_path.is_dir():
            return conditions

        code_dir = gem_path / "Code"
        search_root = code_dir if code_dir.is_dir() else gem_path

        # hasEditor: Editor source folder OR any *editor*.cmake file
        if (search_root / "Source" / "Editor").is_dir():
            conditions['hasEditor'] = True
        elif any(search_root.rglob("*editor*.cmake")):
            conditions['hasEditor'] = True

        # hasTesting: Tests source folder OR any *tests*.cmake file
        if (search_root / "Tests").is_dir():
            conditions['hasTesting'] = True
        elif any(search_root.rglob("*tests*.cmake")):
            conditions['hasTesting'] = True

        return conditions

    def _update_creation_status(self):
        """Update the creation status display based on current inputs"""
        component_name = self.component_name_edit.text().strip()
        template = self.component_type_combo.currentData()
        namespace = self.namespace_edit.text().strip() if hasattr(self, 'namespace_edit') else ""
        target = self.target_combo.currentText() if hasattr(self, 'target_combo') else ""

        if not component_name:
            # Show placeholder in grey
            self.creation_status_label.setText("Creating...")
            self.creation_status_label.setStyleSheet(
                _STATUS_STYLE_BASE.format(color=_STATUS_COLOR_PENDING)
            )
            return

        # Build the status text
        if template and isinstance(template, WizardTemplate):
            full_name = f"{component_name}{template.component_suffix}"
        else:
            full_name = f"{component_name}Component"

        status_parts = ["Creating:", full_name]

        if target:
            status_parts.append(f"in {target}")

        # Show filled status in off-white
        self.creation_status_label.setText(" ".join(status_parts))
        self.creation_status_label.setStyleSheet(
            _STATUS_STYLE_BASE.format(color=_STATUS_COLOR_ACTIVE)
        )

    def _on_component_type_changed(self, comp_type: str):
        """Handle component type selection change"""
        # Clear previous dynamic fields
        for widget in self.dynamic_widgets:
            # Remove row from layout (removes both label and field)
            self.dynamic_layout.removeRow(widget)
        self.dynamic_widgets.clear()

        # Update the creation status display with new template's suffix
        self._update_creation_status()

        # Check if we have a WizardTemplate for this type
        template = self.component_type_combo.currentData()

        if template and isinstance(template, WizardTemplate):
            # Build dynamic fields from template.input_vars, filtered by show_if conditions
            conditions = getattr(self, 'project_conditions', {})
            for input_def in template.input_vars:
                if input_def.show_if and not conditions.get(input_def.show_if, False):
                    continue
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
            widget = BoundedComboBox()
            widget.addItems(input_def.options)
            if input_def.default_value and input_def.default_value in input_def.options:
                widget.setCurrentText(str(input_def.default_value))
            if input_def.description:
                widget.setToolTip(input_def.description)

        elif input_def.input_type == "int":
            widget = QSpinBox()
            lo = int(input_def.min_value) if input_def.min_value is not None else 0
            hi = int(input_def.max_value) if input_def.max_value is not None else 2147483647
            widget.setRange(lo, hi)
            try:
                widget.setValue(int(input_def.default_value))
            except (ValueError, TypeError):
                widget.setValue(lo)
            if input_def.description:
                widget.setToolTip(input_def.description)

        elif input_def.input_type == "float":
            widget = QDoubleSpinBox()
            lo = float(input_def.min_value) if input_def.min_value is not None else 0.0
            hi = float(input_def.max_value) if input_def.max_value is not None else 1e308
            widget.setRange(lo, hi)
            try:
                widget.setValue(float(input_def.default_value))
            except (ValueError, TypeError):
                widget.setValue(lo)
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
            field_item = self.dynamic_layout.itemAt(i, QFormLayout.ItemRole.FieldRole)

            if field_item:
                field = field_item.widget()
                if not field:
                    continue

                var_name = field.property("var_name")
                if not var_name:
                    # Fallback to label text for legacy compatibility
                    label_item = self.dynamic_layout.itemAt(i, QFormLayout.ItemRole.LabelRole)
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
                    elif isinstance(field, QDoubleSpinBox):
                        dynamic_fields[var_name] = field.value()
                    elif isinstance(field, QSpinBox):
                        dynamic_fields[var_name] = field.value()

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
        """Finalize size + layout, center, then force the first paint.

        Runs deferred (after show) to correct embedded-host quirks: the
        pre-show resize() is ignored (window opens oversized), the central
        layout is not computed until the first resize, and the window can stay
        unpainted (black) until a resize event arrives.
        """
        # Re-assert the design size; embedded hosts override the pre-show resize.
        self.resize(self.DEFAULT_WIDTH, self.DEFAULT_HEIGHT)

        # Force the central layout to recompute now instead of on first resize.
        central = self.centralWidget()
        if central and central.layout():
            central.layout().activate()

        # Center on the primary screen using the finalized frame geometry.
        screen = QApplication.primaryScreen().geometry()
        window = self.frameGeometry()
        window.moveCenter(screen.center())
        self.move(window.topLeft())

        # Embedded in the editor the window can open unpainted (black) until it
        # receives a resize event. Generate one programmatically -- the same
        # redraw the user otherwise triggers by dragging the window edge.
        QTimer.singleShot(0, self._force_initial_paint)

    def _force_initial_paint(self):
        """Nudge the window size by 1px and back to emit a resize event.

        The nudge is split across two event-loop ticks; a same-tick resize to
        the identical size coalesces to nothing and would not trigger a repaint.
        """
        self.resize(self.DEFAULT_WIDTH, self.DEFAULT_HEIGHT + 1)
        QTimer.singleShot(0, lambda: self.resize(self.DEFAULT_WIDTH, self.DEFAULT_HEIGHT))


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

    # ---- Identity ----
    print(f"\nTemplate: {template.display_name}")
    print(f"  Class Name:  {template.class_name}")
    if template.description:
        print(f"  Description: {template.description}")
    print(f"  Suffix:      {template.component_suffix}")

    # ---- Full command example ----
    print(f"\n  Full Command:")
    print(f"    python ClassWizard.py \\")
    print(f"      --engine-path  <engine-path>   (required)")
    print(f"      --project-path <project-path>  (required)")
    print(f"      --template     {template.class_name}")
    print(f"      --component-name <Name>         (required)")
    print(f"      --namespace      <GemName>       (required)")
    print(f"      --automatic-register             (optional: register in CMake + modules)")
    print(f"      --keep-comments                  (optional: preserve template comments)")
    print(f"      --default-license                (optional: include license header)")

    for var in template.input_vars:
        arg_name  = f"--{var.var_name.replace('_', '-')}"
        required_str  = "required" if var.required else "optional"
        show_if_str   = f", needs {var.show_if}" if var.show_if else ""

        if var.input_type == "toggle":
            sig = arg_name
        elif var.input_type == "text":
            sig = f"{arg_name} <value>"
        else:
            choices = "|".join(var.options) if var.options else "value"
            sig = f"{arg_name} {{{choices}}}"

        print(f"      {sig:<35} ({required_str}{show_if_str})")

    # ---- Template-specific argument details ----
    if template.input_vars:
        print(f"\n  Template-Specific Arguments:")
        for var in template.input_vars:
            arg_name     = f"--{var.var_name.replace('_', '-')}"
            default_str  = f"  default: {var.default_value}" if var.default_value is not None else ""
            required_str = "  [REQUIRED]" if var.required else ""
            show_if_str  = f"  [show_if: {var.show_if}]" if var.show_if else ""
            print(f"    {arg_name:<28}{var.title}{default_str}{required_str}{show_if_str}")
            if var.description:
                print(f"      {var.description}")

    # ---- Commands that will run ----
    if template.process_commands:
        print(f"\n  Commands ({len(template.process_commands)} total):")
        for cmd in template.process_commands:
            condition_str = f"  [if {cmd.condition}]" if cmd.condition else ""
            if cmd.args:
                args_str = ", ".join(f"{k}={v}" for k, v in cmd.args.items())
                print(f"    {cmd.command}({args_str}){condition_str}")
            else:
                print(f"    {cmd.command}{condition_str}")
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
        elif var.input_type == "int":
            parser.add_argument(
                arg_name,
                type=int,
                default=int(var.default_value) if var.default_value not in (None, "") else 0,
                help=f"{var.title}" + (f": {var.description}" if var.description else "")
            )
        elif var.input_type == "float":
            parser.add_argument(
                arg_name,
                type=float,
                default=float(var.default_value) if var.default_value not in (None, "") else 0.0,
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

    # Fall back to the running editor / script location when the launch command
    # did not supply usable paths. This handles AZ::Utils::GetProjectPath()
    # returning empty, which arrives here as an empty (falsy) --project-path.
    if engine_path is None:
        engine_path = detect_engine_path_fallback()
    if project_path is None:
        project_path = detect_project_path_fallback()

    # Load command plugins from engine/project/gem directories
    if engine_path:
        tool_dir = Path(__file__).resolve().parent
        loader = CommandPluginLoader()
        gem_paths = []
        if project_path:
            scanner = WizardTemplateScanner()
            gem_paths = scanner._resolve_project_gem_paths(project_path)
        loader.discover_and_load(tool_dir, project_path, gem_paths)

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
            window.setAttribute(Qt.WidgetAttribute.WA_DeleteOnClose, True)
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
    except SystemExit:
        raise  # Let an explicit sys.exit (e.g. argparse) propagate normally.
    except Exception as e:
        print(f"Error: {e}")
        traceback.print_exc()
        result = 1

    # Only propagate an exit code for a genuine standalone launch. Inside the O3DE
    # Editor the interpreter is shared and the wizard window lives on after this
    # returns; calling sys.exit() there raises SystemExit into the editor's Python
    # runner, which misreports it as "script failure ... return code -1" even on a
    # clean exit (0). azlmbr is importable only in the editor's embedded interpreter.
    try:
        import azlmbr  # noqa: F401  -- presence signals the embedded editor interpreter
        _embedded_in_editor = True
    except ImportError:
        _embedded_in_editor = False

    if not _embedded_in_editor:
        sys.exit(result if result is not None else 0)