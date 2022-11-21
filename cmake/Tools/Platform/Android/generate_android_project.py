#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import argparse
import logging
import os
import pathlib
import platform
import re
import sys

from packaging.version import Version

ROOT_DEV_PATH = os.path.realpath(os.path.join(os.path.dirname(__file__), '..', '..', '..', '..'))
if ROOT_DEV_PATH not in sys.path:
    sys.path.append(ROOT_DEV_PATH)

from cmake.Tools import common
from cmake.Tools.Platform.Android import android_support


O3DE_SCRIPTS_PATH = os.path.join(ROOT_DEV_PATH, 'scripts', 'o3de')
if O3DE_SCRIPTS_PATH not in sys.path:
    sys.path.append(O3DE_SCRIPTS_PATH)

from o3de import manifest


GRADLE_ARGUMENT_NAME = '--gradle-install-path'
GRADLE_MIN_VERSION = Version('6.5')
GRADLE_MAX_VERSION = Version('7.5.1')
GRADLE_VERSION_REGEX = re.compile(r"Gradle\s(\d+.\d+.?\d*)")
GRADLE_EXECUTABLE = 'gradle.bat' if platform.system() == 'Windows' else 'gradle'


def verify_gradle(override_gradle_path=None):
    """
    Verify the installed gradle requirement.
    """
    return common.verify_tool(override_tool_path=override_gradle_path,
                              tool_name='gradle',
                              tool_filename=GRADLE_EXECUTABLE,
                              argument_name=GRADLE_ARGUMENT_NAME,
                              tool_version_argument='-v',
                              tool_version_regex=GRADLE_VERSION_REGEX,
                              min_version=GRADLE_MIN_VERSION,
                              max_version=GRADLE_MAX_VERSION)


CMAKE_ARGUMENT_NAME = '--cmake-install-path'
CMAKE_MIN_VERSION = Version('3.20.0')
CMAKE_VERSION_REGEX = re.compile(r'cmake version (\d+.\d+.?\d*)')
CMAKE_EXECUTABLE = 'cmake'


def verify_cmake(override_cmake_path=None):
    """
    Verify the installed cmake requirement.
    """
    return common.verify_tool(override_tool_path=override_cmake_path,
                              tool_name='cmake',
                              tool_filename=CMAKE_EXECUTABLE,
                              argument_name=CMAKE_ARGUMENT_NAME,
                              tool_version_argument='--version',
                              tool_version_regex=CMAKE_VERSION_REGEX,
                              min_version=CMAKE_MIN_VERSION,
                              max_version=None)


NINJA_ARGUMENT_NAME = '--ninja-install-path'
NINJA_VERSION_REGEX = re.compile(r'(\d+.\d+.?\d*)')
NINJA_EXECUTABLE = 'ninja'


def verify_ninja(override_ninja_path=None):
    """
    Verify the installed ninja requirement.
    """
    return common.verify_tool(override_tool_path=override_ninja_path,
                              tool_name='ninja',
                              tool_filename='ninja',
                              argument_name=NINJA_ARGUMENT_NAME,
                              tool_version_argument='--version',
                              tool_version_regex=NINJA_VERSION_REGEX,
                              min_version=None,
                              max_version=None)


SIGNING_PROFILE_STORE_FILE_ARGUMENT_NAME = "--signconfig-store-file"
SIGNING_PROFILE_STORE_PASSWORD_ARGUMENT_NAME = "--signconfig-store-password"
SIGNING_PROFILE_KEY_ALIAS_ARGUMENT_NAME = "--signconfig-key-alias"
SIGNING_PROFILE_KEY_PASSWORD_ARGUMENT_NAME = "--signconfig-key-password"


def build_optional_signing_profile(store_file, store_password, key_alias, key_password):
    # If none of the arguments are set, then return None and skip the signing config generation
    if not any([store_file, store_password, key_alias, key_password]):
        return
    return android_support.AndroidSigningConfig(store_file=store_file,
                                                store_password=store_password,
                                                key_alias=key_alias,
                                                key_password=key_password)


ANDROID_SDK_ARGUMENT_NAME = '--android-sdk-path'
ANDROID_SDK_PLATFORM_ARGUMENT_NAME = '--android-sdk-platform'
ANDROID_SDK_PREFERRED_TOOL_VER = '--android-sdk-build-tool-version'
ANDROID_SDK_COMMAND_LINE_TOOLS_VER = '--android-sdk-command-line-tools-version'

