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
_MODULENAME = 'DCCsi.editor.scripts.bootstrap'
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

# temproary force enable these during development
#DCCSI_GDEBUG = True
DCCSI_DEV_MODE = True

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
_settings_core = dccsi_core_config.get_config_settings(enable_o3de_python=False,
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
from PySide2.QtCore import Slot, QObject
from shiboken2 import wrapInstance, getCppPointer

# import additional O3DE QtForPython Gem modules
import az_qt_helpers

# additional DCCsi imports that utilize PySide2
from DccScriptingInterface.azpy.shared.ui.samples import SampleUI
# -------------------------------------------------------------------------


# ---- dccsi app modules --------------------------------------------------
# Blender
from DccScriptingInterface.Tools.DCC.Blender.config import blender_config
import DccScriptingInterface.Tools.DCC.Blender.start as blender_start

# Wing
from DccScriptingInterface.Tools.IDE.Wing.config import wing_config
import DccScriptingInterface.Tools.IDE.Wing.start as wing_start

# Maya, not implemented yet
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def create_menu(parent: QMenu, title: str = 'StudioTools') -> QMenu:
    """! Creates a 'Studio Tools' menu for the DCCsi functionality
    :param parent: The parent QMenu (or QMenuBar)
    :param : The UI text str for the submenu
    :return: returns the created submenu
    """
    _LOGGER.debug(f"Creating a dccsi menu: '{title}'")

    dccsi_menu = None

    menu_list = parent.findChildren(QMenu)
    for m in menu_list:
        if m.title() == f"&{title}":
            dccsi_menu = m

    if not dccsi_menu:
        # create our own dccsi menu
        dccsi_menu = parent.addMenu(f"&{title}")

    return dccsi_menu
# -------------------------------------------------------------------------


# - slot ------------------------------------------------------------------
# as the list of slots/actions grows, refactor into sub-modules
@Slot()
def click_action_sampleui():
    """! Creates a sandalone sample ui with button, this is provided for
    Technicl Artists learning, as one of the purposes of the dccsi is
    onboarding TAs to the editor extensibility experience.

    :return: returns the created ui
    """
    _LOGGER.debug(f'Clicked: click_action_sampleui')

    ui = SampleUI(parent=az_qt_helpers.get_editor_main_window(),
                  title='Dccsi: SampleUI')
    ui.show()
    return
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def start_service(py_file: Path = None,
                  debug: bool = DCCSI_LOCAL_DEBUG) -> subprocess:
    """! Common method to start the external application start script

    :param : The UI text str for the submenu
    :return: returns the created submenu
    """
    _LOGGER.debug(f'Starting Service: {py_file.parent.name}')

    cmd = [str(_settings_core.DCCSI_PY_BASE), str(py_file)]

    # there are at least three ways to go about this ...
    # the first, is to call the function
    # this doesn't work well with O3DE, because the environ is propogated
    # and o3de python and Qt both interfer with wing boot and operation

#     try:
#         #wing_proc = wing_start.call()
#         wing_proc = wing_start.popen()
#     except Exception as e:
#         _LOGGER.error(f'{e} , traceback =', exc_info=True)
#         return None
#     # this sort of works, seems to stall the editor until wing closes.

    # the second, we could try to execute another way ...
    # probably the same results as above, it's also probably not safe
#     py_file = Path(wing_config.settings.PATH_DCCSI_TOOLS_IDE_WING, 'start.py').resolve()
#     try:
#         wing_proc = exec(open(f"{py_file.as_posix()}").read())
#     except Exception as e:
#         _LOGGER.error(f'{e} , traceback =', exc_info=True)
#         return None
    # tries to execute but fails to do so correctly

    if debug:
        # This approach will block the editor but can return a callstack
        # tThis is valuable to debug boot issues with start.py code itself
        # You must manually enable this flag near beginning of this module
        p = subprocess.Popen(cmd,
                             stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE)
        out, err = p.communicate()
        _LOGGER.debug(f'out: {str(out)}')
        _LOGGER.error(f'err: {str(err)}')
        _LOGGER.debug(f'returncode: {str(p.returncode)}')
        _LOGGER.debug('EXIT')

    else:
        # This is non-blocking, but if it fails can be difficult to debug
        p = subprocess.Popen(cmd)
        _LOGGER.debug('pid', p.pid)
        _LOGGER.debug('EXIT')

    return p
# -------------------------------------------------------------------------


# - slot ------------------------------------------------------------------
# as the list of slots/actions grows, refactor into sub-modules
@Slot()
def click_action_start_blender() -> start_service:
    """Start Blender DCC application"""
    _LOGGER.debug(f'Clicked: click_action_start_blender')
    py_file = Path(blender_config.settings.PATH_DCCSI_TOOLS_DCC_BLENDER, 'start.py').resolve()
    return start_service(py_file)
# -------------------------------------------------------------------------


# - slot ------------------------------------------------------------------
@Slot()
def click_action_start_wing() -> start_service:
    """Start Wing IDE"""
    _LOGGER.debug(f'Clicked: click_action_start_wing')
    py_file = Path(wing_config.settings.PATH_DCCSI_TOOLS_IDE_WING, 'start.py').resolve()
    return start_service(py_file)
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def add_action(parent: QMenu,
               title: str = "SampleUI",
               action_slot = click_action_sampleui) -> QAction:
    """! adds an action to the parent QMenu
    :param parent_menu: the parent Qmenu to add an action to
    :param title: The UI text str for the menu action
    :param action_slot: @Slot decorated method, see click_sampleui()
    :return: returns the created action
    """
    _LOGGER.debug(f"Creating '{title}' action for menu '{parent.title()}'")

    action = None

    action_list = parent.findChildren(QAction)
    for a in action_list:
        if a.text() == f"&{title}":
            action = a

    if not action:
        action = parent.addAction(f"&{title}")

        # click_sampleui signal
        action.triggered.connect(action_slot)

    return action
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def bootstrap_Editor():
    """! Put bootstrapping code here to execute in O3DE Editor.exe"""

    # Editor main window (should work with any standalone o3de editor exe)
    EDITOR_MAIN_WINDOW = az_qt_helpers.get_editor_main_window()

    menubar = EDITOR_MAIN_WINDOW.menuBar()

    dccsi_menu = create_menu(parent=menubar, title ='StudioTools')

    # Editor MenuBar, Studio Tools > DCC
    # nest a menu with hooks to start python DCC apps like Blender
    dccsi_dcc_menu = create_menu(parent=dccsi_menu, title="DCC")

    # Editor MenuBar, Studio Tools > DCC > Blender
    # Action manager should make it easier for each tool to
    # inject themselves, instead of this module manually
    # bootstrapping each. Suggestion for a follow up PR.
    action_start_blender = add_action(parent=dccsi_dcc_menu,
                                      title="Blender",
                                      action_slot = click_action_start_blender)

    # Editor MenuBar, Studio Tools > IDE
    # nest a menu with hooks to start python IDE tools like Wing
    dccsi_ide_menu = create_menu(parent=dccsi_menu, title="IDE")

    # MEditor MenuBar, Studio Tools > IDE > Wing
    action_start_wingide = add_action(parent=dccsi_ide_menu,
                                       title="Wing",
                                       action_slot = click_action_start_wing)

    # Editor MenuBar, Studio Tools > Examples
    # nest a menu with samples (promote extensibility)
    dccsi_examples_menu = create_menu(parent=dccsi_menu,
                                            title="Examples")

    # MEditor MenuBar, Studio Tools > Examples > SampleUI
    action_start_sampleui = add_action(parent=dccsi_examples_menu,
                                       title="SampleUI",
                                       action_slot = click_action_sampleui)

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
