"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Fixture that allows Idle check of the AP GUI
"""

# Import builtin libraries
import pytest
import os
import time

# Import fixtures
from . import ap_setup_fixture

# Import LyTestTools
import ly_test_tools.environment.waiter as waiter
from ly_test_tools.o3de.ap_log_parser import APLogParser


@pytest.mark.usefixtures("test_assets")
class TimestampChecker(object):
    def __init__(self, file_path, timeout) -> None:
        self.file = file_path
        self.timeout = timeout
        self.original_mod_time = int(round(time.time() * 1000))
        self.start_time = time.time()
        self.log = None

    def set_file_path(self, file_path):
        self.file = file_path

    def grab_current_timestamp(self) -> None:
        self.original_mod_time = int(round(time.time() * 1000))

    def check_if_idle(self, timeout=None, starttime=None, updatetime_max=30) -> None:
        if timeout is None:
            timeout = self.timeout
        if starttime:
            time.sleep(starttime)

        def log_reports_idle() -> bool:
            """
            Grabs the current log run and reads it line by line from the bottom up
            Returns whether the idle message appears later in the log than the latest "Processing [...]" message
            """
            if updatetime_max and os.path.exists(self.file):
                last_file_update = os.path.getmtime(self.file)
                timedelta = time.time() - last_file_update
                if timedelta > updatetime_max:  # Has the file exceeded the limit
                    # Additionally we want to make sure the test has exceeded the limit so we don't catch
                    # a log from a previous run where our current test hasn't engaged any action from AP
                    if time.time() - self.start_time > updatetime_max:
                        return True
            self.log = APLogParser(self.file)
            line_index = len(self.log.runs[-1]["Lines"]) - 1
            while line_index > 0:
                line = self.log.runs[-1]["Lines"][line_index]
                timestamp = self.log.runs[-1]["Timestamps"][line_index]
                if self.log.get_line_type(line) == "AssetProcessor":
                    message = self.log.remove_line_type(line)
                    if timestamp <= self.original_mod_time:
                        return False
                    elif message.startswith("Processing"):
                        return False
                    elif "Job processing completed. Asset Processor is currently idle." in message:
                        self.original_mod_time = timestamp
                        return True
                line_index -= 1

        waiter.wait_for(lambda: (log_reports_idle()), timeout=timeout or self.timeout)


@pytest.fixture
def ap_idle_fixture(request, workspace, timeout: int) -> TimestampChecker:
    """
    Allows checking the GUI for idle by grabbing the initial modified time of the log file
    and looking for new "idle" messages later
    """
    return TimestampChecker(workspace.paths.ap_gui_log(), timeout, workspace)
