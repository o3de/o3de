#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import argparse
import configparser
import logging
import json
import re
import os

from getpass import getpass
from o3de import manifest
from typing import List, Tuple
from pathlib import Path

logger = logging.getLogger('o3de')


class O3DEConfigError(Exception):
    pass

GENERAL_BOOLEAN_REGEX = '(t|f|true|false|0|1|on|off|yes|no)'

def evaluate_boolean_from_setting(input: str, default: bool = None) -> bool:

    if input is None:
        return default

    if not re.match(GENERAL_BOOLEAN_REGEX, input, re.IGNORECASE):
        raise O3DEConfigError(f"Invalid boolean value {input}. Must match '{GENERAL_BOOLEAN_REGEX}'")

    lower_input = input.lower()
    match lower_input:
        case 't' | 'true' | '1' | 'on' | 'yes':
            return True
        case 'f' | 'false' | '0' | 'off' | 'no':
            return False
        case _:
            raise O3DEConfigError(f"Invalid boolean value {input}. Must match '{GENERAL_BOOLEAN_REGEX}'")

class SettingsDescription(object):

    def __init__(self, key: str, description: str, default: str= None,  is_password:bool = False, is_boolean = False, restricted_regex: str = None, restricted_regex_description: str = None):

        self._key = key
        self._description = description
        self._default = default
        self._is_password = is_password
        self._is_boolean = is_boolean
        self._restricted_regex = re.compile(restricted_regex, re.IGNORECASE) if restricted_regex is not None else None
        self._restricted_regex_description = restricted_regex_description

        assert not (is_boolean and is_password), 'Only is_boolean or is_password is allowed'
        assert (restricted_regex and not (is_boolean and is_password)) or (not restricted_regex), 'restricted_regex cannot be set with either is_boolean or is_password'
        assert (restricted_regex and restricted_regex_description) or not (restricted_regex or restricted_regex_description), 'If restricted_regex is set, then restricted_regex_description must be set as well'


    def __str__(self):
        return self._key

    @property
    def key(self) -> str:
        return self._key

    @property
    def description(self) -> str:
        return self._description

    @property
    def default(self):
        return self._default

    @property
    def is_password(self):
        return self._is_password

    @property
    def is_boolean(self):
        return self._is_boolean

    def validate_value(self, input):
        if self._is_password:
            logger.debug(f"Input value for '{self._key}' is a password. If extra security is required, use the --set-password setting argument.")
        if self._is_boolean:
            evaluate_boolean_from_setting(input)
        if self._restricted_regex and not self._restricted_regex.match(input):
            raise O3DEConfigError(f"Input value '{input}' not valid. {self._restricted_regex_description}")


def resolve_project_name_and_path(starting_path: Path or None = None) -> (str, Path):
    """
    Attempt to resolve the project name and path attempting to find the first 'project.json' that can be discovered based on the 'starting_path'

    :param starting_path:   The starting path to start to search for project.json project marker. If `None`, then the starting path will be the current working directory
    :return: The tuple of project name and its full path
    """
    def _get_project_name(input_project_json_path: Path):
        # Make sure that the project defined with project.json is a valid o3de project and that it is registered properly
        with project_json_path.open(mode='r') as json_data_file:
            try:
                json_data = json.load(json_data_file)
            except json.JSONDecodeError as e:
                raise O3DEConfigError(f"Invalid O3DE project at {project_path}: {e}")
            project_name = json_data.get('project_name', None)
            if not project_name:
                raise O3DEConfigError(f"Invalid O3DE project at {project_path}: Invalid O3DE project json file")
            return project_name

    # Walk up the path util we find a valid 'project.json'
    current_working_dir = Path(starting_path) if starting_path is not None else Path(os.getcwd())
    project_json_path = current_working_dir / 'project.json'
    while current_working_dir != current_working_dir.parent and not project_json_path.is_file():
        project_json_path = current_working_dir / 'project.json'
        current_working_dir = current_working_dir.parent
    if not project_json_path.is_file():
        raise O3DEConfigError(f"Unable to locate a 'project.json' file based on directory {current_working_dir}")

    # Extract the project name from resolved project.json file and use it to look up a registered project by its name
    project_path = project_json_path.parent
    resolved_project_name = _get_project_name(project_json_path)
    resolved_project_path = manifest.get_registered(project_name=resolved_project_name)
    if not resolved_project_path:
        raise O3DEConfigError(f"Project '{resolved_project_name}' found in {project_json_path} is not registered with O3DE.")

    return resolved_project_name, resolved_project_path


