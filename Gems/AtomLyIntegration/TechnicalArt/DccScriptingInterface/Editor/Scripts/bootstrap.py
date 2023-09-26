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
:Version: 0.0.2
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

# auto-attach ide debugging at the earliest possible point in module
from DccScriptingInterface.azpy.config_utils import attach_debugger
if DCCSI_DEV_MODE: # from DccScriptingInterface.globals
    attach_debugger()
#
import DccScriptingInterface.config as dccsi_core_config

# note: if you used win_launch_wingide.bat, settings will be over populated
# because of the .bat file env chain that includes active apps

# note: initializing the config PySide2 access here may not be ideal
# it will set envars that may propagate, and have have the side effect
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
import azlmbr.action

# put the Action Manager handler into a global scope so it's not deleted
handler_action_manager = azlmbr.action.ActionManagerRegistrationNotificationBusHandler()

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

# additional DCCsi imports that utilize PySide2
from DccScriptingInterface.Editor.Scripts.ui import start_service
from DccScriptingInterface.Editor.Scripts.ui import hook_register_action_sampleui

# SLUGS
START_SLUG = 'Start'
HELP_SLUG = 'Help ...'

# slug for editor action context
editor_mainwindow_context_slug = 'o3de.context.editor.mainwindow'

# slug is a string path.to.object.name
editor_mainwindow_menubar_slug = 'o3de.menubar.editor.mainwindow'

# actions are similar path.to.action.name
dccsi_action_context = 'o3de.action.python.dccsi'

# name to display for DCCsi Menu
dccsi_menu_name = 'studiotools'

# add to slug.path.to.object.name
dccsi_menu_slug = 'o3de.menu.studiotools'
# -------------------------------------------------------------------------


# - slot ------------------------------------------------------------------
# as the list of slots/actions grows, refactor into sub-modules
def click_action_blender_start() -> start_service:
    """Start Blender DCC application"""
    _LOGGER.debug(f'Clicked: click_action_blender_start')
    from DccScriptingInterface.Tools.DCC.Blender import PATH_DCCSI_TOOLS_DCC_BLENDER
    py_file = Path(PATH_DCCSI_TOOLS_DCC_BLENDER, 'start.py').resolve()
    return start_service(py_file)
# - hook ------------------------------------------------------------------
def hook_register_action_blender_start(parameters):
    # Create an Action Hook for starting Blender from menu
    _LOGGER.debug(f'Registered: hook_register_action_blender_start')
    action_properties = azlmbr.action.ActionProperties()
    action_properties.name = START_SLUG
    action_properties.description = "A menu item to call an action to Start Blender"
    action_properties.category = "Python"

    azlmbr.action.ActionManagerPythonRequestBus(azlmbr.bus.Broadcast,
                                                'RegisterAction',
                                                editor_mainwindow_context_slug,
                                                'o3de.action.python.dccsi.dcc.blender.start',
                                                action_properties,
                                                click_action_blender_start)
# -------------------------------------------------------------------------


# - slot ------------------------------------------------------------------
def click_action_blender_help():
    """Open Blender DCCsi docs (readme currently)"""
    _LOGGER.debug(f'Clicked: click_action_blender_help')

    url = "https://github.com/o3de/o3de/tree/development/Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface/Tools/DCC/Blender/readme.md"

    return QtGui.QDesktopServices.openUrl(QUrl(url, QUrl.TolerantMode))
# - hook ------------------------------------------------------------------
def hook_register_action_blender_help(parameters):
    _LOGGER.debug(f'Registered: hook_register_action_blender_help')
    action_properties = azlmbr.action.ActionProperties()
    action_properties.name = HELP_SLUG
    action_properties.description = "A menu item to open Dccsi Blender Readme (help docs)"
    action_properties.category = "Python"

    azlmbr.action.ActionManagerPythonRequestBus(azlmbr.bus.Broadcast,
                                                'RegisterAction',
                                                editor_mainwindow_context_slug,
                                                'o3de.action.python.dccsi.dcc.blender.help',
                                                action_properties,
                                                click_action_blender_help)
