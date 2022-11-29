#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -------------------------------------------------------------------------

"""
This module fullfils the maya bootstrap pattern as described in their docs
https://tinyurl.com/y2aoz8es

Pattern is similar to Lumberyard Editor\\Scripts\\bootstrap.py

For now the proper way to initiate Maya boostrapping the DCCsi, is to use
the provided env and launcher bat files.

If you are developing for the DCCsi you can use this launcher to start Maya:
DccScriptingInterface\\Launchers\\Windows\\Launch_Maya_2020.bat"

To Do: ATOM-5861
"""
__project__ = 'DccScriptingInterface'

# it is really hard to debug userSetup bootstrapping
# this enables some rudimentary logging for debugging
_BOOT_INFO = True

# -------------------------------------------------------------------------
# built in's
import sys
import os
import site
import inspect
from pathlib import Path
import logging as _logging

# -- maya imports
import maya.cmds as cmds
import maya.mel as mel
#from pymel.all import *

# Maya is frozen, entry point module path when frozen
_MODULE_PATH = Path(os.path.abspath(inspect.getfile(inspect.currentframe())))
print(f'o3de userSetup.py path is: {_MODULE_PATH}')

PATH_O3DE_TECHART_GEMS = _MODULE_PATH.parents[5].resolve()
print(f'o3de techart gems path is: {PATH_O3DE_TECHART_GEMS}')
sys.path.insert(0, str(PATH_O3DE_TECHART_GEMS))

from DccScriptingInterface import PATH_DCCSI_PYTHON_LIB
# 3rdparty
from unipath import Path
from box import Box
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# global scope
from DccScriptingInterface.Tools.DCC.Maya import _PACKAGENAME
_MODULENAME = f'{_PACKAGENAME}.scripts.userSetup'

__all__ = ['config',
           'constants',
           'setup',
           'start',
           'scripts']

_LOGGER = _logging.getLogger(_MODULENAME)

from DccScriptingInterface import add_site_dir
add_site_dir(PATH_O3DE_TECHART_GEMS) # cleaner add

from DccScriptingInterface.globals import *

from azpy.constants import FRMT_LOG_LONG
_logging.basicConfig(level=_logging.DEBUG,
                     format=FRMT_LOG_LONG,
                    datefmt='%m-%d %H:%M')

_LOGGER = _logging.getLogger(_MODULENAME)

# auto-attach ide debugging at the earliest possible point in module
if DCCSI_DEV_MODE:
    if DCCSI_GDEBUGGER == 'WING':
        import DccScriptingInterface.azpy.test.entry_test
        DccScriptingInterface.azpy.test.entry_test.connect_wing()
    elif DCCSI_GDEBUGGER == 'PYCHARM':
        _LOGGER.warning(f'{DCCSI_GDEBUGGER} debugger auto-attach not yet implemented')
    else:
        _LOGGER.warning(f'{DCCSI_GDEBUGGER} not a supported debugger')

# this should execute the core config.py first and grab settings
from DccScriptingInterface.Tools.DCC.Maya.config import maya_config
maya_config.settings.setenv() # init settings, ensure env is set

from DccScriptingInterface import ENVAR_PATH_DCCSIG
from DccScriptingInterface.azpy.constants import *
from DccScriptingInterface.constants import *
from DccScriptingInterface.Tools.DCC.Maya.constants import *

# message collection
_LOGGER.info(f'Initializing: {_MODULENAME}')
_LOGGER.debug(f'_MODULE_PATH: {_MODULE_PATH}')
_LOGGER.debug(f'{ENVAR_PATH_DCCSIG}: {maya_config.settings.PATH_DCCSIG}')
_LOGGER.debug(f'{ENVAR_DCCSI_CONFIG_DCC_MAYA}: {maya_config.settings.DCCSI_CONFIG_DCC_MAYA}')
_LOGGER.debug(f'{ENVAR_PATH_DCCSI_TOOLS_DCC_MAYA}: {maya_config.settings.PATH_DCCSI_TOOLS_DCC_MAYA}')

