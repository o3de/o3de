#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import argparse
import logging
import os
import pathlib
import queue
import re
import sys
import threading
import time

# Resolve the common python module
ROOT_DEV_PATH = os.path.realpath(os.path.join(os.path.dirname(__file__), '..', '..', '..', '..'))
if ROOT_DEV_PATH not in sys.path:
    sys.path.append(ROOT_DEV_PATH)

from cmake.Tools import common
from cmake.Tools.Platform.Android import android_support


# The name of the unit test target
TEST_PROJECT = 'AzTestRunner'

TEST_ACTIVITY = 'AzTestRunnerActivity'

# Prepare a regex that will strip out the timestamp and PID information from adb's logcat to just show 'LMBR' tagged logs
REGEX_LOGCAT_LINE = re.compile(r'([\d-]+)\s+([\d:\.]+)\s+(\d+)\s+(\d+)\s+(I)\s+(LMBR)([\s]+)(:\s)(.*)')

# The startup delay will allow the test runner to pause so that the we have a chance to query for the PID of the test launcher
TEST_RUNNER_STARTUP_DELAY = 1

LOGCAT_BUFFER_SIZE_MB = 32

LOGCAT_READ_QUEUE = queue.Queue()


def validate_android_test_build_dir(build_dir, configuration):
    """
    Validate an android test build folder

    :param build_dir:       The build directory where the android test project was generated.
    :param configuration:   The configuration of the test build
    :return: tuple of (Path of build_dir, path of the build dir native path (where the native binaries are built for the configuration), and the android AdbTool wrapper
    """
    build_path = pathlib.Path(build_dir) if os.path.isabs(build_dir) else pathlib.Path(ROOT_DEV_PATH) / build_dir
    if not build_path.is_dir():
        raise common.LmbrCmdError(f"Invalid android build directory '{str(build_path)}'")

    # Get the platform settings to validate the test game name (must match TEST_PROJECT)
    platform_settings = common.PlatformSettings(build_path)
    if not platform_settings.projects:
        raise common.LmbrCmdError("Missing required platform settings object from build directory.")
    is_unit_test_str = getattr(platform_settings, 'is_unit_test', 'False')
    is_unit_test = is_unit_test_str.lower() in ('t', 'true', '1')
    if not is_unit_test:
        raise common.LmbrCmdError("Invalid android build folder for tests.")

    # Construct and validate the path to the native binaries that are built for the APK based on the input confiugration
    build_configuration_path = build_path / 'app' / 'cmake' / configuration / 'arm64-v8a'

    if not build_configuration_path.is_dir():
        raise common.LmbrCmdError(f"Invalid android build configuration '{configuration}': Make sure that the APK has been built with this configuration successfully")

    # Validate the android SDK path that was registered in the platform settings
    android_sdk_path = getattr(platform_settings, 'android_sdk_path', None)
    if not android_sdk_path:
        raise common.LmbrCmdError(f"Android SDK Path {android_sdk_path} is missing in the platform settings for {build_dir}.")
    if not os.path.isdir(android_sdk_path):
        raise common.LmbrCmdError(f"Android SDK Path {android_sdk_path} in the platform settings for {build_dir} is not valid.")

    return build_path, build_configuration_path, android_sdk_path


def launch_test_on_device(adb_tool, test_module, timeout_secs, test_filter):
    """
    Launch an test module on the connect android device

    :param adb_tool:        The ADB Tool to exec the adb commands necessary to run a test
    :param test_module:     The name of the test module to run
    :param timeout_secs:    Timeout for a test run
    :return: True if the tests passed, False if not
    """

    # Clear user data before each test
    adb_tool.exec(['shell', 'pm', 'clear', 'com.lumberyard.tests'])

    # Increase the log buffer to prevent 'end of file' error from logcat
    adb_tool.exec(['shell', 'logcat', '-G', f'{LOGCAT_BUFFER_SIZE_MB}M'])

    # Start the test activity
    exec_args = ['shell', 'am', 'start',
                 '-n', f'com.lumberyard.tests/.{TEST_ACTIVITY}',
                 '--es', test_module, 'AzRunUnitTests',
                 '--es', 'startdelay', str(TEST_RUNNER_STARTUP_DELAY)]
    if test_filter:
        exec_args.extend([
            '--es', 'gtest_filter', test_filter
        ])

    ret, result_output, result_error = adb_tool.exec(exec_args, capture_stdout=True)

    if ret != 0:
        raise common.LmbrCmdError(f"Unable to launch test runner activity: {result_error or result_output}")

    tests_passed = False
    result_pid = None

    try:

        # Make multiple attempts to get the PID of the test process
        max_pid_retries = 5
        while max_pid_retries > 0:
            ret, result_pid, _ = adb_tool.exec(['shell', 'pidof', '-s', 'com.lumberyard.tests'], capture_stdout=True)
            if ret == 0:
                break
            time.sleep(1)
            max_pid_retries -= 1
        if not result_pid:
            raise common.LmbrCmdError("Unable to get process id for the Test Runner launcher")
        result_pid = result_pid.strip()

        # Start the adb logcat process for the result PID and filter the stdout
        logcat_proc = adb_tool.popen(['shell', 'logcat', f'--pid={result_pid}', '-s', 'LMBR'])
        start_time = time.time()

        while logcat_proc.poll() is None:
            # Break out of the loop if we triggered the test timeout condition (timeout_secs)
            elapsed_time = time.time() - start_time

            if elapsed_time > timeout_secs > 0:
                logging.error("Test Runner Timeout")
                break

            # Break out of the loop in case the process dies unexpectly
            ret, _, _ = adb_tool.exec(['shell', 'pidof', '-s', 'com.lumberyard.tests'], capture_stdout=True)
            if ret != 0:
                break

            # Use a regex to strip out timestamp/PID logcat line and filter only the 'LMBR' tagged log events
            line = logcat_proc.stdout.readline()

            result = REGEX_LOGCAT_LINE.match(line) if line else None

            if result:
                lmbr_log_line = result.group(9)
                print(lmbr_log_line)
                if '[FAILURE]' in lmbr_log_line:
                    break
                if '[SUCCESS]' in lmbr_log_line:
                    tests_passed = True
                    break

    finally:
        if result_pid:
            adb_tool.exec(['shell', 'logcat', f'--pid={result_pid}', '-c'])
        # Kill the test launcher process
        adb_tool.exec(['shell', 'am', 'force-stop', 'com.lumberyard.tests'])
        time.sleep(2)

    return tests_passed


