#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import argparse
import fnmatch
import glob
import logging
import os
import pathlib
import platform
import psutil
import shutil

from o3de import manifest, utils
from typing import List

# Account for some windows-specific attributes
if platform.system().lower() == 'windows':
    EXECUTABLE_EXTENSION = '.exe'
    O3DE_SCRIPT_NAME = 'o3de.bat'
    GENERATOR = None
    CMAKE_GENERATOR_OPTIONS = ['-DLY_DISABLE_TEST_MODULES=ON']
else:
    EXECUTABLE_EXTENSION = ""
    O3DE_SCRIPT_NAME = 'o3de.sh'
    GENERATOR = "Ninja Multi-Config"
    CMAKE_GENERATOR_OPTIONS = ['-DLY_DISABLE_TEST_MODULES=ON', '-DLY_STRIP_DEBUG_SYMBOLS=ON']

# Regardless of the output package configuration, the tools used for the export process must be built with the profile configuration
PREREQUISITE_TOOL_BUILD_CONFIG = "profile"


class ExportProjectError(RuntimeError):
    """
    Define an error related to the runtime execution of export project so to handle messaging properly
    """
    pass


class O3DEScriptExportContext(object):
    """
    The context object is used to store parameter values and variables throughout the lifetime of an export script's execution.
    It can also be passed onto nested scripts the export script may execute, which can in turn update the context as necessary.
    """
    
    def __init__(self,
                 export_script_path: pathlib.Path,
                 project_path: pathlib.Path,
                 engine_path: pathlib.Path,
                 args: list = [],
                 cmake_additional_build_args: list = []) -> None:

        self._export_script_path = export_script_path
        self._project_path = project_path
        self._engine_path = engine_path
        self._args = args
        self._cmake_additional_build_args = cmake_additional_build_args

        project_json_data = manifest.get_project_json_data(project_path=project_path)
        assert project_json_data, f"Invalid project configuration file '{project_path}/project.json'. Invalid settings."

        project_name = project_json_data.get('project_name')
        assert project_name, f"Invalid project configuration file '{project_path}/project.json'. 'project_name' not found in the settings"

        self._project_name = project_name
        self._is_engine_centric = project_path.is_relative_to(engine_path)

        
    @property
    def export_script_path(self) -> pathlib.Path:
        """The absolute path to the export script being run."""
        return self._export_script_path
    
    @property
    def project_path(self) -> pathlib.Path:
        """The absolute path to the project being exported."""
        return self._project_path
    
    @property
    def engine_path(self) -> pathlib.Path:
        """The absolute path to the engine that the project is built with."""
        return self._engine_path
    
    @property
    def args(self) -> list:
        """A list of the CLI arguments that were unparsed, and passed through for further processing, if necessary."""
        return self._args

    @property
    def cmake_additional_build_args(self) -> list:
        """A list additional CLI arguments that were unparsed, and passed through for further processing, if necessary."""
        return self._cmake_additional_build_args

    @property
    def project_name(self) -> str:
        """The name of the project at the project path"""
        return self._project_name

    @property
    def is_engine_centric(self) -> bool:
        """Flag indicating if the project at project path is an engine-centric project"""
        return self._is_engine_centric


# Helper API
def get_default_asset_platform():
    host_platform_to_asset_platform_map = { 'windows': 'pc',
                                            'linux':   'linux',
                                            'darwin':  'mac' }
    return host_platform_to_asset_platform_map.get(platform.system().lower(), "")

def process_command(args: list,
                    cwd: pathlib.Path = None,
                    env: os._Environ = None) -> int:
    """
    Wrapper for subprocess.Popen, which handles polling the process for logs, reacting to failure, and cleaning up the process.
    :param args: A list of space separated strings which build up the entire command to run. Similar to the command list of subprocess.Popen
    :param cwd: (Optional) The desired current working directory of the command. Useful for commands which require a differing starting environment.
    :param env: (Optional) Environment to use when processing this command.
    :return the exit code of the program that is run or 1 if no arguments were supplied
    """
    if len(args) == 0:
        logging.error("function `process_command` must be supplied a non-empty list of arguments")
        return 1
    return utils.CLICommand(args, cwd, logging.getLogger(), env=env).run()


