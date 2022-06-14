#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import argparse
import pathlib
import sys
import logging

from o3de import manifest, utils

logger = logging.getLogger('o3de.project_properties')
logging.basicConfig(format=utils.LOG_FORMAT)


def get_project_props(name: str = None, path: pathlib.Path = None) -> dict:
    proj_json = manifest.get_project_json_data(project_name=name, project_path=path)
    if not proj_json:
        param = name if name else path
        logger.error(f'Could not retrieve project.json file for {param}')
        return None
    return proj_json


def edit_project_props(proj_path: pathlib.Path = None,
                       proj_name: str = None,
                       new_name: str = None,
                       new_id: str = None,
                       new_origin: str = None,
                       new_display: str = None,
                       new_summary: str = None,
                       new_icon: str = None,
                       new_tags: str or list = None,
                       delete_tags: str or list = None,
                       replace_tags: str or list = None,
                       new_gem_names: str or list = None,
                       delete_gem_names: str or list = None,
                       replace_gem_names: str or list = None,
                       new_engine_name: str or list = None,
                       new_engine_dependencies: str or list = None,
                       delete_engine_dependencies: str or list = None,
                       replace_engine_dependencies: str or list = None,
                       new_version: str = None
                       ) -> int:
    proj_json = get_project_props(proj_name, proj_path)
    
    if not proj_json:
        return 1
    if new_name:
        if not utils.validate_identifier(new_name):
            logger.error(f'Project name must be fewer than 64 characters, contain only alphanumeric, "_" or "-" characters, and start with a letter.  {new_name}')
            return 1
        proj_json['project_name'] = new_name
    if new_id:
        proj_json['project_id'] = new_id
    if new_engine_name:
        proj_json['engine'] = new_engine_name
    if new_origin:
        proj_json['origin'] = new_origin
    if new_display:
        proj_json['display_name'] = new_display
    if new_summary:
        proj_json['summary'] = new_summary
    if new_icon:
        proj_json['icon_path'] = new_icon
    if new_version:
        proj_json['version'] = new_version

    if new_tags or delete_tags or replace_tags:
        proj_json['user_tags'] = utils.update_values_in_key_list(proj_json.get('user_tags', []), new_tags,
                                                        delete_tags, replace_tags)
    if new_gem_names or delete_gem_names or replace_gem_names:
        proj_json['gem_names'] = utils.update_values_in_key_list(proj_json.get('gem_names', []), new_gem_names,
                                                        delete_gem_names, replace_gem_names)

    if new_engine_dependencies and not utils.validate_version_specifier_list(new_engine_dependencies):
        logger.error(f'Compatible versions must be in the format <engine name><version specifiers>. e.g. o3de==1.0.0.0 \n {new_engine_dependencies}')
        return 1

    if delete_engine_dependencies and not utils.validate_version_specifier_list(delete_engine_dependencies):
        logger.error(f'Compatible versions must be in the format <engine name><version specifiers>. e.g. o3de==1.0.0.0 \n {delete_engine_dependencies}')
        return 1

    if replace_engine_dependencies and not utils.validate_version_specifier_list(replace_engine_dependencies):
        logger.error(f'Compatible versions must be in the format <engine name><version specifiers>. e.g. o3de==1.0.0.0 \n {replace_engine_dependencies}')
        return 1

    if new_engine_dependencies or delete_engine_dependencies or replace_engine_dependencies:
        proj_json['engine_dependencies'] = utils.update_values_in_key_list(proj_json.get('engine_dependencies', []), new_engine_dependencies,
                                                        delete_engine_dependencies, replace_engine_dependencies)

    return 0 if manifest.save_o3de_manifest(proj_json, pathlib.Path(proj_path) / 'project.json') else 1


