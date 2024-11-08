#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import o3de.command_utils as command_utils
import o3de.export_project as exp
import export_utility as eutil
import o3de.manifest as manifest

from o3de import android, android_support

import argparse
import logging
import os
import pathlib
import re
import sys


from typing import List

ASSET_PLATFORM = 'android'

def export_source_android_project(ctx: exp.O3DEScriptExportContext,
                                  target_android_project_path: pathlib.Path,
                                  seedlist_paths: List[pathlib.Path],
                                  seedfile_paths: List[pathlib.Path],
                                  level_names: List[str],
                                  asset_pack_mode: str,
                                  engine_centric: bool,
                                  should_build_tools: bool,
                                  should_build_all_assets: bool,
                                  build_config: str,
                                  tool_config: str,
                                  tools_build_path: pathlib.Path | None,
                                  max_bundle_size: int,
                                  fail_on_asset_errors: bool,
                                  deploy_to_device: bool,
                                  logger: logging.Logger|None) -> None:
    if not logger:
        logger = logging.getLogger()
        logger.setLevel(logging.ERROR)

    is_installer_sdk = manifest.is_sdk_engine(engine_path=ctx.engine_path)

    # For installed SDKs, only the release monolithic android artifacts are installed with the SDK
    if is_installer_sdk:
        # Make sure monolithic artifacts are present
        if not exp.has_monolithic_artifacts(ctx):
            logger.error("No monolithic artifacts are detected in the engine installation.")
            raise exp.ExportProjectError("Trying to build monolithic without libraries.")

        if build_config != exp.BUILD_CONFIG_RELEASE:
            logger.warning("Only release packages are supported for Android in the O3DE SDK.")
        build_config = exp.BUILD_CONFIG_RELEASE

    android_arg_parser = argparse.ArgumentParser()
    android_subparser = android_arg_parser.add_subparsers(title="Android sub-commands")
    android.add_args(android_subparser)

    # We invoke the o3de.android APIs to configure, validate, and generate the android project
    # Parsed CLI arguments are passed along to the API in question.
    parsed_android_configure_command = android_arg_parser.parse_args(['android-configure', '--validate'])
    android.configure_android_options(parsed_android_configure_command)
    

    # Calculate the tools and game build paths
    default_base_path = ctx.engine_path if engine_centric else ctx.project_path

    # Resolve (if possible) and validate any provided seedlist files
    validated_seedslist_paths = exp.validate_project_artifact_paths(project_path=ctx.project_path,
                                                                    artifact_paths=seedlist_paths)
    
    # Preprocess the seed file paths in case there are any wildcard patterns to use
    preprocessed_seedfile_paths = exp.preprocess_seed_path_list(project_path=ctx.project_path,
                                                                paths=seedfile_paths)
    
    eutil.process_level_names(ctx, preprocessed_seedfile_paths, level_names, ASSET_PLATFORM)

    tools_build_path = eutil.handle_tools(ctx,
                                          should_build_tools,
                                          is_installer_sdk,
                                          tool_config,
                                          '',
                                          tools_build_path,
                                          default_base_path,
                                          engine_centric,
                                          logger)

    eutil.build_and_bundle_assets(ctx, should_build_all_assets, tools_build_path, is_installer_sdk, 
                                  tool_config, engine_centric, fail_on_asset_errors,
                                  [ASSET_PLATFORM], validated_seedslist_paths, preprocessed_seedfile_paths, 
                                  ctx.project_path/'AssetBundling', max_bundle_size,
                                  logger)

    android_project_config= android_support.get_android_config(project_path=None)

    android_sdk_home = pathlib.Path(android_project_config.get_value(key=android_support.SETTINGS_SDK_ROOT.key))

    parsed_android_generate_command = android_arg_parser.parse_args(['android-generate', '-p', str(ctx.project_name), '-B', str(target_android_project_path), '--asset-mode', asset_pack_mode])
    android.generate_android_project(parsed_android_generate_command)

    gradle_build_command = [target_android_project_path / f'gradlew{android_support.GRADLE_EXTENSION}', f'assemble{build_config.title()}']
    has_errors = exp.process_command(gradle_build_command, cwd=target_android_project_path)
    if has_errors:
        raise exp.ExportProjectError("Gradle Build Failure.")

    if deploy_to_device:
        
        activity_name = f'{ctx.project_name}Activity'
        logger.info(f"Activity name set to {activity_name}")

        
        org_name = f'org.o3de.{ctx.project_name}'
        logger.info(f"Organization name set to  {org_name}")

        signing_config_file_path = target_android_project_path /'app/build.gradle'

        try:
            build_gradle_content = signing_config_file_path.read_text('utf-8','ignore')
            match_signing_config = re.compile("(signingConfigs\\s).*(storeFile\\s).*(storePassword\\s).*(keyPassword\\s).*(keyAlias\\s).*", re.DOTALL)
            signing_config_verified = match_signing_config.search(build_gradle_content) is not None
            if signing_config_verified:
                android_support.deloy_to_android_device(target_android_project_path,
                                                        build_config,
                                                        android_sdk_home,
                                                        org_name,
                                                        activity_name)
            else:
                logger.error("Unable to verify signing configuration in build.gradle. Aborting deployment...")
        except FileNotFoundError:
            logger.error("Unable to open build.gradle file. Aborting deployment...")
    
    logger.info(f"Project exported to '{target_android_project_path}'.")
    
        

