#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
import argparse
import configparser
import datetime
import fnmatch
import puremagic
import json
import logging
import os
import re
import pathlib
import platform
import shutil
import stat
import string
import subprocess

from enum import Enum
from o3de import command_utils, manifest, utils, export_project as exp
from packaging.version import Version
from pathlib import Path
from typing import Dict, List, Tuple

logging.basicConfig(format=utils.LOG_FORMAT)
logger = logging.getLogger('o3de.android')

DEFAULT_READ_ENCODING = 'utf-8'    # The default encoding to use when reading from a text file or stream
DEFAULT_WRITE_ENCODING = 'ascii'
ENCODING_ERROR_HANDLINGS = 'ignore'     # What to do if we encounter any encoding errors

# Set platform specific values and extensions
if platform.system() == 'Windows':
    EXE_EXTENSION = '.exe'
    O3DE_SCRIPT_EXTENSION = '.bat'
    SDKMANAGER_EXTENSION = '.bat'
    GRADLE_EXTENSION = '.bat'
    DEFAULT_ANDROID_SDK_PATH = f"{os.getenv('LOCALAPPDATA')}\\Android\\Sdk"
    PYTHON_SCRIPT = 'python.cmd'
elif platform.system() == 'Darwin':
    EXE_EXTENSION = ''
    O3DE_SCRIPT_EXTENSION = '.sh'
    SDKMANAGER_EXTENSION = ''
    GRADLE_EXTENSION = ''
    DEFAULT_ANDROID_SDK_PATH = f"{os.getenv('HOME')}/Library/Android/Sdk"
    PYTHON_SCRIPT = 'python.sh'
elif platform.system() == 'Linux':
    EXE_EXTENSION = ''
    O3DE_SCRIPT_EXTENSION = '.sh'
    SDKMANAGER_EXTENSION = ''
    GRADLE_EXTENSION = ''
    DEFAULT_ANDROID_SDK_PATH = f"{os.getenv('HOME')}/Android/Sdk"
    PYTHON_SCRIPT = 'python.sh'
else:
    assert False, f"Unknown platform {platform.system()}"

ASSET_MODE_LOOSE = 'LOOSE'
ASSET_MODE_PAK = 'PAK'
ASSET_MODE_NONE = 'NONE'

ASSET_MODES = [ASSET_MODE_LOOSE, ASSET_MODE_PAK, ASSET_MODE_NONE]

BUILD_CONFIGURATIONS = ['Debug', 'Profile', 'Release']
ANDROID_ARCH = 'arm64-v8a'

DEFAULT_ANDROID_GRADLE_PLUGIN_VERSION = '8.1.0'
ANDROID_RESOLUTION_SETTINGS = ('mdpi', 'hdpi', 'xhdpi', 'xxhdpi', 'xxxhdpi')

# Values to set for the android configuration handler
ANDROID_SETTINGS_FILE = '.command_settings'
ANDROID_SETTINGS_SECTION_NAME = 'android'

SUPPORTED_ANDROID_SETTINGS = []

def register_setting(key: str, description: str, default: str= None,  is_password:bool = False, is_boolean = False, restricted_regex: str = None, restricted_regex_description: str = None) -> command_utils.SettingsDescription:
    """
    Register a new settings description for android

    :param key:             The settings key used for configuration storage
    :param description:     The description of this setting value
    :param default:         The default value for the setting
    :param is_password:     Flag indicating that this is a password setting
    :param is_boolean:      Flag indicating that this is a boolean setting
    :param restricted_regex:  (Optional) Validation regex to restrict the possible value
    :param restricted_regex_description:  Description of the validation regex
    :return: The new settings description
    """

    global SUPPORTED_ANDROID_SETTINGS

    new_setting = command_utils.SettingsDescription(key=key,
                                                    description=description,
                                                    default=default,
                                                    is_password=is_password,
                                                    is_boolean=is_boolean,
                                                    restricted_regex=restricted_regex,
                                                    restricted_regex_description=restricted_regex_description)
    SUPPORTED_ANDROID_SETTINGS.append(new_setting)

    return new_setting


#
# Declarations of the android-specific settings
#
SETTINGS_SDK_ROOT              = register_setting(key='sdk.root',
                                                  description='The root path of the android sdk on this system.')

SETTINGS_PLATFORM_SDK_API      = register_setting(key='platform.sdk.api',
                                                  description='The android platform API level (ref https://developer.android.com/tools/releases/platforms)',
                                                  default='31')

SETTINGS_NDK_VERSION           = register_setting(key='ndk.version',
                                                  description='The version of the android NDK (ref https://developer.android.com/ndk/downloads). File matching patterns can be used (i.e. 25.* will search for the most update to date major version 25)',
                                                  default='25.*')

SETTINGS_GRADLE_PLUGIN_VERSION = register_setting(key='android.gradle.plugin',
                                                  description='The version of the android gradle plugin to use for the build (ref https://developer.android.com/reference/tools/gradle-api)',
                                                  default=DEFAULT_ANDROID_GRADLE_PLUGIN_VERSION)

SETTINGS_GRADLE_HOME           = register_setting(key='gradle.home',
                                                  description='The root path of the locally installed version of gradle. If not set, the gradle that is in the PATH environment will be used')

SETTINGS_GRADLE_JVM_ARGS       = register_setting(key='gradle.jvmargs',
                                                  description='Customized jvm arguments to set when invoking gradle. (ref https://docs.gradle.org/current/userguide/config_gradle.html#sec:configuring_jvm_memory)',
                                                  default='')

SETTINGS_CMAKE_HOME            = register_setting(key='cmake.home',
                                                  description='The root path of the locally installed version of cmake. If not set, the cmake that is in the PATH environment will be used')

# Settings related to the signing configuration (ref https://developer.android.com/studio/publish/app-signing)
SETTINGS_SIGNING_STORE_FILE     = register_setting(key='signconfig.store.file',
                                                   description='The key store file to use for creating a signing config. (ref https://developer.android.com/studio/publish/app-signing)')

SETTINGS_SIGNING_STORE_PASSWORD = register_setting(key='signconfig.store.password',
                                                   description='The password for the key store file',
                                                   is_password=True)

SETTINGS_SIGNING_KEY_ALIAS      = register_setting(key='signconfig.key.alias',
                                                   description='The key alias withing the key store that idfentifies the signing key')

SETTINGS_SIGNING_KEY_PASSWORD   = register_setting(key='signconfig.key.password',
                                                   description='The password for the key inside the key store referenced by the key alias',
                                                   is_password=True)

# General O3DE build and deployment options
SETTINGS_ASSET_MODE             = register_setting(key='asset.mode',
                                                   description=f"The asset mode to determine how the assets are stored in the target APK. Valid values are {','.join(ASSET_MODES)}.",
                                                   restricted_regex=f'({ASSET_MODE_LOOSE}|{ASSET_MODE_PAK}|{ASSET_MODE_NONE})',
                                                   restricted_regex_description=f"Valid values are {','.join(ASSET_MODES)}.")

SETTINGS_STRIP_DEBUG            = register_setting(key='strip.debug',
                                                   description='Option to strip the debug symbols of the built native libs before deployment to the APK',
                                                   is_boolean=True)

SETTINGS_OCULUS_PROJECT         = register_setting(key='oculus.project',
                                                   description='Option to set Oculus-specific build options when building the APK',
                                                   is_boolean=True)

SETTINGS_ASSET_BUNDLE_SUBPATH   = register_setting(key='asset.bundle.subpath',
                                                   description='The sub-path from the project root to specify where the bundle/pak files will be generated to. '
                                                                                      '(ref https://www.docs.o3de.org/docs/user-guide/packaging/asset-bundler/)')

SETTINGS_EXTRA_CMAKE_ARGS       = register_setting(key='extra.cmake.args',
                                                   description='Optional string to set additional cmake arguments during the native project generation within the android gradle build process')


def get_android_config(project_path: Path or None) -> command_utils.O3DEConfig:
    """
    Create an android configuration a project. If a project name is provided, then attempt to look for the project and use its
    project-specific settings (if any) as an overlay to the global settings. If the project name is None, then return an
    android configuration object that only represents the global setting

    :param project_path:    The path to the registered O3DE project to look for its project-specific setting. If None, only use the global settings.
    :return: The android configuration object
    """
    return command_utils.O3DEConfig(project_path=project_path,
                                    settings_filename=ANDROID_SETTINGS_FILE,
                                    settings_section_name=ANDROID_SETTINGS_SECTION_NAME,
                                    settings_description_list=SUPPORTED_ANDROID_SETTINGS)



def deloy_to_android_device(target_android_project_path: pathlib.Path,
                            build_config:str,
                            android_sdk_home:pathlib.Path,
                            org_name,
                            activity_name):
    adb_exec_path = android_sdk_home / (f'platform-tools/adb{EXE_EXTENSION}')
    apk_file = target_android_project_path / f'app/build/outputs/apk/{build_config}/app-{build_config}.apk'
    
    has_error = exp.process_command([adb_exec_path, 'install', '-t', '-r', apk_file])
    if has_error:
        raise AndroidToolError(f"Installing APK '{apk_file}' has failed for the android device.")
    
"""
Map to make troubleshooting java issues related to mismatched java versions when running the sdkmanager.
"""
JAVA_CLASS_FILE_MAP = {
    "65": "Java SE 21",
    "64": "Java SE 20",
    "63": "Java SE 19",
    "62": "Java SE 18",
    "61": "Java SE 17",
    "60": "Java SE 16",
    "59": "Java SE 15",
    "58": "Java SE 14",
    "57": "Java SE 13",
    "56": "Java SE 12",
    "55": "Java SE 11",
    "54": "Java SE 10",
    "53": "Java SE 9",
    "52": "Java SE 8",
    "51": "Java SE 7",
    "50": "Java SE 6",
    "49": "Java SE 5",
    "48": "JDK 1.4",
    "47": "JDK 1.3",
    "46": "JDK 1.2"
}

ANDROID_HELP_REGISTER_ANDROID_SDK_MESSAGE = f"""
Building android projects requires the android sdk manager command-line (https://developer.android.com/tools/sdkmanager). 
Please follow the instructions at https://developer.android.com/studio to download the sdk manager command line tool from
either Android Studio or the command line tools only and set the path to the 'sdkmanager{SDKMANAGER_EXTENSION}' path.

If installing from Android Studio (recommended) then the default location of the Android SDK will be %HOME%   

For example:

o3de{O3DE_SCRIPT_EXTENSION} android-register --global --set-value android_sdk_root=<path to android SDK>

"""

class AndroidToolError(Exception):
    pass


class AndroidGradlePluginRequirements(object):
    """
    This class manages the Android Gradle plugin requirements environment and represents each entry from the
    ANDROID_GRADLE_PLUGIN_COMPATIBILITY_MAP.
    """
    def __init__(self,
                 agp_version:str,
                 gradle_version:str,
                 sdk_build_tools_version:str,
                 jdk_version:str,
                 release_note_url:str):
        self._agp_version = Version(agp_version)
        self._gradle_ver = Version(gradle_version)
        self._sdk_build_tools_version = Version(sdk_build_tools_version)
        self._jdk_version = Version(jdk_version)
        self._release_notes_url = release_note_url

    @property
    def gradle_ver(self) -> Version:
        return self._gradle_ver

    @property
    def sdk_build_tools_version(self) -> Version:
        return self._sdk_build_tools_version

    @property
    def version(self) -> Version:
        return self._agp_version

    def validate_java_version(self, java_version:str) -> None:
        """
        Validate a version of Java against the current Android Gradle Plugin. Raise a detailed exception if it doesn't meet the requirements.
        @param java_version:    The version string reported by java (-version)
        @return: None
        """
        java_version_check = Version(java_version)
        if not java_version_check >= self._jdk_version:
            raise AndroidToolError(f"The installed version of java ({java_version_check}) does not meet the minimum version of ({self._jdk_version}) "
                                   f"which is required by the Android Gradle Plugin version ({self._agp_version}). Refer to the android gradle plugin "
                                   f"release notes at {self._release_notes_url}")

    def validate_gradle_version(self, gradle_version:str) -> None:
        """
        Validate a version of gradle against the current Android Gradle Plugin. Raise a a detailed exception if it doesnt meet the requirements.
        @param java_version:    The version string reported by gradle. (--version)
        @return: None
        """
        gradle_version_check = Version(gradle_version)
        if not gradle_version_check >= self._gradle_ver:
            raise AndroidToolError(f"The installed version of gradle ({gradle_version_check}) does not meet the minimum version of ({self._gradle_ver}) "
                                   f"which is required by the Android Gradle Plugin version ({self._agp_version}). Refer to the android gradle plugin "
                                   f"release notes at {self._release_notes_url}")

