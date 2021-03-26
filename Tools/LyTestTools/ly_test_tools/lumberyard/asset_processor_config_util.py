"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

Library of functions to support reading, modifying and writing to
AssetProcessorPlatformConfig.ini

"""

import logging
from ly_test_tools.lumberyard import ini_configuration_util as ini
import os.path as path

logger = logging.getLogger(__name__)
AssetProcessorConfig = "AssetProcessorPlatformConfig.ini"

def platform_exists(config_ini_path, platform):
    """
    Checks to see if a specific Platform Key is in Platforms Section

    :param config_ini_path: The file path to the location of AssetProcessorPlatformConfig.ini
    :param platform: Name of the Platform Key that you're checking exists
    :return: The boolean value of the existance of the platform
    """

    logger.debug("Checking for Platform '{0}' in Platforms Section of '{1}"
                .format(platform, AssetProcessorConfig))

    file_location = path.join(config_ini_path, AssetProcessorConfig)

    assert ini.check_section_exists(file_location, 'Platforms'), \
        'Platforms section does not exist in {0}'.format(file_location)

    return ini.check_key_exists(path.join(config_ini_path, AssetProcessorConfig), 'Platforms', platform)


def is_platform_enabled(config_ini_path, platform):
    """
    Checks to see if a specific Platform Key is enabled in Platforms Section

    :param config_ini_path: The file path to the location of AssetProcessorPlatformConfig.ini
    :param platform: Name of the Platform that you're checking is enabled.
        See asset_processor.SUPPORTED_PLATFORMS for listed of supported platforms
    :return: The boolean value of the enabled state of the platform
    """

    logger.debug("Checking if Platform '{0}' is enabled in Platform Section of '{1}"
                .format(platform, AssetProcessorConfig))

    file_location = path.join(config_ini_path, AssetProcessorConfig)

    assert ini.check_section_exists(file_location, 'Platforms'), \
        'Platforms section does not exist in {0}'.format(file_location)

    enabled = str(ini.get_string_value(file_location, 'Platforms', platform)) == 'enabled'

    return enabled


def enable_platform(config_ini_path, platform):
    """
    Enables a specific Platform Key is in Platforms Section

    :param config_ini_path: The file path to the location of AssetProcessorPlatformConfig.ini
    :param platform: Name of the Platform Key that you're checking exists
        See asset_processor.SUPPORTED_PLATFORMS for listed of supported platforms
    :assert: Assert if the platform is not enabled
    :return: None
    """

    logger.debug("Enabling platform '{0}' in '{1}'".format(platform, AssetProcessorConfig))

    file_location = path.join(config_ini_path, AssetProcessorConfig)

    ini.add_key(file_location, 'Platforms', platform, 'enabled')

    assert is_platform_enabled(config_ini_path, platform), "Platform '{0}' failed to enable in '{1}'"\
        .format(platform, AssetProcessorConfig)


def enable_all_platforms(config_ini_path):
    """
    Enable all supported platforms in the Platforms Section

    :param config_ini_path: The file path to the location of AssetProcessorPlatformConfig.ini
    :return: None
    """

    logger.debug("Enabling all supported platforms in '{0}'.".format(AssetProcessorConfig))

    for platform in SUPPORTED_PLATFORMS:
        enable_platform(config_ini_path, platform)

    logger.debug("All supported platforms have been enabled in '{0}'.".format(AssetProcessorConfig))


def disable_platform(config_ini_path, platform):
    """
    Disables a specific Platform Key is in Platforms Section

    :param config_ini_path: The file path to the location of AssetProcessorPlatformConfig.ini
    :param platform: Name of the Platform Key that you're checking exists.
        See asset_processor_config_util.SUPPORTED_PLATFORMS for listed of supported platforms
    :assert: Assert if the platform is not disabled
    :return: None
    """

    logger.debug("Disabling platform '{0}' in '{1}'".format(platform, AssetProcessorConfig))

    file_location = path.join(config_ini_path, AssetProcessorConfig)

    ini.add_key(file_location, 'Platforms', platform, 'disabled')

    assert not is_platform_enabled(config_ini_path, platform), "Platform '{0}' failed to disable in '{1}'"\
        .format(platform, AssetProcessorConfig)


def disable_all_platforms(config_ini_path):
    """
    Disable all supported platforms in the Platforms Section

    :param config_ini_path: The file path to the location of AssetProcessorPlatformConfig.ini
    :return: None
    """

    logger.debug("Disabling all platforms in '{0}'".format(AssetProcessorConfig))

    for platform in SUPPORTED_PLATFORMS:
        disable_platform(config_ini_path, platform)

    logger.debug("All supported platforms have been disabled in '{0}'.".format(AssetProcessorConfig))


def enable_scanfolder_engine(config_ini_path):
    """
    Enable Asset Processor scanning on the Engine folder

    :param config_ini_path: The file path to the location of AssetProcessorPlatformConfig.ini
    :return: None
    """

    file_location = path.join(config_ini_path, AssetProcessorConfig)
    section = 'ScanFolder Engine'

    logger.debug("Enabling Asset Processor scanning of the Engine folder")

    if not ini.check_section_exists(file_location, section):
        ini.add_section(file_location, section)
    ini.add_key(file_location, section, 'watch', r'@ENGINEROOT@/Engine')
    ini.add_key(file_location, section, 'recursive', '1')
    ini.add_key(file_location, section, 'order', '20000')


def disable_scanfolder_engine(config_ini_path):
    """
    Disable Asset Processor scanning on the Engine folder

    :param config_ini_path: The file path to the location of AssetProcessorPlatformConfig.ini
    :return: None
    """

    file_location = path.join(config_ini_path, AssetProcessorConfig)
    section = 'ScanFolder Engine'

    logger.debug("Disabling Asset Processor scanning of the Engine folder")

    ini.delete_section(file_location, section)


def enable_scanfolder_editor(config_ini_path):
    """
    Enable Asset Processor scanning on the editor folder

    :param config_ini_path: The file path to the location of AssetProcessorPlatformConfig.ini
    :return: None
    """

    file_location = path.join(config_ini_path, AssetProcessorConfig)
    section = 'ScanFolder Editor'


    logger.debug("Enabling Asset Processor scanning of the Editor folder")

    if not ini.check_section_exists(file_location, section):
        ini.add_section(file_location, section)
    ini.add_key(file_location, section, 'watch', r'@ENGINEROOT@/Editor')
    ini.add_key(file_location, section, 'output', 'editor')
    ini.add_key(file_location, section, 'recursive', '1')
    ini.add_key(file_location, section, 'order', '30000')
    ini.add_key(file_location, section, 'include', 'tools,renderer')


def disable_scanfolder_editor(config_ini_path):
    """
    Disable Asset Processor scanning on the Engine folder

    :param config_ini_path: The file path to the location of AssetProcessorPlatformConfig.ini
    :return: None
    """

    file_location = path.join(config_ini_path, AssetProcessorConfig)
    section = 'ScanFolder Editor'

    logger.debug("Disabling Asset Processor scanning of the Editor folder")

    ini.delete_section(file_location, section)


def enable_scanfolder_root(config_ini_path):
    """
    Enable Asset Processor scanning on the Root folder

    :param config_ini_path: The file path to the location of AssetProcessorPlatformConfig.ini
    :return: None
    """

    file_location = path.join(config_ini_path, AssetProcessorConfig)
    section = 'ScanFolder Root'

    logger.debug("Enabling Asset Processor scanning of the Root folder")

    if not ini.check_section_exists(file_location, section):
        ini.add_section(file_location, section)
    ini.add_key(file_location, section, 'watch', r'@ROOT@')
    ini.add_key(file_location, section, 'recursive', '0')
    ini.add_key(file_location, section, 'order', '10000')


def disable_scanfolder_root(config_ini_path):
    """
    Disable Asset Processor scanning on the Root folder

    :param config_ini_path: The file path to the location of AssetProcessorPlatformConfig.ini
    :return: None
    """

    file_location = path.join(config_ini_path, AssetProcessorConfig)
    section = 'ScanFolder Root'

    logger.debug("Disabling Asset Processor scanning of the Root folder")

    ini.delete_section(file_location, section)


def update_pattern(config_ini_path, pattern, key, value):
    """
    Update a RC pattern's behavior options with the provided key and value.

    :param config_ini_path: The file path to the location of AssetProcessorPlatformConfig.ini
    :param pattern: The RC pattern to update
    :param key: The key to update
    :param value: The value to set the key to
    :return: None
    """

    file_location = path.join(config_ini_path, AssetProcessorConfig)
    rc_pattern = 'RC ' + pattern

    logger.debug("Modifying pattern for '{0}'.".format(rc_pattern))
    ini.add_key(file_location, rc_pattern, key, value)


def get_pattern_key(config_ini_path, pattern, key):
    """
    Get the value of a RC pattern's behavior options key value

    :param config_ini_path: The file path to the location of AssetProcessorPlatformConfig.ini
    :param pattern: The RC pattern to retrieve a key from
    :param key: The key to retrieve
    :return: value of RC pattern key value
    """

    file_location = path.join(config_ini_path, AssetProcessorConfig)
    rc_pattern = 'RC ' + pattern
    logger.debug("Retrieving the key '{0}' from pattern '{1}'.".format(key, rc_pattern))
    return ini.get_string_value(file_location, rc_pattern, key)
