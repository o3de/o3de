"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import os
import subprocess

import pytest

import ly_test_tools.environment.process_utils as process_utils
import ly_test_tools.environment.waiter as waiter
from ly_test_tools import WINDOWS

if WINDOWS:
    pytestmark = pytest.mark.SUITE_smoke
else:
    pytestmark = pytest.mark.skipif(not WINDOWS, reason="Only runs on Windows")


class TestSubprocessCheckOutputWrapper(object):

    def test_KillWindowsProgram_WindowsProgramStarted_KilledSuccessfully(self):
        windows_program = 'timeout.exe'
        windows_directory = os.environ.get('windir')
        command = [
            os.path.join(f'{windows_directory}', 'System32', f'{windows_program}'),
            '/T',  # Timeout flag
            '4',  # 4 seconds
        ]

        def process_killed():
            return not process_utils.process_exists(windows_program, ignore_extensions=True)

        assert os.path.exists(command[0]), (
            f'The {windows_program} executable does not exist at: {command[0]}'
        )

        with subprocess.Popen(command, creationflags=subprocess.CREATE_NEW_CONSOLE) as process:
            if process_utils.process_exists(windows_program, ignore_extensions=True):
                process_utils.kill_process_with_pid(process.pid)
                waiter.wait_for(process_killed, timeout=2)  # Raises exception if the process is alive.
