#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import configparser
import hashlib
import logging
import json
import os
import re
import shutil
import stat
import string
import subprocess
import pathlib
import platform

from subprocess import CalledProcessError
from packaging.version import Version
from cmake.Tools import layout_tool

# Text encoding Constants for reading/writing to files.
DEFAULT_TEXT_READ_ENCODING = 'UTF-8'    # The default encoding to use when reading from a text file
DEFAULT_TEXT_WRITE_ENCODING = 'ascii'   # The encoding to use when writing to a text file
ENCODING_ERROR_HANDLINGS = 'ignore'     # What to do if we encounter any encoding errors
DEFAULT_PAK_ROOT = 'Pak'                # The default Pak root folder under engine root where the game paks are built

if platform.system() == 'Windows':
    class PlatformError(WindowsError):
        pass

    # Re-use microsoft error codes since this script is meant to only run on windows host platforms
    ERROR_CODE_FILE_NOT_FOUND = 2
    ERROR_CODE_ERROR_NOT_SUPPORTED = 50
    ERROR_CODE_INVALID_PARAMETER = 87
    ERROR_CODE_CANNOT_COPY = 266
    ERROR_CODE_ERROR_DIRECTORY = 267
else:
    class PlatformError(Exception):
        pass

    # Posix does not match any of the following errors to specific codes, so just the standard '1'
    ERROR_CODE_FILE_NOT_FOUND = 1
    ERROR_CODE_ERROR_NOT_SUPPORTED = 1
    ERROR_CODE_INVALID_PARAMETER = 1
    ERROR_CODE_CANNOT_COPY = 1
    ERROR_CODE_ERROR_DIRECTORY = 1

# Specific error codes that we cant match to any platform code, so just set to the standard '1'
ERROR_CODE_ENVIRONMENT_ERROR = 1
ERROR_CODE_GENERAL_ERROR = 1

ENGINE_ROOT_CHECK_FILE = 'engine.json'
HASH_CHUNK_SIZE = 200000



class LmbrCmdError(Exception):
    """
    Wrapper class to the general exception class where will absorb and prevent the printing of stack.
    We will rely on specific error conditions instead.
    """
    def __init__(self, msg, code=ERROR_CODE_GENERAL_ERROR):
        """
        Init the class
        :param msg:     The detailed error message to print out
        :param code:    The return code to return from the command line execution
        """
        self.msg = msg
        self.code = code

    def __str__(self):
        return str(self.msg)


def read_project_name_from_project_json(project_path):
    project_name = None
    try:
        with (pathlib.Path(project_path) / 'project.json').open('r') as project_file:
            project_json = json.load(project_file)
            project_name = project_json['project_name']
    except OSError as os_error:
        logging.warning(f'Unable to open "project.json" file: {os_error}')
    except json.JSONDecodeError as json_error:
        logging.warning(f'Unable to decode json in {project_file}: {json_error}')
    except KeyError as key_error:
        logging.warning(f'{project_file} is missing project_name key: {key_error}')

    return project_name


def determine_engine_root(starting_path=None):
    """
    Determine the engine root of the engine. By default, the engine root is the engine path, which is determined by walking
    up the current working directory until we find the engine.json marker

    :param starting_path:    Optional starting path to look for the engine.json marker file, otherwise use the current working path
    :return: The root path that is validated to contain engine.json if found, None if not found
    """

    current_path = os.path.normpath(starting_path or os.getcwd())

    check_file = os.path.join(current_path, ENGINE_ROOT_CHECK_FILE)

    while not os.path.isfile(check_file):
        next_path = os.path.dirname(current_path)
        if next_path == current_path:
            # If going up one level results in the same path, we've hit the root
            break
        check_file = os.path.join(next_path, ENGINE_ROOT_CHECK_FILE)
        current_path = next_path

    if not os.path.isfile(check_file):
        return None

    return current_path


