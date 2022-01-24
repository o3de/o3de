#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import argparse
import json
import os
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


def _edit_gem_names(proj_json: dict,
                    new_gem_names: str or list = None,
                    delete_gem_names: str or list = None,
                    replace_gem_names: str or list = None):
    if new_gem_names:
        tag_list = new_gem_names.split() if isinstance(new_gem_names, str) else new_gem_names
        proj_json.setdefault('gem_names', []).extend(tag_list)
    if delete_gem_names:
        removal_list = delete_gem_names.split() if isinstance(delete_gem_names, str) else delete_gem_names
        if 'gem_names' in proj_json:
            for tag in removal_list:
                if tag in proj_json['gem_names']:
                    proj_json['gem_names'].remove(tag)
    if replace_gem_names:
        tag_list = replace_gem_names.split() if isinstance(replace_gem_names, str) else replace_gem_names
        proj_json['gem_names'] = tag_list

    # Remove duplicates from list
    proj_json['gem_names'] = list(dict.fromkeys(proj_json.get('gem_names', [])))


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
                       replace_gem_names: str or list = None
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
    if new_origin:
        proj_json['origin'] = new_origin
    if new_display:
        proj_json['display_name'] = new_display
    if new_summary:
        proj_json['summary'] = new_summary
    if new_icon:
        proj_json['icon_path'] = new_icon
    if new_tags:
        tag_list = new_tags.split() if isinstance(new_tags, str) else new_tags
        proj_json.setdefault('user_tags', []).extend(tag_list)
    if delete_tags:
        removal_list = delete_tags.split() if isinstance(delete_tags, str) else delete_tags
        if 'user_tags' in proj_json:
            for tag in removal_list:
                if tag in proj_json['user_tags']:
                    proj_json['user_tags'].remove(tag)
                else:
                    logger.warning(f'{tag} not found in user_tags for removal.')
        else:
            logger.warning(f'user_tags property not found for removal of {delete_tags}.')
    if replace_tags:
        tag_list = replace_tags.split() if isinstance(replace_tags, str) else replace_tags
        proj_json['user_tags'] = tag_list
    # Update the gem_names field in the project.json
    _edit_gem_names(proj_json, new_gem_names, delete_gem_names, replace_gem_names)

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
                              args.replace_gem_names)


def add_parser_args(parser):
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('-pp', '--project-path', type=pathlib.Path, required=False,
                       help='The path to the project.')
    group.add_argument('-pn', '--project-name', type=str, required=False,
                       help='The name of the project.')
    group = parser.add_argument_group('properties', 'arguments for modifying individual project properties.')
    group.add_argument('-pnn', '--project-new-name', type=str, required=False,
                       help='Sets the name for the project.')
    group.add_argument('-pid', '--project-id', type=str, required=False,
                       help='Sets the ID for the project.')
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
