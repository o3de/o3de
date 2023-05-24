"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

General Asset Processor Batch Tests
"""

# Import builtin libraries
from os import listdir

import pytest
import logging
import os
from pprint import pformat
import stat
import shutil

from ly_test_tools.o3de import asset_processor as asset_processor_utils
import ly_test_tools.environment.waiter as waiter
import ly_test_tools.environment.file_system as file_system

import assetpipeline.assetpipeline_utils.assetpipeline_constants as CONSTANTS

# Import fixtures
from ..ap_fixtures.asset_processor_fixture import asset_processor as asset_processor
from ..ap_fixtures.ap_setup_fixture import ap_setup_fixture as ap_setup_fixture

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
    ap_setup_fixture["tests_dir"] = os.path.dirname(os.path.realpath(__file__))


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
@pytest.mark.SUITE_periodic
class TestsAssetProcessorBatch_AllPlatforms(object):
    """
    Platform Agnostic Tests for Asset Processor Batch
    """

    @pytest.mark.assetpipeline
    def test_APOutputParser_GenericInput_ParseSuccessful(self, asset_processor):
        """
        Tests Basic parsing with APOutputParser
        """

        lines = ['first line', 'asset_processor: second line test', 'asset_processor: third line']
        log = APOutputParser(lines)
        for _ in log.get_lines(-1, ["second", "test"]):
            second_line_found = True

        assert second_line_found, "Parsing in APOutputParser was not successful"

    @pytest.mark.test_case_id("C1564052")
    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    def test_AllSupportedPlatforms_RunAPBatch_ExitCodeZero(self, asset_processor):
        """
        Tests Launching AssetProcessorBatch and verifies that is shuts down without issue
        """
        # Create a sample asset root so we don't process every asset for every platform - this takes
        # Far too long and is not the purpose of the test
        asset_processor.create_temp_asset_root()
        # Launch AssetProcessorBatch, assert batch processing success
        result, _ = asset_processor.batch_process()
        assert result, "AP Batch failed"



    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    def test_RunAPBatch_TwoPlatforms_ExitCodeZero(self, asset_processor):
        """
        Tests Process assets for PC & Mac and verifies that processing exited without error

        Test Steps:
        1. Add Mac and PC as enabled platforms
        2. Process Assets
        3. Validate that AP exited cleanly
        """
        asset_processor.create_temp_asset_root()
        asset_processor.enable_asset_processor_platform("pc")
        asset_processor.enable_asset_processor_platform("mac")
        result, _ = asset_processor.batch_process()
        assert result, "AP Batch failed"


    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    @pytest.mark.test_case_id('C1571826')
    def test_RunAPBatch_OnlyIncludeInvalidAssets_NoAssetsAdded(self, asset_processor, ap_setup_fixture):
        """
        Tests processing invalid assets and validating that no assets were moved to the cache

        Test Steps:
        1. Create a test environment with invalid assets
        2. Run asset processor
        3. Validate that no assets were found in the cache
        """
        asset_processor.prepare_test_environment(ap_setup_fixture["tests_dir"], "test_ProcessAssets_OnlyIncludeInvalidAssets_NoAssetsAdded")

        result, _ = asset_processor.batch_process()
        assert result, "AP Batch failed"
        _, existing_assets = asset_processor.compare_assets_with_cache()
        assert not existing_assets, 'Following assets were found in cache: {}'.format(existing_assets)


    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    @pytest.mark.test_case_id('C1587615')
    def test_ProcessAndDeleteCache_APBatchShouldReprocess(self, asset_processor, ap_setup_fixture):
        """
        Tests processing once, deleting the generated cache, then processing again and validates the cache is created

        Test Steps:
        1. Run asset processor
        2. Compare the cache with expected output
        3. Delete Cache
        4. Compare the cache with expected output to verify that cache is gone
        5. Run asset processor with fastscan disabled
        6. Compare the cache with expected output
        """
        # Deleting assets from Cache will make them re-processed in AP (after start)

        # Copying test assets to project folder and deleting them from cache to make sure APBatch will process them
        asset_processor.prepare_test_environment(ap_setup_fixture["tests_dir"], os.path.join( "TestAssets", "single_working_prefab"))

        # Calling AP first time and checking whether desired assets were processed
        result, _ = asset_processor.batch_process()
        assert result, "AP Batch failed"
        missing_assets, _ = asset_processor.compare_assets_with_cache()
        assert not missing_assets, 'Following assets were not found in cache {}.  Cache location: {}.'.format(
            missing_assets, ap_setup_fixture['cache_test_assets'])

        # Deleting cache for processed assets and checking that they're no longer in cache
        asset_processor.delete_temp_cache()
        _, existing_assets = asset_processor.compare_assets_with_cache()
        assert not existing_assets, 'Following assets were found in cache {}.  Cache location: {}.'.format(
            existing_assets, ap_setup_fixture['cache_test_assets'])

        # Starting AP Batch again to and checking if assets are back in cache. Turn off fast scan because it only scans
        # source files for changes, and not the cache.
        result, _ = asset_processor.batch_process(fastscan=False)
        assert result, "AP Batch failed"
        missing_assets, _ = asset_processor.compare_assets_with_cache()
        assert not missing_assets, 'Following assets were not found in cache {}.  Cache location: {}.'.format(
            missing_assets, ap_setup_fixture['cache_test_assets'])


    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    @pytest.mark.test_case_id('C1591564')
    def test_ProcessAndChangeSource_APBatchShouldReprocess(self, asset_processor, ap_setup_fixture):
        """
        Tests reprocessing of a modified asset and verifies that it was reprocessed

        Test Steps:
        1. Prepare test environment and copy test asset over
        2. Run asset processor
        3. Verify asset processed
        4. Verify asset is in cache
        4. Modify asset
        5. Re-run asset processor
        6. Verify asset was processed
        """
        # AP Batch Processing changed files (after start)

        # Copying test assets to project folder and deleting them from cache to make sure APBatch will process them
        asset_processor.prepare_test_environment(ap_setup_fixture["tests_dir"], os.path.join("TestAssets", "single_working_prefab"))
        source_asset = os.listdir(asset_processor.project_test_source_folder())[0]
        asset_path = os.path.join(asset_processor.project_test_source_folder(), source_asset)

        # Calling AP first time and checking whether desired assets were processed
        batch_success, output = asset_processor.batch_process(capture_output=True)
        assert batch_success, f"AP Batch failed with output: {output}"
        missing_assets, _ = asset_processor.compare_assets_with_cache()

        assert not missing_assets, 'Following assets were not found in cache {}. AP Batch Output {}'.format(
            missing_assets, output)

        # Copy a file with a slight change in it, so the asset will be re-processed.
        shutil.copyfile(os.path.join(ap_setup_fixture["tests_dir"], "assets", "TestAssets", "single_working_prefab_override", "working_prefab.prefab"), asset_path)

        # Reprocessing and getting number of successfully processed assets from output
        batch_success, output = asset_processor.batch_process(capture_output=True)
        # Checking the number of jobs is equal to 1
        num_processed_assets = asset_processor_utils.get_num_processed_assets(output)

        # Print the output if the test is going to fail, to make it easier to debug.        
        if num_processed_assets != 1:
            for output_line in output:
                logger.info(f"{output_line}\n")

        assert num_processed_assets == 1, \
            f'Wrong number of successfully processed assets found in output. Expected: 1 Found: {num_processed_assets}'


    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    @pytest.mark.SUITE_periodic
    def test_ProcessByBothApAndBatch_Md5ShouldMatch(self, asset_processor, ap_setup_fixture):
        """
        Tests that a cache generated by AP GUI is the same as AP Batch

        Test Steps:
        1. Create test environment with test assets
        2. Call asset processor batch
        3. Get checksum for file cache
        4. Clean up test environment
        5. Call asset processor gui with quitonidle
        6. Get checksum for file cache
        7. Verify that checksums are equal
        """
        # AP Batch and AP app processed assets MD5 sums should be the same

        # Copying test assets to project folder and deleting them from cache to make sure APBatch will process them
        asset_processor.prepare_test_environment(ap_setup_fixture["tests_dir"], "test_ProcessByBothApAndBatch_Md5ShouldMatch")

        # Calling APbatch and calculating checksums for processed files
        result, _ = asset_processor.batch_process()
        assert result, "AP Batch failed"

        checksum_apbatch = utils.get_files_hashsum(asset_processor.project_test_cache_folder())

        # Deleting processed and source assets, running APBatch to clear asset database
        asset_processor.delete_temp_cache()
        asset_processor.clear_project_test_assets_dir()
        result, _ = asset_processor.batch_process()
        assert result, "AP Batch failed"

        # Copying test assets once again
        asset_processor.prepare_test_environment(ap_setup_fixture["tests_dir"], "test_ProcessByBothApAndBatch_Md5ShouldMatch")

        result, _ = asset_processor.gui_process()
        assert result, "AP GUI failed"

        checksum_assetprocessor = utils.get_files_hashsum(asset_processor.project_test_cache_folder())

        assert checksum_apbatch == checksum_assetprocessor, 'md5 sums are not identical'


    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    @pytest.mark.test_case_id('C1612446')
    def test_AddSameAssetsDifferentNames_ShouldProcess(self, asset_processor, ap_setup_fixture):
        """
        Tests Asset Processing of duplicate assets with different names and verifies that both assets are processed

        Test Steps:
        1. Create test environment with two identical source assets with different names
        2. Run asset processor
        3. Verify that assets didn't fail to process
        4. Verify the correct number of jobs were performed
        5. Verify that product files are in the cache
        """
        # Feed two similar prefabs with different names - should process without any issues

        # Copying test assets to project folder and deleting them from cache to make sure APBatch will process them
        asset_processor.prepare_test_environment(ap_setup_fixture["tests_dir"], os.path.join("TestAssets", "multiple_working_prefab"))

        # Launching AP, capturing output and asserting on number of processed assets
        result, output = asset_processor.batch_process(capture_output=True)
        assert result, "Asset processor batch failed"

        num_failed_assets = asset_processor_utils.get_num_failed_processed_assets(output)
        assert num_failed_assets is 0, 'Wrong number of failed assets'

        # Checking if number of jobs were two after reprocessing the prefabs
        #    There are possible asset updates we should allow such as bootstrap.cfg for the branch token
        #    Q - But what if <many extra assets are processed question>
        #    A - Add a test specific for that case.  Restrictions on "exactly this number of assets and no more
        #    were processed" are brittle and force engineers to go figure out whether they actually broke something
        #    or an expected behavior has changed.  Processing bootstrap.cfg sometimes but not other times should not
        #    cause a failure in this test.
        num_processed_assets = asset_processor_utils.get_num_processed_assets(output)
        assert num_processed_assets >= 2, f'Wrong number of successfully processed assets found in output: '\
                                          '{num_processed_assets}'

        missing_assets, _ = asset_processor.compare_assets_with_cache()
        assert not missing_assets, 'Following assets are missing in cache: {}'.format(missing_assets)

    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    @pytest.mark.test_case_id('C1612448')
    def test_TwoAssetsWithSameProductName_ShouldProcessAfterRename(self, asset_processor, ap_setup_fixture):
        """
        GHI - https://github.com/o3de/o3de/issues/15250
        
        Tests processing of two assets with the same product name, then verifies that AP will successfully process after
        renaming one of the assets' outputs.

        Test Steps:
        1. Create test environment with two assets that will output products with the same name
        2. Launch Asset Processor
        3. Validate that Asset Processor generates an error
        4. Use an assetinfo file to rename the output product
        5. Run asset processor
        6. Verify that asset processor does not error
        7. Verify that expected product files are in the cache
        """

        def run_ap_reprocess_and_skip_atom_output(asset_processor):
            source_folder = asset_processor.project_test_source_folder()
            reprocess_file_list = [os.path.join(source_folder, "a.fbx"),
                                   os.path.join(source_folder, "b.fbx")]

            result, output = asset_processor.batch_process(capture_output=True,
                                                           skip_atom_output=True)

            # If the test fails, it's helpful to have the output from asset processor in the logs, to track the failure down.
            if not result:
                logger.info(f"Asset Processor Output: {pformat(output)}")
            return result, output

        # Copying test assets to project folder
        asset_processor.prepare_test_environment(ap_setup_fixture["tests_dir"], "test_TwoAssetsWithSameProductName_ShouldProcessAfterRename")

        # Launching AP, verify it is failing
        result, output = run_ap_reprocess_and_skip_atom_output(asset_processor)
        assert result == False, \
            'AssetProcessorBatch should have failed because the generated output products should share the same name.'

        # Renaming output files so they won't collide in cache after second processing
        asset_info_file = os.path.join(
            asset_processor.temp_asset_root(),
            "AutomatedTesting", "test_TwoAssetsWithSameProductName_ShouldProcessAfterRename", "a.fbx.assetinfo"
        )
        data_for_rename = os.path.join(
            asset_processor.temp_asset_root(),
            "AutomatedTesting", "test_TwoAssetsWithSameProductName_ShouldProcessAfterRename", "rename.txt"
        )

        waiter.wait_for(lambda: file_system.delete(asset_info_file, True, False), timeout=CONSTANTS.TIMEOUT_5)
        waiter.wait_for(lambda: file_system.rename(data_for_rename, asset_info_file), timeout=CONSTANTS.TIMEOUT_5)

        # Reprocessing files and making sure there are no failed jobs
        result, output = run_ap_reprocess_and_skip_atom_output(asset_processor)
        assert result, "AssetProcessorBatch failed when it should have succeeded after renaming output."

        num_failed_assets = asset_processor_utils.get_num_failed_processed_assets(output)
        assert num_failed_assets is 0, 'Expected no failed assets.'

        expected_assets = ['a.actor', 'b.actor']
        missing_assets, _ = utils.compare_assets_with_cache(expected_assets, asset_processor.project_test_cache_folder())
        assert not missing_assets, f"The following assets were expected, but not found in cache {missing_assets}."

    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    def test_InvalidServerAddress_Warning_Logs(self, asset_processor):
        """
        Tests running Asset Processor with an invalid server address and verifies that AP returns a warning about
        an invalid server address

        Test Steps:
        1. Launch asset processor while providing an invalid server address
        2. Verify asset processor does not fail
        3. Verify that asset processor generated a warning informing the user about an invalid server address
        """

        asset_processor.create_temp_asset_root()
        # Launching AP and making sure that the warning exists
        result, output = asset_processor.batch_process(capture_output=True, extra_params=[
            CONSTANTS.ASSET_CACHE_INVALID_SERVER_ADDRESS, CONSTANTS.ASSET_CACHE_SERVER_MODE])
        assert result, "AssetProcessorBatch failed when it should have had a warning and no failure"

        assert asset_processor_utils.has_invalid_server_address(output, CONSTANTS.INVALID_SERVER_ADDRESS), \
            'Invalid server address warning not present.'


    @pytest.mark.test_case_id("C1571774")
    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    def test_AllSupportedPlatforms_IncludeValidAssets_AssetsProcessed(self, asset_processor, ap_setup_fixture):
        """
        AssetProcessorBatch is successfully processing newly added assets

        Test Steps:
        1. Create a test environment with test assets
        2. Launch Asset Processor
        3. Verify that asset processor does not fail to process
        4. Verify assets are not missing from the cache
        """
        env = ap_setup_fixture

        # Prepare test assets and start Asset Processor Batch
        asset_processor.prepare_test_environment(env["tests_dir"], os.path.join("TestAssets", "Working_Prefab"))
        result, output_list = asset_processor.batch_process(capture_output=True)
        assert result, f"AP Batch failed with output: {output_list}"

        # Determine if any assets are missing from the cache
        #     _ is a tuple catch for expected_assets but is not used in this test but is available in scenarios that
        #     a test would need to check the expected_assets assets in the cache
        missing_assets, found_assets = asset_processor.compare_assets_with_cache()

        if missing_assets:
            logger.info(f"Missing asset Failure log was {output_list}")
        assert not missing_assets, f"Following assets were not found in cache: {missing_assets}, found was: {found_assets}"

    @pytest.mark.test_case_id("C1568831")
    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    def test_AllSupportedPlatforms_DeletedAssets_DeletedFromCache(self, asset_processor, ap_setup_fixture):
        """
        AssetProcessor successfully deletes cached items when removed from project

        Test Steps:
        1. Create a test environment with test assets
        2. Run asset processor
        3. Verify expected assets are in the cache
        4. Delete test assets
        5. Run asset processor
        6. Verify expected assets are in the cache
        """
        env = ap_setup_fixture

        # Copying test assets to project folder and deleting them from cache to make sure APBatch will process them
        test_assets_folder, cache_folder = asset_processor.prepare_test_environment(env["tests_dir"], "C1568831")

        assert asset_processor.batch_process(), "First AP Batch Failed"

        missing_assets, _ = asset_processor.compare_assets_with_cache()

        # Make sure everything was copied
        # fmt:off
        assert not missing_assets, \
            f"Following assets were not deleted by APBatch when they were deleted from source: {missing_assets}"
        # fmt:on

        # Deleting test assets to verify further deletion from cache by asset processor
        asset_processor.clear_project_test_assets_dir()

        # Running AP Batch again so it will delete assets from cache
        assert asset_processor.batch_process(), "Second AP Batch Failed"

        _, existing_assets = asset_processor.compare_assets_with_cache()
        # fmt:off
        assert not existing_assets, \
            f"Following assets were not deleted by APBatch but they were deleted from source: {existing_assets}"
        # fmt:on

    @pytest.mark.test_case_id("C1564051")
    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    def test_AllSupportedPlatforms_RunAPBatch_StartsAndProcessesAssets(self, asset_processor):
        """
        Tests that when cache is deleted (no cache) and AssetProcessorBatch runs,
        it successfully starts and processes assets.

        Test Steps:
        1. Run asset processor
        2. Verify asset processor exits cleanly
        """
        asset_processor.create_temp_asset_root()

        # fmt:off
        assert asset_processor.batch_process(fastscan=False, ), \
            "AP Batch failed to complete in the required time."
        # fmt:on

    @pytest.mark.test_case_id("C1591338")
    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    # fmt:off
    def test_AllSupportedPlatforms_RunAPBatch_DeletedAssetRecovery(self, asset_processor,
                                                                   ap_setup_fixture):
        # fmt:on
        """
        AssetProcessor successfully recovers assets from cache when deleted.

        Test Steps:
        1. Create test environment with test assets
        2. Run Asset Processor and verify it exits cleanly
        3. Make sure cache folder was generated
        4. Delete temp cache assets but leave database behind
        5. Run asset processor and verify it exits cleanly
        6. Verify expected files were generated in the cache
        """
        env = ap_setup_fixture

        # Add assets to test asset directory
        test_assets_folder, cache_folder = asset_processor.prepare_test_environment(env["tests_dir"], os.path.join("TestAssets", "single_working_prefab"))

        # Run batch to ensure everything is processed
        assert asset_processor.batch_process(), "First AP Batch failed"

        # Make sure folder got created
        assert os.path.exists(test_assets_folder), f"Test assets folder was not created {test_assets_folder}"

        # fmt:off
        assert os.path.exists(cache_folder), \
            "AP did not create a cache folder for the test assets"
        # fmt:on

        # Delete the cached assets
        asset_processor.delete_temp_cache()

        # Run batch to recover deleted test assets
        assert asset_processor.batch_process(fastscan=False), "Second AP Batch Failed"

        missing_assets, _ = asset_processor.compare_assets_with_cache()

        assert not missing_assets, f"The following assets were not found in the cache {missing_assets}"

    @pytest.mark.test_case_id("C3635833")
    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    # fmt:off
    def test_AllSupportedPlatforms_RunFastScanOnEmptyCache_FullScanRuns(self, ap_setup_fixture, asset_processor):
        """
        Tests fast scan processing on an empty cache and verifies that a full analyis will be peformed

        Test Steps:
        1. Create a test environment
        2. Execute asset processor batch with fast scan enabled
        3. Verify that a full analysis is performed
        """
        # fmt:on
        env = ap_setup_fixture
        asset_processor.create_temp_asset_root()
        success, output = asset_processor.batch_process(capture_output=True)
        log = APOutputParser(output)

        # Get "Full Analysis" line data from log.
        #    If exists: will cast to True, if not: will cast to False
        line_found = bool(log.runs[-1]["Full Analysis"])

        assert line_found, "Full analysis was not performed when asset cache and first scan was performed."

    @pytest.mark.test_case_id("C1564055")
    @pytest.mark.test_case_id("C1564056")
    @pytest.mark.BAT
    @pytest.mark.BVT
    @pytest.mark.assetpipeline
    def test_AllSupportedPlatforms_RunAPBatchAndGUI_JobLogsExist(self, ap_setup_fixture, asset_processor, workspace):
        """
        After running the APBatch and AP GUI, Logs directory should exist (C1564055),
        JobLogs, Batch log, and GUI log should exist in the logs directory (C1564056)

        Test Steps:
        1. Run asset processor batch
        2. Run asset processor gui with quit on idle
        3. Verify that logs exist for both AP Batch & AP GUI
        """
        asset_processor.create_temp_asset_root()
        asset_processor.create_temp_log_root()
        LOG_PATH = {
            "batch_log": workspace.paths.ap_batch_log(),
            "gui_log": workspace.paths.ap_gui_log()
        }

        class LogTimes:
            batch_log_start_time = 0
            gui_log_start_time = 0

            batch_log_final_time = 0
            gui_log_final_time = 0

            @staticmethod
            def Report():
                print(
                    f"""
                    Original Times:
                    Batch: {LogTimes.batch_log_start_time}
                    GUI: {LogTimes.gui_log_start_time}

                    Post-Run Times:
                    Batch: {LogTimes.batch_log_final_time}
                    GUI: {LogTimes.gui_log_final_time}
                    """
                )

        def update_time(path_key, key):
            if os.path.exists(LOG_PATH[path_key]):
                if not LogTimes.__dict__[path_key + key]:
                    setattr(LogTimes, path_key + key, os.stat(LOG_PATH[path_key]).st_mtime)
                else:
                    setattr(LogTimes, path_key + key, os.stat(LOG_PATH[path_key]).st_mtime)

        def update_times(key):
            for path_key in LOG_PATH.keys():
                update_time(path_key, key)
            LogTimes.Report()

        def check_existence(name, path):
            assert os.path.exists(path), f"{name} could not be located {path} after running the AP."

        # Check if log files previously exist and grab their modification times
        update_times("_start_time")

        # Run the Batch process
        assert asset_processor.batch_process(create_temp_log=False), "Batch process failed to successfully terminate"

        asset_processor.gui_process(quitonidle=True, create_temp_log=False)

        # Check that the Logs directory exists (C1564055)
        check_existence("Logs Directory", workspace.paths.ap_log_dir())

        update_times("_final_time")

        for key in LOG_PATH.keys():
            # fmt: off
            assert LogTimes.__dict__[key + "_final_time"] > LogTimes.__dict__[key + "_start_time"], \
                f"The {key} log was not modified during the run."
            # fmt: on


    @pytest.mark.test_case_id("C1564073")
    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    # fmt:off
    def test_AllSupportedPlatforms_CorruptPrefab_BatchReportsProcessingFail(self, asset_processor, ap_setup_fixture):
        # fmt:on
        """
        Utilizing corrupted test assets, run the batch process to verify the
        AP logs the failure to process the corrupted file.

        Test Steps:
        1. Create test environment with corrupted_prefab
        2. Launch Asset Processor
        3. Verify that asset processor fails to process corrupted_prefab
        """
        env = ap_setup_fixture
        error_line_found = False

        # Import pre-corrupted test assets
        asset_processor.prepare_test_environment(env["tests_dir"], "TestAssets/single_corrupted_prefab")
        success, output = asset_processor.batch_process(capture_output=True, expect_failure=True)
        log = APOutputParser(output)
        for _ in log.get_lines(-1, ["Createjobs Failed", "corrupted_prefab.prefab"]):
            error_line_found = True

        assert error_line_found, "The error could not be found in the newest run of the AP Batch log."

    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    def test_validateDirectPreloadDependency_Found(self, asset_processor, ap_setup_fixture, workspace):
        """
        Tests processing an asset with a circular dependency and verifies that Asset Processor will return an error
        notifying the user about a circular dependency.

        Test Steps:
        1. Create test environment with an asset that has a preload circular dependency
        2. Launch asset processor
        3. Verify that error is returned informing the user that the asset has a circular dependency
        """
        env = ap_setup_fixture
        error_line_found = False

        asset_processor.prepare_test_environment(env["tests_dir"], "assets_with_direct_preload_dependency")

        success, output = asset_processor.batch_process(capture_output=True, expect_failure=False)
        log = APOutputParser(output)
        for _ in log.get_lines(-1, ["Preload circular dependency detected", "testb.auto_test_asset"]):
            error_line_found = True

        if not error_line_found:
            for line in output:
                print(line)

        assert error_line_found, "The error could not be found in the newest run of the AP Batch log."

    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    def test_validateNestedPreloadDependency_Found(self, asset_processor, ap_setup_fixture, workspace):
        """
        Tests processing of a nested preload circular dependency and verifies that Asset Processor will return an error
        notifying the user about a circular dependency.

        Test Steps:
        1. Create test environment with an asset that has a nested preload circular dependency
        2. Launch asset processor
        3. Verify that error is returned informing the user that the asset has a preload circular dependency
        """

        env = ap_setup_fixture
        error_line_found = False

        asset_processor.prepare_test_environment(env["tests_dir"], "assets_with_nested_preload_dependency")

        success, output = asset_processor.batch_process(capture_output=True, expect_failure=False)
        log = APOutputParser(output)
        for _ in log.get_lines(-1, ["Preload circular dependency detected", "testa.auto_test_asset"]):
            error_line_found = True

        if not error_line_found:
            for line in output:
                print(line)


        assert error_line_found, "The error could not be found in the newest run of the AP Batch log."

    @pytest.mark.assetpipeline
    def test_AssetProcessor_Log_On_Failure(self, asset_processor, ap_setup_fixture, workspace):
        asset_processor.prepare_test_environment(ap_setup_fixture["tests_dir"], "test_AP_Logs")
        result, output = asset_processor.batch_process(expect_failure=True, capture_output=True)
        assert result == False, f'AssetProcessorBatch should have failed because there is a bad asset, output was {output}'

        jobLogs = listdir(workspace.paths.ap_job_logs() + "/test_AP_Logs")
        assert not len(jobLogs) == 0, 'No job logs where output during failure.'
