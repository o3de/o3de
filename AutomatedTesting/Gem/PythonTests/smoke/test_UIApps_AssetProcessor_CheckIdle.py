"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT


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
