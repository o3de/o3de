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

from o3de import manifest, utils

logger = logging.getLogger('o3de.android')
logging.basicConfig(format=utils.LOG_FORMAT)

class AndroidToolError(Exception):
    pass

def execute_command_line(cwd, arguments:list)-> tuple:

    logging.debug("exec(%s)", subprocess.list2cmdline(call_args))
    result = subprocess.run(arguments,
                            shell=(platform.system() == 'Windows'),
                            capture_output=capture_stdout,
                            encoding='utf-8',
                            errors='ignore',
                            cwd=cwd)

    result_code = result.returncode
    result_stdout = result.stdout
    result_stderr = result.stderr

    return result_code, result_stdout, result_stderr




def configure_android_settings(is_global:bool, project_path:str):

    pass



def run_android_tools(args: argparse) -> int:
    if args.configure:
        if getattr(args, 'global'):

            pass
        else:
            if not args.project:
                pass

            manifest.get_registered(project_name=args.project)
            project_path = resolve_project_path(args.project, os.getcwd())
            pass
        pass


def resolve_project_path(starting_path:str) -> pathlib.Path:
    """
       Attempt to resolve project by attempting to find the first 'project.json' that can be discovered based on the 'starting_path'

       :param starting_path: The starting path to use to look for the O3DE project.json and walk up the folder.

       :return tuple of <project name>, <project_path> if a project was resolved. None, None if not.
    """
    def _get_project_name(project_json_path):
        # Make sure that the project defined with project.json is a valid o3de project and that it is registered properly
        with project_json_path.open(mode='r') as json_data_file:
            try:
                json_data = json.load(json_data_file)
            except json.JSONDecodeError as e:
                raise AndroidToolError(f"Invalid O3DE project at {project_path}: {e}")
            project_name = json_data.get('project_name', None)
            if not project_name:
                raise AndroidToolError(f"Invalid O3DE project at {project_path}: Invalid O3DE project json file")
            return project_name

    # Walk up the path util we find a valid 'project.json'
    current_working_dir = pathlib.Path(starting_path)
    project_json_path = current_working_dir / 'project.json'
    while current_working_dir != current_working_dir.parent and not project_json_path.is_file():
        project_json_path = current_working_dir / 'project.json'
        current_working_dir = current_working_dir.parent
    if not project_json_path.is_file():
        raise AndroidToolError(f"Unable to locate a 'project.json' file based on directory {starting_path}")

    # Extract the project name from resolved project.json file and use it to look up a registered project by its name
    project_path = project_json_path.parent
    resolved_project_name = _get_project_name(project_json_path)
    resolved_project_path = manifest.get_registered(project_name=resolved_project_name)
    if not resolved_project_path:
        raise AndroidToolError(f"Project '{resolved_project_name}' found in {project_json_path} is not registered with O3DE.")

    return resolved_project_name, resolved_project_path


ANDROID_SETTINGS_FILE = '.android_settings'


def resolve_android_global_settings_file() -> pathlib.Path:
    """
    Resolve the location of the android settings file based on whether its global, or by project

    :param is_global:       Flag to indicate that we want to use the global android settings
    :param project_name_arg:    If provided, configure project-specific android settings
    :return:                The path to the android settings file to manipulate
    """
    # Resolve the android settings file
    android_settings_path = manifest.get_o3de_folder() / ANDROID_SETTINGS_FILE
    return android_settings_path

def resolve_android_project_settings_file(project_name_arg:str) -> pathlib.Path:
    """
    Resolve the location of the android settings file based on whether its global, or by project

    :param project_name_arg:    If provided, configure project-specific android settings
    :return:                    The path to the android settings file to manipulate
    """
    # Resolve the android settings file
    if project_name_arg:
        project_path = manifest.get_registered(project_name=project_name_arg)
        if not project_path:
            raise AndroidToolError(f"Unable to resolve project named '{project_name_arg}'. Make sure it is registered with O3DE.")
        logger.info(f"Configuring settings for project '{project_name_arg}' at '{project_path}'")
        android_settings_path = pathlib.Path(project_path) / ANDROID_SETTINGS_FILE
    else:
        project_name_arg, project_path = resolve_project_path(os.getcwd())
        logger.info(f"Configuring settings for project '{project_name_arg}' at '{project_path}'")
        android_settings_path = pathlib.Path(project_path) / ANDROID_SETTINGS_FILE

    return android_settings_path


ANDROID_SETTINGS_GLOBAL_SECTION = 'android'

