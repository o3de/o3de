"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT


CLI tool - SerializeContextTools
Launch SerializeContextTools and Verify the help message
"""

import os
import pytest
import subprocess

import ly_test_tools


@pytest.mark.skipif(not ly_test_tools.WINDOWS, reason="Only succeeds on windows https://github.com/o3de/o3de/issues/5539")
@pytest.mark.SUITE_smoke
class TestCLIToolSerializeContextToolsWorks(object):
    def test_CLITool_SerializeContextTools_Works(self, build_directory):
        file_path = os.path.join(build_directory, "SerializeContextTools")
        help_message = "Converts a file with an ObjectStream to the new JSON"
        # Launch SerializeContextTools
        output = subprocess.run([file_path, "-help"], capture_output=True, timeout=10)
        assert (
            len(output.stderr) == 0 and output.returncode == 0
        ), f"Error occurred while launching {file_path}: {output.stderr}"
        # Verify help message
        assert help_message in str(output.stdout), f"Help Message: {help_message} is not present"
