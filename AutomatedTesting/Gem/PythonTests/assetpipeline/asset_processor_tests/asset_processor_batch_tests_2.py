"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

General Asset Processor Batch Tests
"""

# Import builtin libraries
import pytest
import logging
import os
import subprocess

# Import LyTestTools

from ly_test_tools.o3de.asset_processor import AssetProcessor
from ly_test_tools.o3de import asset_processor as asset_processor_utils
import ly_test_tools.environment.file_system as fs

# Import fixtures
from ..ap_fixtures.asset_processor_fixture import asset_processor as asset_processor
from ..ap_fixtures.ap_setup_fixture import ap_setup_fixture as ap_setup_fixture


# Import LyShared
from ly_test_tools.o3de.ap_log_parser import APLogParser, APOutputParser
import ly_test_tools.o3de.pipeline_utils as utils

# Use the following logging pattern to hook all test logging together:
logger = logging.getLogger(__name__)
# Configuring the logging is done in ly_test_tools at the following location:
# ~/dev/Tools/LyTestTools/ly_test_tools/log/py_logging_util.py

# Helper: variables we will use for parameter values in the test:
targetProjects = ["AutomatedTesting"]


@pytest.fixture
def local_resources(request, workspace, ap_setup_fixture):
    # Test-level asset folder. Directory contains a subfolder for each test (i.e. C1234567)
    ap_setup_fixture["tests_dir"] = os.path.abspath(os.path.join(os.path.dirname(os.path.realpath(__file__)), '..',
                                                                 "asset_processor_tests"))


def run_and_check_output(workspace, project, error_expected, error_search_terms, platform=None):
    # Capture AP Batch Console Output
    ap_batch_output = asset_processor_utils.assetprocessorbatch_check_output(
        workspace, project, platform, log_info=True)
    ap_batch_output = [line.decode('utf-8') for line in ap_batch_output]

    # Check for error in output
    if error_expected:
        utils.validate_log_output(ap_batch_output, error_search_terms, [])
    else:
        utils.validate_log_output(ap_batch_output, [], error_search_terms)


@pytest.mark.usefixtures("asset_processor")
@pytest.mark.usefixtures("ap_setup_fixture")
@pytest.mark.usefixtures("local_resources")
@pytest.mark.parametrize("project", targetProjects)
@pytest.mark.assetpipeline
@pytest.mark.SUITE_main
class TestsAssetProcessorBatch_AllPlatforms(object):
    """
    Platform Agnostic Tests for Asset Processor Batch
    """

    @pytest.mark.assetpipeline
    @pytest.mark.test_case_id("C3594360")
    @pytest.mark.test_case_id("C3688013")
    @pytest.mark.SUITE_sandbox(reason="Disabling flaky test, replacing with a more deterministic version")
    # fmt:off
    def test_AllSupportedPlatforms_FastScanWorks_FasterThanFullScan(self, workspace, asset_processor, ap_setup_fixture):
        # fmt:on
        """
        Tests that fast scan mode can be used and is faster than full scan mode.

        Test Steps:
        1. Ensure all assets are processed
        2. Run Asset Processor without fast scan and measure the time it takes to run
        3. Capture Full Analysis wasn't performed and number of assets processed
        4. Run Asset Processor with full scan and measure the time it takes to run
        5. Capture Full Analysis was performed and number of assets processed
        6. Verify that fast scan was faster than full scan
        7. Verify that full scan scanned more assets
        """

        # Prepare the test environment.
        env = ap_setup_fixture
        source_dir, _ = asset_processor.prepare_test_environment(env["tests_dir"], "test_AllSupportedPlatforms_FastScanWorks_FasterThanFullScan")
        assets_name = "manyfiles_forscanning.zip"
        test_assets_zip = os.path.join(source_dir, assets_name)
        destination_path = source_dir

        # Extract test assets to the project folder.
        fs.unzip(destination_path, test_assets_zip)

        # Run once to make sure all assets are processed, so fast scan on a processed asset library can be compared
        # to a non-fast scan.
        assert asset_processor.batch_process(fastscan=True), "AP batch full scan failed"

        # Run with fast scan disabled, to measure the time it takes to perform a slow scan.
        assert asset_processor.batch_process(fastscan=False), "AP batch full scan failed"

        log = APLogParser(workspace.paths.ap_batch_log())

        full_scan_time = log.runs[-1]["Time"]
        full_scan_analysis = log.runs[-1]["Full Analysis"]

        logger.info(f"First scan time: {full_scan_time}")
        logger.info(f" - full analysis required on {full_scan_analysis[0]} / {full_scan_analysis[1]} files")

        assert asset_processor.batch_process(fastscan=True)

        log = APLogParser(workspace.paths.ap_batch_log())
        fast_scan_time = log.runs[-1]["Time"]
        fast_scan_analysis = log.runs[-1]["Full Analysis"]

        logger.info(f"Fast scan time: {fast_scan_time}")
        logger.info(f" - full analysis required on {fast_scan_analysis[0]} / {fast_scan_analysis[1]} files")

        assert full_scan_time > fast_scan_time, "Fast scan was slower that full scan"
        assert full_scan_analysis[0] > fast_scan_analysis[0], "Full scan did not process more assets than fast scan"

    @pytest.mark.test_case_id("C4874121")
    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    @pytest.mark.parametrize("clear_type", ["rewrite", "delete_asset", "delete_dir"])
    @pytest.mark.SUITE_sandbox(reason="Disabling flaky test")
    def test_AllSupportedPlatforms_DeleteBadAssets_BatchFailedJobsCleared(
            self, workspace, request, ap_setup_fixture, asset_processor,  clear_type):
        """
        Tests the ability of Asset Processor to recover from processing of bad assets by removing them from scan folder

        Test Steps:
        1. Create testing environment with good and multiple bad assets
        2. Run Asset Processor
        3. Verify that bad assets fail to process
        4. Fix a bad asset & delete the others
        5. Run Asset Processor
        6. Verify Asset Processor does not have any asset failures
        """
        env = ap_setup_fixture
        error_search_terms = ["JSON parse error at line 1: Invalid value."]

        # Setup Start #
        # Add the working asset to the project folder
        asset_processor.prepare_test_environment(env["tests_dir"], "TestAssets")
        # Ensure that the project is built in cache
        asset_processor.batch_process()
        test_dir = asset_processor.add_scan_folder(os.path.join(workspace.project, "TestAssets", "multiple_corrupted_prefab"))
        # Setup End #

        # Ensure that the test file failed to process
        asset_processor.run_and_check_output(True, error_search_terms)

        # Setup for fixing the intentional error during the reprocessing steps
        fixed_prefab = "{}"
        asset_fix1 = os.path.join(test_dir, "corrupted_prefab1.prefab")
        asset_fix2 = os.path.join(test_dir, "corrupted_prefab2.prefab")
        assets_to_fix = [asset_fix1, asset_fix2]

        # Reprocessing Test Step Variations
        if clear_type == "rewrite":
            for each_asset in assets_to_fix:
                with open(each_asset, "w") as file:
                    file.write(fixed_prefab)
        elif clear_type == "delete_asset":
            for each_asset in assets_to_fix:
                fs.delete([each_asset], True, False)
        elif clear_type == "delete_dir":
            fs.delete([test_dir], False, True)

        # Validate the error was removed #
        asset_processor.run_and_check_output(False, error_search_terms)


@pytest.mark.usefixtures("asset_processor")
@pytest.mark.usefixtures("ap_setup_fixture")
@pytest.mark.usefixtures("local_resources")
@pytest.mark.parametrize("project", targetProjects)
@pytest.mark.assetpipeline
@pytest.mark.SUITE_main
class TestsAssetProcessorBatch_Windows(object):
    """
    Specific Tests for Asset Processor Batch To Only Run on Windows
    """

    @pytest.mark.test_case_id("C1564068")
    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    def test_WindowsPlatforms_RunAPBatchAndConnectGui_RunsWithoutEditor(self, asset_processor):
        """
        Verify the AP batch and Gui can run and process assets independent of the Editor
        We do not want or need to kill running Editors here as they can be involved in other tests
        or simply being run locally in this branch or another

        Test Steps:
        1. Create temporary testing environment
        2. Run asset processor GUI
        3. Verify AP GUI doesn't error
        4. Stop AP GUI
        5. Run Asset Processor Batch with Fast Scan
        5. Verify Asset Processor Batch exits cleanly
        """

        asset_processor.create_temp_asset_root()
        # Start the processor
        # using -ap_disableAssetTreeView=true to skip the UI building of the Asset Tree for this test
        asset_processor.gui_process(quitonidle=False, connect_to_ap=True, extra_params=[f'-ap_disableAssetTreeView=true'])
        asset_processor.stop()

        # fmt:off
        assert asset_processor.batch_process(fastscan=True, timeout=1200), \
            "AP Batch failed to complete in the required time."
        # fmt:on


    @pytest.mark.test_case_id("C1591341")
    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    # fmt:off
    def test_WindowsPlatforms_BatchProcessUnknownPlatform_UnknownPlatformMessageInLog(self, asset_processor):
        # fmt:on
        """
        Request a run for an invalid platform
        "AssetProcessor: Error: Platform in config file or command line 'notaplatform'" should be present in the logs

        Test Steps:
        1. Create temporary testing environment
        2. Run Asset Processor with an invalid platform
        3. Check that asset processor returns an Error notifying the user that the invalid platform is not supported
        """
        asset_processor.create_temp_asset_root()
        error_search_terms = 'AssetProcessor: Error: The list of enabled platforms in the settings registry does not contain platform ' \
                             '"notaplatform"'
        # Run APBatch expecting it to fail
        asset_processor.run_and_check_output(True, error_search_terms, platforms='notaplatform')