def launch_android_test(build_dir, configuration, target_dev_serial, test_target, timeout_secs, test_filter):
    """
    Launch the unit test android apk with specific test target(s)

    :param build_dir:           The cmake build directory to base the launch values on
    :param configuration:       The configuration to base the launch values on
    :param target_dev_serial:   The target device serial number to launch the test on. If none, launch on all connected devices
    :param test_target:         The name of the target module to invoke the test on. If 'all' is specified, then iterate through all of the test modules and launch them individually
    :param timeout_secs:        Timeout value for individual test runs

    :return: True if the test run(s) were successful, false if not
    """

    # Validate the build dir and configuration
    build_path, build_configuration_path, android_sdk_path = validate_android_test_build_dir(build_dir=build_dir,
                                                                                             configuration=configuration)

    test_targets = common.get_validated_test_modules(test_modules=test_target, build_dir_path=build_configuration_path)

    # Track the long text length for formatting/alignment for the final report
    max_module_text_len = max([len(module) for module in test_targets])
    module_column_width = max_module_text_len + 8

    adb_tool = android_support.AdbTool(android_sdk_path)
    adb_tool.connect(target_dev_serial)

    start_time = time.time()
    final_report_map = {}
    test_run_complete_event = threading.Event()
    successful_run = True
    for test_target in test_targets:

        logging.info(f"Launching test for module {test_target}")

        result = launch_test_on_device(adb_tool, test_target, timeout_secs, test_filter)
        if result:
            logging.info(f"Tests for module {test_target} Passed")
            final_report_map[test_target] = 'PASSED'
        else:
            logging.info(f"Tests for module {test_target} Failed")
            final_report_map[test_target] = 'FAILED'
            successful_run = False

        time.sleep(1)

    end_time = time.time()
    elapsed_time = end_time - start_time

    hours = elapsed_time // 3600
    elapsed_time = elapsed_time - 3600 * hours
    minutes = elapsed_time // 60
    seconds = elapsed_time - 60 * minutes

    logging.info(f"Total Time  : {int(hours)}h {int(minutes)}m {int(seconds)}s")
    logging.info(f"Test Modules: {len(test_targets)}")
    logging.info('----------------------------------------------------')
    for test_module, test_result in final_report_map.items():
        module_text_len = len(test_module)
        logging.info(f"{test_module}{' '* (module_column_width-module_text_len)} : {test_result}")

    test_run_complete_event.set()

    adb_tool.disconnect()

    return successful_run


def main(args):

    parser = argparse.ArgumentParser(description="Launch a test module on a target dev kit.")

    parser.add_argument('-b', '--build-dir',
                        help=f'The relative build directory to deploy from.',
                        required=True)

    parser.add_argument('-c', '--configuration',
                        help='The build configuration from the build directory for the source deployment files',
                        default='profile')

    parser.add_argument('test_module',
                        nargs='*',
                        help="The test module(s) to launch on the target device. Defaults to all registered test modules",
                        default=[])

    parser.add_argument('--device-serial',
                        help='The optional device serial to target the launch on. Defaults to the all devices connected.',
                        default=None)

    parser.add_argument('--timeout',
                        help='The timeout in secs for each test module to prevent deadlocked tests',
                        type=int,
                        default=-1)

    parser.add_argument('--test-filter',
                        help="Option gtest filter to pass along to the unit test launcher",
                        default=None)

    parser.add_argument('--debug',
                        help='Enable debug logging',
                        action='store_true')

    parsed_args = parser.parse_args(args)

    logging.basicConfig(format='%(levelname)s: %(message)s',
                        level=logging.DEBUG if parsed_args.debug else logging.INFO)

    result = launch_android_test(build_dir=parsed_args.build_dir,
                                 configuration=parsed_args.configuration,
                                 target_dev_serial=parsed_args.device_serial,
                                 test_target=parsed_args.test_module,
                                 timeout_secs=int(parsed_args.timeout),
                                 test_filter=parsed_args.test_filter)
    return 0 if result else 1


if __name__ == '__main__':

    try:
        result_code = main(sys.argv[1:])
        exit(result_code)

    except common.LmbrCmdError as err:
        logging.error(str(err))
        exit(err.code)
