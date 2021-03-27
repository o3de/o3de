"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

import sqlite3
import os
from typing import List

# Index for ProductID in Products table in DB
PRODUCT_ID_INDEX = 0


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
    product_path = os.path.join(cache_platform, workspace.project, source_path)
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