def execute_python_script(target_script_path: pathlib.Path or str, o3de_context: O3DEScriptExportContext = None) -> int:
    """
    Execute a new python script, using new or existing O3DEScriptExportContexts to streamline communication between multiple scripts
    :param target_script_path: The path to the python script to run.
    :param o3de_context: An O3DEScriptExportContext object that contains necessary data to run the target script. The target script can also write to this context to pass back to its caller.
    :return: return code upon success or failure
    """
    # Prepare import paths for script ease of use
    # Allow for imports from calling script and the target script's local directory
    utils.prepend_to_system_path(pathlib.Path(__file__))
    utils.prepend_to_system_path(target_script_path)

    logging.info(f"Begin loading script '{target_script_path}'...")
    
    return utils.load_and_execute_script(target_script_path, o3de_context = o3de_context, o3de_logger=logging.getLogger())


def _export_script(export_script_path: pathlib.Path, project_path: pathlib.Path, passthru_args: list) -> int:
    if not export_script_path.is_file() or export_script_path.suffix != '.py':
        logging.error(f"Export script path unrecognized: '{export_script_path}'. Please provide a file path to an existing python script with '.py' extension.")
        return 1

    # Compute and validate the given project path is a valid path
    computed_project_path = utils.get_project_path_from_file(export_script_path, project_path)
    if not computed_project_path:
        if project_path:
            logging.error(f"Project path '{project_path}' is invalid: does not contain a project.json file.")
        else:
            logging.error(f"Unable to find project folder associated with file '{export_script_path}'. Please specify using --project-path, or ensure the file is inside a project folder.")
        return 1

    # Collect and isolate any custom cmake build arguments that may have been passed through, use `/` as the separator in the args
    export_process_args = []
    cmake_custom_build_args = []
    additional_arg_marker_found = False
    for arg in passthru_args:
        if arg == '/' and not additional_arg_marker_found:
            additional_arg_marker_found = True
            continue
        if additional_arg_marker_found:
            cmake_custom_build_args.append(arg)
        else:
            export_process_args.append(arg)

    o3de_context = O3DEScriptExportContext(export_script_path=export_script_path,
                                           project_path=computed_project_path,
                                           engine_path=manifest.get_project_engine_path(computed_project_path),
                                           args=export_process_args,
                                           cmake_additional_build_args=cmake_custom_build_args)

    return execute_python_script(export_script_path, o3de_context)

# Export Script entry point
def _run_export_script(args: argparse, passthru_args: list) -> int:
    logging.basicConfig(format=utils.LOG_FORMAT)
    logging.getLogger().setLevel(args.log_level)
    
    return _export_script(args.export_script, args.project_path, passthru_args)


# Argument handling
def add_parser_args(parser) -> None:
    parser.add_argument('-es', '--export-script', type=pathlib.Path, required=True, help="An external Python script to run")
    parser.add_argument('-pp', '--project-path', type=pathlib.Path, required=False,
                        help="Project to export. If not supplied, it will be inferred by the export script.")
    
    parser.add_argument('-ll', '--log-level', default='ERROR',
                        choices=['DEBUG', 'INFO', 'WARNING', 'ERROR', 'CRITICAL'],
                        help="Set the log level")
    
    parser.set_defaults(func=_run_export_script, accepts_partial_args=True)
    

def add_args(subparsers) -> None:
    export_subparser = subparsers.add_parser('export-project')
    add_parser_args(export_subparser)


def get_asset_processor_batch_path(non_mono_build_path: pathlib.Path):
    return non_mono_build_path / f'bin/profile/AssetProcessorBatch{EXECUTABLE_EXTENSION}'


def get_asset_bundler_batch_path(non_mono_build_path: pathlib.Path):
    return non_mono_build_path / f'bin/profile/AssetBundlerBatch{EXECUTABLE_EXTENSION}'


def build_assets(ctx: O3DEScriptExportContext,
                 tools_build_path: pathlib.Path,
                 fail_on_ap_errors: bool,
                 logger: logging.Logger = None) -> None:
    """
    Build the assets for the project
    @param ctx:                 Export Context
    @param tools_build_path:    The tools (cmake) build path to locate AssetProcessorBatch
    @param fail_on_ap_errors:   Option to fail the whole process if an error occurs during asset processing
    @param logger:              Optional Logger
    @return: None
    """

    # Make sure `AssetProcessorBatch` is available
    asset_processor_batch_path = get_asset_processor_batch_path(tools_build_path)
    if not asset_processor_batch_path.exists():
        raise ExportProjectError("Missing AssetProcessorBatch. The pre-requisite tools must be built first.")

    # Build the project assets with the {project_name}.Assets custom target
    if logger:
        logger.info(f"Processing assets for {ctx.project_name}")

    cmake_build_assets_command = [asset_processor_batch_path, "--project-path", ctx.project_path]
    ret = process_command(cmake_build_assets_command,
                          cwd=ctx.engine_path if ctx.is_engine_centric else ctx.project_path)
    if ret != 0:
        if fail_on_ap_errors:
            raise ExportProjectError(f"Error building assets for project {ctx.project_name}.")
        else:
            if logger:
                logger.warning("Some assets failed to processed.")
    if logger:
        logger.info(f"Completed processing assets for {ctx.project_name}!")


