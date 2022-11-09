#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -------------------------------------------------------------------------
"""! This module is for use in bootstrapping the DccScriptingInterface Gem
< dccsi > with O3DE.  The dccsi package if a framework, which includes DCC
tool integrations for O3DE workflows; and also provides technical artists
and out-of-box python development environment.

azlmbr represents the O3DE editor python bindings API
This bootstrap file requires azlmbr and thus only runs within O3DE.

:file: DccScriptingInterface\\editor\\scripts\\boostrap.py
:Status: Prototype
:Version: 0.0.1
:Future: is unknown
:Entrypoint: is an entrypoint and thus configures logging
:Notice:
    Currently windows only (not tested on other platforms)
    No support for legacy DCC tools stuck on py2 (py3 only)
"""
# standard imports
import sys
import os
import site
import subprocess
from pathlib import Path
import logging as _logging
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# global scope
_DCCSI_SLUG = 'DccScriptingInterface'
_MODULENAME = 'DCCsi.Editor.Scripts.bootstrap'
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug('Initializing: {0}.'.format({_MODULENAME}))

# this file
_MODULE_PATH = Path(__file__)
_LOGGER.debug(f'_MODULE_PATH: {_MODULE_PATH.as_posix()}')

# we can't import DccScriptingInterface yet, need to bootstrap core access
#from DccScriptingInterface import PATH_O3DE_TECHART_GEMS
#from DccScriptingInterface import PATH_DCCSIG

# add gems parent, dccsi lives under:
# < o3de >\Gems\AtomLyIntegration\TechnicalArt
PATH_O3DE_TECHART_GEMS = _MODULE_PATH.parents[3].resolve()
os.chdir(PATH_O3DE_TECHART_GEMS.as_posix())

#sys.path.append(PATH_O3DE_TECHART_GEMS.as_posix())
from DccScriptingInterface import add_site_dir
add_site_dir(PATH_O3DE_TECHART_GEMS)

# < o3de >\Gems\AtomLyIntegration\TechnicalArt\< dccsi >
PATH_DCCSIG = _MODULE_PATH.parents[2].resolve()
from DccScriptingInterface.azpy.constants import ENVAR_PATH_DCCSIG
os.environ[ENVAR_PATH_DCCSIG] = PATH_DCCSIG.as_posix()

# make the dccsi cwd
os.chdir(PATH_DCCSIG.as_posix())
PATH_DCCSIG_SETTINGS = PATH_DCCSIG.joinpath('settings.json')
os.environ['SETTINGS_MODULE_FOR_DYNACONF'] = PATH_DCCSIG_SETTINGS.as_posix()
# -------------------------------------------------------------------------


# ---- debug stuff --------------------------------------------------------
# local dccsi imports
# this accesses common global state, e.g. DCCSI_GDEBUG (is True or False)
from DccScriptingInterface.globals import *

# temproary force enable these during development, as bootstrap is a entrypoint
#DCCSI_GDEBUG = True
#DCCSI_DEV_MODE = True

# enable this if you are having difficulty with debugging
# the subprocess booting start.py (note: will block editor)
DCCSI_LOCAL_DEBUG = False # <-- for code branch in this module only

# auto-attach ide debugging at the earliest possible point in module
from DccScriptingInterface.azpy.config_utils import attach_debugger
if DCCSI_DEV_MODE: # from DccScriptingInterface.globals
    attach_debugger()
#
import DccScriptingInterface.config as dccsi_core_config

# note: if you used win_launch_wingide.bat, settings will be over populated
# because of the .bat file env chain that includes active apps

# note: initializing the config PySide2 access here may not be ideal
# it will set envars that may propogate, and have have the side effect
# of interfering with apps like Wing because it is a Qt5 app and the
# envars set cause a boot failure

# if your tool is running inside of o3de you already have PySide2/Qt

# if you are launching a standalone tool that does need access,
# that application will need to run and manage config on it's own

