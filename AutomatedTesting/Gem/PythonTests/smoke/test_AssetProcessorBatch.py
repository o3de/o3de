"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

"""
LY-124061 : CLI tool - AssetProcessorBatch
Launch AssetProcessorBatch and Shutdown AssetProcessorBatch without any crash
"""


# Import builtin libraries
import pytest
import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)) + "/../assetpipeline/")

# Import fixtures
from ap_fixtures.asset_processor_fixture import asset_processor as asset_processor


@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.usefixtures("automatic_process_killer")
@pytest.mark.usefixtures("asset_processor")
@pytest.mark.SUITE_smoke
class TestsAssetProcessorBatchs(object):
    @pytest.mark.test_case_id("LY-124061")
    def test_AssetProcessorBatch(self, asset_processor):
        """
        Test Launching AssetProcessorBatch and verifies that is shuts down without issue
        """
        # Create a sample asset root so we don't process every asset for every platform
        asset_processor.create_temp_asset_root()
        # Launch AssetProcessorBatch, assert batch processing success
        result, _ = asset_processor.batch_process()
        assert result, "AP Batch failed"
