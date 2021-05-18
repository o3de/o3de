"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.


UI Apps: AssetProcessor
Open AssetProcessor, Wait until AssetProcessor is Idle
Close AssetProcessor.
"""


import pytest
from ly_test_tools.o3de.asset_processor import AssetProcessor


@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.SUITE_smoke
class TestsUIAppsAssetProcessorCheckIdle(object):
    @pytest.fixture(autouse=True)
    def setup_teardown(self, request):
        self.asset_processor = None

        def teardown():
            self.asset_processor.stop()

        request.addfinalizer(teardown)

    def test_UIApps_AssetProcessor_CheckIdle(self, workspace):
        """
        Test Launching AssetProcessorBatch and verifies that is shuts down without issue
        """
        self.asset_processor = AssetProcessor(workspace)
        self.asset_processor.gui_process()
