#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
import argparse
import collections
import logging
import os
import pathlib

from o3de import command_utils, manifest, utils, android_support
from getpass import getpass

ENGINE_PATH = pathlib.Path(__file__).parents[3]
O3DE_SCRIPT_PATH = f"{ENGINE_PATH}{os.sep}scripts{os.sep}o3de{android_support.O3DE_SCRIPT_EXTENSION}"
DEFAULT_ANDROID_BUILD_FOLDER = 'build/android'

logging.basicConfig(format=utils.LOG_FORMAT)
logger = logging.getLogger('o3de.android')

O3DE_COMMAND_CONFIGURE = 'android-configure'
O3DE_COMMAND_GENERATE = 'android-generate'
SIGNCONFIG_ARG_STORE_FILE = '--signconfig-store-file'
SIGNCONFIG_ARG_KEY_ALIAS = '--signconfig-key-alias'

VALIDATION_WARNING_SIGNCONFIG_NOT_SET = "Signing configuration not set."
VALIDATION_WARNING_SIGNCONFIG_INCOMPLETE = "Signing configuration settings is incomplete."
VALIDATION_MISSING_PASSWORD = "Signing configuration password not set."

ValidatedEnv = collections.namedtuple('ValidatedEnv', ['java_version',
                                                       'gradle_home',
                                                       'gradle_version',
                                                       'cmake_path',
                                                       'cmake_version',
                                                       'ninja_path',
                                                       'ninja_version',
                                                       'android_gradle_plugin_ver',
                                                       'sdk_build_tools_version',
                                                       'sdk_manager'])