# The ANDROID_GRADLE_PLUGIN_COMPATIBILITY_MAP manages the known android plugin known to O3DE and its compatibility requirements
# Note: This map needs to be updated in conjunction with newer versions of the Android Gradle plugins.

ANDROID_GRADLE_PLUGIN_COMPATIBILITY_MAP = {

    '8.1': AndroidGradlePluginRequirements(agp_version='8.1',
                                           gradle_version='8.0',
                                           sdk_build_tools_version='33.0.1',
                                           jdk_version='17',
                                           release_note_url='https://developer.android.com/build/releases/gradle-plugin'),

    '8.0': AndroidGradlePluginRequirements(agp_version='8.0',
                                           gradle_version='8.0',
                                           sdk_build_tools_version='30.0.3',
                                           jdk_version='17',
                                           release_note_url='https://developer.android.com/build/releases/past-releases/agp-8-0-0-release-notes'),

    '7.4': AndroidGradlePluginRequirements(agp_version='7.4',
                                           gradle_version='7.5',
                                           sdk_build_tools_version='30.0.3',
                                           jdk_version='11',
                                           release_note_url='https://developer.android.com/build/releases/past-releases/agp-7-4-0-release-notes'),

    '7.3': AndroidGradlePluginRequirements(agp_version='7.3',
                                           gradle_version='7.4',
                                           sdk_build_tools_version='30.0.3',
                                           jdk_version='11',
                                           release_note_url='https://developer.android.com/build/releases/past-releases/agp-7-3-0-release-notes'),

    '7.2': AndroidGradlePluginRequirements(agp_version='7.2',
                                           gradle_version='7.3.3',
                                           sdk_build_tools_version='30.0.3',
                                           jdk_version='11',
                                           release_note_url='https://developer.android.com/build/releases/past-releases/agp-7-2-0-release-notes'),

    '7.1': AndroidGradlePluginRequirements(agp_version='7.1',
                                           gradle_version='7.2',
                                           sdk_build_tools_version='30.0.3',
                                           jdk_version='11',
                                           release_note_url='https://developer.android.com/build/releases/past-releases/agp-7-1-0-release-notes'),

    '7.0': AndroidGradlePluginRequirements(agp_version='7.0',
                                           gradle_version='7.0.2',
                                           sdk_build_tools_version='30.0.2',
                                           jdk_version='11',
                                           release_note_url='https://developer.android.com/build/releases/past-releases/agp-7-0-0-release-notes'),

    '4.2': AndroidGradlePluginRequirements(agp_version='4.2',
                                           gradle_version='6.7.1',
                                           sdk_build_tools_version='30.0.2',
                                           jdk_version='8',
                                           release_note_url='https://developer.android.com/build/releases/past-releases/agp-4-2-0-release-notes'),
}

# Determine the latest version of the gradle plugin
MAX_ANDROID_GRADLE_PLUGIN_VER = Version(sorted(list(ANDROID_GRADLE_PLUGIN_COMPATIBILITY_MAP.keys()), reverse=True)[0])


def get_android_gradle_plugin_requirements(requested_agp_version:str) -> AndroidGradlePluginRequirements:
    """
    Lookup up a specific Android Gradle Plugin (AGP) requirements based on a specific version of the AGP
    :param requested_agp_version:   The version of the AGP to look for the requirements
    :return: The instance of AndroidGradlePluginRequirements for the specific version of AGP
    """

    global ANDROID_GRADLE_PLUGIN_COMPATIBILITY_MAP

    # The lookup map is only based on the major and minor version
    lookup_version = Version(requested_agp_version)
    lookup_version_str = f'{lookup_version.major}.{lookup_version.minor}'

    if Version(lookup_version_str) > MAX_ANDROID_GRADLE_PLUGIN_VER:
        raise AndroidToolError(f"Android gradle plugin version {requested_agp_version} is newer than the latest version supported ({MAX_ANDROID_GRADLE_PLUGIN_VER}).")

    matched_agp_requirements = ANDROID_GRADLE_PLUGIN_COMPATIBILITY_MAP.get(lookup_version_str)
    if not matched_agp_requirements:
        raise AndroidToolError(f"Unrecognized/unsupported android gradle plugin version {requested_agp_version} specified. Supported Versions are "
            f"{', '.join(f'{version_key}.0' for version_key in ANDROID_GRADLE_PLUGIN_COMPATIBILITY_MAP.keys())}")
    return matched_agp_requirements


class AndroidSDKManager(object):
    """
    Class to manage the android sdk manager command-line tool which is required to generate and process O3DE
    android projects.
    """

    class BasePackage(object):
        """
        Base package with the common properties for all the android sdk packages
        """
        def __init__(self, components: List):
            """
            Initialize the base package

            :param components:  Dictionary of the components to extract the common properties from
            """

            self._path = components[0]
            self._version = Version(components[1].strip().replace(' ', '.'))  # Fix for versions that have spaces between the version number and potential non-numeric versioning (PEP-0440)
            self._description = components[2]

        @property
        def path(self)->str:
            return self._path

        @property
        def version(self)->Version:
            return self._version

        @property
        def description(self) -> str:
            return self._description

    class InstalledPackage(BasePackage):
        """
        This class represents an installed package
        """
        def __init__(self, installed_package_components):
            super().__init__(installed_package_components)
            assert len(installed_package_components) == 4, '4 sections expected for installed package components (path, version, description, location)'
            self._location = installed_package_components[3]

        @property
        def location(self) -> str:
            return self._location


    class AvailablePackage(BasePackage):
        """
        This class represents an available package (for install)
        """
        def __init__(self, available_package_components):
            super().__init__(available_package_components)
            assert len(available_package_components) == 3, '3 sections expected for installed package components (path, version, description)'

    class AvailableUpdate(BasePackage):
        """
        This class represents an installed package that has an available update
        """
        def __init__(self, available_update_components):
            super().__init__(available_update_components)
            assert len(available_update_components) == 3, '3 sections expected for installed package components (path, version, available)'

    # Enums that track the type of package information that we are tracking
    class PackageCategory(Enum):
        AVAILABLE = 1
        INSTALLED = 2
        UPDATABLE = 3

    def __init__(self, current_java_version: str, android_settings: command_utils.O3DEConfig):
        """
        Initialize the Android SDK Manager

        :param current_java_version:    The current version of java used by gradle
        :param android_settings:        The current android settings
        """

        # Validate the android sdk path is set, contains the command line tools at the expected location, and that it is compatible with the
        # input java version
        self._installed_packages = {}
        self._available_packages = {}
        self._available_updates = {}
        self._android_command_line_tools_sdkmanager_path, self.android_sdk_path = \
            AndroidSDKManager.validate_android_sdk_environment(current_java_version=current_java_version,
                                                               android_settings=android_settings)

        self.refresh_sdk_installation()

    @staticmethod
    def validate_android_sdk_environment(current_java_version: str, android_settings: command_utils.O3DEConfig) -> Path:
        """
        From the android sdk value (android_sdk_root) from the settings, validate that it is set to a valid location,
        contains the command line tools at the expected location, and that it is compatible with the input java version

        :param current_java_version:    The version of java to validate the version of the command line tools (if any)
        :param android_settings:        The android settings to read the android sdk path from
        :return: The path to the android sdk command line tools (in its expected location)
        """

        # Validate the android sdk folder was set to a valid location
        android_sdk_root = android_settings.get_value(SETTINGS_SDK_ROOT.key)
        if not android_sdk_root:
            raise AndroidToolError(f"The android sdk path was not set. Set the value of '{SETTINGS_SDK_ROOT}' to the path where the android sdk base is located.")
        android_sdk_root_path = Path(android_sdk_root)
        if not android_sdk_root_path.is_dir():
            raise AndroidToolError(f"The android sdk path '{SETTINGS_SDK_ROOT}' is set to an invalid path '{android_sdk_root_path}'. Folder does not exist.")

        # Make sure that the android command line tool is installed (unzipped) to the expected location under the specified android SDK
        android_sdk_command_line_tools_root = android_sdk_root_path / 'cmdline-tools' / 'latest'
        android_sdk_command_line_tool = android_sdk_command_line_tools_root / 'bin' / f'sdkmanager{SDKMANAGER_EXTENSION}'
        if not android_sdk_command_line_tool.is_file():
            raise AndroidToolError(f"Unable to locate the android sdk command line tool from the android sdk root set at '{android_sdk_root_path}'. "
                                   f"Make sure that it is installed at {android_sdk_command_line_tools_root}.")

        # Retrieve the version if possible to validate it can be used
        command_arg = [android_sdk_command_line_tool, '--version']
        logging.debug("Validating tool version exec: (%s)", subprocess.list2cmdline(command_arg))
        result = subprocess.run(command_arg,
                                shell=(platform.system() == 'Windows'),
                                capture_output=True,
                                encoding=DEFAULT_READ_ENCODING,
                                errors=ENCODING_ERROR_HANDLINGS,
                                cwd=android_sdk_command_line_tool.parent)

        if result.returncode != 0:

            # Check if the error is caused by a mismatched version of java that was used to run the tool
            match = re.search(r'(class file version\s)([\d\.]+)', result.stdout or result.stderr, re.MULTILINE)
            if match:
                # The error is related to a mismatched version of java.
                java_file_version = match.group(2)
                cmdline_tool_java_version = JAVA_CLASS_FILE_MAP.get(java_file_version.split('.')[0], None)
                if cmdline_tool_java_version:
                    raise AndroidToolError(f"The android SDK command line tool requires java version {cmdline_tool_java_version} but the current version of java is {current_java_version}")
                else:
                    raise AndroidToolError(f"The android SDK command line tool requires java that supports class version {java_file_version}, but the current version of java is {current_java_version}")
            elif re.search('Could not determine SDK root', result.stdout or result.stderr, re.MULTILINE) is not None:
                # The error is related to the android SDK not being able to be resolved.
                raise AndroidToolError(f"The android SDK command line tool at {android_sdk_command_line_tool} is not located under a valid Android SDK root.\n"
                                       "It must exist under a path designated as the Android SDK root, such as: \n\n"
                                       f"<android_sdk>/cmdline-tools/latest/sdkmanager{SDKMANAGER_EXTENSION}' \n\n"
                                       "Refer to https://developer.android.com/tools/sdkmanager for more information.\n")
            else:
                raise AndroidToolError(f"An error occurred attempt to run the android command line tool:\n{result.stdout or result.stderr}")
        else:
            logger.info(f'Verified Android SDK Manager version {result.stdout.strip()}')
            logger.info(f'Verified Android SDK path at {android_sdk_root_path}')
            return android_sdk_command_line_tool, android_sdk_root_path

    def call_sdk_manager(self, arguments):
        """
        Perform the command line call to the sdk manager

        :param arguments:   The arguments to pass to the SDK manager
        :return: The subprocess.Result of the call
        """
        assert isinstance(arguments, list)

        command_args = [self._android_command_line_tools_sdkmanager_path]
        command_args.extend(arguments)

        logger.debug(f"Calling sdkmanager:  {subprocess.list2cmdline(arguments)}")

        result = subprocess.run(command_args,
                                shell=(platform.system() == 'Windows'),
                                capture_output=True,
                                encoding=DEFAULT_READ_ENCODING,
                                errors=ENCODING_ERROR_HANDLINGS)
        return result

    def get_package_list(self, search_package_path: str, packageCategory: PackageCategory) -> List:
        """
        Query for the package information that this manager class maintains based a search query (that uses file patterns)
        on the category of packages (INSTALLED, AVAILABLE, UPDATEABLE)

        :param search_package_path: The path search pattern (file pattern) to search for the package
        :param packageCategory:     The category of packages to search on
        :return:    The list of packages that meet the search path and category inputs
        """

        def _package_sort(package):
            # Sort by version number of the package
            return package.version

        package_detail_result_list = []

        match packageCategory:
            case AndroidSDKManager.PackageCategory.INSTALLED:
                package_dict = self._installed_packages
            case AndroidSDKManager.PackageCategory.AVAILABLE:
                package_dict = self._available_packages
            case AndroidSDKManager.PackageCategory.UPDATABLE:
                package_dict = self._available_updates
            case _:
                assert False, "Unsupported package category"

        for installed_package_path, installed_package_details in package_dict.items():
            if fnmatch.fnmatch(installed_package_path, search_package_path):
                package_detail_result_list.append(installed_package_details)
        package_detail_result_list.sort(reverse=True, key=_package_sort)
        return package_detail_result_list

    def refresh_sdk_installation(self):
        """
        Utilize the sdk_manager command line tool from the Android SDK to collect / refresh the list of
        installed, available, and updateable packages that are managed by the android SDK.
        """
        self._installed_packages = {}
        self._available_packages = {}
        self._available_updates = {}

        def _factory_installed_package(package_map, item_components):
            package_map[item_components[0]] = AndroidSDKManager.InstalledPackage(item_components)

        def _factory_available_package(package_map, item_components):
            package_map[item_components[0]] = AndroidSDKManager.AvailablePackage(item_components)

        def _factory_available_update(package_map, item_components):
            package_map[item_components[0]] = AndroidSDKManager.AvailableUpdate(item_components)

        # Use the SDK manager to collect the available and installed packages
        logger.info("Refreshing installed and available packages...")
        result = self.call_sdk_manager(['--list'])
        if result.returncode != 0:
            raise AndroidToolError(f"Error calling sdkmanager (arguments: '--list' error : {result.stderr or result.stdout}")
        result_lines = result.stdout.split('\n')

        current_append_map = None
        current_item_factory = None
        for package_item in result_lines:
            package_item_stripped = package_item.strip()
            if not package_item_stripped:
                continue
            if '|' not in package_item_stripped:
                if package_item_stripped.upper() == 'INSTALLED PACKAGES:':
                    current_append_map = self._installed_packages
                    current_item_factory = _factory_installed_package
                elif package_item_stripped.upper() == 'AVAILABLE PACKAGES:':
                    current_append_map = self._available_packages
                    current_item_factory = _factory_available_package
                elif package_item_stripped.upper() == 'AVAILABLE UPDATES:':
                    current_append_map = self._available_updates
                    current_item_factory = _factory_available_update
                else:
                    current_append_map = None
                    current_item_factory = None
                continue
            item_parts = [split.strip() for split in package_item_stripped.split('|')]
            if len(item_parts) < 3:
                continue
            elif item_parts[1].upper() in ('VERSION', 'INSTALLED', '-------'):
                continue
            elif current_append_map is None:
                continue
            if current_append_map is not None and current_item_factory is not None:
                current_item_factory(current_append_map, item_parts)

        logger.info(f"Installed packages: {len(self._installed_packages)}")
        logger.info(f"Available packages: {len(self._available_packages)}")
        logger.info(f"Available updates: {len(self._available_updates)}")

    def install_package(self, package_install_path: str, package_description: str):
        """
        Install a package based on the path of an available android sdk package

        :param package_install_path:    The path (as reported by the sdk manager) of the package to install locally
        :param package_description:     A human-readable description to report back status and/or errors from the install operation
        """

        # Skip installation if the package is already installed
        package_result_list = self.get_package_list(package_install_path, AndroidSDKManager.PackageCategory.INSTALLED)
        if package_result_list:
            installed_package_detail = package_result_list[0]
            logger.info(f"{installed_package_detail.description} (version {installed_package_detail.version}) Detected")
            return installed_package_detail

        # Make sure the package name is available
        package_result_list = self.get_package_list(package_install_path, AndroidSDKManager.PackageCategory.AVAILABLE)
        if not package_result_list:
            raise AndroidToolError(f"Invalid Android SDK Package {package_description}: Bad package path {package_install_path}")

        # Reverse sort and pick the first item, which should be the latest (if the install path contains wildcards)
        def _available_sort(item):
            return item.path

        package_result_list.sort(reverse=True, key=_available_sort)

        available_package_to_install = package_result_list[0]  # For multiple hits, resolve to the first item which will be the latest version

        # Perform the package installation
        logger.info(f"Installing {available_package_to_install.description} ...")
        self.call_sdk_manager(['--install', available_package_to_install.path])

        # Refresh the tracked SDK Contents
        self.refresh_sdk_installation()

        # Get the package details to verify
        package_result_list = self.get_package_list(package_install_path, AndroidSDKManager.PackageCategory.INSTALLED)
        if package_result_list:
            installed_package_detail = package_result_list[0]
            logger.info(f"{installed_package_detail.description} (version {installed_package_detail.version}) Installed")
            return installed_package_detail
        else:
            raise AndroidToolError(f"Unable to verify package at {available_package_to_install.path}")

    LICENSE_NOT_ACCEPTED_REGEX = re.compile(r'(^\d of \d.*$)', re.MULTILINE)

    LICENSE_ACCEPTED_REGEX = re.compile(r'(^All SDK package licenses accepted\.$)', re.MULTILINE)

    def check_licenses(self):
        """
        Make sure that all the android SDK licenses have been reviewed and accepted first, otherwise raise an exception
        that provides information on how to accept them.
        """
        logger.info("Checking Android SDK Package licenses state..")
        result = subprocess.run(f"echo 'Y' | \"{self._android_command_line_tools_sdkmanager_path}\" --licenses",
                                shell=True,
                                capture_output=True,
                                encoding=DEFAULT_READ_ENCODING,
                                errors=ENCODING_ERROR_HANDLINGS,
                                timeout=5)
        license_not_accepted_match = AndroidSDKManager.LICENSE_NOT_ACCEPTED_REGEX.search(result.stdout or result.stderr)
        if license_not_accepted_match:
            raise AndroidToolError(f"{license_not_accepted_match.group(1)}\n"
                                   f"Please run '{self._android_command_line_tools_sdkmanager_path} --licenses' and follow the instructions.\n")
        license_accepted_match = AndroidSDKManager.LICENSE_ACCEPTED_REGEX.search(result.stdout or result.stderr)
        if license_accepted_match:
            logger.info(license_accepted_match.group(1))
        else:
            raise AndroidToolError("Unable to determine the Android SDK Package license state. \n"
                                   f"Please run '{self._android_command_line_tools_sdkmanager_path} --licenses' to troubleshoot the issue.\n")

    def get_android_sdk_path(self) -> str:
        return self.android_sdk_path