def get_config_file_values(config_file_path, keys_to_extract):
    """
    Read an o3de config file and extract specific keys if they are set

    @param config_file_path:    The o3de config file to parse
    @param keys_to_extract:     The specific keys to lookup the values if set
    @return: Dictionary of keys and its values (for matched keys)
    """
    result_map = {}
    with open(config_file_path, 'r') as config_file:
        bootstrap_contents = config_file.read()

        for search_key in keys_to_extract:
            search_result = re.search(r'^\s*{}\s*=\s*([\w\.]+)'.format(search_key), bootstrap_contents, re.MULTILINE)
            if search_result:
                result_value = search_result.group(1)
                result_map[search_key] = result_value

    return result_map


def get_bootstrap_values(bootstrap_dir, keys_to_extract):
    """
    Extract requested values from the bootstrap.setreg file in the Registry folder
    :param bootstrap_dir:       The parent directory of the bootstrap.setreg file
    :param keys_to_extract:     The keys to extract into a dictionary
    :return: Dictionary of keys and its values (for matched keys)
    """
    bootstrap_file = os.path.join(bootstrap_dir, 'bootstrap.setreg')
    if not os.path.isfile(bootstrap_file):
        raise logging.error(f'Bootstrap.setreg file {bootstrap_file} does not exist.')

    result_map = {}
    with open(bootstrap_file, 'r') as f:
        try:
            json_data = json.load(f)
        except Exception as e:
            logging.error(f'Bootstrap.setreg failed to load: {str(e)}')
        else:
            for search_key in keys_to_extract:
                try:
                    search_result = json_data["Amazon"]["AzCore"]["Bootstrap"][search_key]
                except KeyError as e:
                    logging.warning(f'Bootstrap.setreg cannot find /Amazon/AzCore/Bootstrap/{search_key}: {str(e)}')
                else:
                    result_map[search_key] = search_result

    return result_map


def validate_ap_config_asset_type_enabled(engine_root, bootstrap_asset_type):
    """
    Validate that the requested bootstrap asset type was enabled in the asset processor configuration file

    :param engine_root:             The engine root to lookup the AP config file
    :param bootstrap_asset_type:    The asset type to validate
    :return:    True if the asset type was enabled, false if not
    """

    ap_config_file = os.path.join(engine_root, 'Registry', 'AssetProcessorPlatformConfig.setreg')
    if not os.path.isfile(ap_config_file):
        raise LmbrCmdError("Missing required asset processor configuration file at '{}'".format(engine_root),
                           ERROR_CODE_FILE_NOT_FOUND)

    parser = configparser.ConfigParser()
    parser.read([ap_config_file])
    if parser.has_option('Platforms', bootstrap_asset_type):
        enabled_value = parser.get('Platforms', bootstrap_asset_type)
    else:
        # If 'pc' is not set, then default it to 'disable'. For other platforms, their default is 'disabled'
        enabled_value = 'disabled' if bootstrap_asset_type != 'pc' else 'enabled'

    return enabled_value == 'enabled'


def file_fingerprint(path, deep_check=False):
    """
    Calculate a file hash for for a file from either its metadata or its content (deep_check=True)

    :param path:        The absolute path to the file to check its fingerprint. (Does not work on directories)
    :param deep_check:  Flag to use a deep check (hash of the entire file content) or just its metadata (timestamp+size)
    :return: The hash
    """
    if os.path.isdir(path):
        raise LmbrCmdError("Cannot fingerprint '{}' because its a directory".format(path),
                           ERROR_CODE_ERROR_DIRECTORY)
    # Use MD5 hash
    hasher = hashlib.md5()

    # Always start with a shallow check: Start the hash by hashing the mod-time and file size
    path_file_stat = os.stat(path)
    hasher.update(str(path_file_stat.st_mtime).encode('UTF-8'))
    hasher.update(str(path_file_stat.st_size).encode('UTF-8'))

    # If doing a deep check, also include the contents
    if deep_check:
        with open(path, 'rb') as file_to_hash:
            while True:
                content = file_to_hash.read(HASH_CHUNK_SIZE)
                hasher.update(content)
                if not content:
                    break

    return hasher.hexdigest()


