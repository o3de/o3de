"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

General Asset Processor Batch Tests
"""

# Import builtin libraries
import logging
import pytest
import os

# Import LyTestTools
from ly_test_tools.o3de import asset_processor as asset_processor_utils

# Import fixtures
from ..ap_fixtures.asset_processor_fixture import asset_processor as asset_processor
from ..ap_fixtures.ap_setup_fixture import ap_setup_fixture as ap_setup_fixture

# Helper: variables we will use for parameter values in the test:
targetProjects = ["AutomatedTesting"]

logger = logging.getLogger(__name__)

@pytest.fixture
def local_resources(request, workspace, ap_setup_fixture):
    ap_setup_fixture["tests_dir"] = os.path.abspath(os.path.join(os.path.dirname(os.path.realpath(__file__))))


@pytest.mark.usefixtures("asset_processor")
@pytest.mark.usefixtures("ap_setup_fixture")
@pytest.mark.usefixtures("local_resources")
@pytest.mark.parametrize("project", targetProjects)
class TestsAssetProcessorBatch_AllPlatforms(object):

    def test_RunAPBatch_ScriptCanvasFileOnlyProcessesOnce(self, asset_processor, ap_setup_fixture):
        """
        Tests processing a script canvas file multiple times will not result in the file re-processing.
        This is a regression test for script canvas files that were generating a unique fingerprint
        each time CreateJobs was called, causing them to re-process each time Asset Processor was run.

        Asset processor is run several times, with multiple assets, because this regression test does not
        reproduce that often, this setup makes a failure occur more often without the fix to stabilize the fingerprint.

        Test Steps:
        1. Create a test environment with invalid assets
        2. Run asset processor
        3. Verify the script canvas test asset was processed
        4. Run asset processor a few more times
        5. Verify nothing was processed
        """
        asset_processor.prepare_test_environment(ap_setup_fixture["tests_dir"], "RunAPBatch_ScriptCanvasFileOnlyProcessesOnce")

        result, output = asset_processor.batch_process(capture_output=True, fastscan=False)
        assert result, "AP Batch failed"
        num_processed_assets = asset_processor_utils.get_num_processed_assets(output)
        expected_processed_assets = 7
        assert num_processed_assets is expected_processed_assets, f'Wrong number of processed assets, expected {expected_processed_assets}, but {num_processed_assets} were processed'

        missing_assets, existing_assets = asset_processor.compare_assets_with_cache()
        assert not missing_assets
        
        expected_assets = ['graphwithmultipleentities-1', 'graphwithmultipleentities-2', 'graphwithmultipleentities-3', 'graphwithmultipleentities-4', 'graphwithmultipleentities', 'test_mathexpression_complex' ]
        
        expected_assets.sort()
        existing_assets.sort()

        assert existing_assets == expected_assets, f'Following assets were found in cache: {existing_assets}, but expected {expected_assets}'
        
        for processing_count in range(0, 5):
            result, output = asset_processor.batch_process(capture_output=True, fastscan=False)
            assert result, "AP Batch failed"
            num_processed_assets = asset_processor_utils.get_num_processed_assets(output)
            expected_processed_assets = 0
            assert num_processed_assets is expected_processed_assets, f'Wrong number of processed assets, expected {expected_processed_assets}, but {num_processed_assets} were processed'
