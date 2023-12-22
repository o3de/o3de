#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import o3de.command_utils as command_utils
import o3de.export_project as exp
import o3de.manifest as manifest

from o3de import android, android_support


import argparse
import logging
import pathlib
import platform
import sys
import time

from typing import List

def supply_android_default_global_configuration(api:int = 31,
                                        ndk_version:str = '25*',
                                        gradle_min_version:str = '8.1.0',
                                        auto_store_pwd:str|None = None,
                                        auto_key_pwd:str|None = None):
    is_pc = exp.get_default_asset_platform() == 'pc'
    android_sdk_home_env_var = '%ANDROID_SDK_HOME%' if is_pc else '$ANDROID_SDK_HOME'
    android_signing_config_keystore_file_env_var = '%ANDROID_SIGNING_CONFIG_KEYSTORE_FILE%' if is_pc else '$ANDROID_SIGNING_CONFIG_KEYSTORE_FILE' 
    android_signing_config_key_alias_env_var = '%ANDROID_SIGNING_CONFIG_KEY_ALIAS%' if is_pc else '$ANDROID_SIGNING_CONFIG_KEY_ALIAS'

    android_global_config, _ = android.get_android_config(is_global=True)
    android_global_config.set_config_value(key=android_support.SETTINGS_SDK_ROOT.key, value=android_sdk_home_env_var, validate_value=False)
    android_global_config.set_config_value(key=android_support.SETTINGS_PLATFORM_SDK_API.key, value=api, validate_value=False)
    android_global_config.set_config_value(key=android_support.SETTINGS_NDK_VERSION.key, value=ndk_version, validate_value=False)
    android_global_config.set_config_value(key=android_support.SETTINGS_GRADLE_PLUGIN_VERSION.key, value=gradle_min_version, validate_value=False)
    android_global_config.set_config_value(key=android_support.SETTINGS_SIGNING_STORE_FILE.key, value=android_signing_config_keystore_file_env_var, validate_value=False)
    android_global_config.set_config_value(key=android_support.SETTINGS_SIGNING_KEY_ALIAS.key, value=android_signing_config_key_alias_env_var, validate_value=False)

    if not auto_store_pwd:
        android_global_config.set_password(android_support.SETTINGS_SIGNING_STORE_PASSWORD.key)
    else:
        android_global_config.set_config_value(key=android_support.SETTINGS_SIGNING_STORE_PASSWORD.key, value=auto_store_pwd, validate_value=False)
    
    if not auto_key_pwd:
        android_global_config.set_password(android_support.SETTINGS_SIGNING_KEY_PASSWORD.key)
    else:
        android_global_config.set_config_value(key=android_support.SETTINGS_SIGNING_KEY_PASSWORD.key, value=auto_key_pwd, validate_value=False)

def deloy_to_android_device(target_android_project_path: pathlib.Path,
                            build_config:str,
                            project_name:str,
                            android_sdk_home:pathlib.Path,
                            org_name:str|None = None,
                            activity_name:str|None = None):
    is_pc = exp.get_default_asset_platform() == 'pc'

    adb_exec_path = android_sdk_home / ('platform-tools/adb' + ('.exe' if is_pc else ''))
    exp.process_command([adb_exec_path, 'install', '-t', '-r', target_android_project_path / f'app/build/outputs/apk/{build_config}/app-{build_config}.apk'])

    time.sleep(1)

    if not activity_name:
        activity_name = f'{project_name}Activity'
    
    if not org_name:
        org_name = f'org.o3de.{project_name}'

    exp.process_command([adb_exec_path, 'shell', 'am', 'start', '-a', 'android.intent.action.MAIN','-n', f'{org_name}/.{activity_name}'])


