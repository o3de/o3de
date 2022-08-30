#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
import site
from box import Box
from pathlib import Path
import logging as _logging
from typing import Union
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
import DccScriptingInterface

# global scope
# we should update pkg, module and logging names to start with dccsi
_MODULENAME = 'azpy.config_class'

__all__ = ['ConfigCore']

_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug(f'Initializing: {_MODULENAME}')

from azpy import _PATH_DCCSIG  # root DCCsi path
# DCCsi imports
from azpy.env_bool import env_bool
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# global constants here
from DccScriptingInterface.constants import DCCSI_DYNAMIC_PREFIX

from azpy.constants import ENVAR_PATH_DCCSIG
from azpy.constants import ENVAR_DCCSI_SYS_PATH
from azpy.constants import ENVAR_DCCSI_PYTHONPATH

_default_settings_filepath = Path('setting.local.json')
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
class ConfigClass(object):
    """! Class constructor: makes a DCCsi Config object.
    ...

    Attributes
    ----------
    config_name : str
        (Optional) name of the config, e.g. Tools.DCC.Substance.config

    auto_set : str
        If set the settings.setenv() will be set automatically
        for example the settings are ensured to be set when pulled
        otherwise if not manually set this would assert

        _bool_test = _foo_test.add_setting('FOO_IS', True)
        if _foo_test.setting.FOO_IS:
            # do this code

        A suggeston would be, that this needs to be profiled?
        It might be slow?
    """

    def __init__(self,
                 config_name=None,
                 auto_set=True,
                 dyna_prefix: str = DCCSI_DYNAMIC_PREFIX,
                 *args, **kwargs):
        '''! The ConfigClass base class initializer.'''

        self._config_name = config_name
        self._auto_set = auto_set
        self._dyna_prefix = dyna_prefix

        # dynaconf, dynamic settings
        self._settings = dict()

        # all settings storage (if tracked)
        self._tracked_settings = dict()

        # special path storage
        self._sys_path = list()

        self._pythonpath = list()

    # -- properties -------------------------------------------------------
    @property
    def auto_set(self):
        ''':Class property: if true, self.settings.setev() is automatic'''
        return self._settings

    @auto_set.setter
    def auto_set(self, value: bool = True):
        ''':param settings: the settings object to store'''
        self._auto_set = value
        return self._auto_set

    @auto_set.getter
    def auto_set(self):
        ''':return: auto_set bool'''
        return self._auto_set

    @property
    def settings(self):
        ''':Class property: storage for dynamic managed settings'''
        return self._settings

    @settings.setter
    def settings(self, settings):
        ''':param settings: the settings object to store'''
        self._settings = settings
        return self._settings

    @settings.getter
    def settings(self):
        '''! If the class property self.auto_set is Rrue, settings.setenv() occurs
        :return: settings, dynaconf'''
        # now standalone we can validate the config. env, settings.
        from dynaconf import settings
        self._settings = settings
        if self.auto_set:
            self._settings.setenv()
        return self._settings

    def get_settings(self, set_env: bool = True):
        '''! this getter will ensure to set the env
        @param set_env: bool, defaults to true
        :return: settings, dynaconf'''
        if set_env:
            self.settings.setenv()
        return self.settings

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
        for path in new_list:
            path = Path(path).resolve()

            path = self.add_code_path(path,
                                      self._sys_path,
                                      sys_envar='PATH',
                                      local_envar=ENVAR_DCCSI_SYS_PATH)

        dynakey = f'{self._dyna_prefix}_{ENVAR_DCCSI_SYS_PATH}'
        os.environ[dynakey] = os.getenv(ENVAR_DCCSI_SYS_PATH)

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
        for path in new_list:
            path = Path(path).resolve()

            path = self.add_code_path(path,
                                      self._pythonpath,
                                      sys_envar='PYTHONPATH',
                                      local_envar=ENVAR_DCCSI_PYTHONPATH)

        dynakey = f'{self._dyna_prefix}_{ENVAR_DCCSI_PYTHONPATH}'
        os.environ[dynakey] = os.getenv(ENVAR_DCCSI_PYTHONPATH)

        return self._pythonpath

    @pythonpath.getter
    def pythonpath(self):
        ''':return: a list for PYTHONPATH, site.addsitedir()'''
        return self._pythonpath
    # ---------------------------------------------------------------------

    # --method-set---------------------------------------------------------
    @classmethod
    def get_classname(cls):
        '''! gets the name of the class type
        @ return: cls.__name__
        '''
        return cls.__name__

    def add_code_path(self,
                      path: Path,
                      path_list: list,
                      sys_envar: str = 'PATH',
                      local_envar: str = ENVAR_DCCSI_SYS_PATH,
                      addsitedir: bool = False) -> Path:
        '''! sets up a sys path
        @param path: the path to add
        @ return: returns the path
        '''
        path = Path(path).resolve()  # ensure Path

        # store and track here for settings export
        path_list.append(path)  # keep path objects

        # set it on the PATH
        self.add_path_to_envar(path, sys_envar)

        # set in our managed setting
        self.add_path_to_envar(path, local_envar, True)

        if addsitedir:
            site.addsitedir(path.as_posix())

        return path

    # ---------------------------------------------------------------------
    def set_envar(self, key: str, value: str):
        '''! sets the envar
        @param key: the enavr key as a string
        @param value: the enavr value as a string
        @return: the os.getenv(key)
        Path type objects are specially handled.'''
        # standard environment
        if isinstance(value, Path):
            os.environ[key] = value.as_posix()
        else:
            os.environ[key] = str(value)
        return os.getenv(key)

    # ---------------------------------------------------------------------
    def add_setting(self,
                    key: str,
                    value: Union[int, str, bool, list, dict, Path],
                    set_dynamic: bool = True,
                    check_envar: bool = True,
                    set_envar: bool = False,
                    set_sys_path: bool = False,
                    set_pythonpath: bool = False):
        '''! adds a settings with various configurable options

        @param key: the key (str) for the setting/envar

        @param value: the stored value for the setting

        @param set_dynamic: makes this a dynamic setting (dynaconf)
            dynamic settings will use the prefix_ to specify membership
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
        '''

        if isinstance(value, Path) or set_sys_path or set_pythonpath:
            value = Path(value).resolve()

        if check_envar:
            if isinstance(value, bool):
                # this checks and returns value as bool
                value = env_bool(key, value)
            else:
                # if the envar is set, it will override the input value!
                value = os.getenv(key, value)

        # these managed lists are handled when settings are retreived
        if set_sys_path:
            value = self.add_code_path(value,
                                       self.sys_path,
                                       sys_envar='PATH',
                                       local_envar=ENVAR_DCCSI_SYS_PATH)
            value = os.getenv(ENVAR_DCCSI_SYS_PATH)

        if set_pythonpath:
            value = self.add_code_path(value,
                                       self.pythonpath,
                                       sys_envar='PYTHONPATH',
                                       local_envar=ENVAR_DCCSI_PYTHONPATH)

            value = os.getenv(ENVAR_DCCSI_PYTHONPATH)

        if set_envar:
            value = self.set_envar(key, value)

        if set_dynamic:
            dynakey = f'{self._dyna_prefix}_{key}'
            value = self.set_envar(dynakey, value)

        return (key, value)

    # ---------------------------------------------------------------------
    def add_path_to_envar(self,
                          path: Path,
                          envar: str = 'PATH',
                          suppress: bool = False):
        '''! add path to a path-type envar like PATH or PYTHONPATH

        @param path: path to add to the envar
        @param envar: envar key
        @param add_sitedir: (optional) add site code access
        @return: return os.environ[envar]'''

        # ensure and resolve path object
        path = Path(path).resolve()

        # get or default to empty
        if envar in os.environ:
            _ENVAR = os.getenv(envar, "< NOT SET >")
        else:
            os.environ[envar] = ''  # create empty
            _ENVAR = os.getenv(envar)

        # this is separated (split) into a list
        known_pathlist = _ENVAR.split(os.pathsep)

        if not path.exists() and not suppress:
            _LOGGER.debug(f'envar:{envar}, adding path: {path}')
            _LOGGER.warning(f'path.exists()={path.exists()}, path: {path.as_posix()}')

        if (path.as_posix() not in known_pathlist):
            os.environ[envar] = path.as_posix() + os.pathsep + os.environ[envar]
            # adding directly to sys.path apparently doesn't work for
            # .dll locations like Qt this pattern by extending the ENVAR
            # seems to work correctly which why this pattern is used.

        return os.environ[envar]

    # -----------------------------------------------------------------
    def add_pathlist_to_envar(self,
                              path_list: list,
                              envar: str = 'PATH',
                              suppress: bool = True):
        """!
        Take in a list of Path objects to add to system ENVAR (like PATH).
        This method explicitly adds the paths to the system ENVAR.
        @param path_list: a list() of paths
        @param envar: add paths to this ENVAR
        @ return: os.environ[envar]
        """

        if len(path_list) == 0:
            _LOGGER.warning('No {} paths added, path_list is empty'.format(envar))
            return None
        else:
            for path in path_list:
                path = Path(path).resolve()
                self.add_path_to_envar(path, envar, suppress)

            return os.environ[envar]

    # -----------------------------------------------------------------
    def export_settings(self,
                        settings_filepath: Path = _default_settings_filepath,
                        prefix: str = DCCSI_DYNAMIC_PREFIX,
                        set_env: bool = True,
                        use_dynabox: bool = False,
                        env: bool = False,
                        merge: bool = False,
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

        @param prefix: the prefix str for dynamic settings

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
        # It would be an improvement to add malformed json validation
        # and pre-check the settings.local.json to report warnings
        # this can easily cause a bad error that is deep and hard to debug

        # all ConfigClass objects that export settings, will generate this
        # value. When inspecting settings we will always know that local
        # settings have been aggregated and include local settings.
        self.add_setting('DCCSI_LOCAL_SETTINGS', True)

        self.add_pathlist_to_envar(self.sys_path,
                                   f'{prefix}_{ENVAR_DCCSI_SYS_PATH}',
                                   True)

        self.add_pathlist_to_envar(self.pythonpath,
                                   f'{prefix}_{ENVAR_DCCSI_PYTHONPATH}',
                                   True)

        # update settings
        if set_env:
            self.settings.setenv()

        if not use_dynabox:  # this writes a prettier file with indents
            _settings_box = Box(self.settings.as_dict())

            # writes settings box
            _settings_box.to_json(filename=settings_filepath.as_posix(),
                                  sort_keys=True,
                                  indent=4)

            if log_settings:
                _LOGGER.info(f'Pretty print, settings: {settings_filepath.as_posix()}')
                _LOGGER.info(str(_settings_box.to_json(sort_keys=True,
                                                       indent=4)))
        else:
            from dynaconf import loaders
            from dynaconf.utils.boxing import DynaBox

            _settings_box = DynaBox(self.settings).to_dict()

            if not env:
                # writes to a file, the format is inferred by extension
                # can be .yaml, .toml, .ini, .json, .py
                loaders.write(settings_filepath.as_posix(), _settings_box, merge)
                return _settings_box

            elif env:
                # The env can also be written, though this isn't yet utilized by config.py
                # A suggested improvement is that we should investigate setting up
                # each DCC tool as it's own env group?
                loaders.write(settings_filepath.as_posix(), _settings_box, merge, env)
                return _settings_box

        return self.settings
    # ---------------------------------------------------------------------
# --- END -----------------------------------------------------------------


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

    # you can aggregate a list for PATH (self.sys_path) this way
    # same with PYTHONPATH
    new_list = list()
    new_list.append('c:/foo')
    new_list.append('c:/fooey')

    # lets pass that list to set out insternal sys_path storage setter
    _foo_test.sys_path = new_list
    # that will also set up the dynamic setting
    _LOGGER.info(f'_foo_test.DCCSI_SYS_PATH = {_foo_test.settings.DCCSI_SYS_PATH}')

    # or you could add them individually like this
    # add some paths to test special tracking
    _foo_test.pythonpath.append(Path('c:/looey').resolve())

    # this just returns the internal list that stores them
    _LOGGER.info(f'_foo_test.pythonpath = {_foo_test.pythonpath}')

    # only pre-existing paths on the raw envar are returned here
    _LOGGER.info(f"PYTHONPATH: {os.getenv('PYTHONPATH')}")

    # although verbose, you can better set those path types like this
    _foo_test.add_setting('KABLOOEY_PATH',
                          Path('c:/kablooey'),
                          set_dynamic = True,
                          check_envar = True,
                          set_envar = True,
                          set_pythonpath = True)

    # With that call, the path will immediately be set on the envar
    _LOGGER.info(f"PYTHONPATH: {os.getenv('PYTHONPATH')}")

    # add a envar bool
    _bool_test = _foo_test.add_setting('FOO_IS', True)

    if _bool_test:
        _LOGGER.info(f'The boolean is: {_bool_test}')
        _LOGGER.info(f'FOO_IS: {_foo_test.settings.FOO_IS}')

    # add a envar int
    _foo_test.add_setting('foo_level', 10)

    # you can pass in lower, but must retreive UPPER from settings
    _LOGGER.info(f'foo_level: {_foo_test.settings.FOO_LEVEL}')

    # add a envar dict
    _foo_test.add_setting('FOO_DICT', {'fooey': 'Is fun!'})

    # add an average envar path
    _foo_test.add_setting('BAR_PATH', Path('c:/bar'))

    _LOGGER.info(f'foo_dict: {_foo_test.settings.FOO_DICT}')

    _LOGGER.info(f'settings: {_foo_test.settings}')

    _export_filepath = Path(_PATH_DCCSIG, '__tmp__', 'settings_export.json').resolve()

    _foo_test.export_settings(settings_filepath = _export_filepath,
                              log_settings = True)