# # to do: refactor config.py and implement ConfigClass
# from DccScriptingInterface.config import ConfigClass
# dccsi_config = ConfigClass(config_name='dccsi', auto_set=True)
# for now, use the legacy code, until CoreConfig class is complete
_settings_core = dccsi_core_config.get_config_settings(enable_o3de_python=True,
                                                       enable_o3de_pyside2=False,
                                                       set_env=True)

# configure basic logger
# it is suggested that this be replaced with a common logging module later
if DCCSI_GDEBUG or DCCSI_DEV_MODE:
    DCCSI_LOGLEVEL = _logging.DEBUG
    _LOGGER.setLevel(DCCSI_LOGLEVEL) # throttle up help

_logging.basicConfig(level=DCCSI_LOGLEVEL,
                     format=FRMT_LOG_LONG,
                     datefmt='%m-%d %H:%M')

_LOGGER = _logging.getLogger(_MODULENAME)
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
O3DE_EDITOR = Path(sys.executable).resolve() # executible
_LOGGER.debug(f'The sys.executable is: {O3DE_EDITOR}')

# O3DE imports
import azlmbr
import azlmbr.bus
import azlmbr.paths

# path devs might be interested in retreiving
_LOGGER.debug(f'engroot: {azlmbr.paths.engroot}')
_LOGGER.debug(f'executableFolder: {azlmbr.paths.executableFolder}')
_LOGGER.debug(f'log: {azlmbr.paths.log}')
_LOGGER.debug(f'products: {azlmbr.paths.products}')
_LOGGER.debug(f'projectroot: {azlmbr.paths.projectroot}')

# base paths
O3DE_DEV = Path(azlmbr.paths.engroot).resolve()
PATH_O3DE_BIN = Path(azlmbr.paths.executableFolder).resolve()
PATH_O3DE_PROJECT = Path(azlmbr.paths.projectroot).resolve()

# the DCCsi Gem expects QtForPython Gem is active
try:
    azlmbr.qt.QtForPythonRequestBus(azlmbr.bus.Broadcast, 'IsActive')
except Exception as e:
    _LOGGER.error(f'O3DE Qt / PySide2 not available, exception: {e}')
    raise e

# debug logging, these are where Qt lives
params = azlmbr.qt.QtForPythonRequestBus(azlmbr.bus.Broadcast, 'GetQtBootstrapParameters')
_LOGGER.debug(f'qtPluginsFolder: {params.qtPluginsFolder}')
_LOGGER.debug(f'qtBinaryFolder: {params.qtBinaryFolder}')

#PySide2 imports
from PySide2 import QtWidgets
from PySide2.QtWidgets import QMenuBar, QMenu, QAction
from PySide2 import QtGui
from PySide2.QtCore import Slot, QObject, QUrl
from shiboken2 import wrapInstance, getCppPointer

# import additional O3DE QtForPython Gem modules
import az_qt_helpers

# additional DCCsi imports that utilize PySide2
from DccScriptingInterface.azpy.shared.ui.samples import SampleUI
from DccScriptingInterface.Editor.Scripts.ui import add_action
from DccScriptingInterface.Editor.Scripts.ui import create_menu
from DccScriptingInterface.Editor.Scripts.ui import start_service
from DccScriptingInterface.Editor.Scripts.ui import click_action_sampleui
# -------------------------------------------------------------------------


# - slot ------------------------------------------------------------------
# as the list of slots/actions grows, refactor into sub-modules
@Slot()
def click_action_blender_start() -> start_service:
    """Start Blender DCC application"""
    _LOGGER.debug(f'Clicked: click_action_blender_start')
    from DccScriptingInterface.Tools.DCC.Blender import PATH_DCCSI_TOOLS_DCC_BLENDER
    py_file = Path(PATH_DCCSI_TOOLS_DCC_BLENDER, 'start.py').resolve()
    return start_service(py_file)
# -------------------------------------------------------------------------


