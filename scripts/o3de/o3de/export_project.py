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
import subprocess

from o3de import manifest, utils
from typing import List
from enum import IntEnum

LOCAL_ENGINE_PATH  = pathlib.Path(__file__).parent.parent.parent.parent

# Account for some windows-specific attributes
CURRENT_PLATFORM = platform.system().lower()

if CURRENT_PLATFORM == 'windows':
    EXECUTABLE_EXTENSION = '.exe'
    O3DE_SCRIPT_NAME = 'o3de.bat'
    GENERATOR = None
    CMAKE_GENERATOR_OPTIONS = ['-DLY_DISABLE_TEST_MODULES=ON']
    CMAKE_MULTI_CONFIGURATION_GENERATOR = True
    ADDITIONAL_PLATFORM_IGNORE_FILES = ['*.pdb', '*.lock']

elif CURRENT_PLATFORM == 'darwin':
    # Test if Ninja is available from the command line to determine the generator and multi-config capability
    test_xcode_result = None
    try:
        test_xcode_result = subprocess.run(['xcodebuild', '--version'])
    except FileNotFoundError:
        pass

    if test_xcode_result and test_xcode_result.returncode == 0:
        GENERATOR = "Xcode"
        CMAKE_MULTI_CONFIGURATION_GENERATOR = True
    else:
        GENERATOR = "Unix Makefiles"
        CMAKE_MULTI_CONFIGURATION_GENERATOR = False

    EXECUTABLE_EXTENSION = ""
    O3DE_SCRIPT_NAME = 'o3de.sh'
    CMAKE_GENERATOR_OPTIONS = ['-DLY_DISABLE_TEST_MODULES=ON']
    ADDITIONAL_PLATFORM_IGNORE_FILES = ['*.dbg', '*.lock']

elif CURRENT_PLATFORM == 'linux':
    # Test if Ninja is available from the command line to determine the generator and multi-config capability
    test_ninja_result = None
    try:
        test_ninja_result = subprocess.run(['ninja', '--version'])
    except FileNotFoundError:
        pass

    if test_ninja_result and test_ninja_result.returncode == 0:
        GENERATOR = "Ninja Multi-Config"
        CMAKE_MULTI_CONFIGURATION_GENERATOR = True
    else:
        GENERATOR = "Unix Makefiles"
        CMAKE_MULTI_CONFIGURATION_GENERATOR = False

    EXECUTABLE_EXTENSION = ""
    O3DE_SCRIPT_NAME = 'o3de.sh'
    CMAKE_GENERATOR_OPTIONS = ['-DLY_DISABLE_TEST_MODULES=ON', '-DLY_STRIP_DEBUG_SYMBOLS=ON']
    ADDITIONAL_PLATFORM_IGNORE_FILES = ['*.dbg', '*.lock']

else:
    # Not a recognized or supported platform
    print(f"Unsupported platform: {CURRENT_PLATFORM}")
    exit(1)

CUSTOM_SCRIPT_HELP_ARGUMENT = '--script-help'

CUSTOM_CMAKE_ARG_HELP_EPILOGUE = "\n\nNote: You can pass in custom arguments to cmake to customize the project generation and build steps with the following:\n\n" \
                                 "  -cca, --cmake-configure-arg  Custom arguments to pass during the 'cmake --configure ...' command used by this process\n" \
                                 "  -cba, --cmake-build-arg      Custom arguments to pass during the 'cmake --build ...' command used by this process\n\n" \
                                 "When these arguments are used, it is expected that the arguments to pass to cmake follows immediately afterwards.\n" \
                                 "To mark the end of the argument list, use '/' to signal the termination of the current cmake custom argument. \n"\
                                 "For example: \n\n" \
                                 "   export-project <script> -pp $PROJECT -out $OUT_DIR -cca -DSTRIP_DEBUG_SYMBOLS=ON / -cba -j 12\n"

