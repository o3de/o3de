"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import logging
import winreg

logger = logging.getLogger(__name__)

LUMBERYARD_SETTINGS_PATH = r'Software\O3DE\O3DE\Settings'

def set_ly_registry_value(reg_path, value_name, new_value, value_type=winreg.REG_DWORD):
    """
    Sets the specified value for the specified value_name in the LY registry key.
    :param reg_path: A string that identifies the registry path to the desired key (e.g. Software\\O3DE\\O3DE\\Settings)
    :param value_name: A string that identifies the value name (e.g. UndoLevels, ViewportInteractionModel)
    :param new_value: Value to set on the specified value_name
    :param value_type: The type of value set. Defaults to a 32-bit number.
    :return: None
    """
    # Open LY Registry key
    try:
        key = winreg.CreateKeyEx(winreg.HKEY_CURRENT_USER, reg_path, 0, access=winreg.KEY_ALL_ACCESS)
    except OSError as err:
        logger.error(err)

    # Set value_name to the specified value
    winreg.SetValueEx(key, value_name, 0, value_type, new_value)

    # Verify the value set to the specified value_name
    value = winreg.QueryValueEx(key, value_name)
    if new_value == value[0]:
        logger.debug(f'Successfully set {value_name} to {value[0]}')
    else:
        logger.debug(f'Failed to set {value_name} to {new_value}. Current value is {value[0]}.')


def get_ly_registry_value(reg_path, value_name):
    """
    Gets the current value for an existing value_name in the LY registry key.
    :param reg_path: A string that identifies the registry path to the desired key (e.g. Software\\O3DE\\O3DE\\Settings)
    :param value_name: A string that identifies the value name (e.g. UndoLevels, ViewportInteractionModel)
    :return: Value set for the specified value_name
    """
    # Open LY Registry key
    try:
        key = winreg.CreateKeyEx(winreg.HKEY_CURRENT_USER, reg_path, 0, access=winreg.KEY_ALL_ACCESS)
    except OSError as err:
        logger.error(err)

    # Check if value name is present and return current value
    try:
        value_name_value = winreg.QueryValueEx(key, value_name)
        logger.debug(f'{value_name} is {value_name_value[0]}')
        return value_name_value[0]
    except OSError as err:
        logger.error(err)


def delete_ly_registry_value(reg_path, value_name):
    """
    Deletes the specific registry value_name found in the reg_path key.
    :param reg_path: A string that identifies the registry path to the desired key (e.g. Software\\O3DE\\O3DE\\Settings)
    :param value_name: A string that identifies the value name (e.g. UndoLevels, ViewportInteractionModel)
    :return: None
    """
    # Open LY Registry key
    try:
        key = winreg.CreateKeyEx(winreg.HKEY_CURRENT_USER, reg_path, 0, access=winreg.KEY_ALL_ACCESS)
    except OSError as err:
        logger.error(err)

    # Attempt to delete the specified key/value
    try:
        winreg.DeleteValue(key, value_name)
    except OSError as err:
        logger.error(err)
