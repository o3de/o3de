#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
import argparse
import configparser

import json
import logging
import os
import re
import platform
import subprocess
import pathlib

from o3de import manifest, utils, android_support
from getpass import getpass

ENGINE_PATH = pathlib.Path(__file__).parents[3]

print(f"ENGINE_PATH={ENGINE_PATH}")

logging.basicConfig(format=utils.LOG_FORMAT)
logger = logging.getLogger('o3de.android')


def validate_android_config(android_config:android_support.AndroidConfig) -> None:

    logger.setLevel(logging.INFO)

    # Validate Java is installed
    java_version = android_support.validate_java_environment()

    # Validate gradle
    gradle_home, gradle_version = android_support.validate_gradle(android_config)

    # Validate cmake
    cmake_home, cmake_version = android_support.validate_cmake(android_config)

    # Validate ninja
    ninja_home, ninja_version = android_support.validate_ninja(android_config)

    # Validate the versions of gradle and ninja against the configured version of the android gradle plugin
    android_gradle_plugin_ver,_ = android_config.get_value('android_gradle_plugin')
    assert android_gradle_plugin_ver, "Missing 'android_gradle_plugin' from .android_settings"
    android_gradle_requirements = android_support.get_android_gradle_plugin_requirements(android_gradle_plugin_ver)
    android_gradle_requirements.validate_gradle_version(gradle_version)
    android_gradle_requirements.validate_java_version(java_version)

    # Verify the Android SDK setting
    sdk_manager = android_support.AndroidSDKManager(java_version, android_config)

    # Make sure that the licenses have been read and accepted
    sdk_manager.check_licenses()





def validate_environment(global_settings_file:pathlib.Path,
                            project_settings_file:pathlib.Path = None) -> None:
    pass

def list_android_config(android_config:android_support.AndroidConfig) -> None:
    """
    List the current android configuration values to stdout
    """

    all_settings = android_config.get_all_values()

    print("\nO3DE Android settings:\n")

    for item, value, source in all_settings:
        if source:
            print(f"*{item} = {value}")
        else:
            print(f" {item} = {value}")
    if not android_config.is_global:
        print(f"\n* - Specific to {android_config.project_name}")

def configure_android_options(args: argparse) -> int:
    """
    Configure the android platform settings for generating, building, and deploying android projects.
    """

    try:
        is_global = getattr(args,'global',False)
        project_name_arg = getattr(args, 'project', None)

        if is_global:
            if project_name_arg:
                logging.warning(f"Both --global and --project ({project_name_arg}) arguments were provided. The --project argument will "
                                "be ignored and the execution of this command will be based on the global settings.")
            android_config = android_support.AndroidConfig(project_name=None,
                                                           is_global=is_global)
        elif not project_name_arg:

            try:
                project_name, _ = android_support.resolve_project_name_and_path(os.getcwd())
            except android_support.AndroidToolError:
                project_name = None
            if project_name:
                logging.warning("Neither --global not --project arguments were supplied. The execution of this command will be based on "
                                f"the currently detected project ({project_name}).")
                android_config = android_support.AndroidConfig(project_name=project_name,
                                                               is_global=False)
            else:
                logging.warning("Neither --global not --project arguments were supplied. The execution of this command will be based on "
                                f"the global settings.")
                android_config = android_support.AndroidConfig(project_name=None,
                                                               is_global=True)
        else:
            android_config = android_support.AndroidConfig(project_name=project_name_arg,
                                                           is_global=False)


        if args.validate:
            validate_android_config(android_config)
        if args.list:
            list_android_config(android_config)
        if args.set_value:
            android_config.set_config_value_from_expression(args.set_value)

    except android_support.AndroidToolError as err:
        logger.error(str(err))
        return 1
    else:
        return 0