def _edit_project_props(args: argparse) -> int:
    return edit_project_props(args.project_path,
                              args.project_name,
                              args.project_new_name,
                              args.project_id,
                              args.project_origin,
                              args.project_display,
                              args.project_summary,
                              args.project_icon,
                              args.add_tags,
                              args.delete_tags,
                              args.replace_tags,
                              args.add_gem_names,
                              args.delete_gem_names,
                              args.replace_gem_names,
                              args.engine_name,
                              args.add_engine_dependencies,
                              args.delete_engine_dependencies,
                              args.replace_engine_dependencies,
                              args.project_version)


def add_parser_args(parser):
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('-pp', '--project-path', type=pathlib.Path, required=False,
                       help='The path to the project.')
    group.add_argument('-pn', '--project-name', type=str, required=False,
                       help='The name of the project.')
    group = parser.add_argument_group('properties', 'arguments for modifying individual project properties.')
    group.add_argument('-pv', '--project-version', type=str, required=False,
                       help='The version of the project.')
    group.add_argument('-pnn', '--project-new-name', type=str, required=False,
                       help='Sets the name for the project.')
    group.add_argument('-pid', '--project-id', type=str, required=False,
                       help='Sets the ID for the project.')
    group.add_argument('-en', '--engine-name', type=str, required=False,
                       help='Sets the engine name for the project.')
    group.add_argument('-po', '--project-origin', type=str, required=False,
                       help='Sets description or url for project origin (such as project host, repository, owner...etc).')
    group.add_argument('-pd', '--project-display', type=str, required=False,
                       help='Sets the project display name.')
    group.add_argument('-ps', '--project-summary', type=str, required=False,
                       help='Sets the summary description of the project.')
    group.add_argument('-pi', '--project-icon', type=str, required=False,
                       help='Sets the path to the projects icon resource.')
    group = parser.add_mutually_exclusive_group(required=False)
    group.add_argument('-at', '--add-tags', type=str, nargs='*', required=False,
                       help='Adds tag(s) to user_tags property. Space delimited list (ex. -at A B C)')
    group.add_argument('-dt', '--delete-tags', type=str, nargs ='*', required=False,
                       help='Removes tag(s) from the user_tags property. Space delimited list (ex. -dt A B C')
    group.add_argument('-rt', '--replace-tags', type=str, nargs ='*', required=False,
                       help='Replace entirety of user_tags property with space delimited list of values')
    group = parser.add_mutually_exclusive_group(required=False)
    group.add_argument('-agn', '--add-gem-names', type=str, nargs='*', required=False,
                       help='Adds gem name(s) to gem_names field. Space delimited list (ex. -at A B C)')
    group.add_argument('-dgn', '--delete-gem-names', type=str, nargs='*', required=False,
                       help='Removes gem name(s) from the gem_names field. Space delimited list (ex. -dt A B C')
    group.add_argument('-rgn', '--replace-gem-names', type=str, nargs='*', required=False,
                       help='Replace entirety of gem_names field with space delimited list of values')
    group = parser.add_mutually_exclusive_group(required=False)
    group.add_argument('-aev', '--add-engine-dependencies', type=str, nargs='*', required=False,
                       help='Add engine version(s) this project is compatible with. Space delimited list (ex. o3de>=1.0.0.0 o3de-sdk~=2.3).')
    group.add_argument('-dev', '--remove-engine-dependencies', type=str, nargs='*', required=False,
                       help='Removes engine version(s) from the engine_dependencies property. Space delimited list (ex. o3de>=1.0.0.0 o3de-sdk~=2.3).')
    group.add_argument('-rev', '--replace-engine-dependencies', type=str, nargs='*', required=False,
                       help='Replace entirety of engine_dependencies field with space delimited list of values.')
    parser.set_defaults(func=_edit_project_props)


def add_args(subparsers) -> None:
    enable_project_props_subparser = subparsers.add_parser('edit-project-properties')
    add_parser_args(enable_project_props_subparser)


def main():
    the_parser = argparse.ArgumentParser()
    add_parser_args(the_parser)
    the_args = the_parser.parse_args()
    ret = the_args.func(the_args) if hasattr(the_args, 'func') else 1
    sys.exit(ret)


if __name__ == "__main__":
    main()