# - slot ------------------------------------------------------------------
@Slot()
def click_action_blender_docs():
    """Open Blender DCCsi docs (readme currently)"""
    _LOGGER.debug(f'Clicked: click_action_blender_docs')

    url = "https://github.com/o3de/o3de/tree/development/Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface/Tools/DCC/Blender/readme.md"

    return QtGui.QDesktopServices.openUrl(QUrl(url, QUrl.TolerantMode))
# -------------------------------------------------------------------------


# - slot ------------------------------------------------------------------
@Slot()
def click_action_maya_start() -> start_service:
    """Start Maya DCC application"""
    _LOGGER.debug(f'Clicked: click_action_maya_start')
    from DccScriptingInterface.Tools.DCC.Maya import PATH_DCCSI_TOOLS_DCC_MAYA
    py_file = Path(PATH_DCCSI_TOOLS_DCC_MAYA, 'start.py').resolve()
    return start_service(py_file)
# -------------------------------------------------------------------------


# - slot ------------------------------------------------------------------
@Slot()
def click_action_maya_docs():
    """Open Maya DCCsi docs (readme currently)"""
    _LOGGER.debug(f'Clicked: click_action_maya_docs')

    url = "https://github.com/o3de/o3de/tree/development/Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface/Tools/DCC/Maya/readme.md"

    return QtGui.QDesktopServices.openUrl(QUrl(url, QUrl.TolerantMode))
# -------------------------------------------------------------------------


# - slot ------------------------------------------------------------------
@Slot()
def click_action_wing_start() -> start_service:
    """Start Wing IDE"""
    _LOGGER.debug(f'Clicked: click_action_wing_start')
    from DccScriptingInterface.Tools.IDE.Wing import PATH_DCCSI_TOOLS_IDE_WING
    py_file = Path(PATH_DCCSI_TOOLS_IDE_WING, 'start.py').resolve()
    return start_service(py_file)
# -------------------------------------------------------------------------


# - slot ------------------------------------------------------------------
@Slot()
def click_action_wing_docs():
    """Open Wing IDE DCCsi docs (readme currently)"""
    _LOGGER.debug(f'Clicked: click_action_wing_docs')

    url = "https://github.com/o3de/o3de/tree/development/Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface/Tools/IDE/Wing/readme.md"

    return QtGui.QDesktopServices.openUrl(QUrl(url, QUrl.TolerantMode))
# -------------------------------------------------------------------------


# - slot ------------------------------------------------------------------
from DccScriptingInterface.Editor.Scripts.about import DccsiAbout

@Slot()
def click_action_about():
    """Open DCCsi About Dialog"""
    _LOGGER.debug(f'Clicked: click_action_about')

    EDITOR_MAIN_WINDOW = az_qt_helpers.get_editor_main_window()

    about_dialog = DccsiAbout(EDITOR_MAIN_WINDOW)
    return about_dialog.exec_()
# -------------------------------------------------------------------------

