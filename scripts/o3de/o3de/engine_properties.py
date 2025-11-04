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

logger = logging.getLogger('o3de.engine_properties')
logging.basicConfig(format=utils.LOG_FORMAT)


def _edit_gem_names(engine_json: dict,
                    new_gem_names: str or list = None,
                    delete_gem_names: str or list = None,
                    replace_gem_names: str or list = None,
                    is_optional: bool = False):
    """
    Edits and modifies the 'gem_names' list in the engine_json parameter.
    :param engine_json: The engine json data
    :param new_gem_names: Any gem names to be added to the list
    :param delete_gem_names: Any gem names to be removed from the list
    :param replace_gem_names: A list of gem names that will completely replace the current list
    :param is_optional: Only applies to new_gem_names, when true will add an 'optional' property to the gem(s)
    """
    if new_gem_names:
        add_list = new_gem_names.split() if isinstance(new_gem_names, str) else new_gem_names
        if is_optional:
            add_list = [dict(name=gem_name, optional=True) for gem_name in add_list]
        engine_json.setdefault('gem_names', []).extend(add_list)

    if delete_gem_names:
        removal_list = delete_gem_names.split() if isinstance(delete_gem_names, str) else delete_gem_names
        if 'gem_names' in engine_json:
            def in_list(gem: str or dict, remove_list: list) -> bool:
                if isinstance(gem, dict):
                    return gem.get('name', '') in remove_list
                else:
                    return gem in remove_list

            engine_json['gem_names'] = [gem for gem in engine_json['gem_names'] if not in_list(gem, removal_list)]

    if replace_gem_names:
        tag_list = replace_gem_names.split() if isinstance(replace_gem_names, str) else replace_gem_names
        engine_json['gem_names'] = tag_list

    # Remove duplicates from list
    engine_json['gem_names'] = utils.remove_gem_duplicates(engine_json.get('gem_names', []))
    # sort the gem name list so that is written to the project.json in order
    # if the gem_name entry is a dictionary it represents a structure
    # with a "name" and "optional" key. Use the "name" key for sorting
    engine_json['gem_names'] = sorted(engine_json['gem_names'],
                                      key=lambda gem_name: gem_name.get('name') if isinstance(gem_name, dict)
                                      else gem_name)


def edit_engine_props(engine_path: pathlib.Path = None,
                      engine_name: str = None,
                      new_name: str = None,
                      new_version: str = None,
                      new_display_version: str = None,
                      new_gem_names: str or list = None,
                      delete_gem_names: str or list = None,
                      replace_gem_names: str or list = None,
                      new_api_versions: str or list = None,
                      delete_api_versions: str or list = None,
                      replace_api_versions: str or list = None
                      ) -> int:
    if not engine_path and not engine_name:
        engine_path = manifest.get_this_engine_path()

    if not engine_path:
        engine_path = manifest.get_registered(engine_name=engine_name)

    if not engine_path:
        logger.error(f'Error unable locate engine path: No engine with name {engine_name} is registered in any manifest')
        return 1

    engine_json_data = manifest.get_engine_json_data(engine_path=engine_path)
    if not engine_json_data:
        return 1

    if new_name:
        if not utils.validate_identifier(new_name):
            logger.error(f'Engine name must be fewer than 64 characters, contain only alphanumeric, "_" or "-"'
                         f' characters, and start with a letter.  {new_name}')
            return 1
        engine_json_data['engine_name'] = new_name
    if isinstance(new_version, str):
        # O3DEVersion will be deprecated in favor of display_version
        engine_json_data['O3DEVersion'] = new_version
        engine_json_data['version'] = new_version

    if isinstance(new_display_version, str):
        engine_json_data['display_version'] = new_display_version

    if new_api_versions or delete_api_versions or replace_api_versions != None:
        engine_json_data['api_versions'] = utils.update_keys_and_values_in_dict(engine_json_data.get('api_versions',{}),
            new_api_versions, delete_api_versions, replace_api_versions)

    # Update the gem_names field in the engine.json
    _edit_gem_names(engine_json_data, new_gem_names, delete_gem_names, replace_gem_names)

    return 0 if manifest.save_o3de_manifest(engine_json_data, pathlib.Path(engine_path) / 'engine.json') else 1


def _edit_engine_props(args: argparse) -> int:
    return edit_engine_props(args.engine_path,
                             args.engine_name,
                             args.engine_new_name,
                             args.engine_version,
                             args.engine_display_version,
                             args.add_gem_names,
                             args.delete_gem_names,
                             args.replace_gem_names,
                             args.add_api_versions,
                             args.delete_api_versions,
                             args.replace_api_versions
                             )


def add_parser_args(parser):
    group = parser.add_mutually_exclusive_group(required=False)
    group.add_argument('-ep', '--engine-path', type=pathlib.Path, required=False,
                       help='The path to the engine.')
    group.add_argument('-en', '--engine-name', type=str, required=False,
                       help='The name of the engine.')
    group = parser.add_argument_group('properties', 'arguments for modifying individual engine properties.')
    group.add_argument('-enn', '--engine-new-name', type=str, required=False,
                       help='Sets the name for the engine.')
    group.add_argument('-ev', '--engine-version', type=str, required=False,
                       help='Sets the version for the engine.')
    group.add_argument('-edv', '--engine-display-version', type=str, required=False,
                       help='Sets the display version for the engine.')
    group = parser.add_mutually_exclusive_group(required=False)
    group.add_argument('-agn', '--add-gem-names', type=str, nargs='*', required=False,
                       help='Adds gem name(s) to gem_names field. Space delimited list (ex. -agn A B C)')
    group.add_argument('-dgn', '--delete-gem-names', type=str, nargs='*', required=False,
                       help='Removes gem name(s) from the gem_names field. Space delimited list (ex. -dgn A B C')
    group.add_argument('-rgn', '--replace-gem-names', type=str, nargs='*', required=False,
                       help='Replace entirety of gem_names field with space delimited list of values')
    group = parser.add_mutually_exclusive_group(required=False)
    group.add_argument('-aav', '--add-api-versions', type=str, nargs='*', required=False,
                       help='Adds api verion(s) to the api_versions field, replacing existing version if the api entry already exists. Space delimited list of key=value pairs (ex. -aav framework=1.2.3)')
    group.add_argument('-dav', '--delete-api-versions', type=str, nargs='*', required=False,
                       help='Removes api entries from the api_versions field. Space delimited list (ex. -dav framework')
    group.add_argument('-rav', '--replace-api-versions', type=str, nargs='*', required=False,
                       help='Replace entirety of api_versions field with space delimited list of key=value pairs (ex. -rav framework=1.2.3)')
    parser.set_defaults(func=_edit_engine_props)


def add_args(subparsers) -> None:
    enable_engine_props_subparser = subparsers.add_parser('edit-engine-properties')
    add_parser_args(enable_engine_props_subparser)


def main():
    the_parser = argparse.ArgumentParser()
    add_parser_args(the_parser)
    the_args = the_parser.parse_args()
    ret = the_args.func(the_args) if hasattr(the_args, 'func') else 1
    sys.exit(ret)


if __name__ == "__main__":
    main()
