# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# note: this module should reamin py2.7 compatible (Maya) so no f'strings
# --------------------------------------------------------------------------
import sys
import os
import re
import site
import logging as _logging
# -------------------------------------------------------------------------


# --------------------------------------------------------------------------
# note: this module is called in other root modules
# must avoid cyclical imports

# global scope
# normally would pull the constant envar string
# but avoiding cyclical imports here
FRMT_LOG_LONG = "[%(name)s][%(levelname)s] >> %(message)s (%(asctime)s; %(filename)s:%(lineno)d)"
from azpy.env_bool import env_bool
_DCCSI_GDEBUG = env_bool('DCCSI_GDEBUG', False)
_DCCSI_LOGLEVEL = env_bool('DCCSI_LOGLEVEL', False)
_DCCSI_LOGLEVEL = int(env_bool('DCCSI_LOGLEVEL', int(20)))
if _DCCSI_GDEBUG:
    _DCCSI_LOGLEVEL = int(10)

_MODULENAME = __name__
if _MODULENAME is '__main__':
    _MODULENAME = 'azpy.config_utils'
    
# set up module logging
#for handler in _logging.root.handlers[:]:
    #_logging.root.removeHandler(handler)
_LOGGER = _logging.getLogger(_MODULENAME)
#_logging.basicConfig(format=FRMT_LOG_LONG, level=_DCCSI_LOGLEVEL)
_LOGGER.propagate = False
_LOGGER.debug('Initializing: {0}.'.format({_MODULENAME}))

__all__ = ['get_os', 'return_stub', 'get_stub_check_path',
           'get_dccsi_config', 'get_current_project']
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# just a quick check to ensure what paths have code access
if _DCCSI_GDEBUG:
    known_paths = list()
    for p in sys.path:
        known_paths.append(p)
    _LOGGER.debug(known_paths)
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# this import can fail in Maya 2020 (and earlier) stuck on py2.7
# wrapped in a try, to trap and providing messaging to help user correct
try:
    from pathlib import Path  # note: we provide this in py2.7
    # so using it here suggests some boostrapping has occured before using azpy
except Exception as e:
    _LOGGER.warning('Maya 2020 and below, use py2.7')
    _LOGGER.warning('py2.7 does not include pathlib')
    _LOGGER.warning('Try installing the O3DE DCCsi py2.7 requirements.txt')
    _LOGGER.warning("See instructions: 'C:\\< your o3de engine >\\Gems\\AtomLyIntegration\\TechnicalArt\\DccScriptingInterface\\SDK\Maya\\readme.txt'")
    _LOGGER.warning("Other code in this module with fail!!!")
    _LOGGER.error(e)
    pass  # fail gracefully, note: code accesing Path will fail!
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def get_os():
    """returns lumberyard dir names used in python path"""
    if sys.platform.startswith('win'):
        os_folder = "windows"
    elif sys.platform.startswith('darwin'):
        os_folder = "mac"
    elif sys.platform.startswith('linux'):
        os_folder = "linux_x64"
    else:
        message = str("DCCsi.azpy.config_utils.py: "
                      "Unexpectedly executing on operating system '{}'"
                      "".format(sys.platform))

        raise RuntimeError(message)
    return os_folder
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
from azpy.core import get_datadir
# there was a method here refactored out to add py2.7 support for Maya 2020
#"DccScriptingInterface\azpy\core\py2\utils.py get_datadir()"
#"DccScriptingInterface\azpy\core\py3\utils.py get_datadir()"
# Warning: planning to deprecate py2 support
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def return_stub_dir(stub_file='dccsi_stub'):
    _dir_to_last_file = None
    '''Take a file name (stub_file) and returns the directory of the file (stub_file)'''
    # To Do: refactor to use pathlib object oriented Path
    if _dir_to_last_file is None:
        path = os.path.abspath(__file__)
        while 1:
            path, tail = os.path.split(path)
            if (os.path.isfile(os.path.join(path, stub_file))):
                break
            if (len(tail) == 0):
                path = ""
                _LOGGER.debug('I was not able to find the path to that file '
                              '({}) in a walk-up from currnet path'
                              ''.format(stub_file))
                break

        _dir_to_last_file = path

    return _dir_to_last_file