def validate_android_config(android_config: command_utils.O3DEConfig) -> None:
    """
    Perform basic settings and environment validation to see if there are any missing dependencies or setup steps
    needed to process android commands.

    :param android_config:  The android configuration to look up any configured values needed for validation
    """

    # Validate Java is installed
    java_version = android_support.validate_java_environment()

    # Validate gradle
    gradle_home, gradle_version = android_support.validate_gradle(android_config)

    # Validate cmake
    cmake_path, cmake_version = android_support.validate_cmake(android_config)

    # Validate ninja
    ninja_path, ninja_version = android_support.validate_ninja(android_config)

    # Validate the versions of gradle and ninja against the configured version of the android gradle plugin
    android_gradle_plugin_ver = android_config.get_value(android_support.SETTINGS_GRADLE_PLUGIN_VERSION.key)
    if not android_gradle_plugin_ver:
        raise android_support.AndroidToolError(f"Missing '{android_support.SETTINGS_GRADLE_PLUGIN_VERSION.key}' from the android settings")

    android_gradle_requirements = android_support.get_android_gradle_plugin_requirements(android_gradle_plugin_ver)
    logger.info(f"Validating settings for requested version {android_gradle_requirements.version} of the Android Gradle Plugin.")
    android_gradle_requirements.validate_gradle_version(gradle_version)
    android_gradle_requirements.validate_java_version(java_version)

    sdk_build_tools_version = android_gradle_requirements.sdk_build_tools_version

    # Verify the Android SDK setting
    sdk_manager = android_support.AndroidSDKManager(java_version, android_config)

    # Make sure that the licenses have been read and accepted
    sdk_manager.check_licenses()

    # Make sure that all the signing config is set (warning)
    has_signing_config_store_file = len(android_config.get_value(android_support.SETTINGS_SIGNING_STORE_FILE.key, default='')) > 0
    has_signing_config_store_password = len(android_config.get_value(android_support.SETTINGS_SIGNING_STORE_PASSWORD.key, default='')) > 0
    has_signing_config_key_alias = len(android_config.get_value(android_support.SETTINGS_SIGNING_KEY_ALIAS.key, default='')) > 0
    has_signing_config_key_password = len(android_config.get_value(android_support.SETTINGS_SIGNING_KEY_PASSWORD.key, default='')) > 0

    signing_config_warned = False
    if not has_signing_config_store_file and not has_signing_config_store_password and not has_signing_config_key_alias and not has_signing_config_key_password:
        signing_config_warned = True
        logger.warning(VALIDATION_WARNING_SIGNCONFIG_NOT_SET)
        print(f"\nNone of the 'signconfig.*' was set. The android scripts will not support APK signing until a signing config is set. The "
              f"\nsigning configuration key store and key alias can be set with the '{SIGNCONFIG_ARG_STORE_FILE}' and '{SIGNCONFIG_ARG_KEY_ALIAS}' "
              f"\nrespectively with the '{O3DE_COMMAND_GENERATE}' command. The signing configuration values can also be stored in the settings.")
    elif has_signing_config_store_file and not has_signing_config_key_alias:
        signing_config_warned = True
        logger.warning(VALIDATION_WARNING_SIGNCONFIG_INCOMPLETE)
        print(f"{android_support.SETTINGS_SIGNING_STORE_FILE}' is set, but '{android_support.SETTINGS_SIGNING_KEY_ALIAS}' is not. "
              f"\nYou will need to provide a '{SIGNCONFIG_ARG_KEY_ALIAS}' argument when calling '{O3DE_COMMAND_GENERATE}'. For example:\n"
              f"\n{O3DE_SCRIPT_PATH} {O3DE_COMMAND_GENERATE} {SIGNCONFIG_ARG_KEY_ALIAS} SIGNCONFIG_KEY_ALIAS ...\n")
    elif not has_signing_config_store_file and has_signing_config_key_alias:
        signing_config_warned = True
        logger.warning(VALIDATION_WARNING_SIGNCONFIG_INCOMPLETE)
        print(f" The setting for '{android_support.SETTINGS_SIGNING_KEY_ALIAS}' is set, but '{android_support.SETTINGS_SIGNING_STORE_FILE}' "
              f"\nis not. You will need to provide a '{SIGNCONFIG_ARG_STORE_FILE}' argument when calling '{O3DE_COMMAND_GENERATE}'. For example:\n"
              f"\n{O3DE_SCRIPT_PATH} {O3DE_COMMAND_GENERATE} {SIGNCONFIG_ARG_STORE_FILE} SIGNCONFIG_STORE_FILE ...\n")

    if has_signing_config_store_file:
        signing_config_store_file = pathlib.Path(android_config.get_value(android_support.SETTINGS_SIGNING_STORE_FILE.key))
        if not signing_config_store_file.is_file():
            raise android_support.AndroidToolError(f"The signing config key store file '{signing_config_store_file}' is not a valid file.")

    if has_signing_config_store_file and has_signing_config_key_alias:
        # If the keystore and key alias is set, then runnnig the generate command will prompt for the password. Provide a warning anyways even though
        # they will still have an opportunity to enter the password
        if not has_signing_config_store_password or not has_signing_config_key_password:
            signing_config_warned = True
            logger.warning(VALIDATION_MISSING_PASSWORD)

            missing_passwords = []
            if not has_signing_config_store_password:
                missing_passwords.append(f"store password ({android_support.SETTINGS_SIGNING_STORE_PASSWORD})")
            if not has_signing_config_key_password:
                missing_passwords.append(f"key password ({android_support.SETTINGS_SIGNING_KEY_PASSWORD})")

            print(f"The signing configuration is set but missing the following settings:\n")
            for missing_password in missing_passwords:
                print(f" - {missing_password}")
            print(f"\nYou will be prompted for these passwords during the call to {O3DE_COMMAND_GENERATE}.")

    if signing_config_warned:
        print(f"\nType in '{O3DE_SCRIPT_PATH} {O3DE_COMMAND_CONFIGURE} --help' for more information about setting the signing config value in the settings.")

    validated_env = ValidatedEnv(java_version=java_version,
                                 gradle_home=gradle_home,
                                 gradle_version=gradle_version,
                                 cmake_path=cmake_path,
                                 cmake_version=cmake_version,
                                 ninja_path=ninja_path,
                                 ninja_version=ninja_version,
                                 android_gradle_plugin_ver=android_gradle_plugin_ver,
                                 sdk_build_tools_version=sdk_build_tools_version,
                                 sdk_manager=sdk_manager)
    return validated_env



def list_android_config(android_config: command_utils.O3DEConfig) -> None:
    """
    Print to stdout the current settings for android that will be applied to android operations

    :param android_config: The android configuration to look up the configured settings
    """

    all_settings = android_config.get_all_values()

    print("\nO3DE Android settings:\n")

    for item, value, source in all_settings:
        if not value:
            continue
        print(f"{'* ' if source else '  '}{item} = {value}")

    if not android_config.is_global:
        print(f"\n* Settings that are specific to {android_config.project_name}. Use the --global argument to see only the global settings.")


