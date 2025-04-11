"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

LySettings provides an API for modifying settings files and creating/restoring backups of them.
"""

import fileinput
import json
import logging
import re
import os

import ly_test_tools.environment.file_system
import ly_test_tools._internal.exceptions as exceptions

logger = logging.getLogger(__name__)


class LySettings(object):
    """
    LySettings provides an API for modifying settings files and creating/restoring backups of them.
    """

    def __init__(self, temp_path, resource_locator):
        self._temp_path = temp_path
        self._resource_locator = resource_locator

    def get_temp_path(self):
        return self._temp_path

    def backup_asset_processor_settings(self, backup_path=None):
        self._backup_settings(self._resource_locator.asset_processor_config_file(), backup_path)


    def restore_asset_processor_settings(self, backup_path):
        self._restore_settings(self._resource_locator.asset_processor_config_file(), backup_path)

    def backup_json_settings(self, json_settings_file, backup_path=None):
        """
        Creates a backup of a Json/Registry Settings file in the backup_path. If no path is provided, it will store in
        the workspace temp path (the contents of the workspace temp directory are removed during workspace teardown)
        :param json_settings_file:
        :param backup_path:
        :return:
        """
        self._backup_settings(json_settings_file, backup_path)

    def restore_json_settings(self, json_settings_file, backup_path=None):
        """
        Restores a Json/Registry settings file from its backup.
        The backup is stored in the backup_path.
        If no backup_path is provided, it will attempt to retrieve the backup from the workspace temp path.
        """
        self._restore_settings(json_settings_file, backup_path)

    def _backup_settings(self, settings_file, backup_path):
        """
        Creates a backup of the settings file in the backup_path. If no path is
        provided, it will store in the workspace temp path (the contents of the workspace temp directory are removed
        during workspace teardown)
        """
        if not backup_path:
            backup_path = self._temp_path
        ly_test_tools.environment.file_system.create_backup(settings_file, backup_path)

    def _restore_settings(self, settings_file, backup_path):
        """
        Restores the settings file from its backup stored in backup_path. If no path is provided, it will attempt
        to retrieve the backup from the workspace temp path.
        """
        if not backup_path:
            backup_path = self._temp_path
        ly_test_tools.environment.file_system.restore_backup(settings_file, backup_path)


class JsonSettings(object):
    """
    JsonSettings provides an API for reading, modifying and writing Json settings files.
    Can be used as a Context Manager.
    """

    def __init__(self, file_path, line_clean_func=None):
        self._data = None
        self._file_path = file_path
        self._line_clean_func = line_clean_func
        self._modified = False
        self._path_separator = '/'

    def __enter__(self):
        self.load()
        return self

    def __exit__(self, exception_type, exception_value, traceback):
        if self._modified:
            self.dump()

    def _get_path_tokens(self, key_path):
        """
        Splits the path using the separator and returns a list of tokens.
        The first empty token is skipped, since it points to the root of the document.
        :param key_path: Path to the key
        :return: List of path tokens
        """
        path_tokens = key_path.split(self._path_separator)
        # Skip first empty token, it points to the root of the document
        path_tokens = path_tokens[1:]
        return path_tokens

    def load(self):
        """
        Load the Json file into memory
        :return: None
        """
        with open(self._file_path, 'r') as contents:
            if not self._line_clean_func:
                self._data = json.load(contents)
            else:
                cleaned_lines = []
                for line in contents:
                    clean_line = self._line_clean_func(line)
                    cleaned_lines.append(clean_line)

                self._data = json.loads(cleaned_lines)

    def dump(self):
        """
        Write the content into the Json file
        :return: None
        """
        with open(self._file_path, 'w') as out_file:
            json.dump(self._data, out_file, indent=4)

    def get_key(self, key_path, default_value=None):
        """
        Retrieve a key by path from the Json
        :param key_path: Path to the key
        :param default_value: Default value to return if the key is not found
        :return: Value of the key or default_value if not found
        """
        path_tokens = self._get_path_tokens(key_path)
        obj = self._data
        try:
            for token in path_tokens:
                obj = obj[token]
            return obj
        except KeyError as err:
            logger.error(f'KeyError when retrieving {key_path} from file {self._file_path}: {err}')
            return default_value

    def set_key(self, key_path, value):
        """
        Set a key to a value
        :param key_path: Path to the key
        :param value: The value
        :return: None
        """
        path_tokens = self._get_path_tokens(key_path)
        obj = self._data
        try:
            for token in path_tokens[0:-1]:
                obj = obj.setdefault(token, {})
            obj[path_tokens[-1]] = value
            self._modified = True
        except KeyError as err:
            logger.error(f'KeyError when setting {key_path} from file {self._file_path}: {err}')

    def remove_key(self, key_path):
        """
        Remove a key by path from the Json
        :param key_path: Path to the key
        :return: None
        """
        path_tokens = self._get_path_tokens(key_path)
        obj = self._data
        parent = None
        key = None
        try:
            for token in path_tokens:
                key = token
                parent = obj
                obj = obj[token]

            if parent:
                del parent[key]
                self._modified = True
            else:
                logger.warning(f'Could not remove key {key} from file {self._file_path}')
        except KeyError as err:
            logger.error(f'KeyError when removing {key_path} from file {self._file_path}: {err}')


class RegistrySettings(JsonSettings):
    """
    JsonSettings provides an API for reading, modifying and writing Registry settings files.
    Files must be Json-compatible with optional line comments starting with '//'.
    """
    def __init__(self, file_path):
        super(RegistrySettings, self).__init__(file_path, RegistrySettings._clean_line)

    @staticmethod
    def _clean_line(line):
        comment_index = line.lstrip.startswith('//')
        return line if comment_index == -1 else line[:comment_index]


def _edit_text_settings_file(settings_file, setting, value, comment_char=""):
    """
    Find and set a specific setting in a text based settings file.  Uses "setting = value" syntax to identify setting,
    ignoring whitespace.  Will append "setting = value" to a text file if it can not find the setting already.

    Note all found instances of the setting key in the file will be changed.
    Unintentional setting changes may happen for files with multiple settings named the same

    Using the comment_char, users can set a value on a corresponding setting but leave it commented out
    --setting=value

    :param settings_file: The path to a settings file
    :param setting: The target key setting to update
    :param value: The new key value
    :param comment_char: A character identifier for commenting out data in a settings file
    """

    if not os.path.isfile(settings_file):
        raise exceptions.LyTestToolsFrameworkException(f"Invalid file and/or path {settings_file}.")

    match_obj = None
    document = None
    try:
        # fileinput can be very destructive when used in conjunction with "with as" due to temp file creation.
        # Allows using print to rewrite lines.
        logger.debug(f"Opening {settings_file}")
        document = fileinput.input(settings_file, inplace=True)
        for line in document:
            # Remove whitespace to avoid double spacing the output.
            line = line.rstrip()
            if setting in line:
                # Run regex on each line, and make the change if we have an exact match.
                setting_regex = re.compile("([-;]+)?(.*)=(.*)")
                possible_match = setting_regex.match(line)
                if possible_match is None or setting != possible_match.group(2).strip():
                    print(line)
                    continue
                match_obj = possible_match
                logger.debug(f"Found {setting} in {settings_file}")
                # Print the new value for the setting.
                if value == "":
                    print("")
                else:
                    print(f"{comment_char}{setting}={value}")
                logger.debug(f"Updated setting {setting} in {settings_file} to {value} from {match_obj.group(3)}")
            else:
                print(line)

    except PermissionError as error:
        logger.warning(f"PermissionError originating from LyTT, possibly due to ({settings_file}) already being open. "
                       f"Error: {error}")
    finally:
        if document is not None:
            document.close()

    # Append the settings change if setting doesn't exist in file.
    if match_obj is None:
        logger.debug(
            "Unable to locate setting in file. "
            f"Appending {comment_char}{setting}={value} to {settings_file}.")
        with open(settings_file, "a") as document:
            document.write(f"{comment_char}{setting}={value}")

