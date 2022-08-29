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

"""!
Module Documentation:
    DccScriptingInterface:: azpy//constants.py

This module is mainly a bunch of commony used constants, and default strings
So we can make an update here once that is used elsewhere.

< To Do: Further document module here >
"""
# -------------------------------------------------------------------------
import timeit
_MODULE_START = timeit.default_timer()  # start tracking

# standard imports
import os
import sys
import site
from os.path import expanduser
import logging as _logging
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# global scope
_MODULENAME = 'azpy.constants'
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug(f'Initializing: {_MODULENAME}')
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# This sets up basic code access to the DCCsi
# <o3de>/Gems/AtomLyIntegration/TechnicalArt/<DCCsi>
_MODULE_PATH = os.path.realpath(__file__)  # To Do: what if frozen?
_PATH_DCCSIG = os.path.normpath(os.path.join(_MODULE_PATH, '../..'))
_PATH_DCCSIG = os.getenv('PATH_DCCSIG', _PATH_DCCSIG)
site.addsitedir(_PATH_DCCSIG)

# Ideally this module is standalone with minimal dependancies
# Most logic, such as a search to derive a path, should happen outside of this module.
# To Do: place best defaults here and logical derivations in a config.py

# now we have azpy api access
import azpy
from azpy.env_bool import env_bool
from azpy.config_utils import return_stub_dir
from azpy.config_utils import get_stub_check_path
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# This is the first set of defined constants (and we use them here)
# to do: remove str(), those were added to improve py2/3 unicode
ENVAR_DCCSI_GDEBUG = str('DCCSI_GDEBUG')
ENVAR_DCCSI_DEV_MODE = str('DCCSI_DEV_MODE')
ENVAR_DCCSI_GDEBUGGER = str('DCCSI_GDEBUGGER')
ENVAR_DCCSI_LOGLEVEL = str('DCCSI_LOGLEVEL')
ENVAR_DCCSI_TESTS = str('DCCSI_TESTS')

# defaults, can be overriden/forced here for development
_DCCSI_GDEBUG = env_bool(ENVAR_DCCSI_GDEBUG, False)
_DCCSI_DEV_MODE = env_bool(ENVAR_DCCSI_DEV_MODE, False)
_DCCSI_LOGLEVEL = env_bool(ENVAR_DCCSI_LOGLEVEL, _logging.INFO)
_DCCSI_GDEBUGGER = env_bool(ENVAR_DCCSI_GDEBUGGER, 'WING')
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# utility: constants, like pretty print strings
STR_CROSSBAR = str('{0}'.format('-' * 74))
STR_CROSSBAR_RL = str('{0}\r'.format(STR_CROSSBAR))
STR_CROSSBAR_NL = str('{0}\n'.format(STR_CROSSBAR))
# Log formating
FRMT_LOG_LONG = "[%(name)s][%(levelname)s] >> %(message)s (%(asctime)s; %(filename)s:%(lineno)d)"
FRMT_LOG_SHRT = "[%(asctime)s][%(name)s][%(levelname)s] >> %(message)s"
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# common: constants like os, sys utils
TAG_DIR_REGISTRY = str('Registry')

# string literals
BITDEPTH64 = str('64bit')

#  some common paths
PATH_PROGRAMFILES_X86 = str(os.environ['PROGRAMFILES(X86)'])
PATH_PROGRAMFILES_X64 = str(os.environ['PROGRAMFILES'])

# os.path.expanduser("~") returns different values in py2.7 vs 3
PATH_USER_HOME = expanduser("~")
_LOGGER.debug(f'user home: {PATH_USER_HOME}')

# special case, make sure didn't return <user>\documents
_uh_parts = os.path.split(PATH_USER_HOME)

if str(_uh_parts[1].lower()) == 'documents':
    PATH_USER_HOME = _uh_parts[0]
    _LOGGER.debug(f'user home CORRECTED: {PATH_USER_HOME}')

