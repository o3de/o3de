"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

General Asset Processor GUI Tests
"""

# Import builtin libraries
import psutil
import pytest
import logging
import os
import time
import configparser
from pathlib import Path

# Import LyTestTools
import ly_test_tools
import ly_test_tools.builtin.helpers as helpers
import ly_test_tools.environment.waiter as waiter
import ly_test_tools.environment.file_system as fs
import ly_test_tools.environment.process_utils as process_utils
import ly_test_tools.launchers.launcher_helper as launcher_helper
from ly_test_tools.o3de.asset_processor import ASSET_PROCESSOR_PLATFORM_MAP, ASSET_PROCESSOR_SETTINGS_ROOT_KEY

# Import fixtures
from ..ap_fixtures.ap_fast_scan_setting_backup_fixture import ap_fast_scan_setting_backup_fixture
from ..ap_fixtures.asset_processor_fixture import asset_processor as asset_processor
from ..ap_fixtures.ap_setup_fixture import ap_setup_fixture as ap_setup_fixture
from ..ap_fixtures.ap_idle_fixture import TimestampChecker


# Import LyShared
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


@pytest.fixture
def ap_idle(workspace, ap_setup_fixture):
    gui_timeout_max = 30
    return TimestampChecker(workspace.paths.asset_catalog(ASSET_PROCESSOR_PLATFORM_MAP[workspace.asset_processor_platform]), gui_timeout_max)



@pytest.mark.usefixtures("asset_processor")
@pytest.mark.usefixtures("ap_setup_fixture")
@pytest.mark.usefixtures("local_resources")
@pytest.mark.parametrize("project", targetProjects)
@pytest.mark.assetpipeline
@pytest.mark.SUITE_periodic
class TestsAssetProcessorGUI_WindowsAndMac(object):
    """
    Specific Tests for Asset Processor GUI To Only Run on Windows and Mac
    """

    @pytest.mark.skip(reason="https://github.com/o3de/o3de/issues/15101")
    @pytest.mark.test_case_id("C3540434")
    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    def test_WindowsAndMacPlatforms_GUIFastScanNoSettingSet_FastScanSettingCreated(self, asset_processor, ap_fast_scan_setting_backup_fixture):
        """
         Tests that a fast scan settings entry gets created for the AP if it does not exist
         and ensures that the entry is defaulted to fast-scan enabled

         Test Steps:
         1. Create temporary testing environment
         2. Delete existing fast scan setting if exists
         3. Run Asset Processor GUI without setting FastScan setting (default:true) and without quitonidle
         4. Wait and check to see if Windows Registry fast scan setting is created
         5. Verify that Fast Scan setting is set to true
        """

        asset_processor.create_temp_asset_root()
        fast_scan_setting = ap_fast_scan_setting_backup_fixture

        # Delete registry value (if it exists)
        fast_scan_setting.delete_entry()

        # Make sure registry was deleted
        assert not fast_scan_setting.entry_exists(), "Registry Key's Value was not deleted"

        # AssetProcessor registry value should be set to true once AssetProcessor.exe has been executed
        result, _ = asset_processor.gui_process(quitonidle=False)
        assert result, "AP GUI failed"

        # Wait for AP GUI long enough to create the registry entry before killing AP
        waiter.wait_for(
            lambda: fast_scan_setting.entry_exists(),
            timeout=20,
            exc=Exception("AP failed to create a fast scan setting in 20 seconds"),
        )

        # Ensure that the DEFAULT value for fast scan is ENABLED
        key_value = fast_scan_setting.get_value()
        asset_processor.stop()

        assert key_value.lower() == "true", f"The fast scan setting found was {key_value}"

    @pytest.mark.SUITE_sandbox
    @pytest.mark.test_case_id("C3635822")
    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    def test_WindowsMacPlatforms_GUIFastScanEnabled_GameLauncherWorksWithAP(self, asset_processor, workspace,
                                                                            ap_fast_scan_setting_backup_fixture):

        """
        Make sure game launcher working with Asset Processor set to turbo mode
        Validate that no fatal errors (crashes) are reported within a certain
        time frame for the AP and the GameLauncher

        Test Steps:
        1. Create temporary testing environment
        2. Set fast scan to true
        3. Verify fast scan is set to true
        4. Launch game launcher
        5. Verify launcher has launched without error
        6. Verify that asset processor has launched
        """
        CHECK_ALIVE_SECONDS = 15

        # AP is running in turbo mode
        fast_scan = ap_fast_scan_setting_backup_fixture
        fast_scan.set_value("True")

        value = fast_scan.get_value()
        assert value.lower() == "true", f"The fast scan setting found is {value}"

        # Launch GameLauncher.exe with Null Renderer enabled so that Non-GPU Automation Nodes don't fail on the renderer
        launcher = launcher_helper.create_game_launcher(workspace, args=["-NullRenderer"])
        launcher.start()

        # Validate that no fatal errors (crashes) are reported within a certain time frame (10 seconds timeout)
        #   This applies to AP and GameLauncher.exe
        time.sleep(CHECK_ALIVE_SECONDS)
        launcher_name = f"{workspace.project.title()}.GameLauncher"

        launcher_exists = False
        asset_processor_exists = False
        if process_utils.process_exists(launcher_name, ignore_extensions=True):
            launcher_exists = True
        if process_utils.process_exists("AssetProcessor", ignore_extensions=True):
            asset_processor_exists = True

        launcher.stop()

        assert launcher_exists, f"{launcher_name} was not live during the check."
        assert asset_processor_exists, "AssetProcessor was not live during the check."


@pytest.mark.usefixtures("asset_processor")
@pytest.mark.usefixtures("ap_setup_fixture")
@pytest.mark.usefixtures("local_resources")
@pytest.mark.parametrize("project", targetProjects)
@pytest.mark.assetpipeline
@pytest.mark.SUITE_periodic
class TestsAssetProcessorGUI_AllPlatforms(object):
    """
    Tests for Asset Processor GUI To Run on All Supported Host Platforms
    """

    @pytest.mark.test_case_id("C4874115")
    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    def test_AllSupportedPlatforms_AddScanFolder_AssetsProcessed(
        self, ap_setup_fixture, asset_processor, workspace, request
    ):
        """
        Process files from the additional scanfolder

        Test Steps:
        1. Create temporary testing environment
        2. Run asset processor batch
        3. Validate that product assets were generated in the cache
        4. Create an additional scan folder with assets
        5. Create additional scan folder params to pass to Asset Processor
        6. Run Asset Processor GUI with QuitOnIdle and pass in params for the additional scan folder settings
        7. Verify additional product assets from additional scan folder are present in the cache
        """
        env = ap_setup_fixture
        # Copy test assets to new folder in dev folder
        # This new folder will be created outside the default project folder and will not be added as a scan folder
        test_assets_folder, cache_folder = asset_processor.prepare_test_environment(env["tests_dir"], "C4874115",
                                                                                    relative_asset_root='',
                                                                                    add_scan_folder=False)
        # The AssetProcessor internal _cache_folder path is updated to point at the cache root in order
        # to allow the comparison between the source asset path of "<source asset path>/C4874115" to match the cache assets
        # in <cache-folder>
        asset_processor._cache_folder = os.path.dirname(cache_folder)
        assert os.path.exists(test_assets_folder), f"Test assets folder was not found {test_assets_folder}"

        # Run AP Batch
        assert asset_processor.batch_process(), "AP Batch failed"

        # Check whether assets currently exist in the cache
        def test_assets_added_to_cache():
            missing_assets, _ = asset_processor.compare_assets_with_cache()
            return not missing_assets

        assert not test_assets_added_to_cache(), "Test assets are present in cache before adding scan folder"

        # Supply an additional scan folder for the test via the settings registry regset parameters
        test_scan_folder_params = []

        test_scan_folder_root_key = f"{ASSET_PROCESSOR_SETTINGS_ROOT_KEY}/ScanFolder C4874115"
        test_scan_folder_params.append(f'--regset="{test_scan_folder_root_key}/watch=@ROOT@/C4874115"')
        test_scan_folder_params.append(f'--regset="{test_scan_folder_root_key}/recursive=1"')
        test_scan_folder_params.append(f'--regset="{test_scan_folder_root_key}/order=5000"')

        # Run AP GUI and read the config file we just modified to pick up our scan folder
        # Pass in a pattern so we don't spend time processing unrelated folders
        result, _ = asset_processor.gui_process(add_config_scan_folders=True,
                                                scan_folder_pattern="*C4874115*",
                                                extra_params=test_scan_folder_params)
        assert result, "AP GUI failed"

        # Verify test assets processed into cache after adding scan folder
        assert test_assets_added_to_cache(), "Test assets are missing from cache after adding scan folder"

    @pytest.mark.test_case_id("C4874114")
    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    def test_AllSupportedPlatforms_InvalidAddress_AssetsProcessed(self, workspace, request, asset_processor):
        """
        Launch AP with invalid address in bootstrap.cfg
        Assets should process regardless of the new address

        Test Steps:
        1. Create a temporary testing environment
        2. Set an invalid ip address in Asset Processor settings file
        3. Launch Asset Processor GUI
        4. Verify that it processes assets and exits cleanly even though it has an invalid IP.
        """
        test_ip_address = "1.1.1.1"  # an IP address without Asset Processor

        asset_processor.create_temp_asset_root()
        # Set the remote_ip setting through the settings registry to an IP address without Asset Processor
        extra_params = []
        extra_params.append(f'--regset="/Amazon/AzCore/Bootstrap/remote_ip={test_ip_address}"')

        # Run AP Gui to verify that assets process regardless of the new address
        result, _ = asset_processor.gui_process()
        assert result, "AP GUI failed"

    @pytest.mark.test_case_id("C24168802")
    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    def test_AllSupportedPlatforms_ModifyAssetInfo_AssetsReprocessed(self, ap_setup_fixture, asset_processor):
        """
        Modifying assetinfo files triggers file reprocessing

        Test Steps:
        1. Create temporary testing environment with test assets
        2. Run Asset Processor GUI
        3. Verify that Asset Processor exited cleanly and product assets are in the cache
        4. Modify the .assetinfo file by adding a newline
        5. Wait for Asset Processor to go idle
        6. Verify that product files were regenerated (Time Stamp compare)
        """
        env = ap_setup_fixture

        # Expected test asset sources and products
        # *.assetinfo and *.fbx files are not produced in cache, and file.fbx produces file.actor in cache
        expected_test_assets = ["Jack.fbx", "Jack.fbx.assetinfo"]
        expected_cache_assets = ["jack.actor"]

        # Copy test assets to project folder and verify test assets folder exists
        test_assets_folder, cache_folder = asset_processor.prepare_test_environment(env["tests_dir"], "C24168802")
        assert os.path.exists(test_assets_folder), f"Test assets folder was not found {test_assets_folder}"

        # Collect test asset relative paths and verify original test assets
        test_assets_list = utils.get_relative_file_paths(test_assets_folder)
        assert utils.compare_lists(test_assets_list, expected_test_assets), "Test assets are not as expected"

        # Run AP Gui
        result, _ = asset_processor.gui_process(run_until_idle=True, extra_params="--regset=\"/O3DE/SceneAPI/AssetImporter/SkipAtomOutput=true\"")
        assert result, "AP GUI failed"

        # Verify test assets in cache folder
        cache_assets_list = utils.get_relative_file_paths(cache_folder)

        assert utils.compare_lists(cache_assets_list, expected_cache_assets), \
            "One or more assets is missing between project folder and cache folder"

        # Grab timestamps of cached assets before edit of *.assetinfo file in project folder
        timestamps = [os.stat(os.path.join(cache_folder, asset)).st_mtime for asset in cache_assets_list]

        # Append new line to contents of *.assetinfo file in project folder
        asset_info_list = [item for item in test_assets_list if Path(item).suffix == '.assetinfo']
        assetinfo_paths_list = [os.path.join(test_assets_folder, item) for item in asset_info_list]
        for assetinfo_path in assetinfo_paths_list:
            fs.unlock_file(assetinfo_path)
            with open(assetinfo_path, "a") as project_asset_file:
                project_asset_file.write("\n")

        asset_processor.next_idle()
        asset_processor.stop()

        # Verify new timestamp for *.actor file in cache folder after edit of *.assetinfo file in project folder
        for asset in cache_assets_list:
            if ".actor" in asset:

                assert os.stat(os.path.join(cache_folder, asset)).st_mtime not in timestamps, \
                    f"Cached {asset} was not updated"
