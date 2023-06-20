"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT


CLI tool - PythonBindingsExample
Launch PythonBindingsExample and Verify the help message
"""

import os
import pytest
import subprocess


@pytest.mark.skip(reason="GHI #13693: Test Periodically Fails")
class TestCLIToolPythonBindingsExampleWorks(object):
    def test_CLITool_PythonBindingsExample_Works(self, build_directory):
        file_path = os.path.join(build_directory, "PythonBindingsExample")
        help_message = "--help Prints the help text"
        # Launch PythonBindingsExample
        output = subprocess.run([file_path, "-help"], capture_output=True, timeout=10)
        assert (
            len(output.stderr) == 0 and output.returncode == 1
        ), f"Error occurred while launching {file_path}: {output.stderr}"
        # Verify help message
        assert help_message in str(output.stdout), f"Help Message: {help_message} is not present"
