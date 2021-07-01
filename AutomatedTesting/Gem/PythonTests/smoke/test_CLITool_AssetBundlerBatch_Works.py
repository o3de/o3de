"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT


CLI tool - AssetBundlerBatch
Launch AssetBundlerBatch and Verify the help message
"""

import os
import pytest
import subprocess


@pytest.mark.SUITE_smoke
class TestCLIToolAssetBundlerBatchWorks(object):
    def test_CLITool_AssetBundlerBatch_Works(self, build_directory):
        file_path = os.path.join(build_directory, "AssetBundlerBatch")
        help_message = "Specifies the Seed List file to operate on by path"
        # Launch AssetBundlerBatch
        output = subprocess.run([file_path, "--help"], capture_output=True, timeout=10)
        assert (
            len(output.stderr) == 0 and output.returncode == 0
        ), f"Error occurred while launching {file_path}: {output.stderr}"
        # Verify help message
        assert help_message in str(output.stdout), f"Help Message: {help_message} is not present"