# -------------------------------------------------------------------------


# - slot ------------------------------------------------------------------
def click_action_maya_start() -> start_service:
    """Start Maya DCC application"""
    _LOGGER.debug(f'Clicked: click_action_maya_start')
    from DccScriptingInterface.Tools.DCC.Maya import PATH_DCCSI_TOOLS_DCC_MAYA
    py_file = Path(PATH_DCCSI_TOOLS_DCC_MAYA, 'start.py').resolve()
    return start_service(py_file)
# - hook ------------------------------------------------------------------
def hook_register_action_maya_start(parameters):
    # Create an Action Hook for starting Maya from menu
    _LOGGER.debug(f'Registered: hook_register_action_maya_start')
    action_properties = azlmbr.action.ActionProperties()
    action_properties.name = START_SLUG
    action_properties.description = "A menu item to call an action to Start Maya"
    action_properties.category = "Python"

    azlmbr.action.ActionManagerPythonRequestBus(azlmbr.bus.Broadcast,
                                                'RegisterAction',
                                                editor_mainwindow_context_slug,
                                                'o3de.action.python.dccsi.dcc.maya.start',
                                                action_properties,
                                                click_action_maya_start)
# -------------------------------------------------------------------------


# - slot ------------------------------------------------------------------
def click_action_maya_help():
    """Open Maya DCCsi docs (readme currently)"""
    _LOGGER.debug(f'Clicked: click_action_maya_help')

    url = "https://github.com/o3de/o3de/tree/development/Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface/Tools/DCC/Maya/readme.md"

    return QtGui.QDesktopServices.openUrl(QUrl(url, QUrl.TolerantMode))
# - hook ------------------------------------------------------------------
def hook_register_action_maya_help(parameters):
    _LOGGER.debug(f'Registered: hook_register_action_maya_help')
    action_properties = azlmbr.action.ActionProperties()
    action_properties.name = HELP_SLUG
    action_properties.description = "A menu item to open Dccsi Maya Readme (help docs)"
    action_properties.category = "Python"

    azlmbr.action.ActionManagerPythonRequestBus(azlmbr.bus.Broadcast,
                                                'RegisterAction',
                                                editor_mainwindow_context_slug,
                                                'o3de.action.python.dccsi.dcc.maya.help',
                                                action_properties,
                                                click_action_blender_help)
# -------------------------------------------------------------------------


# - slot ------------------------------------------------------------------
def click_action_wing_start() -> start_service:
    """Start Wing IDE"""
    _LOGGER.debug(f'Clicked: click_action_wing_start')
    from DccScriptingInterface.Tools.IDE.Wing import PATH_DCCSI_TOOLS_IDE_WING
    py_file = Path(PATH_DCCSI_TOOLS_IDE_WING, 'start.py').resolve()
    return start_service(py_file)
# - hook ------------------------------------------------------------------
def hook_register_action_wing_start(parameters):
    # Create an Action Hook for starting WingIDE from menu
    _LOGGER.debug(f'Registered: hook_register_action_wing_start')
    action_properties = azlmbr.action.ActionProperties()
    action_properties.name = START_SLUG
    action_properties.description = "A menu item to call an action to Start Wing IDE"
    action_properties.category = "Python"

    azlmbr.action.ActionManagerPythonRequestBus(azlmbr.bus.Broadcast,
                                                'RegisterAction',
                                                editor_mainwindow_context_slug,
                                                'o3de.action.python.dccsi.ide.wing.start',
                                                action_properties,
                                                click_action_wing_start)
# -------------------------------------------------------------------------


# - slot ------------------------------------------------------------------
def click_action_wing_help():
    """Open Wing IDE DCCsi docs (readme currently)"""
    _LOGGER.debug(f'Clicked: click_action_wing_help')

    url = "https://github.com/o3de/o3de/tree/development/Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface/Tools/IDE/Wing/readme.md"

    return QtGui.QDesktopServices.openUrl(QUrl(url, QUrl.TolerantMode))
