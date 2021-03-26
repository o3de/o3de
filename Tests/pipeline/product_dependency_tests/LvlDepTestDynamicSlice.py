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

from TestFixtures import EmptyProjectFixture

import fileinput
import os
import pytest
import shutil
import sqlite3
import time

import SubprocessUtils

def MakeEditorPythonFile(testLevelName, assetGuid, tempFolder, templatePythonFile):
    outputFileName = templatePythonFile.replace(".template", ".py")
    outputFilePath = os.path.join(tempFolder, outputFileName)
    shutil.copy(os.path.join(os.path.dirname(os.path.realpath(
        __file__)), templatePythonFile), outputFilePath)
    for line in fileinput.FileInput(outputFilePath, inplace=1):
        line = line.replace("${LevelName}", str.format('"{}"', testLevelName))
        line = line.replace("${MeshGuid}", str.format('"{}"', assetGuid))
        print (line)
    return outputFilePath

@pytest.mark.skip(reason="This test takes too long on Jenkins, and bundler tests catch everything we want from here")
def test_productDependencies_EntityInLevelWithAssetReference_ReferencedAssetIsLevelProductDependency(
        EmptyProjectFixture, tmpdir):
    print ("RunLvlDynamicSliceTest")
    engineRoot, projectName, buildInfo, dbCheckWaitTime = EmptyProjectFixture


    tempFolder = tmpdir.mkdir("EditorPyScripts")
    print ("  * Launch editor with dynamic slice test creation script")

    testLevelName = "SimpleLevel"
    expectedDependencyGuid = "81C4A6AF-C57D-5734-81B7-822074358C4D"
    exportLevelScriptPath = MakeEditorPythonFile(testLevelName, expectedDependencyGuid, str(tempFolder), "export_test_level.template")

    lyCommand = [buildInfo.editorExe, '/BatchMode', '/runpython', exportLevelScriptPath]
    SubprocessUtils.SubprocessWithTimeout(lyCommand, engineRoot, 60)

    # Logic that the export_test_level.py script will run in the editor:
    # * Create new level
    # * Place an entity in the level
    # * Add the mesh component to the entity
    # * Assign a test asset to that component [dev\Engine\Objects\default\primitive_sphere.cgf]
    # * Export the level

    print ("   * Wait for the Asset Processor to copy the asset to the cache and update the asset database")

    projectCacheRoot = os.path.join(engineRoot, "Cache", projectName)
    levelRelativeSubFolder = os.path.join("Levels", testLevelName)
    cachePath = os.path.join(projectCacheRoot, buildInfo.cacheSubfolder, projectName, levelRelativeSubFolder, "level.pak")
    # On an i7 running Lumberyard on an SSD, it normally takes about 1-2 minutes to complete this step.
    # Add a few minutes onto that because the Jenkins machines may not be as fast.
    pakTimeoutSeconds = 10 * 60
    pakTimeoutWaitTimeSeconds = 1
    # Wait for the level.pak file to exist in the cache
    while not os.path.exists(cachePath) and pakTimeoutSeconds > 0:
        time.sleep(pakTimeoutWaitTimeSeconds)
        pakTimeoutSeconds -= pakTimeoutWaitTimeSeconds

    assert(os.path.exists(cachePath))

    print ("   * Open the asset database, check that the correct product dependency is set for the exported level.pak")

    # A newly created level will have all of these dependencies by default.
    # These are tracked by the relative source path instead of the UUID because it's more readable.
    expectedDependencyPaths = {
        "materials/material_terrain_default.mtl", # from leveldata.xml
        "EngineAssets/Materials/sky/sky.mtl", # from mission_mission0.xml
        "EngineAssets/Materials/Water/ocean_default.mtl", # from mission_mission0.xml
        "textures/skys/night/half_moon.tif"
    }

    # Nothing is currently expected to unresolved, but this is left here in case that changes.
    expectedUnresolvedPaths = {
        # Hardcoded levelbuilder relative path output
        os.path.join(levelRelativeSubFolder, "auto_resourcelist.txt"),
        os.path.join(levelRelativeSubFolder, "level.cfg"),
        os.path.join(levelRelativeSubFolder, "levelparticles.xml"),
        os.path.join(levelRelativeSubFolder, "occluder.ocm"),
        os.path.join(levelRelativeSubFolder, "preloadlibs.txt"),
        os.path.join(levelRelativeSubFolder, "terrain", "cover.ctc"),
        os.path.join(levelRelativeSubFolder, "terrain", "merged_meshes_sectors", "mmrm_used_meshes.lst"),
        os.path.join(levelRelativeSubFolder, str.format("{}.xml",testLevelName))
    }

    # All expected GUIDs should match the format in the database: Lowercase, with no separators.
    expectedDependencyGuids = {
        expectedDependencyGuid.lower().replace('-','')
    }

    CheckDatabaseForDependency(projectCacheRoot, projectName, testLevelName, expectedDependencyGuids,
                               expectedDependencyPaths, expectedUnresolvedPaths, buildInfo, dbCheckWaitTime)

    print ("/RunLvlDynamicSliceTest")


