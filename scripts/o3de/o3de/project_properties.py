#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

import argparse
import json
import os
import pathlib
import sys
import logging

from o3de import manifest

logger = logging.getLogger()
logging.basicConfig()

def get_project_props(name: str = None, path: pathlib.Path = None) -> dict:
    proj_json = manifest.get_project_json_data(project_name=name, project_path=path)
    if not proj_json:
        param = name if name else path
        logger.error(f'Could not retrieve project.json file for {param}')
        return None
    return proj_json

def edit_project_props(proj_path, proj_name, new_origin, new_display,
                       new_summary, new_icon, new_tag, remove_tag) -> int:
    proj_json = get_project_props(proj_name, proj_path)
    
    if not proj_json:
        return 1

    if new_origin:
        proj_json['origin'] = new_origin
    if new_display:
        proj_json['display_name'] = new_display
    if new_summary:
        proj_json['summary'] = new_summary
    if new_icon:
        proj_json['icon_path'] = new_icon
    if new_tag:
        proj_json.setdefault('user_tags', []).append(new_tag)
    if remove_tag:
        if 'user_tags' in proj_json:
            if remove_tag in proj_json['user_tags']:
                proj_json['user_tags'].remove(remove_tag)
            else:
                logger.warn(f'{remove_tag} not found in user_tags for removal.')
        else:
            logger.warn(f'user_tags property not found for removal of tag {remove_tag}.')

    manifest.save_o3de_manifest(proj_json, pathlib.Path(proj_path) / 'project.json')
    return 0

def _edit_project_props(args: argparse) -> int:
    return edit_project_props(args.project_path,
                              args.project_name,
                               args.project_origin,
                               args.project_display,
                               args.project_summary,
                               args.project_icon,
                               args.project_tag,
                               args.remove_tag)

def add_parser_args(parser):
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('-pp', '--project-path', type=pathlib.Path, required=False,
                       help='The path to the project.')
    group.add_argument('-pn', '--project-name', type=str, required=False,
                       help='The name of the project.')
    group = parser.add_argument_group('properties', 'arguments for modifying individual project properties.')
    group.add_argument('-po', '--project-origin', type=str, required=False,
                       help='Sets description or url for project origin (such as project host, repository, owner...etc).')
    group.add_argument('-pd', '--project-display', type=str, required=False,
                       help='Sets the project display name.')
    group.add_argument('-ps', '--project-summary', type=str, required=False,
                       help='Sets the summary description of the project.')
    group.add_argument('-pi', '--project-icon', type=str, required=False,
                       help='Sets the path to the projects icon resource.')
    group.add_argument('-pt', '--project-tag', type=str, required=False,
                       help='Adds a tag to user_tags property. These tags are intended for documentation and filtering.')
    group.add_argument('-rt', '--remove-tag', type=str, required=False,
                       help='Removes a tag from the user_tags property.')
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