DEFAULT_ANDROID_SETTINGS = [
    ('sdk_path', ''),
    ('ndk_version', '22.*'),
    ('sdk_api_level', '29')
]

def apply_default_global_android_settings() -> None:
    """
    Apply the default global android settings. This will ensure that :
     1. O3DE has been registered properly
     2. The global android settings file exists
     3. Any missing default values will be populated accordingly
    """
    # Make sure that we have a global .o3de folder
    o3de_folder = manifest.get_o3de_folder()
    assert o3de_folder.is_dir(), 'manifest.get_o3de_folder is expected to ensure the folder exists'

    # Make sure a global settings file exists
    global_android_settings = resolve_android_global_settings_file()
    if not global_android_settings.is_file():
        global_android_settings.write_text(f"[{ANDROID_SETTINGS_GLOBAL_SECTION}]")

    global_config = configparser.ConfigParser()
    global_config.read(global_android_settings.absolute())
    global_section = global_config[ANDROID_SETTINGS_GLOBAL_SECTION]
    modified = False
    for config_key, default_value in DEFAULT_ANDROID_SETTINGS:
        if not config_key in global_section:
            global_section[config_key] = default_value
            modified = True

    if modified:
        with global_android_settings.open('w') as global_android_settings_file:
            global_config.write(global_android_settings_file)

def read_android_config(base_settings_file:pathlib.Path,
                        override_settings_file:pathlib.Path = None) -> dict:
    """
    Read the settings from the android settings file, and apply the optional
    override for values settings on top of that if supplied
    """
    assert base_settings_file.is_file(), f'base_settings_file {base_settings_file} is invalid'
    android_global_config_reader = configparser.ConfigParser()
    android_global_config_reader.read(base_settings_file.absolute())
    android_config = android_global_config_reader[ANDROID_SETTINGS_GLOBAL_SECTION]

    android_settings = {}
    for config_key, config_value in android_config.items():
        android_settings[config_key] = config_value

    if override_settings_file and override_settings_file.is_file():
        android_project_config_reader = configparser.ConfigParser()
        android_project_config_reader.read(override_settings_file.absolute())
        android_project_config = android_project_config_reader[ANDROID_SETTINGS_GLOBAL_SECTION]
        for config_key, config_value in android_project_config.items():
            android_settings[config_key] = config_value

    return android_settings


def validate_tool_version(tool_path:str, version_argument:str, version_regex:str=None)->(str, str):

    #TODO: Add unit test or refactor to utils

    if not os.path.isabs(tool_path):
        # If the path is not absolute, it must be able to resolve from the system path
        tool_cmd = tool_path
        tool_cwd = None
    else:
        tool_cmd = tool_path
        tool_cwd = tool_path.parent

    command_arg = [tool_cmd, version_argument]

    logging.debug("exec(%s)", subprocess.list2cmdline(command_arg))
    result = subprocess.run(command_arg,
                            shell=(platform.system() == 'Windows'),
                            capture_output=True,
                            encoding='utf-8',
                            errors='ignore',
                            cwd=tool_cwd)
    if result.returncode != 0:
        raise AndroidToolError(f"Unable to validate version for '{tool_cmd}': Command returned result code {result.returncode}\n{result.stderr}")

    if version_regex is None:
        version_str = str(result.stdout).strip()
    else:
        match = re.match(version_regex, result.stdout)
        if match is None:
            raise AndroidToolError(f"Unable to read version for tool {tool_cmd} from result {result.stdout}")
        version_str = match.group(0)

    return tool_path, version_str


def list_android_config(global_settings_file:pathlib.Path,
                        project_settings_file:pathlib.Path = None) -> None:

    # TODO: Unit Test
    global_settings = read_android_config(base_settings_file=global_settings_file)
    project_settings = read_android_config(base_settings_file=project_settings_file) if project_settings_file else {}
    print("\nO3DE Android settings:\n")
    for item, value in global_settings.items():
        if item in project_settings:
            # Skip items that are overridden in a project specific setting
            continue
        print(f"{item} = {value}")

    for override_item, override_value in project_settings.items():
        print(f"*{override_item} = {override_value}")
    if project_settings_file:
        print(f"\n* - Specific to {project_settings_file}")


