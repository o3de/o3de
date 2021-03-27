"""

 All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 its licensors.

 For complete copyright and license terms please see the LICENSE at the root of this
 distribution (the "License"). All use of this software is governed by the License,
 or, if provided, by the license below or the license accompanying this file. Do not
 remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

import os
import sys
from validate_ly_version import _read_version_from_branch_spec
from argparse import ArgumentParser


def main(args):
    waf_branch_spec_file_directory = os.path.join(os.environ['WORKSPACE'], 'dev')
    waf_branch_spec_file_name = 'waf_branch_spec.py'
    
    if not os.path.exists(os.path.join(waf_branch_spec_file_directory, waf_branch_spec_file_name)):
        raise Exception("Invalid workspace directory: {}".format(waf_branch_spec_file_directory))

    waf_branch_spec_version = _read_version_from_branch_spec(waf_branch_spec_file_directory, waf_branch_spec_file_name)
    if waf_branch_spec_version is None:
        raise Exception("Unable to read branch spec version from {}.".format(waf_branch_spec_file_name))

    if args.version:
        waf_branch_spec_version = args.version
        
    if waf_branch_spec_version=='0.0.0.0' and not args.allow_unversioned:
        raise Exception('Version "{}" is invalid. Please specify a valid, non-zero LUMBERYARD_VERSION in {}.'.format(
            waf_branch_spec_version,
            os.path.join(waf_branch_spec_file_directory, waf_branch_spec_file_name)
        ))

    versions = waf_branch_spec_version.split('.')
    if len(versions) != 4:
        raise Exception("Invalid branch spec version '{}'. Must use format 'X.X.X.X'".format(waf_branch_spec_version))
        
    major_version = versions[0]
    minor_version = versions[1]
     
    env_inject_file_path = os.path.join(os.environ['WORKSPACE'], os.environ['ENV_INJECT_FILE'])

    print major_version
    print minor_version
     
    with open(env_inject_file_path, 'w') as env_inject_file:
        env_inject_file.write('MAJOR_VERSION={}\n'.format(major_version))
        env_inject_file.write('MINOR_VERSION={}\n'.format(minor_version))

    
def check_env(*vars):
    missing = []
    for var in vars:
        if var not in os.environ:
            missing += (var,)
    if missing:
        raise Exception("Missing one or more environment variables: {}".format(", ".join(missing)))


if __name__ == "__main__":
    parser = ArgumentParser()
    parser.add_argument('--allow-unversioned', default=False, action='store_true',
                        help="Allow version '0.0.0.0'. Default is to fail if an invalid version is found")
    parser.add_argument('--version', type=str,
                        help="Manually specify version to use, instead of scanning dev root.")
    args = parser.parse_args()
    
    check_env("WORKSPACE", "ENV_INJECT_FILE")
    
    main(args)
