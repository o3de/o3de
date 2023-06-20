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
from pathlib import Path
from os.path import expanduser
import logging as _logging
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# global scope
from DccScriptingInterface.azpy import _PACKAGENAME
_MODULENAME = f'{_PACKAGENAME}.constants'
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug(f'Initializing: {_MODULENAME}')
_MODULE_PATH = Path(__file__)  # what if frozen?
_LOGGER.debug(f'_MODULE_PATH: {_MODULE_PATH}')
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# Ideally this module is standalone with minimal dependancies
# Most logic, such as a search to derive a path, should happen outside of this module.
# To Do: place best defaults here and logical derivations in a config.py

# This sets up basic code access to the DCCsi
from DccScriptingInterface.azpy import PATH_DCCSIG  # root DCCsi path
# DCCsi imports
from DccScriptingInterface.azpy.env_bool import env_bool

from azpy.config_utils import return_stub_dir
from azpy.config_utils import get_stub_check_path
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# This is the first set of defined constants (and we use them here)
from DccScriptingInterface.constants import ENVAR_DCCSI_GDEBUG
from DccScriptingInterface.constants import ENVAR_DCCSI_DEV_MODE
from DccScriptingInterface.constants import ENVAR_DCCSI_GDEBUGGER
from DccScriptingInterface.constants import ENVAR_DCCSI_LOGLEVEL
from DccScriptingInterface.constants import ENVAR_DCCSI_TESTS

from DccScriptingInterface.globals import *
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# utility: constants, like pretty print strings
from DccScriptingInterface.constants import STR_CROSSBAR
from DccScriptingInterface.constants import STR_CROSSBAR_RL
from DccScriptingInterface.constants import STR_CROSSBAR_NL
# Log formating
from DccScriptingInterface.constants import FRMT_LOG_LONG
from DccScriptingInterface.constants import FRMT_LOG_SHRT
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# common: constants like os, sys utils
from DccScriptingInterface.constants import SLUG_DIR_REGISTRY

# string literals
BITDEPTH64 = str('64bit')

#  some common paths
from DccScriptingInterface.constants import PATH_PROGRAMFILES_X86
from DccScriptingInterface.constants import PATH_PROGRAMFILES_X64

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
from DccScriptingInterface.constants import DCCSI_SETTINGS_LOCAL_FILENAME

# py path string, parts, etc.
TAG_DEFAULT_PY = str('Launch_pyBASE.bat')

# python related
#python and site-dir
TAG_DCCSI_PY_VERSION_MAJOR = str(3)
TAG_DCCSI_PY_VERSION_MINOR = str(10)
TAG_DCCSI_PY_VERSION_RELEASE = str(5)
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
ENVAR_O3DE_BUILD_DIR_NAME = str('O3DE_BUILD_DIR_NAME')
ENVAR_PATH_O3DE_BUILD = str('PATH_O3DE_BUILD')
ENVAR_PATH_O3DE_BIN = str('PATH_O3DE_BIN')
ENVAR_PATH_O3DE_PYTHON_INSTALL = str('PATH_O3DE_PYTHON_INSTALL')

# MOVED to DccScriptingInterface __init__
ENVAR_O3DE_PROJECT = str('O3DE_PROJECT') # project name
ENVAR_PATH_O3DE_PROJECT = str('PATH_O3DE_PROJECT') # path to project
ENVAR_O3DE_DEV = str('O3DE_DEV')

# DCCSI
from DccScriptingInterface.constants import ENVAR_PATH_DCCSIG
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
from DccScriptingInterface.constants import ENVAR_PATH_DCCSI_PYTHON_LIB
ENVAR_DCCSI_PY_BASE = str('DCCSI_PY_BASE')
ENVAR_DCCSI_PY_DCCSI = str('DCCSI_PY_DCCSI')
ENVAR_DCCSI_PY_DEFAULT = str('DCCSI_PY_DEFAULT')

# Qt / PySide2
ENVAR_QT_PLUGIN_PATH = str('QT_PLUGIN_PATH')
ENVAR_QTFORPYTHON_PATH = str('QTFORPYTHON_PATH')

# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# dcc:(all) API constants

# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# material API constants
DCCSI_IMAGE_TYPES = ['.tif', '.tiff', '.png', '.jpg', '.jpeg', '.tga']


# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# To Do: these really should not be here
from DccScriptingInterface import O3DE_DEV
from DccScriptingInterface import PATH_DCCSIG
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
PATH_O3DE_BUILD = str(STR_CONSTRUCT_PATH_O3DE_BUILD.format(O3DE_DEV,
                                                            TAG_DIR_O3DE_BUILD_FOLDER))

# ENVAR_QT_PLUGIN_PATH = TAG_QT_PLUGIN_PATH
STR_QTPLUGIN_DIR = str('{0}\\bin\\profile\\EditorPlugins')
STR_QTFORPYTHON_PATH = str('{0}\\Gems\\QtForPython\\3rdParty\\pyside2\\windows\\release')
STR_PATH_O3DE_BIN = str('{0}\\bin\\profile')

STR_PATH_O3DE_BUILD = str('{0}\\{1}')
PATH_O3DE_BUILD = STR_PATH_O3DE_BUILD.format(O3DE_DEV, TAG_DIR_O3DE_BUILD_FOLDER)

PATH_QTFORPYTHON_PATH = str(STR_QTFORPYTHON_PATH.format(O3DE_DEV))
PATH_QT_PLUGIN_PATH = str(STR_QTPLUGIN_DIR).format(PATH_O3DE_BUILD)
PATH_O3DE_BIN = str(STR_PATH_O3DE_BIN).format(PATH_O3DE_BUILD)

STR_USER_O3DE_REGISTRY_PATH = str('{0}\\{1}')
PATH_USER_O3DE_REGISTRY = str(STR_USER_O3DE_REGISTRY_PATH).format(PATH_USER_O3DE, SLUG_DIR_REGISTRY)

STR_USER_O3DE_BOOTSTRAP_PATH = str('{reg}\\{file}')
PATH_USER_O3DE_BOOTSTRAP = str(STR_USER_O3DE_BOOTSTRAP_PATH).format(reg=PATH_USER_O3DE_REGISTRY,
                                                                    file=TAG_O3DE_BOOTSTRAP)

STR_CONSTRUCT_PATH_O3DE_PYTHON_INSTALL = str('{0}\\{1}\\{2}.{3}.{4}\\{5}')
PATH_DCCSI_PYTHON = str(STR_CONSTRUCT_PATH_O3DE_PYTHON_INSTALL.format(O3DE_DEV,
                                                                       TAG_TOOLS_DIR,
                                                                       TAG_DCCSI_PY_VERSION_MAJOR,
                                                                       TAG_DCCSI_PY_VERSION_MINOR,
                                                                       TAG_DCCSI_PY_VERSION_RELEASE,
                                                                       TAG_PLATFORM))
PATH_DCCSI_PY_BASE = str('{0}\\{1}').format(PATH_DCCSI_PYTHON, TAG_PYTHON_EXE)
PATH_DCCSI_PY_DEFAULT = PATH_DCCSI_PY_BASE

# bootstrap site-packages by version
from DccScriptingInterface import PATH_DCCSI_PYTHON_LIB


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
    DCCSI_GDEBUG = False
    DCCSI_DEV_MODE = False
    _DCCSI_LOGLEVEL = _logging.INFO
    DCCSI_GDEBUGGER = 'WING'

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
    _stash_dict['O3DE_DEV'] = Path(O3DE_DEV)
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
