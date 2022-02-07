"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Reg cleaner: tools for working with the lumberyard windows registry keys
"""
import logging
import os

import ly_test_tools
if ly_test_tools.WINDOWS:
    import winreg  # OS-specific module availability, must be mocked if this file is accessed elsewhere e.g. unit tests
import ly_test_tools.environment.process_utils as process_utils

CONST_LY_REG = r'SOFTWARE\O3DE\O3DE'
AUTOMATION_EXCEPTION_LIST = [
    os.path.join(CONST_LY_REG, r"Identity"),
    os.path.join(CONST_LY_REG, r"Settings\DXInstalled"),
    os.path.join(CONST_LY_REG, r"Settings\EditorSettingsVersion"),
    os.path.join(CONST_LY_REG, r"Settings\EnableSourceControl"),
    os.path.join(CONST_LY_REG, r"Settings\RC_EnableSourceControl"),
]

logger = logging.getLogger(__name__)


def _delete_child_keys_and_values(path, exception_list=None):
    """
    Deletes all of the keys and values under the target registry path

    :param path: the target path
    :param exception_list: list of child keys and values to skip
    :return: True if all of the keys and values were deleted. False, if any of the keys and values were skipped
    """
    if exception_list is None:
        exception_list = []

    is_empty = True

    handle = winreg.OpenKey(winreg.HKEY_CURRENT_USER, path, 0, winreg.KEY_ALL_ACCESS)

    # Delete all of the empty keys under the target path
    # Before checking for emptiness, attempt to empty out each key by calling delete_all_keys_and_values recursively
    keys_to_delete = []
    try:
        index = 0
        while True:
            key_name = winreg.EnumKey(handle, index)
            key_path = os.path.join(path, key_name)
            if key_path not in exception_list and _delete_child_keys_and_values(key_path, exception_list):
                keys_to_delete.append(key_name)
            else:
                is_empty = False
            index += 1
    except WindowsError:
        pass

    for key_name in keys_to_delete:
        winreg.DeleteKey(handle, key_name)

    # Delete all of the values under the target path
    values_to_delete = []
    try:
        index = 0
        while True:
            value_name, _, _ = winreg.EnumValue(handle, index)
            value_path = os.path.join(path, value_name)
            if value_path not in exception_list:
                values_to_delete.append(value_name)
            else:
                is_empty = False
            index += 1
    except WindowsError:
        pass

    for value_name in values_to_delete:
        winreg.DeleteValue(handle, value_name)

    winreg.CloseKey(handle)

    return is_empty


def _delete_key(path, exception_list=None):
    """
    Deletes the key at the target registry path

    :param path: the target path
    :param exception_list: list of child keys and values to skip
    """
    try:
        if _delete_child_keys_and_values(path, exception_list):
            winreg.DeleteKey(winreg.HKEY_CURRENT_USER, path)
    except WindowsError:
        logger.debug("Cannot delete key because it does not exist, ignoring.", exc_info=True)


def clean_ly_keys(exception_list=None):
    """
    Convenience function to delete all Lumberyard registry keys

    :param exception_list: list of child keys and values to skip
    """
    _delete_key(CONST_LY_REG, exception_list)


def create_ly_keys():
    """
    Helper function to create Lumberyard registry keys that are essential to automated testing
    """
    target_key = fr'HKEY_CURRENT_USER\{CONST_LY_REG}\Settings'
    # DXInstalled is a flag set to ensure all machines have DirectX installed
    process_utils.check_call(
        ["reg", "add", target_key, "/v", "DXInstalled", "/t", "REG_DWORD", "/d", "1", "/f"])
    # The perforce plugin is enabled by default which is strongly recommended to be turned off for automation
    process_utils.check_call(
        ["reg", "add", target_key, "/v", "EnableSourceControl", "/t", "REG_DWORD", "/d", "0", "/f"])
    process_utils.check_call(
        ["reg", "add", target_key, "/v", "RC_EnableSourceControl", "/t", "REG_DWORD", "/d", "0", "/f"])
    # The editor will ignore settings unless EditorSettingsVersion is what it expects
    # The value it expects is in Code\Editor\Settings.cpp
    process_utils.check_call(
        ["reg", "add", target_key, "/v", "EditorSettingsVersion", "/t", "REG_DWORD", "/d", "2", "/f"])
