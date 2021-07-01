"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT


CLI tool - AssetProcessorBatch
Launch AssetProcessorBatch and Shutdown AssetProcessorBatch without any crash
"""

import pytest


@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.SUITE_smoke
class TestsCLIToolAssetProcessorBatchWorks(object):
    def test_CLITool_AssetProcessorBatch_Works(self, workspace):
        """
        Test Launching AssetProcessorBatch and verifies that is shuts down without issue
        """
        workspace.asset_processor.batch_process()