def read_android_settings_for_project(project_path:Path) -> Tuple[dict, dict]:
    """
    Read the general project and android specific settings for the project, and return the general project settings
    from project.json, and the android specific settings from 'Platform/Android/android_project.json'

    :param project_path:
    :return: Tuple of the project settings, and the android-specific settings
    """
    if not project_path.is_dir():
        raise AndroidToolError(f"Invalid project path {project_path}. Path does not exist or is a file.")

    # Read the project.json first
    project_json_path = project_path / 'project.json'
    if not project_json_path.is_file():
        raise AndroidToolError(f"Invalid project path {project_path}. Path does not contain 'project.json.")
    project_json_content = project_json_path.read_text(encoding=DEFAULT_READ_ENCODING,
                                                       errors=ENCODING_ERROR_HANDLINGS)
    project_settings = json.loads(project_json_content)
    # Read the android_project.json next. If it does not exist, check if this is a legacy project where the android
    # settings are in the root project.json
    android_project_json_file = project_path / 'Platform' / 'Android' / 'android_project.json'
    if android_project_json_file.is_file():
        android_project_json_content = android_project_json_file.read_text(encoding=DEFAULT_READ_ENCODING,
                                                                           errors=ENCODING_ERROR_HANDLINGS)
        android_json = json.loads(android_project_json_content)
        android_settings = android_json.get('android_settings')
        if android_settings is None:
            raise AndroidToolError(f"Missing android settings in file {android_project_json_file}")
        android_settings = android_json['android_settings']
    else:
        android_settings = project_settings.get('android_settings')
        if android_settings is None:
            raise AndroidToolError(f"Missing android settings file {android_project_json_file} and cannot located legacy "
                                   f"'android_settings' from {project_json_path}")

    return project_settings, android_settings


class AndroidSigningConfig(object):
    """
    Class to manage android signing configs section in a gradle build script
    """
    def __init__(self, store_file:Path, store_password:str, key_alias:str, key_password:str):
        """
        Initialize this android signing config object

        :param store_file:      The path to the key store file used for the signing configuration
        :param store_password:  The password to the key store file
        :param key_alias:       The alias of the private signing key in the key store
        :param key_password:    The password to the private signing key in the key store referenced by the key alias
        """

        self._store_file = Path(store_file)
        self._store_password = store_password
        self._key_alias = key_alias
        self._key_password = key_password

    def to_template_string(self, tabs) -> str:
        """
        Generate a string that represents a signing config section that can be inserted into a gradle script.

        :param tabs:    The number of leading tabs for each line to insert for better readability of the gradle script
        :return:    The signing config string section to insert
        """
        tab_prefix = ' '* 4 * tabs  # 4 spaces per tab
        return f"{tab_prefix}storeFile file('{self._store_file.as_posix()}')\n" \
               f"{tab_prefix}storePassword '{self._store_password}'\n" \
               f"{tab_prefix}keyPassword '{self._key_password}'\n" \
               f"{tab_prefix}keyAlias '{self._key_alias}'"


JAVA_VERSION_REGEX = re.compile(r'.*(\w)\s(version)\s*\"?(?P<version>[\d\_\.]+)', re.MULTILINE)


def validate_java_environment() -> str:
    """
    Java is required in order to build android projects with gradle. Check if java is either on the system PATH
    environment, or set by the JAVA_HOME environment variable and return the version string. This is the same
    java search approach used by the sdkmanager.
    """

    java_home = os.getenv('JAVA_HOME', None)
    if java_home:
        java_exe_working_dir = os.path.join(java_home,'bin')
        java_exe_path = os.path.join(java_exe_working_dir, f'java{EXE_EXTENSION}')
        if not os.path.isfile(java_exe_path):
            raise AndroidToolError(f"JAVA_HOME is set to an invalid location ({java_home}). There is no java executable located in that path.")
    else:
        java_exe_working_dir = None

    result = subprocess.run([f'java{EXE_EXTENSION}', '-version'],
                            shell=(platform.system() == 'Windows'),
                            capture_output=True,
                            encoding=DEFAULT_READ_ENCODING,
                            errors=ENCODING_ERROR_HANDLINGS,
                            cwd=java_exe_working_dir)

    if result.returncode != 0:
        if java_exe_working_dir:
            raise AndroidToolError(f"Unable to determine java version from {java_exe_working_dir} ({result.stderr or result.stdout})")
        else:
            raise AndroidToolError(f"Unable to locate java. Either set it in the PATH environment or set JAVA_HOME to a valid java installation. ({result.stderr or result.stdout})")

    java_version_match = JAVA_VERSION_REGEX.search(result.stdout or result.stderr)
    if java_version_match is None:
        raise AndroidToolError(f"Unable to determine java version")

    java_version = java_version_match.group('version')
    if java_home:
        logger.info(f"Detected java version {java_version} (from JAVA_HOME)")
    else:
        logger.info(f"Detected java version {java_version}")

    return java_version


