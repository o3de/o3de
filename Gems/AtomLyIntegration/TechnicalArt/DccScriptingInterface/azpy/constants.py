# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -- This line is 75 characters -------------------------------------------

"""
Module Documentation:
    DccScriptingInterface:: azpy//constants.py

This module is mainly a bunch of commony used constants, and default strings
So we can make an update here once that is used elsewhere.

< To Do: Further document module here>
"""
# -------------------------------------------------------------------------
#  built-ins
import os
import sys
import site
from os.path import expanduser
import logging as _logging

# for this module to perform standalone
# we need to set up basic access to the DCCsi
_MODULE_PATH = os.path.realpath(__file__)  # To Do: what if frozen?
_DCCSIG_PATH = os.path.normpath(os.path.join(_MODULE_PATH, '../..'))
_DCCSIG_PATH = os.getenv('DCCSIG_PATH', _DCCSIG_PATH)
site.addsitedir(_DCCSIG_PATH)

#  azpy module
#import azpy.constants as cnst
from azpy.config_utils import return_stub_dir
import azpy.env_bool as env_bool
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# This is the first set of defined constants (and we use them here)
ENVAR_DCCSI_GDEBUG = str('DCCSI_GDEBUG')
ENVAR_DCCSI_DEV_MODE = str('DCCSI_DEV_MODE')
ENVAR_DCCSI_GDEBUGGER = str('DCCSI_GDEBUGGER')
ENVAR_DCCSI_LOGLEVEL = str('DCCSI_LOGLEVEL')
# Log formating
FRMT_LOG_LONG = "[%(name)s][%(levelname)s] >> %(message)s (%(asctime)s; %(filename)s:%(lineno)d)"
FRMT_LOG_SHRT = "[%(asctime)s][%(name)s][%(levelname)s] >> %(message)s"

#  global space
_G_DEBUG = env_bool.env_bool(ENVAR_DCCSI_GDEBUG, False)
_DCCSI_DEV_MODE = env_bool.env_bool(ENVAR_DCCSI_DEV_MODE, False)

for handler in _logging.root.handlers[:]:
    _logging.root.removeHandler(handler)

_PACKAGENAME = 'azpy.constants'

_LOG_LEVEL = int(20)
if _G_DEBUG:
    _LOG_LEVEL = int(10)
_logging.basicConfig(level=_LOG_LEVEL,
                     format=FRMT_LOG_LONG,
                     datefmt='%m-%d %H:%M')
_LOGGER = _logging.getLogger(_PACKAGENAME)
_LOGGER.debug('Initializing: {0}.'.format({_PACKAGENAME}))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# String Literals
BITDEPTH64 = str('64bit')

# string literals
STR_CROSSBAR = str('{0}'.format('-' * 74))
STR_CROSSBAR_RL = str('{0}\r'.format(STR_CROSSBAR))
STR_CROSSBAR_NL = str('{0}\n'.format(STR_CROSSBAR))

# some common str tags
TAG_DEFAULT_COMPANY = str('Amazon.Lumberyard')
TAG_DEFAULT_PROJECT = str('DccScriptingInterface')
TAG_MOCK_PROJECT = str('MockProject')
TAG_DIR_LY_DEV = str('dev')
TAG_DIR_DCCSI_AZPY = str('azpy')
TAG_DIR_DCCSI_SDK = str('SDK')
TAG_DIR_LY_BUILD = str('build')
TAG_QT_PLUGIN_PATH = str('QT_PLUGIN_PATH')

TAG_O3DE_FOLDER = str('.o3de')
TAG_O3DE_BOOTSTRAP = str('bootstrap.setreg')

# filesystem markers, stub file names.
STUB_LY_DEV = str('engine.json')
STUB_LY_ROOT_PROJECT = str('ly_project_stub')
STUB_LY_ROOT_DCCSI = str('dccsi_stub')
STUB_LY_DCCSI_AZPY = str('dccsi_azpy_stub')
STUB_LY_DCCSI_SDK = str('dccsi_sdk_stub')

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

#  some common paths
PATH_PROGRAMFILES_X86 = str(os.environ['PROGRAMFILES(X86)'])
PATH_PROGRAMFILES_X64 = str(os.environ['PROGRAMFILES'])

#  base env var key as str
ENVAR_COMPANY = str('COMPANY')
ENVAR_LY_PROJECT = str('LY_PROJECT')
ENVAR_LY_PROJECT_PATH = str('LY_PROJECT_PATH')
ENVAR_LY_DEV = str('LY_DEV')
ENVAR_DCCSIG_PATH = str('DCCSIG_PATH')
ENVAR_DCCSI_AZPY_PATH = str('DCCSI_AZPY_PATH')
ENVAR_DCCSI_SDK_PATH = str('DCCSI_SDK_PATH')
ENVAR_LY_BUILD_DIR_NAME = str('LY_BUILD_DIR_NAME')

ENVAR_LY_BUILD_PATH = str('LY_BUILD_PATH')
ENVAR_QT_PLUGIN_PATH = TAG_QT_PLUGIN_PATH
ENVAR_QTFORPYTHON_PATH = str('QTFORPYTHON_PATH')
ENVAR_LY_BIN_PATH = str('LY_BIN_PATH')

