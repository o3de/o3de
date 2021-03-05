/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <QTemporaryDir>
#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>
#include "AzToolsFramework/API/AssetDatabaseBus.h"
#include "AssetDatabase/AssetDatabase.h"
#include <AssetManager/PathDependencyManager.h>

namespace UnitTests
{
    class MockDatabaseLocationListener : public AzToolsFramework::AssetDatabase::AssetDatabaseRequests::Bus::Handler
    {
    public:
        MOCK_METHOD1(GetAssetDatabaseLocation, bool(AZStd::string&));
    };

    namespace Util
    {
        using namespace AzToolsFramework::AssetDatabase;

        void CreateSourceJobAndProduct(AssetProcessor::AssetDatabaseConnection* stateData, AZ::s64 scanfolderPk, SourceDatabaseEntry& source, JobDatabaseEntry& job, ProductDatabaseEntry& product, const char* sourceName, const char* productName)
        {
            source = SourceDatabaseEntry(scanfolderPk, sourceName, AZ::Uuid::CreateRandom(), "fingerprint");
            EXPECT_TRUE(stateData->SetSource(source));

            job = JobDatabaseEntry(source.m_sourceID, "jobkey", 1111, "pc", AZ::Uuid::CreateRandom(), AzToolsFramework::AssetSystem::JobStatus::Completed, 4444);
            EXPECT_TRUE(stateData->SetJob(job));

            product = ProductDatabaseEntry(job.m_jobID, 0, productName, AZ::Data::AssetType::CreateRandom());
            EXPECT_TRUE(stateData->SetProduct(product));
        }
    }

    struct PathDependencyDeletionTest
        : UnitTest::ScopedAllocatorSetupFixture
        , UnitTest::TraceBusRedirector
    {
        void SetUp() override;
        void TearDown() override;

        QTemporaryDir m_tempDir;
        AZStd::string m_databaseLocation;
        ::testing::NiceMock<MockDatabaseLocationListener> m_databaseLocationListener;
        AZStd::shared_ptr<AssetProcessor::AssetDatabaseConnection> m_stateData;
        AZStd::unique_ptr<AssetProcessor::PlatformConfiguration> m_platformConfig;
    };

    void PathDependencyDeletionTest::SetUp()
    {
        using namespace ::testing;
        using namespace AzToolsFramework::AssetDatabase;

        BusConnect();

        QDir tempPath(m_tempDir.path());

        m_databaseLocationListener.BusConnect();

        // in other unit tests we may open the database called ":memory:" to use an in-memory database instead of one on disk.
        // in this test, however, we use a real database, because the file processor shares it and opens its own connection to it.
        // ":memory:" databases are one-instance-only, and even if another connection is opened to ":memory:" it would
        // not share with others created using ":memory:" and get a unique database instead.
        m_databaseLocation = tempPath.absoluteFilePath("test_database.sqlite").toUtf8().constData();

        ON_CALL(m_databaseLocationListener, GetAssetDatabaseLocation(_))
            .WillByDefault(
                DoAll( // set the 0th argument ref (string) to the database location and return true.
                    SetArgReferee<0>(m_databaseLocation),
                    Return(true)));

        m_stateData = AZStd::shared_ptr<AssetProcessor::AssetDatabaseConnection>(new AssetProcessor::AssetDatabaseConnection());
        m_stateData->OpenDatabase();

        m_platformConfig = AZStd::make_unique<AssetProcessor::PlatformConfiguration>();
    }

    void PathDependencyDeletionTest::TearDown()
    {
        BusDisconnect();
    }

    TEST_F(PathDependencyDeletionTest, ExistingSourceWithUnmetDependency_RemovedFromDB_DependentSourceCreatedWithoutError)
    {
        using namespace AzToolsFramework::AssetDatabase;

        // Add a product to the db with an unmet dependency
        ScanFolderDatabaseEntry scanFolder("folder", "test", "test", "");
        m_stateData->SetScanFolder(scanFolder);

        SourceDatabaseEntry source1, source2;
        JobDatabaseEntry job1, job2;
        ProductDatabaseEntry product1, product2;

        Util::CreateSourceJobAndProduct(m_stateData.get(), scanFolder.m_scanFolderID, source1, job1, product1, "source1.txt", "product1.jpg");

        ProductDependencyDatabaseEntry dependency(product1.m_productID, AZ::Uuid::CreateRandom(), 0, 0, "pc", 0, "source2.txt", ProductDependencyDatabaseEntry::DependencyType::ProductDep_SourceFile);
        m_stateData->SetProductDependency(dependency);

        AssetProcessor::PathDependencyManager manager(m_stateData, m_platformConfig.get());

        // Delete the data from the database
        m_stateData->RemoveSource(source1.m_sourceID);

        Util::CreateSourceJobAndProduct(m_stateData.get(), scanFolder.m_scanFolderID, source2, job2, product2, "source2.txt", "product2.jpg");

        manager.RetryDeferredDependencies(source2);
    }

