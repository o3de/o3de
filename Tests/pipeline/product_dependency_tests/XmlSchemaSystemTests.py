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

import pytest
import shutil
import subprocess
import os
import sqlite3
import time

TEST_ASSETS_FOLDER_NAME = 'TestAssetS'
TEST_XML_NAME = 'xml_schema_test.xml'
TEST_SCHEMA_NAME = 'xml_schema_test.xmlschema'
UPDATED_TEST_SCHEMA_NAME = 'updated_xml_schema_test.xmlschema'
SCHEMA_FOLDER_NAME = 'Schema'

def test_XmlSchemaSystem_AddNewSchema_ReprocessXml(HeliosProjectFixture):
    engineRoot, projectName, buildInfo, dbCheckWaitTime = HeliosProjectFixture
    projectFolder = os.path.join(engineRoot, projectName)

    testAssetsFolder = os.path.dirname(os.path.realpath(__file__))
    testAssetsFolder = os.path.join(testAssetsFolder, TEST_ASSETS_FOLDER_NAME)
    testXmlPath = os.path.join(testAssetsFolder, TEST_XML_NAME)
    testSchemaPath = os.path.join(testAssetsFolder, TEST_SCHEMA_NAME)
    updatedTestSchemaPath = os.path.join(testAssetsFolder, UPDATED_TEST_SCHEMA_NAME)
    
    projectXmlPath = os.path.join(projectFolder, TEST_XML_NAME)
    schemaFolderInProject = os.path.join(projectFolder, SCHEMA_FOLDER_NAME)
    projectSchemaPath = os.path.join(schemaFolderInProject, TEST_SCHEMA_NAME)

    # Clean up the existing test xml and schema assets
    if CleanUpTestAsset(projectXmlPath) or CleanUpTestAsset(projectSchemaPath):
        # Remove the test assets and its dependencies from the database
        subprocess.call(
            [os.path.join(buildInfo.buildFolder, 'AssetProcessorBatch'), "/gamefolder=Helios"])
    
    # Copy test XML asset to the project
    # No product dependency should be output for the XML asset without a corresponding schema
    print ("Add new XML asset")
    expectedUnresolvedPaths = {}
    UpdateProjectAsset(testXmlPath, projectXmlPath, engineRoot, projectName,
                       expectedUnresolvedPaths, buildInfo, dbCheckWaitTime, False)

    # Copy the XML schema asset to the project
    # XML assets are expected to be reprocessed and output product dependencies
    print ("Add new XML schema")
    if not os.path.exists(schemaFolderInProject):
        os.makedirs(schemaFolderInProject)
    expectedUnresolvedPaths = { 'dependency1' }
    UpdateProjectAsset(testSchemaPath, projectSchemaPath, engineRoot, projectName,
                       expectedUnresolvedPaths, buildInfo, dbCheckWaitTime, False)

    # Update the XML schema asset inside the project
    # XML assets are expected to be reprocessed and output product dependencies
    print ("Update XML schema")
    expectedUnresolvedPaths = { 'dependency1',  'dependency2'}
    UpdateProjectAsset(updatedTestSchemaPath, projectSchemaPath, engineRoot, projectName,
                       expectedUnresolvedPaths, buildInfo, dbCheckWaitTime, False)

    # Delete the schema asset in the project folder
    # No product dependency should be output for the XML asset since the schema has been deleted
    print ("Delete XML schema")
    expectedUnresolvedPaths = {}
    UpdateProjectAsset(updatedTestSchemaPath, projectSchemaPath, engineRoot, projectName,
                       expectedUnresolvedPaths, buildInfo, dbCheckWaitTime, True)

    # Clean up the test assets
    CleanUpTestAsset(projectXmlPath)
    if not os.listdir(schemaFolderInProject):
        os.rmdir(schemaFolderInProject)

def test_XmlSchemaSystem_MoveSchemaToGem_ReprocessXml(HeliosProjectFixture):
    engineRoot, projectName, buildInfo, dbCheckWaitTime = HeliosProjectFixture

    projectFolder = os.path.join(engineRoot, projectName)
    testAssetsFolder = os.path.dirname(os.path.realpath(__file__))
    testAssetsFolder = os.path.join(testAssetsFolder, TEST_ASSETS_FOLDER_NAME)
    testXmlPath = os.path.join(testAssetsFolder, TEST_XML_NAME)
    testSchemaPath = os.path.join(testAssetsFolder, TEST_SCHEMA_NAME)
    projectXmlPath = os.path.join(projectFolder, TEST_XML_NAME)
    projectSchemaPath = os.path.join(projectFolder, SCHEMA_FOLDER_NAME, TEST_SCHEMA_NAME)
    # The CertificateManager Gem was arbitrarily chosen. Schema can be added to any enabled gem
    gemSchemaFolder = os.path.join(engineRoot, 'Gems', 'CertificateManager', 'Assets', SCHEMA_FOLDER_NAME)
    gemSchemaPath = os.path.join(gemSchemaFolder, TEST_SCHEMA_NAME)
    
    # Clean up all the potential test xml and schema assets
    if CleanUpTestAsset(projectXmlPath) or CleanUpTestAsset(projectSchemaPath) or CleanUpTestAsset(gemSchemaPath):
        # Remove the test assets and its dependencies from the database
        subprocess.call(
            [os.path.join(buildInfo.buildFolder, 'AssetProcessorBatch'), "/gamefolder=Helios"])

    print ('Add XML schema to CertificateManager gem')
    if not os.path.exists(gemSchemaFolder):
        os.makedirs(gemSchemaFolder)
    shutil.copyfile(testSchemaPath, gemSchemaPath)
    
    print ('Reprocess test assets')
    expectedUnresolvedPaths = { 'dependency1' }
    UpdateProjectAsset(testXmlPath, projectXmlPath, engineRoot, projectName, expectedUnresolvedPaths, buildInfo, dbCheckWaitTime, False)

    # Clean up the test assets
    CleanUpTestAsset(projectXmlPath)
    CleanUpTestAsset(gemSchemaPath)
    if not os.listdir(gemSchemaFolder):
        os.rmdir(gemSchemaFolder)