# flag to turn off setting up callbacks, until they are fully implemented
# To Do: consider making it a settings option to define and enable/disable
_G_LOAD_CALLBACKS = True  # couple bugs, couple NOT IMPLEMENTED
_LOGGER.info(f'DCCSI_MAYA_SET_CALLBACKS: {_G_LOAD_CALLBACKS}.')
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# To Do REMOVE this block and replace with dev module
# debug prints, To Do: this should be moved to bootstrap config
#_G_DEBUGGER = os.getenv(ENVAR_DCCSI_GDEBUGGER, "WING")

#if _DCCSI_DEV_MODE:
    #if _G_DEBUGGER == "WING":
        #_LOGGER.info('{0}'.format('-' * 74))
        #_LOGGER.info('Developer Debug Mode: {0}, Basic debugger: {1}'.format(_G_DEBUG, _G_DEBUGGER))
        #try:
            #_LOGGER.info('Attempting to start basic WING debugger')
            #import azpy.lmbr.test

            #_LOGGER.info('Package Imported: azpy.test')
            #ouput = azpy.entry_test.main(verbose=False,
                                         #connectDebugger=True,
                                         #returnOuput=_G_DEBUG)
            #_LOGGER.info(ouput)
            #pass
        #except Exception as e:
            #_LOGGER.info("Error: azpy.test, entry_test (didn't perform)")
            #_LOGGER.info("Exception: {0}".format(e))
        #pass
    #elif _G_DEBUGGER == "PYCHARM":
        ## https://github.com/juggernate/PyCharm-Maya-Debugging
        #_LOGGER.info('{0}'.format('-' * 74))
        #_LOGGER.info('Developer Debug Mode: {0}, Basic debugger: {1}'.format(_G_DEBUG, _G_DEBUGGER))
        #sys.path.append('C:\Program Files\JetBrains\PyCharm 2019.1.3\debug-eggs\pydevd-pycharm.egg')
        #try:
            #_LOGGER.info('Attempting to start basic PYCHARM debugger')
            ## Inside Maya Python Console (Tip: add to a shelf button for quick access)
            #import pydevd

            #_LOGGER.info('Package Imported: pydevd')
            #pydevd.settrace('localhost', port=7720, suspend=False)
            #_LOGGER.info('PYCHARM Debugger Attach Success!!!')
            ## To disconnect run:
            ## pydevd.stoptrace()
            #pass
        #except Exception as e:
            #_LOGGER.info("Error: pydevd.settrace (didn't perform)")
            #_LOGGER.info("Exception: {0}".format(e))
        #pass
    #else:
        #pass
## -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# To Do: implement data driven config
# Currently not used, but will be where we store the ordered dict
# which is parsed from the project bootstrapping config files.
_G_app_config = {}

# global scope maya callbacks container
_G_callbacks = Box(box_dots=True)  # global scope container

