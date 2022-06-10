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

logger = logging.getLogger('o3de.gem_properties')
logging.basicConfig(format=utils.LOG_FORMAT)


def edit_gem_props(gem_path: pathlib.Path = None,
                   gem_name: str = None,
                   new_name: str = None,
                   new_display: str = None,
                   new_origin: str = None,
                   new_type: str = None,
                   new_summary: str = None,
                   new_icon: str = None,
                   new_requirements: str = None,
                   new_documentation_url: str = None,
                   new_license: str = None,
                   new_license_url: str = None,
                   new_version: str = None,
                   new_compatible_engines: list or str = None,
                   remove_compatible_engines: list or str = None,
                   replace_compatible_engines: list or str = None,
                   new_tags: list or str = None,
                   remove_tags: list or str = None,
                   replace_tags: list or str = None
                   ) -> int:

    if not gem_path and not gem_name:
        logger.error(f'Either a gem path or a gem name must be supplied to lookup gem.json')
        return 1
    if not gem_path:
        gem_path = manifest.get_registered(gem_name=gem_name)

    if not gem_path:
        logger.error(f'Error unable locate gem path: No gem with name {gem_name} is registered in any manifest')
        return 1

    gem_json_data = manifest.get_gem_json_data(gem_path=gem_path)
    if not gem_json_data:
        return 1

    update_key_dict = {}
    if new_name:
        if not utils.validate_identifier(new_name):
            logger.error(f'Engine name must be fewer than 64 characters, contain only alphanumeric, "_" or "-"'
                         f' characters, and start with a letter.  {new_name}')
            return 1
        update_key_dict['gem_name'] = new_name
    if new_display:
        update_key_dict['display_name'] = new_display
    if new_origin:
        update_key_dict['origin'] = new_origin
    if new_type:
        update_key_dict['type'] = new_type
    if new_summary:
        update_key_dict['summary'] = new_summary
    if new_icon:
        update_key_dict['icon_path'] = new_icon
    if new_requirements:
        update_key_dict['requirements'] = new_requirements
    if new_documentation_url:
        update_key_dict['documentation_url'] = new_documentation_url
    if new_license:
        update_key_dict['license'] = new_license
    if new_license_url:
        update_key_dict['license_url'] = new_license_url
    if new_version:
        update_key_dict['version'] = new_version

    if new_tags or remove_tags or replace_tags:
        update_key_dict['user_tags'] = utils.update_values_in_key_list(gem_json_data.get('user_tags', []), new_tags,
                                                        remove_tags, replace_tags)

    if new_compatible_engines and not utils.validate_version_specifier_list(new_compatible_engines):
        logger.error(f'Compatible versions must be in the format <engine name><version specifiers>. e.g. o3de==1.0.0.0 \n {new_compatible_engines}')
        return 1

    if remove_compatible_engines and not utils.validate_version_specifier_list(remove_compatible_engines):
        logger.error(f'Compatible versions must be in the format <engine name><version specifiers>. e.g. o3de==1.0.0.0 \n {remove_compatible_engines}')
        return 1

    if replace_compatible_engines and not utils.validate_version_specifier_list(replace_compatible_engines):
        logger.error(f'Compatible versions must be in the format <engine name><version specifiers>. e.g. o3de==1.0.0.0 \n {replace_compatible_engines}')
        return 1

    if new_compatible_engines or remove_compatible_engines or replace_compatible_engines:
        update_key_dict['compatible_engines'] = utils.update_values_in_key_list(gem_json_data.get('compatible_engines', []), 
                                                        new_compatible_engines, remove_compatible_engines, 
                                                        replace_compatible_engines)

    gem_json_data.update(update_key_dict)

    return 0 if manifest.save_o3de_manifest(gem_json_data, pathlib.Path(gem_path) / 'gem.json') else 1


