"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import pytest
from ly_test_tools.o3de.asset_processor import ASSET_PROCESSOR_PLATFORM_MAP

from assetpipeline.ap_fixtures.asset_processor_fixture import asset_processor as asset_processor
from assetpipeline.ap_fixtures.ap_setup_fixture import ap_setup_fixture as ap_setup_fixture
from assetpipeline.ap_fixtures.ap_config_backup_fixture import ap_config_backup_fixture as ap_config_backup_fixture
from assetpipeline.ap_fixtures.ap_config_default_platform_fixture \
    import ap_config_default_platform_fixture as ap_config_default_platform_fixture

from automatedtesting_shared import asset_database_utils as asset_db_utils
import assetpipeline.ap_fixtures.test_builder as test_builder

targetProjects = ["AutomatedTesting"]


@pytest.fixture
def local_resources(request, workspace, ap_setup_fixture):
    ap_setup_fixture["tests_dir"] = os.path.dirname(os.path.realpath(__file__))


blackbox_scene_tests = []
blackbox_test_ids = []

def parametrize_blackbox_scene_test():
    """
    Summary:
    Helper function that parametrizes tests based on json files found within the test directory and all sub-folders.

    :returns:
    blackbox_scene_tests: List of parametrized tests
    blackbox_test_ids: Name of tests to be used as IDs during parametrization
    """
    dir_test_path = os.path.join(os.path.dirname(os.path.realpath(__file__)), "tests")
    for root, dirs, files in os.walk(dir_test_path):
        file_list = [file for file in files if file.endswith(".json")]
        for test in file_list:
            test_path = os.path.join(root, os.path.relpath(test))
            blackbox_scene_tests.append(
                pytest.param(
                    test_builder.build_test(test_path)
                )
            )
            blackbox_test_ids.append(test.split(".")[0])
    return blackbox_scene_tests, blackbox_test_ids


parametrize_blackbox_scene_test()

@pytest.mark.usefixtures("asset_processor")
@pytest.mark.usefixtures("ap_setup_fixture")
@pytest.mark.usefixtures("local_resources")
@pytest.mark.parametrize("project", targetProjects)
@pytest.mark.assetpipeline
@pytest.mark.SUITE_sandbox
class TestScene_AllPlatforms(object):

    @pytest.mark.BAT
    @pytest.mark.parametrize("blackbox_param", blackbox_scene_tests, ids=blackbox_test_ids)
    def test_SceneBlackboxTest_SourceFiles_Processed_ResultInExpectedProducts(self, workspace, ap_setup_fixture,
                                                                              asset_processor, project, blackbox_param):
        if blackbox_param is None:
            return
        self.run_scene_test(workspace, ap_setup_fixture, asset_processor, project, blackbox_param)

        override_assets = blackbox_param.override_assets
        if override_assets is not None:
            # If override_assets is set, run the test again and validate the information in the override assets
            self.run_scene_test(workspace, ap_setup_fixture,
                                asset_processor, project, blackbox_param, True)

    @staticmethod
    def populate_asset_info(workspace, assets):

        # Check that each given source asset resulted in the expected jobs and products.
        for expected_source in assets:
            for job in expected_source.jobs:
                job.platform = ASSET_PROCESSOR_PLATFORM_MAP[workspace.asset_processor_platform]
                for product in job.products:
                    product.product_name = job.platform + "/" + product.product_name

    @staticmethod
    def compare_lists(asset_processor, expected_debug, actual_debug):
        # Set dbgsg file paths.
        expected_dbgsg_path = os.path.join(asset_processor.project_test_source_folder(), expected_debug)
        actual_dbgsg_path = os.path.join(asset_processor.project_test_cache_folder(), actual_debug)

        expected_lines = []

        def populate_expected_lines():
            with open(expected_dbgsg_path, "r") as scene_file:
                for line in scene_file:
                    expected_lines.append(line)

        def compare_actual_with_expected_lines():
            with open(actual_dbgsg_path, "r") as scene_file:
                for line in scene_file:
                    if len(expected_lines) > 0:
                        if line in expected_lines:
                            expected_lines.remove(line)
                    else:
                        break

        populate_expected_lines()
        compare_actual_with_expected_lines()
        assert len(expected_lines) == 0, \
            f"The following nodes were expected within the scene file but not found: {expected_lines}"

    # Helper to run Asset Processor with debug output enabled and Atom output disabled
    @staticmethod
    def run_ap_debug_skip_atom_output(asset_processor):
        result = asset_processor.batch_process(extra_params=["--debugOutput",
                                                             "--regset=\"/O3DE/SceneAPI/AssetImporter/SkipAtomOutput=true\""])
        assert result, "Asset Processor Failed"

    def run_scene_test(self, workspace, ap_setup_fixture, asset_processor,
                       project, blackbox_params: "BlackboxAssetTest", overrideAsset=False):
        """
        These tests work by having the test case ingest the test data and determine the run pattern.
        Tests will process scene settings files and will additionally do a verification against a provided debug file
        Additionally, if an override is passed, the output is checked against the override.

        Test Steps:
        1. Create temporary test environment
        2. Process Assets
        3. Determine what assets to validate based upon test data
        4. Validate assets were created in cache
        5. If debug file provided, verify scene files were generated correctly
        6. Verify that each given source asset resulted in the expected jobs and products
        """

        # Test Setup: Setup variables and test environment based on whether or not the test is using overrides
        if not overrideAsset:
            assets_to_validate = blackbox_params.assets
            test_assets_folder = blackbox_params.asset_folder
            expected_scene_debug_file = blackbox_params.scene_debug_file
            asset_processor.prepare_test_environment(ap_setup_fixture['tests_dir'], test_assets_folder)
        else:
            assets_to_validate = blackbox_params.override_assets
            test_assets_folder = blackbox_params.override_asset_folder
            expected_scene_debug_file = blackbox_params.override_scene_debug_file
            asset_processor.prepare_test_environment(ap_setup_fixture['tests_dir'], test_assets_folder,
                                                     use_current_root=True, add_scan_folder=False,
                                                     existing_function_name=blackbox_params.asset_folder)

        # Run AP and generate scene product outputs - Skip atom product output.
        self.run_ap_debug_skip_atom_output(asset_processor)

        # Load the asset database and cache root paths.
        db_path = os.path.join(asset_processor.temp_asset_root(), "Cache", "assetdb.sqlite")
        cache_root = os.path.dirname(os.path.join(asset_processor.temp_asset_root(), "Cache",
                                                  ASSET_PROCESSOR_PLATFORM_MAP[workspace.asset_processor_platform]))

        # Compare expected and actual debug output from .dbgsg files
        if blackbox_params.scene_debug_file:
            self.compare_lists(asset_processor, expected_scene_debug_file, blackbox_params.scene_debug_file)

        # Check that each given source asset resulted in the expected jobs and products.
        self.populate_asset_info(workspace, assets_to_validate)
        for expected_source in assets_to_validate:
            for job in expected_source.jobs:
                for product in job.products:
                    asset_db_utils.compare_expected_asset_to_actual_asset(db_path, expected_source,
                                                                          f"{blackbox_params.asset_folder}/"
                                                                          f"{expected_source.source_file_name}",
                                                                          cache_root)