def validate_build_tool(tool_name: str, tool_command: str, tool_config_key: str or None, tool_environment_var: str, tool_config_sub_path: str or None, tool_version_arg: str,
                        version_regex: str, android_config: command_utils.O3DEConfig) -> (Path, str):
    """
    Perform a tool validation by checking on its version number if possible

    :param tool_name:               The name of the tool to display for status purposes
    :param tool_command:            The name of the tool command that is executed
    :param tool_config_key:         Optional. If provided, check against the path represented by this key for the tool, otherwise the tool must exist in the PATH environment
    :param tool_environment_var:    Optional. If provided, check if there is an environment variable that matches this value and use to check the tool
    :param tool_config_sub_path:    Optional. The sub path that leads to the binary from the value of the tool home folder
    :param tool_version_arg:        The argument to pass to the <tool_command> to query for its version
    :param version_regex:           The regex to search for the raw version string from the result of the version query command
    :param android_config:          The configuration to look up the <tool_config_key> if necessary
    :return: Tuple of the full tool path, and its raw version string
    """

    # Locate the tool command. The order of precedence for the search is: 'tool_config_key', 'tool_environment_var', PATH (None)
    tool_home_and_src_list = []
    if tool_config_key:
        config_home = android_config.get_value(tool_config_key)
        if config_home:
            tool_home_and_src_list.append( (config_home, f'configration {tool_config_key}') )
    if tool_environment_var:
        env_home = os.getenv(tool_environment_var)
        if env_home:
            tool_home_and_src_list.append( (env_home, f'environment variable {tool_environment_var}') )
    tool_home_and_src_list.append( (None, None) )
    tool_located = False

    for tool_home, tool_home_src in tool_home_and_src_list:
        if tool_home is not None:
            tool_test_command = os.path.join(tool_home, tool_config_sub_path, tool_command)
        else:
            tool_test_command = tool_command

        # Run the command to get the tool version
        try:
            result = subprocess.run([tool_test_command, tool_version_arg],
                                    shell=(platform.system() == 'Windows'),
                                    capture_output=True,
                                    encoding=DEFAULT_READ_ENCODING,
                                    errors=ENCODING_ERROR_HANDLINGS)
            if result.returncode == 0:
                if tool_home:
                    tool_full_path = Path(tool_home) / tool_config_sub_path / tool_command
                else:
                    tool_full_path = Path(shutil.which(tool_command))
                tool_located = True
                break
            else:
                logger.warning(f"Unable to resolve tool {tool_name} from {tool_home_src}")
        except FileNotFoundError:
            logger.warning(f"Unable to resolve tool {tool_name} from {tool_home_src}")

    if not tool_located:
        error_msgs = [f"Unable to resolve {tool_name}. Make sure its installed and in the PATH environment"]
        if tool_config_key:
            error_msgs.append(f', or the {tool_config_key} settings')
        if tool_environment_var:
            error_msgs.append(f', or the {tool_environment_var} environment variable')
        error_msgs.append('.')
        raise AndroidToolError(''.join(error_msgs))

    # Extract the version number from the results of running the command with the version argument
    tool_version_match = re.search(version_regex, result.stdout, re.MULTILINE)
    if tool_version_match is None:
        raise AndroidToolError(f"Unable to determine {tool_name} version: {result.stdout or result.stderr}")
    tool_version = tool_version_match.group(2)

    # Report the status
    logger.info(f"Detected {tool_name} version {tool_version} (From {tool_full_path})")
    return tool_full_path, tool_version


def validate_gradle(android_config) -> (Path, str):
    """
    Specialization of validate_build_tool for Gradle
    :param android_config:      The android configuration to get the tool home if needed
    :return:    Tuple of : The full path of the tool and the tool version
    """
    return validate_build_tool(tool_name='Gradle',
                               tool_command=f'gradle{GRADLE_EXTENSION}',
                               tool_config_key=SETTINGS_GRADLE_HOME.key,
                               tool_environment_var='GRADLE_HOME',
                               tool_config_sub_path='bin',
                               tool_version_arg='--version',
                               version_regex=r'.*(Gradle)\s*\"?([\d\_\.]+)',
                               android_config=android_config)


def validate_cmake(android_config) -> (Path, str):
    """
    Specialization of validate_build_tool for CMake
    :param android_config:      The android configuration to get the tool home if needed
    :return:    Tuple of : The full path of the tool and the tool version
    """
    return validate_build_tool(tool_name='Cmake',
                               tool_command=f'cmake{EXE_EXTENSION}',
                               tool_config_key=SETTINGS_CMAKE_HOME.key,
                               tool_environment_var='CMAKE_HOME',
                               tool_config_sub_path='bin',
                               tool_version_arg='--version',
                               version_regex=r'.*(cmake version)\s*\"?([\d\_\.]+)',
                               android_config=android_config)


def validate_ninja(android_config) -> (Path, str):
    """
    Specialization of validate_build_tool for Ninja Build
    :param android_config:      The android configuration to get the tool home if needed
    :return:    Tuple of : The full path of the tool and the tool version
    """
    return validate_build_tool(tool_name='Ninja',
                               tool_command=f'ninja{EXE_EXTENSION}',
                               tool_config_key=None,
                               tool_environment_var=None,
                               tool_config_sub_path='',
                               tool_version_arg='--version',
                               version_regex=r'(\s*)([\d\_\.]+)',
                               android_config=android_config)


PLATFORM_SETTINGS_FORMAT = """
# Auto Generated from last cmake project generation ({generation_timestamp})

[settings]
platform={platform}
game_projects={project_path}
asset_deploy_mode={asset_mode}

[android]
android_sdk_path={android_sdk_path}
android_gradle_plugin={android_gradle_plugin_version}
"""

PROJECT_DEPENDENCIES_VALUE_FORMAT = """
dependencies {{
{dependencies}
    api 'androidx.core:core:1.1.0'
}}
"""

NATIVE_CMAKE_SECTION_ANDROID_FORMAT = """
    externalNativeBuild {{
        cmake {{
            buildStagingDirectory "{native_build_path}"
            version "{cmake_version}"
            path "{absolute_cmakelist_path}"
        }}
    }}
"""

NATIVE_CMAKE_SECTION_DEFAULT_CONFIG_NDK_FORMAT_STR = """
        ndk {{
            abiFilters '{abi}'
        }}
"""

OVERRIDE_JAVA_SOURCESET_STR = """
            java {{
                srcDirs = ['{absolute_azandroid_path}', 'src/main/java']
            }}
"""

NATIVE_CMAKE_SECTION_BUILD_TYPE_CONFIG_FORMAT_STR = """
            externalNativeBuild {{
                cmake {{
                    {targets_section}
                    arguments {arguments}
                }}
            }}
"""

CUSTOM_APPLY_ASSET_LAYOUT_TASK_FORMAT_STR = """
    task syncLYLayoutMode{config}(type:Exec) {{
        workingDir '{working_dir}'
        commandLine {full_command_line}
    }}

    process{config}MainManifest.dependsOn syncLYLayoutMode{config}

    syncLYLayoutMode{config}.mustRunAfter {{
        tasks.findAll {{ task->task.name.contains('strip{config}DebugSymbols') }}
    }}
    
    merge{config}Assets.dependsOn syncLYLayoutMode{config}
"""

DEFAULT_CONFIG_CHANGES = [
    'keyboard',
    'keyboardHidden',
    'orientation',
    'screenSize',
    'smallestScreenSize',
    'screenLayout',
    'uiMode',
]

# Android Orientation Constants
ORIENTATION_LANDSCAPE = 1 << 0
ORIENTATION_PORTRAIT = 1 << 1
ORIENTATION_ALL = (ORIENTATION_LANDSCAPE | ORIENTATION_PORTRAIT)

ORIENTATION_FLAG_TO_KEY_MAP = {
    ORIENTATION_LANDSCAPE: 'land',
    ORIENTATION_PORTRAIT: 'port',
}

ORIENTATION_MAPPING = {
    'landscape': ORIENTATION_LANDSCAPE,
    'reverseLandscape': ORIENTATION_LANDSCAPE,
    'sensorLandscape': ORIENTATION_LANDSCAPE,
    'userLandscape': ORIENTATION_LANDSCAPE,
    'portrait': ORIENTATION_PORTRAIT,
    'reversePortrait': ORIENTATION_PORTRAIT,
    'sensorPortrait': ORIENTATION_PORTRAIT,
    'userPortrait': ORIENTATION_PORTRAIT
}

MIPMAP_PATH_PREFIX = 'mipmap'

APP_ICON_NAME = 'app_icon.png'
APP_SPLASH_NAME = 'app_splash.png'


APP_NAME = 'app'
ANDROID_MANIFEST_FILE = 'AndroidManifest.xml'
ANDROID_LIBRARIES_JSON_FILE = 'android_libraries.json'

ANDROID_LAUNCHER_NAME_PATTERN = "{project_name}.GameLauncher"