def export_source_android_project(ctx: exp.O3DEScriptExportContext,
                           target_android_project_path: pathlib.Path,
                           seedlist_paths: List[pathlib.Path],
                           seedfile_paths: List[pathlib.Path],
                           level_names: List[str],
                           asset_pack_mode: str = 'PAK',
                           engine_centric: bool = False,
                           should_build_tools: bool = True,
                           should_build_all_assets: bool = True,
                           build_config: str = 'profile',
                           tool_config: str = 'profile',
                           tools_build_path: pathlib.Path | None =None,
                           max_bundle_size: int = 2048,
                           fail_on_asset_errors: bool = False,
                           deploy_to_device: bool = False,
                           org_name: str|None = None,
                           activity_name:str|None = None,
                           logger: logging.Logger|None = None) -> None:
    
    expected_platform = 'android'
    project_path = ctx.project_path
    engine_path = ctx.engine_path
    project_name = ctx.project_name

    if not logger:
        logger = logging.getLogger()
        logger.setLevel(logging.ERROR)

    is_installer_sdk = manifest.is_sdk_engine(engine_path=engine_path)

    is_pc = exp.get_default_asset_platform() == 'pc'

    o3de_cli_script_path = engine_path / ('scripts/o3de' + ('.bat' if is_pc else '.sh'))

    assert exp.process_command([o3de_cli_script_path, 'android-configure', '--validate'], cwd=project_path) == 0, "Android config validation went wrong!"

    # Calculate the tools and game build paths
    default_base_path = engine_path if engine_centric else project_path

    # Resolve (if possible) and validate any provided seedlist files
    validated_seedslist_paths = exp.validate_project_artifact_paths(project_path=ctx.project_path,
                                                                    artifact_paths=seedlist_paths)
    
    # Preprocess the seed file paths in case there are any wildcard patterns to use
    preprocessed_seedfile_paths = exp.preprocess_seed_path_list(project_path=ctx.project_path,
                                                                paths=seedfile_paths)
    
    # Convert level names into seed files that the asset bundler can utilize for packaging
    for level in level_names:
        preprocessed_seedfile_paths.append(ctx.project_path / f'Cache/{expected_platform}/levels' / level.lower() / (level.lower() + ".spawnable"))
    
    if is_installer_sdk:
        # Check if we have any monolithic entries if building monolithic
        if not exp.has_monolithic_artifacts(ctx):
            logger.error("Trying to create monolithic build, but no monolithic artifacts are detected in the engine installation!.")
            raise exp.ExportProjectError("Trying to build monolithic without libraries.")
        tools_build_path = engine_path / 'bin' / exp.get_platform_installer_folder_name() / tool_config / 'Default'
    
    if not tools_build_path:
        tools_build_path = default_base_path / 'build/tools'
    elif not tools_build_path.is_absolute():
        tools_build_path = default_base_path / tools_build_path
    
    logger.info(f"Tools build path set to {tools_build_path}")
    
    if should_build_tools and not is_installer_sdk:
        exp.build_export_toolchain(ctx=ctx,
                                   tools_build_path=tools_build_path,
                                   engine_centric=engine_centric,
                                   tool_config=tool_config,
                                   logger=logger)

    if should_build_all_assets:
        asset_processor_path = exp.get_asset_processor_batch_path(tools_build_path=tools_build_path,
                                                                  using_installer_sdk=is_installer_sdk,
                                                                  tool_config=tool_config,
                                                                  required=True)
        logger.info(f"Using '{asset_processor_path}' to process the assets.")
        exp.build_assets(ctx=ctx,
                         tools_build_path=tools_build_path,
                         engine_centric=engine_centric,
                         fail_on_ap_errors=fail_on_asset_errors,
                         using_installer_sdk=is_installer_sdk,
                         tool_config=tool_config,
                         selected_platform=expected_platform,
                         logger=logger)

    asset_bundles_path = exp.bundle_assets(ctx=ctx,
                      selected_platform=expected_platform,
                      seedlist_paths=validated_seedslist_paths,
                      seedfile_paths=preprocessed_seedfile_paths,
                      tools_build_path=tools_build_path,
                      engine_centric=engine_centric,
                      asset_bundling_path=project_path/'AssetBundling',
                      using_installer_sdk=is_installer_sdk,
                      tool_config=tool_config,
                      max_bundle_size=max_bundle_size)

    android_project_config= android_support.get_android_config(project_path=None)

    android_project_config.set_config_value(key=android_support.SETTINGS_ASSET_BUNDLE_SUBPATH.key, value=str(asset_bundles_path).replace('\\', '/'), validate_value=False)
    
    android_sdk_home = pathlib.Path(android_project_config.get_value(key=android_support.SETTINGS_SDK_ROOT.key))
    
    if not android_sdk_home.is_dir():
        logger.error("ANDROID_SDK_HOME is not properly configured! Please ensure SDK_ROOT is properly configured in o3de android-configure")
        raise exp.ExportProjectError("Invalid ANDROID_SDK_HOME.")

    exp.process_command([o3de_cli_script_path, 'android-generate', '-p', project_name, '-B', target_android_project_path, "--asset-mode", asset_pack_mode], 
                cwd=default_base_path)

    exp.process_command([target_android_project_path / ('gradlew'+ (".bat" if is_pc else "")), f'assemble{build_config.title()}'], cwd=target_android_project_path)

    if deploy_to_device:
        deloy_to_android_device(target_android_project_path,
                                build_config, 
                                project_name, 
                                android_sdk_home,
                                org_name,
                                activity_name)

