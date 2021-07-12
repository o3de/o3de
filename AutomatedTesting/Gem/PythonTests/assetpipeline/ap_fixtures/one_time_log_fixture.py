"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Fixture for creating a single-use log to read instead of parsing through the bundled logs
"""

import pytest
import os
import subprocess
import time

import ly_test_tools.environment.file_system as fs


class Single_Use_Log:
    """
    Class used during test, can use create_log_file multiple times during test but will completely overwrite
    """

    def __init__(self, workspace):
        seconds = str(time.time())[-6:]
        self.file_name = "Single_Use_Log" + seconds + ".txt"
        self.log_path = os.path.join(workspace.paths.bin(), "logs", self.file_name)

    def create_log_file(self, commands):
        with open(self.log_path, "w") as log_file:
            subprocess.check_call(commands, stdout=log_file)

    def cleanup(self):
        fs.delete([self.log_path], True, False)


@pytest.fixture
def one_time_log_fixture(request, workspace) -> Single_Use_Log:
    """
    Pytest Fixture for setting up a single use log file
    At test conclusion, runs the cleanup to delete the single use text file
    :return: Single_Use_Log class
    """
    log_class = Single_Use_Log(workspace)

    request.addfinalizer(log_class.cleanup)

    return log_class
