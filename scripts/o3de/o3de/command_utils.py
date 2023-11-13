#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import configparser
import logging
import json
import re
import os

from o3de import manifest
from typing import List, Tuple
from pathlib import Path

logger = logging.getLogger('o3de')


class O3DEConfigError(Exception):
    pass


def resolve_project_name_and_path(starting_path: str or None = None) -> (str, Path):
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
    def __init__(self, project_name: str or None, settings_filename: str, settings_section_name: str,
                 default_settings: List[Tuple]):
        """
        Initialize the configuration object

        :param project_name:            Optional. The name of the registered o3de-project if this object is to handle both the global settings and project-specific overrides.
        :param settings_filename:
        :param settings_section_name:
        :param default_settings:
        """

        self.settings_filename = settings_filename
        self.settings_section_name = settings_section_name

        # Always apply and read the global configuration
        self.global_settings_file = self.apply_default_global_settings(settings_filename=settings_filename,
                                                                       settings_section_name=settings_section_name,
                                                                       default_settings=default_settings)
        global_config_reader = configparser.ConfigParser()
        global_config_reader.read(self.global_settings_file.absolute())
        if not global_config_reader.has_section(self.settings_section_name):
            global_config_reader.add_section(self.settings_section_name)
        self.global_settings = global_config_reader[self.settings_section_name]

        if project_name is None:
            # No project name, set to handle only the global configuration
            self.project_settings_file = None
            self.project_settings = None
            self.project_name = None
        else:
            # The project name was specified, locate and read the project settings and use it as overlay values
            self.project_settings_file = O3DEConfig.resolve_project_settings_file(project_name=project_name,
                                                                                  settings_filename=settings_filename,
                                                                                  settings_section_name=settings_section_name,
                                                                                  create_default_if_missing=True)
            self.project_name = project_name
            project_config_reader = configparser.ConfigParser()
            project_config_reader.read(self.project_settings_file.absolute())
            if not project_config_reader.has_section(self.settings_section_name):
                project_config_reader.add_section(self.settings_section_name)
            self.project_settings = project_config_reader[self.settings_section_name]

    @property
    def is_global(self) -> bool:
        """
        Determine if this object represents only global settings or both project and global settings
        :return: bool: True if only the global settings are managed, false if not
        """
        return self.project_settings_file is None

    @staticmethod
    def apply_default_global_settings(settings_filename: str,
                                      settings_section_name: str,
                                      default_settings: List[Tuple]) -> Path:
        """
        Make sure that the global settings file exists and is populated with the default settings if they are missing

        :param settings_filename:       The name of the settings file (file name only) to use to locate/create/update the settings file.
        :param settings_section_name:   The settings file section name to create (if needed) for the default settings
        :param default_settings:        The list of tuples (key,value) to populate the default values if needed
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
        for config_key, default_value in default_settings:
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

    @staticmethod
    def resolve_project_settings_file(project_name: str or None,
                                      settings_filename: str,
                                      settings_section_name: str,
                                      create_default_if_missing: bool = False) -> Path:
        """
        Resolve the location of the configuration file. The file <settings_filename> will be stored
        in the root location of the registered project <project_name_arg>

        :param project_name:                The name of the registered project to look up its root path
        :param settings_filename:           The name of the settings file to use (or create)
        :param settings_section_name:       The name of the settings section to use if a new settings file needs to be created
        :param create_default_if_missing:   Flag to request that the settings file be automatically create if missing
        :return: The path to the project-specific settings file
        """
        # Resolve the android settings file
        if project_name:
            project_path = manifest.get_registered(project_name=project_name)
            if not project_path:
                raise O3DEConfigError(f"Unable to resolve project named '{project_name}'. "
                                      f"Make sure it is registered with O3DE.")
            logger.info(f"Configuring settings for project '{project_name}' at '{project_path}'")
            settings_path = Path(project_path) / settings_filename
        else:
            project_name, project_path = resolve_project_name_and_path()
            logger.info(f"Configuring settings for project '{project_name}' at '{project_path}'")
            settings_path = Path(project_path) / settings_filename

        if create_default_if_missing and settings_path and not settings_path.is_file():
            settings_path.write_text(f"[{settings_section_name}]\n")

        return settings_path

    def set_config_value(self, key: str, value: str) -> str:
        """
        Apply a settings value to the configuration. If there is a project overlay configured, then only apply the value
        to the project override. Only apply the value globally if this object is not managing an overlay setting

        :param key:     The key of the entry to set or add.
        :param value:   The value of the entry to set or add.
        :return: The previous value if the key if it is being overwritten, or None if this is a new key+value
        """
        if not key:
            raise O3DEConfigError("Missing 'key' argument to set a config value")

        if self.is_global:
            settings = self.global_settings
            settings_file = self.global_settings_file
        else:
            settings = self.project_settings
            settings_file = self.project_settings_file

        # Read the settings and apply the change if necessary
        project_config = configparser.ConfigParser()
        project_config.read(settings_file.absolute())
        if not project_config.has_section(self.settings_section_name):
            project_config.add_section(self.settings_section_name)
        project_config_section = project_config[self.settings_section_name]
        current_value = project_config_section.get(key, None)
        if current_value != value:
            try:
                settings[key] = value
                project_config_section[key] = value
            except ValueError as e:
                raise O3DEConfigError(f"Invalid settings value for key '{key}': {e}")
            with settings_file.open('w') as settings_file:
                project_config.write(settings_file)

        return current_value

    REGEX_NAME_AND_VALUE_MATCH_UNQUOTED = re.compile(r'([\w]+)[\s]*=[\s]*((.*))')
    REGEX_NAME_AND_VALUE_MATCH_SINGLE_QUOTED = re.compile(r"([\w]+)[\s]*=[\s]*('(.*)')")
    REGEX_NAME_AND_VALUE_MATCH_DOUBLE_QUOTED = re.compile(r'([\w]+)[\s]*=[\s]*("(.*)")')

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
        if self.project_settings:
            value = self.project_settings.get(key, None)
            if value:
                return value, self.project_name

        value = self.global_settings.get(key, None)
        return value, ""

    def get_value(self, key: str) -> str:
        """
        Get the value of a particular setting based on a key.
        :param key: The key to look up the value
        :return: The value of the settings based on the key. If the key is not found, return None
        """
        return self.get_value_and_source(key)[0]

    def get_all_values(self) -> List[Tuple]:
        """
        Get all the values that this configuration object represents. The values are stored as a list of tuples of
        the key, value, and project source (if any, otherwise it represents a global setting)

        :return: List of tuples of the key, value, and project source
        """

        all_settings_map = {}
        for key, value in self.global_settings.items():
            all_settings_map[key] = value
        if not self.is_global:
            for key, value in self.project_settings.items():
                all_settings_map[key] = value

        all_settings_list = []
        for key, value in all_settings_map.items():
            if not self.is_global:
                all_settings_list.append((key, value, self.project_name if key in self.project_settings else ''))
            else:
                all_settings_list.append((key, value, ''))

        return all_settings_list
