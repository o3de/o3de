#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

import argparse
import logging
import os
import pathlib
import platform
import re
import sys

from distutils.version import LooseVersion

ROOT_DEV_PATH = os.path.realpath(os.path.join(os.path.dirname(__file__), '..', '..', '..', '..'))
if ROOT_DEV_PATH not in sys.path:
    sys.path.append(ROOT_DEV_PATH)

from cmake.Tools import common
from cmake.Tools.Platform.Android import android_support


GRADLE_ARGUMENT_NAME = '--gradle-install-path'
GRADLE_MIN_VERSION = LooseVersion('4.10.1')
GRADLE_MAX_VERSION = LooseVersion('5.6.4')
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
CMAKE_MIN_VERSION = LooseVersion('3.17.0')
CMAKE_VERSION_REGEX = re.compile(r'cmake version (\d+.\d+.?\d*)')
CMAKE_EXECUTABLE = 'cmake.exe' if platform.system() == 'Windows' else 'cmake'


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
NINJA_EXECUTABLE = 'ninja.exe' if platform.system() == 'Windows' else 'ninja'


def verify_ninja(override_ninja_path=None):
    """
    Verify the installed ninja requirement.
    """
    return common.verify_tool(override_tool_path=override_ninja_path,
                              tool_name='ninja',
                              tool_filename='ninja.exe' if platform.system() == 'Windows' else 'ninja',
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
ANDROID_SDK_PLATFORM_ARGUMENT_NAME = '--android-sdk-version'
ANDROID_SDK_PREFERRED_TOOL_VER = '--android-sdk-build-tool-version'


ANDROID_NDK_ARGUMENT_NAME = '--android-ndk-path'
ANDROID_NDK_PLATFORM_ARGUMENT_NAME = '--android-ndk-version'

# Constants for asset-related options for APK generation
INCLUDE_APK_ASSETS_ARGUMENT_NAME = "--include-apk-assets"
ASSET_MODE_ARGUMENT_NAME = "--asset-mode"
ASSET_MODE_PAK = 'PAK'
ASSET_MODE_LOOSE = 'LOOSE'
ASSET_MODE_VFS = 'VFS'
ALL_ASSET_MODES = [ASSET_MODE_PAK, ASSET_MODE_LOOSE, ASSET_MODE_VFS]
ASSET_TYPE_ARGUMENT_NAME = '--asset-type'
DEFAULT_ASSET_TYPE = 'es3'


def wrap_parsed_args(parsed_args):
    """
    Function to add a method to the parsed argument object to transform a long-form argument name to and get the
    parsed values based on the input long form.

    This will allow us to read an argument like '--foo-bar=Orange' by using the built in method rather than looking for
    the argparsed transformed attrobite 'foo_bar'

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

    parser.add_argument('--dev-root',
                        help='The path to the dev root. Defaults to the current working directory.',
                        default=os.getcwd())

    parser.add_argument('--build-dir',
                        help='The build dir subpath from the dev root',
                        required=True)

    parser.add_argument('--third-party-path',
                        help='The path to the 3rd Party root directory',
                        required=True)

    parser.add_argument(ANDROID_NDK_ARGUMENT_NAME,
                        help='The path to the android NDK',
                        required=True)

    parser.add_argument(ANDROID_SDK_ARGUMENT_NAME,
                        help='The path to the android SDK',
                        required=True)

    parser.add_argument(ANDROID_SDK_PLATFORM_ARGUMENT_NAME,
                        help='The android SDK version',
                        required=True)

    parser.add_argument(ANDROID_SDK_PREFERRED_TOOL_VER,
                        help='The preferred android sdk build version (i.e. 28.0.3). Will default to the first one detected under the android sdk',
                        default=None,
                        required=False)

    parser.add_argument(ANDROID_NDK_PLATFORM_ARGUMENT_NAME,
                        help='The android NDK version',
                        required=True)

    parser.add_argument(GRADLE_ARGUMENT_NAME,
                        help=f'The path to installed gradle. The version of gradle must fall in between {str(GRADLE_MIN_VERSION)} and {str(GRADLE_MAX_VERSION)}.',
                        default=None,
                        required=False)

    parser.add_argument(CMAKE_ARGUMENT_NAME,
                        help=f'The path to cmake build tool if not installed on the system path. The version of cmake must be at least version {str(CMAKE_MIN_VERSION)}.',
                        default=None,
                        required=False)

    parser.add_argument(NINJA_ARGUMENT_NAME,
                        help='The path to the ninja build tool if not installed on the system path.',
                        default=None,
                        required=False)

    parser.add_argument('-g', '--game-name',
                        help='The game project to base off of')

    # Asset Options
    parser.add_argument(INCLUDE_APK_ASSETS_ARGUMENT_NAME,
                        action='store_true',
                        help='Option to include the game assets when building the APK. If this option is set, you must have the android assets built.')

    parser.add_argument(ASSET_MODE_ARGUMENT_NAME,
                        choices=ALL_ASSET_MODES,
                        default=ASSET_MODE_LOOSE,
                        help='Asset Mode (vfs|pak|loose) to use when including assets into the APK')

    parser.add_argument(ASSET_TYPE_ARGUMENT_NAME,
                        default=DEFAULT_ASSET_TYPE,
                        help='Asset Type to use when including assets into the APK')

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

    # Verify the android sdk path and sdk version
    verified_android_sdk_platform, verified_android_sdk_path, android_sdk_build_tool_ver = android_support.verify_android_sdk(android_sdk_platform=parsed_args.get_argument(ANDROID_SDK_PLATFORM_ARGUMENT_NAME),
                                                                                                                              argument_name=ANDROID_SDK_ARGUMENT_NAME,
                                                                                                                              override_android_sdk_path=parsed_args.get_argument(ANDROID_SDK_ARGUMENT_NAME),
                                                                                                                              preferred_sdk_build_tools_ver=parsed_args.get_argument(ANDROID_SDK_PREFERRED_TOOL_VER))

    # Verify the android ndk path and ndk version
    verified_android_ndk_platform, verified_android_ndk_path = android_support.verify_android_ndk(android_ndk_platform=parsed_args.get_argument(ANDROID_NDK_PLATFORM_ARGUMENT_NAME),
                                                                                                  argument_name=ANDROID_NDK_ARGUMENT_NAME,
                                                                                                  override_android_ndk_path=parsed_args.get_argument(ANDROID_NDK_ARGUMENT_NAME))
    if parsed_args.unit_test or parsed_args.game_name == android_support.TEST_RUNNER_PROJECT:
        verified_game_name = android_support.TEST_RUNNER_PROJECT
        _, verified_dev_root = common.verify_game_project_and_dev_root(game_name=None,
                                                                       dev_root=parsed_args.dev_root)
        is_test_project = True
    else:
        # Verify the dev-root and game name
        verified_game_name, verified_dev_root = common.verify_game_project_and_dev_root(game_name=parsed_args.game_name,
                                                                                        dev_root=parsed_args.dev_root)
        is_test_project = False

    # Verify the 3rd Party Root Path
    third_party_path = pathlib.Path(parsed_args.third_party_path) / '3rdParty.txt'
    if not third_party_path.is_file():
        raise common.LmbrCmdError("Invalid --third-party-path '{}'. Make sure it exists and contains "
                                  "3rdParty.txt".format(parsed_args.third_party_path),
                                  common.ERROR_CODE_INVALID_PARAMETER)
    third_party_path = third_party_path.parent

    build_dir = verified_dev_root / parsed_args.build_dir

    signing_config = build_optional_signing_profile(store_file=parsed_args.get_argument(SIGNING_PROFILE_STORE_FILE_ARGUMENT_NAME),
                                                    store_password=parsed_args.get_argument(SIGNING_PROFILE_STORE_PASSWORD_ARGUMENT_NAME),
                                                    key_alias=parsed_args.get_argument(SIGNING_PROFILE_KEY_ALIAS_ARGUMENT_NAME),
                                                    key_password=parsed_args.get_argument(SIGNING_PROFILE_KEY_PASSWORD_ARGUMENT_NAME))

    logging.debug("Dev Root         : %s", str(verified_dev_root.resolve()))
    logging.debug("Build Path       : %s", str(build_dir.resolve()))
    logging.debug("Android NDK Path : %s", str(verified_android_ndk_path.resolve()))
    logging.debug("Android SDK Path : %s", str(verified_android_sdk_path.resolve()))

    # Prepare the generator and execute
    generator = android_support.AndroidProjectGenerator(dev_root=verified_dev_root,
                                                        build_dir=build_dir,
                                                        android_sdk_path=verified_android_sdk_path,
                                                        android_ndk_path=verified_android_ndk_path,
                                                        android_sdk_version=verified_android_sdk_platform,
                                                        android_ndk_platform=verified_android_ndk_platform,
                                                        game_name=verified_game_name,
                                                        third_party_path=third_party_path,
                                                        cmake_version=cmake_version,
                                                        override_cmake_path=override_cmake_path,
                                                        override_gradle_path=override_gradle_path,
                                                        override_ninja_path=override_ninja_path,
                                                        android_sdk_build_tool_version=android_sdk_build_tool_ver,
                                                        include_assets_in_apk=parsed_args.get_argument(INCLUDE_APK_ASSETS_ARGUMENT_NAME),
                                                        asset_mode=parsed_args.get_argument(ASSET_MODE_ARGUMENT_NAME),
                                                        asset_type=parsed_args.get_argument(ASSET_TYPE_ARGUMENT_NAME),
                                                        signing_config=signing_config,
                                                        is_test_project=is_test_project)
    generator.execute()


if __name__ == '__main__':

    try:
        main(sys.argv[1:])
        exit(0)

    except common.LmbrCmdError as err:
        print(str(err), file=sys.stderr)
        exit(err.code)