# Regardless of the output package configuration, the tools used for the export process must be built with
# the profile configuration
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
                 cmake_additional_configure_args: list = [],
                 cmake_additional_build_args: list = []) -> None:

        self._export_script_path = export_script_path
        self._project_path = project_path
        self._engine_path = engine_path
        self._args = args

        self._cmake_additional_configure_args = cmake_additional_configure_args
        self._cmake_additional_build_args = cmake_additional_build_args

        project_json_data = manifest.get_project_json_data(project_path=project_path)
        assert project_json_data, f"Invalid project configuration file '{project_path}/project.json'. Invalid settings."

        project_name = project_json_data.get('project_name')
        assert project_name, f"Invalid project configuration file '{project_path}/project.json'. 'project_name' not found in the settings"

        self._project_name = project_name


        
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
        """A list additional CLI arguments to use for the cmake build commands that are generated during this process"""
        return self._cmake_additional_build_args

    @property
    def cmake_additional_configure_args(self) -> list:
        """A list additional CLI arguments to use for the cmake configure commands that are generated during this process"""
        return self._cmake_additional_configure_args

    @property
    def project_name(self) -> str:
        """The name of the project at the project path"""
        return self._project_name


class ExportLayoutConfig(object):
    def __init__(self,
                 output_path: pathlib.Path,
                 project_file_patterns: List[str],
                 ignore_file_patterns: List[str]):
        self.output_path = output_path
        self.project_file_patterns = project_file_patterns
        self.ignore_file_patterns = ignore_file_patterns


class LauncherType(IntEnum):
    """
    Enum support to identify the different type of launchers that can be exported into a single package.
    """
    GAME = 1
    SERVER = 2
    UNIFIED = 4
    HEADLESS = 8

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



def extract_cmake_custom_args(arg_list: List[str])->tuple:
    """
    Given an argument list, strip out any custom cmake configure/build args into there own lists and return:
     - List of args that are not custom cmake (configure/build) arguments
     - List of custom cmake configure arguments
     - List of custom cmake build arguments
     - Arg Parse epilogue string to display the custom arguments and how to use them

     refer to CUSTOM_CMAKE_ARG_HELP_EPILOGUE for the help description for the arg options

    :param arg_list: The original argument list to extract the arguments from
    :return     Tuple of arguments and the epilogue string as described in the description
    """

    export_process_args = []
    cmake_configure_args = []
    cmake_build_args = []
    in_cca = False
    in_cba = False

    for arg in arg_list:
        if not in_cca and not in_cba:
            if arg in ('-cca', '--cmake-configure-arg'):
                in_cca = True
                in_cba = False
            elif arg in ('-cba', '--cmake-build-arg'):
                in_cba = True
                in_cca = False
            elif arg == '/':
                raise ExportProjectError("Invalid argument '/'. This argument marks terminator for the '-cca'  or '-cba' argument, but is not part of that argument")
            else:
                export_process_args.append(arg)
        elif in_cca:
            if arg == '/':
                in_cca = False
            elif arg in ('-cba', '--cmake-build-arg'):
                in_cca = False
                in_cca = True
            elif arg not in ('-cca', '--cmake-configure-arg'):
                cmake_configure_args.append(arg)
        elif in_cba:
            if arg == '/':
                in_cba = False
            elif arg in ('-cca', '--cmake-configure-arg'):
                in_cca = True
                in_cba = False
            elif arg not in ('-cba', '--cmake-build-arg'):
                cmake_build_args.append(arg)
        else:
            export_process_args.append(arg)

    return export_process_args, cmake_configure_args, cmake_build_args