def build_export_toolchain(ctx: O3DEScriptExportContext,
                           tools_build_path: pathlib.Path,
                           logger: logging.Logger = None) -> None:
    """
    Build (or rebuild) the export tool chain (AssetProcessorBatch and AssetBundlerBatch)

    @param ctx:                 Export Context
    @param tools_build_path:    The tools (cmake) build path to create the build project for the tools
    @param build_cwd:           The working directory to use when executing A
    @param logger:              Optional Logger
    @return: None
    """

    # Generate the project for export toolchain
    cmake_configure_command = ["cmake", "-B", tools_build_path]

    if ctx.is_engine_centric:
        cmake_configure_command.extend(["-S", ctx.engine_path])
    else:
        cmake_configure_command.extend(["-S", ctx.project_path])

    # Generate the project for the pre-requisite tools
    if GENERATOR:
        cmake_configure_command.extend(["-G", GENERATOR])
    if CMAKE_GENERATOR_OPTIONS:
        cmake_configure_command.extend(CMAKE_GENERATOR_OPTIONS)
    if ctx.is_engine_centric:
        cmake_configure_command.extend([f'-DLY_PROJECTS={ctx.project_path.name}'])
    if logger:
        logger.info(f"Generating tool chain files for project {ctx.project_name}.")
    ret = process_command(cmake_configure_command)
    if ret != 0:
        raise ExportProjectError("Error generating the project for the pre-requisite tools.")

    # Build the project for the pre-requisite tools
    cmake_build_command = ["cmake", "--build", tools_build_path,
                                    "--target", "AssetProcessorBatch", "AssetBundlerBatch",
                                    "--config", PREREQUISITE_TOOL_BUILD_CONFIG]
    if ctx.cmake_additional_build_args:
        cmake_build_command.extend(ctx.cmake_additional_build_args)
    if logger:
        logger.info(f"Building tool chain files for project {ctx.project_name}.")
    ret = process_command(cmake_build_command, )
    if ret != 0:
        raise ExportProjectError("Error building the project for the pre-requisite tools.")
    if logger:
        logger.info(f"Tool chain built successfully.")


def build_game_targets(ctx: O3DEScriptExportContext,
                       build_config: str,
                       game_build_path: pathlib.Path,
                       should_build_game_launcher: bool,
                       should_build_server_launcher: bool,
                       should_build_unified_launcher: bool,
                       allow_registry_overrides: bool,
                       logger: logging.Logger = None) -> None:
    """
    Build the launchers for the project (game, server, unified)

    @param ctx:                             Export Context
    @param build_config:                    The build config to build (profile or release)
    @param game_build_path:                 The cmake build folder target
    @param should_build_game_launcher:      Option to build the game launcher
    @param should_build_server_launcher:    Option to build the server launcher
    @param should_build_unified_launcher:   Option to build the unified launcher
    @param allow_registry_overrides:        Custom Flag argument for 'DALLOW_SETTINGS_REGISTRY_DEVELOPMENT_OVERRIDES' to pass down to the project generation
    @param logger:              Optional Logger
    @return: None
    """

    if not (should_build_server_launcher or should_build_game_launcher or should_build_unified_launcher):
        return

    cmake_configure_command = ["cmake", "-B", game_build_path]
    if ctx.is_engine_centric:
        cmake_configure_command.extend(["-S", ctx.engine_path])
    else:
        cmake_configure_command.extend(["-S", ctx.project_path])

    if GENERATOR:
        cmake_configure_command.extend(["-G", GENERATOR])
    if CMAKE_GENERATOR_OPTIONS:
        cmake_configure_command.extend(CMAKE_GENERATOR_OPTIONS)
    if ctx.is_engine_centric:
        cmake_configure_command.extend([f'-DLY_PROJECTS={ctx.project_path.name}'])

    cmake_configure_command.extend(["-DLY_MONOLITHIC_GAME=1",
                                    f"-DALLOW_SETTINGS_REGISTRY_DEVELOPMENT_OVERRIDES={'0' if not allow_registry_overrides else '1'}"])
    if logger:
        logger.info(f"Generating (monolithic) project the build folder for project {ctx.project_name}")
    ret = process_command(cmake_configure_command)
    if ret != 0:
        raise ExportProjectError(f"Error generating projects for project {ctx.project_name}.")

    mono_build_args = ["cmake", "--build", game_build_path,
                                "--config", build_config,
                                "--target"]
    if should_build_server_launcher:
        mono_build_args.append(f"{ctx.project_name}.ServerLauncher")
    if should_build_game_launcher:
        mono_build_args.append(f"{ctx.project_name}.GameLauncher")
    if should_build_unified_launcher:
        mono_build_args.append(f"{ctx.project_name}.UnifiedLauncher")

    if ctx.cmake_additional_build_args:
        mono_build_args.extend(ctx.cmake_additional_build_args)

    if logger:
        logger.info(f"Building project {ctx.project_name} for export.")
    ret = process_command(mono_build_args)
    if ret != 0:
        raise ExportProjectError("Error building the monolithic launcher(s).")
    if logger:
        logger.info(f"Project {ctx.project_name} built successfully.")