def CheckDatabaseForDependency(projectCacheRoot, projectName, testLevelName, expectedDependencyGuids, expectedDependencyPaths, expectedUnresolvedPaths, buildInfo, dbCheckWaitTime):
    print ("CheckDatabaseForDependency")

    print (str.format("   * Checking expected dependencies for level.pak for level {}", testLevelName))
    print (str.format("       * Searching for these GUIDs as dependencies: {}", str(expectedDependencyGuids)))
    print (str.format("       * Searching for these paths as dependencies: {}", str(expectedDependencyPaths)))
    print (str.format("       * Searching for these paths as unresolved paths: {}", str(expectedUnresolvedPaths)))

    sqlDatabasePath = os.path.join(projectCacheRoot, "assetdb.sqlite")
    print ("   * Connecting to database " + sqlDatabasePath)
    sqlConnection = sqlite3.connect(sqlDatabasePath)
    try:
        # Not using os.path.join because this is an expected string in a database
        levelPakProduct = str.format('{}/{}/levels/{}/level.pak', buildInfo.cacheSubfolder, projectName.lower(), testLevelName.lower())
        print ("   * Looking in product table for " + levelPakProduct)
        productRows = sqlConnection.execute(
            str.format("SELECT ProductID FROM Products where ProductName='{}'", levelPakProduct))

        productRowsList = list(productRows.fetchall())

        productDbWait = dbCheckWaitTime
        while len(productRowsList) == 0 and productDbWait > 0:
            time.sleep(1)
            productDbWait = productDbWait - 1
            productRows = sqlConnection.execute(
                str.format("SELECT ProductID FROM Products where ProductName='{}'", levelPakProduct))
            productRowsList = list(productRows.fetchall())


        assert len(productRowsList) == 1, "productRowsList= {}".format(productRowsList)

        print ("   * Searching product results for product ID")
        productId = int(productRowsList[0][0])

        assert productId

        print (str.format("   * Searching for dependencies for product ID {}", str(productId)))
        dependencyDbSuccess = False
        dependencyDbTimeout = dbCheckWaitTime
        # Make copies of the list in case multiple runs are required
        expectedUnresolvedPathsCopy = []
        expectedDependencyGuidsCopy = []
        remainingDependencies = []

        while (not dependencyDbSuccess) and dependencyDbTimeout > 0:
            expectedUnresolvedPathsCopy = expectedUnresolvedPaths.copy()
            expectedDependencyGuidsCopy = expectedDependencyGuids.copy()
            productDependencyRows = sqlConnection.execute("SELECT * FROM ProductDependencies where ProductPK={}".format(productId))

            dependencyRowIndex_SourceId = 2
            dependencyRowIndex_SubId = 3
            dependencyRowIndex_UnresolvedPath = 6

            productDependencyRowList = list(productDependencyRows.fetchall())

            expectedDependencyCount = len(expectedDependencyGuidsCopy) + len(
                expectedDependencyPaths) + len(expectedUnresolvedPathsCopy)

            dependencyDbSuccess = len(
                productDependencyRowList) == expectedDependencyCount

            expectedSubId = 0

            # This will contain SQL data buffers, which are not hashable.
            remainingDependencies = []

            for dependencyRow in productDependencyRowList:
                dependencySourceId = dependencyRow[dependencyRowIndex_SourceId]
                dependencySubId = int(dependencyRow[dependencyRowIndex_SubId])
                dependencyDbSuccess = dependencyDbSuccess and dependencySubId == expectedSubId

                dependencySourceAsHex = str(dependencySourceId).encode('hex')
                wasExpectedDependency = False
                # If this dependency's UUID is in our expected UUID list, then count it as found.
                if dependencySourceAsHex in expectedDependencyGuidsCopy:
                    wasExpectedDependency = True
                    expectedDependencyGuidsCopy.remove(dependencySourceAsHex)

                # If this dependency has an unresolved path that we expect, then count it as found.
                unresolvedPath = dependencyRow[dependencyRowIndex_UnresolvedPath]
                if unresolvedPath in expectedUnresolvedPathsCopy:
                    wasExpectedDependency = True
                    expectedUnresolvedPathsCopy.remove(unresolvedPath)

                if not wasExpectedDependency:
                    remainingDependencies.append(dependencySourceId)

                dependencyDbSuccess = dependencyDbSuccess and (len(expectedDependencyGuidsCopy) == 0 and
                                       len(expectedUnresolvedPathsCopy) == 0 and
                                        len(remainingDependencies) == len(expectedDependencyPaths))
            if not dependencyDbSuccess:
                time.sleep(1)
                dependencyDbTimeout = dependencyDbTimeout - 1

        # do all the checks in asserts, instead of just assert dependencyDbSuccess so that error messages are more specific
        assert len(productDependencyRowList) == expectedDependencyCount, str.format("Expected {} dependencies, found {}", expectedDependencyCount, len(productDependencyRowList))
        assert len(expectedDependencyGuidsCopy) == 0, str.format(
            "Expected dependencies were not found in the asset database: {}", str(expectedDependencyGuids))
        assert len(expectedUnresolvedPathsCopy) == 0, str.format(
            "Expected unresolved paths were not found in the asset database: {}", str(expectedUnresolvedPathsCopy))
        assert len(remainingDependencies) == len(expectedDependencyPaths), str.format("Expected dependency sizes do not match for {} and {}", str(remainingDependencies), str(expectedDependencyPaths))

        for remainingDependency in remainingDependencies:
            sourceRows = sqlConnection.execute("SELECT SourceName FROM Sources where SourceGuid=?", (sqlite3.Binary(remainingDependency),) )
            sourceRowsList = list(sourceRows.fetchall())
            assert len(sourceRowsList) == 1, str.format("Expected to find 1 entry for {}, found {} instead.", str(remainingDependency).encode('hex'), len(sourceRowsList))
            sourcePath = sourceRowsList[0][0]
            assert sourcePath in expectedDependencyPaths, str.format("Could not find {} for UUID {} in the list of expected dependencies.", str(sourcePath), str(remainingDependency).encode('hex'))
            expectedDependencyPaths.remove(sourcePath)
        assert len(expectedDependencyPaths) == 0, str.format("Missing expected dependencies {}", str(expectedDependencyPaths))

        print ("   * Found all expected dependencies")
    finally:
        print ("   * Closing database connection")
        sqlConnection.close()
    print ("/CheckDatabaseForDependency")

