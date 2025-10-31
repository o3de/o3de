#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
"""
Contains command to add a gem to a project's project.json file
"""

import argparse
import logging
import os
import pathlib
import sys

from o3de import compatibility, manifest, project_properties, utils

logging.basicConfig(format=utils.LOG_FORMAT)
logger = logging.getLogger('o3de.enable_gem')
logger.setLevel(logging.INFO)


def enable_gem_in_project(gem_name: str = None,
                          gem_path: pathlib.Path = None,
                          project_name: str = None,
                          project_path: pathlib.Path = None,
                          force: bool = False,
                          dry_run: bool = False,
                          optional: bool = False) -> int:
    """
    enable a gem in a projects project.json file
    :param gem_name: name of the gem to add with optional version specifier, e.g. atom>=1.2.3
    :param gem_path: path to the gem to add
    :param project_name: name of to the project to add the gem to
    :param project_path: path to the project to add the gem to
    :param force: bypass version compatibility checks 
    :param dry_run: check version compatibility without modifying anything
    :param optional: mark the gem as optional
    :return: 0 for success or non 0 failure code
    """
    # we need either a project name or path
    if not project_name and not project_path:
        logger.error(f'Must either specify a Project path or Project Name.')
        return 1

    # if project name resolve it into a path
    if project_name and not project_path:
        project_path = manifest.get_registered(project_name=project_name)
    if not project_path:
        logger.error(f'Unable to locate project path from the registered manifest.json files:'
                     f' {str(pathlib.Path.home() / ".o3de/manifest.json")}, engine.json')
        return 1

    project_path = pathlib.Path(project_path).resolve()
    if not project_path.is_dir():
        logger.error(f'Project path {project_path} is not a folder.')
        return 1

    # we need either a gem name or path
    if not gem_name and not gem_path:
        logger.error(f'Must either specify a Gem path or Gem Name.')
        return 1

    # if gem name resolve it into a path
    if gem_name and not gem_path:
        gem_path = manifest.get_registered(gem_name=gem_name, project_path=project_path)
    if not gem_path:
        logger.error(f'Unable to locate "{gem_name}" from the registered gems in:'
                     f' {str(pathlib.Path( "~/.o3de/o3de_manifest.json").expanduser())},'
                     f' {project_path / "project.json"}, engine.json')
        return 1

    gem_path = pathlib.Path(gem_path).resolve()
    # make sure this gem already exists if we're adding.  We can always remove a gem.
    if not gem_path.is_dir():
        logger.error(f'Gem Path {gem_path} does not exist.')
        return 1

    # Read gem.json from the gem path
    gem_json_data = manifest.get_gem_json_data(gem_path=gem_path, project_path=project_path)
    if not gem_json_data:
        logger.error(f'Could not read gem.json content under {gem_path}.')
        return 1

    # include the version specifier if provided e.g. gem==1.2.3
    gem_name = gem_name or gem_json_data['gem_name']

    # check compatibility
    if force:
        logger.info(f'Bypassing version compatibility check for {gem_json_data["gem_name"]}.')
    else:
        # do not check compatibility if the project has not been registered with an engine 
        # because most gems depend on engine gems which would not be found 
        if manifest.get_project_engine_path(project_path):
            # Note: we don't remove gems that are not active or dependencies
            # because they will be implicitly found and activated via cmake 
            incompatible_objects = compatibility.get_gems_project_incompatible_objects([gem_path], [gem_name], project_path)
            if incompatible_objects:
                logger.error(f'{gem_json_data["gem_name"]} has the following dependency compatibility issues and '
                    'requires the --force parameter to activate:\n  '+ 
                    "\n  ".join(incompatible_objects))
                return 1

        if dry_run:
            logger.info(f'{gem_json_data["gem_name"]} is compatible with this project')
            return 0

    return project_properties.edit_project_props(proj_path=project_path,
                                                 new_gem_names=gem_name,
                                                 is_optional_gem=optional)


