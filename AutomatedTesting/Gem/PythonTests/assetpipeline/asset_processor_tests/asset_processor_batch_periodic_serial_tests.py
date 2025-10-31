"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

General Asset Processor Batch Tests
"""

import pytest
import os
import re
# Import LyTestTools
from ly_test_tools.o3de import asset_processor as asset_processor_utils

# Import fixtures
from ..ap_fixtures.asset_processor_fixture import asset_processor as asset_processor
from ..ap_fixtures.ap_setup_fixture import ap_setup_fixture as ap_setup_fixture

# Helper: variables we will use for parameter values in the test:
targetProjects = ["AutomatedTesting"]

@pytest.fixture
def local_resources(request, workspace, ap_setup_fixture):
    # Test-level asset folder. Directory contains a subfolder for each test (i.e. C1234567)
    ap_setup_fixture["tests_dir"] = os.path.dirname(os.path.realpath(__file__))

@pytest.mark.usefixtures("asset_processor")
@pytest.mark.usefixtures("ap_setup_fixture")
@pytest.mark.usefixtures("local_resources")
@pytest.mark.parametrize("project", targetProjects)
@pytest.mark.assetpipeline
@pytest.mark.SUITE_periodic
class TestsAssetProcessorBatch_PeriodicSerial_AllPlatforms(object):
    """
    Tests to be run in the periodic test suite, in serial.
    """

    @pytest.mark.assetpipeline
    def test_reprocessFileList_processesInOrder(self, asset_processor, ap_setup_fixture):
        """
        Tests the reprocessFileList and debugOutput commands to verify that all assets
        in the given list are reprocessed, in the order passed in.

        The test assets include an asset not in the reprocessFileList to verify it's not
        reprocessing additional files.

        Test steps:
        1. Create a test environment with the assets in the reprocessFileList.
        2. Generate the reprocessFileList.
        3. Process assets.
        4. Process again, verifying nothing processed.
        5. Process one more time, passing in the reprocessFileList and debug output commands.
        6. Verify from the log output that the given list of assets processed in the correct order.
        """
        asset_processor.prepare_test_environment(ap_setup_fixture["tests_dir"], "test_reprocessFileList_processesInOrder")
        
        source_folder = asset_processor._test_assets_source_folder
        # Skip a few files, to prove that it isn't reprocessing files not in the list.
        reprocess_file_list = [
            os.path.join(source_folder, "preview_1.png"),
            os.path.join(source_folder, "preview_3.png"),
            os.path.join(source_folder, "preview_5.png")
        ]
        reprocess_file_path = os.path.join(source_folder, "reprocess_list.txt")
        reprocess_file_handle = open(reprocess_file_path, "w")
        for reprocess_file in reprocess_file_list:
            reprocess_file_handle.write(reprocess_file + "\n")
        reprocess_file_handle.close()

        result, output = asset_processor.batch_process(capture_output=True, fastscan=False)
        assert result, "AP Batch failed"
        num_processed_assets = asset_processor_utils.get_num_processed_assets(output)
        expected_processed_assets = 7
        assert num_processed_assets is expected_processed_assets, f'Wrong number of processed assets, expected {expected_processed_assets}, but {num_processed_assets} were processed'

        result, output = asset_processor.batch_process(capture_output=True, fastscan=False)
        assert result, "AP Batch failed"
        num_processed_assets = asset_processor_utils.get_num_processed_assets(output)
        expected_processed_assets = 0
        assert num_processed_assets is expected_processed_assets, f'Wrong number of processed assets, expected {expected_processed_assets}, but {num_processed_assets} were processed'

        extra_params = [
            "--debugOutput",
            f"--reprocessFileList={reprocess_file_path}",
            # Setting max jobs to 1 to make the log order match the reprocess file list order.
            # Otherwise, the logs print out per-builder, and the jobs will be split across builders, printed in any order.
            "--regset=\"/Amazon/AssetProcessor/Settings/Jobs/maxJobs=1\""
        ]
        result, output = asset_processor.batch_process(capture_output=True, fastscan=False, extra_params=extra_params)
        assert result, "AP Batch failed"
        num_processed_assets = asset_processor_utils.get_num_processed_assets(output)
        expected_processed_assets = 3
        assert num_processed_assets is expected_processed_assets, f'Wrong number of processed assets, expected {expected_processed_assets}, but {num_processed_assets} were processed'

        expected_processed_assets_list = None
        builder_found = False
        builder_start_regex = "Builder (\\{[0-9A-F]{8}-[0-9A-F]{4}-[0-9A-F]{4}-[0-9A-F]{4}-[0-9A-F]{12}\\}) processed these assets:"
        for output_line in output:
            output_line_path_fix = output_line.replace('\\', '/')
            if expected_processed_assets_list is None:
                match_result = re.search(builder_start_regex, output_line_path_fix)
                if match_result:
                    builder_found = True
                    builder_uuid = match_result.group(1)
                    expected_processed_assets_list = []
                    for reprocess_file in reprocess_file_list:
                        reprocess_file_path_fix = reprocess_file.replace('\\', '/')
                        expected_processed_assets_list.append(f"Builder with ID {builder_uuid} processed {reprocess_file_path_fix}")
            else:
                if output_line_path_fix.endswith(expected_processed_assets_list[0]):
                    expected_processed_assets_list.pop(0)
                    if not expected_processed_assets_list:
                        break
        assert builder_found, "Could not find expected builder output message"
        assert not expected_processed_assets_list, f"Did not find expected processed output message(s) {expected_processed_assets_list}"

