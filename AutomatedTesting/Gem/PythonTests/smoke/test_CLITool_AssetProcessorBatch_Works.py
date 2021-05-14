"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.


CLI tool - AssetProcessorBatch
Launch AssetProcessorBatch and Shutdown AssetProcessorBatch without any crash
"""


# Import builtin libraries
import pytest
from ly_test_tools.o3de.asset_processor import AssetProcessor


@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.timeout(10)
@pytest.mark.SUITE_smoke
class TestsCLIToolAssetProcessorBatchWorks(object):
    def test_CLITool_AssetProcessorBatch_Works(self, workspace):
        """
        Test Launching AssetProcessorBatch and verifies that is shuts down without issue
        """
        asset_processor = AssetProcessor(workspace)
        asset_processor.batch_process()