class AndroidProjectManifestEnvironment(object):
    """
    This class manages the environment for the AndroidManifest.xml template file, based on project settings and environments
    that were passed in or calculated from the command line arguments.
    """

    def __init__(self, project_path: Path, project_settings: dict, android_settings: dict, android_gradle_plugin_version: Version, android_platform_sdk_api_level: str, oculus_project: bool):
        """
        Initialize the object with the project specific parameters and values for the game project

        :param project_path:                    The path were the project is located
        :param android_settings:                The android settings to key of custom values
        :param android_gradle_plugin_version:   The version of the android gradle plugin
        :param android_platform_sdk_api_level:  The android SDK platform version
        :param oculus_project:                  Indicates if it's an oculus project
        """

        # The project name is required
        project_name = project_settings.get('project_name')
        if not project_name:
            raise AndroidToolError(f"Invalid project settings for project at '{project_path}'. Missing required 'project_name' key")

        # The product name is optional. If 'product_name' is missing for the project, fallback to the project name
        product_name = project_settings.get('product_name', project_name)

        # The 'package_name' setting for android settings is required
        package_name = android_settings.get('package_name')
        if not package_name:
            raise AndroidToolError(f"Invalid android settings for project at '{project_path}'. Missing required 'package_name' key in the android settings.")
        package_path = package_name.replace('.', '/')

        project_activity = f'{project_name}Activity'

        # Multiview options require special processing
        multi_window_options = AndroidProjectManifestEnvironment.process_android_multi_window_options(android_settings)

        oculus_intent_filter_category = '<category android:name="com.oculus.intent.category.VR" />' if oculus_project else ''

        self.internal_dict = {
            'ANDROID_PACKAGE':                  package_name,
            'ANDROID_PACKAGE_PATH':             package_path,
            'ANDROID_VERSION_NUMBER':           android_settings["version_number"],
            "ANDROID_VERSION_NAME":             android_settings["version_name"],
            "ANDROID_SCREEN_ORIENTATION":       android_settings["orientation"],
            'ANDROID_APP_NAME':                 product_name,       # external facing name
            'ANDROID_PROJECT_NAME':             project_name,     # internal facing name
            'ANDROID_PROJECT_ACTIVITY':         project_activity,
            'ANDROID_LAUNCHER_NAME':            ANDROID_LAUNCHER_NAME_PATTERN.format(project_name=project_name),
            'ANDROID_CONFIG_CHANGES':           multi_window_options['ANDROID_CONFIG_CHANGES'],
            'ANDROID_APP_PUBLIC_KEY':           android_settings.get('app_public_key', 'NoKey'),
            'ANDROID_APP_OBFUSCATOR_SALT':      android_settings.get('app_obfuscator_salt', ''),
            'ANDROID_USE_MAIN_OBB':             android_settings.get('use_main_obb', 'false'),
            'ANDROID_USE_PATCH_OBB':            android_settings.get('use_patch_obb', 'false'),
            'ANDROID_ENABLE_KEEP_SCREEN_ON':    android_settings.get('enable_keep_screen_on', 'false'),
            'ANDROID_DISABLE_IMMERSIVE_MODE':   android_settings.get('disable_immersive_mode', 'false'),
            'ANDROID_TARGET_SDK_VERSION':       android_platform_sdk_api_level,
            'ICONS':                            android_settings.get('icons', None),
            'SPLASH_SCREEN':                    android_settings.get('splash_screen', None),

            'ANDROID_MULTI_WINDOW':             multi_window_options['ANDROID_MULTI_WINDOW'],
            'ANDROID_MULTI_WINDOW_PROPERTIES':  multi_window_options['ANDROID_MULTI_WINDOW_PROPERTIES'],

            'SAMSUNG_DEX_KEEP_ALIVE':           multi_window_options['SAMSUNG_DEX_KEEP_ALIVE'],
            'SAMSUNG_DEX_LAUNCH_WIDTH':         multi_window_options['SAMSUNG_DEX_LAUNCH_WIDTH'],
            'SAMSUNG_DEX_LAUNCH_HEIGHT':        multi_window_options['SAMSUNG_DEX_LAUNCH_HEIGHT'],

            'OCULUS_INTENT_FILTER_CATEGORY':    oculus_intent_filter_category,
            'ANDROID_MANIFEST_PACKAGE_OPTION':  '' if android_gradle_plugin_version >= Version('7.0') else f'package="{package_name}"'
        }

    def __getitem__(self, item):
        return self.internal_dict.get(item)

    @staticmethod
    def process_android_multi_window_options(game_project_android_settings):
        """
        Perform custom processing for game projects that have custom 'multi_window_options' in their project.json definition
        :param game_project_android_settings:   The parsed out android settings from the game's project.json
        :return: Dictionary of attributes for any optional multiview option detected from the android settings
        """

        def is_number_option_valid(value, name):
            if value:
                if isinstance(value, int):
                    return True
                else:
                    logging.warning('[WARN] Invalid value for property "%s", expected whole number', name)
            return False

        def get_int_attribute(settings, key_name):
            settings_value = settings.get(key_name, None)
            if not settings_value:
                return None
            if not isinstance(settings_value, int):
                logging.warning('[WARN] Invalid value for property "%s", expected whole number', key_name)
                return None
            return settings_value

        multi_window_options = {
            'SAMSUNG_DEX_LAUNCH_WIDTH':         '',
            'SAMSUNG_DEX_LAUNCH_HEIGHT':        '',
            'SAMSUNG_DEX_KEEP_ALIVE':           '',
            'ANDROID_CONFIG_CHANGES':           '|'.join(DEFAULT_CONFIG_CHANGES),
            'ANDROID_MULTI_WINDOW_PROPERTIES':  '',
            'ANDROID_MULTI_WINDOW':             '',
            'ORIENTATION':                      ORIENTATION_ALL
        }

        multi_window_settings = game_project_android_settings.get('multi_window_options', None)
        if not multi_window_settings:
            # If there are no multi-window options, then set the orientation to the orientation attribute if set, otherwise use the default 'ALL' orientation
            requested_orientation = game_project_android_settings['orientation']
            multi_window_options['ORIENTATION'] = ORIENTATION_MAPPING.get(requested_orientation, ORIENTATION_ALL)
            return multi_window_options

        launch_in_fullscreen = False

        # the Samsung DEX specific values can be added regardless of target API and multi-window support
        samsung_dex_options = multi_window_settings.get('samsung_dex_options', None)
        if samsung_dex_options:
            launch_in_fullscreen = samsung_dex_options.get('launch_in_fullscreen', False)

            # setting the launch window size in DEX mode since launching in fullscreen is strictly tied
            # to multi-window being enabled
            launch_width = get_int_attribute(samsung_dex_options, 'launch_width')
            launch_height = get_int_attribute(samsung_dex_options, 'launch_height')

            # both have to be specified otherwise they are ignored
            if launch_width and launch_height:
                multi_window_options['SAMSUNG_DEX_LAUNCH_WIDTH'] = (f'<meta-data '
                                                                    f'android:name="com.samsung.android.sdk.multiwindow.dex.launchwidth" '
                                                                    f'android:value="{launch_width}"'
                                                                    f'/>')

                multi_window_options['SAMSUNG_DEX_LAUNCH_HEIGHT'] = (f'<meta-data '
                                                                     f'android:name="com.samsung.android.sdk.multiwindow.dex.launchheight" '
                                                                     f'android:value="{launch_height}"'
                                                                     f'/>')

                keep_alive = samsung_dex_options.get('keep_alive', None)
                if keep_alive in (True, False):
                    multi_window_options['SAMSUNG_DEX_KEEP_ALIVE'] = f'<meta-data ' \
                                                                     f'android:name="com.samsung.android.keepalive.density" ' \
                                                                     f'android:value="{str(keep_alive).lower()}" ' \
                                                                     f'/>'

        multi_window_enabled = multi_window_settings.get('enabled', False)

        # the option to change the display resolution was added in API 24 as well, these changes are sent as density changes
        multi_window_options['ANDROID_CONFIG_CHANGES'] = '|'.join(DEFAULT_CONFIG_CHANGES + ['density'])

        # if targeting above the min API level the default value for this attribute is true so we need to explicitly disable it
        multi_window_options['ANDROID_MULTI_WINDOW'] = f'android:resizeableActivity="{str(multi_window_enabled).lower()}"'

        if not multi_window_enabled:
            return multi_window_options

        # remove the DEX launch window size if requested to launch in fullscreen mode
        if launch_in_fullscreen:
            multi_window_options['SAMSUNG_DEX_LAUNCH_WIDTH'] = ''
            multi_window_options['SAMSUNG_DEX_LAUNCH_HEIGHT'] = ''

        default_width = multi_window_settings.get('default_width', None)
        default_height = multi_window_settings.get('default_height', None)

        min_width = multi_window_settings.get('min_width', None)
        min_height = multi_window_settings.get('min_height', None)

        gravity = multi_window_settings.get('gravity', None)

        layout = ''
        if any([default_width, default_height, min_width, min_height, gravity]):
            layout = '<layout '

            # the default width/height values are respected as launch values in DEX mode so they should
            # be ignored if the intention is to launch in fullscreen when running in DEX mode
            if not launch_in_fullscreen:
                if is_number_option_valid(default_width, 'default_width'):
                    layout += f'android:defaultWidth="{default_width}dp" '

                if is_number_option_valid(default_height, 'default_height'):
                    layout += f'android:defaultHeight="{default_height}dp" '

            if is_number_option_valid(min_height, 'min_height'):
                layout += f'android:minHeight="{min_height}dp" '

            if is_number_option_valid(min_width, 'min_width'):
                layout += f'android:minWidth="{min_width}dp" '

            if gravity:
                layout += f'android:gravity="{gravity}" '

            layout += '/>'

        multi_window_options['ANDROID_MULTI_WINDOW_PROPERTIES'] = layout

        return multi_window_options