def add_explicit_gem_activation_for_all_paths(gem_root_folders: list,
                                              project_name: str = None,
                                              project_path: pathlib.Path = None) -> int:
    """
    walks each gem root folder directory structure and adds explicit
    activation of the gems to the project
    :param gem_root_folders: name of the gem to add
    :param project_name: name of to the project to add the gem to
    :param project_path: path to the project to add the gem to
    :return: 0 for success or non 0 failure code
    """
    if not gem_root_folders:
        logger.error('gem_root_folders list cannot be empty')
        return 1

    def stop_on_template_folders(directories: list, filenames: list) -> bool:
        return 'template.json' in filenames

    gem_dirs_set = set()
    ret_val = 0
    for gem_root_folder in gem_root_folders:
        gem_root_folder = pathlib.Path(gem_root_folder).resolve()
        if not gem_root_folder.is_dir():
            logger.error(f'gem root folder of {gem_root_folder} is not a directory')
            ret_val = 1
        for root, dirs, files in os.walk(gem_root_folder):
            # Skip activating gems within template directories
            if stop_on_template_folders(dirs, files):
                dirs[:] = []
            elif 'gem.json' in files:
                gem_dirs_set.add(pathlib.Path(root))

    for gem_dir in sorted(gem_dirs_set):
        # Run the command to add explicit activation even if previous calls failed
        ret_val = enable_gem_in_project(gem_path=gem_dir,
                                        project_name=project_name,
                                        project_path=project_path) or ret_val

    return ret_val


def _run_enable_gem_in_project(args: argparse) -> int:
    if args.all_gem_paths:
        return add_explicit_gem_activation_for_all_paths(
            args.all_gem_paths,
            args.project_name,
            args.project_path
        )
    else:
        return enable_gem_in_project(gem_name=args.gem_name,
                                     gem_path=args.gem_path,
                                     project_name=args.project_name,
                                     project_path=args.project_path,
                                     force=args.force,
                                     dry_run=args.dry_run,
                                     optional=args.optional
                                     )


def add_parser_args(parser):
    """
    add_parser_args is called to add arguments to each command such that it can be
    invoked locally or added by a central python file.
    Ex. Directly run from this file with: python enable_gem.py --project-path "D:/TestProject" --gem-path "D:/TestGem"
    :param parser: the caller passes an argparse parser like instance to this method
    """

    # Sub-commands should declare their own verbosity flag, if desired
    utils.add_verbosity_arg(parser)

    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('-pp', '--project-path', type=pathlib.Path, required=False,
                       help='The path to the project.')
    group.add_argument('-pn', '--project-name', type=str, required=False,
                       help='The name of the project.')
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('-gp', '--gem-path', type=pathlib.Path, required=False,
                       help='The path to the gem.')
    group.add_argument('-gn', '--gem-name', type=str, required=False,
                       help='The name of the gem (e.g. "atom"). May also include a version specifier, e.g. "atom>=1.2.3"')
    group.add_argument('-agp', '--all-gem-paths', type=pathlib.Path, nargs='*', required=False,
                       help='Explicitly activates all gems in the path recursively.')
    group = parser.add_mutually_exclusive_group(required=False)
    group.add_argument('-f', '--force', required=False, action='store_true', default=False,
                       help='Bypass version compatibility checks')
    group.add_argument('-dry', '--dry-run', required=False, action='store_true', default=False,
                       help='Performs a dry run, reporting the result without changing anything.')
    parser.add_argument('-o', '--optional', action='store_true', required=False, default=False,
                        help='Marks the gem as optional so a project can still be configured if not found.')

    parser.set_defaults(func=_run_enable_gem_in_project)


def add_args(subparsers) -> None:
    """
    add_args is called to add subparsers arguments to each command such that it can be
    a central python file such as o3de.py.
    It can be run from the o3de.py script as follows
    call add_args and execute: python o3de.py add-gem-to-project --project-path "D:/TestProject" --gem-path "D:/TestGem"
    :param subparsers: the caller instantiates subparsers and passes it in here
    """
    enable_gem_project_subparser = subparsers.add_parser('enable-gem')
    add_parser_args(enable_gem_project_subparser)


def main():
    """
    Runs enable_gem.py script as standalone script
    """
    # parse the command line args
    the_parser = argparse.ArgumentParser()

    # add args to the parser
    add_parser_args(the_parser)

    # parse args
    the_args = the_parser.parse_args()

    # run
    ret = the_args.func(the_args) if hasattr(the_args, 'func') else 1
    logger.info('Success!' if ret == 0 else 'Completed with issues: result {}'.format(ret))

    # return
    sys.exit(ret)


if __name__ == "__main__":
    main()
