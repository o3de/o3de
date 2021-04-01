"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

General Asset Processor Batch Tests
"""

# Import builtin libraries
import pytest
import logging
import os
import subprocess

# Import LyTestTools

from ly_test_tools.lumberyard.asset_processor import AssetProcessor
from ly_test_tools.lumberyard import asset_processor as asset_processor_utils
import ly_test_tools.environment.file_system as fs

# Import fixtures
from ..ap_fixtures.asset_processor_fixture import asset_processor as asset_processor
from ..ap_fixtures.ap_setup_fixture import ap_setup_fixture as ap_setup_fixture


# Import LyShared
from ly_test_tools.lumberyard.ap_log_parser import APLogParser, APOutputParser
import ly_test_tools.lumberyard.pipeline_utils as utils

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
class TestsAssetProcessorBatch_AllPlatforms(object):
    """
    Platform Agnostic Tests for Asset Processor Batch
    """

    @pytest.mark.assetpipeline
    @pytest.mark.test_case_id("C3594360")
    @pytest.mark.test_case_id("C3688013")
    @pytest.mark.SUITE_sandbox
    # fmt:off
    def test_AllSupportedPlatforms_FastScanWorks_FasterThanFullScan(self, workspace, asset_processor):
        # fmt:on
        """
        Tests that fast scan mode can be used and is faster than full scan mode.
        """

        asset_processor.create_temp_asset_root()
        
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

    @pytest.mark.test_case_id("C18787404")
    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    @pytest.mark.skip(reason="External project is currently broken.")  # LY-119863
    def test_AllSupportedPlatforms_ExternalProject_APRuns(self, workspace, ap_external_project_setup_fixture):

        external_resources = ap_external_project_setup_fixture
        logger.info(f"Running external project test at path {external_resources['project_dir']}")
        # Delete existing "external project" build if it exists
        if os.path.exists(external_resources["project_dir"]):
            fs.delete([external_resources["project_dir"]], True, True)

        # fmt:off
        assert not os.path.exists(external_resources["project_dir"]), \
            f'{external_resources["project_dir"]} was not deleted'
        # fmt:on

        lmbr_cmd = [
            workspace.paths.lmbr(),
            "projects",
            "create",
            external_resources["project_name"],
            "--template",
            "EmptyTemplate",
            "--app-root",
            external_resources["project_dir"],
        ]

        logger.info(f"Running lmbr projects create command '{lmbr_cmd}'")

        try:
            subprocess.check_call(lmbr_cmd)
        except subprocess.CalledProcessError as e:
            assert False, f"lmbr projects create failed\n{e.stderr}"

        logger.info("...lmbr finished")
        assert os.path.exists(external_resources["project_dir"]), "Project folder was not created"

        # AssetProcessor for new External project. Uses mock workspace to emulate external project workspace
        external_ap = AssetProcessor(external_resources["external_workspace"])

        # fmt:off
        assert external_ap.batch_process(fastscan=False), \
            "Asset Processor Batch failed on external project"
        # fmt:on

        # Parse log looking for errors or failures
        log = APLogParser(workspace.paths.ap_batch_log())
        failures, errors = log.runs[-1]["Failures"], log.runs[-1]["Errors"]
        assert failures == 0, f"There were {failures} asset processing failures"
        assert errors == 0, f"There were {errors} asset processing errors"

        # Check that project cache was created (DNE until AP makes it)
        project_cache = os.path.join(external_resources["project_dir"], "Cache")
        assert os.path.exists(project_cache), f"{project_cache} was not created by AP"

        # Clean up external project
        fs.delete([external_resources["project_dir"]], True, True)

        # fmt:off
        assert not os.path.exists(external_resources["project_dir"]), \
            f"{external_resources['project_dir']} was not deleted"
        # fmt:on

    @pytest.mark.test_case_id("C4874121")
    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    @pytest.mark.parametrize("clear_type", ["rewrite", "delete_asset", "delete_dir"])
    def test_AllSupportedPlatforms_DeleteBadAssets_BatchFailedJobsCleared(
            self, workspace, request, ap_setup_fixture, asset_processor,  clear_type):
        env = ap_setup_fixture
        error_search_terms = ["WWWWWWWWWWWW"]

        # # Setup Start # #
        # Add the working asset to the project folder
        asset_processor.prepare_test_environment(env["tests_dir"], "C4874121")

        # Ensure that the project is built in cache
        asset_processor.batch_process()

        # Find any WORKING .slice asset in the projects folders (not the cache)
        # Make a new folder there and copy the WORKING slice to that new folder
        test_dir = os.path.join(workspace.project, "Slices", "Test_C4874121")

        test_dir = asset_processor.add_scan_folder(test_dir)

        test_source_folder = os.path.join(asset_processor.temp_asset_root(), workspace.project, "C4874121")
        copied_asset = os.path.join(test_source_folder, "working_slice.slice")
        asset_processor.copy_assets_to_project(["working_slice.slice"], test_source_folder, test_dir)

        # Ensure that the test file did not fail to process
        asset_processor.run_and_check_output(False, error_search_terms)
        # Open the slice file and make an intentional error in it to test reprocessing steps
        original_slice = ""
        corrupted_slice = ""

        with open(copied_asset, "r") as file:
            original_slice = file.read()
            # This creates an intentional error in the slice file which corrupts it and makes the asset fail
            corrupted_slice = original_slice.replace("Components", "WWWWWWWWWWWW", 1)

        with open(copied_asset, "w") as file:
            file.write(corrupted_slice)

        asset_processor.run_and_check_output(True, error_search_terms)
        # # Setup End # #

        # Reprocessing Test Step Variations
        if clear_type == "rewrite":
            with open(copied_asset, "w") as file:
                file.write(original_slice)
        elif clear_type == "delete_asset":
            fs.delete([copied_asset], True, False)
        elif clear_type == "delete_dir":
            fs.delete([test_dir], False, True)

        # Validate the error was removed #
        asset_processor.run_and_check_output(False, error_search_terms)


@pytest.mark.usefixtures("asset_processor")
@pytest.mark.usefixtures("ap_setup_fixture")
@pytest.mark.usefixtures("local_resources")
@pytest.mark.parametrize("project", targetProjects)
@pytest.mark.assetpipeline
class TestsAssetProcessorBatch_Windows(object):
    """
    Specific Tests for Asset Processor Batch To Only Run on Windows
    """

    @pytest.mark.test_case_id("C1564068")
    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    @pytest.mark.SUITE_sandbox
    def test_WindowsPlatforms_RunAPBatchAndConnectGui_RunsWithoutEditor(self, asset_processor):
        """
        Verify the AP batch and Gui can run and process assets independent of the Editor
        We do not want or need to kill running Editors here as they can be involved in other tests
        or simply being run locally in this branch or another
        """

        asset_processor.create_temp_asset_root()
        # Start the processor
        asset_processor.gui_process(quitonidle=False, connect_to_ap=True)
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
        """
        asset_processor.create_temp_asset_root()
        error_search_terms = 'AssetProcessor: Error: The list of enabled platforms in the settings registry does not contain platform ' \
                             '"notaplatform"'
        # Run APBatch expecting it to fail
        asset_processor.run_and_check_output(True, error_search_terms, platforms='notaplatform')