def load_template_file(template_file_path, template_env):
    """
    Helper method to load in a template file and return the processed template based on the input template environment
    This will also handle '###' tokens to strip out of the final output completely to support things like adding
    copyrights to the template that is not intended for the output text

    :param template_file_path:  The path to the template file to load
    :param template_env:        The template environment dictionary for the template file to process
    :return:    The processed content from the template file
    :raises:    FileNotFoundError: If the template file path cannot be found
    """
    try:
        template_file_content = template_file_path.resolve(strict=True).read_text(encoding=DEFAULT_TEXT_READ_ENCODING,
                                                                                  errors=ENCODING_ERROR_HANDLINGS)
        # Filter out all lines that start with '###' before replacement
        filtered_template_file_content = (str(re.sub('###.*', '', template_file_content)).strip())

        return string.Template(filtered_template_file_content).substitute(template_env)
    except FileNotFoundError:
        raise FileNotFoundError(f"Invalid file path. Cannot find template file located at {str(template_file_path)}")


# Determine the possible file extensions for executable files based on the host platform
PLATFORM_EXECUTABLE_EXTENSIONS = [''] # Files without extensions are always considered

if platform.system() == 'Windows':
    # Windows manages its executable extensions through the %PATHEXT% environment variable
    path_extensions_str = os.environ.get('PATHEXT', default='.EXE;.COM;.BAT;.CMD')
    PLATFORM_EXECUTABLE_EXTENSIONS.extend([pathext.lower() for pathext in path_extensions_str.split(';')])
elif platform.system() == 'Linux':
    PLATFORM_EXECUTABLE_EXTENSIONS = ['', '.out']
else:
    PLATFORM_EXECUTABLE_EXTENSIONS = ['']


