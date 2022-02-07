"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT


CLI tool - AssetBuilder
Launch AssetBuilder and Verify the help message
"""

import os
import pytest
import subprocess


@pytest.mark.SUITE_sandbox
class TestCLIToolAssetBuilderWorks(object):
    def test_CLITool_AssetBuilder_Works(self, build_directory):
        file_path = os.path.join(build_directory, "AssetBuilder")
        help_message = "AssetBuilder is part of the Asset Processor"
        # Launch AssetBuilder
        output = subprocess.run([file_path, "-help"], capture_output=True, timeout=10)
        assert (
            len(output.stderr) == 0 and output.returncode == 0
        ), f"Error occurred while launching {file_path}: {output.stderr}"
        # Verify help message
        assert help_message in str(output.stdout), f"Help Message: {help_message} is not present"
