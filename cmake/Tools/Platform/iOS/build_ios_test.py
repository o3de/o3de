#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import argparse
import os
import pathlib
import sys

SCHEME_NAME = 'AzTestRunner'

# Resolve the common python module
ROOT_DEV_PATH = os.path.realpath(os.path.join(os.path.dirname(__file__), '..', '..', '..', '..'))
if ROOT_DEV_PATH not in sys.path:
    sys.path.append(ROOT_DEV_PATH)

from cmake.Tools import common

def build_ios_test(build_dir, configuration):
    build_path = pathlib.Path(build_dir) if os.path.isabs(build_dir) else pathlib.Path(ROOT_DEV_PATH) / build_dir
    if not build_path.is_dir():
        raise common.LmbrCmdError(f"Invalid build directory '{str(build_path)}'")
    
    xcode_build = common.CommandLineExec('/usr/bin/xcodebuild')
    command_line_arguments = ['build-for-testing',
                              '-project', 'Lumberyard.xcodeproj',
                              '-scheme', SCHEME_NAME,
                              '-configuration', configuration,
                              '-allowProvisioningUpdates',
                              '-allowProvisioningDeviceRegistration']
                              
    xcode_out = xcode_build.popen(command_line_arguments, cwd=build_path, shell=False)
    while xcode_out.poll() is None:
        print(xcode_out.stdout.readline())

def main(args):

    parser = argparse.ArgumentParser(description="Launch a test module on a target iOS device.")

    parser.add_argument('-b', '--build-dir',
                        help='The relative build directory to deploy from.',
                        required=True)
                        
    parser.add_argument('-c', '--configuration',
                        help='The build configuration from the build directory for the source deployment files',
                        default='profile')

    parsed_args = parser.parse_args(args)

    build_ios_test(build_dir=parsed_args.build_dir,
                                 configuration=parsed_args.configuration)
    return 0


if __name__ == '__main__':

    try:
        result_code = main(sys.argv[1:])
        exit(result_code)

    except common.LmbrCmdError as err:
        logging.error(str(err))
        exit(err.code)