def export_source_android_parse_args(o3de_context: exp.O3DEScriptExportContext,
                                     export_config: command_utils.O3DEConfig):
    parser = argparse.ArgumentParser(
                    prog=f'o3de.py export-project -es {__file__}',
                    description="Exports a project as an Android APK that is optionally deployed to an android device. "
                                "In order to use this script, the engine and project must be setup and registered beforehand. ",
                    epilog=exp.CUSTOM_CMAKE_ARG_HELP_EPILOGUE,
                    formatter_class=argparse.RawTextHelpFormatter,
                    add_help=False
    )
    
    eutil.create_common_export_params_and_config(parser, export_config)

    parser.add_argument('--deploy-to-android',action='store_true',help='At completion of build, deploy to a connected android device.')
 
    default_android_build_path = pathlib.Path(export_config.get_value(key=exp.SETTINGS_DEFAULT_ANDROID_BUILD_PATH.key, default=android.DEFAULT_ANDROID_BUILD_FOLDER))
    parser.add_argument('-abp', '--android-build-path', type=str, 
                                          help=f"The location to write the android project scripts to. Default: '{android.DEFAULT_ANDROID_BUILD_FOLDER}'", 
                                          default=default_android_build_path) 

    asset_mode = export_config.get_value(exp.SETTINGS_ANDROID_ASSET_MODE.key, default=exp.ASSET_MODE_LOOSE)
    parser.add_argument('--asset-mode', type=str,
                                            help=f"The mode of asset deployment to use. "
                                                 f" Default: {asset_mode}",
                                            choices=exp.ASSET_MODES,
                                            default=asset_mode)

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
    
    option_android_deploy = export_config.get_parsed_boolean_option(parsed_args=args,
                                                                    key=exp.SETTINGS_ANDROID_DEPLOY.key,
                                                                    enable_attribute='deploy_to_android',
                                                                    disable_attribute='no_deploy_to_android')
    
    target_android_project_path = pathlib.Path(args.android_build_path)

    if not target_android_project_path.is_absolute():
        target_android_project_path = o3de_context.project_path / target_android_project_path

    if args.quiet:
        o3de_logger.setLevel(logging.ERROR)
    else:
        o3de_logger.setLevel(logging.INFO)

    try:
        export_source_android_project(ctx=o3de_context,
                                      target_android_project_path=target_android_project_path,
                                      seedlist_paths=args.seedlist_paths,
                                      seedfile_paths=args.seedfile_paths,
                                      level_names=args.level_names,
                                      asset_pack_mode=args.asset_mode,
                                      engine_centric=option_build_engine_centric,
                                      should_build_all_assets=option_build_assets,
                                      should_build_tools=option_build_tools,
                                      build_config=args.config,
                                      tool_config=args.tool_config,
                                      tools_build_path=args.tools_build_path,
                                      max_bundle_size=args.max_bundle_size,
                                      fail_on_asset_errors=fail_on_asset_errors,
                                      deploy_to_device=option_android_deploy,
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
            export_config = exp.get_export_project_config(project_path=project_path)
        else:
            project_path=o3de_context.project_path
            export_config = exp.get_export_project_config(project_path=project_path)
    except command_utils.O3DEConfigError:
        o3de_logger.debug(f"No project detected at {os.getcwd()}, using default settings from global config.")
        project_name = None
        project_path = None
        export_config = exp.get_export_project_config(project_path=None)
    
    #Resolve the android config
    try:
        android_config = android_support.get_android_config(project_path=project_path)
    except (android_support.AndroidToolError, command_utils.O3DEConfigError):
        android_config = android_support.get_android_config(project_path=None)
        

    args = export_source_android_parse_args(o3de_context, export_config)

    export_source_android_run_command(o3de_context, args, export_config, o3de_logger)
    o3de_logger.info(f"Finished exporting.")
    sys.exit(0)
