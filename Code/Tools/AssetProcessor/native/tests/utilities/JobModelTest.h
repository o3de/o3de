/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <native/resourcecompiler/JobsModel.h>
#include <native/tests/AssetProcessorTest.h>
#include <native/tests/MockAssetDatabaseRequestsHandler.h>
#include <AzToolsFramework/API/AssetDatabaseBus.h>
#include <QCoreApplication>
#include <tests/UnitTestUtilities.h>

namespace AzToolsFramework
{
    namespace AssetDatabase
    {
        class JobDatabaseEntry;
    }
}

class UnitTestJobModel
    : public AssetProcessor::JobsModel
{
public:
    ~UnitTestJobModel() override = default;
    friend class GTEST_TEST_CLASS_NAME_(JobModelUnitTests, Test_RemoveMiddleJob);
    friend class GTEST_TEST_CLASS_NAME_(JobModelUnitTests, Test_RemoveFirstJob);
    friend class GTEST_TEST_CLASS_NAME_(JobModelUnitTests, Test_RemoveLastJob);
    friend class GTEST_TEST_CLASS_NAME_(JobModelUnitTests, Test_RemoveAllJobsBySource);
    friend class GTEST_TEST_CLASS_NAME_(JobModelUnitTests, Test_RemoveAllJobsBySourceFolder);
    friend class GTEST_TEST_CLASS_NAME_(JobModelUnitTests, Test_PopulateJobsFromDatabase);
    friend class JobModelUnitTests;
};

class JobModelUnitTests
    : public AssetProcessor::AssetProcessorTest
{
public:
    void SetUp() override;
    void TearDown() override;

    void VerifyModel();

    UnitTestJobModel* m_unitTestJobModel;

protected:
    struct StaticData
    {
        AssetProcessor::MockAssetDatabaseRequestsHandler m_databaseLocationListener;
        AssetProcessor::AssetDatabaseConnection m_connection;

        const AZStd::string m_sourceName{ "theFile.fbx" };
        AZStd::vector<AzToolsFramework::AssetDatabase::JobDatabaseEntry> m_jobEntries;
        UnitTests::MockPathConversion mockPathConversion{ "c:/test" };
    };
    AZStd::unique_ptr<StaticData> m_data;
    void CreateDatabaseTestData();
};