def _edit_gem_props(args: argparse) -> int:
    return edit_gem_props(args.gem_path,
                          args.gem_name,
                          args.gem_new_name,
                          args.gem_display,
                          args.gem_origin,
                          args.gem_type,
                          args.gem_summary,
                          args.gem_icon,
                          args.gem_requirements,
                          args.gem_documentation_url,
                          args.gem_license,
                          args.gem_license_url,
                          args.gem_version,
                          args.add_compatible_engines,
                          args.remove_compatible_engines,
                          args.replace_compatible_engines,
                          args.add_tags,
                          args.remove_tags,
                          args.replace_tags)


def add_parser_args(parser):
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('-gp', '--gem-path', type=pathlib.Path, required=False,
                       help='The path to the gem.')
    group.add_argument('-gn', '--gem-name', type=str, required=False,
                       help='The name of the gem.')
    group = parser.add_argument_group('properties', 'arguments for modifying individual gem properties.')
    group.add_argument('-gnn', '--gem-new-name', type=str, required=False,
                       help='Sets the name for the gem.')
    group.add_argument('-gd', '--gem-display', type=str, required=False,
                       help='Sets the gem display name.')
    group.add_argument('-go', '--gem-origin', type=str, required=False,
                       help='Sets description for gem origin.')
    group.add_argument('-gt', '--gem-type', type=str, required=False, choices=['Code', 'Tool', 'Asset'],
                       help='Sets the gem type. Can only be one of the selected choices.')
    group.add_argument('-gs', '--gem-summary', type=str, required=False,
                       help='Sets the summary description of the gem.')
    group.add_argument('-gi', '--gem-icon', type=str, required=False,
                       help='Sets the path to the projects icon resource.')
    group.add_argument('-gr', '--gem-requirements', type=str, required=False,
                       help='Sets the description of the requirements needed to use the gem.')
    group.add_argument('-gdu', '--gem-documentation-url', type=str, required=False,
                       help='Sets the url for documentation of the gem.')
    group.add_argument('-gl', '--gem-license', type=str, required=False,
                       help='Sets the name for the license of the gem.')
    group.add_argument('-glu', '--gem-license-url', type=str, required=False,
                       help='Sets the url for the license of the gem.')
    group.add_argument('-gv', '--gem-version', type=str, required=False,
                       help='Sets the version of the gem.')
    group = parser.add_mutually_exclusive_group(required=False)
    group.add_argument('-aev', '--add-compatible-engines', type=str, nargs='*', required=False,
                       help='Add engine version(s) this gem is compatible with. Can be specified multiple times.')
    group.add_argument('-dev', '--remove-compatible-engines', type=str, nargs='*', required=False,
                       help='Removes engine version(s) from the compatible_engines property. Can be specified multiple times.')
    group.add_argument('-rev', '--replace-compatible-engines', type=str, nargs='*', required=False,
                       help='Replace engine version(s) in the compatible_engines property. Can be specified multiple times.')
    group = parser.add_mutually_exclusive_group(required=False)
    group.add_argument('-at', '--add-tags', type=str, nargs='*', required=False,
                       help='Adds tag(s) to user_tags property. Can be specified multiple times.')
    group.add_argument('-dt', '--remove-tags', type=str, nargs='*', required=False,
                       help='Removes tag(s) from the user_tags property. Can be specified multiple times.')
    group.add_argument('-rt', '--replace-tags', type=str, nargs='*', required=False,
                       help='Replace tag(s) in user_tags property. Can be specified multiple times.')
    parser.set_defaults(func=_edit_gem_props)


def add_args(subparsers) -> None:
    enable_gem_props_subparser = subparsers.add_parser('edit-gem-properties')
    add_parser_args(enable_gem_props_subparser)


def main():
    the_parser = argparse.ArgumentParser()
    add_parser_args(the_parser)
    the_args = the_parser.parse_args()
    ret = the_args.func(the_args) if hasattr(the_args, 'func') else 1
    sys.exit(ret)


if __name__ == "__main__":
    main()