def generate_android_project(args: argparse) -> int:
    try:

        # Create the android configuration
        project_name_arg = getattr(args, 'project', None)
        if project_name_arg is None:
            project_name_arg, _ = android_support.resolve_project_name_and_path(os.getcwd())
        android_config = android_support.AndroidConfig(project_name=project_name_arg,
                                                       is_global=False)

        # Resolve the project path and get the project and android settings
        resolved_project_path = manifest.get_registered(project_name=project_name_arg)
        if not resolved_project_path:
            raise AndroidToolError(f"Project '{project_name_arg}' is not registered with O3DE.")
        project_settings, android_settings = android_support.read_android_settings_for_project(resolved_project_path)

        logger.setLevel(logging.DEBUG)

        # Validate Java is installed
        java_version = android_support.validate_java_environment()

        # Validate gradle
        grade_path, gradle_version = android_support.validate_gradle(android_config)

        # Validate cmake
        cmake_path, cmake_version = android_support.validate_cmake(android_config)

        # Validate ninja
        ninja_path, ninja_version = android_support.validate_ninja(android_config)

        # Validate the engine path

        # Validate the versions of gradle and ninja against the configured version of the android gradle plugin
        android_gradle_plugin_ver, _ = android_config.get_value('android_gradle_plugin')
        assert android_gradle_plugin_ver, "Missing 'android_gradle_plugin' from .android_settings"
        android_gradle_requirements = android_support.get_android_gradle_plugin_requirements(android_gradle_plugin_ver)
        android_gradle_requirements.validate_gradle_version(gradle_version)
        android_gradle_requirements.validate_java_version(java_version)
        sdk_build_tools_version = android_gradle_requirements.sdk_build_tools_version

        # Verify the Android SDK setting
        sdk_manager = android_support.AndroidSDKManager(java_version, android_config)

        # Make sure that the licenses have been read and accepted
        sdk_manager.check_licenses()

        # Make sure we have the extra android packages "market_apk_expansion" and "market_licensing" which is needed by the APK
        sdk_manager.install_package(package_install_path='extras;google;market_apk_expansion',
                                    package_description='Google APK Expansion Library')

        sdk_manager.install_package(package_install_path='extras;google;market_licensing',
                                    package_description='Google Play Licensing Library')

        # Make sure the requested Platform SDK (defined by API Level) is installed
        android_platform_sdk_api_level = args.platform_sdk_api_level
        platform_package_name = f"platforms;android-{android_platform_sdk_api_level}"
        platform_sdk_package = sdk_manager.install_package(package_install_path=platform_package_name,
                                                           package_description=f'Android SDK Platform {android_platform_sdk_api_level}')

        # Make sure the requested Android NDK version is installed
        android_ndk_version = args.ndk_version
        ndk_package_name = f"ndk;{android_ndk_version}"
        ndk_package = sdk_manager.install_package(package_install_path=ndk_package_name,
                                                  package_description=f'Android NDK version {android_ndk_version}')

        # Make sure that the android build tools (based on the spec from the android gradle plugin) is installed
        build_tools_package_name = f"build-tools;{sdk_build_tools_version}"
        sdk_manager.install_package(package_install_path=build_tools_package_name,
                                    package_description=f'Android Build Tools {sdk_build_tools_version}')

        engine_path = ENGINE_PATH
        logger.info(f'Engine Path: {engine_path}')

        project_path = resolved_project_path
        logger.info(f'Project Path: {project_path}')

        # Create the android build folder
        build_folder = pathlib.Path(args.build_dir)
        logger.info(f'Project Android Build Folder : {build_folder}')

        # Check if there is a signing config from the arguments
        signconfig_store_file = getattr(args, 'signconfig_store_file', None)
        signconfig_key_alias = getattr(args, 'signconfig_key_alias', None)
        if signconfig_store_file:
            if not signconfig_key_alias:
                raise android_support.AndroidToolError(f"Missing required '--signconfig-key-alias' argument to accompany the provided '--signconfig-store-file' ({signconfig_store_file})")

            stored_signconfig_store_password, _ = android_config.get_value('signconfig_store_password')
            if stored_signconfig_store_password:
                signconfig_store_password = stored_signconfig_store_password
            else:
                signconfig_store_password = getpass(f"Please enter the key store password for {signconfig_store_file} : ")

            stored_signconfig_key_password, _ = android_config.get_value('signconfig_key_password')
            if stored_signconfig_key_password:
                signconfig_key_password = stored_signconfig_key_password
            else:
                signconfig_key_password = getpass(f"Please enter the password for key referenced by alias {signconfig_key_alias} : ")

            signing_config = android_support.AndroidSigningConfig(store_file=signconfig_store_file,
                                                                  store_password=signconfig_store_password,
                                                                  key_alias=signconfig_key_alias,
                                                                  key_password=signconfig_key_password)
        else:
            signing_config = None

        # For now only support monolithic builds
        monolithic_build = True

        apg = android_support.AndroidProjectGenerator(engine_root=engine_path,
                                                      android_build_dir=build_folder,
                                                      android_sdk_path=sdk_manager.get_android_sdk_path(),
                                                      android_build_tool_version=sdk_build_tools_version,
                                                      android_platform_sdk_api_level=android_platform_sdk_api_level,
                                                      android_ndk_package=ndk_package,
                                                      project_name=project_name_arg,
                                                      project_path=project_path,
                                                      project_general_settings=project_settings,
                                                      project_android_settings=android_settings,
                                                      cmake_version=cmake_version,
                                                      cmake_path=cmake_path,
                                                      gradle_path=grade_path,
                                                      gradle_version=gradle_version,
                                                      android_gradle_plugin_version=android_gradle_plugin_ver,
                                                      ninja_path=ninja_path,
                                                      include_assets_in_apk=True,
                                                      asset_mode=args.asset_mode,
                                                      asset_type=None,
                                                      signing_config=signing_config,
                                                      native_build_path=args.native_build_path,
                                                      vulkan_validation_path=None,
                                                      extra_cmake_configure_args=None,
                                                      overwrite_existing=True,
                                                      monolithic_build=monolithic_build,
                                                      oculus_project=False)
        apg.execute()

    except android_support.AndroidToolError as err:
        logger.error(str(err))
        return 1
    else:
        return 0


