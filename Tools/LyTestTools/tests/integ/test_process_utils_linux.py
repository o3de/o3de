"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import subprocess
import pytest

import ly_test_tools.environment.process_utils as process_utils
import ly_test_tools.environment.waiter as waiter
from ly_test_tools import LINUX

if LINUX:
    pytestmark = pytest.mark.SUITE_smoke
else:
    pytestmark = pytest.mark.skipif(not LINUX, reason="Only runs on Linux")


class TestProcessUtils(object):

    def test_KillLinuxProcess_ProcessStarted_KilledSuccessfully(self):
        # Construct a simple timeout command
        linux_executable = 'timeout'
        command = [linux_executable, '5s', 'echo']

        # Verification function for the waiter to call
        def process_killed():
            return not process_utils.process_exists(linux_executable, ignore_extensions=True)

        # Create a new process with no output in a new session
        with subprocess.Popen(command, stdout=subprocess.DEVNULL, start_new_session=True):
            # Ensure that the process was started
            assert process_utils.process_exists(linux_executable, ignore_extensions=True), \
                f"Process '{linux_executable}' was expected to exist, but could not be found."
            # Kill the process using the process_utils module
            process_utils.kill_processes_named(linux_executable, ignore_extensions=True)
            # Verify that the process was killed
            waiter.wait_for(process_killed, timeout=2)  # Raises exception if the process is alive.
