# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the
# root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -------------------------------------------------------------------------
"""A common Class object for DCCsi config

    < DCCsi >/azpy/config_class.py

:Status: Prototype
:Version: 0.0.1
"""
# -------------------------------------------------------------------------
# standard imports
import os
from pathlib import Path
import logging as _logging
from typing import Union
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# global scope
_MODULENAME = 'azpy.config_class'

__all__ = ['ConfigCore']

_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug(f'Initializing: {_MODULENAME}')

from azpy import _PATH_DCCSIG  # root DCCsi path
# DCCsi imports
from azpy.env_bool import env_bool
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# temporarily put defaults/constants here
DCCSI_DYNAMIC_PREFIX = 'DYNACONF'

ENVAR_0 = 'DYNACONF_DCCSI_SYS_PATH'
ENVAR_1 = 'DYNACONF_DCCSI_PYTHONPATH'

# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
class ConfigCore(object):
    """Class constructor: makes a DCCsi Config object.

    ...

    Attributes
    ----------
    config_name : str
        the name of the config, e.g. Tools.DCC.Substance.config

    parent_config : ConfigCore
        another ConfigCore object, as parent config

    Methods
    -------
    foo(in=None)
        Does something amazing
    """

    def __init__(self,
                 config_name=None,
                 parent_config=None,
                 *args, **kwargs):

        self._sys_path = list()
        self._pythonpath = list()
        self._pythonpath_exclude = list()
        self._local_settings = {}

    # -- properties -------------------------------------------------------
    @property
    def sys_path(self):
        '''List for stashing PATHs for managed settings'''
        return self._sys_path

    @sys_path.setter
    def sys_path(self, new_list: list) -> list:
        ''':param new_list: replace entire sys_path'''
        self._sys_path = new_list
        return self._sys_path

    @sys_path.getter
    def sys_path(self) -> list:
        ''':return: a list for PATH, sys.path'''
        return self._sys_path

    @property
    def pythonpath(self):
        '''List for stashing PYTHONPATHs for managed settings'''
        return self._pythonpath

    @pythonpath.setter
    def pythonpath(self, new_list: list) -> list:
        ''':param new_list: replace entire pythonpath'''
        self._pythonpath = new_list
        return self._pythonpath

    @pythonpath.getter
    def pythonpath(self):
        ''':return: a list for PYTHONPATH, site.addsitedir()'''
        return self._pythonpath

    @property
    def pythonpath_exclude(self):
        '''List() stash local PYTHONPATHs in a non-managed way'''
        return self._pythonpath_exclude

    @pythonpath_exclude.setter
    def pythonpath_exclude(self, new_list: list) -> list:
        ''':param new_list: replace entire pythonpath_exclude'''
        self._pythonpath_exclude = new_list
        return self._pythonpath_exclude

    @pythonpath_exclude.getter
    def pythonpath_exclude(self):
        ''':return: list of paths to be excluded from PYTHONPATH'''
        return self._pythonpath_exclude

    @property
    def local_settings(self):
        '''dict for non-managed settings (fully local to this cofig object)'''
        return self._local_settings

    @local_settings.setter
    def local_settings(self, new_dict: dict) -> dict:
        ''':param new_dict: replace entire local_settings'''
        self._local_settings = new_dict
        return self._local_settings

    @local_settings.getter
    def local_settings(self):
        return self._local_settings
    # ---------------------------------------------------------------------


    # --method-set---------------------------------------------------------
    def method():
        '''a method docstring'''
        pass

    def add_setting(self,
                    key: str,
                    value: Union[int, str, bool, list, Path],
                    set_dyanmic: bool = True,
                    prefix: str = DCCSI_DYNAMIC_PREFIX,
                    get_envar: bool = True,
                    set_envar: bool = True,
                    set_sys_path: bool = False,
                    set_pythonpath: bool = False,
                    set_pythonpath_exclude: bool = False):
        '''to do ...'''

        if isinstance(value, Path) or set_sys_path \
           or set_pythonpath or set_pythonpath_exclude:
            value = Path(value).resolve()

        if get_envar:
            if isinstance(value, bool):
                # this checks and returns value as bool
                value = env_bool(key)
            elif isinstance(value, list):
                # to do
                pass
            else:
                # if the envar is set, it will override the input value!
                value = os.getenv(key, value)

        if set_envar:
            # standard environment
            if isinstance(value, Path):
                os.environ[key] = value.as_posix()
            elif isinstance(value, bool) or isinstance(value, int):
                os.environ[key] = str(value)
            else:
                os.environ[key] = value

        if set_dyanmic:
            # dynamic environment, aka dynaconf settings
            if isinstance(value, Path):
                os.environ[f'{prefix}_{key}'] = value.as_posix()
            elif isinstance(value, bool) or isinstance(value, int):
                os.environ[f'{prefix}_{key}'] = str(value)
            else:
                os.environ[f'{prefix}_{key}'] = value

        if set_sys_path:
            self.sys_path.append(value.as_posix())

        if set_pythonpath:
            self.pythonpath.append(value.as_posix())

        if set_pythonpath_exclude:
            self.pythonpath_exclude.append(value.as_posix())

        return (key, value)

    def add_path_list_to_envar(self,
                               envar: str,
                               path_list: list):
        """!
        Take in a list of Path objects to add to system ENVAR (like PATH).
        This method explicitly adds the paths to the system ENVAR.

        @param path_list: list
            a list() of paths

        @param envar: str
            add paths to this ENVAR

        @ return: os.environ[envar]"""

        _LOGGER.info('checking envar: {}'.format(envar))

        # get or default to empty
        if envar in os.environ:
            _ENVAR = os.getenv(envar, "< NOT SET >")
        else:
            os.environ[envar]='' # create empty
            _ENVAR = os.getenv(envar)

        # this is separated (split) into a list
        known_pathlist = _ENVAR.split(os.pathsep)

        if len(path_list) == 0:
            _LOGGER.warning('No {} paths added, path_list is empty'.format(envar))
            return None
        else:
            _LOGGER.info('Adding paths to envar: {}'.format(envar))
            # To Do: more validation and error checking?
            for p in path_list:
                p = Path(p)
                if (p.exists() and p.as_posix() not in known_pathlist):
                    os.environ[envar] = p.as_posix() + os.pathsep + os.environ[envar]
                    # adding directly to sys.path apparently doesn't work for .dll locations like Qt
                    # this pattern by extending the ENVAR seems to work correctly

        return os.environ[envar]

    def add_path_list_to_addsitedir(self,
                                    envar: str,
                                    path_list: list):
        """!
        Take in a list of Path objects to add to system ENVAR (like PYTHONPATH).
        This makes sure each path is fully added as searchable code access.
        Mainly to use/access site.addsitedir so from imports work in our namespace.

        @param path_list: list
            a list() of paths

        @param envar: str
            add paths to this ENVAR, and site.addsitedir

        @ return: os.environ[envar]"""

        _LOGGER.info('checking envar: {}'.format(envar))

        # get or default to empty
        if envar in os.environ:
            _ENVAR = os.getenv(envar, "< NOT SET >")
        else:
            os.environ[envar]='' # create empty
            _ENVAR = os.getenv(envar)

        # this is separated (split) into a list
        known_pathlist = _ENVAR.split(os.pathsep)

        new_pathlist = known_pathlist.copy()

        if len(path_list) == 0:
            _LOGGER.warning('No {} paths added, path_list is empty'.format(envar))
            return None
        else:
            _LOGGER.info('Adding paths to envar: {}'.format(envar))
            for p in path_list:
                p = Path(p)
                if (p.exists() and p.as_posix() not in known_pathlist):
                    sys.path.insert(0, p.as_posix())
                    new_pathlist.insert(0, p.as_posix())
                    # Add site directory to make from imports work in namespace
                    site.addsitedir(p.as_posix())

            os.environ[envar] = os.pathsep.join(new_pathlist)

        return os.environ[envar]

    def get_config_settings(self, set_env: bool = True):
        '''To do ...'''
        pass
    # ---------------------------------------------------------------------

###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """Run in debug perform local tests from IDE or CLI"""

    from azpy.constants import STR_CROSSBAR

    _LOGGER.info(STR_CROSSBAR)
    _LOGGER.info(f'~ {_MODULENAME}.py ... Running script as __main__')
    _LOGGER.info(STR_CROSSBAR)

    _foo_test = ConfigCore()

    new_list = list()
    new_list.append('c:/foo')
    new_list.append('c:/fooey')
    _foo_test.sys_path = new_list

    # add an envar path
    _foo_test.add_setting('kablooey', Path('c:/kablooey'))

    # add a envar bool
    _foo_test.add_setting('foo_is', True)

    # add a envar int
    _foo_test.add_setting('foo_level', 10)

    # add a envar dict
    _foo_test.add_setting('foo_level', {'fooey': 'Is fun!'})

    _LOGGER.debug(f'_foo_test.sys_path = {_foo_test.sys_path}')



    settings = _foo_test.get_config_settings()
