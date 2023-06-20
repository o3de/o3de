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


def get_project_props(name: str = None, path: pathlib.Path = None, user: bool = False) -> dict:
    proj_json = manifest.get_project_json_data(project_name=name, project_path=path, user=user)
    if not isinstance(proj_json, dict):
        param = name if name else path
        logger.error(f'Could not retrieve project.json file for {param}')
        return None
    return proj_json


def _edit_gem_names(proj_json: dict,
                    new_gem_names: str or list = None,
                    delete_gem_names: str or list = None,
                    replace_gem_names: str or list = None,
                    is_optional_gem: bool = False):
    """
    Edits and modifies the 'gem_names' list in the proj_json parameter.
    :param proj_json: The project json data
    :param new_gem_names: Any gem names to be added to the list
    :param delete_gem_names: Any gem names to be removed from the list
    :param replace_gem_names: A list of gem names that will completely replace the current list
    :param is_optional_gem: Only applies to new_gem_names, when true will add an 'optional' property to the gem(s)
    """
    if new_gem_names:
        add_list = new_gem_names.split() if isinstance(new_gem_names, str) else new_gem_names
        if is_optional_gem:
            add_list = [dict(name=gem_name, optional=True) for gem_name in add_list]

        def is_version_of_gem(candidate: str or dict, gem_name) -> bool:
            candidate_gem_name = candidate if isinstance(candidate, str) else candidate.get('name')
            candidate_gem_name_only, _ = utils.get_object_name_and_optional_version_specifier(candidate_gem_name)
            return candidate_gem_name_only == gem_name

        # remove any versions of the existing gem first
        if proj_json.get('gem_names',[]):
            for new_gem_name in add_list:
                new_gem_name = new_gem_name if isinstance(new_gem_name,str) else new_gem_name['name']
                gem_name_only, _ = utils.get_object_name_and_optional_version_specifier(new_gem_name)
                proj_json['gem_names'] = [gem for gem in proj_json['gem_names'] if not is_version_of_gem(gem, gem_name_only)]

        proj_json.setdefault('gem_names', []).extend(add_list)

    if delete_gem_names:
        removal_list = delete_gem_names.split() if isinstance(delete_gem_names, str) else delete_gem_names
        if 'gem_names' in proj_json:
            def in_list(gem: str or dict, remove_list: list) -> bool:
                gem_name_with_specifier = gem.get('name', '') if isinstance(gem, dict) else gem
                gem_name, _ = utils.get_object_name_and_optional_version_specifier(gem_name_with_specifier)
                return gem_name_with_specifier in remove_list or gem_name in remove_list

            proj_json['gem_names'] = [gem for gem in proj_json['gem_names'] if not in_list(gem, removal_list)]

    if replace_gem_names:
        tag_list = replace_gem_names.split() if isinstance(replace_gem_names, str) else replace_gem_names
        proj_json['gem_names'] = tag_list

    # Remove duplicates from list
    proj_json['gem_names'] = utils.remove_gem_duplicates(proj_json.get('gem_names', []))
    # sort the gem name list so that is written to the project.json in order
    proj_json['gem_names'] = sorted(proj_json['gem_names'],
                                    key=lambda gem_name: gem_name.get('name') if isinstance(gem_name, dict)
                                    else gem_name)


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
                       new_compatible_engines: str or list = None,
                       delete_compatible_engines: str or list = None,
                       replace_compatible_engines: str or list = None,
                       new_version: str = None,
                       is_optional_gem: bool = False,
                       new_engine_api_dependencies: list or str = None,
                       delete_engine_api_dependencies: list or str = None,
                       replace_engine_api_dependencies: list or str = None,
                       user: bool = False,
                       new_engine_path: pathlib.Path = None,
                       new_engine_finder_cmake_path: pathlib.Path = None,
                       ) -> int:
    """
    Edits and modifies the project properties for the project located at 'proj_path' or with the name 'proj_name'.
    :param proj_path: The path to the project folder
    :param proj_name: The name of the project
    :param new_name: The new name for the project
    :param new_id: The new ID for the project
    :param new_origin: The new origin for the project
    :param new_display: The new display name for the project
    :param new_summary: The new gem summary text
    :param new_icon: The new path to the gem's icon file
    :param new_tags: New tags to add to 'user_tags'
    :param delete_tags: Tags to remove from 'user_tags'
    :param replace_tags: Tags to replace 'user_tags' with
    :param new_gem_names: Any gem names to be added to the list
    :param delete_gem_names: Any gem names to be removed from the list
    :param replace_gem_names: A list of gem names that will completely replace the current list
    :param new_engine_name: The new engine name this project is registered with
    :param new_compatible_engines: Compatible engine version specifiers to add e.g. o3de==1.2.3
    :param delete_compatible_engines: Engine version specifiers to remove from 'compatible_engines'
    :param replace_compatible_engines: Engine version specifiers to replace everything in'compatible_engines'
    :param new_version: The new project version e.g. 1.2.3
    :param is_optional_gem: Only applies to new_gem_names, when true will add an 'optional' property to the gem(s)
    :param new_engine_api_dependencies: Engine API version specifiers to add e.g. launcher==1.2.3
    :param remove_engine_api_dependencies: Version specifiers to remove from 'engine_api_dependencies'
    :param replace_engine_api_dependencies: Version specifiers to replace 'engine_api_dependencies' with
    """
    proj_json = get_project_props(proj_name, proj_path, user)

    if not proj_json and not user:
        return 1
    if isinstance(new_name, str):
        if (not user or (user and new_name)) and not utils.validate_identifier(new_name):
            logger.error(f'Project name must be fewer than 64 characters, contain only alphanumeric, "_" or "-" characters, and start with a letter.  {new_name}')
            return 1
        proj_json['project_name'] = new_name
    if isinstance(new_id, str):
        proj_json['project_id'] = new_id
    if isinstance(new_engine_name, str):
        proj_json['engine'] = new_engine_name
    if isinstance(new_origin, str):
        proj_json['origin'] = new_origin
    if isinstance(new_display, str):
        proj_json['display_name'] = new_display
    if isinstance(new_summary, str):
        proj_json['summary'] = new_summary
    if isinstance(new_icon, str):
        proj_json['icon_path'] = new_icon
    if isinstance(new_version, str):
        proj_json['version'] = new_version
    if isinstance(new_engine_path, pathlib.Path):
        if not user:
            logger.error('Setting the engine_path in the shared project.json is not allowed to prevent adding local paths.  Run the command again with the --user argument to set the engine_path locally only.')
            return 1

        if new_engine_path and new_engine_path.name:
            # engine_path is absolute or relative to the project folder to simulate overriding the shared project.json
            engine_path_absolute = proj_path / new_engine_path
            engine_manifest_data = manifest.get_engine_json_data(engine_path=new_engine_path.resolve())
            if not engine_manifest_data:
                logger.error(f'Cannot load engine.json data at path {new_engine_path} ({engine_path_absolute.resolve()}), please verify an engine exists at the supplied location with a valid engine.json file.')
                return 1

        # if the path is empty use an empty string because as_posix() will return "."
        proj_json['engine_path'] = new_engine_path.as_posix() if new_engine_path.name else ""
    if isinstance(new_engine_finder_cmake_path, pathlib.Path):
        proj_json['engine_finder_cmake_path'] = new_engine_finder_cmake_path.as_posix() if new_engine_finder_cmake_path.name else ""

    if new_tags or delete_tags or replace_tags != None:
        proj_json['user_tags'] = utils.update_values_in_key_list(proj_json.get('user_tags', []), new_tags,
                                                        delete_tags, replace_tags)

    if new_gem_names or delete_gem_names or replace_gem_names != None:
        _edit_gem_names(proj_json, new_gem_names, delete_gem_names, replace_gem_names, is_optional_gem)

    if new_compatible_engines or delete_compatible_engines or replace_compatible_engines != None:
        if (new_compatible_engines and not utils.validate_version_specifier_list(new_compatible_engines)) or \
           (delete_compatible_engines and not utils.validate_version_specifier_list(delete_compatible_engines)) or \
           (replace_compatible_engines and not utils.validate_version_specifier_list(replace_compatible_engines)):
            return 1

        proj_json['compatible_engines'] = utils.update_values_in_key_list(proj_json.get('compatible_engines', []),
                                            new_compatible_engines, delete_compatible_engines, replace_compatible_engines)

    if new_engine_api_dependencies or delete_engine_api_dependencies or replace_engine_api_dependencies != None:
        if (new_engine_api_dependencies and not utils.validate_version_specifier_list(new_engine_api_dependencies)) or \
           (delete_engine_api_dependencies and not utils.validate_version_specifier_list(delete_engine_api_dependencies)) or \
           (replace_engine_api_dependencies and not utils.validate_version_specifier_list(replace_engine_api_dependencies)):
            return 1

        proj_json['engine_api_dependencies'] = utils.update_values_in_key_list(proj_json.get('engine_api_dependencies', []),
                                                new_engine_api_dependencies, delete_engine_api_dependencies, replace_engine_api_dependencies)

    if user:
        # remove all empty overrides
        keys = proj_json.copy().keys()
        for key in keys:
            if proj_json.get(key,'') == '':
                del proj_json[key]

    proj_json_path = pathlib.Path(proj_path) if not user else pathlib.Path(proj_path) / 'user'
    return 0 if manifest.save_o3de_manifest(proj_json, proj_json_path / 'project.json') else 1