def add_args(subparsers) -> None:
    """
    add_args is called to add subparsers arguments to each command such that it can be
    a central python file such as o3de.py.
    It can be run from the o3de.py script as follows
    call add_args and execute: python o3de.py register --engine-path "C:/o3de"
    :param subparsers: the caller instantiates subparsers and passes it in here
    """

    # Read from the android config if possible to try to display default values
    try:
        project_name, project_path = android_support.resolve_project_name_and_path(os.getcwd())
        android_config = android_support.AndroidConfig(project_name=project_name, is_global=False)
    except android_support.AndroidToolError:
        project_name = None
        android_config = android_support.AndroidConfig(project_name=None, is_global=True)

    android_configure_subparser = subparsers.add_parser('android-configure',
                                                        help='Configure the android platform settings for generating, building, and deploying android projects.',
                                                        epilog='Configure the android platform settings for generating, building, and deploying android projects.')

    android_configure_subparser.add_argument('--global', default=False, action='store_true',
                                             help='Used with the configure command, specify whether the settings are project local or global.')
    android_configure_subparser.add_argument('-p', '--project', type=str, required=False,
                                             help="The name of the registered project to configure the settings for. This value is ignored if '--global' was specified."
                                                  "Note: If both '--global' and '-p/--project' is not specified, the script will attempt to deduce the project from the "
                                                  "current working directory if possible")
    android_configure_subparser.add_argument('-l', '--list', default=False, action='store_true',
                                             help='Display the current android settings. ')
    android_configure_subparser.add_argument('--validate', default=False, action='store_true',
                                             help='Validate the settings and values in the android settings. ')
    android_configure_subparser.add_argument('--set-value', type=str, required=False,
                                             help='Set the value for an android setting, using the format <argument>=<value>. For example: \'ndk_version=22.5*\'')

    android_configure_subparser.set_defaults(func=configure_android_options)

    android_generate_subparser = subparsers.add_parser('android-generate',
                                                        help='Generate an android/gradle project.')
    if project_name:
        android_generate_subparser.add_argument('-p', '--project', type=str, required=False,
                                                 help="The name of the registered project to generate the android project for. If not supplied, the current detected project from the "
                                                      f"current path ({project_name}) will be used.")
    else:
        android_generate_subparser.add_argument('-p', '--project', type=str, required=True,
                                                help="The name of the registered project to generate the android project for.")
    android_generate_subparser.add_argument('-B', '--build-dir', type=str, required=True,
                                             help="The location to write the android project scripts to.")

    platform_sdk_api_level, _ = android_config.get_value('platform_sdk_api_level')
    if platform_sdk_api_level:
        android_generate_subparser.add_argument('--platform-sdk-api-level', type=str,
                                                help=f"Specify the platform SDK API Level.\nDefault: {platform_sdk_api_level}",
                                                default=platform_sdk_api_level)
    else:
        android_generate_subparser.add_argument('--platform-sdk-api-level', type=str, required=True,
                                                help="Specify the platform SDK API Level. Set with key 'platform_sdk_api_level'")

    ndk_version, _ = android_config.get_value('ndk_version')
    if ndk_version:
        android_generate_subparser.add_argument('--ndk-version', type=str,
                                                help=f"Specify the android NDK version.\nDefault: {ndk_version}.",
                                                default=ndk_version)
    else:
        android_generate_subparser.add_argument('--ndk-version', type=str, required=True,
                                                help="Specify the android NDK version. Set with key 'ndk_version'")

    signconfig_store_file, _ = android_config.get_value('signconfig_store_file')
    if signconfig_store_file:
        android_generate_subparser.add_argument('--signconfig-store-file', type=str,
                                                help=f"Specify the location of the jks keystore file to enable a signing configuration for this project automatically. "
                                                     f"If set, then a store file password, key alias, and key password will be required as well. "
                                                     f"\nDefault {signconfig_store_file}",
                                                default=signconfig_store_file)
    else:
        android_generate_subparser.add_argument('--signconfig-store-file', type=str,
                                                help=f"Specify the location of the jks keystore file to enable a signing configuration for this project automatically. "
                                                     f"If set, then a store file password, key alias, and key password will be required as well.")

    signconfig_key_alias, _ = android_config.get_value('signconfig_key_alias')
    if signconfig_key_alias:
        android_generate_subparser.add_argument('--signconfig-key-alias', type=str,
                                                help=f"Specify the key alias for the configured jks keystore file if set.\n"
                                                     f"Default: {signconfig_key_alias}.",
                                                default=signconfig_key_alias)
    else:
        android_generate_subparser.add_argument('--signconfig-key-alias', type=str,
                                                help="Specify the key alias for the configured jks keystore file if set.")

    native_build_path, _ = android_config.get_value('native_build_path')
    if not native_build_path:
        native_build_path = '.'
    if native_build_path:
        android_generate_subparser.add_argument('--native-build-path', type=str,
                                                help=f"Specify the optional intermediate build folder to use during the native c++ build process, relative to the main 'app' "
                                                      "folder.\n"
                                                     f"Default: {native_build_path}",
                                                default=native_build_path)

    asset_mode, _ = android_config.get_value('asset_mode')
    if asset_mode:
        android_generate_subparser.add_argument('--native-build-path', type=str,
                                                help=f"The mode of asset deployment to use. ({','.join(android_support.ASSET_MODES)}).\n"
                                                     f"Default: {asset_mode}",
                                                default=native_build_path)
    else:
        android_generate_subparser.add_argument('--native-build-path', type=str,
                                                help=f"The mode of asset deployment to use. ({','.join(android_support.ASSET_MODES)}).\n"
                                                     f"Default: {android_support.ASSET_MODE_LOOSE}",
                                                default=android_support.ASSET_MODE_LOOSE)

    android_generate_subparser.set_defaults(func=generate_android_project)