class O3DEConfig(object):
    """
    This class manages settings for o3de command line tools which are serialized globally, but can be overlayed with
    values for specified registered projects.
    """
    def __init__(self, project_path: Path or None, settings_filename: str, settings_section_name: str,
                 settings_description_list: List[SettingsDescription]):
        """
        Initialize the configuration object

        :param project_path:                Optional. Path to the project root where the project-specific overlay settings will reside. If `None`, then this is a global only configuration with no project-specific override
        :param settings_filename:           The filename for the setting
        :param settings_section_name:       The section name that this settings object manages for the settings file
        :param settings_description_list:   The list of supported setting descriptions
        """

        self._settings_filename = settings_filename
        self._settings_section_name = settings_section_name

        # Construct a map to the settings by its key
        self._settings_description_map = {}
        for setting in settings_description_list:
            assert setting.key not in self._settings_description_map, f"Duplicate settings key '{setting.key}' detected"
            self._settings_description_map[setting.key] = setting

        # Always apply and read the global configuration
        self._global_settings_file = O3DEConfig.apply_default_global_settings(settings_filename=settings_filename,
                                                                              settings_section_name=settings_section_name,
                                                                              settings_descriptions=settings_description_list)
        global_config_reader = configparser.ConfigParser()
        global_config_reader.read(self._global_settings_file.absolute())
        if not global_config_reader.has_section(self._settings_section_name):
            global_config_reader.add_section(self._settings_section_name)
        self._global_settings = global_config_reader[self._settings_section_name]

        if project_path is None:
            # No project name, set to handle only the global configuration
            self._project_settings_file = None
            self._project_settings = None
            self._project_name = None
        else:
            self._project_name, project_path = resolve_project_name_and_path(project_path)
            self._project_settings_file = project_path / settings_filename
            if self._project_settings_file and not self._project_settings_file.is_file():
                self._project_settings_file.write_text(f"[{settings_section_name}]\n")

            project_config_reader = configparser.ConfigParser()
            project_config_reader.read(self._project_settings_file.absolute())
            if not project_config_reader.has_section(self._settings_section_name):
                project_config_reader.add_section(self._settings_section_name)
            self._project_settings = project_config_reader[self._settings_section_name]

    @property
    def is_global(self) -> bool:
        """
        Determine if this object represents only global settings or both project and global settings
        :return: bool: True if only the global settings are managed, false if not
        """
        return self._project_settings_file is None

    @property
    def project_name(self) -> str or None:
        return self._project_name

    @property
    def setting_descriptions(self) -> list:
        return [setting for _, setting in self._settings_description_map.items()]

    @staticmethod
    def apply_default_global_settings(settings_filename: str,
                                      settings_section_name: str,
                                      settings_descriptions: List[SettingsDescription]) -> Path:
        """
        Make sure that the global settings file exists and is populated with the default settings if they are missing

        :param settings_filename:       The name of the settings file (file name only) to use to locate/create/update the settings file.
        :param settings_section_name:   The settings file section name to create (if needed) for the default settings
        :return: The path to the global settings file
        """

        # Make sure that we have a global .o3de folder
        o3de_folder = manifest.get_o3de_folder()
        if not o3de_folder.is_dir():
            raise O3DEConfigError('The .o3de is not registered yet. Make sure to register the engine first.')

        # Make sure a global settings file exists
        global_settings = manifest.get_o3de_folder() / settings_filename
        if not global_settings.is_file():
            # If not create a new one with a single section
            global_settings.write_text(f"[{settings_section_name}]")

        # Read the global settings file and apply the defaults
        global_config = configparser.ConfigParser()
        global_config.read(global_settings.absolute())
        if not global_config.has_section(settings_section_name):
            global_config.add_section(settings_section_name)
        global_section = global_config[settings_section_name]
        modified = False

        for setting in settings_descriptions:

            config_key = setting.key
            default_value = setting.default

            # Only apply default values if it is not None
            if default_value is None:
                continue

            # Only add default values to keys that don't exist. We don't want to overwrite any existing value
            if config_key not in global_section:
                global_section[config_key] = default_value
                modified = True

        # Write back to the settings file only if there was a modification
        if modified:
            with global_settings.open('w') as global_settings_file:
                global_config.write(global_settings_file)
            logger.debug(f"Missing default values applied to {global_settings}")

        return global_settings

    def set_config_value(self, key: str, value: str, validate_value: bool = True, show_log: bool = True) -> str:
        """
        Apply a settings value to the configuration. If there is a project overlay configured, then only apply the value
        to the project override. Only apply the value globally if this object is not managing an overlay setting

        :param key:             The key of the entry to set or add.
        :param value:           The value of the entry to set or add.
        :param validate_value:  Option to validate the value validity against the key
        :param show_log:        Option to show logging information
        :return: The previous value if the key if it is being overwritten, or None if this is a new key+value
        """

        # Validate the setting key and its value
        if not key:
            raise O3DEConfigError("Missing 'key' argument to set a config value")
        settings_description = self._settings_description_map.get(key, None)
        if not settings_description:
            raise O3DEConfigError(f"Unrecognized setting '{key}'")
        if validate_value:
            settings_description.validate_value(value)

        is_clear_operation = len(value) == 0

        if self.is_global:
            # Only apply the setting to the global map
            current_settings = self._global_settings
            current_settings_file = self._global_settings_file
        else:
            # Apply the setting locally
            current_settings = self._project_settings
            current_settings_file = self._project_settings_file

        # Read the settings and apply the change if necessary
        project_config = configparser.ConfigParser()
        project_config.read(current_settings_file.absolute())
        if not project_config.has_section(self._settings_section_name):
            project_config.add_section(self._settings_section_name)
        project_config_section = project_config[self._settings_section_name]
        current_value = project_config_section.get(key, None)
        if current_value != value:
            try:
                current_settings[key] = value
                project_config_section[key] = value
            except ValueError as e:
                raise O3DEConfigError(f"Invalid settings value for setting '{key}': {e}")
            with current_settings_file.open('w') as current_settings_file:
                project_config.write(current_settings_file)
            if show_log:
                logger.info(f"Setting for {key} cleared.")
        elif is_clear_operation and not self.is_global and len(self._global_settings.get(key)) > 0:
            # If the was a clear value request, but the key is only set globally and the global flag was not applied,
            # then present a warning
            if show_log:
                logger.warning(f"Operation skipped. The settings value for {key} was requested to be cleared locally, but is only "
                               "set globally. Run this request again but with the global flag specified.")

        return current_value

    REGEX_NAME_AND_VALUE_MATCH_UNQUOTED = re.compile(r'(\w[\d\w\.]+)[\s]*=[\s]*((.*))')
    REGEX_NAME_AND_VALUE_MATCH_SINGLE_QUOTED = re.compile(r"(\w[\d\w\.]+)[\s]*=[\s]*('(.*)')")
    REGEX_NAME_AND_VALUE_MATCH_DOUBLE_QUOTED = re.compile(r'(\w[\d\w\.]+)[\s]*=[\s]*("(.*)")')

    def set_config_value_from_expression(self, key_and_value: str) -> str:
        """
        Apply a settings value to the configuration based on a '<key>=<value>' expression.

        The follow formats are recognized:
             key=value
             key='value'
             key="value"

        If there is a project overlay configured, then only apply the value to the project override. Only apply the
        value globally if this object is not managing an overlay setting.

        :param key_and_value:     The '<key>=<value>' expression to apply to the settings
        :return: The previous value if the key if it is being overwritten, or None if this is a new key+value
        """
        match = O3DEConfig.REGEX_NAME_AND_VALUE_MATCH_DOUBLE_QUOTED.match(key_and_value)
        if not match:
            match = O3DEConfig.REGEX_NAME_AND_VALUE_MATCH_SINGLE_QUOTED.match(key_and_value)
        if not match:
            match = O3DEConfig.REGEX_NAME_AND_VALUE_MATCH_UNQUOTED.match(key_and_value)
        if not match:
            raise O3DEConfigError(f"Invalid setting key argument: {key_and_value}")

        key = match.group(1)
        value = match.group(3)

        return self.set_config_value(key, value)

    def get_value_and_source(self, key: str) -> (str, str):
        """
        Get a tuple of a particular settings value and its project source based on a key. If the project source is empty, then
        it represents a global setting.

        :param key: The key to look up the value
        :return: Tuple (str,str) that represents (value, project_name). If project_name is empty, then the value is global for all projects
        """
        settings_description = self._settings_description_map.get(key, None)
        if not settings_description:
            raise O3DEConfigError(f"Unrecognized setting '{key}'")

        if self._project_settings:
            value = self._project_settings.get(key, None)
            if value:
                return value, self._project_name

        value = self._global_settings.get(key, None)
        return value, ""

    def get_value(self, key: str, default: str = None) -> str:
        """
        Get the value of a particular setting based on a key.
        :param key: The key to look up the value
        :param default: The default value to return if the key is not set
        :return: The value of the settings based on the key. If the key is not found, return the default
        """
        settings_description = self._settings_description_map.get(key, None)
        if not settings_description:
            raise O3DEConfigError(f"Unrecognized setting '{key}'")
        result = self.get_value_and_source(key)[0]
        return result or default

    def get_boolean_value(self, key: str, default: bool = False) -> bool:

        settings_description = self._settings_description_map.get(key, None)
        if not settings_description:
            raise O3DEConfigError(f"Unrecognized setting '{key}'")
        if not settings_description.is_boolean:
            raise O3DEConfigError(f"Setting '{key}' is not a boolean value.")

        str_value = self.get_value(key, str(default))
        return evaluate_boolean_from_setting(str_value)

    def get_all_values(self) -> List[Tuple]:
        """
        Get all the values that this configuration object represents. The values are stored as a list of tuples of
        the key, value, and project source (if any, otherwise it represents a global setting)

        :return: List of tuples of the key, value, and project source
        """

        all_settings_map = {}
        for key, value in self._global_settings.items():
            all_settings_map[key] = value
        if not self.is_global:
            for key, value in self._project_settings.items():
                if value:
                    all_settings_map[key] = value

        all_settings_list = []
        for key, value in all_settings_map.items():
            if not self.is_global:
                all_settings_list.append((key, value, self._project_name if self._project_settings.get(key) else ''))
            else:
                all_settings_list.append((key, value, ''))

        return all_settings_list

    def set_password(self, key: str) -> None:
        """
        Set a password for a password-specified key 
        :param key:     The key to the password setting to set
        """
        
        settings_description = self._settings_description_map.get(key, None)
        if not settings_description:
            raise O3DEConfigError(f"Unrecognized setting '{key}'")
        if not settings_description.is_password:
            raise O3DEConfigError(f"Setting '{key}' is not a password setting.")

        input_password = getpass(f'Please enter the password for {key}: ')
        if not input_password:
            raise O3DEConfigError(f"Invalid empty password")

        verify_password = getpass(f'Please verify the password for {key}: ')
        if input_password != verify_password:
            raise O3DEConfigError(f"Passwords do not match.")
    
        # Set the password bypassing the validity check
        self.set_config_value(key=key,
                              value=input_password,
                              validate_value=False,
                              show_log=False)

        logger.info(f"Password set for {key}.")

    def add_boolean_argument(self, parser: argparse.ArgumentParser, key: str, enable_override_arg: str or List, enable_override_desc: str, disable_override_arg: str or List, disable_override_desc: str) -> None:
        """
        Add a boolean argument to a parser to present options to negate the default value represented by a command settings key
        :param parser:                  The arg parser to add the argument to
        :param key:                     The command settings key to get the default boolean value from
        :param enable_override_arg:     The argument parameter name(s) to set if the default value represented by the key defaults to false
        :param enable_override_desc:    The help description to show for the enable arg
        :param disable_override_arg:    The argument parameter name(s) to set if the default value represented by the key defaults to true
        :param disable_override_desc:   The help description to show for the disable arg
        """
        default_option = self.get_boolean_value(key)
        if default_option:
            # The default is true, add the argument to enable the option to set to override to false
            if isinstance(enable_override_arg, str):
                parser.add_argument(disable_override_arg, action='store_true', help=disable_override_desc)
            elif isinstance(enable_override_arg, List):
                parser.add_argument(*disable_override_arg, action='store_true', help=disable_override_desc)
            else:
                raise O3DEConfigError("parameter 'disable_override_arg' must be either a string or list of string")
        else:
            # The default is false, add the argument to enable the option to set to override to true
            if isinstance(enable_override_arg, str):
                parser.add_argument(enable_override_arg, action='store_true', help=enable_override_desc)
            elif isinstance(enable_override_arg, List):
                parser.add_argument(*enable_override_arg, action='store_true', help=enable_override_desc)
            else:
                raise O3DEConfigError("parameter 'enable_override_arg' must be either a string or list of string")

    def get_parsed_boolean_option(self, parsed_args, key: str, enable_attribute: str, disable_attribute: str) -> bool:
        """
        Get the boolean value from parsed args based on whether an argument override was enabled or not, otherwise
        use the default value from the command settings key
        :param parsed_args:         The parsed arguments to look for the overridden enable or disable attribute
        :param key:                 The key to look up the default value
        :param enable_attribute:    The enable attribute to search for to see if the override to enable was passed in
        :param disable_attribute:   The disable attribute to search for to see if the override to enable was passed in
        :return: The final boolean result
        """
        default_value = self.get_boolean_value(key)
        if getattr(parsed_args, enable_attribute, False):
            option = True
        elif getattr(parsed_args, disable_attribute, False):
            option = False
        else:
            option = default_value
        return option

    def add_multi_part_argument(self, parser: argparse.ArgumentParser, argument: str or List, key: str, dest: str, description: str, is_path_type: bool) -> None:
        """
        Add an argument that allows multi values and provide any defaults (separated by a semi-colon) as the default values if any

        :param argument:        The argument name(s) for the argument
        :param parser:          The parser to add the argument to
        :param key:             The key to look up the default value
        :param dest:            The destination attribute that this argument represents
        :param description:     The argument help description
        :param is_path_type:    Whether this is a list or paths or string
        """
        default_str = self.get_value(key, "")
        if is_path_type:
            default_values = [Path(default) for default in default_str.split(';') if default]
            if isinstance(argument, str):
                parser.add_argument(argument,
                                    type=Path, dest=dest, action='append',
                                    help=description, default=default_values)
            elif isinstance(argument, List):
                parser.add_argument(*argument,
                                    type=Path, dest=dest, action='append',
                                    help=description, default=default_values)
            else:
                raise O3DEConfigError("parameter 'argument' must be either a string or list of string")

        else:
            default_values = [default for default in default_str.split(';') if default]
            if isinstance(argument, str):
                parser.add_argument(argument,
                                    type=str, dest=dest, action='append',
                                    help=description, default=default_values)
            elif isinstance(argument, List):
                parser.add_argument(*argument,
                                    type=str, dest=dest, action='append',
                                    help=description, default=default_values)
            else:
                raise O3DEConfigError("parameter 'argument' must be either a string or list of string")

    def get_settings_description(self, key: str) -> SettingsDescription:
        """
        Get the value of a particular setting based on a key.
        :param key: The key to look up the value
        :param default: The default value to return if the key is not set
        :return: The value of the settings based on the key. If the key is not found, return the default
        """
        settings_description = self._settings_description_map.get(key, None)
        if not settings_description:
            raise O3DEConfigError(f"Unrecognized setting '{key}'")
        return settings_description
    