def verify_tool(override_tool_path, tool_name, tool_filename, argument_name, tool_version_argument, tool_version_regex, min_version, max_version):
    """
    Support method to validate a required system tool needed for the build either through an installed tool in the
    environment path, or an override path provided

    :param override_tool_path:      The override path to use to locate the tool's binary. If not provided, the rely on the fact that the tool is in the PATH enviroment
    :param tool_name:               The name of the tool to present to the output
    :param tool_filename:           The filename to use to search for when the override path is provided
    :param argument_name:           The name of the command line argument to display to the user if needed
    :param tool_version_argument:   The argument that the tool expects when querying for the version
    :param tool_version_regex:      The regex used to parse the version number from the 'version' argument
    :param min_version:             Optional min_version to validate against. (None to skip min version validation)
    :param max_version:             Optional max_version to validate against. (None to skip max version validation)
    :return:    Tuple of the resolved tool version and the resolved override tool path if provided
    """

    tool_source = tool_name

    try:
        # Use either the provided gradle override or the gradle in the path environment
        if override_tool_path:

            # The path can be either to an install base folder or its actual 'bin' folder
            if isinstance(override_tool_path, str):
                override_tool_path = pathlib.Path(override_tool_path)
            elif not isinstance(override_tool_path, pathlib.Path):
                raise LmbrCmdError(f"Invalid {tool_name} path argument. '{override_tool_path}' must be a string or Path",
                                   ERROR_CODE_INVALID_PARAMETER)

            file_found = False
            for executable_path_ext in PLATFORM_EXECUTABLE_EXTENSIONS:
                check_tool_filename = f'{tool_filename}{executable_path_ext}'

                check_tool_path = override_tool_path / check_tool_filename
                if check_tool_path.is_file():
                    file_found = True
                    break
                check_tool_path = override_tool_path / 'bin' / check_tool_filename
                if check_tool_path.is_file():
                    file_found = True
                    break

            if not file_found:
                raise LmbrCmdError(f"Invalid {tool_name} path argument. '{override_tool_path}' is not a valid {tool_name} path",
                                   ERROR_CODE_INVALID_PARAMETER)
            resolved_override_tool_path = str(check_tool_path.resolve())
            tool_source = str(check_tool_path.resolve())
            tool_desc = f"{tool_name} path provided in the command line argument '{argument_name}={override_tool_path}' "
        else:
            resolved_override_tool_path = None
            tool_desc = f"installed {tool_name} in the system path"

        # Extract the version and verify
        version_output = subprocess.check_output([tool_source, tool_version_argument],
                                                 shell=(platform.system() == 'Windows'),
                                                 stderr=subprocess.PIPE).decode(DEFAULT_TEXT_READ_ENCODING,
                                                                                ENCODING_ERROR_HANDLINGS)
        version_match = tool_version_regex.search(version_output)
        if not version_match:
            raise RuntimeError()

        # Since we are doing a compare, strip out any non-numeric and non . character from the version otherwise we will get a TypeError on the Version comparison
        result_version_str = re.sub(r"[^\.0-9]", "", str(version_match.group(1)).strip())
        result_version = Version(result_version_str)

        if min_version and result_version < min_version:
            raise LmbrCmdError(f"The {tool_desc} does not meet the minimum version of {tool_name} required ({str(min_version)}).",
                               ERROR_CODE_ENVIRONMENT_ERROR)
        elif max_version and result_version > max_version:
            raise LmbrCmdError(f"The {tool_desc} exceeds maximum version of {tool_name} supported ({str(max_version)}).",
                               ERROR_CODE_ENVIRONMENT_ERROR)

        return result_version, resolved_override_tool_path

    except CalledProcessError as e:
        error_msg = e.output.decode(DEFAULT_TEXT_READ_ENCODING,
                                    ENCODING_ERROR_HANDLINGS)
        raise LmbrCmdError(f"{tool_name} cannot be resolved or there was a problem determining its version number. "
                           f"Either make sure its in the system path environment or a valid path is passed in "
                           f"through the {argument_name} argument.\n{error_msg}",
                           ERROR_CODE_ERROR_NOT_SUPPORTED)
    except (PlatformError, RuntimeError) as e:
        logging.error(f"Call to '{tool_source}' resulted in error: {e}")
        raise LmbrCmdError(f"{tool_name} cannot be resolved or there was a problem determining its version number. "
                           f"Either make sure its in the system path environment or a valid path is passed in "
                           f"through the {argument_name} argument.",
                           ERROR_CODE_ERROR_NOT_SUPPORTED)


def verify_project_and_engine_root(project_root, engine_root):
    """
    Verify the engine root folder and the project root folder. This will perform basic minimal checks
    for validation:
        1. Make sure ${engine_root}/engine.json
        2. Make sure ${project_root}/project.json exists
        3. Make sure that the project.json minimally has a json structure with a 'project_name' attribute

    The project name will be verified by returning the value of 'project_name' from the json file to minimize issues
    on case-insensitive file systems because we rely on the fact that the project name matches the folder in which it resides

    :param project_root:   The project root to verify. If None, skip the project name verification
    :param engine_root:    The engine root directory to verify
    :return: A tuple of the actual 'project_name' from the game's project.json and the pathlib.Path of the engine root if verified
    """
    engine_root_path = pathlib.Path(engine_root)
    if not engine_root_path.exists():
        raise LmbrCmdError(f"Invalid engine root path ({engine_root})",
                           ERROR_CODE_INVALID_PARAMETER)
    # Sanity check: engine.json
    engine_json_path = engine_root_path / ENGINE_ROOT_CHECK_FILE
    if not engine_json_path.exists():
        raise LmbrCmdError(f"Invalid engine root path ({engine_root}). Missing {ENGINE_ROOT_CHECK_FILE}",
                           ERROR_CODE_INVALID_PARAMETER)

    if project_root is None:
        return None, engine_root_path
    else:
        project_path = engine_root_path / project_root
        project_path_project_properties = project_path / 'project.json'
        if not project_path_project_properties.is_file():
            raise LmbrCmdError(f'Invalid project at path "{project_path}". It is missing the project.json file',
                               ERROR_CODE_INVALID_PARAMETER)
        project_name = read_project_name_from_project_json(project_path)
        if not project_name:
            raise LmbrCmdError(f'Invalid project at path "{project_path}". Its project.json does not contains a "project_name" key',
                           ERROR_CODE_INVALID_PARAMETER)
        return project_path, engine_root_path