def _export_script(export_script_path: pathlib.Path, project_path: pathlib.Path, passthru_args: list) -> int:

    if export_script_path.suffix != '.py':
        logging.error(f"Invalid export script type for '{export_script_path}'. Please provide a file path to an existing python script with '.py' extension.")
        return 1

    # Validate that the export script being passed in is valid.
    validated_export_script_path = None
    if export_script_path.is_absolute():
        # If it is an absolute path, then validate it directly
        if export_script_path.is_file():
            validated_export_script_path = export_script_path
    else:
        # If the script is relative, try to match its root path based on the following order of search priorities
        possible_root_paths = [project_path,
                               LOCAL_ENGINE_PATH,
                               LOCAL_ENGINE_PATH / 'scripts' / 'o3de',
                               LOCAL_ENGINE_PATH / 'scripts' / 'o3de' / 'ExportScripts']
        for possible_root_path in possible_root_paths:
            if possible_root_path is None:
                continue
            if (possible_root_path / export_script_path).is_file():
                validated_export_script_path = possible_root_path / export_script_path
                break

    if validated_export_script_path is None:
        logging.error(f"Invalid export script '{export_script_path}'. File does not exist.")
        return 1

    # Look for a custom export script argument to process the argument '--help' functionality in the pass-through
    # arguments since the parent arg parsing will eat up any '--help' argument before we can get to this point.
    if CUSTOM_SCRIPT_HELP_ARGUMENT in passthru_args:
        return execute_python_script(validated_export_script_path, None)

    # Compute and validate the given project path is a valid path
    computed_project_path = utils.get_project_path_from_file(validated_export_script_path, project_path)
    if not computed_project_path and project_path is not None and not project_path.is_absolute():
        # If the `project_path` is not absolute and we couldn't find it based on the logic from 'utils.get_project_path_from_file', then
        # perform a second attempt assuming that the `project_path` is relative to the engine folder from where this script originates
        computed_project_path = utils.get_project_path_from_file(validated_export_script_path, LOCAL_ENGINE_PATH / project_path)

    if not computed_project_path:
        if project_path:
            logging.error(f"Project path '{project_path}' is invalid: does not contain a project.json file.")
        else:
            logging.error(f"Unable to find project folder associated with file '{validated_export_script_path}'. Please specify using --project-path, or ensure the file is inside a project folder.")
        return 1

    export_process_args, cmake_configure_args, cmake_build_args= extract_cmake_custom_args(passthru_args)

    o3de_context = O3DEScriptExportContext(export_script_path=validated_export_script_path,
                                           project_path=computed_project_path,
                                           engine_path=manifest.get_project_engine_path(computed_project_path),
                                           args=export_process_args,
                                           cmake_additional_configure_args=cmake_configure_args,
                                           cmake_additional_build_args=cmake_build_args)

    return execute_python_script(validated_export_script_path, o3de_context)

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


def get_asset_processor_batch_path(tools_build_path: pathlib.Path,
                                   required: bool = False) -> pathlib.Path:
    """
    Get the expected path to the asset processor tool

    @param tools_build_path:    The tools (cmake) build path to locate AssetProcessorBatch
    @param required:            If true, check if the asset processor actually exists on file at the expected location, and raise an error if not
    @return: Path to the asset processor tool
    """
    asset_processor_batch_path = tools_build_path / f'bin/profile/AssetProcessorBatch{EXECUTABLE_EXTENSION}'
    if required and not asset_processor_batch_path.is_file():
        raise ExportProjectError(f"Missing the 'AssetProcessorBatch' tool, expected at '{asset_processor_batch_path}'")
    return asset_processor_batch_path


def get_asset_bundler_batch_path(tools_build_path: pathlib.Path,
                                 required: bool = False) -> pathlib.Path:
    """
    Get the expected path to the asset bundler tool

    @param tools_build_path:    The tools (cmake) build path to locate AssetBundlerBatch
    @param required:            If true, check if the asset bundler actually exists on file at the expected location, and raise an error if not
    @return: Path to the asset bundler tool
    """

    asset_bundler_batch_path = tools_build_path / f'bin/profile/AssetBundlerBatch{EXECUTABLE_EXTENSION}'
    if required and not asset_bundler_batch_path.is_file():
        raise ExportProjectError(f"Missing the 'AssetBundlerBatch' tool, expected at '{asset_bundler_batch_path}'")
    return asset_bundler_batch_path


def build_assets(ctx: O3DEScriptExportContext,
                 tools_build_path: pathlib.Path,
                 engine_centric: bool,
                 fail_on_ap_errors: bool,
                 logger: logging.Logger = None) -> int:
    """
    Build the assets for the project
    @param ctx:                 Export Context
    @param tools_build_path:    The tools (cmake) build path to locate AssetProcessorBatch
    @param fail_on_ap_errors:   Option to fail the whole process if an error occurs during asset processing
    @param logger:              Optional Logger
    @return: None
    """

    # Make sure `AssetProcessorBatch` is available
    asset_processor_batch_path = get_asset_processor_batch_path(tools_build_path, required=True)
    if not asset_processor_batch_path.exists():
        raise ExportProjectError("Missing AssetProcessorBatch. The pre-requisite tools must be built first.")

    # Build the project assets with the {project_name}.Assets custom target
    if logger:
        logger.info(f"Processing assets for {ctx.project_name}")

    cmake_build_assets_command = [asset_processor_batch_path, "--project-path", ctx.project_path]
    ret = process_command(cmake_build_assets_command,
                          cwd=ctx.engine_path if engine_centric else ctx.project_path)
    if ret != 0:
        if fail_on_ap_errors:
            raise ExportProjectError(f"Error building assets for project {ctx.project_name}.")
        else:
            if logger:
                logger.warning("Some assets failed to processed.")
    if logger:
        logger.info(f"Completed processing assets for {ctx.project_name}!")
    return ret


