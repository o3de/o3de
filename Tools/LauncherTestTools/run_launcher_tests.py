"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import json
import logging
import os
import time
import shutil
import subprocess
import sys

logger = logging.getLogger(__name__)


class PlatformDriver:

    def __init__(self, project_json_path, project_launcher_tests_folder, test_names, screenshots_folder, screenshots_interval):
        self.project_json_path = project_json_path
        self.project_launcher_tests_folder = project_launcher_tests_folder
        self.test_names = test_names
        self.screenshots_folder = screenshots_folder
        self.screenshots_interval = screenshots_interval

    def read_json_data(self, path):
        """Read a json file and return the data"""
        try:
            with open(path) as json_file:
                json_data = json.load(json_file)
        except Exception, e:
            logger.error("Failed to read json file: '{}'".format(path))
            logger.error("Exception: '{}'".format(e))
            sys.exit(1)
        return json_data

    def read_project_name(self):
        project_data = self.read_json_data(self.project_json_path)
        return project_data['project_name']


    def run_test(self, map, dynamic_slice, timeout, pass_string, fail_string):
        """ Meant to be overridden in derived classes. """
        return True

    def run_launcher_tests(self):
        """Discovers all of the available tests in launcher_tests.json and runs them.
        """
        # Delete the old screenshots folder if it exists
        if os.path.exists(self.screenshots_folder):
            shutil.rmtree(self.screenshots_folder)
        
        # Read the launcher_tests.json file.
        launcher_tests_data = self.read_json_data(os.path.join(self.project_launcher_tests_folder, 'launcher_tests.json'))

        # Run each of the tests found in the launcher_tests.json file.
        ok = True
        for launcher_test_data in launcher_tests_data['launcher_tests']:
        
            # Skip over this test if specific tests are specified and this is not one of them.
            if self.test_names and launcher_test_data.get('name').lower() not in [x.lower() for x in self.test_names]:
                continue
        
            ok = ok and self.run_test(
                launcher_test_data.get('name'),
                launcher_test_data.get('map'),
                launcher_test_data.get('dynamic_slice'),
                launcher_test_data.get('timeout'),
                launcher_test_data.get('pass_string', 'AUTO_LAUNCHER_TEST_COMPLETE'),
                launcher_test_data.get('fail_string', 'AUTO_LAUNCHER_TEST_FAIL'))
        return ok

    def monitor_process_output(self, test_name, command, pass_string, fail_string, timeout, log_file=None):

        self.process = subprocess.Popen(command, stdout=subprocess.PIPE)

        # On windows, function was failing (I think) because it checked the poll before the process
        # had a chance to start, so added a short delay to give it some time to startup.
        # It also failed if the log_file was open()'d when it didn't exist yet.
        # Delay seems to have fixed the problem. 0.25 sec was too short.
        time.sleep(0.5)

        if log_file:
            # The process we're starting sends it's output to the log file instead of stdout
            # so we need to monitor that instead of the stdout.
            fp = open(log_file)

        # Detect log output messages or timeout exceeded
        start = time.time()
        last_time = start
        screenshot_time_remaining = self.screenshots_interval
        screenshot_index = 0
        logger.info('Waiting for test to complete.')
        message = ""
        result = False
        while True:
            if log_file:
                line = fp.readline()
            else:
                line = self.process.stdout.readline()
            if line == '' and self.process.poll() is not None:
                break
            if line:
                logger.info(line.rstrip())
                if pass_string in line:
                    message = "Detected {}. Test completed.".format(pass_string)
                    result = True
                    break
                if fail_string in line:
                    message = "Detected {}. Test failed.".format(fail_string)
                    break
                if time.time() - start > timeout:
                    message = "Timeout of {} reached. Test failed.".format(timeout)
                    break

            rc = self.process.poll()
            cur_time = time.time()
            screenshot_time_remaining = screenshot_time_remaining - (cur_time - last_time)
            last_time = cur_time
            if screenshot_time_remaining <= 0:
                self.take_screenshot(os.path.join(self.screenshots_folder, "{}_screen{}".format(test_name.replace(' ', '_'), screenshot_index)))
                screenshot_index = screenshot_index + 1
                screenshot_time_remaining = self.screenshots_interval

        logger.info(message)

        if log_file:
            fp.close()

        return result