STR_USER_O3DE_PATH = str('{home}\\{o3de}')
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# O3DE: common
# some common str tags
TAG_DEFAULT_COMPANY = str('Amazon.O3DE')
TAG_DEFAULT_PROJECT = str('DccScriptingInterface')
TAG_DCCSI_NICKNAME = str('DCCsi')
TAG_MOCK_PROJECT = str('MockProject')
TAG_DIR_O3DE_DEV = str('dev')
TAG_DIR_DCCSI_AZPY = str('azpy')
TAG_DIR_DCCSI_TOOLS = str('Tools')
TAG_DIR_O3DE_BUILD_FOLDER = str('build')
TAG_O3DE_FOLDER = str('.o3de')
TAG_O3DE_BOOTSTRAP = str('bootstrap.setreg')
TAG_DCCSI_CONFIG = str('dccsi_configuration.setreg')
TAG_DCCSI_LOCAL_SETTINGS_SLUG = str('settings.local.json')

# py path string, parts, etc.
TAG_DEFAULT_PY = str('Launch_pyBASE.bat')

# python related
#python and site-dir
TAG_DCCSI_PY_VERSION_MAJOR = str(3)
TAG_DCCSI_PY_VERSION_MINOR = str(7)
TAG_DCCSI_PY_VERSION_RELEASE = str(10)
TAG_PYTHON_EXE = str('python.exe')
TAG_TOOLS_DIR = str('Tools\\Python')
TAG_PLATFORM = str('windows')
TAG_PY_MAJOR = str(sys.version_info.major)  # future proof
TAG_PY_MINOR = str(sys.version_info.minor)

# config file stuff
FILENAME_DEFAULT_CONFIG = str('settings.json')

# filesystem markers, stub file names.
STUB_O3DE_DEV = str('engine.json')
STUB_O3DE_BUILD = str('CMakeCache.txt')
STUB_O3DE_ROOT_DCCSI = str('dccsi_stub')
STUB_O3DE_DCCSI_AZPY = str('dccsi_azpy_stub')
STUB_O3DE_DCCSI_TOOLS = str('dccsi_tools_stub')
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# JSON: constants used for parsing our dccsi json data
# config string consts, Meta Qualifiers
QUALIFIER_COMMENT = str('_meta_COMMENT')
QUALIFIER_TEMPLATE = str('_meta_{0}_TEMPLATE')

# config string consts, Section Qualifiers
QUALIFIER_EXAMPLE_SECTION = str('_Example_Block_')
QUALIFIER_INFO_SECTION = str('Info_Block')
QUALIFIER_GLOBAL_SECTION = str('Global_Block')
QUALIFIER_DEFAULT_ENV_SECTION = str('Default_Env_Block')
QUALIFIER_SERVICES_SECTION = str('Services_Block')

# config string consts, value, root and flags
QUALIFIER_VALUE = str('value')
QUALIFIER_ROOTPATH = str('rootPath')
QUALIFIER_ENVROOT = str('envRoot')
QUALIFIER_FLAGS = str('flags')

# config action flags
# the presence of a flag descriptor following a value block specifies a use case
FLAG_PATH_ADDPYTHONSITEDIR = str('addPySiteDir')
FLAG_PATH_ADDSYSPATH = str('addSysPath')
FLAG_PATH_SETSYSPATH = str('setSysPath')
FLAG_VAR_ADDENV = str('addEnv')
FLAG_VAR_SETENV = str('setEnv')
FLAG_PATH_ABSOLUTE = str('absolute')
FLAG_PATH_RELATIVE = str('relative')
FLAG_PROJECT_RELATIVE = str('project_relative')
FLAG_PATH_ROOT = str('root')
FLAG_PATH_BLOCKROOT = str('blockRoot')
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# ENV: constants, ENVARs, configuration and settings
# O3DE
# base env var key as str
ENVAR_COMPANY = str('COMPANY')
ENVAR_O3DE_PROJECT = str('O3DE_PROJECT') # project name
ENVAR_PATH_O3DE_PROJECT = str('PATH_O3DE_PROJECT') # path to project
ENVAR_O3DE_DEV = str('O3DE_DEV')
ENVAR_O3DE_BUILD_DIR_NAME = str('O3DE_BUILD_DIR_NAME')
ENVAR_PATH_O3DE_BUILD = str('PATH_O3DE_BUILD')
ENVAR_PATH_O3DE_BIN = str('PATH_O3DE_BIN')
ENVAR_PATH_O3DE_PYTHON_INSTALL = str('PATH_O3DE_PYTHON_INSTALL')

# DCCSI
ENVAR_PATH_DCCSIG = str('PATH_DCCSIG')
ENVAR_DCCSI_AZPY_PATH = str('DCCSI_AZPY_PATH')
ENVAR_PATH_DCCSI_TOOLS = str('PATH_DCCSI_TOOLS')
ENVAR_DCCSI_LOG_PATH = str('DCCSI_LOG_PATH')
ENVAR_DCCSI_LAUNCHERS_PATH = str('DCCSI_LAUNCHERS_PATH')