def export_source_android_parse_args(o3de_context: exp.O3DEScriptExportContext, export_config: command_utils.O3DEConfig):
    parser = argparse.ArgumentParser(
                    prog=f'o3de.py export-project -es {__file__}',
                    description="Exports a project as standalone to the desired output directory with release layout. "
                                "In order to use this script, the engine and project must be setup and registered beforehand. ",
                    epilog=exp.CUSTOM_CMAKE_ARG_HELP_EPILOGUE,
                    formatter_class=argparse.RawTextHelpFormatter,
                    add_help=False
    )
    parser.add_argument(exp.CUSTOM_SCRIPT_HELP_ARGUMENT,default=False,action='store_true',help='Show this help message and exit.')
    
    parser.add_argument('-deploy', '--deploy-to-android',default=False,action='store_true',help='At completion of build, deploy to android device.')

    parser.add_argument('-actname', '--activity_name', type=str, default='',
                        help='The name of the activity to invoke when deploying and running the APK.')
    
    parser.add_argument('-orgname', '--org_name', type=str, default='',
                        help='The name of the organization identifier (i.e. org.o3de.PROJECT) to use when deploying and running the APK.')

    parser.add_argument('-defaults', '--provide-default-android-config',default=False,action='store_true',help='Sets up a new android configuration if requested.')
    
    parser.add_argument('-out', '--android-output-path', type=pathlib.Path, required=True, help='Path that describes the final resulting Android Project and APK location.')

    default_project_build_config = export_config.get_value(key=exp.SETTINGS_PROJECT_BUILD_CONFIG.key,
                                                            default=exp.SETTINGS_PROJECT_BUILD_CONFIG.default)
    parser.add_argument('-cfg', '--config', type=str, default=default_project_build_config, choices=[exp.BUILD_CONFIG_RELEASE, exp.BUILD_CONFIG_PROFILE],
                        help='The CMake build configuration to use when building project binaries.')
    
    parser.add_argument('-apkm', '--asset_pack_mode', type=str, default='PAK', choices=['PAK', 'LOOSE'],
                        help='Choose how O3DE will package project assets into the final APK file.')
    
    

    export_config.add_multi_part_argument(argument=['-sl', '--seedlist'],
                                              parser=parser,
                                              key=exp.SETTINGS_SEED_LIST_PATHS.key,
                                              dest='seedlist_paths',
                                              description='Path to a seed list file for asset bundling. Specify multiple times for each seed list. You can also specify wildcard patterns.',
                                              is_path_type=True)

    export_config.add_multi_part_argument(argument=['-sf', '--seedfile'],
                                            parser=parser,
                                            key=exp.SETTINGS_SEED_FILE_PATHS.key,
                                            dest='seedfile_paths',
                                            description='Path to a seed file for asset bundling. Example seed files are levels or prefabs. You can also specify wildcard patterns.',
                                            is_path_type=True)

    export_config.add_multi_part_argument(argument=['-lvl', '--level-name'],
                                            parser=parser,
                                            key=exp.SETTINGS_DEFAULT_LEVEL_NAMES.key,
                                            dest='level_names',
                                            description='The name of the level you want to export. This will look in <o3de_project_path>/Levels to fetch the right level prefab.',
                                            is_path_type=False)

    export_config.add_boolean_argument(parser=parser,
                                           key=exp.SETTINGS_OPTION_ENGINE_CENTRIC.key,
                                           enable_override_arg=['-ec', '--engine-centric'],
                                           enable_override_desc="Enable the engine-centric work flow to export the project.",
                                           disable_override_arg=['-pc', '--project-centric'],
                                           disable_override_desc="Enable the project-centric work flow to export the project.")
    
    export_config.add_boolean_argument(parser=parser,
                                           key=exp.SETTINGS_OPTION_BUILD_ASSETS.key,
                                           enable_override_arg=['-assets', '--build-assets'],
                                           enable_override_desc='Build and update all assets before bundling.',
                                           disable_override_arg=['-noassets', '--skip-build-assets'],
                                           disable_override_desc='Skip building of assets and use assets that were already built.')

    export_config.add_boolean_argument(parser=parser,
                                           key=exp.SETTINGS_OPTION_BUILD_TOOLS.key,
                                           enable_override_arg=['-bt', '--build-tools'],
                                           enable_override_desc="Enable the building of the O3DE toolchain executables (AssetBundlerBatch, AssetProcessorBatch) as part of the export process. "
                                                                "The tools will be built into the location specified by the '--tools-build-path' argument",
                                           disable_override_arg=['-sbt', '--skip-build-tools'],
                                           disable_override_desc="Skip the building of the O3DE toolchain executables (AssetBundlerBatch, AssetProcessorBatch) as part of the export process. "
                                                                 "The tools must be already available at the location specified by the '--tools-build-path' argument.")


    default_tool_build_config = export_config.get_value(key=exp.SETTINGS_TOOL_BUILD_CONFIG.key,
                                                            default=exp.SETTINGS_TOOL_BUILD_CONFIG.default)
    parser.add_argument('-tcfg', '--tool-config', type=str, default=default_tool_build_config, choices=[exp.BUILD_CONFIG_RELEASE, exp.BUILD_CONFIG_PROFILE, exp.BUILD_CONFIG_DEBUG],
                        help='The CMake build configuration to use when building tool binaries.')

    default_tools_build_path = export_config.get_value(exp.SETTINGS_DEFAULT_BUILD_TOOLS_PATH.key, exp.SETTINGS_DEFAULT_BUILD_TOOLS_PATH.default)
    parser.add_argument('-tbp', '--tools-build-path', type=pathlib.Path, default=pathlib.Path(default_tools_build_path),
                        help=f"Designates where the build files for the O3DE toolchain are generated. If not specified, default is '{default_tools_build_path}'.")

    default_max_size = export_config.get_value(exp.SETTINGS_MAX_BUNDLE_SIZE.key, exp.SETTINGS_MAX_BUNDLE_SIZE.default)
    parser.add_argument('-maxsize', '--max-bundle-size', type=int, default=int(default_max_size),
                        help=f"Specify the maximum size of a given asset bundle.. If not specified, default is {default_max_size}.")


    export_config.add_boolean_argument(parser=parser,
                                           key=exp.SETTINGS_OPTION_FAIL_ON_ASSET_ERR.key,
                                           enable_override_arg=['-foa', '--fail-on-asset-errors'],
                                           enable_override_desc='Fail the export if there are errors during the building of assets. (Only relevant if assets are set to be built).',
                                           disable_override_arg=['-coa', '--continue-on-asset-errors'],
                                           disable_override_desc='Continue export even if there are errors during the building of assets. (Only relevant if assets are set to be built).')


    parser.add_argument('-q', '--quiet', action='store_true', help='Suppresses logging information unless an error occurs.')
    if o3de_context is None:
        parser.print_help()
        exit(0)
    
    parsed_args = parser.parse_args(o3de_context.args)
    if parsed_args.script_help:
        parser.print_help()
        exit(0)

    return parsed_args

