"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

General Asset Processor GUI Tests
"""

# Import built-in libraries
import shutil
import pytest
import logging
import os
import tempfile
import hashlib

# Import LyTestTools
import ly_test_tools.builtin.helpers as helpers
import ly_test_tools.environment.waiter as waiter
import ly_test_tools.environment.file_system as fs
import ly_test_tools.environment.process_utils as process_utils
import ly_test_tools.launchers.launcher_helper as launcher_helper
from ly_test_tools.o3de.asset_processor import ASSET_PROCESSOR_PLATFORM_MAP
from ly_test_tools.o3de.asset_processor import StopReason

# Import fixtures
from ..ap_fixtures.asset_processor_fixture import asset_processor as asset_processor
from ..ap_fixtures.ap_setup_fixture import ap_setup_fixture as ap_setup_fixture
from ..ap_fixtures.ap_idle_fixture import TimestampChecker

# Use the following logging pattern to hook all test logging together:
logger = logging.getLogger(__name__)

# Helper: variables we will use for parameter values in the test:
targetProjects = ["AutomatedTesting"]

@pytest.fixture
def local_resources(request, workspace, ap_setup_fixture):
    # Test-level asset folder. Directory contains a sub-folder for each test (i.e. C1234567)
    ap_setup_fixture["tests_dir"] = os.path.dirname(os.path.realpath(__file__))

@pytest.fixture
def ap_idle(workspace, ap_setup_fixture):
    gui_timeout_max = 30
    return TimestampChecker(workspace.paths.asset_catalog(ASSET_PROCESSOR_PLATFORM_MAP[workspace.asset_processor_platform]), gui_timeout_max)

@pytest.mark.usefixtures("asset_processor")
@pytest.mark.usefixtures("ap_setup_fixture")
@pytest.mark.usefixtures("local_resources")
@pytest.mark.parametrize("project", targetProjects)
@pytest.mark.SUITE_periodic
@pytest.mark.assetpipeline
class TestsAssetProcessorGUI_AllPlatforms(object):
    """
    Tests for Asset Processor's Asset Cache Server (aka Shared Cache) feature
    """

    def get_file_hash(self, filename):
        """
        Utility function to hash the contents of a file
        """
        BUF_SIZE = 1024
        sha1 = hashlib.sha1()

        with open(filename, 'rb') as f:
            while True:
                data = f.read(BUF_SIZE)
                if not data:
                    return sha1.hexdigest()
                sha1.update(data)

    def find_archive_parts(self, path, prefix):
        """
        Finds the files that start with a certain prefix inside a path folder
        """
        result = {}
        for _, _, files in os.walk(path):
            for name in files:
                if name.startswith(prefix):
                    fingerprint = name[(len(prefix)):]
                    result[fingerprint] = name
        return result, files

    def cycle_asset_processor(self, asset_processor, cache_mode, cache_folder, cache_pattern_name):
        """
        This launches the Asset Processor with the Asset Cache Server mode either enabled or inactive
        """
        extra_parameters = [
            f' --regset="/O3DE/AssetProcessor/Settings/Server/cacheServerAddress={cache_folder}" ',
            f' --regset="/O3DE/AssetProcessor/Settings/Server/assetCacheServerMode={cache_mode}" ',
            f' --regset="/O3DE/AssetProcessor/Settings/Server/ACS {cache_pattern_name}/checkServer=true" '
        ]

        # execute in cache mode
        result, _ = asset_processor.gui_process(quitonidle=False, extra_params=extra_parameters)
        assert result, "AP GUI failed"

        # Wait for AP to have finished processing
        asset_processor.wait_for_idle()
        asset_processor.terminate()

    @pytest.mark.assetpipeline
    def test_AssetCacheServer_LocalWorkUnaffected(self, workspace, asset_processor, ap_setup_fixture):
        """
        Test to make sure that a client working on local changes is not affected by the remote server folder changes.

        Test Steps:
        1. Cycle Asset Processor - in Server Mode
        2. Process a source asset
        3. Stop Asset Processor
        5. Make a local change
        4. Cycle Asset Processor - in Client Mode
        6. The source asset is processed again, in client mode
        7. Stop Asset Processor

        Expected result(s):
        1. The client's change is now the definitive edition of the source asset
        2. The cache was updated with the local client change
        """

        # Copy test assets to project folder and verify test assets folder exists
        test_folder = 'acs_local_change'
        asset_path = ap_setup_fixture["tests_dir"]
        test_assets_folder, cache_path = asset_processor.prepare_test_environment(asset_path, test_folder)
        assert os.path.exists(test_assets_folder), f"Test asset was not copied to test folder: {test_assets_folder}"

        # Save path to test asset in project folder and path to test asset in cache
        source_asset_a = os.path.join(test_assets_folder, "test_00.png")
        source_asset_b = os.path.join(test_assets_folder, "test_01.png")
        product_asset_a = os.path.join(cache_path, "test_00.png.streamingimage")
        product_asset_b = os.path.join(cache_path, "test_01.png.streamingimage")
        asset_cache_folder = tempfile.mkdtemp()
        asset_cache_target_folder = os.path.join(asset_cache_folder, test_folder)

        # check that source files exist (sanity checks)
        assert os.path.exists(source_asset_a), "{source_asset_a} source one file does not exist"
        assert os.path.exists(source_asset_b), "{source_asset_b} source two file does not exist"

        # 1. Start Asset Processor - in Server Mode
        # 2. Process a source asset
        # 3. Stop Asset Processor
        self.cycle_asset_processor(
            asset_processor, 
            cache_mode='Server', 
            cache_folder=asset_cache_folder, 
            cache_pattern_name='Atom Image Builder')

        # check that product entries exist (sanity checks)
        assert os.path.exists(product_asset_a), "{product_asset_a} does not exist"
        assert os.path.exists(product_asset_b), "{product_asset_b} does not exist"

        # make sure the product asset archive was created
        results, files = self.find_archive_parts(
            asset_cache_target_folder,
            f'test_00_Image Compile  PNG_{ASSET_PROCESSOR_PLATFORM_MAP[workspace.asset_processor_platform]}_'
        )
        assert len(results.keys()) == 1, f"{results} should have exactly one entry; found files {files}"

        # 5. Make a local change
        # Modify contents of test asset in project folder
        file_hash_a = self.get_file_hash(source_asset_a)
        file_hash_b = self.get_file_hash(source_asset_b)
        assert not(file_hash_a == file_hash_b), "The source files should not be equal"
        shutil.copyfile(source_asset_b, source_asset_a)

        # 4. Cycle Asset Processor - in Server Mode
        # 6. The source asset is processed again, new fingerprint for local change
        # 7. Stop Asset Processor
        self.cycle_asset_processor(
            asset_processor, 
            cache_mode='Server', 
            cache_folder=asset_cache_folder, 
            cache_pattern_name='Atom Image Builder')

        # Result(s):
        # 1. The client's change is now the definitive edition of the source asset
        # 2. The cache was updated with the local client change
        results, files = self.find_archive_parts(
            asset_cache_target_folder,
            f'test_00_Image Compile  PNG_{ASSET_PROCESSOR_PLATFORM_MAP[workspace.asset_processor_platform]}_'
        )
        assert len(results.keys()) == 2, f"{results} should have exactly two entries; found files {files}"