ENVAR_DCCSI_SYS_PATH = str('DCCSI_SYS_PATH')
ENVAR_DCCSI_PYTHONPATH = str('DCCSI_PYTHONPATH')

# DCCsi Python related
ENVAR_DCCSI_PY_VERSION_MAJOR = str('DCCSI_PY_VERSION_MAJOR')
ENVAR_DCCSI_PY_VERSION_MINOR = str('DCCSI_PY_VERSION_MINOR')
ENVAR_PATH_DCCSI_PYTHON = str('PATH_DCCSI_PYTHON')
ENVAR_PATH_DCCSI_PYTHON_LIB = str('PATH_DCCSI_PYTHON_LIB')
ENVAR_DCCSI_PY_BASE = str('DCCSI_PY_BASE')
ENVAR_DCCSI_PY_DCCSI = str('DCCSI_PY_DCCSI')
ENVAR_DCCSI_PY_DEFAULT = str('DCCSI_PY_DEFAULT')

# Qt / PySide2
ENVAR_QT_PLUGIN_PATH = str('QT_PLUGIN_PATH')
ENVAR_QTFORPYTHON_PATH = str('QTFORPYTHON_PATH')

# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# IDE: constants, like wing ENVARS
TAG_DEFAULT_WING_MAJOR_VER = str(7)
TAG_DEFAULT_WING_MINOR_VER = str(2)  # I had to bump so I could locally debug
TAG_WING_IDE = str('Wing IDE ')  # old, pre 7
TAG_WING_PRO = str('Wing Pro ')  # new 7+

ENVAR_WINGHOME = str('WINGHOME')
ENVAR_DCCSI_WING_VERSION_MAJOR = str('DCCSI_WING_VERSION_MAJOR')
ENVAR_DCCSI_WING_VERSION_MINOR = str('DCCSI_WING_VERSION_MINOR')

# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# dcc:(all) API constants

# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# To do: deprecate this block and redcode it, it has moved to:
# DccScriptingInterface\Tools\DCC\Maya\constants.py
# dcc: Maya ENVAR constants
TAG_O3DE_DCC_MAYA_MEL = 'dccsi_setup.mel'
TAG_MAYA_WORKSPACE = 'workspace.mel'

ENVAR_DCCSI_PY_MAYA = str('DCCSI_PY_MAYA')

ENVAR_MAYA_VERSION = str('MAYA_VERSION')
ENVAR_MAYA_LOCATION = str('MAYA_LOCATION')

ENVAR_PATH_DCCSI_TOOLS_MAYA = str('PATH_DCCSI_TOOLS_MAYA')
ENVAR_MAYA_MODULE_PATH = str('MAYA_MODULE_PATH')
ENVAR_MAYA_BIN_PATH = str('MAYA_BIN_PATH')

ENVAR_DCCSI_MAYA_PLUG_IN_PATH = str('DCCSI_MAYA_PLUG_IN_PATH')
ENVAR_MAYA_PLUG_IN_PATH = str('MAYA_PLUG_IN_PATH')

ENVAR_DCCSI_MAYA_SHELF_PATH = str('DCCSI_MAYA_SHELF_PATH')
ENVAR_MAYA_SHELF_PATH = str('MAYA_SHELF_PATH')

ENVAR_DCCSI_MAYA_XBMLANGPATH = str('DCCSI_MAYA_XBMLANGPATH')
ENVAR_XBMLANGPATH = str('XBMLANGPATH')

ENVAR_DCCSI_MAYA_SCRIPT_MEL_PATH = str('DCCSI_MAYA_SCRIPT_MEL_PATH')
ENVAR_DCCSI_MAYA_SCRIPT_PY_PATH = str('DCCSI_MAYA_SCRIPT_PY_PATH')
ENVAR_MAYA_SCRIPT_PATH = str('MAYA_SCRIPT_PATH')

ENVAR_DCCSI_MAYA_SET_CALLBACKS = str('DCCSI_MAYA_SET_CALLBACKS')
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# dcc:Blender API constants

# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# material API constants
DCCSI_IMAGE_TYPES = ['.tif', '.tiff', '.png', '.jpg', '.jpeg', '.tga']


# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# To Do: these should not be here
# Let's avoid the need for constants.py to call other modules!
PATH_O3DE_DEV = str(return_stub_dir(STUB_O3DE_DEV))
PATH_DCCSIG = str(return_stub_dir(STUB_O3DE_ROOT_DCCSI))
PATH_DCCSI_AZPY_PATH = str(return_stub_dir(STUB_O3DE_DCCSI_AZPY))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# Path constructors and paths
PATH_USER_O3DE = str(STR_USER_O3DE_PATH).format(home=PATH_USER_HOME,
                                              o3de=TAG_O3DE_FOLDER)

PATH_DCCSI_TOOLS = str('{0}\\{1}'.format(PATH_DCCSIG, TAG_DIR_DCCSI_TOOLS))

# logging into the user home o3de cache (temporarily)
PATH_DCCSI_LOG_PATH = str('{PATH_O3DE_PROJECT}\\user\\log\{TAG_DCCSI_NICKNAME}')

# dev \ <build> \
STR_CONSTRUCT_PATH_O3DE_BUILD = str('{0}\\{1}')
PATH_O3DE_BUILD = str(STR_CONSTRUCT_PATH_O3DE_BUILD.format(PATH_O3DE_DEV,
                                                            TAG_DIR_O3DE_BUILD_FOLDER))

# ENVAR_QT_PLUGIN_PATH = TAG_QT_PLUGIN_PATH
STR_QTPLUGIN_DIR = str('{0}\\bin\\profile\\EditorPlugins')
STR_QTFORPYTHON_PATH = str('{0}\\Gems\\QtForPython\\3rdParty\\pyside2\\windows\\release')
STR_PATH_O3DE_BIN = str('{0}\\bin\\profile')

STR_PATH_O3DE_BUILD = str('{0}\\{1}')
PATH_O3DE_BUILD = STR_PATH_O3DE_BUILD.format(PATH_O3DE_DEV, TAG_DIR_O3DE_BUILD_FOLDER)

PATH_QTFORPYTHON_PATH = str(STR_QTFORPYTHON_PATH.format(PATH_O3DE_DEV))
PATH_QT_PLUGIN_PATH = str(STR_QTPLUGIN_DIR).format(PATH_O3DE_BUILD)
PATH_O3DE_BIN = str(STR_PATH_O3DE_BIN).format(PATH_O3DE_BUILD)

STR_USER_O3DE_REGISTRY_PATH = str('{0}\\{1}')
PATH_USER_O3DE_REGISTRY = str(STR_USER_O3DE_REGISTRY_PATH).format(PATH_USER_O3DE, TAG_DIR_REGISTRY)

STR_USER_O3DE_BOOTSTRAP_PATH = str('{reg}\\{file}')
PATH_USER_O3DE_BOOTSTRAP = str(STR_USER_O3DE_BOOTSTRAP_PATH).format(reg=PATH_USER_O3DE_REGISTRY,
                                                                    file=TAG_O3DE_BOOTSTRAP)

STR_CONSTRUCT_PATH_O3DE_PYTHON_INSTALL = str('{0}\\{1}\\{2}.{3}.{4}\\{5}')
PATH_DCCSI_PYTHON = str(STR_CONSTRUCT_PATH_O3DE_PYTHON_INSTALL.format(PATH_O3DE_DEV,
                                                                       TAG_TOOLS_DIR,
                                                                       TAG_DCCSI_PY_VERSION_MAJOR,
                                                                       TAG_DCCSI_PY_VERSION_MINOR,
                                                                       TAG_DCCSI_PY_VERSION_RELEASE,
                                                                       TAG_PLATFORM))
PATH_DCCSI_PY_BASE = str('{0}\\{1}').format(PATH_DCCSI_PYTHON, TAG_PYTHON_EXE)
PATH_DCCSI_PY_DEFAULT = PATH_DCCSI_PY_BASE

# bootstrap site-packages by version
STR_PATH_DCCSI_PYTHON_LIB = str('{0}\\3rdParty\\Python\\Lib\\{1}.x\\{1}.{2}.x\\site-packages')
PATH_DCCSI_PYTHON_LIB = STR_PATH_DCCSI_PYTHON_LIB.format(PATH_DCCSIG,
                                                              TAG_PY_MAJOR,
                                                              TAG_PY_MINOR)


