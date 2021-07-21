#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import argparse
import glob
import logging
import os
import pathlib
import plistlib
import sys

TEST_TARGET_NAME = 'TestLauncherTarget'
TEST_STARTED_STRING = 'TEST STARTED'
TEST_SUCCESS_STRING = 'TEST SUCCEEDED'
TEST_FAILURE_STRING = 'TEST FAILED'

# Resolve the common python module
ROOT_DEV_PATH = os.path.realpath(os.path.join(os.path.dirname(__file__), '..', '..', '..', '..'))
if ROOT_DEV_PATH not in sys.path:
    sys.path.append(ROOT_DEV_PATH)

from cmake.Tools import common

def launch_ios_test(build_dir, target_dev_name, test_target, timeout_secs, test_filter, xctestrun_file):
    
    build_path = pathlib.Path(build_dir) if os.path.isabs(build_dir) else pathlib.Path(ROOT_DEV_PATH) / build_dir
    if not build_path.is_dir():
        raise common.LmbrCmdError(f"Invalid build directory '{str(build_path)}'")
        
    if xctestrun_file:
        xctestrun_file_path = build_path / xctestrun_file
        if not os.path.exists(xctestrun_file_path):
            raise common.LmbrCmdError(f"'{str(xctestrun_file_path)}' not found in '{str(build_path)}'.")
        else:
            test_run_file = xctestrun_file_path
    else:
        # By default, xcodebuild will place the xctestrun file at the root of the build directory. There is only one file.
        # The xctestrun filename has the format <scheme_name>_iphoneos<sdk_version>-<arch>.xctestrun
        # Our scheme name is always "AzTestRunner" and the only iOS architecture we support is arm64.
        # The SDK version is the only variable. But, Xcode only allows the installation of the latest SDK(based on Xcode version).
        glob_pattern = str(build_path) + '/AzTestRunner_iphoneos*-arm64.xctestrun'
        test_run_files = glob.glob(glob_pattern)

        if not test_run_files:
            raise common.LmbrCmdError(f"No xctestrun file found in '{str(build_path)}'. Run build_ios_test.py first.")
    
        test_run_file = test_run_files[0]
    
    test_targets = common.get_validated_test_modules(test_modules=test_target, build_dir_path=build_path)

    test_run_contents = []
    with open(test_run_file, 'rb') as fp:
        test_run_contents = plistlib.load(fp)
        
    xcode_build = common.CommandLineExec('/usr/bin/xcodebuild')
    
    for target in test_targets:
        with open(test_run_file, 'wb') as fp:
            fp.truncate(0)
        
        command_line_arguments = [target, 'AzRunUnitTests']
        if test_filter:
            command_line_arguments.extend(['gtest_filter', test_filter])
        test_run_contents[TEST_TARGET_NAME]['CommandLineArguments'] = command_line_arguments
        
        with open(test_run_file, 'wb') as fp:
            plistlib.dump(test_run_contents, fp, sort_keys=False)
        
        xcode_args = ['test-without-building', '-xctestrun', test_run_file, '-destination', f'platform=iOS,name={target_dev_name}']
        if timeout_secs < 0:
            xcode_args.extend(['-test-timeouts-enabled', 'NO'])
        else:
            xcode_args.extend(['-test-timeouts-enabled', 'YES'])
            xcode_args.extend(['-maximum-test-execution-time-allowance', f'{timeout_secs}'])

        xcode_out = xcode_build.popen(xcode_args, cwd=build_path, shell=False)
        
        # Log XCTest's output to debug.
        # Use test start and end markers to separate XCTest output from AzTestRunner's output
        test_success = False
        test_output = False
        while xcode_out.poll() is None:
            line = xcode_out.stdout.readline()
            if TEST_STARTED_STRING in line:
                test_output = True
            
            if TEST_SUCCESS_STRING in line:
                test_success = True
                test_output = False
            elif TEST_FAILURE_STRING in line:
                test_output = False
                
            if test_output:
                print(line)
            else:
                logging.debug(line)
            
        print(f'{target} Succeeded') if test_success else print(f'{target} Failed')


def main(args):

    parser = argparse.ArgumentParser(description="Launch a test module on a target iOS device.")

    parser.add_argument('-b', '--build-dir',
                        help='The relative build directory to deploy from.',
                        required=True)

    parser.add_argument('test_module',
                        nargs='*',
                        help='The test module(s) to launch on the target device. Defaults to all registered test modules',
                        default=[])

    parser.add_argument('--device-name',
                        help='The name of the iOS device on which to launch.',
                        required=True)

    parser.add_argument('--timeout',
                        help='The timeout in secs for each test module to prevent deadlocked tests',
                        type=int,
                        default=-1)

    parser.add_argument('--test-filter',
                        help='Optional gtest filter to pass along to the unit test launcher',
                        default=None)

    parser.add_argument('--xctestrun-file',
                        help='Optional parameter to specify custom xctestrun file (path relative to build directory)',
                        default=None)

    parser.add_argument('--debug',
                        help='Enable debug logging',
                        action='store_true')

    parsed_args = parser.parse_args(args)

    logging.basicConfig(format='%(levelname)s: %(message)s',
                        level=logging.DEBUG if parsed_args.debug else logging.INFO)

    result = launch_ios_test(build_dir=parsed_args.build_dir,
                                 target_dev_name=parsed_args.device_name,
                                 test_target=parsed_args.test_module,
                                 timeout_secs=int(parsed_args.timeout),
                                 test_filter=parsed_args.test_filter,
                                 xctestrun_file=parsed_args.xctestrun_file)
    return 0 if result else 1


if __name__ == '__main__':

    try:
        result_code = main(sys.argv[1:])
        exit(result_code)

    except common.LmbrCmdError as err:
        logging.error(str(err))
        exit(err.code)
