"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Class for querying and setting a windows registry setting.

"""
import pytest
import logging
from typing import List, Optional, Tuple, Any

from winreg import (
    CreateKey,
    OpenKey,
    QueryValueEx,
    DeleteValue,
    SetValueEx,
    KEY_ALL_ACCESS,
    KEY_WRITE,
    REG_SZ,
    REG_MULTI_SZ,
    REG_DWORD,
    HKEY_CURRENT_USER,
)


from .platform_setting import PlatformSetting

logger = logging.getLogger(__name__)


class WindowsRegistrySetting(PlatformSetting):
    def __init__(self, workspace: pytest.fixture, subkey: str, key: str, hive: Optional[str] = None) -> None:
        super().__init__(workspace, subkey, key)
        self._hive = None
        try:
            if hive is not None:
                self._hive = self._str_to_hive(hive)
        except ValueError:
            logger.warning(f"Windows Registry Hive {hive} not recognized, using default: HKEY_CURRENT_USER")
        finally:
            if self._hive is None:
                self._hive = HKEY_CURRENT_USER

    def get_value(self, get_type: Optional[bool] = False) -> Any:
        """Retrieves the fast scan value in Windows registry (and optionally the type). If entry DNE, returns None."""
        if self.entry_exists():
            registryKey = OpenKey(self._hive, self._key)
            value = QueryValueEx(registryKey, self._subkey)
            registryKey.Close()
            # Convert windows data type to universal data type flag: PlatformSettings.DATA_TYPE
            # And handles unicode conversion for strings
            value = self._convert_value(value)
            return value if get_type else value[0]

        else:
            logger.warning(f"Could not retrieve Registry entry; key: {self._key}, subkey: {self._subkey}.")
            return None

    def set_value(self, value: Any) -> bool:
        """Sets the Windows registry value."""
        value, win_type = self._format_data(value)
        registryKey = None
        result = False
        try:
            CreateKey(self._hive, self._subkey)
            registryKey = OpenKey(self._hive, self._key, 0, KEY_WRITE)
            SetValueEx(registryKey, self._subkey, 0, win_type, value)
            result = True
        except WindowsError as e:
            logger.warning(f"Windows error caught while setting fast scan registry: {e}")
        finally:
            if registryKey is not None:
                # Close key if it's been opened successfully
                registryKey.Close()
        return result

    def delete_entry(self) -> bool:
        """Deletes the Windows registry entry for fast scan enabled"""
        try:
            if self.entry_exists():
                registryKey = OpenKey(self._hive, self._key, 0, KEY_ALL_ACCESS)
                DeleteValue(registryKey, self._subkey)
                registryKey.Close()
                return True
        except WindowsError:
            logger.error(f"Could not delete registry entry; key: {self._key}, subkey: {self._subkey}")
        finally:
            return False

    def entry_exists(self) -> bool:
        """Checks for existence of the setting in Windows registry."""
        try:
            # Attempt to open and query key. If fails then the entry DNE
            registryKey = OpenKey(self._hive, self._key)
            QueryValueEx(registryKey, self._subkey)
            registryKey.Close()
            return True

        except WindowsError:
            return False

    @staticmethod
    def _format_data(value: bool or int or str or List[str]) -> Tuple[int or str or List[str], int]:
        """Formats the type of the value provided. Returns the formatted value and the windows registry type (int)."""
        if type(value) == str:
            return value, REG_SZ
        elif type(value) == bool:
            value = "true" if value else "false"
            return value, REG_SZ
        elif type(value) == int or type(value) == float:
            if type(value) == float:
                logger.warning(f"Windows registry does not support floats. Truncating {value} to integer")
            value = int(value)
            return value, REG_DWORD
        elif type(value) == list:
            for single_value in value:
                if type(single_value) != str:
                    # fmt:off
                    raise ValueError(
                        f"Windows Registry lists only support strings, got a {type(single_value)} in the list")
                    # fmt:on
            return value, REG_MULTI_SZ
        else:
            raise ValueError(f"Windows registry expected types: int, str and [str], found {type(value)}")

    @staticmethod
    def _convert_value(value_tuple: Tuple[Any, int]) -> Tuple[Any, PlatformSetting.DATA_TYPE]:
        """Converts the Windows registry data and type (tuple) to a (standardized) data and PlatformSetting.DATA_TYPE"""
        value, windows_type = value_tuple
        if windows_type == REG_SZ:
            # Convert from unicode to string
            return value, PlatformSetting.DATA_TYPE.STR
        elif windows_type == REG_MULTI_SZ:
            # Convert from unicode to string
            return [string for string in value], PlatformSetting.DATA_TYPE.STR_LIST
        elif windows_type == REG_DWORD:
            return value, PlatformSetting.DATA_TYPE.INT
        else:
            raise ValueError(f"Type flag not recognized: {windows_type}")

    @staticmethod
    def _str_to_hive(hive_str: str) -> int:
        """Converts a string to a Windows Registry Hive enum (int)"""
        from winreg import HKEY_CLASSES_ROOT, HKEY_CURRENT_CONFIG, HKEY_LOCAL_MACHINE, HKEY_USERS

        lower = hive_str.lower()
        if lower == "hkey_current_user" or lower == "current_user":
            return HKEY_CURRENT_USER
        elif lower == "hkey_classes_root" or lower == "classes_root":
            return HKEY_CLASSES_ROOT
        elif lower == "hkey_current_config" or lower == "current_config":
            return HKEY_CURRENT_CONFIG
        elif lower == "hkey_local_machine" or lower == "local_machine":
            return HKEY_LOCAL_MACHINE
        elif lower == "hkey_users" or lower == "users":
            return HKEY_USERS
        else:
            raise ValueError(f"Hive: {hive_str} not recognized")
