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
import sys
import subprocess

def parse_args():
    cur_dir = os.path.dirname(os.path.abspath(__file__))
    parser = argparse.ArgumentParser()
    parser.add_argument('-p', '--platform', dest="build_platform", help="Platform to build")
    parser.add_argument('-t', '--type', dest="build_type", help="Target config type to build")
    parser.add_argument('-c', '--config', dest="build_config_filename",
                        default="build_config.json",
                        help="JSON filename in Platform/<platform> that defines build configurations for the platform")
    args = parser.parse_args()

    # Input validation
    if args.build_platform is None:
        print('[ci_build] No platform specified')
        sys.exit(-1)

    if args.build_type is None:
        print('[ci_build] No type specified')
        sys.exit(-1)

    return args

def build(build_config_filename, build_platform, build_type):
    # Read build_config and locate build_type
    cwd_dir = os.getcwd()
    script_dir = os.path.dirname(os.path.abspath(__file__))
    engine_dir = os.path.abspath(os.path.join(script_dir, '../..')) 
    config_dir = os.path.abspath(os.path.join(script_dir, 'Platform', build_platform))
    build_config_abspath = os.path.join(config_dir, build_config_filename)
    if not os.path.exists(build_config_abspath):
        config_dir = os.path.abspath(os.path.join(engine_dir, 'restricted', build_platform, os.path.relpath(script_dir, engine_dir)))
        build_config_abspath = os.path.join(config_dir, build_config_filename)
        if not os.path.exists(build_config_abspath):
            print('[ci_build] File: {} not found'.format(build_config_abspath))
            return -1

    with open(build_config_abspath) as f:
        build_config_json = json.load(f)

    build_type_config = build_config_json[build_type]
    if build_type_config is None:
        print('[ci_build] Build type {} was not found in {}'.format(build_type, build_config_abspath))

    # Load the command to execute
    build_cmd = build_type_config['COMMAND']
    if build_cmd is None:
        print('[ci_build] Build type {} in {} is missing required COMMAND entry'.format(build_type, build_config_abspath))
        return -1

    build_params = build_type_config['PARAMETERS']
    # Parameters are optional, so we could have none

    # build_cmd is relative to the folder where this file is
    build_cmd_path = os.path.join(script_dir, 'Platform/{}/{}'.format(build_platform, build_cmd))
    if not os.path.exists(build_cmd_path):
        config_dir = os.path.abspath(os.path.join(engine_dir, 'restricted', build_platform, os.path.relpath(script_dir, engine_dir)))
        build_cmd_path = os.path.join(config_dir, build_cmd)
        if not os.path.exists(build_cmd_path):
            print('[ci_build] File: {} not found'.format(build_cmd_path))
            return -1

    print('[ci_build] Executing \"{}\"'.format(build_cmd_path))
    print('  cwd = {}'.format(cwd_dir))
    print('  engine_dir = {}'.format(engine_dir))
    print('  parameters:')
    env_params = os.environ.copy()
    env_params['ENGINE_DIR'] = engine_dir
    for v in build_params:
        if v[:6] == "FORCE:":
            env_params[v[6:]] = build_params[v] 
            print('    {} = {} (forced)'.format(v[6:], env_params[v[6:]]))
        else:
            existing_param = env_params.get(v)
            if not existing_param:
                env_params[v] = build_params[v]
            print('    {} = {} {}'.format(v, env_params[v], '(environment override)' if existing_param else ''))
    print('--------------------------------------------------------------------------------', flush=True)
    process_return = subprocess.run([build_cmd_path], cwd=cwd_dir, env=env_params, shell=True)
    print('--------------------------------------------------------------------------------')
    if process_return.returncode != 0:
        print('[ci_build] FAIL: Command {} returned {}'.format(build_cmd_path, process_return.returncode), flush=True)
        return process_return.returncode
    else:
        print('[ci_build] OK', flush=True)

    return 0

if __name__ == "__main__":
    args = parse_args()
    ret = build(args.build_config_filename, args.build_platform, args.build_type)
    sys.exit(ret)