# --------------------------------------------------------------------------


# -------------------------------------------------------------------------
def get_stub_check_path(in_path=os.getcwd(), check_stub='engine.json'):
    '''
    Returns the branch root directory of the dev\\'engine.json'
    (... or you can pass it another known stub)

    so we can safely build relative filepaths within that branch.

    If the stub is not found, it returns None
    '''
    path = Path(in_path).absolute()

    while 1:
        test_path = Path(path, check_stub)

        if test_path.is_file():
            return Path(test_path).parent

        else:
            path, tail = (path.parent, path.name)

            if (len(tail) == 0):
                return None
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def get_o3de_engine_root(check_stub='engine.json'):
    # get the O3DE engine root folder
    # if we are running within O3DE we can ensure which engine is running
    _O3DE_DEV = None
    try:
        import azlmbr  # this file will fail outside of O3DE
    except ImportError as e:
        # if that fails, we can search up
        # search up to get \dev
        _O3DE_DEV = get_stub_check_path(check_stub='engine.json')
        # To Do: What if engine.json doesn't exist?
    else:
        # execute if no exception
        # allow for external ENVAR override
        from azpy.constants import ENVAR_O3DE_DEV
        _O3DE_DEV = Path(os.getenv(ENVAR_O3DE_DEV, azlmbr.paths.engroot))
    finally:
        # note: can't use fstrings as this module gets called with py2.7 in maya
        _LOGGER.info('O3DE engine root: {}'.format(_O3DE_DEV.resolve()))
    return _O3DE_DEV
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# settings.setenv()  # doing this will add the additional DYNACONF_ envars
def get_dccsi_config(dccsi_dirpath=return_stub_dir()):
    """Convenience method to set and retreive settings directly from module."""

    # we can go ahead and just make sure the the DCCsi env is set
    # config is SO generic this ensures we are importing a specific one
    _module_tag = "dccsi.config"
    _dccsi_path = Path(dccsi_dirpath, "config.py")
    if _dccsi_path.exists():
        if sys.version_info.major >= 3:
            import importlib  # note: py2.7 doesn't have importlib.util
            import importlib.util
            #from importlib import util
            _spec_dccsi_config = importlib.util.spec_from_file_location(_module_tag,
                                                                        str(_dccsi_path.resolve()))
            _dccsi_config = importlib.util.module_from_spec(_spec_dccsi_config)
            _spec_dccsi_config.loader.exec_module(_dccsi_config)

            _LOGGER.debug('Executed config: {}'.format(_spec_dccsi_config))
        else:  # py2.x
            import imp
            _dccsi_config = imp.load_source(_module_tag, str(_dccsi_path.resolve()))
            _LOGGER.debug('Imported config: {}'.format(_spec_dccsi_config))
        return _dccsi_config

    else:
        return None
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def get_current_project_cfg(dev_folder=get_stub_check_path()):
    """Uses regex in lumberyard Dev\\bootstrap.cfg to retreive project tag str
    Note: boostrap.cfg will be deprecated.  Don't use this method anymore."""
    boostrap_filepath = Path(dev_folder, "bootstrap.cfg")
    if boostrap_filepath.exists():
        bootstrap = open(str(boostrap_filepath), "r")
        regex_str = r"^project_path\s*=\s*(.*)"
        game_project_regex = re.compile(regex_str)
        for line in bootstrap:
            game_folder_match = game_project_regex.match(line)
            if game_folder_match:
                _LOGGER.debug('Project is: {}'.format(game_folder_match.group(1)))
                return game_folder_match.group(1)
    return None
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def get_check_global_project():
    """Gets o3de project via .o3de data in user directory"""

    from azpy.constants import PATH_USER_O3DE_BOOTSTRAP
    from collections import OrderedDict
    from box import Box
    from azpy.core import get_datadir
    
    bootstrap_box = None
    json_file_path = Path(PATH_USER_O3DE_BOOTSTRAP)
    if json_file_path.exists():
        try:
            bootstrap_box = Box.from_json(filename=str(json_file_path.resolve()),
                                          encoding="utf-8",
                                          errors="strict",
                                          object_pairs_hook=OrderedDict)
        except IOError as e:
            # this file runs in py2.7 for Maya 2020, FileExistsError is not defined
            _LOGGER.error('Bad file interaction: {}'.format(json_file_path.resolve()))
            _LOGGER.error('Exception is: {}'.format(e))
            pass
    if bootstrap_box:
        # this seems fairly hard coded - what if the data changes?
        project_path=Path(bootstrap_box.Amazon.AzCore.Bootstrap.project_path)
        return project_path
    else:
        return None
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def get_o3de_project_path():
    """figures out the o3de project path
    if not found defaults to the engine folder"""
    _O3DE_PROJECT_PATH = None
    try:
        import azlmbr  # this file will fail outside of O3DE
    except ImportError as e:
        # (fallback 1) this checks if a global project is set
        # This check user home for .o3de data
        _O3DE_PROJECT_PATH = get_check_global_project()
    else:
        # execute if no exception, this would indicate we are in O3DE land
        # allow for external ENVAR override
        from azpy.constants import ENVAR_O3DE_PROJECT_PATH
        _O3DE_PROJECT_PATH = Path(os.getenv(ENVAR_O3DE_PROJECT_PATH, azlmbr.paths.projectroot))
    finally:
        # (fallback 2) if None, fallback to engine folder
        if not _O3DE_PROJECT_PATH:
            _O3DE_PROJECT_PATH = get_o3de_engine_root()
        # note: can't use fstrings as this module gets called with py2.7 in maya
        _LOGGER.info('O3DE project root: {}'.format(_O3DE_PROJECT_PATH.resolve()))
    return _O3DE_PROJECT_PATH
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def bootstrap_dccsi_py_libs(dccsi_dirpath=return_stub_dir()):
    """Builds and adds local site dir libs based on py version"""

    from azpy.constants import STR_DCCSI_PYTHON_LIB_PATH  # a path string constructor
    _DCCSI_PYTHON_LIB_PATH = Path(STR_DCCSI_PYTHON_LIB_PATH.format(dccsi_dirpath,
                                                                   sys.version_info[0],
                                                                   sys.version_info[1]))

    if _DCCSI_PYTHON_LIB_PATH.exists():
        site.addsitedir(_DCCSI_PYTHON_LIB_PATH.resolve())  # PYTHONPATH
        _LOGGER.debug('Performed site.addsitedir({})'
                      ''.format(_DCCSI_PYTHON_LIB_PATH.resolve()))
        return _DCCSI_PYTHON_LIB_PATH
    else:
        message = "Doesn't exist: {}".format(_DCCSI_PYTHON_LIB_PATH)
        _LOGGER.error(message)
        raise NotADirectoryError(message)
# -------------------------------------------------------------------------


###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':

    # happy print
    _LOGGER.info("# {0} #".format('-' * 72))
    _LOGGER.info('~ config_utils.py ... Running script as __main__')
    _LOGGER.info("# {0} #".format('-' * 72))

    _LOGGER.info('Current Work dir: {0}'.format(os.getcwd()))

    _LOGGER.info('OS: {}'.format(get_os()))

    _LOGGER.info('DCCSIG_PATH: {}'.format(return_stub_dir('dccsi_stub')))

    _config = get_dccsi_config()
    _LOGGER.info('DCCSI_CONFIG_PATH: {}'.format(_config))

    _LOGGER.info('O3DE_DEV: {}'.format(get_o3de_engine_root(check_stub='engine.json')))

    # new o3de version
    _LOGGER.info('O3DE_PROJECT: {}'.format(get_check_global_project()))

    _LOGGER.info('DCCSI_PYTHON_LIB_PATH: {}'.format(bootstrap_dccsi_py_libs(return_stub_dir('dccsi_stub'))))

    # custom prompt
    sys.ps1 = "[azpy]>>"
