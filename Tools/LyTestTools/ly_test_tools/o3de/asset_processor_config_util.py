"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Library of functions to support reading, modifying and writing to
AssetProcessorPlatformConfig.setreg

"""

import logging
import os.path as path
from ly_test_tools.o3de.settings import RegistrySettings
from ly_test_tools.o3de.asset_processor import ASSET_PROCESSOR_SETTINGS_ROOT_KEY

logger = logging.getLogger(__name__)
AssetProcessorConfig = "AssetProcessorPlatformConfig.setreg"

def platform_exists(config_setreg_path, platform):
    """
    Checks to see if a specific Platform Key is in Platforms Section

    :param config_setreg_path: The file path to the location of AssetProcessorPlatformConfig.setreg
    :param platform: Name of the Platform Key that you're checking exists
    :return: The boolean value of the existance of the platform
    """
    logger.debug("Checking for Platform '{0}' in Platforms Section of '{1}"
                .format(platform, AssetProcessorConfig))

    file_location = path.join(config_setreg_path, AssetProcessorConfig)

    with RegistrySettings(file_location) as settings:
        platformExist = settings.get_key(f'{ASSET_PROCESSOR_SETTINGS_ROOT_KEY}/Platforms/{platform}', False)

    return platformExist


def is_platform_enabled(config_setreg_path, platform):
    """
    Checks to see if a specific Platform Key is enabled in Platforms Section

    :param config_setreg_path: The file path to the location of AssetProcessorPlatformConfig.setreg
    :param platform: Name of the Platform that you're checking is enabled.
        See asset_processor.SUPPORTED_PLATFORMS for listed of supported platforms
    :return: The boolean value of the enabled state of the platform
    """
    logger.debug("Checking if Platform '{0}' is enabled in Platform Section of '{1}"
                .format(platform, AssetProcessorConfig))

    file_location = path.join(config_setreg_path, AssetProcessorConfig)

    with RegistrySettings(file_location) as settings:
        enabled = settings.get_key(f'{ASSET_PROCESSOR_SETTINGS_ROOT_KEY}/Platforms/{platform}', None) == 'enabled'

    return enabled


def enable_platform(config_setreg_path, platform):
    """
    Enables a specific Platform Key is in Platforms Section

    :param config_setreg_path: The file path to the location of AssetProcessorPlatformConfig.setreg
    :param platform: Name of the Platform Key that you're checking exists
        See asset_processor.SUPPORTED_PLATFORMS for listed of supported platforms
    :assert: Assert if the platform is not enabled
    :return: None
    """
    logger.debug("Enabling platform '{0}' in '{1}'".format(platform, AssetProcessorConfig))

    file_location = path.join(config_setreg_path, AssetProcessorConfig)
    with RegistrySettings(file_location) as settings:
        settings.set_key(f'{ASSET_PROCESSOR_SETTINGS_ROOT_KEY}/Platforms/{platform}', 'enabled')


def enable_all_platforms(config_setreg_path):
    """
    Enable all supported platforms in the Platforms Section

    :param config_setreg_path: The file path to the location of AssetProcessorPlatformConfig.setreg
    :return: None
    """
    logger.debug("Enabling all supported platforms in '{0}'.".format(AssetProcessorConfig))

    for platform in SUPPORTED_PLATFORMS:
        enable_platform(config_setreg_path, platform)

    logger.debug("All supported platforms have been enabled in '{0}'.".format(AssetProcessorConfig))


def disable_platform(config_setreg_path, platform):
    """
    Disables a specific Platform Key is in Platforms Section

    :param config_setreg_path: The file path to the location of AssetProcessorPlatformConfig.setreg
    :param platform: Name of the Platform Key that you're checking exists.
        See asset_processor_config_util.SUPPORTED_PLATFORMS for listed of supported platforms
    :assert: Assert if the platform is not disabled
    :return: None
    """
    logger.debug("Disabling platform '{0}' in '{1}'".format(platform, AssetProcessorConfig))

    file_location = path.join(config_setreg_path, AssetProcessorConfig)
    # Disable platform in settings registry
    file_location = path.join(config_setreg_path, AssetProcessorConfig)
    with RegistrySettings(file_location) as settings:
        settings.set_key(f'{ASSET_PROCESSOR_SETTINGS_ROOT_KEY}/Platforms/{platform}', 'disabled')


def disable_all_platforms(config_setreg_path):
    """
    Disable all supported platforms in the Platforms Section

    :param config_setreg_path: The file path to the location of AssetProcessorPlatformConfig.setreg
    :return: None
    """
    logger.debug("Disabling all platforms in '{0}'".format(AssetProcessorConfig))

    for platform in SUPPORTED_PLATFORMS:
        disable_platform(config_setreg_path, platform)

    logger.debug("All supported platforms have been disabled in '{0}'.".format(AssetProcessorConfig))


def enable_scanfolder_engine(config_setreg_path):
    """
    Enable Asset Processor scanning on the Engine folder

    :param config_setreg_path: The file path to the location of AssetProcessorPlatformConfig.setreg
    :return: None
    """
    file_location = path.join(config_setreg_path, AssetProcessorConfig)
    section = 'ScanFolder Engine'

    logger.debug("Enabling Asset Processor scanning of the Engine folder")

    with RegistrySettings(file_location) as settings:
        settings.set_key(f'{ASSET_PROCESSOR_SETTINGS_ROOT_KEY}/{section}/watch', '@ENGINEROOT@/Engine')
        settings.set_key(f'{ASSET_PROCESSOR_SETTINGS_ROOT_KEY}/{section}/recursive', '1')
        settings.set_key(f'{ASSET_PROCESSOR_SETTINGS_ROOT_KEY}/{section}/order', '20000')


def disable_scanfolder_engine(config_setreg_path):
    """
    Disable Asset Processor scanning on the Engine folder

    :param config_setreg_path: The file path to the location of AssetProcessorPlatformConfig.setreg
    :return: None
    """
    file_location = path.join(config_setreg_path, AssetProcessorConfig)
    section = 'ScanFolder Engine'

    logger.debug("Disabling Asset Processor scanning of the Engine folder")
    with RegistrySettings(file_location) as settings:
        settings.remove_key(f'{ASSET_PROCESSOR_SETTINGS_ROOT_KEY}/{section}')


def enable_scanfolder_editor(config_setreg_path):
    """
    Enable Asset Processor scanning on the editor folder

    :param config_setreg_path: The file path to the location of AssetProcessorPlatformConfig.setreg
    :return: None
    """
    file_location = path.join(config_setreg_path, AssetProcessorConfig)
    section = 'ScanFolder Editor'

    logger.debug("Enabling Asset Processor scanning of the Editor folder")

    with RegistrySettings(file_location) as settings:
        settings.set_key(f'{ASSET_PROCESSOR_SETTINGS_ROOT_KEY}/{section}/watch', '@ENGINEROOT@/Editor')
        settings.set_key(f'{ASSET_PROCESSOR_SETTINGS_ROOT_KEY}/{section}/output', 'editor')
        settings.set_key(f'{ASSET_PROCESSOR_SETTINGS_ROOT_KEY}/{section}/recursive', '1')
        settings.set_key(f'{ASSET_PROCESSOR_SETTINGS_ROOT_KEY}/{section}/order', '30000')
        settings.set_key(f'{ASSET_PROCESSOR_SETTINGS_ROOT_KEY}/{section}/include', 'tools,renderer')


def disable_scanfolder_editor(config_setreg_path):
    """
    Disable Asset Processor scanning on the Engine folder

    :param config_setreg_path: The file path to the location of AssetProcessorPlatformConfig.setreg
    :return: None
    """
    file_location = path.join(config_setreg_path, AssetProcessorConfig)
    section = 'ScanFolder Editor'

    logger.debug("Disabling Asset Processor scanning of the Editor folder")

    with RegistrySettings(file_location) as settings:
        settings.remove_key(f'{ASSET_PROCESSOR_SETTINGS_ROOT_KEY}/{section}')


def enable_scanfolder_root(config_setreg_path):
    """
    Enable Asset Processor scanning on the Root folder

    :param config_setreg_path: The file path to the location of AssetProcessorPlatformConfig.setreg
    :return: None
    """
    file_location = path.join(config_setreg_path, AssetProcessorConfig)
    section = 'ScanFolder Root'

    logger.debug("Enabling Asset Processor scanning of the Root folder")

    with RegistrySettings(file_location) as settings:
        settings.set_key(f'{ASSET_PROCESSOR_SETTINGS_ROOT_KEY}/{section}/watch', '@ROOT@')
        settings.set_key(f'{ASSET_PROCESSOR_SETTINGS_ROOT_KEY}/{section}/recursive', '0')
        settings.set_key(f'{ASSET_PROCESSOR_SETTINGS_ROOT_KEY}/{section}/order', '10000')


def disable_scanfolder_root(config_setreg_path):
    """
    Disable Asset Processor scanning on the Root folder

    :param config_setreg_path: The file path to the location of AssetProcessorPlatformConfig.setreg
    :return: None
    """
    file_location = path.join(config_setreg_path, AssetProcessorConfig)
    section = 'ScanFolder Root'

    logger.debug("Disabling Asset Processor scanning of the Root folder")

    with RegistrySettings(file_location) as settings:
        settings.remove_key(f'{ASSET_PROCESSOR_SETTINGS_ROOT_KEY}/{section}')


def update_pattern(config_setreg_path, pattern, key, value):
    """
    Update a RC pattern's behavior options with the provided key and value.

    :param config_setreg_path: The file path to the location of AssetProcessorPlatformConfig.setreg
    :param pattern: The RC pattern to update
    :param key: The key to update
    :param value: The value to set the key to
    :return: None
    """
    file_location = path.join(config_setreg_path, AssetProcessorConfig)
    rc_pattern = 'RC ' + pattern

    logger.debug("Modifying pattern for '{0}'.".format(rc_pattern))
    with RegistrySettings(file_location) as settings:
        settings.set_key(f'{ASSET_PROCESSOR_SETTINGS_ROOT_KEY}/{rc_pattern}/{key}', value)


def get_pattern_key(config_setreg_path, pattern, key):
    """
    Get the value of a RC pattern's behavior options key value

    :param config_setreg_path: The file path to the location of AssetProcessorPlatformConfig.setreg
    :param pattern: The RC pattern to retrieve a key from
    :param key: The key to retrieve
    :return: value of RC pattern key value
    """

    file_location = path.join(config_setreg_path, AssetProcessorConfig)
    rc_pattern = 'RC ' + pattern
    logger.debug("Retrieving the key '{0}' from pattern '{1}'.".format(key, rc_pattern))
    with RegistrySettings(file_location) as settings:
        value = settings.get_key(f'{ASSET_PROCESSOR_SETTINGS_ROOT_KEY}/{rc_pattern}/{key}', None)

    return value
