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
"""! A common Class object for DCCsi configs

:file: < DCCsi >/azpy/config_class.py
:Status: Prototype
:Version: 0.0.1

This class reduces boilerplate and should streamline code used across
various config.py files within the DCCsi, by wrapping various functionality,
and providing common methods to inheret and/or extend.

The DCCsi config pattern utilizes a robust configuration and settings
package called dynaconf: https://www.dynaconf.com
"""
# -------------------------------------------------------------------------
# standard imports
import os
from pathlib import Path
import logging as _logging
from typing import Union
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# a suggestion is that we make the root DccScriptingInterfance a pkg,
# by placing a top-level __init__ so we can have something like the following
# _MODULENAME = 'DCCsi.azpy.config_class'
# import DccScriptingInterface as DCCsi
# from DCCsi import _PATH_DCCSIG

# global scope
_MODULENAME = 'azpy.config_class'

__all__ = ['ConfigCore']

_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug(f'Initializing: {_MODULENAME}')

from azpy import _PATH_DCCSIG  # root DCCsi path
# DCCsi imports
from azpy.env_bool import env_bool
# -------------------------------------------------------------------------

6
# -------------------------------------------------------------------------
# global constants here
DCCSI_DYNAMIC_PREFIX = 'DYNACONF'

from azpy.constants import ENVAR_PATH_DCCSIG
from azpy.constants import ENVAR_DCCSI_SYS_PATH
from azpy.constants import ENVAR_DCCSI_PYTHONPATH
from azpy.constants import TAG_DCCSI_LOCAL_SETTINGS_SLUG

