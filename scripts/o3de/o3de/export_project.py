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
import sys

from o3de import manifest, utils

class O3DEScriptExportContext(object):
    """
    The context object is used to store parameter values and variables throughout the lifetime of an export script's execution.
    It can also be passed onto nested scripts the export script may execute, which can in turn update the context as necessary.
    """
    
    def __init__(self, export_script_path: pathlib.Path,
                       project_path: pathlib.Path,
                       engine_path: pathlib.Path,
                       args: list = []) -> None:
        self._export_script_path = export_script_path
        self._project_path = project_path
        self._engine_path = engine_path
        self._args = args
        
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

    computed_project_path = utils.get_project_path_from_file(export_script_path, project_path)

    if not computed_project_path:
        if project_path:
            logging.error(f"Project path '{project_path}' is invalid: does not contain a project.json file.")
        else:
            logging.error(f"Unable to find project folder associated with file '{export_script_path}'. Please specify using --project-path, or ensure the file is inside a project folder.")
        return 1
    
    o3de_context = O3DEScriptExportContext(export_script_path= export_script_path,
                                        project_path = computed_project_path,
                                        engine_path = manifest.get_project_engine_path(computed_project_path),
                                        args = passthru_args)

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

