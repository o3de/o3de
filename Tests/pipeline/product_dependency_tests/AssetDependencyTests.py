"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

Automated scripts for tests calling AssetProcessorBatch validating basic features.

"""

from TestFixtures import HeliosProjectFixture

import os
import pytest
import sqlite3
import time
import codecs

def AssertProductHasDependencies(engineRoot, projectName, buildInfo, dbCheckWaitTime, product, pathDependencies, assetIdDependencies):
    productNameInDB = os.path.join(buildInfo.cacheSubfolder, projectName, product)
    productNameInDB = productNameInDB.replace("\\", "/")

    sqlDatabasePath = os.path.join(engineRoot, "Cache", projectName, "assetdb.sqlite")
    print ("  * Connecting to database {}".format(sqlDatabasePath))
    sqlConnection = sqlite3.connect(sqlDatabasePath)
    productRowsList = list()
    productDbWait = dbCheckWaitTime
    while len(productRowsList) == 0 and productDbWait > 0:
        productRows = sqlConnection.execute(
            "SELECT ProductID FROM Products where ProductName='{}'".format(productNameInDB))
        productRowsList = list(productRows.fetchall())
        time.sleep(1)
        productDbWait = productDbWait - 1
    assert len(productRowsList) == 1, str.format("productRowsList= {}", productRowsList)
    productId = int(productRowsList[0][0])

    foundDependencies = list()
    dependencyDbTimeout = dbCheckWaitTime
    while len(foundDependencies) == 0 and dependencyDbTimeout > 0:
        dependencyRows = sqlConnection.execute(
            "SELECT * FROM ProductDependencies where ProductPK={}".format(productId))
        foundDependencies = list(dependencyRows.fetchall())
        time.sleep(1)
        dependencyDbTimeout = dependencyDbTimeout - 1

    foundAssetIds = list()
    foundUnresolvedPaths = list()

    uuidIndex = 2
    subIdIndex = 3
    unresolvedPathIndex = 6
    for foundDependency in foundDependencies:
        if foundDependency[unresolvedPathIndex] != "":
            # If there's a path, there won't be an asset ID
            foundUnresolvedPaths.append(foundDependency[unresolvedPathIndex])
        else:
            dependencyUUIDAsHex = codecs.encode(foundDependency[uuidIndex], 'hex_codec')
            subId = str(foundDependency[subIdIndex])
            assetId = "{}:{}".format(dependencyUUIDAsHex.decode('utf8'), subId)
            foundAssetIds.append(assetId)

    assert sorted(pathDependencies) == sorted(foundUnresolvedPaths)
    assert sorted(assetIdDependencies) == sorted(foundAssetIds)


def test_VegdescriptorlistValidDependencies_DependenciesInDb(HeliosProjectFixture):
    engineRoot, projectName, buildInfo, dbCheckWaitTime = HeliosProjectFixture
    pathDependencies = {}
    assetIdDependencies = {
        # MeshAsset reference to "objects/default/primative_wedge_30.cgf"
        "e8b39f901f905e3998aa6f8ec4e91507:0",
        # MaterialAsset reference to "materials/am_grass1.mtl"
        "1151f14d38a65579888abe3139882e68:0"
    }
    AssertProductHasDependencies(engineRoot, projectName, buildInfo, dbCheckWaitTime, "heliosvegetation.vegdescriptorlist", pathDependencies, assetIdDependencies)

def test_CloudLibrarytValidDependencies_DependenciesInDb(HeliosProjectFixture):
    engineRoot, projectName, buildInfo, dbCheckWaitTime = HeliosProjectFixture
    pathDependencies = {}
    assetIdDependencies = {
        # MaterialAsset reference to "materials/clouds/baseclouds.mtl"
        "f249f13854055cfba3b6d95bdc1a3db0:0"
    }
    AssertProductHasDependencies(engineRoot, projectName, buildInfo, dbCheckWaitTime, "libs/clouds/default.xml", pathDependencies, assetIdDependencies)
