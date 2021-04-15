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
import stat

# Import LyTestTools
from ly_test_tools.o3de import asset_processor as asset_processor_utils

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
@pytest.mark.SUITE_sandbox
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
        asset_processor.create_temp_asset_root()
        asset_processor.enable_asset_processor_platform("pc")
        asset_processor.enable_asset_processor_platform("osx_gl")
        result, _ = asset_processor.batch_process()
        assert result, "AP Batch failed"


    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    @pytest.mark.test_case_id('C1571826')
    def test_RunAPBatch_OnlyIncludeInvalidAssets_NoAssetsAdded(self, asset_processor, ap_setup_fixture):
        asset_processor.prepare_test_environment(ap_setup_fixture["tests_dir"], "test_ProcessAssets_OnlyIncludeInvalidAssets_NoAssetsAdded")

        result, _ = asset_processor.batch_process()
        assert result, "AP Batch failed"
        _, existing_assets = asset_processor.compare_assets_with_cache()
        assert not existing_assets, 'Following assets were found in cache: {}'.format(existing_assets)


    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    @pytest.mark.test_case_id('C1571827')
    @pytest.mark.skip(
        reason="Race condition on AP batch shutdown can cause the failure to not yet be registered even though it's "
               "recognized as failing in the logs.  There appears to be a window where the AutoFailJob doesn't complete"
               "before the shutdown completes and the failure doesn't end up counting")
    def test_ProcessAssets_IncludeTwoAssetsWithSameProduct_FailingOnSecondAsset(self, asset_processor, ap_setup_fixture):
        asset_processor.prepare_test_environment(ap_setup_fixture["tests_dir"], "test_ProcessAssets_IncludeTwoAssetsWithSameProduct_FailingOnSecondAsset")
        result, output = asset_processor.batch_process(capture_output = True, expect_failure = True)

        assert not result, "Expected asset processor to fail, but it did not fail"
        num_failed_assets = asset_processor_utils.get_num_failed_processed_assets(output)
        assert num_failed_assets == 1, \
            'Wrong number of failed assets'
        missing_assets, _ = asset_processor.compare_assets_with_cache()
        assert missing_assets, 'Unexpectedly found two products in the cache with the same name: {}'.format(
            missing_assets)


    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    @pytest.mark.test_case_id('C1587615')
    def test_ProcessAndDeleteCache_APBatchShouldReprocess(self, asset_processor, ap_setup_fixture):
        # Deleting assets from Cache will make them re-processed in AP (after start)

        # Copying test assets to project folder and deleting them from cache to make sure APBatch will process them
        asset_processor.prepare_test_environment(ap_setup_fixture["tests_dir"], "test_ProcessAndDeleteCache_APBatchShouldReprocess")

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
        # AP Batch Processing changed files (after start)

        # Copying test assets to project folder and deleting them from cache to make sure APBatch will process them
        asset_processor.prepare_test_environment(ap_setup_fixture["tests_dir"], "test_ProcessAndChangeSource_APBatchShouldReprocess")

        # Calling AP first time and checking whether desired assets were processed
        batch_success, output = asset_processor.batch_process(capture_output=True)
        assert batch_success, f"AP Batch failed with output: {output}"
        missing_assets, _ = asset_processor.compare_assets_with_cache()

        assert not missing_assets, 'Following assets were not found in cache {}. AP Batch Output {}'.format(
            missing_assets, output)

        # Appending a newline to source slice
        source_asset = os.listdir(asset_processor.project_test_source_folder())[0]
        asset_path = os.path.join(asset_processor.project_test_source_folder(), source_asset)
        os.chmod(asset_path, stat.S_IWRITE)
        with open(asset_path, 'a') as source_file:
            source_file.write('\n')
            source_file.close()

        # Reprocessing and getting number of successfully processed assets from output
        batch_success, output = asset_processor.batch_process(capture_output=True)

        # Checking if number of jobs were at least two after reprocessing a slice:
        #    Two jobs for the test slice (See Test Case Test Assets Folder for Asset Names)
        num_processed_assets = asset_processor_utils.get_num_processed_assets(output)
        assert num_processed_assets >= 2, \
            f'Wrong number of successfully processed assets found in output: {num_processed_assets}'


    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    def test_ProcessByBothApAndBatch_Md5ShouldMatch(self, asset_processor, ap_setup_fixture):
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

        result, _ = asset_processor.gui_process(quitonidle=True)
        assert result, "AP GUI failed"

        checksum_assetprocessor = utils.get_files_hashsum(asset_processor.project_test_cache_folder())

        assert checksum_apbatch == checksum_assetprocessor, 'md5 sums are not identical'


    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    @pytest.mark.test_case_id('C1612446')
    def test_AddSameAssetsDifferentNames_ShouldProcess(self, asset_processor, ap_setup_fixture):
        # Feed two similar slices and texture with different names - should process without any issues

        # Copying test assets to project folder and deleting them from cache to make sure APBatch will process them
        asset_processor.prepare_test_environment(ap_setup_fixture["tests_dir"], "test_AddSameAssetsDifferentNames_ShouldProcess")

        # Launching AP, capturing output and asserting on number of processed assets
        result, output = asset_processor.batch_process(capture_output=True)
        assert result, "Asset processor batch failed"

        num_failed_assets = asset_processor_utils.get_num_failed_processed_assets(output)
        assert num_failed_assets is 0, 'Wrong number of failed assets'

        # Checking if number of jobs were three after reprocessing a slice:
        #    6 jobs for three slices (all slices have 2 processing jobs),
        #    2 jobs for 2 png textures (See Test Case Test Assets Folder for Asset Names),
        #    There are possible asset updates we should allow such as bootstrap.cfg for the branch token
        #    Q - But what if <many extra assets are processed question>
        #    A - Add a test specific for that case.  Restrictions on "exactly this number of assets and no more
        #    were processed" are brittle and force engineers to go figure out whether they actually broke something
        #    or an expected behavior has changed.  Processing bootstrap.cfg sometimes but not other times should not
        #    cause a failure in this test.
        num_processed_assets = asset_processor_utils.get_num_processed_assets(output)
        assert num_processed_assets >= 8, f'Wrong number of successfully processed assets found in output: '\
                                          '{num_processed_assets}'

        missing_assets, _ = asset_processor.compare_assets_with_cache()
        assert not missing_assets, 'Following assets are missing in cache: {}'.format(missing_assets)


    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    @pytest.mark.test_case_id('C1612448')
    @pytest.mark.skip(
        reason="Race condition on AP batch shutdown can cause the failure to not yet be registered even though it's "
               "recognized as failing in the logs.  There appears to be a window where the AutoFailJob doesn't complete"
               "before the shutdown completes and the failure doesn't end up counting")
    def test_AddTwoTexturesWithSameName_ShouldProcessAfterRename(self, asset_processor, ap_setup_fixture):
        # Feed two different textures with same name (but different extensions) - ap will fail
        # Rename one of textures and failure should go away

        # Copying test assets to project folder and deleting them from cache to make sure APBatch will process them
        asset_processor.prepare_test_environment(ap_setup_fixture["tests_dir"], "test_AddTwoTexturesWithSameName_ShouldProcessAfterRename")

        # Launching AP, making sure it is failing
        result, output = asset_processor.batch_process(expect_failure=True, capture_output=True)
        assert result == False, f'AssetProcessorBatch should have failed because there are textures that are generating same output product, output was {output}'

        # Renaming original files so they won't collide in cache after second processing
        files_to_rename = os.listdir(asset_processor.project_test_source_folder())
        logger.info(f"Renaming files: {files_to_rename}")
        utils.append_to_filename(files_to_rename[0], asset_processor.project_test_source_folder(), '_', False)
        utils.append_to_filename(files_to_rename[1], asset_processor.project_test_source_folder(), '__', False)
        logger.info(f"Files renamed to: {os.listdir(asset_processor.project_test_source_folder())}")

        # Reprocessing files and making sure there are no failed jobs
        result, output = asset_processor.batch_process(capture_output=True)
        assert result, "AssetProcessorBatch failed when it should have succeeded after renaming textures"
        missing_assets, _ = asset_processor.compare_assets_with_cache()
        assert not missing_assets, 'Following assets were not found in cache {}'.format(missing_assets)

        num_failed_assets = asset_processor_utils.get_num_failed_processed_assets(output)
        assert num_failed_assets is 0, 'Wrong number of failed assets'

        num_processed_assets = asset_processor_utils.get_num_processed_assets(output)
        expected_asset_count = 2
        assert num_processed_assets >= expected_asset_count, f'Wrong number of successfully processed assets found in output: '\
                                          f'expected at least {expected_asset_count}, but only {num_processed_assets} were processed'


    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    def test_InvalidServerAddress_Warning_Logs(self, asset_processor):

        asset_processor.create_temp_asset_root()
        # Launching AP and making sure that the warning exists
        result, output = asset_processor.batch_process(capture_output=True, extra_params=["/serverAddress=InvalidAddress", "/server"])
        assert result, "AssetProcessorBatch failed when it should have had a warning and no failure"

        assert asset_processor_utils.has_invalid_server_address(output), 'Invalid server address warning not present.'


    @pytest.mark.test_case_id("C1571774")
    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    def test_AllSupportedPlatforms_IncludeValidAssets_AssetsProcessed(self, asset_processor, ap_setup_fixture):
        """
        AssetProcessorBatch is successfully processing newly added assets
        """
        env = ap_setup_fixture

        # Prepare test assets and start Asset Processor Batch
        asset_processor.prepare_test_environment(env["tests_dir"], "C1571774")
        result, output_list = asset_processor.batch_process(capture_output=True)
        assert result, f"AP Batch failed with output: {output_list}"

        # Determine if any assets are missing from the cache
        #     _ is a tuple catch for expected_assets but is not used in this test but is available in scenarios that
        #     a test would need to check the expected_assets assets in the cache
        missing_assets, found_assets = asset_processor.compare_assets_with_cache()

        if missing_assets:
            logger.info(f"Missing asset Failure log was {output_list}")
        assert not missing_assets, f"Following assets were not found in cache: {missing_assets} found was {found_assets}"

    @pytest.mark.test_case_id("C1568831")
    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    def test_AllSupportedPlatforms_DeletedAssets_DeletedFromCache(self, asset_processor, ap_setup_fixture):
        """
        AssetProcessor successfully deletes cached items when removed from project
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
        """
        env = ap_setup_fixture

        # Add assets to test asset directory
        test_assets_folder, cache_folder = asset_processor.prepare_test_environment(env["tests_dir"], "C1591338")

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
        """
        asset_processor.create_temp_asset_root()
        LOG_PATH = {
            "batch_log": workspace.paths.ap_batch_log(),
            "gui_log": workspace.paths.ap_gui_log(),
            "job_logs": workspace.paths.ap_job_logs(),
        }

        class LogTimes:
            batch_log_start_time = 0
            gui_log_start_time = 0
            job_logs_start_time = 0

            batch_log_final_time = 0
            gui_log_final_time = 0
            job_logs_final_time = 0

            @staticmethod
            def Report():
                print(
                    f"""
                    Original Times:
                    Batch: {LogTimes.batch_log_start_time}
                    GUI: {LogTimes.gui_log_start_time}
                    JobLogs:{LogTimes.job_logs_start_time}

                    Post-Run Times:
                    Batch: {LogTimes.batch_log_final_time}
                    GUI: {LogTimes.gui_log_final_time}
                    JobLogs:{LogTimes.job_logs_final_time}
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
            assert os.path.exists(path), f"{name} could not be located after running the AP."

        # Check if log files previously exist and grab their modification times
        update_times("_start_time")

        # Run the Batch process
        assert asset_processor.batch_process(), "Batch process failed to successfully terminate"

        asset_processor.gui_process(quitonidle=True)

        # Check that the Logs directory exists (C1564055)
        check_existence("Logs Directory", workspace.paths.ap_log_dir())

        # Check that the logs and JobLogs directory have updated modified times (C1564056)
        for key in LOG_PATH.keys():
            check_existence(key, LOG_PATH[key])

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
    def test_AllSupportedPlatforms_CorruptSlice_BatchReportsProcessingFail(self, asset_processor, ap_setup_fixture):
        # fmt:on
        """
        Utilizing corrupted test assets, run the batch process to verify the
        AP logs the failure to process the corrupted file.
        """
        env = ap_setup_fixture
        error_line_found = False

        # Import pre-corrupted test assets
        asset_processor.prepare_test_environment(env["tests_dir"], "C1564073")
        success, output = asset_processor.batch_process(capture_output=True, expect_failure=True)
        log = APOutputParser(output)
        for _ in log.get_lines(-1, ["Createjobs Failed", "test_slice.slice"]):
            error_line_found = True

        assert error_line_found, "The error could not be found in the newest run of the AP Batch log."

    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    def test_validateDirectPreloadDependency_Found(self, asset_processor, ap_setup_fixture, workspace):
        env = ap_setup_fixture
        error_line_found = False

        asset_processor.prepare_test_environment(env["tests_dir"], "assets_with_direct_preload_dependency")

        success, output = asset_processor.batch_process(capture_output=True, expect_failure=False)
        log = APOutputParser(output)
        for _ in log.get_lines(-1, ["Preload circular dependency detected", "testc.dynamicslice"]):
            error_line_found = True

        assert error_line_found, "The error could not be found in the newest run of the AP Batch log."

    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    def test_validateNestedPreloadDependency_Found(self, asset_processor, ap_setup_fixture, workspace):

        env = ap_setup_fixture
        error_line_found = False

        asset_processor.prepare_test_environment(env["tests_dir"], "assets_with_nested_preload_dependency")

        success, output = asset_processor.batch_process(capture_output=True, expect_failure=False)
        log = APOutputParser(output)
        for _ in log.get_lines(-1, ["Preload circular dependency detected", "testa.dynamicslice"]):
            error_line_found = True

        assert error_line_found, "The error could not be found in the newest run of the AP Batch log."