def remove_dir_path(path):
    """
    Helper function to delete a folder, ignoring all errors if possible
    :param path: The Path to the folder to delete
    """
    if not path.is_dir() and path.is_file():
        # Avoid accidentally deleting a file
        raise RuntimeError("Cannot perform 'remove_dir_path' on file {path}. It must be a directory.")
    if path.exists():
        files_to_delete = []
        for root, dirs, files in os.walk(path):
            for file in files:
                files_to_delete.append(os.path.join(root, file))
        for file in files_to_delete:
            os.chmod(file, stat.S_IWRITE)
            os.remove(file)

        shutil.rmtree(path.resolve(), ignore_errors=True)


def normalize_path_for_settings(path, escape_drive_sep=False):
    """
    Normalize a path for a settings file in case backslashes are treated as escape characters

    :param path:                The path to process (string or pathlib.Path)
    :param escape_drive_sep:    Option to escape any ':' driver separator (windows)
    :return: The normalized path
    """
    if isinstance(path, str):
        processed = path
    else:
        processed = str(path.resolve())

    processed = processed.replace('\\', '/')
    if escape_drive_sep:
        processed = processed.replace(':', '\\:')
    return processed


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


class PlatformSettings(object):
    """
    Platform settings reader

    This will generate a simple settings object based on the cmake generated 'platform.settings' file
    generated by cmake/FileUtil.cmake
    """

    def __init__(self, build_dir):
        platform_last_file = build_dir / 'platform.settings'
        if not platform_last_file.exists():
            raise LmbrCmdError(f"Invalid build directory {build_dir}. Missing 'platform.settings'.")

        config = configparser.ConfigParser()
        config.read(platform_last_file)

        # Look up the general common settings across all platforms
        projects_str = config['settings']['game_projects']
        if projects_str:
            self.projects = projects_str.split(';')

        asset_deploy_mode = config['settings'].get('asset_deploy_mode')
        self.asset_deploy_mode = asset_deploy_mode if asset_deploy_mode else None

        asset_deploy_type = config['settings'].get('asset_deploy_type')
        self.asset_deploy_type = asset_deploy_type if asset_deploy_type else None

        self.override_pak_root = config['settings'].get('override_pak_root', '')

        # Apply all platform-specific settings under the '[<platform_name>]' section in the config file
        platform_name = config['settings']['platform']
        if platform_name in config.sections():
            platform_items = config.items(platform_name)
            for platform_item_key, platform_item_value in platform_items:
                # Prevent any custom platform setting to overwrite a common one
                if platform_item_key in ('asset_deploy_mode', 'asset_deploy_type', 'projects'):
                    logging.warning(f"Reserved key '{platform_item_key}' found in platform section of {str(platform_last_file)}. Ignoring")
                    continue
                setattr(self, platform_item_key, platform_item_value)


def validate_build_dir_and_config(build_dir_name, configuration):
    """
    Validate the build directory and configuration. The current working directory must be the engine root

    :param build_dir_name:  The name of the build directory
    :param configuration:   The configuration name (debug, profile, or release)
    :return: tuple of pathlibs for the build directory, and the configuration directory
    """
    build_dir = pathlib.Path(os.getcwd()) / build_dir_name
    if not build_dir.is_dir():
        raise LmbrCmdError(f"Invalid build directory {build_dir_name}")

    build_config_dir = build_dir / 'bin' / configuration
    if not build_config_dir.is_dir():
        raise LmbrCmdError(f"Output path for build configuration {configuration} not found. Make sure that it was built.")
    return build_dir, build_config_dir


