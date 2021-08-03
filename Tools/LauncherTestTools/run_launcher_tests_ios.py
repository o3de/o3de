"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import argparse
import logging
import os
from run_launcher_tests import PlatformDriver
import subprocess

logger = logging.getLogger(__name__)


class IOSDriver(PlatformDriver):

    def __init__(self, project_json_path, project_launcher_tests_folder, test_names, screenshots_folder, screenshots_interval, device_udid):
        self.project_json_path = project_json_path
        self.project_launcher_tests_folder = project_launcher_tests_folder
        self.test_names = test_names
        self.screenshots_folder = screenshots_folder
        self.screenshots_interval = screenshots_interval
        self.device_udid = device_udid

    def run_test(self, test_name, map, dynamic_slice, timeout, pass_string, fail_string):

        project = self.read_project_name()
        bundle_id = "com.amazon.lumberyard.{}".format(project)
            
        # Start the process and pass in the test args        
        command_line = ['idevicedebug', '-u', self.device_udid, 'run', bundle_id]
        command_line += ['-ltest_map', map]
        command_line += ['-ltest_slice', dynamic_slice]        
        test_result = self.monitor_process_output(test_name, command_line, pass_string, fail_string, timeout)

        # TODO: Figure out some way to kill the running app. Because we dont know how to do this,
        # we currently have a limitation on iOS we can only run one test at a time.

        # Stop here if we failed.
        if not test_result:
            raise Exception("Test failed.")
    
    def take_screenshot(self, output_path_no_ext):
        # Create the output folder if it is not there
        output_folder = os.path.dirname(output_path_no_ext)
        if not os.path.exists(output_folder):
            os.makedirs(output_folder)
            
        # idevicescreenshot to take a screenshot and save to output path.
        p = subprocess.Popen(['idevicescreenshot', "{}.tiff".format(output_path_no_ext)])
        p.communicate()            
        
def discover_device():
    """ Discover the connected device and get the UDID."""
    logger.info("Getting the connected device UDID ...")
    p = subprocess.Popen(['idevice_id', '--list'], stdout=subprocess.PIPE)
    out, err = p.communicate()
    if not out:
        raise Exception("No output.\nout:{}\nerr:{}\n".format(out, err))
            
    lines = out.splitlines()
    if not len(lines):
        raise Exception("No devices connected.\nout:{}\nerr:{}\n".format(out, err))
    if len(lines) != 1:
        raise Exception("More than one device connected. Use --device-udid\nout:{}\nerr:{}\n".format(out, err))    
    return lines[0]

def main():

    parser = argparse.ArgumentParser(description='Sets up and runs iOS Launcher Tests.')
    parser.add_argument('--project-json-path', required=True, help='Path to the project.json project settings file.')
    parser.add_argument('--project-launcher-tests-folder', required=True, help='Path to the LauncherTests folder in a Project.')
    parser.add_argument('--device-udid', help='The UDID of the iOS device. Will auto detect if just one device is attached.')
    parser.add_argument('--test-names', nargs='+', help='A list of test names to run, default runs all tests.')
    parser.add_argument('--screenshots-folder', default="./temp/iOS/screenshots", help='Output folder for screenshots.')
    parser.add_argument('--screenshots-interval', default=5, help='Time interval between taking screenshots in seconds.')
    args = parser.parse_args()

    logging.basicConfig(level=logging.DEBUG)
    
    device_udid = args.device_udid
    if not device_udid:
        device_udid = discover_device()
    
    logger.info("Running Launcher tests at {} ...".format(args.project_launcher_tests_folder))
    driver = IOSDriver(args.project_json_path, args.project_launcher_tests_folder, args.test_names, args.screenshots_folder, args.screenshots_interval, device_udid)
    driver.run_launcher_tests()
    
if __name__== "__main__":
    main()