# used to store fixPaths in the global scope
_fix_paths = None
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# add appropriate common tools paths to the maya environment variables
def startup():
    """Early starup execution before mayautils.executeDeferred().
    Some things like UI and plugins should be defered to avoid failure"""
    _LOGGER.info('{}.startup() fired'.format(_MODULENAME))

    from DccScriptingInterface import PATH_DCCSIG
    from DccScriptingInterface.Tools import PATH_DCCSI_TOOLS

    try:
        import DccScriptingInterface.azpy.test
        _LOGGER.info('SUCCESS, import DccScriptingInterface.azpy.test')
    except Exception as e:
        _LOGGER.warning('startup(), could not import DccScriptingInterface.azpy.test')

    _LOGGER.info('startup(), COMPLETE')
    return 0
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# verify Shared\Python exists and add it as a site dir. Begin imports and config.
def post_startup():
    """Allows for a defered execution startup sequence (post UI boot)"""

    # NOTE: at this point, in Maya 2022+, the logging will show in script editor
    # not in the boot console window
    # to do: investigate if we want to figure out how to stream into output console also
    _LOGGER.info('{}.post_startup() fired'.format(_MODULENAME))

    # plugins, To Do: these should be moved to bootstrapping config
    try:
        maya.cmds.loadPlugin("dx11Shader")
    except Exception as e:
        _LOGGER.exception(f'{e} , traceback =', exc_info=True)

    # Lumberyard DCCsi environment ready or error out.
    try:
        import DccScriptingInterface.azpy.dcc.maya
        from DccScriptingInterface.azpy.dcc.maya import _PACKAGENAME
        _LOGGER.info('Python module imported: {}'.format(_PACKAGENAME))
    except Exception as e:
        _LOGGER.exception(f'{e} , traceback =', exc_info=True)
        return 1

    # DEPRECATE: don't nee to .init() anymore, auto-init (experimental)
    ## Dccsi azpy maya ready or error out.
    #try:
        #azpy.dcc.maya.init()
        #from azpy.dcc.maya import _PACKAGENAME
        #_LOGGER.info('SUCCESS, {}.init(), code accessible.'.format(_PACKAGENAME))
    #except Exception as e:
        #_LOGGER.error(e)
        #_LOGGER.error(traceback.print_exc())
        #return 1

    # callbacks, To Do: these should also be moved to the bootstrapping config
    # Defered startup after the Ui is running.
    _G_CALLBACKS = Box(box_dots=True)  # this just ensures a global scope container
    if _G_LOAD_CALLBACKS:
        try:
            import DccScriptingInterface.Tools.DCC.Maya.Scripts
            import DccScriptingInterface.Tools.DCC.Maya.Scripts.set_callbacks
            from DccScriptingInterface.Tools.DCC.Maya.Scripts.set_callbacks import _G_CALLBACKS
            # ^ need to hold on to this as the install repopulate set
        except ImportError as e:
            _LOGGER.exception(f'{e} , traceback =', exc_info=True)
            raise e

    # this ensures the fixPaths callback is loaded
    # even when the other global callbacks are disabled
    #from set_callbacks import install_fix_paths
    DccScriptingInterface.Tools.DCC.Maya.Scripts.set_callbacks.install_fix_paths()

    # set the project workspace
    _project_workspace = os.path.join(maya_config.settings.MAYA_PROJECT, SLUG_MAYA_WORKSPACE)
    if os.path.isfile(_project_workspace):
        try:
            # load workspace
            maya.cmds.workspace(maya_config.settings.MAYA_PROJECT, openWorkspace=True)
            _LOGGER.info('Loaded workspace file: {0}'.format(_project_workspace))
            maya.cmds.workspace(maya_config.settings.MAYA_PROJECT, update=True)
        except Exception as e:
            _LOGGER.exception(f'{e} , traceback =', exc_info=True)
    else:
        _LOGGER.warning('Workspace file not found: {1}'.format(maya_config.settings.MAYA_PROJECT))

    # Set up Lumberyard, maya default setting
    import DccScriptingInterface.Tools.DCC.Maya.Scripts.set_defaults
    DccScriptingInterface.Tools.DCC.Maya.Scripts.set_defaults.set_defaults()

    # Setup UI tools
    # if not maya.cmds.about(batch=True):
    #     _LOGGER.info('Add UI dependent tools')
    #     # wrap in a try, because we haven't implmented it yet
    #     try:
    #         mel.eval(str(r'source "{}"'.format(SLUG_O3DE_DCC_MAYA_MEL)))
    #     except Exception as e:
    #         _LOGGER.exception(f'{e} , traceback =', exc_info=True)
    #         pass

    # manage custom menu in a sub-module
    import DccScriptingInterface.Tools.DCC.Maya.Scripts.set_menu
    DccScriptingInterface.Tools.DCC.Maya.Scripts.set_menu.set_main_menu()

    # To Do: manage custom shelf in a sub-module

    _LOGGER.info('post_startup(), COMPLETE')
    _LOGGER.info('DCCsi Bootstrap, COMPLETE')
    return 0
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
if __name__ == '__main__':
    try:
        # Early startup config.
        startup()

        # This allows defered action post boot (atfer UI is active)
        from maya.utils import executeDeferred

        _LOGGER.info('attempting to run, post_startup()')

        post = executeDeferred(post_startup)

        _LOGGER.info('executing userSetup COMPLETE')

    except Exception as e:
        _LOGGER.exception(f'{e} , traceback =', exc_info=True)
        raise e