    TEST_F(PathDependencyDeletionTest, ExistingSourceWithUnmetDependency_RemovedFromDB_DependentProductCreatedWithoutError)
    {
        using namespace AzToolsFramework::AssetDatabase;

        // Add a product to the db with an unmet dependency
        ScanFolderDatabaseEntry scanFolder("folder", "test", "test", "");
        m_stateData->SetScanFolder(scanFolder);

        SourceDatabaseEntry source1, source2;
        JobDatabaseEntry job1, job2;
        ProductDatabaseEntry product1, product2;

        Util::CreateSourceJobAndProduct(m_stateData.get(), scanFolder.m_scanFolderID, source1, job1, product1, "source1.txt", "product1.jpg");

        ProductDependencyDatabaseEntry dependency(product1.m_productID, AZ::Uuid::CreateRandom(), 0, 0, "pc", 0, "product2.jpg", ProductDependencyDatabaseEntry::DependencyType::ProductDep_ProductFile);
        m_stateData->SetProductDependency(dependency);

        AssetProcessor::PathDependencyManager manager(m_stateData, m_platformConfig.get());

        // Delete the data from the database
        m_stateData->RemoveSource(source1.m_sourceID);

        Util::CreateSourceJobAndProduct(m_stateData.get(), scanFolder.m_scanFolderID, source2, job2, product2, "source2.txt", "product2.jpg");

        manager.RetryDeferredDependencies(source2);
    }

    TEST_F(PathDependencyDeletionTest, NewSourceWithUnmetDependency_RemovedFromDB_DependentSourceCreatedWithoutError)
    {
        using namespace AzToolsFramework::AssetDatabase;

        AssetProcessor::PathDependencyManager manager(m_stateData, m_platformConfig.get());

        // Add a product to the db with an unmet dependency
        ScanFolderDatabaseEntry scanFolder("folder", "test", "test", "");
        m_stateData->SetScanFolder(scanFolder);

        SourceDatabaseEntry source1, source2;
        JobDatabaseEntry job1, job2;
        ProductDatabaseEntry product1, product2;

        Util::CreateSourceJobAndProduct(m_stateData.get(), scanFolder.m_scanFolderID, source1, job1, product1, "source1.txt", "product1.jpg");

        AssetBuilderSDK::ProductPathDependencySet set;
        set.insert(AssetBuilderSDK::ProductPathDependency("source2.txt", AssetBuilderSDK::ProductPathDependencyType::SourceFile));

        manager.SaveUnresolvedDependenciesToDatabase(set, product1, "pc");

        // Delete the data from the database
        m_stateData->RemoveSource(source1.m_sourceID);

        Util::CreateSourceJobAndProduct(m_stateData.get(), scanFolder.m_scanFolderID, source2, job2, product2, "source2.txt", "product2.jpg");

        manager.RetryDeferredDependencies(source2);
    }

    TEST_F(PathDependencyDeletionTest, NewSourceWithUnmetDependency_RemovedFromDB_DependentProductCreatedWithoutError)
    {
        using namespace AzToolsFramework::AssetDatabase;

        AssetProcessor::PathDependencyManager manager(m_stateData, m_platformConfig.get());

        // Add a product to the db with an unmet dependency
        ScanFolderDatabaseEntry scanFolder("folder", "test", "test", "");
        m_stateData->SetScanFolder(scanFolder);

        SourceDatabaseEntry source1, source2;
        JobDatabaseEntry job1, job2;
        ProductDatabaseEntry product1, product2;

        Util::CreateSourceJobAndProduct(m_stateData.get(), scanFolder.m_scanFolderID, source1, job1, product1, "source1.txt", "product1.jpg");

        AssetBuilderSDK::ProductPathDependencySet set;
        set.insert(AssetBuilderSDK::ProductPathDependency("product2.jpg", AssetBuilderSDK::ProductPathDependencyType::ProductFile));

        manager.SaveUnresolvedDependenciesToDatabase(set, product1, "pc");

        // Delete the data from the database
        m_stateData->RemoveSource(source1.m_sourceID);

        Util::CreateSourceJobAndProduct(m_stateData.get(), scanFolder.m_scanFolderID, source2, job2, product2, "source2.txt", "product2.jpg");

        manager.RetryDeferredDependencies(source2);
    }

    TEST_F(PathDependencyDeletionTest, NewSourceWithUnmetDependency_Wildcard_RemovedFromDB_DependentSourceCreatedWithoutError)
    {
        using namespace AzToolsFramework::AssetDatabase;

        AssetProcessor::PathDependencyManager manager(m_stateData, m_platformConfig.get());

        // Add a product to the db with an unmet dependency
        ScanFolderDatabaseEntry scanFolder("folder", "test", "test", "");
        m_stateData->SetScanFolder(scanFolder);

        SourceDatabaseEntry source1, source2;
        JobDatabaseEntry job1, job2;
        ProductDatabaseEntry product1, product2;

        Util::CreateSourceJobAndProduct(m_stateData.get(), scanFolder.m_scanFolderID, source1, job1, product1, "source1.txt", "product1.jpg");

        AssetBuilderSDK::ProductPathDependencySet set;
        set.insert(AssetBuilderSDK::ProductPathDependency("sou*ce2.txt", AssetBuilderSDK::ProductPathDependencyType::SourceFile));

        manager.SaveUnresolvedDependenciesToDatabase(set, product1, "pc");

        // Delete the data from the database
        m_stateData->RemoveSource(source1.m_sourceID);

        Util::CreateSourceJobAndProduct(m_stateData.get(), scanFolder.m_scanFolderID, source2, job2, product2, "source2.txt", "product2.jpg");

        manager.RetryDeferredDependencies(source2);
    }
}