def set_android_config(key:str,
                       value:str,
                       global_settings_file:str,
                       project_settings_file:str = None)->None:
    """
    Given the global android settings file and an optional project settings file, apply a settings value if needed.
    :param key:
    :param value:
    :param global_settings_file:
    :param key:
    """

    assert key, "Missing required key"
    assert global_settings_file
    assert global_settings_file.is_file

    # TODO: establish what are valid keys
    if project_settings_file:
        # Configure for a specific project (override)
        settings_file = project_settings_file
        section_name = ANDROID_SETTINGS_GLOBAL_SECTION
        if not settings_file.is_file():
            settings_file.write(f'[{ANDROID_SETTINGS_GLOBAL_SECTION}]\n')
    else:
        # Configure for the global settings
        settings_file = global_settings_file
        section_name = ANDROID_SETTINGS_GLOBAL_SECTION

    # Read the settings and apply the change if necessary
    project_config = configparser.ConfigParser()
    project_config.read(settings_file.absolute())
    project_config_section = project_config[section_name]
    if project_config_section.get(key, None) != value:
        project_config_section[key] = value

        with settings_file.open('w') as android_settings_file:
            project_config.write(android_settings_file)

REGEX_NAME_AND_VALUE_MATCH_UNQUOTED = re.compile(r'([\w]+)[\s]*=[\s]*((.*))')
REGEX_NAME_AND_VALUE_MATCH_SINGLE_QUOTED = re.compile(r"([\w]+)[\s]*=[\s]*('(.*)')")
REGEX_NAME_AND_VALUE_MATCH_DOUBLE_QUOTED = re.compile(r'([\w]+)[\s]*=[\s]*("(.*)")')

def set_android_config_arg(key_and_value_arg:str,
                           global_settings_file:str,
                           project_settings_file:str = None)->None:
    """
    Apply a named setting to and project override setting or the global setting file
    """

    match = REGEX_NAME_AND_VALUE_MATCH_DOUBLE_QUOTED.match(key_and_value_arg)
    if not match:
        match = REGEX_NAME_AND_VALUE_MATCH_SINGLE_QUOTED.match(key_and_value_arg)
    if not match:
        match = REGEX_NAME_AND_VALUE_MATCH_UNQUOTED.match(key_and_value_arg)
    if not match:
        raise AndroidToolError(f"Invalid setting key argument: {key_and_value_arg}")

    key = match.group(1)
    value = match.group(3)
    set_android_config(key, value, global_settings_file, project_settings_file)


ANDROID_HELP_REGISTER_ANDROID_SDK_MESSAGE = """
Please register the location of the android sdk command line tool.
"""

def validate_android_config(global_settings_file:pathlib.Path,
                            project_settings_file:pathlib.Path = None) -> None:

    android_config = read_android_config(base_settings_file=global_settings_file,
                                         override_settings_file=project_settings_file)

    # Verify the Android SDK setting
    android_sdk_pathname = android_config.get('sdk_path', None)
    if not android_sdk_pathname:
        raise AndroidToolError(f"Missing android sdk key in the android settings. {ANDROID_HELP_REGISTER_ANDROID_SDK_MESSAGE}")
    android_sdk_path = pathlib.Path(android_sdk_pathname)
    if not android_sdk_path.is_file():
        raise AndroidToolError(f"Invalid android sdk path {android_sdk_path} in the android settings. {ANDROID_HELP_REGISTER_ANDROID_SDK_MESSAGE}")
    _, android_sdk_version = validate_tool_version(android_sdk_path, '--version', "[\\d\\.]+")
    logger.info(f"Verified Android SDK Manager version {android_sdk_version}")





def configure_android_options(args: argparse) -> int:
    """
    Configure the android platform settings for generating, building, and deploying android projects.
    """

    try:
        apply_default_global_android_settings()

        is_global = getattr(args,'global',False)
        project_name_arg = getattr(args, 'project', None)

        # Get the global and (optional) project settings file paths
        global_settings_file = resolve_android_global_settings_file()
        project_settings_file = resolve_android_project_settings_file(project_name_arg) if not is_global else None
        if not project_settings_file.is_file():
            project_settings_file.write_text(f"[{ANDROID_SETTINGS_GLOBAL_SECTION}]\n")

        if args.validate:
            validate_android_config(global_settings_file=global_settings_file,
                                    project_settings_file=project_settings_file)
        if args.list:
            list_android_config(global_settings_file=global_settings_file,
                                project_settings_file=project_settings_file)
        if args.set_value:
            set_android_config_arg(key_and_value_arg=args.set_value,
                                   global_settings_file=global_settings_file,
                                   project_settings_file=project_settings_file)

    except AndroidToolError as err:
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