# - hook ------------------------------------------------------------------
def hook_register_action_wing_help(parameters):
    _LOGGER.debug(f'Registered: hook_register_action_wing_help')
    action_properties = azlmbr.action.ActionProperties()
    action_properties.name = HELP_SLUG
    action_properties.description = "A menu item to open Dccsi Wing IDE Readme (help docs)"
    action_properties.category = "Python"

    azlmbr.action.ActionManagerPythonRequestBus(azlmbr.bus.Broadcast,
                                                'RegisterAction',
                                                editor_mainwindow_context_slug,
                                                'o3de.action.python.dccsi.ide.wing.help',
                                                action_properties,
                                                click_action_wing_help)
# -------------------------------------------------------------------------


# - slot ------------------------------------------------------------------
from DccScriptingInterface.Editor.Scripts.about import DccsiAbout

def click_action_dccsi_about():
    """Open DCCsi About Dialog"""
    _LOGGER.debug(f'Clicked: click_action_dccsi_about')

    # import additional O3DE QtForPython Gem modules
    import az_qt_helpers
    EDITOR_MAIN_WINDOW = az_qt_helpers.get_editor_main_window()

    about_dialog = DccsiAbout(EDITOR_MAIN_WINDOW)
    return about_dialog.exec_()
# - hook ------------------------------------------------------------------
def hook_register_action_dccsi_about(parameters):
    _LOGGER.debug(f'Registered: hook_register_action_dccsi_about')
    action_properties = azlmbr.action.ActionProperties()
    action_properties.name = 'About'
    action_properties.description = "Open DccScriptingInterface About Dialog"
    action_properties.category = "Python"

    azlmbr.action.ActionManagerPythonRequestBus(azlmbr.bus.Broadcast,
                                                'RegisterAction',
                                                editor_mainwindow_context_slug,
                                                'o3de.action.python.dccsi.about',
                                                action_properties,
                                                click_action_dccsi_about)
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def hook_on_menu_registration(parameters):

    _LOGGER.debug('DCCsi:bootstrap_Editor:hook_on_menu_registration')

    # Create a StudioTools Menu (o3de.menu.studiotools)
    menu_studiotools_properties = azlmbr.action.MenuProperties()
    menu_studiotools_properties.name = "Studio Tools"

    azlmbr.action.MenuManagerPythonRequestBus(azlmbr.bus.Broadcast,
                                              'RegisterMenu',
                                              'o3de.menu.studiotools',
                                              menu_studiotools_properties)

    # Studio Tools > DCC
    menu_studiotools_dcc_properties = azlmbr.action.MenuProperties()
    menu_studiotools_dcc_properties.name = "DCC"

    azlmbr.action.MenuManagerPythonRequestBus(azlmbr.bus.Broadcast,
                                              'RegisterMenu',
                                              'o3de.menu.studiotools.dcc',
                                              menu_studiotools_dcc_properties)

    # Studio Tools > DCC > Blender
    menu_studiotools_dcc_blender_properties = azlmbr.action.MenuProperties()
    menu_studiotools_dcc_blender_properties.name = "Blender"

    azlmbr.action.MenuManagerPythonRequestBus(azlmbr.bus.Broadcast,
                                              'RegisterMenu',
                                              'o3de.menu.studiotools.dcc.blender',
                                              menu_studiotools_dcc_blender_properties)

    # Studio Tools > DCC > Maya
    menu_studiotools_dcc_maya_properties = azlmbr.action.MenuProperties()
    menu_studiotools_dcc_maya_properties.name = "Maya"

    azlmbr.action.MenuManagerPythonRequestBus(azlmbr.bus.Broadcast,
                                              'RegisterMenu',
                                              'o3de.menu.studiotools.dcc.maya',
                                              menu_studiotools_dcc_maya_properties)

    # Studio Tools > IDE
    menu_studiotools_ide_properties = azlmbr.action.MenuProperties()
    menu_studiotools_ide_properties.name = "IDE"

    azlmbr.action.MenuManagerPythonRequestBus(azlmbr.bus.Broadcast,
                                              'RegisterMenu',
                                              'o3de.menu.studiotools.ide',
                                              menu_studiotools_ide_properties)

    # Studio Tools > IDE > Wing
    menu_studiotools_ide_wing_properties = azlmbr.action.MenuProperties()
    menu_studiotools_ide_wing_properties.name = "Wing"

    azlmbr.action.MenuManagerPythonRequestBus(azlmbr.bus.Broadcast,
                                              'RegisterMenu',
                                              'o3de.menu.studiotools.ide.wing',
                                              menu_studiotools_ide_wing_properties)

    # Studio Tools > examples
    menu_studiotools_examples_properties = azlmbr.action.MenuProperties()
    menu_studiotools_examples_properties.name = "Examples"

    azlmbr.action.MenuManagerPythonRequestBus(azlmbr.bus.Broadcast,
                                              'RegisterMenu',
                                              'o3de.menu.studiotools.examples',
                                              menu_studiotools_examples_properties)
