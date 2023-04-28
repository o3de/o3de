"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import logging
from os import path
from pprint import pformat
from pytest import fixture, mark

from assetpipeline.ap_fixtures.asset_processor_fixture import asset_processor
from assetpipeline.ap_fixtures.ap_setup_fixture import ap_setup_fixture

from assetpipeline.scene_tests_fixtures.scene_test_builder import BlackboxAssetTest, parametrize_scene_test
from assetpipeline.scene_tests_fixtures.scene_test_debug_compare import compare_scene_debug_file, \
    compare_scene_debug_xml_file, missing_assets
import assetpipeline.scene_tests_fixtures.scene_tests_utilities as helper

from ly_test_tools._internal.managers.workspace import AbstractWorkspaceManager

from automatedtesting_shared.asset_database_utils import compare_expected_asset_to_actual_asset

logger = logging.getLogger(__name__)
targetProjects = ["AutomatedTesting"]
test_dir_path = path.join(path.dirname(path.realpath(__file__)), "tests")
scene_tests, test_ids = parametrize_scene_test(test_dir_path)


@fixture
def local_resources(workspace: AbstractWorkspaceManager,
                    ap_setup_fixture: ap_setup_fixture) -> None:
    """Test-level asset folder. Directory contains a sub-folder for each test."""
    ap_setup_fixture["tests_dir"] = path.dirname(path.realpath(__file__))


@mark.usefixtures("asset_processor")
@mark.usefixtures("ap_setup_fixture")
@mark.usefixtures("local_resources")
@mark.parametrize("project", targetProjects)
@mark.assetpipeline
@mark.SUITE_sandbox
class TestSceneAllPlatforms(object):

    @mark.BAT
    @mark.parametrize("blackbox_param", scene_tests, ids=test_ids)
    def test_source_files_output_expected_products(self,
                                                   workspace: AbstractWorkspaceManager,
                                                   ap_setup_fixture: ap_setup_fixture,
                                                   asset_processor: asset_processor,
                                                   blackbox_param: BlackboxAssetTest) -> None:
        """
        Please see run_fbx_test(...) for details
        Test Steps:
        1. Determine if blackbox is set to none
        2. Run FBX Test
        3. Determine if override assets is set to none
        4. If not, re-run FBX test and validate the information in override assets
        """

        self.run_scene_test(workspace, ap_setup_fixture, asset_processor, blackbox_param)

        if blackbox_param.override_assets:
            # Run the test again and validate the information in the override assets
            self.run_scene_test(workspace, ap_setup_fixture,
                                asset_processor, blackbox_param, True)

    @staticmethod
    def run_ap_debug_skip_atom_output(asset_processor: asset_processor) -> None:
        """Helper: Run Asset Processor with debug output enabled and Atom output disabled."""
        result, output = asset_processor.batch_process(capture_output=True, skip_atom_output=True, debug_output=True)

        # It's helpful to have the output from asset processor in the logs, to track down failures.
        logger.debug(f"Asset Processor Output: {pformat(output)}")
        assert result, "Asset Processor Failed"

    def run_scene_test(self,
                       workspace: AbstractWorkspaceManager,
                       ap_setup_fixture: ap_setup_fixture,
                       asset_processor: asset_processor,
                       blackbox_params: "BlackboxAssetTest",
                       override_asset: bool = False) -> None:
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

        def _test_setup():
            """
            Helper function that assists with setting up test environment and variables based on whether the test is
            using asset overrides.
            """
            if not override_asset:
                self.assets_to_validate = blackbox_params.assets
                self.test_assets_folder = blackbox_params.asset_folder
                self.expected_scene_debug_file = blackbox_params.scene_debug_file
                asset_processor.prepare_test_environment(ap_setup_fixture['tests_dir'], self.test_assets_folder)
            else:
                self.assets_to_validate = blackbox_params.override_assets
                self.test_assets_folder = blackbox_params.override_asset_folder
                self.expected_scene_debug_file = blackbox_params.override_scene_debug_file
                asset_processor.prepare_test_environment(ap_setup_fixture['tests_dir'], self.test_assets_folder,
                                                         use_current_root=True, add_scan_folder=False,
                                                         existing_function_name=blackbox_params.asset_folder)
            self.expected_product_list = helper.populate_expected_product_assets(workspace, self.assets_to_validate)
            self.cache_root = asset_processor.temp_project_cache_path()
            self.asset_folder_in_cache = asset_processor.project_test_cache_folder()
            self.db_path = path.join(self.cache_root, "assetdb.sqlite")
            self.actual_scene_debug_file_path = path.join(self.asset_folder_in_cache, blackbox_params.scene_debug_file)
            self.expected_scene_debug_file_path = path.join(asset_processor.project_test_source_folder(), "SceneDebug",
                                                            self.expected_scene_debug_file)

        logger.info("Initiating test setup.")
        _test_setup()

        logger.info(f"{blackbox_params.test_name}: Processing assets in folder '"
                    f"{self.test_assets_folder}' and verifying they match expected output.")
        self.run_ap_debug_skip_atom_output(asset_processor)

        logger.info(f"Comparing expected vs actual product assets in cache.")
        assert missing_assets(self.expected_product_list, self.asset_folder_in_cache), \
            f"Expected assets are missing from the cache."

        logger.info("Comparing expected vs actual debug output.")
        if blackbox_params.scene_debug_file:
            expected_hashes_to_skip, actual_hashes_to_skip = compare_scene_debug_file(
                self.expected_scene_debug_file_path,
                self.actual_scene_debug_file_path
            )

            # Run again for the .dbgsg.xml file
            compare_scene_debug_xml_file(
                self.expected_scene_debug_file_path + ".xml",
                self.actual_scene_debug_file_path + ".xml",
                expected_hashes_to_skip=expected_hashes_to_skip if expected_hashes_to_skip is not None else [],
                actual_hashes_to_skip=actual_hashes_to_skip if actual_hashes_to_skip is not None else []
            )
        # Check that each given source asset resulted in the expected jobs and products.
        for expected_source in self.assets_to_validate:
            compare_expected_asset_to_actual_asset(
                self.db_path,
                expected_source,
                f"{blackbox_params.asset_folder}/"
                f"{expected_source.source_file_name}",
                self.cache_root
            )