ENVAR_DCCSI_LOG_PATH = str('DCCSI_LOG_PATH')
ENVAR_DCCSI_LAUNCHERS_PATH = str('DCCSI_LAUNCHERS_PATH')

ENVAR_DCCSI_PY_VERSION_MAJOR = str('DCCSI_PY_VERSION_MAJOR')
ENVAR_DCCSI_PY_VERSION_MINOR = str('DCCSI_PY_VERSION_MINOR')
ENVAR_DCCSI_PYTHON_PATH = str('DCCSI_PYTHON_PATH')
ENVAR_DCCSI_PYTHON_LIB_PATH = str('DCCSI_PYTHON_LIB_PATH')
ENVAR_DCCSI_PYTHON_INSTALL = str('DCCSI_PYTHON_INSTALL')

ENVAR_WINGHOME = str('WINGHOME')
ENVAR_DCCSI_WING_VERSION_MAJOR = str('DCCSI_WING_VERSION_MAJOR')
ENVAR_DCCSI_WING_VERSION_MINOR = str('DCCSI_WING_VERSION_MINOR')

ENVAR_DCCSI_PY_BASE = str('DCCSI_PY_BASE')
ENVAR_DCCSI_PY_DCCSI = str('DCCSI_PY_DCCSI')
ENVAR_DCCSI_PY_MAYA = str('DCCSI_PY_MAYA')
ENVAR_DCCSI_PY_DEFAULT = str('DCCSI_PY_DEFAULT')

ENVAR_DCCSI_MAYA_VERSION = str('DCCSI_MAYA_VERSION')
ENVAR_MAYA_LOCATION = str('MAYA_LOCATION')

ENVAR_DCCSI_SDK_MAYA_PATH = str('DCCSI_SDK_MAYA_PATH')
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

TAG_LY_DCC_MAYA_MEL = 'dccsi_setup.mel'
TAG_MAYA_WORKSPACE = 'workspace.mel'


# dcc scripting interface common and default paths
PATH_LY_DEV = str(return_stub_dir(STUB_LY_DEV))
PATH_DCCSIG_PATH = str(return_stub_dir(STUB_LY_ROOT_DCCSI))
PATH_DCCSI_AZPY_PATH = str(return_stub_dir(STUB_LY_DCCSI_AZPY))
PATH_DCCSI_SDK_PATH = str('{0}\\{1}'.format(PATH_DCCSIG_PATH, TAG_DIR_DCCSI_SDK))

# logging into the cache
PATH_DCCSI_LOG_PATH = str('{LY_DEV}\\Cache\\{LY_PROJECT}\\pc\\user\\log\\logs')

# dev \ <build> \
STR_CONSTRUCT_LY_BUILD_PATH = str('{0}\\{1}')
PATH_LY_BUILD_PATH = str(STR_CONSTRUCT_LY_BUILD_PATH.format(PATH_LY_DEV,
                                                            TAG_DIR_LY_BUILD))

# ENVAR_QT_PLUGIN_PATH = TAG_QT_PLUGIN_PATH
STR_QTPLUGIN_DIR = str('{0}\\bin\\profile\\EditorPlugins')
STR_QTFORPYTHON_PATH = str('{0}\\Gems\\QtForPython\\3rdParty\\pyside2\\windows\\release')
STR_LY_BIN_PATH = str('{0}\\bin\\profile')

PATH_LY_BUILD_PATH = str('{0}\\{1}'.format(PATH_LY_DEV, TAG_DIR_LY_BUILD))
PATH_QTFORPYTHON_PATH = str(STR_QTFORPYTHON_PATH.format(PATH_LY_DEV))
PATH_QT_PLUGIN_PATH = str(STR_QTPLUGIN_DIR).format(PATH_LY_BUILD_PATH)
PATH_LY_BIN_PATH = str(STR_LY_BIN_PATH).format(PATH_LY_BUILD_PATH)

# py path string, parts, etc.
TAG_DEFAULT_PY = str('Launch_pyBASE.bat')

# config file stuff
FILENAME_DEFAULT_CONFIG = str('DCCSI_config.json')

# new o3de related paths
# os.path.expanduser("~") returns different values in py2.7 vs 3
PATH_USER_HOME = expanduser("~")
_LOGGER.debug('user home: {}'.format(PATH_USER_HOME))

# special case, make sure didn't return <user>\documents
parts = os.path.split(PATH_USER_HOME)

if str(parts[1].lower()) == 'documents':
    PATH_USER_HOME = parts[0]
    _LOGGER.debug('user home CORRECTED: {}'.format(PATH_USER_HOME))

PATH_USER_O3DE = str('{home}\\{o3de}').format(home=PATH_USER_HOME,
                                              o3de=TAG_O3DE_FOLDER)
PATH_USER_O3DE_REGISTRY = str('{0}\\Registry').format(PATH_USER_O3DE)
PATH_USER_O3DE_BOOTSTRAP = str('{reg}\\{file}').format(reg=PATH_USER_O3DE_REGISTRY,
                                                       file=TAG_O3DE_BOOTSTRAP)