def validate_deployment_arguments(build_dir_name, configuration, project_path):
    """
    Validate the minimal platform deployment arguments

    @param build_dir_name:  The name of the build directory relative to the current working directory
    @param configuration:   The configuration the deployment is based on
    @param project_path:    The path the  project to deploy
    @return: Tuple of (resolved build_dir, game name, asset mode, asset_type, and Pak root folder)
    """

    build_dir, build_config_dir = validate_build_dir_and_config(build_dir_name, configuration)

    platform_settings = PlatformSettings(build_dir)

    if not project_path:
        if not platform_settings.projects:
            raise LmbrCmdError("Missing required game project argument. Unable to determine a default one.")
        internal_project_path = pathlib.PurePath(platform_settings.projects[0]).resolve()
        logging.info(f"Using project_path '{internal_project_path}' as the game project")
    else:
        if project_path not in platform_settings.projects:
            raise LmbrCmdError(f"Game project {project_path} not valid. Was not configured for build directory {build_dir_name}.")

    return build_dir, project_path, platform_settings.asset_deploy_mode, platform_settings.asset_deploy_type, platform_settings.override_pak_root or DEFAULT_PAK_ROOT


class CommandLineExec(object):

    def __init__(self, executable_path):

        if not os.path.isfile(executable_path):
            raise LmbrCmdError(f"Invalid command-line executable '{executable_path}'")
        self.executable_path = executable_path

    def exec(self, arguments, capture_stdout=False, suppress_stderr=False, cwd=None):
        """
        Wrapper to executing calls

        @param arguments:       Arguments to pass to ''
        @param capture_stdout:  If true, capture the stdout of the command to the result object. Enable this if you need the results of the call to continue a workflow
        @param suppress_stderr: If true, suppress capturing the stderr stream
        @param cwd:             Specify an optional current working directory for the execution
        @return: Tuple of the exit code from command line executable, the stdout (if capture_stdout is set to True), and the stderr if any
        """

        try:
            call_args = [self.executable_path]
            if isinstance(arguments, list):
                call_args.extend(arguments)
            else:
                call_args.append(str(arguments))
            logging.debug("exec(%s)", subprocess.list2cmdline(call_args))
            result = subprocess.run(call_args,
                                    shell=(platform.system() == 'Windows'),
                                    capture_output=capture_stdout,
                                    stderr=subprocess.DEVNULL if not capture_stdout and suppress_stderr else None,
                                    encoding='utf-8',
                                    errors='ignore',
                                    cwd=cwd)

            result_code = result.returncode
            result_stdout = result.stdout
            result_stderr = None if suppress_stderr else result.stderr

            return result_code, result_stdout, result_stderr

        except subprocess.CalledProcessError as err:
            raise LmbrCmdError(f"Error trying to call '{self.executable_path}': {str(err)}")

    def popen(self, arguments, cwd=None, shell=True):
        """
        Wrapper to executing calls

        @param arguments:       Arguments to pass to ''
        @param capture_stdout:  If true, capture the stdout of the command to the result object. Enable this if you need the results of the call to continue a workflow
        @param cwd:             Specify an optional current working directory for the execution
        @return: Tuple of the exit code from command line executable, the stdout (if capture_stdout is set to True), and the stderr if any
        """

        try:
            call_args = [self.executable_path]
            if isinstance(arguments, list):
                call_args.extend(arguments)
            else:
                call_args.append(str(arguments))

            logging.debug("exec(%s)", subprocess.list2cmdline(call_args))
            result = subprocess.Popen(call_args,
                                      universal_newlines=True,
                                      bufsize=1,
                                      shell=shell,
                                      stdout=subprocess.PIPE,
                                      stderr=subprocess.STDOUT,
                                      encoding='utf-8',
                                      errors='ignore',
                                      cwd=cwd)

            return result

        except subprocess.CalledProcessError as err:
            raise LmbrCmdError(f"Error trying to call '{self.executable_path}': {str(err)}")


