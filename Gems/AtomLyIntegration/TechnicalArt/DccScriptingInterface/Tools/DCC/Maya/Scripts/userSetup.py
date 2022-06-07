# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -------------------------------------------------------------------------
from __future__ import unicode_literals

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
import os
import site
import inspect
import traceback
from pathlib import Path
import logging as _logging
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# global scope
_MODULENAME = 'Tools.DCC.Maya.scripts.userSetup'

__all__ = ['config',
           'constants',
           'setup',
           'start',
           'scripts']

_LOGGER = _logging.getLogger(_MODULENAME)
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# Maya is frozen
# module path when frozen
_MODULE_PATH = Path(os.path.abspath(inspect.getfile(inspect.currentframe())))
_LOGGER.debug('_MODULE_PATH: {}'.format(_MODULE_PATH))

_DCCSI_TOOLS_MAYA_SCRIPTS_PATH = Path(_MODULE_PATH.parent.as_posix())
_LOGGER.debug('_DCCSI_TOOLS_MAYA_SCRIPTS_PATH: {}'.format(_DCCSI_TOOLS_MAYA_SCRIPTS_PATH))

_PATH_DCCSI_TOOLS_MAYA = Path(_DCCSI_TOOLS_MAYA_SCRIPTS_PATH.parent)
_PATH_DCCSI_TOOLS_MAYA = Path(os.getenv('PATH_DCCSI_TOOLS_MAYA', _PATH_DCCSI_TOOLS_MAYA.as_posix()))
site.addsitedir(_PATH_DCCSI_TOOLS_MAYA.as_posix())

_PATH_DCCSI_TOOLS_DCC = Path(_PATH_DCCSI_TOOLS_MAYA.parent)
_PATH_DCCSI_TOOLS_DCC = Path(os.getenv('PATH_DCCSI_TOOLS_DCC', _PATH_DCCSI_TOOLS_DCC.as_posix()))
site.addsitedir(_PATH_DCCSI_TOOLS_DCC.as_posix())

_PATH_DCCSI_TOOLS = Path(_PATH_DCCSI_TOOLS_DCC.parent)
_PATH_DCCSI_TOOLS = Path(os.getenv('PATH_DCCSI_TOOLS', _PATH_DCCSI_TOOLS.as_posix()))

_PATH_DCCSIG = Path(_PATH_DCCSI_TOOLS.parent)
_PATH_DCCSIG = Path(os.getenv('PATH_DCCSIG', _PATH_DCCSIG.as_posix()))
site.addsitedir(_PATH_DCCSIG.as_posix())
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# now we have access to the DCCsi code and azpy
from azpy.env_bool import env_bool
from azpy.constants import *
#from azpy.constants import ENVAR_DCCSI_GDEBUG
#from azpy.constants import ENVAR_DCCSI_DEV_MODE
#from azpy.constants import ENVAR_DCCSI_LOGLEVEL
#from azpy.constants import ENVAR_DCCSI_GDEBUGGER
#from azpy.constants import FRMT_LOG_LONG

#  global space
_DCCSI_GDEBUG = env_bool(ENVAR_DCCSI_GDEBUG, False)
_DCCSI_DEV_MODE = env_bool(ENVAR_DCCSI_DEV_MODE, False)
_DCCSI_GDEBUGGER = env_bool(ENVAR_DCCSI_GDEBUGGER, 'WING')

# default loglevel to info unless set
_DCCSI_LOGLEVEL = int(env_bool(ENVAR_DCCSI_LOGLEVEL, _logging.INFO))
if _DCCSI_GDEBUG:
    # override loglevel if runnign debug
    _DCCSI_LOGLEVEL = _logging.DEBUG
    _logging.basicConfig(level=_DCCSI_LOGLEVEL,
                        format=FRMT_LOG_LONG,
                        datefmt='%m-%d %H:%M')
    _LOGGER = _logging.getLogger(_MODULENAME)
    
# early attach WingIDE debugger (can refactor to include other IDEs later)
if _DCCSI_DEV_MODE:
    from azpy.test.entry_test import connect_wing
    foo = connect_wing()
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# message collection
_LOGGER.debug(f'Initializing: {_MODULENAME}')
_LOGGER.debug(f'_MODULE_PATH: {_MODULE_PATH}')
_LOGGER.debug(f'PATH_DCCSIG: {_PATH_DCCSIG}')
_LOGGER.debug(f'PATH_DCCSI_TOOLS: {_PATH_DCCSI_TOOLS}')
_LOGGER.debug(f'PATH_DCCSI_TOOLS_DCC: {_PATH_DCCSI_TOOLS_DCC}')
_LOGGER.debug(f'PATH_DCCSI_TOOLS_MAYA: {_PATH_DCCSI_TOOLS_MAYA}')

# flag to turn off setting up callbacks, until they are fully implemented
# To Do: consider making it a settings option to define and enable/disable
_G_LOAD_CALLBACKS = True  # couple bugs, couple NOT IMPLEMENTED
_LOGGER.info('DCCSI_MAYA_SET_CALLBACKS: {0}.'.format({_G_LOAD_CALLBACKS}))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# To Do: needs to be updated to use dynaconf and config.py
from azpy.env_base import _BASE_ENVVAR_DICT

# -- maya imports
import maya.cmds as cmds
import maya.mel as mel
#from pymel.all import *
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
# validate access to the DCCsi and it's Lib site-packages
# bootstrap site-packages by version
from azpy.constants import PATH_DCCSI_PYTHON_LIB