def bundle_assets(ctx: O3DEScriptExportContext,
                  selected_platform: str,
                  seedlist_paths: List[str],
                  non_mono_build_path: pathlib.Path,
                  custom_asset_list_path: pathlib.Path|None = None,
                  max_bundle_size: int = 2048) -> None:
    """
    Execute the 'bundle assets' phase of the export

    @param ctx:                         Export Context
    @param selected_platform:           The desired asset platform
    @param seedlist_paths:              The list of seedlist files
    @param non_mono_build_path:         The path to the tools cmake build project
    @param custom_asset_list_path:      Optional custom asset list path, otherwise use the expectged 'AssetBundling/AssetLists'path in the project folder
    @param max_bundle_size:             The size limit to put on the bundle
    @return: None
    """

    asset_bundler_batch_path = get_asset_bundler_batch_path(non_mono_build_path)
    asset_list_path = (ctx.project_path / 'AssetBundling/AssetLists') if not custom_asset_list_path else custom_asset_list_path

    game_asset_list_path = asset_list_path / f'game_{selected_platform}.assetlist'
    engine_asset_list_path = asset_list_path / f'engine_{selected_platform}.assetlist'
    bundles_path = ctx.project_path / 'AssetBundling/Bundles'

    # Generate the asset lists for the project
    gen_game_asset_list_command = [asset_bundler_batch_path, 'assetLists',
                                                             '--assetListFile', game_asset_list_path,
                                                             '--platform', selected_platform,
                                                             '--project-path', ctx.project_path,
                                                             '--allowOverwrites']
    for seed in seedlist_paths:
        gen_game_asset_list_command.append("--seedListFile")
        gen_game_asset_list_command.append(str(seed))
    ret = process_command(gen_game_asset_list_command,
                          cwd=ctx.engine_path if ctx.is_engine_centric else ctx.project_path)
    if ret != 0:
        raise RuntimeError(f"Error generating game assets lists for {game_asset_list_path}")

    gen_engine_asset_list_command = [asset_bundler_batch_path, "assetLists",
                                                               "--assetListFile", engine_asset_list_path,
                                                               "--platform", selected_platform,
                                                               '--project-path', ctx.project_path,
                                                               "--allowOverwrites",
                                                               "--addDefaultSeedListFiles"]
    ret = process_command(gen_engine_asset_list_command,
                          cwd=ctx.engine_path if ctx.is_engine_centric else ctx.project_path)
    if ret != 0:
        raise RuntimeError(f"Error generating engine assets lists for {engine_asset_list_path}")

    # Generate the bundles. We will place it in the project directory for now, since the files need to be copied multiple times (one for each separate launcher distribution)
    gen_game_bundle_command = [asset_bundler_batch_path, "bundles",
                                                         "--maxSize", str(max_bundle_size),
                                                         "--platform", selected_platform,
                                                         '--project-path', ctx.project_path,
                                                         "--allowOverwrites",
                                                         "--outputBundlePath", bundles_path / f"game_{selected_platform}.pak",
                                                         "--assetListFile", game_asset_list_path]
    ret = process_command(gen_game_bundle_command,
                          cwd=ctx.engine_path if ctx.is_engine_centric else ctx.project_path)
    if ret != 0:
        raise RuntimeError(f"Error generating game bundle for {bundles_path / f'game_{selected_platform}.pak'}")

    gen_engine_bundle_command = [asset_bundler_batch_path, "bundles",
                                                           "--maxSize", str(max_bundle_size),
                                                           "--platform", selected_platform,
                                                           '--project-path', ctx.project_path,
                                                           "--allowOverwrites",
                                                           "--outputBundlePath", bundles_path / f"engine_{selected_platform}.pak",
                                                           "--assetListFile", engine_asset_list_path]
    ret = process_command(gen_engine_bundle_command,
                          cwd=ctx.engine_path if ctx.is_engine_centric else ctx.project_path)
    if ret != 0:
        raise RuntimeError(f"Error generating engine bundle for {bundles_path / f'game_{selected_platform}.pak'}")

    return bundles_path


