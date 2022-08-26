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
import timeit
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

# add gems parent, dccsi lives under:
# < o3de >\Gems\AtomLyIntegration\TechnicalArt
PATH_O3DE_TECHART_GEMS = _MODULE_PATH.parents[3].resolve()
sys.path.append(PATH_O3DE_TECHART_GEMS.as_posix())
site.addsitedir(PATH_O3DE_TECHART_GEMS.as_posix())

# < o3de >\Gems\AtomLyIntegration\TechnicalArt\< dccsi >
PATH_DCCSIG = _MODULE_PATH.parents[2].resolve()
from DccScriptingInterface.azpy.constants import ENVAR_PATH_DCCSIG
os.environ[ENVAR_PATH_DCCSIG] = PATH_DCCSIG.as_posix()
# -------------------------------------------------------------------------


# ---- debug stuff --------------------------------------------------------
# local dccsi imports
# this accesses common global state, e.g. DCCSI_GDEBUG (is True or False)
from DccScriptingInterface.globals import *

# temproary force enable these during development
DCCSI_GDEBUG = True
DCCSI_DEV_MODE = True

# auto-attach ide debugging at the earliest possible point in module
from DccScriptingInterface.azpy.config_utils import attach_debugger
if DCCSI_DEV_MODE: # from DccScriptingInterface.globals
    attach_debugger(debugger_type=DCCSI_GDEBUGGER)

if DCCSI_GDEBUG: # provides some basic profiling to ensure dccsi speediness
    _START = timeit.default_timer() # start tracking

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
# O3DE imports
import azlmbr
import azlmbr.bus
import azlmbr.paths
# the DCCsi Gem expects QtForPython Gem is active
try:
    azlmbr.qt.QtForPythonRequestBus(azlmbr.bus.Broadcast, 'IsActive')
except Exception as e:
    _LOGGER.error(f'O3DE Qt / PySide2 not available, exception: {e}')
    raise e
#PySide2 imports
from PySide2 import QtWidgets
from PySide2 import QtGui
from PySide2.QtCore import Slot
from shiboken2 import wrapInstance, getCppPointer

# import additional O3DE QtForPython Gem modules
import az_qt_helpers

# additional DCCsi imports
from DccScriptingInterface.azpy.shared.ui.samples import SampleUI
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
O3DE_EDITOR = Path(sys.executable).resolve() # executible
_LOGGER.debug(f'The sys.executable is: {O3DE_EDITOR}')

# ditor main window (should work with any standalone o3de editor exe)
EDIOT_MAIN_WINDOW = az_qt_helpers.get_editor_main_window()

# base paths and config
O3DE_DEV = Path(azlmbr.paths.engroot).resolve()
PATH_O3DE_BIN = O3DE_EDITOR.parent
PATH_O3DE_PROJECT = Path(azlmbr.paths.projectroot).resolve()

# # to do: refactor config.py and implement ConfigClass
# from DccScriptingInterface.config import ConfigClass
# # build config for this module
# dccsi_config = ConfigClass(config_name='dccsi', auto_set=True)

# for now, use the legacy code
import DccScriptingInterface.config as core_config

dccsi_config = core_config.get_config_settings(engine_path=O3DE_DEV,
                                               engine_bin_path=PATH_O3DE_BIN,
                                               project_path=PATH_O3DE_PROJECT,
                                               enable_o3de_python=True,
                                               enable_o3de_pyside2=True,
                                               set_env=True)
# -------------------------------------------------------------------------


# - slot ------------------------------------------------------------------
@Slot()
def click_sampleui():
    _LOGGER.debug(f'Clicked click_action_sampleUI')

    ui = SampleUI(parent=EDIOT_MAIN_WINDOW, title='Dccsi: SampleUI')
    ui.show()
    return
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def bootstrap_Editor():
    """Put bootstrapping code here to execute in O3DE Editor.exe"""

    _MENU_SLUG = 'Studio Tools'
    _LOGGER.debug(f"Creating '{_MENU_SLUG}' menus for {_DCCSI_SLUG}")

    # create our own dccsi menuBar
    dccsi_menu = EDIOT_MAIN_WINDOW.menuBar().addMenu(f"&{_MENU_SLUG}")

    # Editor MenuBar, Studio Tools > Examples
    # nest a menu with samples (promote extensibility)
    dccsi_examples_menu = dccsi_menu.addMenu("Examples")

    # MEditor MenuBar, Studio Tools > Examples > SampleUI
    action_start_sampleui = dccsi_examples_menu.addAction("SampleUI")
    # click_sampleui signal
    action_start_sampleui.triggered.connect(click_sampleui)

    return None
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def bootstrap_MaterialEditor():
    """Put bootstrapping code here to execute in O3DE MaterialEdito.exe"""
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

    if O3DE_EDITOR.stem.lower() == "editor":
        # if _DCCSI_GDEBUG then run the pyside2 test
        _settings = bootstrap_Editor()

    elif O3DE_EDITOR.stem.lower() == "materialeditor":
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

    if DCCSI_GDEBUG:
        _LOGGER.debug('{0} took: {1} sec'.format(_MODULENAME, timeit.default_timer() - _START))
    # -------------------------------------------------------------------------