def CleanUpTestAsset(assetName):
    assetExists = False;
    if os.path.exists(assetName):
        assetExists = True;
        os.remove(assetName)

    assert not os.path.exists(assetName)

    return assetExists


def UpdateProjectAsset(testAssetPath, projectAssetPath, engineRoot, projectName, expectedUnresolvedPaths, buildInfo, dbCheckWaitTime, deleteAsset):
    if deleteAsset:
        os.remove(projectAssetPath)
    else:
        shutil.copyfile(testAssetPath, projectAssetPath)

    # Let AP reprocess the new/updated asset
    subprocess.call(
        [os.path.join(buildInfo.buildFolder, 'AssetProcessorBatch'), "/gamefolder=Helios"])

    projectCacheRoot = os.path.join(engineRoot, 'Cache', projectName)
    CheckDatabaseForDependency(projectCacheRoot, projectName, 
                               expectedUnresolvedPaths, buildInfo, dbCheckWaitTime)


def CheckDatabaseForDependency(projectCacheRoot, projectName, expectedUnresolvedPaths, buildInfo, dbCheckWaitTime):
    print ("CheckDatabaseForDependency")

    print ("   * Searching for these paths as unresolved paths: {}".format(str(expectedUnresolvedPaths)))

    sqlDatabasePath = os.path.join(projectCacheRoot, "assetdb.sqlite")
    print ("   * Connecting to database " + sqlDatabasePath)
    sqlConnection = sqlite3.connect(sqlDatabasePath)
    try:
        # Not using os.path.join because this is an expected string in a database
        xmlProduct = '{}/{}/xml_schema_test.xml'.format(buildInfo.cacheSubfolder, projectName.lower())
        print ("   * Looking in product table for " + xmlProduct)
        productRows = sqlConnection.execute(
            "SELECT ProductID FROM Products where ProductName='{}'".format(xmlProduct))
        productRowsList = list(productRows.fetchall())

        productDbWait = dbCheckWaitTime
        while len(productRowsList) == 0 and productDbWait > 0:
            time.sleep(1)
            productDbWait = productDbWait - 1
            productRows = sqlConnection.execute(
                "SELECT ProductID FROM Products where ProductName='{}'".format(xmlProduct))
            productRowsList = list(productRows.fetchall())

        assert len(productRowsList) == 1, "productRowsList= {}".format(productRowsList)

        print ("   * Searching product results for product ID")
        productId = int(productRowsList[0][0])

        assert productId

        print ("   * Searching for dependencies for product ID {}".format(productId))
        dependencyDbSuccess = False
        dependencyDbTimeout = dbCheckWaitTime
        # Make copies of the list in case multiple runs are required
        expectedUnresolvedPathsCopy = []
        dependencyRowIndex_UnresolvedPath = 6
        while (not dependencyDbSuccess) and dependencyDbTimeout > 0:
            expectedUnresolvedPathsCopy = expectedUnresolvedPaths.copy()
            productDependencyRows = sqlConnection.execute("SELECT * FROM ProductDependencies where ProductPK={}".format(productId))

            productDependencyRowList = list(productDependencyRows.fetchall())
            expectedDependencyCount = len(expectedUnresolvedPathsCopy)
            dependencyDbSuccess = len(
                productDependencyRowList) == expectedDependencyCount

            for dependencyRow in productDependencyRowList:
                # If this dependency has an unresolved path that we expect, then count it as found.
                unresolvedPath = dependencyRow[dependencyRowIndex_UnresolvedPath]
                if unresolvedPath in expectedUnresolvedPathsCopy:
                    expectedUnresolvedPathsCopy.remove(unresolvedPath)

                dependencyDbSuccess = dependencyDbSuccess and len(expectedUnresolvedPathsCopy) == 0

            if not dependencyDbSuccess:
                time.sleep(1)
                dependencyDbTimeout = dependencyDbTimeout - 1

        # do all the checks in asserts, instead of just assert dependencyDbSuccess so that error messages are more specific
        assert len(expectedUnresolvedPathsCopy) == 0, str.format(
            "Expected unresolved paths were not found in the asset database: {}", str(expectedUnresolvedPathsCopy))

        print ("   * Found all expected dependencies")
    finally:
        print ("   * Closing database connection")
        sqlConnection.close()
    print ("/CheckDatabaseForDependency")