def kill_existing_processes(project_name: str):
    o3de_process_names = ['o3de', 'editor', 'assetprocessor', f'{project_name.lower()}.serverlauncher',
                          f'{project_name.lower()}.gamelauncher']
    processes_to_kill = []
    for process in psutil.process_iter():

        # strip off .exe and check process name
        if os.path.splitext(process.name())[0].lower() in o3de_process_names and process.name() != O3DE_SCRIPT_NAME:
            processes_to_kill.append(process)

    if processes_to_kill:
        logging.error(
            f"The following processes are still running: {', '.join([process.name() for process in processes_to_kill])}")
        user_input = input(f'Continuing may cause build errors.\nStop them before continuing? (y/n). Quit(q)')
        if user_input.lower() == 'y':
            for p in processes_to_kill:
                p.terminate()
                p.wait()
        elif user_input.lower() == 'q':
            quit()


def setup_launcher_layout_directory(project_path: pathlib.Path,
                                    output_path: pathlib.Path,
                                    asset_platform: str,
                                    mono_build_path: pathlib.Path,
                                    build_config: str,
                                    bundles_to_copy: List[pathlib.Path],
                                    project_file_patterns_to_copy: List[str],
                                    archive_output_format: str = "none",
                                    logger: logging.Logger | None = None,
                                    ignore_file_patterns: List[str] = []) -> None:
    """
    Setup the launcher layout directory for a path

    @param project_path:                    The base project path
    @param output_path:                     The target path for the layout
    @param asset_platform:                  The desired asset platform
    @param mono_build_path:                 The path where the launcher executables cmake build project was created
    @param build_config:                    The build configuration to locate the launcher executables in the cmake build project
    @param bundles_to_copy:                 List of bundles to copy to the layout
    @param project_file_patterns_to_copy:   List of additional file patterns to copy over from the project to the layout
    @param archive_output_format:           The archive format to use when archiving the layout
    @param logger:                          Optional Logger
    @param ignore_file_patterns:            List of additional file ignore patterns to prevent from copying into the layout
    @return:
    """
    if output_path.exists():
        shutil.rmtree(output_path)

    output_cache_path = output_path / 'Cache' / asset_platform

    os.makedirs(output_cache_path, exist_ok=True)

    for bundle in bundles_to_copy:
        shutil.copy(bundle, output_cache_path)

    for file in glob.glob(str(mono_build_path / f'bin/{build_config}/*')):
        file_path = pathlib.Path(file)
        if file_path.is_dir():
            shutil.copytree(file, output_path / file_path.name, dirs_exist_ok=True)
        else:
            # Make sure the individual file is not in any ignore patterns before copying
            skip_file = False
            for ignore_file_pattern in ignore_file_patterns:
                if fnmatch.fnmatch(file, ignore_file_pattern):
                    skip_file = True
                    break
            if not skip_file:
                shutil.copy(file, output_path)

    for project_file_pattern in project_file_patterns_to_copy:
        for file in glob.glob(str(pathlib.PurePath(project_path / project_file_pattern))):
            shutil.copy(file, output_path)

    # Optionally compress the layout directory into an archive if the user requests
    if archive_output_format != "none":
        if logger:
            logger.info(f"Archiving output directory {output_path} (this may take a while)...")
        shutil.make_archive(output_path, archive_output_format, root_dir=output_path)