def build_export_toolchain(ctx: O3DEScriptExportContext,
                           tools_build_path: pathlib.Path,
                           engine_centric: bool,
                           logger: logging.Logger = None) -> None:
    """
    Build (or rebuild) the export tool chain (AssetProcessorBatch and AssetBundlerBatch)

    @param ctx:                                 Export Context
    @param tools_build_path:                    The tools (cmake) build path to create the build project for the tools
    @param engine_centric:                      Option to generate/build an engine-centric workflow
    @param logger:                              Optional Logger
    @return: None
    """

    # Generate the project for export toolchain
    cmake_configure_command = ["cmake", "-B", tools_build_path]

    if engine_centric:
        cmake_configure_command.extend(["-S", ctx.engine_path])
    else:
        cmake_configure_command.extend(["-S", ctx.project_path])

    # Generate the project for the pre-requisite tools
    if GENERATOR:
        cmake_configure_command.extend(["-G", GENERATOR])
    if CMAKE_GENERATOR_OPTIONS:
        cmake_configure_command.extend(CMAKE_GENERATOR_OPTIONS)
    if not CMAKE_MULTI_CONFIGURATION_GENERATOR:
        cmake_configure_command.extend([f'-DCMAKE_BUILD_TYPE={PREREQUISITE_TOOL_BUILD_CONFIG}'])
    if engine_centric:
        cmake_configure_command.extend([f'-DLY_PROJECTS={ctx.project_path}'])
    if ctx.cmake_additional_configure_args:
        cmake_configure_command.extend(ctx.cmake_additional_configure_args)
    if logger:
        logger.info(f"Generating tool chain files for project {ctx.project_name}.")
    ret = process_command(cmake_configure_command)
    if ret != 0:
        raise ExportProjectError("Error generating the project for the pre-requisite tools.")

    # Build the project for the pre-requisite tools
    cmake_build_command = ["cmake", "--build", tools_build_path]

    if CMAKE_MULTI_CONFIGURATION_GENERATOR:
        cmake_build_command.extend(["--config", PREREQUISITE_TOOL_BUILD_CONFIG])

    cmake_build_command.extend(["--target", "AssetProcessorBatch", "AssetBundlerBatch"])

    if ctx.cmake_additional_build_args:
        cmake_build_command.extend(ctx.cmake_additional_build_args)
    if logger:
        logger.info(f"Building tool chain files for project {ctx.project_name}.")
    ret = process_command(cmake_build_command)
    if ret != 0:
        raise ExportProjectError("Error building the project for the pre-requisite tools.")
    if logger:
        logger.info(f"Tool chain built successfully.")


def validate_export_toolchain(tools_build_path: pathlib.Path):
    """
    Validate that the required command line tools are available for the export process

    @param tools_build_path:    The base tools build path
    """
    # Otherwise make sure the tools exist already
    build_tools = [get_asset_bundler_batch_path(tools_build_path),
                   get_asset_processor_batch_path(tools_build_path)]
    tools_missing = [b for b in build_tools if not b.exists()]
    if len(tools_missing) > 0:
        raise ExportProjectError(f"Necessary Build Tools have not been created! The following are missing: {', '.join(tools_missing)}"
                                 "Please ensure that these tools exist before proceeding on with the build!")


