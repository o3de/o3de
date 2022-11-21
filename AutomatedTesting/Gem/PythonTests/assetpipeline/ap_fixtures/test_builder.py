"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import json
from typing import List
from dataclasses import dataclass

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


def populate_product_list_from_json(data_products):
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


def build_test(json_file):
    """
    A function that reads the contents of a json file to create a test.
    Returns the test parameters and data as an instance of the BlackboxAssetTest dataclass.
    :param json_file: A json file that provides the data required to drive a test.
    :return: Test data to be parametrized using pytest.param() from within the test runner module.
    """

    # Open the json file and load it into a variable.
    with open(json_file) as file:
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