def get_android_config_from_args(args: argparse) -> (command_utils.O3DEConfig, str):
    """
    Resolve and create the appropriate O3DE config object based on whether operation is based on the global
    settings or a project specific setting

    :param args:    The args object to parse
    :return:  Tuple of the appropriate config manager object and the project name if this is not a global settings, otherwise None
    """

    is_global = getattr(args, 'global', False)
    project = getattr(args, 'project', None)

    if is_global:
        if project:
            logger.warning(f"Both --global and --project ({project}) arguments were provided. The --project argument will "
                            "be ignored and the execution of this command will be based on the global settings.")
        android_config = android_support.get_android_config(project_path=None)
        project_name = None
    elif not project:
        try:
            project_name, project_path = command_utils.resolve_project_name_and_path()
        except command_utils.O3DEConfigError:
            project_name = None
        if project_name:
            logger.info(f"The execution of this command will be based on the currently detected project ({project_name}).")
            android_config = android_support.get_android_config(project_path=project_path)
        else:
            logger.info("The execution of this command will be based on the global settings.")
            android_config = android_support.get_android_config(project_path=None)
    else:
        project_path = pathlib.Path(project)
        if project_path.is_dir():
            # If '--project' was set to a project path, then resolve the project path and name
            project_name, project_path = command_utils.resolve_project_name_and_path(project_path)

        else:
            project_name = project

            # If '--project' was not a project path, check to see if its a registered project by its name
            project_path = manifest.get_registered(project_name=project_name)
            if not project_path:
                raise command_utils.O3DEConfigError(f"Unable to resolve project named '{project_name}'. "
                                                    f"Make sure it is registered with O3DE.")

        logger.info(f"The execution of this command will be based on project ({project_name}).")
        android_config = android_support.get_android_config(project_path=project_path)

    return android_config, project_name


def configure_android_options(args: argparse) -> int:
    """
    Configure the android platform settings for generating, building, and deploying android projects.

    :param args:    The args from the arg parser to determine the actions
    :return: The result code to return back
    """
    if args.debug:
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.INFO)

    try:
        android_config, _ = get_android_config_from_args(args)

        if args.list:
            list_android_config(android_config)
        if args.validate:
            validate_android_config(android_config)
        if args.set_value:
            android_config.set_config_value_from_expression(args.set_value)
        if args.set_password:
            android_config.set_password(args.set_password)
        if args.clear_value:
            android_config.set_config_value(key=args.clear_value,
                                            value='',
                                            validate_value=False)
            logger.info(f"Setting '{args.clear_value}' cleared.")

    except (command_utils.O3DEConfigError, android_support.AndroidToolError) as err:
        logger.error(str(err))
        return 1
    else:
        return 0


def prompt_validated_password(name: str) -> str:
    """
    Request a password with validation prompt to the user. If the password validation fails (either empty or doesnt match), an exception is thrown

    :param name:    The password name to display
    :return: The validated password
    """
    enter_password = getpass(f"Please enter the password for {name} : ")
    if not enter_password:
        raise android_support.AndroidToolError(f"Password for {name} required.")
    confirm_password = getpass(f"Please verify the password for {name} : ")
    if confirm_password != enter_password:
        raise android_support.AndroidToolError(f"Passwords for {name} do not match.")

    return enter_password


