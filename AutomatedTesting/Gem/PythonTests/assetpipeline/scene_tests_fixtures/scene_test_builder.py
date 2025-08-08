"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from dataclasses import dataclass
from os import PathLike, path
from typing import List
import json
import pytest
from _pytest.mark import ParameterSet

from assetpipeline.scene_tests_fixtures.scene_tests_utilities import find_files_in_dir

from automatedtesting_shared import asset_database_utils as asset_db_utils


@dataclass
class BlackboxAssetTest:
    """
    Data class used in the construction and parametrization of a scene test.
    Contains the general information for a scene test.
    """
    test_name: str
    asset_folder: str
    override_asset_folder: str = ""
    scene_debug_file: str = ""
    override_scene_debug_file: str = ""
    assets: List[asset_db_utils.DBSourceAsset] = ()
    override_assets: List[asset_db_utils.DBSourceAsset] = ()


def populate_product_list_from_json(product_data: {iter}) -> List[asset_db_utils.DBProduct]:
    """
    Helper function used by the test_builder.py module to populate an expected product assets list from a json file.
    :param product_data: The "product" key from the target json dict.
    :return: product_list: A list of DBProduct assets.
    """

    product_list = []
    for product in product_data:
        product_list.append(
            asset_db_utils.DBProduct(
                product_name=product["product_name"],
                sub_id=product["sub_id"],
                asset_type=bytes(product["asset_type"], 'utf-8')
            )
        )
    return product_list


def build_test(scene_test_file: str | bytes | PathLike[str] | PathLike[bytes]) -> BlackboxAssetTest:
    """
    A function that reads the contents of a '.scenetest' json file to create a test.
    Returns the test parameters as an instance of the BlackboxAssetTest dataclass.
    Scene Test json template example available at
    'AutomatedTesting/Gem/PythonTests/assetpipeline/scene_tests/tests/template.json'
    :param scene_test_file: Path to a json formatted file that provides the data required to drive a test.
    :return: Test data to be parametrized using pytest.param() from within the test runner module in
        'AutomatedTesting/Gem/PythonTests/assetpipeline/scene_tests/scene_tests.py'.
    """

    # Open the json file and load it into a variable.
    with open(scene_test_file) as file:
        data = json.load(file)

    # Create an instance of the BlackboxAssetTest dataclass and populate the parameters with data from the json file.
    test_params = BlackboxAssetTest(
        test_name=data["test_name"],
        asset_folder=data["asset_folder"],
        override_asset_folder=data["override_asset_folder"],
        scene_debug_file=data["scene_debug_file"],
        override_scene_debug_file=data["override_scene_debug_file"],
        assets=[
            asset_db_utils.DBSourceAsset(
                source_file_name=data["assets"]["source_file_name"],
                uuid=bytes(data["assets"]["uuid"], 'utf-8'),
                jobs=[
                    asset_db_utils.DBJob(
                        job_key=data["assets"]["jobs"]["job_key"],
                        builder_guid=bytes(data["assets"]["jobs"]["builder_guid"], 'utf-8'),
                        status=data["assets"]["jobs"]["status"],
                        error_count=data["assets"]["jobs"]["error_count"],
                        products=populate_product_list_from_json(data["assets"]["jobs"]["products"])
                    )
                ]
            )
        ],
        override_assets=[
            asset_db_utils.DBSourceAsset(
                source_file_name=data["override_assets"]["source_file_name"],
                uuid=bytes(data["override_assets"]["uuid"], 'utf-8'),
                jobs=[
                    asset_db_utils.DBJob(
                        job_key=data["override_assets"]["jobs"]["job_key"],
                        builder_guid=bytes(data["override_assets"]["jobs"]["builder_guid"], 'utf-8'),
                        status=data["override_assets"]["jobs"]["status"],
                        error_count=data["override_assets"]["jobs"]["error_count"],
                        products=populate_product_list_from_json(data["override_assets"]["jobs"]["products"])
                    )
                ]
            )
        ] if data["override_assets"] is not None else None
    )

    return test_params


def parametrize_scene_test(dir_test_path: str | bytes | PathLike[str] | PathLike[bytes]) \
        -> tuple[list[ParameterSet], list]:
    """
    Parametrize tests based on .scenetest files found within the test directory and all sub-folders.
    :returns:
    scene_tests: List of parametrized tests
    test_ids: Name of tests to be used as IDs during parametrization
    """
    scene_tests = []
    test_ids = []
    test_path_list = find_files_in_dir(dir_test_path, ".scenetest")

    for path_to_test in test_path_list:
        test_name = path.basename(path_to_test.split(".")[0])
        scene_tests.append(
            pytest.param(
                build_test(path_to_test)
            )
        )
        test_ids.append(test_name)

    return scene_tests, test_ids
