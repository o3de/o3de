"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import argparse
import itertools
import logging
import os
from run_launcher_tests import PlatformDriver
import subprocess
import time

logger = logging.getLogger(__name__)


class AndroidDriver(PlatformDriver):

    def read_package_name(self):
        project_data = self.read_json_data(self.project_json_path)
        return project_data['android_settings']['package_name']

    def run_test(self, test_name, map, dynamic_slice, timeout, pass_string, fail_string):

        package = self.read_package_name()
        project = self.read_project_name()
        package_and_activity = "{}/{}.{}Activity".format(package, package, project)

        # clear any old logcat
        command_line = ['adb', 'logcat', '-c']
        p = subprocess.Popen(command_line)
        p.communicate()

        # increase ring buffer size because we are going to be printing a large amount of data in a short time.
        command_line = ['adb', 'logcat', '-G', '10m']
        p = subprocess.Popen(command_line)
        p.communicate()
        
        # Start the process and pass in the test args
        logger.info("Start the activity {} ...".format(package_and_activity))
        command_line = ['adb', 'shell', 'am', 'start', '-a', 'android.intent.action.MAIN', '-n', package_and_activity]
        command_line += ['-e', 'ltest_map', map]
        command_line += ['-e', 'ltest_slice', dynamic_slice]
        p = subprocess.Popen(command_line)
        p.communicate()

        # Get the pid of the app to use in the logcat monitoring. If we don't
        # do this we might get residual output from previous test run. Even
        # though we do a clear.
        for _ in itertools.repeat(None, 10):
            command_line = ['adb', 'shell', 'pidof', package]
            p = subprocess.Popen(command_line, stdout=subprocess.PIPE)
            stdoutdata, stderrdata = p.communicate()
            pid = stdoutdata.strip()
            if pid:
                logger.info("Get pid of {}".format(pid))
                break
            else:
                logger.info('Failed to get pid, waiting 1 second to retry ...')
                time.sleep(1)
        if not pid:
            raise Exception('Unable to determin the pid of the process.')
        
        command_line = ['adb', '-d', 'logcat', "--pid={}".format(pid), 'LMBR:I', '*:S']
        test_result = self.monitor_process_output(test_name, command_line, pass_string, fail_string, timeout)

        logger.info("Kill the app ...")
        p = subprocess.Popen(['adb', 'shell', 'am', 'force-stop', package])
        p.communicate()

        # Stop here if we failed.
        if not test_result:
            raise Exception("Test failed.")
            
    def take_screenshot(self, output_path_no_ext):
        # Create the output folder if it is not there
        output_folder = os.path.dirname(output_path_no_ext)
        if not os.path.exists(output_folder):
            os.makedirs(output_folder)
            
        # Take the screenshot
        p = subprocess.Popen(['adb', 'shell', 'screencap', '-p', '/sdcard/screen.png'])
        p.communicate()
        
        # copy it off of the device
        p = subprocess.Popen(['adb', 'pull', '/sdcard/screen.png', "{}.png".format(output_path_no_ext)])
        p.communicate()            
            
def main():

    parser = argparse.ArgumentParser(description='Sets up and runs Android Launcher Tests.')
    parser.add_argument('--project-json-path', required=True, help='Path to the project.json project settings file.')
    parser.add_argument('--project-launcher-tests-folder', required=True, help='Path to the LauncherTests folder in a Project.')
    parser.add_argument('--test-names', nargs='+', help='A list of test names to run, default runs all tests.')
    parser.add_argument('--screenshots-folder', default="./temp/Android/screenshots", help='Output folder for screenshots.')
    parser.add_argument('--screenshots-interval', default=5, help='Time interval between taking screenshots in seconds.')
    
    args = parser.parse_args()

    logging.basicConfig(level=logging.DEBUG)
    
    logger.info("Running Launcher tests at {} ...".format(args.project_launcher_tests_folder))
    driver = AndroidDriver(args.project_json_path, args.project_launcher_tests_folder, args.test_names, args.screenshots_folder, args.screenshots_interval)
    driver.run_launcher_tests()

if __name__== "__main__":
    main()