#python and site-dir
TAG_DCCSI_PY_VERSION_MAJOR = str(3)
TAG_DCCSI_PY_VERSION_MINOR = str(7)
TAG_DCCSI_PY_VERSION_RELEASE = str(10)
TAG_PYTHON_EXE = str('python.exe')
TAG_TOOLS_DIR = str('Tools\\Python')
TAG_PLATFORM = str('windows')
STR_CONSTRUCT_DCCSI_PYTHON_INSTALL = str('{0}\\{1}\\{2}.{3}.{4}\\{5}')
PATH_DCCSI_PYTHON_PATH = str(STR_CONSTRUCT_DCCSI_PYTHON_INSTALL.format(PATH_LY_DEV,
                                                                       TAG_TOOLS_DIR,
                                                                       TAG_DCCSI_PY_VERSION_MAJOR,
                                                                       TAG_DCCSI_PY_VERSION_MINOR,
                                                                       TAG_DCCSI_PY_VERSION_RELEASE,
                                                                       TAG_PLATFORM))
PATH_DCCSI_PY_BASE = str('{0}\\{1}').format(PATH_DCCSI_PYTHON_PATH, TAG_PYTHON_EXE)
PATH_DCCSI_PY_DEFAULT = PATH_DCCSI_PY_BASE

# bootstrap site-packages by version
TAG_PY_MAJOR = str(sys.version_info.major)  # future proof
TAG_PY_MINOR = str(sys.version_info.minor)
STR_DCCSI_PYTHON_LIB_PATH = str('{0}\\3rdParty\\Python\\Lib\\{1}.x\\{1}.{2}.x\\site-packages')
PATH_DCCSI_PYTHON_LIB_PATH = STR_DCCSI_PYTHON_LIB_PATH.format(PATH_DCCSIG_PATH,
                                                              TAG_PY_MAJOR,
                                                              TAG_PY_MINOR)
# default path strings (and afe associated attributes)
TAG_DEFAULT_WING_MAJOR_VER = str(7)
TAG_DEFAULT_WING_MINOR_VER = str(1)
TAG_WING_IDE = str('\\Wing IDE ')  # old, pre 7
TAG_WING_PRO = str('\\Wing Pro ')  # new 7+
STR_CONSTRUCT_WING_PATH = str('{progX86}{wing_tag}{major}.{minor}')
PATH_DEFAULT_WINGHOME = str('{0}{1}{2}.{3}'
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
    # there are not really tests to run here due to this being a list of
    # constants for shared use.
    _G_DEBUG = True
    _DCCSI_DEV_MODE = True
    _LOGGER.setLevel(_logging.DEBUG)  # force debugging

    # this is a top level module and to reduce cyclical azpy imports
    # it only has a basic logger configured, add log to console
    _handler = _logging.StreamHandler(sys.stdout)
    _handler.setLevel(_logging.DEBUG)
    _formatter = _logging.Formatter(FRMT_LOG_LONG)
    _handler.setFormatter(_formatter)
    _LOGGER.addHandler(_handler)

    # happy print
    _LOGGER.info(STR_CROSSBAR)
    _LOGGER.info('~ constants.py ... Running script as __main__')
    _LOGGER.info(STR_CROSSBAR)

    #  this is just a debug developer convenience print (for testing acess)
    import pkgutil
    _LOGGER.info('Current working dir: {0}'.format(os.getcwd()))
    search_path = ['.']  # set to None to see all modules importable from sys.path
    all_modules = [x[1] for x in pkgutil.iter_modules(path=search_path)]
    _LOGGER.info('All Available Modules in working dir: {0}'.format(all_modules))

    #  test anything procedurally generated
    _LOGGER.info('Testing procedural env paths ...')
    from pathlib import Path

    _stash_dict = {}
    _stash_dict['LY_DEV'] = Path(PATH_LY_DEV)
    _stash_dict['DCCSIG_PATH'] = Path(PATH_DCCSIG_PATH)
    _stash_dict['DCCSI_AZPY_PATH'] = Path(PATH_DCCSI_AZPY_PATH)
    _stash_dict['DCCSI_SDK_PATH'] = Path(PATH_DCCSI_SDK_PATH)
    _stash_dict['DCCSI_PYTHON_PATH'] = Path(PATH_DCCSI_PYTHON_PATH)
    _stash_dict['DCCSI_PY_BASE'] = Path(PATH_DCCSI_PY_BASE)
    _stash_dict['DCCSI_PYTHON_LIB_PATH'] = Path(PATH_DCCSI_PYTHON_LIB_PATH)
    _stash_dict['LY_BUILD_PATH'] = Path(PATH_LY_BUILD_PATH)
    _stash_dict['LY_BIN_PATH'] = Path(PATH_LY_BIN_PATH)
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
            _LOGGER.info('{0}: {1}'.format(key, value))
        except Exception as e:
            _LOGGER.warning('FAILED PATH: {}'.format(e)) 

    # custom prompt
    sys.ps1 = "[azpy]>>"