# -------------------------------------------------------------------------
def bootstrap_Editor():
    """! Put bootstrapping code here to execute in O3DE Editor.exe"""

    START_SLUG = 'Start'
    HELP_SLUG = 'Help ...'

    # Editor main window (should work with any standalone o3de editor exe)
    EDITOR_MAIN_WINDOW = az_qt_helpers.get_editor_main_window()

    menubar = EDITOR_MAIN_WINDOW.menuBar()

    dccsi_menu = create_menu(parent=menubar, title ='StudioTools')

    # Editor MenuBar, Studio Tools > DCC
    # nest a menu with hooks to start python DCC apps like Blender
    dccsi_dcc_menu = create_menu(parent=dccsi_menu, title="DCC")

    dccsi_dcc_menu_blender = create_menu(parent=dccsi_dcc_menu, title="Blender")

    # Editor MenuBar, Studio Tools > DCC > Blender
    # Action manager should make it easier for each tool to
    # inject themselves, instead of this module manually
    # bootstrapping each. Suggestion for a follow up PR.
    action_start_blender = add_action(parent=dccsi_dcc_menu_blender,
                                      title=START_SLUG,
                                      action_slot = click_action_blender_start)

    action_docs_blender = add_action(parent=dccsi_dcc_menu_blender,
                                      title=HELP_SLUG,
                                      action_slot = click_action_blender_docs)

    # Editor MenuBar, Studio Tools > DCC > Maya
    dccsi_dcc_menu_maya = create_menu(parent=dccsi_dcc_menu, title="Maya")

    action_start_maya = add_action(parent=dccsi_dcc_menu_maya,
                                   title=START_SLUG,
                                   action_slot = click_action_maya_start)

    action_docs_maya = add_action(parent=dccsi_dcc_menu_maya,
                                  title=HELP_SLUG,
                                  action_slot = click_action_maya_docs)

    # Editor MenuBar, Studio Tools > IDE
    # nest a menu with hooks to start python IDE tools like Wing
    dccsi_ide_menu = create_menu(parent=dccsi_menu, title="IDE")

    # MEditor MenuBar, Studio Tools > IDE > Wing
    dccsi_ide_menu_wing = create_menu(parent=dccsi_ide_menu, title="Wing")

    action_start_wingide = add_action(parent=dccsi_ide_menu_wing,
                                       title=START_SLUG,
                                       action_slot = click_action_wing_start)

    action_start_blender = add_action(parent=dccsi_ide_menu_wing,
                                      title=HELP_SLUG,
                                      action_slot = click_action_wing_docs)

    # Editor MenuBar, Studio Tools > Examples
    # nest a menu with samples (promote extensibility)
    dccsi_examples_menu = create_menu(parent=dccsi_menu,
                                            title="Examples")

    # MEditor MenuBar, Studio Tools > Examples > SampleUI
    action_start_sampleui = add_action(parent=dccsi_examples_menu,
                                       title="SampleUI",
                                       action_slot = click_action_sampleui)

    # MEditor MenuBar, Studio Tools > About
    action_start_sampleui = add_action(parent=dccsi_menu,
                                       title="About",
                                       action_slot = click_action_about)

    return dccsi_menu
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def bootstrap_MaterialEditor():
    """Put bootstrapping code here to execute in O3DE MaterialEditor.exe"""
    pass
    return None
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def bootstrap_MaterialCanvas():
    """Put bootstrapping code here to execute in O3DE MaterialCanvas.exe"""
    pass
    return None
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def bootstrap_AssetProcessor():
    """Put boostrapping code here to execute in O3DE AssetProcessor.exe"""
    pass
    return None
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def bootstrap_AssetBuilder():
    """Put boostrapping code here to execute in O3DE AssetBuilder.exe"""
    pass
    return None
# -------------------------------------------------------------------------


###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    # set and retreive the *basic* env context/_settings on import
    # What application is executing the bootstrap?
    # Python is being run from:
    #    editor.exe
    #    materialeditor.exe
    #    assetprocessor.exe
    #    assetbuilder.exe, or the Python executable.
    # Exclude the .exe so it works on other platforms

    if sys.platform.startswith('win'):

        if O3DE_EDITOR.stem.lower() == "editor":
            # if _DCCSI_GDEBUG then run the pyside2 test
            _settings = bootstrap_Editor()

        elif O3DE_EDITOR.stem.lower() == "materialeditor":
            _settings = bootstrap_MaterialEditor()

        elif O3DE_EDITOR.stem.lower() == "materialcanvas":
            _settings = bootstrap_MaterialEditor()

        elif O3DE_EDITOR.stem.lower() == "assetprocessor":
            _settings = bootstrap_AssetProcessor()

        elif O3DE_EDITOR.stem.lower() == "assetbuilder":
            _settings= bootstrap_AssetBuilder()

        elif O3DE_EDITOR.stem.lower() == "python":
            # in this case, we can re-use the editor settings
            # which will init python and pyside2 access externally
            _settings = bootstrap_Editor()

        else:
            _LOGGER.warning(f'No bootstrapping code for: {O3DE_EDITOR}')

    else:
        _LOGGER.warning(f'Non-windows platforms not implemented or tested.')
    # -------------------------------------------------------------------------
