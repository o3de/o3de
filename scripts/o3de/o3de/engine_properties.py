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

logger = logging.getLogger('o3de.engine_properties')
logging.basicConfig(format=utils.LOG_FORMAT)


def edit_engine_props(engine_path: pathlib.Path = None,
                      engine_name: str = None,
                      new_name: str = None,
                      new_version: str = None) -> int:
    if not engine_path and not engine_name:
        logger.error(f'Either a engine path or a engine name must be supplied to lookup engine.json')
        return 1

    if not new_name and not new_version:
        logger.error('A new engine name or new version, or both must be supplied.')
        return 1

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
    if new_version:
        engine_json_data['O3DEVersion'] = new_version

    return 0 if manifest.save_o3de_manifest(engine_json_data, pathlib.Path(engine_path) / 'engine.json') else 1

def _edit_engine_props(args: argparse) -> int:
    return edit_engine_props(args.engine_path,
                              args.engine_name,
                              args.engine_new_name,
                              args.engine_version)

def add_parser_args(parser):
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('-ep', '--engine-path', type=pathlib.Path, required=False,
                       help='The path to the engine.')
    group.add_argument('-en', '--engine-name', type=str, required=False,
                       help='The name of the engine.')
    group = parser.add_argument_group('properties', 'arguments for modifying individual engine properties.')
    group.add_argument('-enn', '--engine-new-name', type=str, required=False,
                       help='Sets the name for the engine.')
    group.add_argument('-ev', '--engine-version', type=str, required=False,
                       help='Sets the version for the engine.')
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
