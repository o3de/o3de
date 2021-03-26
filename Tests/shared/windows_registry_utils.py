"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

Small library of functions to support autotests for utilizing Windows Utilities

"""

import logging
import winreg
logger = logging.getLogger(__name__)


def registry_key_exists(registry_hive, registry_key, registry_subkey):
    """
    Searches the Windows registry for the existance of a registry key
    :param registry_hive: The hive in which to find keys & subkeys. EG: HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER
    :param registry_key: The bin directory from which to launch the AssetProcessor executable.
    :param registry_subkey: The subkey that can contain a value assignment
    :return: A boolean value of the existance of the key
    """

    try:
        registryKey = winreg.OpenKey(registry_hive, registry_key)
        logger.debug("Registry Key: {0} found.".format(registry_key))

        winreg.QueryValueEx(registryKey, registry_subkey)
        logger.debug("Registry Subkey: {0} found.".format(registry_subkey))

        registryKey.Close()

        return True
    except WindowsError:
        # Do not raise an assert since tests could revolve around a non-existant key
        logger.debug("Registry SubKey: {0} was not found.".format(registry_key))
        return False


def get_registry_key_value(registry_hive, registry_key, registry_subkey):
    """
    If a registry key exists, it will return the value else return None
    :param registry_hive: The hive in which to find keys & subkeys. EG: HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER
    :param registry_key: The bin directory from which to launch the AssetProcessor executable.
    :param registry_subkey: The subkey that can contain a value assignment
    :return: The value of the registry key value
    """

    if registry_key_exists(registry_hive, registry_key, registry_subkey):

        registryKey = winreg.OpenKey(registry_hive, registry_key)
        logger.debug("Registry Key: {0} found.".format(registry_key))

        subkeyValue = winreg.QueryValueEx(registryKey, registry_subkey)
        logger.debug("Registry Subkey: {0} value found is is set to {1}".format(registry_subkey, subkeyValue))

        registryKey.Close()
        logger.debug("Registry Key hander closed")

        return str(subkeyValue[0])  # Index 0 contains the value, Index 1 contains the registry value type
    else:
        assert None, "Could not retrieve Registry Key Value since Registry Key '{0}' was not found.".format(registry_key)


def check_registry_key_value(registry_hive, registry_key, registry_subkey, expected='', case_sensitive=True):
    """
    If registry key exists, then case insensitively  checks that the registry key value is as expected
    :param registry_hive: The hive in which to find keys & subkeys. EG: HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER
    :param registry_key: The bin directory from which to launch the AssetProcessor executable.
    :param registry_subkey: The subkey that can contain a value assignment
    :param expected: The expected value to be found in the registry
    :param case_sensitive: Whether or not to perform a case sensitive or insentive validation
    :return: A boolean value if the registry key value matches expected
    """

    if registry_key_exists(registry_hive, registry_key, registry_subkey):

        subkeyValue = get_registry_key_value(registry_hive, registry_key, registry_subkey)

        if case_sensitive:
            logger.debug("Case sensitive comparison of Subkey '{0}' to Expected Value '{1}'"
                        .format(subkeyValue, expected))
            return subkeyValue == expected
        else:
            logger.debug("Case insensitive comparison of Subkey '{0}' to Expected Value '{1}'"
                        .format(subkeyValue, expected))
            return subkeyValue.lower() == expected.lower()
    else: 
        logger.debug("Could not compare Subkey Value to expected value, Subkey '{0}' was not found at Key {1}."
                    .format(registry_subkey, registry_key))
        return False
