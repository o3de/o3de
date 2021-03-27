"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

Library of functions to support reading, modifying and writing to
AssetProcessorPlatformConfig.setreg

"""

import json
import logging
import os.path as path

logger = logging.getLogger(__name__)
AssetProcessorConfig = "AssetProcessorPlatformConfig.setreg"

def load_asset_processor_platform_config(file_location):
    """
    Loads the AssetProcessorPlatformConfig.setreg file removing any comments
    
    :param file_location: The file path of AssetProcessorPlatformConfig.setreg
    """
    json_dict = {}
    with open(file_location, 'r') as json_file:
        cleaned_lines = []
        for line in json_file.readlines():
            lineIndex = line.lstrip().startswith('//')
            cleaned_lines.append(line if lineIndex == -1 else line[:lineIndex])
        json_dict = json.loads(''.join(cleaned_lines))
    return json_dict

def save_asset_processor_platform_config(file_location, json_dict):
    """
    Saves the json_dict to the AssetProcessorPlatformConfig.setreg file

    :param file_location: The file path of AssetProcessorPlatformConfig.setreg
    """
    with open(file_location, 'w') as json_file:
        json.dump(json_dict, json_file, indent=4)
    return json_dict

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
    json_dict = load_asset_processor_platform_config(file_location)
    try:
        platformExist = platform in json_dict['Amazon']['AssetProcessor']['Settings']['Platforms']
        return platformExist
    except KeyError as err:
        logger.error(f'KeyError when attempting to query platform existance of {platform} in file {file_location}: {err}')

    return False


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
    # load AssetProcessorPlatformConfig.setreg removing any comments
    json_dict = load_asset_processor_platform_config(file_location)
    try:
        enabled = json_dict['Amazon']['AssetProcessor']['Settings']['Platforms'][platform] == 'enabled'
        return enabled
    except KeyError as err:
        logger.error(f'KeyError when attempting to check if platform {platform} is enabled in file {file_location}: {err}')

    return False


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
    json_dict = load_asset_processor_platform_config(file_location)
    # Enable platform in settings registry
    json_dict.setdefault('Amazon', {}).setdefault('AssetProcessor', {}).setdefault('Settings',{}) \
        .setdefault('Platforms',{})[platform] = 'enabled'
    save_asset_processor_platform_config(file_location)


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
    json_dict = load_asset_processor_platform_config(file_location)
    json_dict.setdefault('Amazon', {}).setdefault('AssetProcessor', {}).setdefault('Settings',{}) \
            .setdefault('Platforms',{})[platform] = 'disabled'
    save_asset_processor_platform_config(file_location)


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

    json_dict = load_asset_processor_platform_config(file_location)
    json_dict.setdefault('Amazon', {}).setdefault('AssetProcessor', {}).setdefault('Settings',{}) \
        .setdefault(section,{})['watch'] = '@ENGINEROOT@/Engine'
    json_dict['Amazon']['AssetProcessor']['Settings'][section]['recursive'] = '1'
    json_dict['Amazon']['AssetProcessor']['Settings'][section]['order'] = '20000'
    save_asset_processor_platform_config(file_location)


def disable_scanfolder_engine(config_setreg_path):
    """
    Disable Asset Processor scanning on the Engine folder

    :param config_setreg_path: The file path to the location of AssetProcessorPlatformConfig.setreg
    :return: None
    """

    file_location = path.join(config_setreg_path, AssetProcessorConfig)
    section = 'ScanFolder Engine'

    logger.debug("Disabling Asset Processor scanning of the Engine folder")

    json_dict = load_asset_processor_platform_config(file_location)
    try:
        del json_dict['Amazon']['AssetProcessor']['Settings'][section]
        save_asset_processor_platform_config(file_location)
    except KeyError as err:
        logger.debug(f'No-op: f{section} key does not exist in file {file_location}')


def enable_scanfolder_editor(config_setreg_path):
    """
    Enable Asset Processor scanning on the editor folder

    :param config_setreg_path: The file path to the location of AssetProcessorPlatformConfig.setreg
    :return: None
    """

    file_location = path.join(config_setreg_path, AssetProcessorConfig)
    section = 'ScanFolder Editor'


    logger.debug("Enabling Asset Processor scanning of the Editor folder")

    json_dict = load_asset_processor_platform_config(file_location)
    json_dict.setdefault('Amazon', {}).setdefault('AssetProcessor', {}).setdefault('Settings',{}) \
        .setdefault(section,{})['watch'] = '@ENGINEROOT@/Editor'
    json_dict['Amazon']['AssetProcessor']['Settings'][section]['output'] = 'editor'
    json_dict['Amazon']['AssetProcessor']['Settings'][section]['recursive'] = '1'
    json_dict['Amazon']['AssetProcessor']['Settings'][section]['order'] = '30000'
    json_dict['Amazon']['AssetProcessor']['Settings'][section]['include'] = 'tools,renderer'
    save_asset_processor_platform_config(file_location)


def disable_scanfolder_editor(config_setreg_path):
    """
    Disable Asset Processor scanning on the Engine folder

    :param config_setreg_path: The file path to the location of AssetProcessorPlatformConfig.setreg
    :return: None
    """

    file_location = path.join(config_setreg_path, AssetProcessorConfig)
    section = 'ScanFolder Editor'

    logger.debug("Disabling Asset Processor scanning of the Editor folder")

    try:
        del json_dict['Amazon']['AssetProcessor']['Settings'][section]
        save_asset_processor_platform_config(file_location)
    except KeyError as err:
        logger.debug(f'No-op: f{section} key does not exist in file {file_location}')


def enable_scanfolder_root(config_setreg_path):
    """
    Enable Asset Processor scanning on the Root folder

    :param config_setreg_path: The file path to the location of AssetProcessorPlatformConfig.setreg
    :return: None
    """

    file_location = path.join(config_setreg_path, AssetProcessorConfig)
    section = 'ScanFolder Root'

    logger.debug("Enabling Asset Processor scanning of the Root folder")

    json_dict = load_asset_processor_platform_config(file_location)
    json_dict.setdefault('Amazon', {}).setdefault('AssetProcessor', {}).setdefault('Settings',{}) \
        .setdefault(section,{})['watch'] = '@ROOT@'
    json_dict['Amazon']['AssetProcessor']['Settings'][section]['recursive'] = '0'
    json_dict['Amazon']['AssetProcessor']['Settings'][section]['order'] = '10000'
    save_asset_processor_platform_config(file_location)


def disable_scanfolder_root(config_setreg_path):
    """
    Disable Asset Processor scanning on the Root folder

    :param config_setreg_path: The file path to the location of AssetProcessorPlatformConfig.setreg
    :return: None
    """

    file_location = path.join(config_setreg_path, AssetProcessorConfig)
    section = 'ScanFolder Root'

    logger.debug("Disabling Asset Processor scanning of the Root folder")

    try:
        del json_dict['Amazon']['AssetProcessor']['Settings'][section]
        save_asset_processor_platform_config(file_location)
    except KeyError as err:
        logger.debug(f'No-op: f{section} key does not exist in file {file_location}')


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
    json_dict = load_asset_processor_platform_config(file_location)
    json_dict.setdefault('Amazon', {}).setdefault('AssetProcessor', {}).setdefault('Settings',{}) \
        .setdefault(rc_pattern,{})[key] = value
    save_asset_processor_platform_config(file_location)


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
    json_dict = load_asset_processor_platform_config(file_location)
    try:
        value = json_dict['Amazon']['AssetProcessor']['Settings'][rc_pattern][key]
        return value
    except KeyError as err:
        logger.error(f'KeyError attempting to query value for key "/Amazon/AssetProcessor/Settings/{rc_pattern}/{key}" in file {file_location}: {err}')

    return None
