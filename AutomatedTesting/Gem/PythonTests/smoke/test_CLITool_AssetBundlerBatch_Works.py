"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.


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