_default_settings_filepath = Path(TAG_DCCSI_LOCAL_SETTINGS_SLUG)
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
class ConfigClass(object):
    """! Class constructor: makes a DCCsi Config object.
    ...

    Attributes
    ----------
    config_name : str
        (Optional) name of the config, e.g. Tools.DCC.Substance.config
    """

    def __init__(self,
                 config_name=None,
                 *args, **kwargs):
        '''! The ConfigClass base class initializer.'''

        self._dccsi_dir = _PATH_DCCSIG

        self._tracked_settings = list()
        self._sys_path = list()
        self._pythonpath = list()
        self._settings = None

    # -- properties -------------------------------------------------------
    @property
    def tracked_settings(self):
        ''':Class property: settings this class object will track. Normally
        this may be superfluous, as settings can be retreived via the
        standard dynaconf patterns. Only add a setting to this property if
        you have a reason to track a specialized setting.
         :return tracked_settings: dict'''
        return self._tracked_settings

    @tracked_settings.setter
    def tracked_settings(self, new_dict: dict) -> dict:
        ''':param new_dict: replace entire tracked_settings
        :return tracked_settings: dict'''
        self._tracked_settings = new_dict
        return self._tracked_settings

    @tracked_settings.getter
    def tracked_settings(self):
        ''':return: tracked_settings dict'''
        return self._tracked_settings

    @property
    def local_settings(self):
        ''':Class property: non-managed settings (fully local to this config object)
         :return local_settings: dict'''
        return self._local_settings

    @local_settings.setter
    def local_settings(self, new_dict: dict) -> dict:
        ''':param new_dict: replace entire local_settings
        :return local_settings: dict'''
        self._local_settings = new_dict
        return self._local_settings

    @local_settings.getter
    def local_settings(self):
        ''':return: local_settings dict'''
        return self._local_settings

    @property
    def sys_path(self):
        ''':Class property: for stashing PATHs for managed settings
        :return sys_path: list'''
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
        ''':Class property: List for stashing PYTHONPATHs for managed settings'''
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
    def settings(self):
        ''':Class property: storage for settings'''
        return self._settings

    @settings.setter
    def settings(self, settings):
        ''':param settings: the settings object to store'''
        self._settings = new_list
        return self._settings

    @settings.getter
    def settings(self, set):
        ''':return: a list for PYTHONPATH, site.addsitedir()'''

        # now standalone we can validate the config. env, settings.
        from dynaconf import settings
        if set_env:
            settings.setenv()

        return self._settings
    # ---------------------------------------------------------------------


    # --method-set---------------------------------------------------------
    def set_envar(self, key: str, value: str):
        '''! sets the envar
        @param key: the enavr key as a string
        @ param value: the enavr value as a string
        Path type objects are specially handled.'''
        # standard environment
        if isinstance(value, Path):
            os.environ[key] = value.as_posix()
        else:
            os.environ[key] = str(value)

    def add_setting(self,
                    key: str,
                    value: Union[int, str, bool, list, dict, Path],
                    set_dyanmic: bool = True,
                    prefix: str = DCCSI_DYNAMIC_PREFIX,
                    check_envar: bool = True,
                    set_envar: bool = False,
                    set_sys_path: bool = False,
                    set_pythonpath: bool = False,
                    tracked_setting=False):
        '''! adds a settings with various configurable options

        @param key: the key (str) for the setting/envar

        @param value: the stored value for the setting

        @param set_dyanmic: makes this a dynamic setting (dynaconf)
            Dyanmic settings will use the prefix_ to specify membership
                os.environ['DYNACONF_FOO'] = 'foo'
            This setting will be present in the dynaconf settings object
                from dynaconf import settings
            And will be in the dynamic settings
                print(settings.FOO)
            And the dynamic environment
                settings.setenv()
                print(os.getenv('FOO'))

        @param prefix: specifies the default prefix for the dyanamic env
            The default is:
            DCCSI_DYNAMIC_PREFIX = 'DYNACONF'

        @param check_envar: This will check if this is set in the external
            env such that we can retain the external setting as an
            override, if it is not externally set we will use the passed
            in value

        @param set_envar: this will set an envar in the traditional way
            os.environ['key'] = value

            this is optional, but may be important if ...
            you are running other code that relies on this envar
            and want or need access, before the following occures:

                from dynaconf import settings
                settings.setenv()

                class_object.get_config_settings(set_env=True)

        @param set_sys_path: a list of paths to be added to PATH
            This uses traditional direct manipulation of env PATH
            @see self.add_path_list_to_envar()

        @param set_pythonpath: a list of paths to be added to PYTHONPATH
            This uses the site.addsitedir() approach to add sire access
            @see self.add_path_list_to_addsitedir()

        @param tracked_setting: tracks the setting on the class object
            This isn't normally needed, since settings object can be accessed
            This would be used in a specail case where you need to retreive
            a settings directly from the ConfigClass rather then via settings.
        '''
        # -----------------------------------------------------------------

        if isinstance(value, Path) or set_sys_path or set_pythonpath:
            value = Path(value).resolve()

        if check_envar:
            if isinstance(value, bool):
                # this checks and returns value as bool
                value = env_bool(key, value)
            else:
                # if the envar is set, it will override the input value!
                value = os.getenv(key, value)

        if set_envar:
            self.set_envar(key, value)

        if set_dyanmic:
            dynakey = f'{prefix}_{key}'
            self.set_envar(dynakey, value)

        if set_sys_path:
            self.sys_path.append(value.as_posix())

        if set_pythonpath:
            self.pythonpath.append(value.as_posix())

        return (key, value)

    # -----------------------------------------------------------------
    def add_path_list_to_envar(self,
                               envar: str = 'PATH',
                               path_list: list = self._sys_path):
        """! add list of Paths to update system ENVAR (like PATH).
        This method explicitly adds the paths to the system ENVAR.

        Note: this handles PATH in a way

        @param envar: str
            add paths to this ENVAR

        @param path_list: list
            a list() of paths

        @return: the envar setting"""

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

    # -----------------------------------------------------------------
    def add_path_list_to_addsitedir(self,
                                    envar: str = 'PYTHONPATH',
                                    path_list: list):
        """! Ensures each path in list, is fully added as searchable
        code access.(site.addsitedir). Works in conjuction with paths
        from the defined ENVAR.

        @param path_list: list
            a list() of paths

        @param envar: str
            add paths to this ENVAR, and site.addsitedir

        @return: os.environ[envar]"""

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

    # -----------------------------------------------------------------
    def export_settings(self,
                        settings_filepath: Path = _default_settings_filepath,
                        use_dynabox: bool = False,
                        env: bool = False,
                        merge: bool = False
                        log_settings: bool = False):
        '''! exports the settings to a file

        @param settings_filepath: The file path for the exported
        settings. Default file is: settings.local.json
        This file name is chosen as it's also a default settings file
        dynaconf will read in when initializing settings.

            # basic file writer looks something like
            dynaconf.loaders.write(settings_path=Path('settings.local.json'),
                                   settings_data=DynaBox(settings).to_dict(),
                                   env='core',
                                   merge=True)

        @param use_dynabox: use dynaconf.utils.boxing module
            https://dynaconf.readthedocs.io/en/docs_223/reference/dynaconf.utils.html

        @param env: put settings into an env, e.g. development
            https://dynaconf.readthedocs.io/en/docs_223/reference/dynaconf.loaders.html

        @param merge: whether existing file should be merged with new data

        @param log_settings: ouptus settings contents to logging

        The default is to write: < dccsi >/settings.local.json

        Dynaconf is configured by default to behave in the following
        manner,

        This is the Dynaconf call to initialize and retreive settings,
        make this call from your entrypoint e.g. main.py:

            from dynaconf import settings

        That will find and execute the local config and aggregate
        settings from the following:
            config.py
            settings.py
            settings.json
            settings.local.json

        We do not commit settings.local.json to source control,
        effectively it can be created and used as a local stash of the
        settings, with the beneift that a user can overide settings locally.

        '''
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

    _foo_test = ConfigClass()

    new_list = list()
    new_list.append('c:/foo')
    new_list.append('c:/fooey')
    _foo_test.sys_path = new_list

    # add an envar path
    _foo_test.add_setting('kablooey', Path('c:/kablooey'))

    # add a envar bool
    _bool_test = _foo_test.add_setting('foo_is', True)

    if _bool_test:
        _LOGGER.info(f'The boolean is: {_bool_test}')

    # add a envar int
    _foo_test.add_setting('foo_level', 10)

    # add a envar dict
    _foo_test.add_setting('foo_dict', {'fooey': 'Is fun!'})

    _LOGGER.debug(f'_foo_test.sys_path = {_foo_test.sys_path}')

    settings = _foo_test.get_config_settings()

    _LOGGER.info(f'settings: {settings}')