ANDROID_NATIVE_API_LEVEL = '--android-native-api-level'


MIN_ANDROID_SDK_PLATFORM = 28   # The minimum platform/api level that is supported for the SDK Platform
MIN_NATIVE_API_LEVEL = 24       # The minimum Native API level that is supported for the NDK


ANDROID_NDK_PLATFORM_ARGUMENT_NAME = '--android-ndk-version'

ANDROID_GRADLE_PLUGIN_ARGUMENT_NAME = '--gradle-plugin-version'
ANDROID_GRADLE_MIN_PLUGIN_VERSION = Version("4.2.2")

# Constants for asset-related options for APK generation
INCLUDE_APK_ASSETS_ARGUMENT_NAME = "--include-apk-assets"
ASSET_MODE_ARGUMENT_NAME = "--asset-mode"
ASSET_MODE_PAK = 'PAK'
ASSET_MODE_LOOSE = 'LOOSE'
ASSET_MODE_VFS = 'VFS'
ALL_ASSET_MODES = [ASSET_MODE_PAK, ASSET_MODE_LOOSE, ASSET_MODE_VFS]
ASSET_TYPE_ARGUMENT_NAME = '--asset-type'
DEFAULT_ASSET_TYPE = 'android'

manifest_json = manifest.load_o3de_manifest()
DEFAULT_3RD_PARTY_PATH = pathlib.Path(manifest_json.get('default_third_party_folder', manifest.get_o3de_third_party_folder()))


def wrap_parsed_args(parsed_args):
    """
    Function to add a method to the parsed argument object to transform a long-form argument name to and get the
    parsed values based on the input long form.

    This will allow us to read an argument like '--foo-bar=Orange' by using the built-in method rather than looking for
    the argparsed transformed attribute 'foo_bar'.

    :param parsed_args: The parsed args object to wrap
    """

    def parse_argument_attr(argument):
        argument_attr = argument[2:].replace('-', '_')
        return getattr(parsed_args, argument_attr)

    parsed_args.get_argument = parse_argument_attr