def export_source_android_run_command(o3de_context: exp.O3DEScriptExportContext, args, export_config: command_utils.O3DEConfig, o3de_logger):
    option_build_assets = export_config.get_parsed_boolean_option(parsed_args=args,
                                                                  key=exp.SETTINGS_OPTION_BUILD_ASSETS.key,
                                                                  enable_attribute='build_assets',
                                                                  disable_attribute='skip_build_assets')

    option_build_tools = export_config.get_parsed_boolean_option(parsed_args=args,
                                                                 key=exp.SETTINGS_OPTION_BUILD_TOOLS.key,
                                                                 enable_attribute='build_tools',
                                                                 disable_attribute='skip_build_tools')    

    fail_on_asset_errors = export_config.get_parsed_boolean_option(parsed_args=args,
                                                                   key=exp.SETTINGS_OPTION_FAIL_ON_ASSET_ERR.key,
                                                                   enable_attribute='fail_on_asset_errors',
                                                                   disable_attribute='continue_on_asset_errors')

    option_build_engine_centric = export_config.get_parsed_boolean_option(parsed_args=args,
                                                                          key=exp.SETTINGS_OPTION_ENGINE_CENTRIC.key,
                                                                          enable_attribute='engine_centric',
                                                                          disable_attribute='project_centric')
    
    if args.quiet:
        o3de_logger.setLevel(logging.ERROR)
    else:
        o3de_logger.setLevel(logging.INFO)

    

    try:
        if args.provide_default_android_config:
            supply_android_default_global_configuration()

        export_source_android_project(ctx=o3de_context,
                                  target_android_project_path=args.android_output_path,
                                  seedlist_paths=args.seedlist_paths,
                                  seedfile_paths=args.seedfile_paths,
                                  level_names=args.level_names,
                                  asset_pack_mode=args.asset_pack_mode,
                                  engine_centric=option_build_engine_centric,
                                  should_build_all_assets=option_build_assets,
                                  should_build_tools=option_build_tools,
                                  build_config=args.config,
                                  tool_config=args.tool_config,
                                  tools_build_path=args.tools_build_path,
                                  max_bundle_size=args.max_bundle_size,
                                  fail_on_asset_errors=fail_on_asset_errors,
                                  deploy_to_device=args.deploy_to_android,
                                  activity_name=args.activity_name,
                                  org_name=args.org_name,
                                  logger=o3de_logger)
    except exp.ExportProjectError as err:
        print(err)
        sys.exit(1)

# This code is only run by the 'export-project' O3DE CLI command
if "o3de_context" in globals():
    global o3de_context
    global o3de_logger

    # Resolve the export config
    try:
        if o3de_context is None:
            project_name, project_path = command_utils.resolve_project_name_and_path()
            export_config = exp.get_export_project_config(project_path=project_name)
        else:
            export_config = exp.get_export_project_config(project_path=o3de_context.project_path)
    except command_utils.O3DEConfigError:
        o3de_logger.debug(f"No project detected at {os.getcwd()}, using default settings from global config.")
        project_name = None
        export_config = exp.get_export_project_config(project_path=None)
    
    args = export_source_android_parse_args(o3de_context, export_config)

    export_source_android_run_command(o3de_context, args, export_config, o3de_logger)
    o3de_logger.info(f"Finished exporting android project to {args.android_output_path}")
    sys.exit(0)
