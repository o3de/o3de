"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import binascii
from dataclasses import dataclass, field
import logging
import sqlite3
import os
from typing import List

import ly_test_tools.o3de.pipeline_utils as pipeline_utils

# Index for ProductID in Products table in DB
PRODUCT_ID_INDEX = 0

logger = logging.getLogger(__name__)

def do_select(asset_db_path, cmd):
    try:
        connection = sqlite3.connect(asset_db_path)
        # Get ProductID from database
        db_rows = connection.execute(cmd)
        return_result = db_rows.fetchall()
        connection.close()
        return return_result
    except sqlite3.Error as sqlite_error:
        print(f'select on db {asset_db_path} failed with exception {sqlite_error}')
        return []


def get_active_platforms_from_db(asset_db_path) -> List[str]:
    """Returns a list of platforms that are active in the database, based on what jobs were run"""
    platform_rows = do_select(asset_db_path, f"select distinct Platform from Jobs")
    # Condense this into a single list of platforms.
    platforms = [platform[0] for platform in platform_rows]
    return platforms


# Convert a source product path into a db product path
# cache_platform/projectname/product_path
def get_db_product_path(workspace, source_path, cache_platform):
    product_path = os.path.join(cache_platform, source_path)
    product_path = product_path.replace('\\', '/')
    return product_path


def get_product_id(asset_db_path, product_name) -> str:
    # Get ProductID from database
    product_id = list(do_select(asset_db_path, f"SELECT ProductID FROM Products where ProductName='{product_name}'"))
    if len(product_id) == 0:
        return product_id  # return empty list
    return product_id[0][PRODUCT_ID_INDEX]  # Get product id from 'first' row


# Retrieve a product_id given a source_path assuming the source is copied into the cache with the same
# name or a product name without cache_platform or projectname prepended
def get_product_id_from_relative(workspace, source_path, asset_platform):
    return get_product_id(workspace.paths.asset_db(), get_db_product_path(workspace, source_path, asset_platform))


def get_missing_dependencies(asset_db_path, product_id) -> List[str]:
    return list(do_select(asset_db_path, f"SELECT * FROM MissingProductDependencies where ProductPK={product_id}"))


def do_single_transaction(asset_db_path, cmd):
    try:
        connection = sqlite3.connect(asset_db_path)
        cursor = connection.cursor()  # SQL cursor used for issuing commands
        cursor.execute(cmd)
        connection.commit()  # Save changes
        connection.close()
    except sqlite3.Error as sqlite_error:
        print(f'transaction on db {asset_db_path} cmd {cmd} failed with exception {sqlite_error}')


def clear_missing_dependencies(asset_db_path, product_id) -> None:
    do_single_transaction(asset_db_path, f"DELETE FROM MissingProductDependencies where ProductPK={product_id}")


def clear_all_missing_dependencies(asset_db_path) -> None:
    do_single_transaction(asset_db_path, "DELETE FROM MissingProductDependencies;")


@dataclass
class DBProduct:
    product_name: str = None
    sub_id: int = 0
    asset_type: str = None


@dataclass
class DBJob:
    job_key: str = None
    builder_guid: str = None
    status: int = 0
    error_count: int = 0
    warning_count: int = 0
    platform: str = None
    # Key: Product ID
    products: List[DBProduct] = field(default_factory=list)


@dataclass
class DBSourceAsset:
    source_file_name: str = None
    uuid: str = None
    scan_folder_key: str = field(compare=False, default=None)
    id: str = field(compare=False, default=None)
    # Key: Job ID
    jobs: List[DBJob] = field(default_factory=list)


def compare_expected_asset_to_actual_asset(asset_db_path, expected_asset: DBSourceAsset, asset_path, cache_root):
    actual_asset = get_db_source_job_product_info(asset_db_path, asset_path, cache_root)

    assert expected_asset.uuid == actual_asset.uuid, \
        f"UUID for asset {expected_asset.source_file_name} is '{actual_asset.uuid}', but expected '{expected_asset.uuid}'"

    for expected_job in expected_asset.jobs:
        for actual_job in actual_asset.jobs:
            found_job = False
            if expected_job.job_key == actual_job.job_key and expected_job.platform == actual_job.platform:
                found_job = True

                assert expected_job == actual_job, \
                    f"Expected job did not match actual job for key {expected_job.job_key} and platform {expected_job.platform}.\nExpected: {expected_job}\nActual: {actual_job}"

                # Remove the found job to speed up other searches.
                actual_asset.jobs.remove(actual_job)
                break

            assert found_job, f"For asset {expected_asset.source_file_name}, could not find job with key '{expected_job.job_key}' and platform '{expected_job.platform}'"
            # Don't assert on any actual jobs remaining, they could be for platforms not checked by this test, or new job keys not yet handled.


def get_db_source_job_product_info(asset_db_path, filename, cache_root):
    source_db = get_source_info_from_filename(asset_db_path, filename)

    source = DBSourceAsset()
    source.source_file_name = filename
    source.id = source_db[0]
    source.scan_folder_key = source_db[1]
    source.uuid = binascii.hexlify(source_db[2])

    jobs_db = get_jobs_for_source(asset_db_path, source.id)
    for job_db in jobs_db:
        job_id = job_db[0]

        job = DBJob()
        job.job_key = job_db[1]
        job.platform = job_db[3]
        job.builder_guid = binascii.hexlify(job_db[4])
        job.status = job_db[5]
        job.error_count = job_db[6]
        job.warning_count = job_db[7]

        products_db = get_products_for_job(asset_db_path, job_id)

        for product_db in products_db:
            product = DBProduct()
            product.product_name = product_db[1]
            product.sub_id = product_db[2]
            product.asset_type = binascii.hexlify(product_db[3])

            job.products.append(product)
        source.jobs.append(job)
    return source


def get_source_info_from_filename(asset_db_path, filename):
    sources = do_select(asset_db_path,
        f"SELECT SourceID,ScanFolderPK,SourceGuid FROM Sources where SourceName=\"{filename}\"")
    assert len(sources) == 1, f"Zero or multiple source assets found when only one was expected for '{filename}'"
    return sources[0]


def get_jobs_for_source(asset_db_path, source_id):
    return do_select(asset_db_path,
        f"SELECT JobID,JobKey,Fingerprint,Platform,BuilderGuid,Status,ErrorCount,WarningCount FROM Jobs where SourcePK={source_id} order by JobKey")


def get_products_for_job(asset_db_path, job_id):
    return do_select(asset_db_path,
        f"SELECT ProductID,ProductName,SubID,AssetType FROM Products where JobPK={job_id} order by ProductName")