def sync_platform_layout(platform_name, project_path, asset_mode, asset_type, layout_root):
    """
    Perform a layout sync directly on the game project for a platform, game project, asset mode, asset type

    @param platform_name:   The platform (lower) name to sync from
    @param project_path:    The path to project to sync to
    @param asset_mode:      The asset mode to base the sync on
    @param asset_type:      The asset type to base the sync on
    @param layout_root:     The root of the layout to sync to
    """
    layout_tool.ASSET_SYNC_MODE_FUNCTION[asset_mode](target_platform=platform_name,
                                                     project_path=project_path,
                                                     asset_type=asset_type,
                                                     warning_on_missing_assets=True,
                                                     layout_target=layout_root,
                                                     override_pak_folder=None,
                                                     copy=False)


def get_cmake_dependency_modules(build_dir_path, target, module_type):
    """
    Read a dependency registry file for a particular target and get the modules defined for that target.
    If the file does not exist, that means means the either target is not configured, or the special test
    runner setreg is not generated because it is not generated in monolithic mode

    :param build_dir_path:  The build directory (Pathlib) base directory
    :param target:          The name of the target the dependency file is registered for
    :param module_type:     The module type to query for
    :return: List of modules that is registered for the target. An empty list if there is no dependencies for a target or the registry setting isnt set
    """

    dep_modules = []
    try:
        cmake_dep_path = build_dir_path / 'Registry' / f'cmake_dependencies.{target.lower()}.setreg'
        if not cmake_dep_path.is_file():
            return dep_modules

        with cmake_dep_path.open() as cmake_dep_json_file:
            cmake_dep_json = json.load(cmake_dep_json_file)

        test_module_items = cmake_dep_json['Amazon'][module_type]
        for _, test_module_item in test_module_items.items():
            module_file = test_module_item['Module']
            dep_modules.append(module_file)

    except FileNotFoundError:
        raise LmbrCmdError(f'{target} registry not found')

    except (KeyError, json.JSONDecodeError) as err:
        raise LmbrCmdError(f'AzTestRunner registry issue: {str(err)}')

    return dep_modules


def get_test_module_registry(build_dir_path):
    """
    Read a test module registry file for for all test modules that are enabled for a target build directory

    :param build_dir_path:  The target build directory (Pathlib) base directory
    :return: List of modules that is registered for the target. An empty list if there is no dependencies for a target or the registry setting isnt set
    """

    dep_modules = []
    try:
        unit_test_module_path = build_dir_path / 'unit_test_modules.json'

        with unit_test_module_path.open() as unit_test_json_file:
            unit_test_json = json.load(unit_test_json_file)

        test_module_items = unit_test_json['Amazon']
        for _, test_module_item in test_module_items.items():
            module_files = test_module_item['Modules']
            dep_modules.extend(module_files)

    except FileNotFoundError:
        raise LmbrCmdError(f"Unit test registry not found ('{str(unit_test_module_path)}')")

    except (KeyError, json.JSONDecodeError) as err:
        raise LmbrCmdError(f'Unit test registry file issue: {str(err)}')

    return dep_modules


def get_validated_test_modules(test_modules, build_dir_path):
    """
    Validatate the provided test modules against all test modules

    :param test_modules: List of test target names
    :param build_dir_path: The target build directory (Pathlib) base directory
    :return: List of valid test modules that match the input test modules. If the input test modules is an empty list, return all valid test modules
    """

    # Collect the test modules that can be launched
    all_test_modules = get_test_module_registry(build_dir_path=build_dir_path)
    validated_test_modules = []

    # Validate input test targets or use all test modules if no specific test target is supplied
    if test_modules:
        assert isinstance(test_modules, list)
        for test_target_check in test_modules:
            if test_target_check not in all_test_modules:
                raise LmbrCmdError(f"Invalid test module {test_target_check}")
            if isinstance(test_target_check, list):
                validated_test_modules.extend(test_target_check)
            else:
                validated_test_modules.append(test_target_check)
    else:
        validated_test_modules = all_test_modules

    return validated_test_modules