def main(args):
    """
    Perform the main argument processing and execution of the project generator
    :param args:    The arguments to process
    """

    parser = argparse.ArgumentParser(description="Prepare the android studio subfolder")

    # Required Arguments
    parser.add_argument('--engine-root',
                        help='The path to the engine root. Defaults to the current working directory.',
                        default=os.getcwd())

    parser.add_argument('--build-dir',
                        help='The build dir path. It will be concatenated to the project-path using the rules of os.path.join',
                        type=pathlib.Path,
                        required=True)

    parser.add_argument('--third-party-path',
                        help=f'The path to the 3rd Party root directory (defaults to {DEFAULT_3RD_PARTY_PATH})',
                        type=pathlib.Path,
                        default=DEFAULT_3RD_PARTY_PATH)

    parser.add_argument(ANDROID_SDK_ARGUMENT_NAME,
                        help='The path to the android SDK',
                        required=True)

    parser.add_argument('-g', '--project-path',
                        help='The project path to generate an android project',
                        required=True)

    parser.add_argument(ANDROID_SDK_PLATFORM_ARGUMENT_NAME,
                        help=f'The android SDK platform number version to use for the APK. (Minimum {MIN_ANDROID_SDK_PLATFORM})',
                        type=int,
                        default=-1)

    parser.add_argument(ANDROID_NATIVE_API_LEVEL,
                        help=f'The android native API level to use for the APK. If not set, this will default to the android SDK platform. (Minimum {MIN_NATIVE_API_LEVEL})',
                        type=int,
                        default=-1)

    # Override arguments
    parser.add_argument(ANDROID_SDK_COMMAND_LINE_TOOLS_VER,
                        default='latest',
                        help='The android SDK command line tools version.',
                        required=False)

    parser.add_argument(ANDROID_SDK_PREFERRED_TOOL_VER,
                        help='The android SDK build tools version.',
                        required=False)

    parser.add_argument(ANDROID_NDK_PLATFORM_ARGUMENT_NAME,
                        help='The android NDK version',
                        required=False)

    parser.add_argument(GRADLE_ARGUMENT_NAME,
                        help=f'The path to installed gradle. The version of gradle must fall in between {str(GRADLE_MIN_VERSION)} and {str(GRADLE_MAX_VERSION)}.',
                        default=None,
                        required=False)

    parser.add_argument(ANDROID_GRADLE_PLUGIN_ARGUMENT_NAME,
                        help=f'The version of the android gradle plugin to use. Defaults to the minimum version ({ANDROID_GRADLE_MIN_PLUGIN_VERSION})',
                        default=str(ANDROID_GRADLE_MIN_PLUGIN_VERSION))

    parser.add_argument(CMAKE_ARGUMENT_NAME,
                        help=f'The path to cmake build tool if not installed on the system path. The version of cmake must be at least version {str(CMAKE_MIN_VERSION)}.',
                        default=None,
                        required=False)

    parser.add_argument(NINJA_ARGUMENT_NAME,
                        help='The path to the ninja build tool if not installed on the system path.',
                        default=None,
                        required=False)

    parser.add_argument('--native-build-path',
                        help='Custom path to place native build artifacts.',
                        default=None,
                        required=False)

    parser.add_argument('--vulkan-validation-path',
                        help='Override path to where the Vulkan Validation Layers libraries are.  Required for use with NDK r23+',
                        default=None,
                        required=False)
    parser.add_argument('--extra-cmake-configure-args',
                        help='Extra arguments to supply to the cmake configure step',
                        nargs='*')

    # Asset Options
    parser.add_argument(INCLUDE_APK_ASSETS_ARGUMENT_NAME,
                        action='store_true',
                        help='Option to include the game assets when building the APK. If this option is set, you must have the android assets built.')

    parser.add_argument(ASSET_MODE_ARGUMENT_NAME,
                        choices=ALL_ASSET_MODES,
                        default=ASSET_MODE_LOOSE,
                        help=f'Asset Mode (vfs|pak|loose) to use when including assets into the APK. (Defaults to {ASSET_MODE_LOOSE})')

    parser.add_argument(ASSET_TYPE_ARGUMENT_NAME,
                        default=DEFAULT_ASSET_TYPE,
                        help=f'Asset Type to use when including assets into the APK. (Defaults to {DEFAULT_ASSET_TYPE})')

    parser.add_argument('--debug',
                        action='store_true',
                        help='Enable debug logs.')

    # Signing Config options
    parser.add_argument(SIGNING_PROFILE_STORE_FILE_ARGUMENT_NAME,
                        default=None,
                        help='(Optional) If specified, create a signing profile based on this supplied android jks keystore file.')

    parser.add_argument(SIGNING_PROFILE_STORE_PASSWORD_ARGUMENT_NAME,
                        default=None,
                        help='If an android jks keystore file is specified, this is the store password for the keystore.')

    parser.add_argument(SIGNING_PROFILE_KEY_ALIAS_ARGUMENT_NAME,
                        default=None,
                        help='If an android jks keystore file is specified, this is the alias of the signing key in the keystore.')

    parser.add_argument(SIGNING_PROFILE_KEY_PASSWORD_ARGUMENT_NAME,
                        default=None,
                        help='If an android jks keystore file is specified, this is the password of the signing key in the keystore.')

    parser.add_argument('--unit-test',
                        action='store_true',
                        help='Generate a unit test APK instead of a game APK.')

    parser.add_argument('--overwrite-existing',
                        action='store_true',
                        help='Option to overwrite existing scripts in the target build folder if they exist already.')

    parser.add_argument('--enable-unity-build',
                        action='store_true',
                        help='Enable unity build')

    parser.add_argument('--oculus-project',
                        action='store_true',
                        help='Generate android project for Oculus app.')

    parsed_args = parser.parse_args(args)
    wrap_parsed_args(parsed_args)

    # Prepare the logging
    logging.basicConfig(format='%(levelname)s: %(message)s', level=logging.DEBUG if parsed_args.debug else logging.INFO)

    # Verify the gradle requirements
    gradle_version, override_gradle_path = verify_gradle(override_gradle_path=parsed_args.get_argument(GRADLE_ARGUMENT_NAME))
    logging.info("Detected Gradle version %s", str(gradle_version))

    # Verify the cmake requirements
    cmake_version, override_cmake_path = verify_cmake(override_cmake_path=parsed_args.get_argument(CMAKE_ARGUMENT_NAME))
    logging.info("Detected CMake version %s", str(cmake_version))

    # Verify the ninja requirements
    ninja_version, override_ninja_path = verify_ninja(override_ninja_path=parsed_args.get_argument(NINJA_ARGUMENT_NAME))
    logging.info("Detected Ninja version %s", str(ninja_version))

    # Get the android sdk platform version to use from the arguments, but also handle the deprecated argument name
    android_sdk_platform_version = parsed_args.get_argument(ANDROID_SDK_PLATFORM_ARGUMENT_NAME)

    # Get the gradle plugin details and validate against the current environment
    android_gradle_plugin_version = parsed_args.get_argument(ANDROID_GRADLE_PLUGIN_ARGUMENT_NAME)
    android_gradle_plugin = android_support.AndroidGradlePluginInfo(android_gradle_plugin_version)
    logging.info(f"Generating Android Gradle Plugin version {android_gradle_plugin_version} based project")

    if gradle_version < android_gradle_plugin.min_gradle_version:
        raise common.LmbrCmdError(f"The current version of gradle ({gradle_version}) does not satisfy the minimum version "
                                  f"({android_gradle_plugin.min_gradle_version}) needed for the android gradle plugin "
                                  f"({android_gradle_plugin_version}). Please upgrade your gradle.")
    if cmake_version < android_gradle_plugin.min_cmake_version:
        raise common.LmbrCmdError(f"The current version of cmake ({cmake_version}) does not satisfy the minimum version "
                                  f"({android_gradle_plugin.min_cmake_version}) needed for the android gradle plugin "
                                  f"({android_gradle_plugin_version}). Please upgrade your cmake.")
    if android_gradle_plugin.max_cmake_version and cmake_version > android_gradle_plugin.max_cmake_version:
        raise common.LmbrCmdError(f"The current version of cmake ({cmake_version}) exceeds the maximum version "
                                  f"({android_gradle_plugin.max_cmake_version}) of the android gradle plugin "
                                  f"({android_gradle_plugin_version}).")

    # Use the SDK Resolver to make sure the build tools and ndk
    android_sdk = android_support.AndroidSDKResolver(android_sdk_path=parsed_args.get_argument(ANDROID_SDK_ARGUMENT_NAME),
                                                     command_line_tools_version=parsed_args.get_argument(ANDROID_SDK_COMMAND_LINE_TOOLS_VER))

    # If no SDK platform is provided, check for any installed one
    if android_sdk_platform_version < 0:
        android_sdk_platform_version = MIN_ANDROID_SDK_PLATFORM
        installed_android_sdk_platforms = android_sdk.is_package_installed('platforms;*')
        if installed_android_sdk_platforms:
            # If there are installed platforms, check the most recent one
            latest_platform_version = -1
            for installed_android_sdk_platform in installed_android_sdk_platforms:
                platform_number_match = re.match(r'platforms;android-([0-9]*)', installed_android_sdk_platform.path)
                if not platform_number_match:
                    continue
                check_platform_version = int(platform_number_match.group(1))
                if check_platform_version > latest_platform_version:
                    latest_platform_version = check_platform_version
            if latest_platform_version >= MIN_ANDROID_SDK_PLATFORM:
                android_sdk_platform_version = latest_platform_version
    else:
        if android_sdk_platform_version < MIN_ANDROID_SDK_PLATFORM:
            raise common.LmbrCmdError(f"Invalid argument for {ANDROID_SDK_PLATFORM_ARGUMENT_NAME} ({android_sdk_platform_version}). Must be greater than the minimum value supported {MIN_ANDROID_SDK_PLATFORM}.")

    # Get the android native api level from the arguments. Default to the sdk platform version if not provided
    android_native_api_level = parsed_args.get_argument(ANDROID_NATIVE_API_LEVEL)
    if android_native_api_level < 0:
        android_native_api_level = android_sdk_platform_version
    else:
        if android_native_api_level < MIN_NATIVE_API_LEVEL:
            raise common.LmbrCmdError(f"Invalid argument for {ANDROID_NATIVE_API_LEVEL} ({android_native_api_level}). Must be greater than the minimum value supported {MIN_NATIVE_API_LEVEL}.")

    # Check and make sure that the requested sdk platform exists, download if necessary
    platform_package_name = f"platforms;android-{android_sdk_platform_version}"
    android_sdk.install_package(package_install_path=platform_package_name,
                                package_description=f'Android SDK Platform {android_sdk_platform_version}')

    # Make sure we have the extra android packages "market_apk_expansion" and "market_licensing" which is needed by the APK
    android_sdk.install_package(package_install_path='extras;google;market_apk_expansion',
                                package_description='Google APK Expansion Library')

    android_sdk.install_package(package_install_path='extras;google;market_licensing',
                                package_description='Google Play Licensing Library')

    # Install either the requested SDK build tools or the default one for the android gradle plugin version
    build_tools_version = parsed_args.get_argument(ANDROID_SDK_PREFERRED_TOOL_VER) or android_gradle_plugin.default_sdk_build_tools_version
    build_tools_package_name = f'build-tools;{build_tools_version}'
    build_tools_package = android_sdk.install_package(package_install_path=build_tools_package_name,
                                                      package_description='Android SDK Build Tools')

    # Install either the requested NDK version or the default one for the android gradle plugin version
    android_ndk_version = parsed_args.get_argument(ANDROID_NDK_PLATFORM_ARGUMENT_NAME) or android_gradle_plugin.default_ndk_version
    android_ndk_package_name = f'ndk;{android_ndk_version}'
    android_ndk_package = android_sdk.install_package(package_install_path=android_ndk_package_name,
                                                      package_description='Android NDK')

    # Verify the engine root path and project path
    verified_project_path, verified_engine_root = common.verify_project_and_engine_root(project_root=parsed_args.project_path,
                                                                                        engine_root=parsed_args.engine_root)
    is_test_project = parsed_args.unit_test

    # Verify the 3rd Party Root Path
    third_party_path = pathlib.Path(parsed_args.third_party_path)
    if not third_party_path.is_dir():
        raise common.LmbrCmdError(f"Invalid --third-party-path '{parsed_args.third_party_path}'.",
                                  common.ERROR_CODE_INVALID_PARAMETER)

    build_dir = parsed_args.build_dir

    signing_config = build_optional_signing_profile(store_file=parsed_args.get_argument(SIGNING_PROFILE_STORE_FILE_ARGUMENT_NAME),
                                                    store_password=parsed_args.get_argument(SIGNING_PROFILE_STORE_PASSWORD_ARGUMENT_NAME),
                                                    key_alias=parsed_args.get_argument(SIGNING_PROFILE_KEY_ALIAS_ARGUMENT_NAME),
                                                    key_password=parsed_args.get_argument(SIGNING_PROFILE_KEY_PASSWORD_ARGUMENT_NAME))

    logging.debug("Engine Root      : %s", str(verified_engine_root.resolve()))
    logging.debug("Build Path       : %s", str(build_dir.resolve()))

    # Prepare the generator and execute
    generator = android_support.AndroidProjectGenerator(engine_root=verified_engine_root,
                                                        build_dir=build_dir,
                                                        android_sdk_path=android_sdk.android_sdk_path,
                                                        build_tool=build_tools_package,
                                                        android_sdk_platform=android_sdk_platform_version,
                                                        android_native_api_level=android_native_api_level,
                                                        android_ndk=android_ndk_package,
                                                        project_path=verified_project_path,
                                                        third_party_path=third_party_path,
                                                        cmake_version=cmake_version,
                                                        override_cmake_path=override_cmake_path,
                                                        override_gradle_path=override_gradle_path,
                                                        gradle_version=gradle_version,
                                                        gradle_plugin_version=android_gradle_plugin_version,
                                                        override_ninja_path=override_ninja_path,
                                                        include_assets_in_apk=parsed_args.include_apk_assets,
                                                        asset_mode=parsed_args.get_argument(ASSET_MODE_ARGUMENT_NAME),
                                                        asset_type=parsed_args.get_argument(ASSET_TYPE_ARGUMENT_NAME),
                                                        signing_config=signing_config,
                                                        is_test_project=is_test_project,
                                                        overwrite_existing=parsed_args.overwrite_existing,
                                                        unity_build_enabled=parsed_args.enable_unity_build,
                                                        oculus_project=parsed_args.oculus_project,
                                                        native_build_path=parsed_args.native_build_path,
                                                        vulkan_validation_path=parsed_args.vulkan_validation_path,
                                                        extra_cmake_configure_args=parsed_args.extra_cmake_configure_args)
    generator.execute()


if __name__ == '__main__':

    try:
        main(sys.argv[1:])
        exit(0)

    except common.LmbrCmdError as err:
        print(str(err), file=sys.stderr)
        exit(err.code)