def _edit_project_props(args: argparse) -> int:
    return edit_project_props(proj_path=args.project_path,
                              proj_name=args.project_name,
                              new_name=args.project_new_name,
                              new_id=args.project_id,
                              new_origin=args.project_origin,
                              new_display=args.project_display,
                              new_summary=args.project_summary,
                              new_icon=args.project_icon,
                              new_tags=args.add_tags,
                              delete_tags=args.delete_tags,
                              replace_tags=args.replace_tags,
                              new_gem_names=args.add_gem_names,
                              delete_gem_names=args.delete_gem_names,
                              replace_gem_names=args.replace_gem_names,
                              new_engine_name=args.engine_name,
                              new_compatible_engines=args.add_compatible_engines,
                              delete_compatible_engines=args.delete_compatible_engines,
                              replace_compatible_engines=args.replace_compatible_engines,
                              new_version=args.project_version,
                              is_optional_gem=False,
                              new_engine_api_dependencies=args.add_engine_api_dependencies,
                              delete_engine_api_dependencies=args.delete_engine_api_dependencies,
                              replace_engine_api_dependencies=args.replace_engine_api_dependencies,
                              user=args.user,
                              new_engine_path=args.engine_path,
                              new_engine_finder_cmake_path=args.engine_finder_cmake_path
                              )


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
    group.add_argument('-efcp', '--engine-finder-cmake-path', type=pathlib.Path, required=False,
                       help='Sets the path to the engine finder cmake file for this project.')
    group.add_argument('-ep', '--engine-path', type=pathlib.Path, required=False,
                       help='Sets the engine path for the project. This setting is only allowed with the --user argument to avoid adding local paths to the shared project.json')
    group.add_argument('-po', '--project-origin', type=str, required=False,
                       help='Sets description or url for project origin (such as project host, repository, owner...etc).')
    group.add_argument('-pd', '--project-display', type=str, required=False,
                       help='Sets the project display name.')
    group.add_argument('-ps', '--project-summary', type=str, required=False,
                       help='Sets the summary description of the project.')
    group.add_argument('-pi', '--project-icon', type=str, required=False,
                       help='Sets the path to the projects icon resource.')
    group.add_argument('--user', action='store_true', required=False, default=False,
                       help='Make changes to the <project>/user/project.json only. This is useful to locally override settings in <project>/project.json which are shared.')
    group = parser.add_mutually_exclusive_group(required=False)
    group.add_argument('-at', '--add-tags', type=str, nargs='*', required=False,
                       help='Adds tag(s) to user_tags property. Space delimited list (ex. -at A B C)')
    group.add_argument('-dt', '--delete-tags', type=str, nargs='*', required=False,
                       help='Removes tag(s) from the user_tags property. Space delimited list (ex. -dt A B C')
    group.add_argument('-rt', '--replace-tags', type=str, nargs='*', required=False,
                       help='Replace entirety of user_tags property with space delimited list of values')
    group = parser.add_mutually_exclusive_group(required=False)
    group.add_argument('-agn', '--add-gem-names', type=str, nargs='*', required=False,
                       help='Adds gem name(s) to gem_names field. Space delimited list (ex. -agn A B C)')
    group.add_argument('-dgn', '--delete-gem-names', type=str, nargs='*', required=False,
                       help='Removes gem name(s) from the gem_names field. Space delimited list (ex. -dgn A B C')
    group.add_argument('-rgn', '--replace-gem-names', type=str, nargs='*', required=False,
                       help='Replace entirety of gem_names field with space delimited list of values')
    group = parser.add_mutually_exclusive_group(required=False)
    group.add_argument('-aev', '--add-compatible-engines', type=str, nargs='*', required=False,
                       help='Add engine version(s) this project is compatible with. Space delimited list (ex. -aev o3de>=1.2.3 o3de-sdk~=2.3).')
    group.add_argument('-dev', '--delete-compatible-engines', type=str, nargs='*', required=False,
                       help='Removes engine version(s) from the compatible_engines property. Space delimited list (ex. -dev o3de>=1.2.3 o3de-sdk~=2.3).')
    group.add_argument('-rev', '--replace-compatible-engines', type=str, nargs='*', required=False,
                       help='Replace entirety of compatible_engines field with space delimited list of values.')
    group = parser.add_mutually_exclusive_group(required=False)
    group.add_argument('-aav', '--add-engine-api-dependencies', type=str, nargs='*', required=False,
                       help='Add engine api dependencies this gem is compatible with. Can be specified multiple times.')
    group.add_argument('-dav', '--delete-engine-api-dependencies', type=str, nargs='*', required=False,
                       help='Removes engine api dependencies from the compatible_engines property. Can be specified multiple times.')
    group.add_argument('-rav', '--replace-engine-api-dependencies', type=str, nargs='*', required=False,
                       help='Replace engine api dependencies in the compatible_engines property. Can be specified multiple times.')
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