try:
    os.path.exists(PATH_DCCSI_PYTHON_LIB)
    site.addsitedir(PATH_DCCSI_PYTHON_LIB)
    _LOGGER.info('azpy 3rdPary site-packages: is: {0}'.format(PATH_DCCSI_PYTHON_LIB))
except Exception as e:
    _LOGGER.error('ERROR: {0}, {1}'.format(e, PATH_DCCSI_PYTHON_LIB))
    raise e

# 3rdparty
from unipath import Path
from box import Box
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# Maya is frozen
# module path when frozen
_MODULE_PATH = os.path.abspath(inspect.getfile(inspect.currentframe()))
_MODULE_PATH = os.path.dirname(_MODULE_PATH)
if _BOOT_INFO:
    _LOGGER.debug('Boot: CWD: {}'.format(os.getcwd()))
    _LOGGER.debug('Frozen: _MODULE_FILEPATH: {}'.format(_MODULE_PATH))
    _LOGGER.debug('Frozen: _MODULE_PATH: {}'.format(_MODULE_PATH))
    _LOGGER.debug('Module __name__: {}'.format(__name__))
# root: INFO: Module __name__: __main__

_LOGGER.info('_MODULENAME: {}'.format(_MODULENAME))

# -------------------------------------------------------------------------
# check some env var tags (fail if no, likely means no proper code access)

_PATH_O3DE_PROJECT = None
try:
    _PATH_O3DE_PROJECT = _BASE_ENVVAR_DICT[ENVAR_PATH_O3DE_PROJECT]
except Exception as e:
    _LOGGER.critical(_STR_ERROR_ENVAR.format(_BASE_ENVVAR_DICT[ENVAR_PATH_O3DE_PROJECT]))
    
_O3DE_DEV = _BASE_ENVVAR_DICT[ENVAR_O3DE_DEV]
_O3DE_PATH_DCCSIG = _BASE_ENVVAR_DICT[ENVAR_PATH_DCCSIG]
_O3DE_DCCSI_LOG_PATH = _BASE_ENVVAR_DICT[ENVAR_DCCSI_LOG_PATH]
_O3DE_AZPY_PATH = _BASE_ENVVAR_DICT[ENVAR_DCCSI_AZPY_PATH]
# -------------------------------------------------------------------------


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

    # get known paths
    _KNOWN_PATHS = site._init_pathinfo()

    if os.path.isdir(_PATH_DCCSI_TOOLS.as_posix()):
        site.addsitedir(_PATH_DCCSI_TOOLS.as_posix(), _KNOWN_PATHS)
        try:
            import azpy.test
            _LOGGER.info('SUCCESS, import azpy.test')
        except Exception as e:
            _LOGGER.warning('startup(), could not import azpy.test')

    _LOGGER.info('startup(), COMPLETE')
    return 0
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# verify Shared\Python exists and add it as a site dir. Begin imports and config.
def post_startup():
    """Allows for a defered execution startup sequence (post UI boot)"""
    
    # note: at this point, in Maya 2022, the logging will show in script editor
    # to do: investigate if we want to figure out how to stream into output console also
    _LOGGER.info('{}.post_startup() fired'.format(_MODULENAME))

    # plugins, To Do: these should be moved to bootstrapping config
    try:
        maya.cmds.loadPlugin("dx11Shader")
    except Exception as e:
        _LOGGER.error(e)  # not a hard failure

    # Lumberyard DCCsi environment ready or error out.
    try:
        import azpy.dcc.maya
        from azpy.dcc.maya import _PACKAGENAME
        _LOGGER.info('Python module imported: {}'.format(_PACKAGENAME))
    except Exception as e:
        _LOGGER.error(e)
        _LOGGER.error(traceback.print_exc())
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
        from set_callbacks import _G_CALLBACKS
        # ^ need to hold on to this as the install repopulate set

    # this ensures the fixPaths callback is loaded
    # even when the other global callbacks are disabled
    from set_callbacks import install_fix_paths
    install_fix_paths()    

    # set the project workspace
    _project_workspace = os.path.join(_PATH_O3DE_PROJECT, TAG_MAYA_WORKSPACE)
    if os.path.isfile(_project_workspace):
        try:
            # load workspace
            maya.cmds.workspace(_PATH_O3DE_PROJECT, openWorkspace=True)
            _LOGGER.info('Loaded workspace file: {0}'.format(_project_workspace))
            maya.cmds.workspace(_PATH_O3DE_PROJECT, update=True)
        except Exception as e:
            _LOGGER.error(e)
    else:
        _LOGGER.warning('Workspace file not found: {1}'.format(_PATH_O3DE_PROJECT))

    # Set up Lumberyard, maya default setting
    from set_defaults import set_defaults
    set_defaults()

    # Setup UI tools
    if not maya.cmds.about(batch=True):
        _LOGGER.info('Add UI dependent tools')
        # wrap in a try, because we haven't implmented it yet
        try:
            mel.eval(str(r'source "{}"'.format(TAG_O3DE_DCC_MAYA_MEL)))
        except Exception as e:
            _LOGGER.error(e)

    # manage custom menu in a sub-module
    from set_menu import set_main_menu
    set_main_menu()
    
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
        post = executeDeferred(post_startup)

    except Exception as e:
        traceback.print_exc()
