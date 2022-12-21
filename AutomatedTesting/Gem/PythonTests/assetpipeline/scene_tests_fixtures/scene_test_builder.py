"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import json
import pytest
from typing import List
from dataclasses import dataclass

from _pytest.mark import ParameterSet

from automatedtesting_shared import asset_database_utils as asset_db_utils


@dataclass
class BlackboxAssetTest:

    test_name: str
    asset_folder: str
    override_asset_folder: str = ""
    scene_debug_file: str = ""
    override_scene_debug_file: str = ""
    assets: List[asset_db_utils.DBSourceAsset] = ()
    override_assets: List[asset_db_utils.DBSourceAsset] = ()


def populate_product_list_from_json(data_products: {iter}) -> list[asset_db_utils.DBProduct]:
    """
    Helper function used by the test_builder.py module to populate an expected product assets list from a json file.
    :param data_products: The "product" key from the target json dict.
    :return: list_products: A list of products.
    """

    list_products = []
    for product in data_products:
        list_products.append(
            asset_db_utils.DBProduct(
                product_name=product["product_name"],
                sub_id=product["sub_id"],
                asset_type=bytes(product["asset_type"], 'utf-8')
            )
        )
    return list_products


def build_test(scenetest_file: object) -> BlackboxAssetTest:
    """
    A function that reads the contents of a scenetest json file to create a test.
    Returns the test parameters and data as an instance of the BlackboxAssetTest dataclass.
    Scene Test json template example at '\assetpipeline\scene_tests\tests\template.json'
    :param scenetest_file: A json formatted file that provides the data required to drive a test.
    :return: Test data to be parametrized using pytest.param() from within the test runner module in
    '\assetpipeline\scene_tests\scene_tests.py'.
    """

    # Open the json file and load it into a variable.
    with open(scenetest_file) as file:
        data = json.load(file)

    # Create an instance of the BlackboxAssetTest dataclass and populate the parameters with data from the json file.
    TestParams = BlackboxAssetTest(
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

    return TestParams


def parametrize_blackbox_scene_test(dir_test_path: str) -> tuple[list[ParameterSet], list]:
    """
    Summary:
    Helper function that parametrizes tests based on json files found within the test directory and all sub-folders.

    :returns:
    blackbox_scene_tests: List of parametrized tests
    blackbox_test_ids: Name of tests to be used as IDs during parametrization
    """
    blackbox_scene_tests = []
    blackbox_test_ids = []

    for root, dirs, files in os.walk(dir_test_path):
        file_list = [file for file in files if file.endswith(".scenetest")]
        for test in file_list:
            test_path = os.path.join(root, os.path.relpath(test))
            blackbox_scene_tests.append(
                pytest.param(
                    build_test(test_path)
                )
            )
            blackbox_test_ids.append(test.split(".")[0])
    return blackbox_scene_tests, blackbox_test_ids