# wing paths
STR_CONSTRUCT_WING_PATH = str(f'{PATH_PROGRAMFILES_X86}\\{TAG_WING_PRO} {TAG_DEFAULT_WING_MAJOR_VER}.{TAG_DEFAULT_WING_MINOR_VER}')
PATH_DEFAULT_WINGHOME = str('{0}\\{1}{2}.{3}'
                            ''.format(PATH_PROGRAMFILES_X86,
                                      TAG_WING_PRO,
                                      TAG_DEFAULT_WING_MAJOR_VER,
                                      TAG_DEFAULT_WING_MINOR_VER))

PATH_SAT_INSTALL_PATH = str('{0}\\{1}\\{2}\\{3}\\{4}'
                            ''.format(PATH_PROGRAMFILES_X64,
                                    'Allegorithmic',
                                    'Substance Automation Toolkit',
                                    'Python API',
                                    'install'))
# -------------------------------------------------------------------------


###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """Run this file as a standalone script"""

    # turn all of these off/on for testing
    _DCCSI_GDEBUG = False
    _DCCSI_DEV_MODE = False
    _DCCSI_LOGLEVEL = _logging.INFO
    _DCCSI_GDEBUGGER = 'WING'

    # configure basic logger
    from azpy.constants import FRMT_LOG_LONG
    _logging.basicConfig(level=_DCCSI_LOGLEVEL,
                         format=FRMT_LOG_LONG,
                         datefmt='%m-%d %H:%M')

    _LOGGER = _logging.getLogger(_MODULENAME)

    _LOGGER.info(STR_CROSSBAR)
    _LOGGER.info(f'~ {_MODULENAME} ... Running script as __main__')
    _LOGGER.info(STR_CROSSBAR)

    #  this is just a debug developer convenience print (for testing acess)
    import pkgutil
    _LOGGER.info(f'Current working dir: {os.getcwd()}')
    search_path = ['.']  # set to None to see all modules importable from sys.path
    all_modules = [x[1] for x in pkgutil.iter_modules(path=search_path)]
    _LOGGER.info(f'All Available Modules in working dir: {all_modules}')

    #  test anything procedurally generated
    _LOGGER.info('Testing procedural env paths ...')
    from pathlib import Path

    _stash_dict = {}
    _stash_dict['O3DE_DEV'] = Path(PATH_O3DE_DEV)
    _stash_dict['PATH_DCCSIG'] = Path(PATH_DCCSIG)
    _stash_dict['DCCSI_AZPY_PATH'] = Path(PATH_DCCSI_AZPY_PATH)
    _stash_dict['PATH_DCCSI_TOOLS'] = Path(PATH_DCCSI_TOOLS)
    _stash_dict['PATH_DCCSI_PYTHON'] = Path(PATH_DCCSI_PYTHON)
    _stash_dict['DCCSI_PY_BASE'] = Path(PATH_DCCSI_PY_BASE)
    _stash_dict['PATH_DCCSI_PYTHON_LIB'] = Path(PATH_DCCSI_PYTHON_LIB)
    _stash_dict['PATH_O3DE_BUILD'] = Path(PATH_O3DE_BUILD)
    _stash_dict['PATH_O3DE_BIN'] = Path(PATH_O3DE_BIN)
    _stash_dict['QTFORPYTHON_PATH'] = Path(PATH_QTFORPYTHON_PATH)
    _stash_dict['QT_PLUGIN_PATH'] = Path(PATH_QT_PLUGIN_PATH)
    _stash_dict['SAT_INSTALL_PATH'] = Path(PATH_SAT_INSTALL_PATH)
    _stash_dict['PATH_USER_O3DE_BOOTSTRAP'] = Path(PATH_USER_O3DE_BOOTSTRAP)

    # ---------------------------------------------------------------------
    # py 2 and 3 compatible iter
    def get_items(dict_object):
        for key in dict_object:
            yield key, dict_object[key]

    for key, value in get_items(_stash_dict):
        # check if path exists
        try:
            value.exists()
            _LOGGER.info(F'{key}: {value}')
        except Exception as e:
            _LOGGER.warning(f'FAILED PATH: {e}')

    # custom prompt
    sys.ps1 = f"[{_MODULENAME}]>>"

_LOGGER.debug(f'{_MODULENAME} took: {(timeit.default_timer() - _MODULE_START)} sec')
# --- END -----------------------------------------------------------------
