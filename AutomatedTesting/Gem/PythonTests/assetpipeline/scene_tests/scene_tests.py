"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import logging
import os
import re
from pprint import pformat
import pytest

import ly_test_tools.o3de.pipeline_utils as utils
from _pytest.mark import ParameterSet
from ly_test_tools._internal.managers.workspace import AbstractWorkspaceManager
from ly_test_tools.o3de.asset_processor import ASSET_PROCESSOR_PLATFORM_MAP

from assetpipeline.ap_fixtures.asset_processor_fixture import asset_processor
from assetpipeline.ap_fixtures.ap_setup_fixture import ap_setup_fixture

import assetpipeline.scene_tests_fixtures.scene_test_debug_compare as parse_and_compare
import assetpipeline.scene_tests_fixtures.scene_test_builder as test_builder
from automatedtesting_shared import asset_database_utils as asset_db_utils

logger = logging.getLogger(__name__)
targetProjects = ["AutomatedTesting"]
test_dir_path = os.path.join(os.path.dirname(os.path.realpath(__file__)), "tests")
blackbox_scene_tests, blackbox_test_ids = test_builder.parametrize_blackbox_scene_test(test_dir_path)


def get_cache_folder(asset_processor: object) -> str:
    """Helper: Gets a case correct version of the cache folder."""

    # Leave the "c" in Cache uppercase.
    return re.sub("ache[/\\\\](.*)", lambda m: m.group().lower(), asset_processor.project_test_cache_folder())


@pytest.fixture
def local_resources(request: any, workspace: AbstractWorkspaceManager, ap_setup_fixture: dict) -> None:
    """Test-level asset folder. Directory contains a subfolder for each test."""
    ap_setup_fixture["tests_dir"] = os.path.dirname(os.path.realpath(__file__))


@pytest.mark.usefixtures("asset_processor")
@pytest.mark.usefixtures("ap_setup_fixture")
@pytest.mark.usefixtures("local_resources")
@pytest.mark.parametrize("project", targetProjects)
@pytest.mark.assetpipeline
@pytest.mark.SUITE_sandbox
class TestScene_AllPlatforms(object):

    @pytest.mark.BAT
    @pytest.mark.parametrize("blackbox_param", blackbox_scene_tests, ids=blackbox_test_ids)
    def test_SceneBlackboxTest_SourceFiles_Processed_ResultInExpectedProducts(self, workspace: AbstractWorkspaceManager,
                                                                              ap_setup_fixture: dict,
                                                                              asset_processor: object, project: any,
                                                                              blackbox_param: ParameterSet) -> None:
        """
        Please see run_fbx_test(...) for details
        Test Steps:
        1. Determine if blackbox is set to none
        2. Run FBX Test
        3. Determine if override assets is set to none
        4. If not, re-run FBX test and validate the information in override assets
        """

        self.run_scene_test(workspace, ap_setup_fixture, asset_processor, project, blackbox_param)

        if blackbox_param.override_assets is not None:
            # Run the test again and validate the information in the override assets
            self.run_scene_test(workspace, ap_setup_fixture,
                                asset_processor, project, blackbox_param, True)


    @staticmethod
    def run_ap_debug_skip_atom_output(asset_processor: object) -> None:
        """Helper: Run Asset Processor with debug output enabled and Atom output disabled."""
        result, output = asset_processor.batch_process(capture_output=True, extra_params=["--debugOutput",
                                                                                          "--regset=\"/O3DE/SceneAPI/AssetImporter/SkipAtomOutput=true\""])
        # If the test fails, it's helpful to have the output from asset processor in the logs, to track the failure down.
        logger.info(f"Asset Processor Output: {pformat(output)}")
        assert result, "Asset Processor Failed"

    @staticmethod
    def check_missing_assets(expected_product_list: list, cache_folder: str) -> None:
        """Helper: Compare expected products with products in cache, report on missing products."""
        missing_assets, _ = utils.compare_assets_with_cache(expected_product_list,
                                                            cache_folder)

        # If the test is going to fail, print information to help track down the cause of failure.
        if missing_assets:
            in_cache = os.listdir(cache_folder)
            logger.info(f"The following assets were missing from cache: {pformat(missing_assets)}")
            logger.info(f"The cache {cache_folder} contains this content: {pformat(in_cache)}")

        assert not missing_assets, \
            f'The following assets were expected to be in, but not found in cache: {str(missing_assets)}'

    @staticmethod
    def populate_asset_info(workspace: AbstractWorkspaceManager, project: any, assets: iter) -> None:
        """Helper: Populates the platform info for each given source asset in the expected jobs and products."""

        for expected_source in assets:
            for expected_job in expected_source.jobs:
                expected_job.platform = ASSET_PROCESSOR_PLATFORM_MAP[workspace.asset_processor_platform]
                for expected_product in expected_job.products:
                    expected_product.product_name = expected_job.platform + "/" \
                                           + expected_product.product_name

    def run_scene_test(self, workspace: AbstractWorkspaceManager, ap_setup_fixture: dict, asset_processor: object,
                       project: any, blackbox_params: "BlackboxAssetTest", overrideAsset: bool = False) -> None:
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

        logger.info(f"{blackbox_params.test_name}: Processing assets in folder '"
                    f"{test_assets_folder}' and verifying they match expected output.")

        self.run_ap_debug_skip_atom_output(asset_processor)

        logger.info(f"Validating assets.")
        # Get a list of the expected product assets and compare it with the actual product assets in cache.
        expected_product_list = parse_and_compare.populate_expected_product_assets(assets_to_validate)
        cache_folder = get_cache_folder(asset_processor)
        self.check_missing_assets(expected_product_list, cache_folder)

        # Load the asset database and cache root paths.
        db_path = os.path.join(asset_processor.temp_asset_root(), "Cache", "assetdb.sqlite")
        cache_root = os.path.dirname(os.path.join(asset_processor.temp_asset_root(), "Cache",
                                                  ASSET_PROCESSOR_PLATFORM_MAP[workspace.asset_processor_platform]))
        actual_scene_debug_file = os.path.join(get_cache_folder(asset_processor), blackbox_params.scene_debug_file)

        # Compare expected and actual debug output from .dbgsg files
        if blackbox_params.scene_debug_file:
            expected_hashes_to_skip, actual_hashes_to_skip = parse_and_compare.compare_scene_debug_file(
                asset_processor,
                expected_scene_debug_file,
                actual_scene_debug_file
            )

            # Run again for the .dbgsg.xml file
            parse_and_compare.compare_scene_debug_file(
                asset_processor,
                expected_scene_debug_file + ".xml",
                actual_scene_debug_file + ".xml",
                expected_hashes_to_skip=expected_hashes_to_skip,
                actual_hashes_to_skip=actual_hashes_to_skip
            )
        # Check that each given source asset resulted in the expected jobs and products.
        self.populate_asset_info(workspace, project, assets_to_validate)
        for expected_source in assets_to_validate:
            for expected_job in expected_source.jobs:
                for expected_product in expected_job.products:
                    asset_db_utils.compare_expected_asset_to_actual_asset(
                        db_path,
                        expected_source,
                        f"{blackbox_params.asset_folder}/"
                        f"{expected_source.source_file_name}",
                        cache_root
                    )