class AndroidProjectGenerator(object):
    """
    Class the manages the process to generate an android project folder in order to build with gradle/android studio
    """

    def __init__(self, engine_root: Path, android_build_dir: Path, android_sdk_path: Path, android_build_tool_version: str, android_platform_sdk_api_level: str,
                 android_ndk_package: str, project_name: str, project_path: Path, project_general_settings: dict, project_android_settings: dict,
                 cmake_path: Path, cmake_version: str, gradle_path: Path, gradle_version: str, gradle_custom_jvm_args: str, android_gradle_plugin_version: str,
                 ninja_path: Path, asset_mode:str, signing_config: AndroidSigningConfig or None, extra_cmake_configure_args: str, src_pak_file_path: str,
                 strip_debug_symbols: bool = False, overwrite_existing: bool = True, oculus_project: bool = False):
        """
        Initialize the object with all the required parameters needed to create an Android Project. The parameters should be verified before initializing this object
        
        :param engine_root:                     The Path location of the Engine
        :param android_build_dir:               The Path of the target folder where the android project will be created 
        :param android_sdk_path:                The Path to the Android SDK Root used to generate and process the android build script.
        :param android_build_tool_version:      The Android SDK build-tool version.
        :param android_platform_sdk_api_level:  The Android Platform SDK API Level to use for the android build
        :param android_ndk_package:             The Android NDK package version to use
        :param project_name:                    The name of the project the android build script is being generated for.
        :param project_path:                    The Path to the root of the project that the android build script is being generated for.
        :param project_general_settings:        The general project settings (from <project_path>/project.json) for the project (to get the legacy android settings if specified)
        :param project_android_settings:        The android project settings (from <project_path>/Platform/Android/android_project.json) to get the android settings
        :param cmake_path:                      The path to cmake to use for the native android build
        :param cmake_version:                   The version of cmake to use for the native android build
        :param gradle_path:                     The path to gradle to use for the android gradle build script
        :param gradle_version:                  The path to gradle to use for the android gradle build script
        :param android_gradle_plugin_version:   The version of the android gradle plugin to specify for the android build script
        :param ninja_path:                      The path to the ninja-build tool needed for the native android build step
        :param asset_mode:                      The asset mode to use when applying the asset layout (see ASSET_MODES)
        :param signing_config:                  The optional signing config to embed in the android build script. (Required for APK signing)
        :param extra_cmake_configure_args:      The optional additional cmake arguments to pass down to the cmake project generation command for the native code
        :param src_pak_file_path:               The sub-path from the project root to where the bundled pak files are expected
        :param strip_debug_symbols:             Option to strip the debug symbols from the native built libraries
        :param overwrite_existing:              Option to overwrite the any existing build script
        :param oculus_project:                  Option to indicate that we are building the android script for oculus devices.
        """

        # General properties
        self._engine_root = engine_root
        self._build_dir = android_build_dir

        # Android SDK Properties
        self._android_sdk_path = android_sdk_path
        self._android_sdk_build_tool_version = android_build_tool_version
        self._android_ndk = android_ndk_package
        self._android_platform_sdk_api_level = android_platform_sdk_api_level

        # Target project properties
        self._project_name = project_name
        self._project_path = project_path
        self._project_general_settings = project_general_settings
        self._project_android_settings = project_android_settings

        # Build tool related properties
        self._cmake_version = cmake_version
        self._cmake_path = cmake_path
        self._extra_cmake_configure_args = extra_cmake_configure_args
        self._gradle_path = gradle_path
        self._gradle_version = gradle_version
        self._gradle_plugin_version = Version(android_gradle_plugin_version)
        self._gradle_custom_jvm_args = gradle_custom_jvm_args
        self._ninja_path = ninja_path
        self._strip_debug_symbols = strip_debug_symbols

        # Asset type and location properties
        self._src_pak_file_path = src_pak_file_path
        self._asset_mode = asset_mode

        # Gradle script related properties
        self._android_project_builder_path = self._engine_root / 'Code/Tools/Android/ProjectBuilder'
        self._native_build_path = 'o3de'
        self._signing_config = signing_config
        self._is_oculus_project = oculus_project

        self._overwrite_existing = overwrite_existing
        self._android_replacement_map_env = {}

    def execute(self):
        """
        Execute the android project creation workflow
        """
        # If we are using asset PAK mode, then make sure we have pak files, and warn if they are missing
        if self._asset_mode == ASSET_MODE_PAK:
            src_pak_file_full_path = self._project_path / self._src_pak_file_path
            if not src_pak_file_full_path.is_dir():
                logger.warning(f"Android PAK files are expected at location {src_pak_file_full_path}, but the folder doesnt exist. Make sure "
                                "to create release bundles (PAK files) before building and deploying to an android device. Refer to "
                                "https://www.docs.o3de.org/docs/user-guide/packaging/asset-bundler/bundle-assets-for-release/ for more "
                                "information.")
            else:
                pak_count = 0
                for pak_dir_item in src_pak_file_full_path.iterdir():
                    if pak_dir_item.name.lower().endswith('_android.pak'):
                        pak_count += 1
                if pak_count == 0:
                    logger.warning(f"Android PAK files are expected at location {src_pak_file_full_path}, but none was detected. Make sure "
                                    "to create release bundles (PAK files) before building and deploying to an android device. Refer to "
                                    "https://www.docs.o3de.org/docs/user-guide/packaging/asset-bundler/bundle-assets-for-release/ for more "
                                    "information.")

        # Prepare the working build directory
        self._build_dir.mkdir(parents=True, exist_ok=True)

        self.create_platform_settings()

        self.create_default_local_properties()

        project_names = self.patch_and_transfer_android_libs()

        project_names.extend(self.create_lumberyard_app(project_names))

        root_gradle_env = {
            'ANDROID_GRADLE_PLUGIN_VERSION': str(self._gradle_plugin_version),
            'SDK_VER': self._android_platform_sdk_api_level,
            'MIN_SDK_VER': self._android_platform_sdk_api_level,
            'NDK_VERSION': self._android_ndk.version,
            'SDK_BUILD_TOOL_VER': self._android_sdk_build_tool_version,
            'LY_ENGINE_ROOT': self._engine_root.as_posix()
        }
        # Generate the gradle build script
        self.create_file_from_project_template(src_template_file='root.build.gradle.in',
                                               template_env=root_gradle_env,
                                               dst_file=self._build_dir / 'build.gradle')

        self.write_settings_gradle(project_names)

        self.prepare_gradle_wrapper()

        logger.info(f"Android project scripts written to '{self._build_dir.absolute()}'.")

    def create_file_from_project_template(self, src_template_file, template_env, dst_file):
        """
        Create a file from an android template file

        :param src_template_file:       The name of the template file that is located under Code/Tools/Android/ProjectBuilder
        :param template_env:            The dictionary that contains the template substitution values
        :param dst_file:                The target concrete file to write to
        """

        src_template_file_path = self._android_project_builder_path / src_template_file
        if not dst_file.exists() or self._overwrite_existing:

            default_local_properties_content = utils.load_template_file(template_file_path=src_template_file_path,
                                                                        template_env=template_env)

            dst_file.write_text(default_local_properties_content,
                                encoding=DEFAULT_WRITE_ENCODING,
                                errors=ENCODING_ERROR_HANDLINGS)

            logger.info('Generated default {}'.format(dst_file.name))
        else:
            logger.info('Skipped {} (file exists)'.format(dst_file.name))

    def prepare_gradle_wrapper(self):
        """
        Generate the gradle wrapper by calling the validated version of gradle.
        """
        logger.info('Preparing Gradle Wrapper')

        if self._gradle_path:
            gradle_wrapper_cmd = [self._gradle_path]
        else:
            gradle_wrapper_cmd = ['gradle']

        gradle_wrapper_cmd.extend(['wrapper', '-p', str(self._build_dir.resolve())])

        proc_result = subprocess.run(gradle_wrapper_cmd,
                                     shell=(platform.system() == 'Windows'))
        if proc_result.returncode != 0:
            raise AndroidToolError(f"Gradle was unable to generate a gradle wrapper for this project (code {proc_result.returncode}): {proc_result.stderr or ''}")


    def create_platform_settings(self):
        """
        Create the 'platform.settings' file for the deployment script to use
        """
        platform_settings_content = PLATFORM_SETTINGS_FORMAT.format(generation_timestamp=str(datetime.datetime.now().strftime("%c")),
                                                                    platform='android',
                                                                    project_path=self._project_path,
                                                                    asset_mode=self._asset_mode,
                                                                    android_sdk_path=str(self._android_sdk_path),
                                                                    android_gradle_plugin_version=self._gradle_plugin_version)

        platform_settings_file = self._build_dir / 'platform.settings'

        # Check if there already exists the build folder and a 'platform.settings' file. If there is an android gradle
        # plugin version set, and it is different from the one configured here, we will always overwrite it since
        # there could be significant differences from one plug-in to the next
        if platform_settings_file.is_file():
            config = configparser.ConfigParser()
            config.read([str(platform_settings_file.resolve(strict=True))])
            if config.has_option('android', 'android_gradle_plugin'):
                exist_agp_version = config.get('android', 'android_gradle_plugin')
                if exist_agp_version != str(self._gradle_plugin_version):
                    self._overwrite_existing = True

        platform_settings_file.open('w').write(platform_settings_content)

    def create_default_local_properties(self):
        """
        Create the default 'local.properties' file in the build folder
        """
        if self._cmake_path:
            # The cmake dir references the base cmake folder, not the executable path itself, so resolve to the base folder
            template_cmake_path = Path(self._cmake_path).parents[1].as_posix()
        else:
            template_cmake_path = None

        local_properties_env = {
            "GENERATION_TIMESTAMP": str(datetime.datetime.now().strftime("%c")),
            "ANDROID_SDK_PATH": self._android_sdk_path.resolve().as_posix(),
            "CMAKE_DIR_LINE": f'cmake.dir={template_cmake_path}' if template_cmake_path else ''
        }

        self.create_file_from_project_template(src_template_file='local.properties.in',
                                               template_env=local_properties_env,
                                               dst_file=self._build_dir / 'local.properties')

    def patch_and_transfer_android_libs(self):
        """
        Patch and transfer android libraries from the android SDK path based on the rules outlined in Code/Tools/Android/ProjectBuilder/android_libraries.json
        """

        # The android_libraries.json is templatized and needs to be provided the following environment for processing
        # before we can process it.
        android_libraries_substitution_table = {
            "ANDROID_SDK_HOME": self._android_sdk_path.as_posix(),
            "ANDROID_SDK_VERSION": f"android-{self._android_platform_sdk_api_level}"
        }
        android_libraries_template_json_path = self._android_project_builder_path / ANDROID_LIBRARIES_JSON_FILE

        android_libraries_template_json_content = android_libraries_template_json_path.resolve(strict=True) \
                                                                                      .read_text(encoding=DEFAULT_READ_ENCODING,
                                                                                                 errors=ENCODING_ERROR_HANDLINGS)

        android_libraries_json_content = string.Template(android_libraries_template_json_content) \
                                               .substitute(android_libraries_substitution_table)

        android_libraries_json = json.loads(android_libraries_json_content)

        # Process the android library rules
        libs_to_patch = []

        for libName, value in android_libraries_json.items():
            # The library is in different places depending on the revision, so we must check multiple paths.
            src_dir = None
            for path in value['srcDir']:
                path = string.Template(path).substitute(self._android_replacement_map_env)
                if os.path.exists(path):
                    src_dir = path
                    break

            if not src_dir:
                failed_paths = ", ".join(string.Template(path).substitute(self._android_replacement_map_env) for path in value['srcDir'])
                raise AndroidToolError(f'Failed to find library - {libName} - in path(s) [{failed_paths}]. Please download the '
                                          'library from the Android SDK Manager and run this command again.')

            if 'patches' in value:
                lib_to_patch = self._Library(name=libName,
                                             path=src_dir,
                                             overwrite_existing=self._overwrite_existing,
                                             gradle_plugin_version=self._gradle_plugin_version,
                                             signing_config=self._signing_config)
                for patch in value['patches']:
                    file_to_patch = self._File(patch['path'])
                    for change in patch['changes']:
                        line_num = change['line']
                        old_lines = change['old']
                        new_lines = change['new']
                        for oldLine in old_lines[:-1]:
                            change = self._Change(line_num, oldLine, (new_lines.pop() if new_lines else None))
                            file_to_patch.add_change(change)
                            line_num += 1
                        else:
                            change = self._Change(line_num, old_lines[-1], ('\n'.join(new_lines) if new_lines else None))
                            file_to_patch.add_change(change)

                    lib_to_patch.add_file_to_patch(file_to_patch)
                    lib_to_patch.dependencies = value.get('dependencies', [])
                    lib_to_patch.build_dependencies = value.get('buildDependencies', [])

                libs_to_patch.append(lib_to_patch)

        patched_lib_names = []

        # Patch the libraries
        for lib in libs_to_patch:
            lib.process_patch_lib(android_project_builder_path=self._android_project_builder_path,
                                  dest_root=self._build_dir)
            patched_lib_names.append(lib.name)

        return patched_lib_names

    def create_lumberyard_app(self, project_dependencies):
        """
        This will create the main lumberyard 'app' which will be packaged as an APK.

        :param project_dependencies:    Local project dependencies that may have been detected previously during construction of the android project folder
        :returns    List (of one) project name for the gradle build properties (see write_settings_gradle)
        """

        az_android_dst_path = self._build_dir / APP_NAME

        # We must always delete 'src' any existing copied AzAndroid projects since building may pick up stale java sources
        lumberyard_app_src = az_android_dst_path / 'src'
        if lumberyard_app_src.exists():
            # special case the 'assets' directory before cleaning the whole directory tree
            utils.remove_link(lumberyard_app_src / 'main' / 'assets')
            utils.remove_dir_path(lumberyard_app_src)

        logging.debug("Copying AzAndroid to '%s'", az_android_dst_path.resolve())

        # The folder structure from the base lib needs to be mapped to a structure that gradle can process as a
        # build project, and we need to generate some additional files

        # Prepare the target folder
        az_android_dst_path.mkdir(parents=True, exist_ok=True)

        # Prepare the 'PROJECT_DEPENDENCIES' environment variable
        gradle_project_dependencies = [f"    api project(path: ':{project_dependency}')" for project_dependency in project_dependencies]

        template_engine_root = self._engine_root.as_posix()
        template_project_path = self._project_path.as_posix()
        template_ndk_path = (self._android_sdk_path / self._android_ndk.location).resolve().as_posix()

        native_build_path = self._native_build_path

        gradle_build_env = dict()

        absolute_cmakelist_path = (self._engine_root / 'CMakeLists.txt').resolve().as_posix()
        absolute_azandroid_path = (self._engine_root / 'Code/Framework/AzAndroid/java').resolve().as_posix()

        gradle_build_env['TARGET_TYPE'] = 'application'
        gradle_build_env['PROJECT_DEPENDENCIES'] = PROJECT_DEPENDENCIES_VALUE_FORMAT.format(dependencies='\n'.join(gradle_project_dependencies))
        gradle_build_env['NATIVE_CMAKE_SECTION_ANDROID'] = NATIVE_CMAKE_SECTION_ANDROID_FORMAT.format(cmake_version=str(self._cmake_version), native_build_path=native_build_path, absolute_cmakelist_path=absolute_cmakelist_path)
        gradle_build_env['NATIVE_CMAKE_SECTION_DEFAULT_CONFIG'] = NATIVE_CMAKE_SECTION_DEFAULT_CONFIG_NDK_FORMAT_STR.format(abi=ANDROID_ARCH)

        gradle_build_env['OVERRIDE_JAVA_SOURCESET'] = OVERRIDE_JAVA_SOURCESET_STR.format(absolute_azandroid_path=absolute_azandroid_path)

        gradle_build_env['OPTIONAL_JNI_SRC_LIB_SET'] = ', "outputs/native-lib"'

        for native_config in BUILD_CONFIGURATIONS:

            native_config_upper = native_config.upper()
            native_config_lower = native_config.lower()

            # Prepare the cmake argument list based on the collected android settings and each build config
            cmake_argument_list = [
                '"-GNinja"',
                f'"-S{template_project_path if self._gradle_plugin_version >= Version("7.0") else str(self._engine_root.as_posix())}"',
                f'"-DCMAKE_BUILD_TYPE={native_config_lower}"',
                f'"-DCMAKE_TOOLCHAIN_FILE={template_engine_root}/cmake/Platform/Android/Toolchain_android.cmake"',
                '"-DLY_DISABLE_TEST_MODULES=ON"',

            ]

            if self._strip_debug_symbols:
                cmake_argument_list.append('"-DLY_STRIP_DEBUG_SYMBOLS=ON"')

            if self._asset_mode == ASSET_MODE_PAK:
                cmake_argument_list.append('"-DLY_ARCHIVE_FILE_SEARCH_MODE=1"')

            cmake_argument_list.extend([
                f'"-DANDROID_NATIVE_API_LEVEL={self._android_platform_sdk_api_level}"',
                f'"-DLY_NDK_DIR={template_ndk_path}"',
                '"-DANDROID_STL=c++_shared"',
                '"-Wno-deprecated"',
                '"-DLY_MONOLITHIC_GAME=ON"'
            ])
            if self._ninja_path:
                cmake_argument_list.append(f'"-DCMAKE_MAKE_PROGRAM={self._ninja_path.as_posix()}"')

            if self._is_oculus_project:
                cmake_argument_list.append('"-DANDROID_USE_OCULUS_OPENXR=ON"')

            if self._gradle_plugin_version < Version('7.0'):
                cmake_argument_list.append(f'"-DLY_PROJECTS={template_project_path}"')

            if self._extra_cmake_configure_args:
                # Extra cmake configure arguments are passed in as a single string separated by spaces and wrapped in quotes.
                # We need to remove the quotes from the beginning and end of the string, and then split the string by spaces.
                extra_cmake_args = self._extra_cmake_configure_args
                if extra_cmake_args.startswith('"') and extra_cmake_args.endswith('"'):
                    extra_cmake_args = extra_cmake_args[1:-1]
                # Splits the arguments by white space but only if it's not in a quoted string (e.g. when specifiying a path with white spaces)
                extra_cmake_configure_arg_list = [f'"{arg}"' for arg in re.split('''\s(?=(?:[^'"]|'[^']*'|"[^"]*")*$)''', extra_cmake_args)]
                cmake_argument_list.extend(extra_cmake_configure_arg_list)

            # Prepare the config-specific section to place the cmake argument list in the build.gradle for the app
            gradle_build_env[f'NATIVE_CMAKE_SECTION_{native_config_upper}_CONFIG'] = \
                NATIVE_CMAKE_SECTION_BUILD_TYPE_CONFIG_FORMAT_STR.format(arguments=','.join(cmake_argument_list),
                                                                         targets_section=f'targets "{self._project_name}.GameLauncher"')

            # Prepare the config-specific section to copy the related .so files that are marked as dependencies for the target
            # (launcher) since gradle will not include them automatically for APK import
            gradle_build_env[f'CUSTOM_GRADLE_COPY_NATIVE_{native_config_upper}_LIB_TASK'] = ''

            # If assets must be included inside the APK do the assets layout under
            # 'main' folder so they will be included into the APK. Otherwise
            # do the layout under a different folder so it's created, but not
            # copied into the APK.
            python_full_path = self._engine_root / 'python' / PYTHON_SCRIPT
            sync_layout_command_line_source = [f'{python_full_path.resolve().as_posix()}',
                                            'android_post_build.py', az_android_dst_path.resolve().as_posix(),  # android_app_root
                                            '--project-root', self._project_path.as_posix(),
                                            '--gradle-version', self._gradle_version,
                                            '--asset-mode', self._asset_mode,
                                            '--asset-bundle-folder', self._src_pak_file_path]

            sync_layout_command_line = ','.join([f"'{arg}'" for arg in sync_layout_command_line_source])

            gradle_build_env[f'CUSTOM_APPLY_ASSET_LAYOUT_{native_config_upper}_TASK'] = \
                CUSTOM_APPLY_ASSET_LAYOUT_TASK_FORMAT_STR.format(working_dir=(self._engine_root / 'cmake/Tools/Platform/Android').resolve().as_posix(),
                                                                 full_command_line=sync_layout_command_line,
                                                                 config=native_config)

            gradle_build_env[f'SIGNING_{native_config_upper}_CONFIG'] = f'signingConfig signingConfigs.{native_config_lower}' if self._signing_config else ''

        if self._signing_config:
            gradle_build_env['SIGNING_CONFIGS'] = f"""
    signingConfigs {{
        debug {{
{self._signing_config.to_template_string(3)}
        }}
        profile {{
{self._signing_config.to_template_string(3)}
        }}
        release {{
{self._signing_config.to_template_string(3)}
        }}
    }}
"""
        else:
            gradle_build_env['SIGNING_CONFIGS'] = ""

        if self._gradle_plugin_version >= Version('7.0'):
            package_namespace = self._project_android_settings['package_name']
            gradle_build_env['PROJECT_NAMESPACE_OPTION'] = f'namespace "{package_namespace}"'
        else:
            gradle_build_env['PROJECT_NAMESPACE_OPTION'] = ''

        az_android_gradle_file = az_android_dst_path / 'build.gradle'
        self.create_file_from_project_template(src_template_file='build.gradle.in',
                                               template_env=gradle_build_env,
                                               dst_file=az_android_gradle_file)

        # Generate a AndroidManifest.xml and write to ${az_android_dst_path}/src/main/AndroidManifest.xml
        dest_src_main_path = az_android_dst_path / 'src/main'
        dest_src_main_path.mkdir(parents=True)
        az_android_package_env = AndroidProjectManifestEnvironment(project_path=self._project_path,
                                                                   project_settings=self._project_general_settings,
                                                                   android_settings=self._project_android_settings,
                                                                   android_gradle_plugin_version=self._gradle_plugin_version,
                                                                   android_platform_sdk_api_level=self._android_platform_sdk_api_level,
                                                                   oculus_project=self._is_oculus_project)
        self.create_file_from_project_template(src_template_file=ANDROID_MANIFEST_FILE,
                                               template_env=az_android_package_env,
                                               dst_file=dest_src_main_path / ANDROID_MANIFEST_FILE)

        # Apply the 'android_builder.json' rules to copy over additional files to the target
        self.apply_android_builder_rules(az_android_dst_path=az_android_dst_path,
                                         az_android_package_env=az_android_package_env)

        self.resolve_icon_overrides(az_android_dst_path=az_android_dst_path,
                                    az_android_package_env=az_android_package_env)

        self.resolve_splash_overrides(az_android_dst_path=az_android_dst_path,
                                      az_android_package_env=az_android_package_env)

        self.clear_unused_assets(az_android_dst_path=az_android_dst_path,
                                 az_android_package_env=az_android_package_env)

        return [APP_NAME]

    def write_settings_gradle(self, project_list):
        """
        Generate and write the 'settings.gradle' and 'gradle.properties file at the root of the android project folder

        :param project_list:    List of dependent projects to include in the gradle build
        """

        settings_gradle_lines = [f"include ':{project_name}'" for project_name in project_list]
        settings_gradle_content = '\n'.join(settings_gradle_lines)
        settings_gradle_file = self._build_dir / 'settings.gradle'
        settings_gradle_file.write_text(settings_gradle_content,
                                        encoding=DEFAULT_READ_ENCODING,
                                        errors=ENCODING_ERROR_HANDLINGS)
        logger.info("Generated settings.gradle -> %s", str(settings_gradle_file.resolve()))

        # Write the default gradle.properties

        # TODO: Add substitution entries here if variables are added to gradle.properties.in
        # Refer to the Code/Tools/Android/ProjectBuilder/gradle.properties.in for reference.
        grade_properties_env = {
            'GRADLE_JVM_ARGS': self._gradle_custom_jvm_args
        }
        gradle_properties_file = self._build_dir / 'gradle.properties'
        self.create_file_from_project_template(src_template_file='gradle.properties.in',
                                               template_env=grade_properties_env,
                                               dst_file=gradle_properties_file)
        logger.info("Generated gradle.properties -> %s", str(gradle_properties_file.resolve()))

    def apply_android_builder_rules(self, az_android_dst_path, az_android_package_env):
        """
        Apply the 'android_builder.json' rule file that was used by WAF to prepare the gradle application apk file.

        :param az_android_dst_path:     The target application folder underneath the android target folder
        :param az_android_package_env:  The template environment to use to process all the source template files
        """

        android_builder_json_path = self._android_project_builder_path / 'android_builder.json'
        android_builder_json_content = utils.load_template_file(template_file_path=android_builder_json_path,
                                                                template_env=az_android_package_env)
        android_builder_json = json.loads(android_builder_json_content)

        # Legacy files that don't need to be copied to the path (not needed for APK processing)
        skip_files = ['wscript']

        def _copy(src_file, dst_path,  dst_is_directory):
            """
            Perform a specialized copy
            :param src_file:    Source file to copy (relative to ${android_project_builder_path})
            :param dst_path:    The destination to copy to
            :param dst_is_directory: Flag to indicate if the destination is a path or a file
            """
            if src_file in skip_files:
                # Filter out files that shouldn't be copied
                return

            src_path = self._android_project_builder_path / src_file
            resolved_src = src_path.resolve(strict=True)

            if puremagic.what(resolved_src) in ('rgb', 'gif', 'pbm', 'ppm', 'tiff', 'rast', 'xbm', 'jpeg', 'bmp', 'png'):
                # If the source file is a binary asset, then perform a copy to the target path
                logging.debug("Copy Binary file %s -> %s", str(src_path.resolve(strict=True)), str(dst_path.resolve()))
                dst_path.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(resolved_src, dst_path.resolve())
            else:
                if dst_is_directory:
                    # If the dst_path is a directory, then we are copying the file to that directory
                    dst_path.mkdir(parents=True, exist_ok=True)
                    dst_file = dst_path / src_file
                else:
                    # Otherwise, we are copying the file to the dst_path directly. A renaming may occur
                    dst_path.parent.mkdir(parents=True, exist_ok=True)
                    dst_file = dst_path

                project_activity_for_game_content = utils.load_template_file(template_file_path=src_path,
                                                                             template_env=az_android_package_env)
                dst_file.write_text(project_activity_for_game_content)
                logging.debug("Copy/Update file %s -> %s", str(src_path.resolve(strict=True)), str(dst_path.resolve()))

        def _process_dict(node, dst_path):
            """
            Process a node from the android_builder.json file
            :param node:        The node to process
            :param dst_path:    The destination path derived from the node
            """

            assert isinstance(node, dict), f"Node for {android_builder_json_path} expected to be a dictionary"

            for key, value in node.items():

                if isinstance(value, str):
                    _copy(key, dst_path / value, False)

                elif isinstance(value, list):
                    for item in value:
                        assert isinstance(node, dict), f"Unexpected type found in '{android_builder_json_path}'.  Only lists of strings are supported"
                        _copy(item, dst_path / key, True)

                elif isinstance(value, dict):
                    _process_dict(value, dst_path / key)
                else:
                    assert False, f"Unexpected type '{type(value)}' found in '{android_builder_json_path}'. Only str, list, and dict is supported"

        _process_dict(android_builder_json, az_android_dst_path)

    def construct_source_resource_path(self, source_path):
        """
        Helper to construct the source path to an asset override such as
        application icons or splash screen images

        :param source_path: Source path or file to attempt to locate
        :return: The path to the resource file
        """
        if os.path.isabs(source_path):
            # Always return itself if the path is already and absolute path
            return Path(source_path)

        game_gem_resources = self._project_path / 'Gem' / 'Resources'
        if game_gem_resources.is_dir(game_gem_resources):
            # If the source is relative and the game gem's resource is present, construct the path based on that
            return game_gem_resources / source_path

        raise AndroidToolError(f'Unable to locate resources folder for project at path "{self._project_path}"')

    def resolve_icon_overrides(self, az_android_dst_path, az_android_package_env):
        """
        Resolve any icon overrides

        :param az_android_dst_path:     The destination android path (app project folder)
        :param az_android_package_env:  Dictionary of env values to retrieve override information
        """

        dst_resource_path = az_android_dst_path / 'src/main/res'

        icon_overrides = az_android_package_env['ICONS']
        if not icon_overrides:
            return

        # if a default icon is specified, then copy it into the generic mipmap folder
        default_icon = icon_overrides.get('default', None)

        if default_icon is not None:

            src_default_icon_file = self.construct_source_resource_path(default_icon)

            default_icon_target_dir = dst_resource_path / MIPMAP_PATH_PREFIX
            default_icon_target_dir.mkdir(parents=True, exist_ok=True)
            dst_default_icon_file = default_icon_target_dir / APP_ICON_NAME

            shutil.copyfile(src_default_icon_file.resolve(), dst_default_icon_file.resolve())
            os.chmod(dst_default_icon_file.resolve(), stat.S_IWRITE | stat.S_IREAD)
        else:
            logging.debug(f'No default icon override specified for project_at path {self._project_path}')

        # process each of the resolution overrides
        warnings = []
        for resolution in ANDROID_RESOLUTION_SETTINGS:

            target_directory = dst_resource_path / f'{MIPMAP_PATH_PREFIX}-{resolution}'
            target_directory.mkdir(parent=True, exist_ok=True)

            # get the current resolution icon override
            icon_source = icon_overrides.get(resolution, default_icon)
            if icon_source is default_icon:

                # if both the resolution and the default are unspecified, warn the user but do nothing
                if icon_source is None:
                    warnings.append(f'No icon override found for "{resolution}".  Either supply one for "{resolution}" or a '
                                    f'"default" in the android_settings "icon" section of the project.json file for {self._project_path}')

                # if only the resolution is unspecified, remove the resolution specific version from the project
                else:
                    logging.debug(f'Default icon being used for "{resolution}" in {self._project_path}', resolution)
                    utils.remove_dir_path(target_directory)
                continue

            src_icon_file = self.construct_source_resource_path(icon_source)
            dst_icon_file = target_directory / APP_ICON_NAME

            shutil.copyfile(src_icon_file.resolve(), dst_icon_file.resolve())
            os.chmod(dst_icon_file.resolve(), stat.S_IWRITE | stat.S_IREAD)

        # guard against spamming warnings in the case the icon override block is full of garbage and no actual overrides
        if len(warnings) != len(ANDROID_RESOLUTION_SETTINGS):
            for warning_msg in warnings:
                logging.warning(warning_msg)

    def resolve_splash_overrides(self, az_android_dst_path, az_android_package_env):
        """
        Resolve any splash screen overrides

        :param az_android_dst_path:     The destination android path (app project folder)
        :param az_android_package_env:  Dictionary of env values to retrieve override information
        """

        dst_resource_path = az_android_dst_path / 'src/main/res'

        splash_overrides = az_android_package_env['SPLASH_SCREEN']
        if not splash_overrides:
            return

        orientation = az_android_package_env['ORIENTATION']
        drawable_path_prefix = 'drawable-'

        for orientation_flag, orientation_key in ORIENTATION_FLAG_TO_KEY_MAP.items():
            orientation_path_prefix = drawable_path_prefix + orientation_key

            oriented_splashes = splash_overrides.get(orientation_key, {})

            unused_override_warning = None
            if (orientation & orientation_flag) == 0:
                unused_override_warning = f'Splash screen overrides specified for "{orientation_key}" when desired orientation ' \
                                          f'is set to "{ORIENTATION_FLAG_TO_KEY_MAP[orientation]}" in project {self._project_path}. ' \
                                          f'These overrides will be ignored.'

            # if a default splash image is specified for this orientation, then copy it into the generic drawable-<orientation> folder
            default_splash_img = oriented_splashes.get('default', None)

            if default_splash_img is not None:
                if unused_override_warning:
                    logging.warning(unused_override_warning)
                    continue

                src_default_splash_img_file = self.construct_source_resource_path(default_splash_img)

                dst_default_splash_img_dir = dst_resource_path / orientation_path_prefix
                dst_default_splash_img_dir.mkdir(parents=True, exist_ok=True)
                dst_default_splash_img_file = dst_default_splash_img_dir / APP_SPLASH_NAME

                shutil.copyfile(src_default_splash_img_file.resolve(), dst_default_splash_img_file.resolve())
                os.chmod(dst_default_splash_img_file.resolve(), stat.S_IWRITE | stat.S_IREAD)
            else:
                logging.debug(f'No default splash screen override specified for "%s" orientation in %s', orientation_key,
                              self._project_path)

            # process each of the resolution overrides
            warnings = []

            # The xxxhdpi resolution is only for application icons, its overkill to include them for drawables... for now
            valid_resolutions = set(ANDROID_RESOLUTION_SETTINGS)
            valid_resolutions.discard('xxxhdpi')

            for resolution in valid_resolutions:
                target_directory = dst_resource_path / f'{orientation_path_prefix}-{resolution}'

                # get the current resolution splash image override
                splash_img_source = oriented_splashes.get(resolution, default_splash_img)
                if splash_img_source is default_splash_img:

                    # if both the resolution and the default are unspecified, warn the user but do nothing
                    if splash_img_source is None:
                        section = f"{orientation_key}-{resolution}"
                        warnings.append(f'No splash screen override found for "{section}".  Either supply one for "{resolution}" '
                                        f'or a "default" in the android_settings "splash_screen-{orientation_key}" section of the '
                                        f'project.json file for {self._project_path}.')
                    else:
                        # if only the resolution is unspecified, remove the resolution specific version from the project
                        logging.debug(f'Default splash screen being used for "{orientation_key}-{resolution}" in {self._project_path}')
                        utils.remove_dir_path(target_directory)
                    continue
                src_splash_img_file = self.construct_source_resource_path(splash_img_source)
                dst_splash_img_file = target_directory / APP_SPLASH_NAME

                shutil.copyfile(src_splash_img_file.resolve(), dst_splash_img_file.resolve())
                os.chmod(dst_splash_img_file.resolve(), stat.S_IWRITE | stat.S_IREAD)

            # guard against spamming warnings in the case the splash override block is full of garbage and no actual overrides
            if len(warnings) != len(valid_resolutions):
                if unused_override_warning:
                    logging.warning(unused_override_warning)
                else:
                    for warning_msg in warnings:
                        logging.warning(warning_msg)

    @staticmethod
    def clear_unused_assets(az_android_dst_path, az_android_package_env):
        """
        micro-optimization to clear assets from the final bundle that won't be used

        :param az_android_dst_path:     The destination android path (app project folder)
        :param az_android_package_env:  Dictionary of env values to retrieve override information
        """

        orientation = az_android_package_env['ORIENTATION']
        if orientation == ORIENTATION_LANDSCAPE:
            path_prefix = 'drawable-land'
        elif orientation == ORIENTATION_PORTRAIT:
            path_prefix = 'drawable-port'
        else:
            return

        # Prepare all the sub-folders to clear
        clear_folders = [path_prefix]
        clear_folders.extend([f'{path_prefix}-{resolution}' for resolution in ANDROID_RESOLUTION_SETTINGS if resolution != 'xxxhdpi'])

        # Clear out the base folder
        dst_resource_path = az_android_dst_path / 'src/main/res'

        for clear_folder in clear_folders:
            target_directory = dst_resource_path / clear_folder
            if target_directory.is_dir():
                logging.debug("Clearing folder %s", target_directory)
                utils.remove_dir_path(target_directory)

    class _Library:
        """
        Library class to manage the library node in android_libraries.json
        """
        def __init__(self, name: str, path: str, overwrite_existing: bool, gradle_plugin_version: str, signing_config=None):
            self.name = name
            self.path = Path(path)
            self.signing_config = signing_config
            self.overwrite_existing = overwrite_existing
            self._gradle_plugin_version = gradle_plugin_version
            self.patch_files = []
            self.dependencies = []
            self.build_dependencies = []

        def add_file_to_patch(self, file):
            self.patch_files.append(file)

        def read_package_namespace(self):
            # Determine the library namespace (required for Android Gradle Plugin 8+)
            library_android_manifest_file = self.path / 'AndroidManifest.xml'
            if not library_android_manifest_file.is_file():
                raise AndroidToolError(f"Missing expected 'AndroidManifest.xml' file from the anroid library package {self.name}")

            library_android_manifest = library_android_manifest_file.read_text(encoding=DEFAULT_READ_ENCODING, errors=ENCODING_ERROR_HANDLINGS)
            match_package_name = re.search(r'[\s]*package="([a-zA-Z\.]+)"', library_android_manifest, re.MULTILINE)
            if not match_package_name:
                raise AndroidToolError(f"Error reading 'AndroidManifest.xml' file from the anroid library package {self.name}. Unable to locate 'package' attribute.")
            return match_package_name.group(1)

        def process_patch_lib(self, android_project_builder_path, dest_root):
            """
            Perform the patch logic on the library node of 'android_libraries.json' (root level)
            :param android_project_builder_path:    Path to the Android/ProjectBuilder path for the templates
            :param dest_root:                       The target android project folder
            """

            # Clear out any existing target path's src and recreate
            dst_path = dest_root / self.name

            dst_path_src = dst_path / 'src'
            if dst_path_src.exists():
                utils.remove_dir_path(dst_path_src)
            dst_path.mkdir(parents=True, exist_ok=True)

            logging.debug("Copying library '{}' to '{}'".format(self.name, dst_path))

            # Determine the library namespace (required for Android Gradle Plugin 8+)
            name_space = self.read_package_namespace()

            # The folder structure from the base lib needs to be mapped to a structure that gradle can process as a
            # build project, and we need to generate some additional files

            # Generate the gradle build script for the library based on the build.gradle.in template file
            gradle_dependencies = []
            if self.build_dependencies:
                gradle_dependencies.extend([f"    api '{build_dependency}'" for build_dependency in self.build_dependencies])
            if self.dependencies:
                gradle_dependencies.extend([f"    api project(path: ':{dependency}')" for dependency in self.dependencies])
            if gradle_dependencies:
                project_dependencies = "dependencies {{\n{}\n}}".format('\n'.join(gradle_dependencies))
            else:
                project_dependencies = ""

            # Prepare an environment for a basic, no-native (cmake) gradle project (java only)
            build_gradle_env = {
                'PROJECT_DEPENDENCIES': project_dependencies,
                'PROJECT_NAMESPACE': name_space,
                'TARGET_TYPE': 'library',
                'NATIVE_CMAKE_SECTION_DEFAULT_CONFIG': '',
                'NATIVE_CMAKE_SECTION_ANDROID': '',
                'NATIVE_CMAKE_SECTION_DEBUG_CONFIG': '',
                'NATIVE_CMAKE_SECTION_PROFILE_CONFIG': '',
                'NATIVE_CMAKE_SECTION_RELEASE_CONFIG': '',
                'OVERRIDE_JAVA_SOURCESET': '',
                'OPTIONAL_JNI_SRC_LIB_SET': '',

                'CUSTOM_APPLY_ASSET_LAYOUT_DEBUG_TASK': '',
                'CUSTOM_APPLY_ASSET_LAYOUT_PROFILE_TASK': '',
                'CUSTOM_APPLY_ASSET_LAYOUT_RELEASE_TASK': '',

                'CUSTOM_GRADLE_COPY_NATIVE_DEBUG_LIB_TASK': '',
                'CUSTOM_GRADLE_COPY_NATIVE_PROFILE_LIB_TASK': '',
                'CUSTOM_GRADLE_COPY_NATIVE_RELEASE_LIB_TASK': '',
                'SIGNING_CONFIGS': '',
                'SIGNING_DEBUG_CONFIG': '',
                'SIGNING_PROFILE_CONFIG': '',
                'SIGNING_RELEASE_CONFIG': '',
                'PROJECT_NAMESPACE_OPTION': ''
            }

            if self._gradle_plugin_version >= Version('7.0'):
                build_gradle_env['PROJECT_NAMESPACE_OPTION'] = f'namespace "{name_space}"' if self._gradle_plugin_version >= Version('7.0') else ''

            build_gradle_content = utils.load_template_file(template_file_path=android_project_builder_path / 'build.gradle.in',
                                                             template_env=build_gradle_env)
            dest_gradle_script_file = dst_path / 'build.gradle'
            if not dest_gradle_script_file.exists() or self.overwrite_existing:
                dest_gradle_script_file.write_text(build_gradle_content,
                                                   encoding=DEFAULT_WRITE_ENCODING,
                                                   errors=ENCODING_ERROR_HANDLINGS)

            src_path = Path(self.path)

            # Prepare a 'src/main' folder
            dst_src_main_path = dst_path / 'src/main'
            dst_src_main_path.mkdir(parents=True, exist_ok=True)

            # Prepare a copy mapping list of tuples to process the copying of files and perform the straight file
            # copying
            library_copy_subfolder_pairs = [('res', 'res'),
                                            ('src', 'java')]

            for copy_subfolder_pair in library_copy_subfolder_pairs:

                src_subfolder = copy_subfolder_pair[0]
                dst_subfolder = copy_subfolder_pair[1]

                # {SRC}/{src_subfolder}/ -> {DST}/src/main/{dst_subfolder}/
                src_library_res_path = src_path / src_subfolder
                if not src_library_res_path.exists():
                    continue
                dst_library_res_path = dst_src_main_path / dst_subfolder
                shutil.copytree(src_library_res_path.resolve(),
                                dst_library_res_path.resolve(),
                                copy_function=shutil.copyfile)

            # Process the files identified for patching
            for file in self.patch_files:

                input_file_path = src_path / file.path
                if file.path == ANDROID_MANIFEST_FILE:
                    # Special case: AndroidManifest.xml does not go under the java/ parent path
                    output_file_path = dst_src_main_path / ANDROID_MANIFEST_FILE
                else:
                    output_subpath = f"java{file.path[3:]}"   # Strip out the 'src' from the library json and replace it with the target 'java' sub-path folder heading
                    output_file_path = dst_src_main_path / output_subpath

                logging.debug("  Patching file '%s'", os.path.basename(file.path))
                with open(input_file_path.resolve()) as input_file:
                    lines = input_file.readlines()

                with open(output_file_path.resolve(), 'w') as outFile:
                    for replace in file.changes:
                        lines[replace.line] = str.replace(lines[replace.line], replace.old,
                                                          (replace.new if replace.new else ""), 1)
                    outFile.write(''.join([line for line in lines if line]))

    class _File:
        """
        Helper class to manage individual files for each library (_Library) and their changes
        """
        def __init__(self, path):
            self.path = path
            self.changes = []

        def add_change(self, change):
            self.changes.append(change)

    class _Change:
        """
        Helper class to manage a change/patch as defined in android_libraries.json
        """
        def __init__(self, line, old, new):
            self.line = line
            self.old = old
            self.new = new