def build_game_targets(ctx: O3DEScriptExportContext,
                       build_config: str,
                       game_build_path: pathlib.Path,
                       engine_centric: bool,
                       launcher_types: int,
                       allow_registry_overrides: bool,
                       logger: logging.Logger = None) -> None:
    """
    Build the launchers for the project (game, server, unified, headless)

    @param ctx:                         Export Context
    @param build_config:                The build config to build (profile or release)
    @param game_build_path:             The cmake build folder target
    @engine_centric:                    Option to generate/build an engine-centric workflow
    @additional_cmake_configure_options:List of additional configure arguments to pass to cmake during the cmake project generation process
    @param launcher_types:              The launcher type options (bit mask from the LauncherType enum) to specify which launcher types to build
    @param allow_registry_overrides:    Custom Flag argument for 'DALLOW_SETTINGS_REGISTRY_DEVELOPMENT_OVERRIDES' to pass down to the project generation
    @param logger:                      Optional Logger
    @return: None
    """

    if launcher_types == 0:
        return

    should_build_game_launcher = (launcher_types & LauncherType.GAME) == LauncherType.GAME
    should_build_server_launcher = (launcher_types & LauncherType.SERVER) == LauncherType.SERVER
    should_build_unified_launcher = (launcher_types & LauncherType.UNIFIED) == LauncherType.UNIFIED
    should_build_headless_server_launcher = (launcher_types & LauncherType.HEADLESS) == LauncherType.HEADLESS


    if not (should_build_server_launcher or should_build_game_launcher or should_build_unified_launcher or should_build_headless_server_launcher):
        return

    cmake_configure_command = ["cmake", "-B", game_build_path]
    if engine_centric:
        cmake_configure_command.extend(["-S", ctx.engine_path])
    else:
        cmake_configure_command.extend(["-S", ctx.project_path])

    if GENERATOR:
        cmake_configure_command.extend(["-G", GENERATOR])
    if not CMAKE_MULTI_CONFIGURATION_GENERATOR:
        cmake_configure_command.extend([f'-DCMAKE_BUILD_TYPE={PREREQUISITE_TOOL_BUILD_CONFIG}'])
    if CMAKE_GENERATOR_OPTIONS:
        cmake_configure_command.extend(CMAKE_GENERATOR_OPTIONS)
    if engine_centric:
        cmake_configure_command.extend([f'-DLY_PROJECTS={ctx.project_path}'])
    if ctx.cmake_additional_configure_args:
        cmake_configure_command.extend(ctx.cmake_additional_configure_args)

    cmake_configure_command.extend(["-DLY_MONOLITHIC_GAME=1",
                                    f"-DALLOW_SETTINGS_REGISTRY_DEVELOPMENT_OVERRIDES={'0' if not allow_registry_overrides else '1'}"])
    if logger:
        logger.info(f"Generating (monolithic) project the build folder for project {ctx.project_name}")
    ret = process_command(cmake_configure_command)
    if ret != 0:
        raise ExportProjectError(f"Error generating projects for project {ctx.project_name}.")

    mono_build_args = ["cmake", "--build", game_build_path]

    if CMAKE_MULTI_CONFIGURATION_GENERATOR:
        mono_build_args.extend(["--config", build_config])

    mono_build_args.extend(["--target"])

    if should_build_server_launcher:
        mono_build_args.append(f"{ctx.project_name}.ServerLauncher")
    if should_build_game_launcher:
        mono_build_args.append(f"{ctx.project_name}.GameLauncher")
    if should_build_unified_launcher:
        mono_build_args.append(f"{ctx.project_name}.UnifiedLauncher")
    if should_build_headless_server_launcher:
        mono_build_args.append(f"{ctx.project_name}.HeadlessServerLauncher")

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
                  seedlist_paths: List[pathlib.Path],
                  tools_build_path: pathlib.Path,
                  engine_centric: bool,
                  asset_bundling_path: pathlib.Path | None = None,
                  max_bundle_size: int = 2048) -> pathlib.Path:
    """
    Execute the 'bundle assets' phase of the export

    @param ctx:                      Export Context
    @param selected_platform:        The desired asset platform
    @param seedlist_paths:           The list of seedlist files
    @param tools_build_path:         The path to the tools cmake build project
    @param asset_bundling_path:      The path to use to write all the intermediate and final artifacts from the bundling process
    @param max_bundle_size:          The size limit to put on the bundle
    @return: The path to the bundle
    """

    asset_bundler_batch_path = get_asset_bundler_batch_path(tools_build_path, required=True)
    asset_list_path = asset_bundling_path / 'AssetLists'

    game_asset_list_path = asset_list_path / f'game_{selected_platform}.assetlist'
    engine_asset_list_path = asset_list_path / f'engine_{selected_platform}.assetlist'
    bundles_path = asset_bundling_path / 'Bundles'

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
                          cwd=ctx.engine_path if engine_centric else ctx.project_path)
    if ret != 0:
        raise RuntimeError(f"Error generating game assets lists for {game_asset_list_path}")

    gen_engine_asset_list_command = [asset_bundler_batch_path, "assetLists",
                                                               "--assetListFile", engine_asset_list_path,
                                                               "--platform", selected_platform,
                                                               '--project-path', ctx.project_path,
                                                               "--allowOverwrites",
                                                               "--addDefaultSeedListFiles"]
    ret = process_command(gen_engine_asset_list_command,
                          cwd=ctx.engine_path if engine_centric else ctx.project_path)
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
                          cwd=ctx.engine_path if engine_centric else ctx.project_path)
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
                          cwd=ctx.engine_path if engine_centric else ctx.project_path)
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
                                    asset_platform: str,
                                    launcher_build_path: pathlib.Path,
                                    build_config: str,
                                    bundles_to_copy: List[pathlib.Path],
                                    export_layout: ExportLayoutConfig,
                                    archive_output_format: str = "none",
                                    logger: logging.Logger | None = None
                                    ) -> None:
    """
    Setup the launcher layout directory for a path

    @param project_path:            The base project path
    @param asset_platform:          The desired asset platform
    @param launcher_build_path:     The path where the launcher executables cmake build project was created
    @param build_config:            The build configuration to locate the launcher executables in the cmake build project
    @param bundles_to_copy:         List of bundles to copy to the layout
    @param export_layout:           The export layout information to build the layout directory
    @param archive_output_format:   The archive format to use when archiving the layout
    @param logger:                  Optional Logger
    @return: None
    """
    if export_layout.output_path.exists():
        shutil.rmtree(export_layout.output_path)

    output_cache_path = export_layout.output_path / 'Cache' / asset_platform

    os.makedirs(output_cache_path, exist_ok=True)

    ignore_file_patterns = export_layout.ignore_file_patterns + ADDITIONAL_PLATFORM_IGNORE_FILES

    for bundle in bundles_to_copy:
        shutil.copy(bundle, output_cache_path)

    for file in glob.glob(str(launcher_build_path / f'bin/{build_config}/*')):
        file_path = pathlib.Path(file)

        # Make sure the individual file path is not in any ignore patterns before copying
        skip = False
        for ignore_file_pattern in ignore_file_patterns:
            if fnmatch.fnmatch(file_path.name, ignore_file_pattern):
                skip = True
                break
        if not skip:
            if file_path.is_dir():
                shutil.copytree(file, export_layout.output_path / file_path.name, dirs_exist_ok=True)
            else:
                shutil.copy(file, export_layout.output_path)

    for project_file_pattern in export_layout.project_file_patterns:
        for file in glob.glob(str(pathlib.PurePath(project_path / project_file_pattern))):
            shutil.copy(file, export_layout.output_path)

    # Optionally compress the layout directory into an archive if the user requests
    if archive_output_format != "none":
        if logger:
            logger.info(f"Archiving output directory {export_layout.output_path} (this may take a while)...")
        shutil.make_archive(export_layout.output_path, archive_output_format, root_dir=export_layout.output_path)


def validate_project_artifact_paths(project_path: pathlib.Path,
                                    artifact_paths: List[pathlib.Path],
                                    file_description: str = "file") -> List[pathlib.Path]:
    """
    Validate and adjust project artifact paths as necessary. If paths are provide as relative, then check it
    against the project path for existence.

    @param project_path:            The base project path
    @param artifact_paths:          The project artifact file to check
    @param file_description:        A description of the artifact file type for error reporting
    @return: List of validate project artifact files, all absolute paths and verified to exist
    """
    validated_project_artifact_paths = []
    for input_artifact_path in artifact_paths:
        validated_artifact_path = None
        if input_artifact_path.is_file():
            validated_artifact_path = input_artifact_path
        elif not input_artifact_path.is_absolute():
            abs_input_seedlist_path = project_path / input_artifact_path
            if abs_input_seedlist_path.is_file():
                validated_artifact_path = abs_input_seedlist_path
        if validated_artifact_path is None:
            raise ExportProjectError(f"Invalid {file_description} provided: '{input_artifact_path}' does not exist.")
        else:
            validated_project_artifact_paths.append(validated_artifact_path)
    return validated_project_artifact_paths