def generate_android_project(args: argparse) -> int:
    """
    Execute the android gradle project creation workflow from the input arguments

    :param args:    The args from the arg parser to extract the necessary parameters for generating the project
    :return: The result code to return back
    """

    if args.debug:
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.INFO)

    try:
        # Create the android configuration
        android_config, project_name = get_android_config_from_args(args)

        # Resolve the project path and get the project and android settings
        resolved_project_path = manifest.get_registered(project_name=project_name)
        if not resolved_project_path:
            raise android_support.AndroidToolError(f"Project '{project_name}' is not registered with O3DE.")
        project_settings, android_settings = android_support.read_android_settings_for_project(resolved_project_path)

        # Perform validation on the config and the environment
        android_env = validate_android_config(android_config)

        # Make sure we have the extra android packages "market_apk_expansion" and "market_licensing" which is needed by the APK
        android_env.sdk_manager.install_package(package_install_path='extras;google;market_apk_expansion',
                                                package_description='Google APK Expansion Library')

        android_env.sdk_manager.install_package(package_install_path='extras;google;market_licensing',
                                                package_description='Google Play Licensing Library')

        # Make sure the requested Platform SDK (defined by API Level) is installed
        if args.platform_sdk_api_level:
            android_platform_sdk_api_level = args.platform_sdk_api_level
        else:
            android_platform_sdk_api_level = android_config.get_value(android_support.SETTINGS_PLATFORM_SDK_API.key)
            if not android_platform_sdk_api_level:
                raise android_support.AndroidToolError(f"The Android Platform SDK API Level was not specified in either the command argument (--platform-sdk-api-level) "
                                                       f"nor the android settings ({android_support.SETTINGS_PLATFORM_SDK_API.key}).")

        platform_package_name = f"platforms;android-{android_platform_sdk_api_level}"
        platform_sdk_package = android_env.sdk_manager.install_package(package_install_path=platform_package_name,
                                                                       package_description=f'Android SDK Platform {android_platform_sdk_api_level}')
        logger.info(f"Selected Android Platform API Level : {android_platform_sdk_api_level}")

        # Make sure the NDK is specified and installed
        if args.ndk_version:
            android_ndk_version = args.ndk_version
        else:
            android_ndk_version = android_config.get_value(android_support.SETTINGS_NDK_VERSION.key)
            if not android_ndk_version:
                raise android_support.AndroidToolError(f"The Android NDK version was not specified in either the command argument (--ndk-version) "
                                                       f"nor the android settings ({android_support.SETTINGS_NDK_VERSION.key}).")

        ndk_package_name = f"ndk;{android_ndk_version}"
        ndk_package = android_env.sdk_manager.install_package(package_install_path=ndk_package_name,
                                                              package_description=f'Android NDK version {android_ndk_version}')
        logger.info(f"Selected Android NDK Version : {ndk_package.version}")

        # Make sure that the android build tools (based on the spec from the android gradle plugin) is installed
        build_tools_package_name = f"build-tools;{android_env.sdk_build_tools_version}"
        android_env.sdk_manager.install_package(package_install_path=build_tools_package_name,
                                                package_description=f'Android Build Tools {android_env.sdk_build_tools_version}')

        engine_path = ENGINE_PATH
        logger.info(f'Engine Path : {engine_path}')

        project_path = resolved_project_path
        logger.info(f'Project Path : {project_path}')

        # Debug stripping option
        if getattr(args, 'strip_debug', False):
            strip_debug = True
        elif getattr(args, 'no_strip_debug', False):
            strip_debug = False
        else:
            strip_debug = android_config.get_boolean_value(android_support.SETTINGS_STRIP_DEBUG.key)

        logger.info(f"Debug symbol stripping {'enabled' if strip_debug else 'disabled'}.")

        # Optional additional cmake args for the native project generation
        extra_cmake_args = args.extra_cmake_args

        # Optional custom gradle JVM arguments
        custom_jvm_args = args.custom_jvm_args

        # Oculus project options
        if getattr(args, 'oculus_project', False):
            is_oculus_project = True
        elif getattr(args, 'no_oculus_project', False):
            is_oculus_project = False
        else:
            is_oculus_project = android_config.get_boolean_value(android_support.SETTINGS_OCULUS_PROJECT.key)
        if is_oculus_project:
            logger.info(f"Oculus flags enabled.")

        # Get the android build folder
        build_folder = pathlib.Path(args.build_dir)
        logger.info(f'Project Android Build Folder : {build_folder}')

        # Get the bundled assets sub folder
        src_bundle_pak_subfolder = android_config.get_value(android_support.SETTINGS_ASSET_BUNDLE_SUBPATH.key,
                                                            default='AssetBundling/Bundles')

        # Check if there is a signing config from the arguments
        signconfig_store_file = getattr(args, 'signconfig_store_file', None)
        signconfig_key_alias = getattr(args, 'signconfig_key_alias', None)
        if signconfig_store_file and signconfig_key_alias:

            signconfig_store_file_path = pathlib.Path(signconfig_store_file)
            if not signconfig_store_file_path.is_file():
                raise android_support.AndroidToolError(f"Invalid signing configuration store file '{signconfig_store_file}' specified. File does not exist.")

            stored_signconfig_store_password = android_config.get_value(android_support.SETTINGS_SIGNING_STORE_PASSWORD.key)
            if stored_signconfig_store_password:
                signconfig_store_password = stored_signconfig_store_password
            else:
                signconfig_store_password = prompt_validated_password(f"key store for {signconfig_store_file}")

            stored_signconfig_key_password = android_config.get_value(android_support.SETTINGS_SIGNING_KEY_PASSWORD.key)
            if stored_signconfig_key_password:
                signconfig_key_password = stored_signconfig_key_password
            else:
                signconfig_key_password = prompt_validated_password(f"key alias for {signconfig_key_alias}")

            signing_config = android_support.AndroidSigningConfig(store_file=signconfig_store_file_path,
                                                                  store_password=signconfig_store_password,
                                                                  key_alias=signconfig_key_alias,
                                                                  key_password=signconfig_key_password)
        elif signconfig_store_file and not signconfig_key_alias:
            raise android_support.AndroidToolError(f"Missing required '{SIGNCONFIG_ARG_KEY_ALIAS}' argument to accompany the provided '{SIGNCONFIG_ARG_STORE_FILE}' ({signconfig_store_file})")
        elif not signconfig_store_file and signconfig_key_alias:
            raise android_support.AndroidToolError(f"Missing required '{SIGNCONFIG_ARG_STORE_FILE}' argument to accompany the provided '{SIGNCONFIG_ARG_KEY_ALIAS}' ({signconfig_store_file})")
        else:
            signing_config = None

        if signing_config is not None:
            logger.info("Signing configuration enabled")
        else:
            logger.warning("No signing configuration enabled. This output APK will not be signed until a signing configuration is added.")

        apg = android_support.AndroidProjectGenerator(engine_root=engine_path,
                                                      android_build_dir=build_folder,
                                                      android_sdk_path=android_env.sdk_manager.get_android_sdk_path(),
                                                      android_build_tool_version=android_env.sdk_build_tools_version,
                                                      android_platform_sdk_api_level=android_platform_sdk_api_level,
                                                      android_ndk_package=ndk_package,
                                                      project_name=project_name,
                                                      project_path=project_path,
                                                      project_general_settings=project_settings,
                                                      project_android_settings=android_settings,
                                                      cmake_version=android_env.cmake_version,
                                                      cmake_path=android_env.cmake_path,
                                                      gradle_path=android_env.gradle_home,
                                                      gradle_version=android_env.gradle_version,
                                                      gradle_custom_jvm_args=custom_jvm_args,
                                                      android_gradle_plugin_version=android_env.android_gradle_plugin_ver,
                                                      ninja_path=android_env.ninja_path,
                                                      asset_mode=args.asset_mode,
                                                      signing_config=signing_config,
                                                      extra_cmake_configure_args=extra_cmake_args,
                                                      overwrite_existing=True,
                                                      strip_debug_symbols=strip_debug,
                                                      src_pak_file_path=src_bundle_pak_subfolder,
                                                      oculus_project=is_oculus_project)
        apg.execute()

    except (command_utils.O3DEConfigError, android_support.AndroidToolError) as err:
        logger.error(str(err))
        return 1
    else:
        return 0


