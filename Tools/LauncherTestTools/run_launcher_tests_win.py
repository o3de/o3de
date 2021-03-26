"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

import argparse
import logging
import os
from run_launcher_tests import PlatformDriver

import test_tools.platforms.win.win as win
from test_tools.shared.platform_map import PLATFORM_MAP

logger = logging.getLogger(__name__)

class WinDriver(PlatformDriver):

    def __init__(self, project_json_path, project_launcher_tests_folder):
        self.project_json_path = project_json_path
        self.project_launcher_tests_folder = project_launcher_tests_folder

        self.platform_map = PLATFORM_MAP[win.platform_name() + "_" + win.default_compiler_option()]


    def run_test(self, map, dynamic_slice, timeout, pass_string, fail_string):

        project = self.read_project_name()
            
        # Start the process and pass in the test args

        dev_dir = os.path.dirname(os.path.realpath(__file__))
        dev_dir = os.path.join(dev_dir, os.path.pardir)
        dev_dir = os.path.join(dev_dir, os.path.pardir)
        dev_dir = os.path.realpath(dev_dir)
        launcher_dir = os.path.join(dev_dir, self.platform_map["bin_dir"])
        command_line = [os.path.join(launcher_dir, project + 'Launcher.exe')]
        command_line += ['-ltest_map', map]
        command_line += ['-ltest_slice', dynamic_slice]
        log_file = os.path.join(dev_dir, "cache", project, "pc", "user", "log", "Game.log")
        test_result = self.monitor_process_output(command_line, pass_string, fail_string, timeout, log_file)

        if self.process:
            self.process.kill()

        # Stop here if we failed.
        if not test_result:
            raise Exception("Test failed.")

        return test_result

def main():

    parser = argparse.ArgumentParser(description='Sets up and runs Windows Launcher Tests.')
    parser.add_argument('--project-json-path', required=True, help='Path to the project.json project settings file.')
    parser.add_argument('--project-launcher-tests-folder', required=True, help='Path to the LauncherTests folder in a Project.')
    args = parser.parse_args()

    logging.basicConfig(level=logging.DEBUG)
    
    logger.info("Running Launcher tests at {} ...".format(args.project_launcher_tests_folder))
    driver = WinDriver(args.project_json_path, args.project_launcher_tests_folder)
    driver.run_launcher_tests()
    
if __name__== "__main__":
    main()