# -------------------------------------------------------------------------



# -------------------------------------------------------------------------
def hook_on_menu_binding(parameters):

    _LOGGER.debug('DCCsi:bootstrap_Editor:hook_on_menu_binding')

    # Add Blender Actions to menu tree
    azlmbr.action.MenuManagerPythonRequestBus(azlmbr.bus.Broadcast,
                                              'AddActionToMenu',
                                              'o3de.menu.studiotools.dcc.blender',
                                              'o3de.action.python.dccsi.dcc.blender.start',
                                              100)

    azlmbr.action.MenuManagerPythonRequestBus(azlmbr.bus.Broadcast,
                                              'AddActionToMenu',
                                              'o3de.menu.studiotools.dcc.blender',
                                              'o3de.action.python.dccsi.dcc.blender.help',
                                              200)
    # Add Maya Actions to menu tree
    azlmbr.action.MenuManagerPythonRequestBus(azlmbr.bus.Broadcast,
                                              'AddActionToMenu',
                                              'o3de.menu.studiotools.dcc.maya',
                                              'o3de.action.python.dccsi.dcc.maya.start',
                                              100)

    azlmbr.action.MenuManagerPythonRequestBus(azlmbr.bus.Broadcast,
                                              'AddActionToMenu',
                                              'o3de.menu.studiotools.dcc.maya',
                                              'o3de.action.python.dccsi.dcc.maya.help',
                                              200)
    # Add Wing Actions to menu tree
    azlmbr.action.MenuManagerPythonRequestBus(azlmbr.bus.Broadcast,
                                              'AddActionToMenu',
                                              'o3de.menu.studiotools.ide.wing',
                                              'o3de.action.python.dccsi.ide.wing.start',
                                              100)

    azlmbr.action.MenuManagerPythonRequestBus(azlmbr.bus.Broadcast,
                                              'AddActionToMenu',
                                              'o3de.menu.studiotools.ide.wing',
                                              'o3de.action.python.dccsi.ide.wing.help',
                                              200)
    # Add examples SampleUI
    azlmbr.action.MenuManagerPythonRequestBus(azlmbr.bus.Broadcast,
                                              'AddActionToMenu',
                                              'o3de.menu.studiotools.examples',
                                              'o3de.action.python.dccsi.examples.sampleui',
                                              100)
    # Add dccsi About
    azlmbr.action.MenuManagerPythonRequestBus(azlmbr.bus.Broadcast,
                                              'AddActionToMenu',
                                              'o3de.menu.studiotools',
                                              'o3de.action.python.dccsi.about',
                                              100)

    # manage dccsi menu tree
    # Add StudioTools > DCC (subMenu)
    azlmbr.action.MenuManagerPythonRequestBus(azlmbr.bus.Broadcast,
                                              'AddSubMenuToMenu',
                                              'o3de.menu.studiotools',
                                              'o3de.menu.studiotools.dcc',
                                              100)

    # Add StudioTools > DCC > Blender (subMenu)
    azlmbr.action.MenuManagerPythonRequestBus(azlmbr.bus.Broadcast,
                                              'AddSubMenuToMenu',
                                              'o3de.menu.studiotools.dcc',
                                              'o3de.menu.studiotools.dcc.blender',
                                              100)

    # Add StudioTools > DCC > Maya (subMenu)
    azlmbr.action.MenuManagerPythonRequestBus(azlmbr.bus.Broadcast,
                                              'AddSubMenuToMenu',
                                              'o3de.menu.studiotools.dcc',
                                              'o3de.menu.studiotools.dcc.maya',
                                              200)

    # Add StudioTools > IDE (subMenu)
    azlmbr.action.MenuManagerPythonRequestBus(azlmbr.bus.Broadcast,
                                              'AddSubMenuToMenu',
                                              'o3de.menu.studiotools',
                                              'o3de.menu.studiotools.ide',
                                              200)

    # Add StudioTools > IDE > Wing (subMenu)
    azlmbr.action.MenuManagerPythonRequestBus(azlmbr.bus.Broadcast,
                                              'AddSubMenuToMenu',
                                              'o3de.menu.studiotools.ide',
                                              'o3de.menu.studiotools.ide.wing',
                                              100)

    # Add StudioTools > Examples (subMenu)
    azlmbr.action.MenuManagerPythonRequestBus(azlmbr.bus.Broadcast,
                                              'AddSubMenuToMenu',
                                              'o3de.menu.studiotools',
                                              'o3de.menu.studiotools.examples',
                                              300)

    # Add our 'Studio Tools' Menu to O3DE Editor Menu Bar
    azlmbr.action.MenuManagerPythonRequestBus(azlmbr.bus.Broadcast,
                                              'AddMenuToMenuBar',
                                              editor_mainwindow_menubar_slug,
                                              'o3de.menu.studiotools',
                                              1000)
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def hook_on_action_registration(parameters):
    """DCCsi Action Registration"""

    _LOGGER.debug('DCCsi:bootstrap_Editor:hook_on_action_registration')

    hook_register_action_blender_start(parameters)
    hook_register_action_blender_help(parameters)
    hook_register_action_maya_start(parameters)
    hook_register_action_maya_help(parameters)
    hook_register_action_wing_start(parameters)
    hook_register_action_wing_help(parameters)
    hook_register_action_sampleui(parameters)
    hook_register_action_dccsi_about(parameters)
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def bootstrap_Editor(handler_action_manager):
    """! Put bootstrapping code here to execute in O3DE Editor.exe"""

    _LOGGER.debug('DCCsi:bootstrap_Editor')
    handler_action_manager.connect()

    # dccsi actions
    handler_action_manager.add_callback('OnActionRegistrationHook', hook_on_action_registration)

    # dccsi StudioTools menu
    handler_action_manager.add_callback('OnMenuRegistrationHook', hook_on_menu_registration)
    handler_action_manager.add_callback('OnMenuBindingHook', hook_on_menu_binding)
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
def bootstrap_PassCanvas():
    """Put bootstrapping code here to execute in O3DE PassCanvas.exe"""
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
            _settings = bootstrap_Editor(handler_action_manager)

        elif O3DE_EDITOR.stem.lower() == "materialeditor":
            _settings = bootstrap_MaterialEditor()

        elif O3DE_EDITOR.stem.lower() == "materialcanvas":
            _settings = bootstrap_MaterialCanvas()

        elif O3DE_EDITOR.stem.lower() == "passcanvas":
            _settings = bootstrap_PassCanvas()

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