def add_args(subparsers) -> None:
    """
    add_args is called to add subparsers for the following commands to the central o3de.py command processor:

    - android_configure
    - android_generate

    :param subparsers: the caller instantiates subparsers and passes it in here
    """

    # Read from the android config if possible to try to display default values
    try:
        project_name, project_path = command_utils.resolve_project_name_and_path()
        android_config = android_support.get_android_config(project_path=project_path)
    except (android_support.AndroidToolError, command_utils.O3DEConfigError):
        logger.debug(f"No project detected at {os.getcwd()}, default settings from global config.")
        project_name = None
        android_config = android_support.get_android_config(project_path=None)

    #
    # Configure the subparser for 'android-configure'
    #

    # Generate the epilog string to describe the settings that can be configured
    epilog_lines = ['Configure the default values that control the android helper scripts for O3DE.',
                    'The list settings that can be set are:',
                    '']
    max_key_len = 0
    for setting in android_config.setting_descriptions:
        max_key_len = max(len(setting.key), max_key_len)
    max_key_len += 4
    for setting in android_config.setting_descriptions:
        setting_line = f"{'* ' if setting.is_password else '  '}{setting.key: <{max_key_len}} {setting.description}"
        epilog_lines.append(setting_line)
    epilog_lines.extend([
        '',
        '* Denotes password settings that can only be set using the --set-password command'
    ])

    epilog = '\n'.join(epilog_lines)

    android_configure_subparser = subparsers.add_parser(O3DE_COMMAND_CONFIGURE,
                                                        help='Configure the Android platform settings for generating, building, and deploying Android projects.',
                                                        epilog=epilog,
                                                        formatter_class=argparse.RawTextHelpFormatter)

    android_configure_subparser.add_argument('--global', default=False, action='store_true',
                                             help='Used with the configure command, specify whether the settings are project local or global.')
    android_configure_subparser.add_argument('-p', '--project', type=str, required=False,
                                             help="The name of the registered project to configure the settings for. This value is ignored if '--global' was specified."
                                                  "Note: If both '--global' and '-p/--project' is not specified, the script will attempt to deduce the project from the "
                                                  "current working directory if possible")
    android_configure_subparser.add_argument('-l', '--list', default=False, action='store_true',
                                             help='Display the current Android settings. ')
    android_configure_subparser.add_argument('--validate', default=False, action='store_true',
                                             help='Validate the settings and values in the Android settings. ')
    android_configure_subparser.add_argument('--set-value', type=str, required=False,
                                             help='Set the value for an Android setting, using the format <argument>=<value>. For example: \'ndk_version=22.5*\'',
                                             metavar='VALUE')
    android_configure_subparser.add_argument('--clear-value', type=str, required=False,
                                             help='Clear a previously configured setting.',
                                             metavar='VALUE')
    android_configure_subparser.add_argument('--set-password', type=str, required=False,
                                             help='Set the password for a password setting. A password prompt will be presented.',
                                             metavar='SETTING')
    android_configure_subparser.add_argument('--debug',
                                             help=f"Enable debug level logging for this script.",
                                             action='store_true')
    android_configure_subparser.set_defaults(func=configure_android_options)

    #
    # Configure the subparser for 'android_generate'
    #
    android_generate_subparser = subparsers.add_parser(O3DE_COMMAND_GENERATE,
                                                       help='Generate an Android/Gradle project.')
    android_generate_subparser.add_argument('--settings-asset-bundle-path', type=str, default=android_support.SETTINGS_ASSET_BUNDLE_SUBPATH.key,
                                            help="The sub-path from the project root to specify where the bundle/pak files will be generated to.")

    # Project Name
    android_generate_subparser.add_argument('-p', '--project', type=str,
                                             help="The name of the registered project or the full path to the O3DE project to generate the Android build scripts for. If not supplied, this operation will attempt to "
                                                  "resolve the project from the current directory.")

    # Build Directory
    android_generate_subparser.add_argument('-B', '--build-dir', type=str,
                                             help=f"The location to write the android project scripts to. Default: '{DEFAULT_ANDROID_BUILD_FOLDER}'",
                                             default=DEFAULT_ANDROID_BUILD_FOLDER)

    # Platform SDK API Level (https://developer.android.com/tools/releases/platforms)
    platform_sdk_api_level = android_config.get_value(android_support.SETTINGS_PLATFORM_SDK_API.key)
    if platform_sdk_api_level:
        android_generate_subparser.add_argument('--platform-sdk-api-level', type=str,
                                                help=f"Specify the platform SDK API Level. Default: {platform_sdk_api_level}",
                                                default=platform_sdk_api_level)
    else:
        android_generate_subparser.add_argument('--platform-sdk-api-level', type=str,
                                                help=f"Specify the platform SDK API Level ({android_support.SETTINGS_PLATFORM_SDK_API.key})")

    # Android NDK Version version (https://developer.android.com/ndk/downloads)
    ndk_version = android_config.get_value(android_support.SETTINGS_NDK_VERSION.key)
    if ndk_version:
        android_generate_subparser.add_argument('--ndk-version', type=str,
                                                help=f"Specify the android NDK version. Default: {ndk_version}.",
                                                default=ndk_version)
    else:
        android_generate_subparser.add_argument('--ndk-version', type=str,
                                                help=f"Specify the android NDK version ({android_support.SETTINGS_NDK_VERSION.key}).")

    # Signing configuration key store file
    signconfig_store_file = android_config.get_value(android_support.SETTINGS_SIGNING_STORE_FILE.key)
    if signconfig_store_file:
        android_generate_subparser.add_argument(SIGNCONFIG_ARG_STORE_FILE, type=str,
                                                help=f"Specify the location of the jks keystore file to enable a signing configuration for this project automatically. "
                                                     f"If set, then a store file password, key alias, and key password will be required as well. "
                                                     f" Default : {signconfig_store_file}",
                                                default=signconfig_store_file)
    else:
        android_generate_subparser.add_argument(SIGNCONFIG_ARG_STORE_FILE, type=str,
                                                help=f"Specify the location of the jks keystore file to enable a signing configuration for this project automatically "
                                                     f"({android_support.SETTINGS_SIGNING_STORE_FILE.key}). "
                                                     f"If set, then a store file password, key alias, and key password will be required as well.")

    # Signing configuration key store alias
    signconfig_key_alias = android_config.get_value(android_support.SETTINGS_SIGNING_KEY_ALIAS.key)
    if signconfig_key_alias:
        android_generate_subparser.add_argument(SIGNCONFIG_ARG_KEY_ALIAS, type=str,
                                                help=f"Specify the key alias for the configured jks keystore file if set.\n"
                                                     f"Default: {signconfig_key_alias}.",
                                                default=signconfig_key_alias)
    else:
        android_generate_subparser.add_argument(SIGNCONFIG_ARG_KEY_ALIAS, type=str,
                                                help=f"Specify the key alias for the configured jks keystore file if set "
                                                     f"({android_support.SETTINGS_SIGNING_KEY_ALIAS.key}.")
    # Asset Mode
    asset_mode = android_config.get_value(android_support.SETTINGS_ASSET_MODE.key, default=android_support.ASSET_MODE_LOOSE)
    android_generate_subparser.add_argument('--asset-mode', type=str,
                                            help=f"The mode of asset deployment to use. "
                                                 f" Default: {asset_mode}",
                                            choices=android_support.ASSET_MODES,
                                            default=asset_mode)

    # Extra CMake configure args
    extra_cmake_args = android_config.get_value(android_support.SETTINGS_EXTRA_CMAKE_ARGS.key, default='')
    android_generate_subparser.add_argument('--extra-cmake-args', type=str,
                                            help=android_support.SETTINGS_EXTRA_CMAKE_ARGS.description,
                                            default=extra_cmake_args)

    # Custom JVM args    ANDROID_SETTINGS_GRADLE_JVM_ARGS
    custom_jvm_args = android_config.get_value(android_support.SETTINGS_GRADLE_JVM_ARGS.key, default='')
    android_generate_subparser.add_argument('--custom-jvm-args', type=str,
                                            help=android_support.SETTINGS_GRADLE_JVM_ARGS.description,
                                            default=custom_jvm_args)

    # Flag to strip the debug symbols
    strip_debug = android_config.get_value(android_support.SETTINGS_STRIP_DEBUG.key)
    if strip_debug:
        android_generate_subparser.add_argument('--no-strip-debug',
                                                help=f"Don't strip the debug symbols from the built binaries.",
                                                action='store_true')
    else:
        android_generate_subparser.add_argument('--strip-debug',
                                                help=f"Strip the debug symbols from the built binaries.",
                                                action='store_true')

    # Flag indicating this is an oculus project
    is_oculus_project = android_config.get_value(android_support.SETTINGS_OCULUS_PROJECT.key)
    if is_oculus_project:
        android_generate_subparser.add_argument('--no-oculus-project',
                                                help=f"Turn off the flag that marks the project as an Oculus project.",
                                                action='store_true')
    else:
        android_generate_subparser.add_argument('--oculus-project',
                                                help="Marks the project as an Oculus project. This flag enables Oculus-specific settings "
                                                     "that are required for the Android project generation.",
                                                action='store_true')

    android_generate_subparser.add_argument('--debug',
                                            help=f"Enable debug level logging for this script.",
                                            action='store_true')

    android_generate_subparser.set_defaults(func=generate_android_project)
