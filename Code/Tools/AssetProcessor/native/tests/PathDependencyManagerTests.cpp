/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QTemporaryDir>
#include <AzTest/AzTest.h>
#if !defined(Q_MOC_RUN)
#include <AzCore/UnitTest/TestTypes.h>
#endif
#include "AzToolsFramework/API/AssetDatabaseBus.h"
#include "AssetDatabase/AssetDatabase.h"
#include <AssetManager/PathDependencyManager.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Jobs/JobContext.h>
#include <AzCore/Jobs/JobManager.h>
#include <AzCore/Jobs/JobManagerComponent.h>

#include <native/tests/MockAssetDatabaseRequestsHandler.h>

namespace UnitTests
{
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

    struct PathDependencyBase
        : ::UnitTest::TraceBusRedirector
    {
        void Init();
        void Destroy();

        QTemporaryDir m_tempDir;
        AZStd::string m_databaseLocation;
        AssetProcessor::MockAssetDatabaseRequestsHandler m_databaseLocationListener;
        AZStd::shared_ptr<AssetProcessor::AssetDatabaseConnection> m_stateData;
        AZStd::unique_ptr<AssetProcessor::PlatformConfiguration> m_platformConfig;
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
        AZ::Entity* m_jobManagerEntity{};
        AZ::ComponentDescriptor* m_descriptor{};
    };

    struct PathDependencyDeletionTest
        : ::UnitTest::ScopedAllocatorSetupFixture
        , PathDependencyBase
    {
        void SetUp() override
        {
            PathDependencyBase::Init();
        }

        void TearDown() override
        {
            PathDependencyBase::Destroy();
        }
    };

    void PathDependencyBase::Init()
    {
        using namespace ::testing;
        using namespace AzToolsFramework::AssetDatabase;

        ::UnitTest::TestRunner::Instance().m_suppressAsserts = false;
        ::UnitTest::TestRunner::Instance().m_suppressErrors = false;

        BusConnect();

        m_stateData = AZStd::shared_ptr<AssetProcessor::AssetDatabaseConnection>(new AssetProcessor::AssetDatabaseConnection());
        m_stateData->OpenDatabase();

        m_platformConfig = AZStd::make_unique<AssetProcessor::PlatformConfiguration>();

        AZ::AllocatorInstance<AZ::PoolAllocator>::Create();
        AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Create();

        m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
        m_descriptor = AZ::JobManagerComponent::CreateDescriptor();
        m_descriptor->Reflect(m_serializeContext.get());

        m_jobManagerEntity = aznew AZ::Entity{};
        m_jobManagerEntity->CreateComponent<AZ::JobManagerComponent>();
        m_jobManagerEntity->Init();
        m_jobManagerEntity->Activate();
    }

    void PathDependencyBase::Destroy()
    {
        m_stateData = nullptr;
        m_platformConfig = nullptr;

        m_jobManagerEntity->Deactivate();
        delete m_jobManagerEntity;

        delete m_descriptor;

        AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Destroy();
        AZ::AllocatorInstance<AZ::PoolAllocator>::Destroy();

        BusDisconnect();
    }

    TEST_F(PathDependencyDeletionTest, ExistingSourceWithUnmetDependency_RemovedFromDB_DependentSourceCreatedWithoutError)
    {
        using namespace AzToolsFramework::AssetDatabase;

        // Add a product to the db with an unmet dependency
        ScanFolderDatabaseEntry scanFolder("folder", "test", "test", 0);
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

        manager.QueueSourceForDependencyResolution(source2);
        manager.ProcessQueuedDependencyResolves();
    }

    TEST_F(PathDependencyDeletionTest, ExistingSourceWithUnmetDependency_RemovedFromDB_DependentProductCreatedWithoutError)
    {
        using namespace AzToolsFramework::AssetDatabase;

        // Add a product to the db with an unmet dependency
        ScanFolderDatabaseEntry scanFolder("folder", "test", "test", 0);
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

        manager.QueueSourceForDependencyResolution(source2);
        manager.ProcessQueuedDependencyResolves();
    }

    TEST_F(PathDependencyDeletionTest, NewSourceWithUnmetDependency_RemovedFromDB_DependentSourceCreatedWithoutError)
    {
        using namespace AzToolsFramework::AssetDatabase;

        AssetProcessor::PathDependencyManager manager(m_stateData, m_platformConfig.get());

        // Add a product to the db with an unmet dependency
        ScanFolderDatabaseEntry scanFolder("folder", "test", "test", 0);
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

        manager.QueueSourceForDependencyResolution(source2);
        manager.ProcessQueuedDependencyResolves();
    }

    TEST_F(PathDependencyDeletionTest, NewSourceWithUnmetDependency_RemovedFromDB_DependentProductCreatedWithoutError)
    {
        using namespace AzToolsFramework::AssetDatabase;

        AssetProcessor::PathDependencyManager manager(m_stateData, m_platformConfig.get());

        // Add a product to the db with an unmet dependency
        ScanFolderDatabaseEntry scanFolder("folder", "test", "test", 0);
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

        manager.QueueSourceForDependencyResolution(source2);
        manager.ProcessQueuedDependencyResolves();
    }

    TEST_F(PathDependencyDeletionTest, NewSourceWithUnmetDependency_Wildcard_RemovedFromDB_DependentSourceCreatedWithoutError)
    {
        using namespace AzToolsFramework::AssetDatabase;

        AssetProcessor::PathDependencyManager manager(m_stateData, m_platformConfig.get());

        // Add a product to the db with an unmet dependency
        ScanFolderDatabaseEntry scanFolder("folder", "test", "test", 0);
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

        manager.QueueSourceForDependencyResolution(source2);
        manager.ProcessQueuedDependencyResolves();
    }

    using PathDependencyTests = PathDependencyDeletionTest;

    TEST_F(PathDependencyTests, SourceAndProductHaveSameName_SourceFileDependency_MatchesSource)
    {
        using namespace AzToolsFramework::AssetDatabase;

        AssetProcessor::PathDependencyManager manager(m_stateData, m_platformConfig.get());

        ScanFolderDatabaseEntry scanFolder("folder", "test", "test", 0);
        m_stateData->SetScanFolder(scanFolder);

        SourceDatabaseEntry source1, source2;
        JobDatabaseEntry job1, job2;
        ProductDatabaseEntry product1, product2, product3;

        Util::CreateSourceJobAndProduct(
            m_stateData.get(), scanFolder.m_scanFolderID, source1, job1, product1, "source1.txt", "product1.jpg");

        AssetBuilderSDK::ProductPathDependencySet set;
        set.insert(AssetBuilderSDK::ProductPathDependency("*.xml", AssetBuilderSDK::ProductPathDependencyType::SourceFile));

        manager.SaveUnresolvedDependenciesToDatabase(set, product1, "pc");

        Util::CreateSourceJobAndProduct(
            m_stateData.get(), scanFolder.m_scanFolderID, source2, job2, product2, "source2.xml", "source2.xml");

        // Create a 2nd product for this source
        product3 = ProductDatabaseEntry{ job2.m_jobID, product2.m_subID + 1, "source2.txt", AZ::Data::AssetType::CreateRandom() };
        ASSERT_TRUE(m_stateData->SetProduct(product3));

        manager.QueueSourceForDependencyResolution(source2);
        manager.ProcessQueuedDependencyResolves();

        ProductDependencyDatabaseEntryContainer productDependencies;
        m_stateData->GetProductDependencies(productDependencies);

        EXPECT_EQ(productDependencies.size(), 3);
    }

    TEST_F(PathDependencyTests, SourceAndProductHaveSameName_ProductFileDependency_MatchesProduct)
    {
        using namespace AzToolsFramework::AssetDatabase;

        AssetProcessor::PathDependencyManager manager(m_stateData, m_platformConfig.get());

        ScanFolderDatabaseEntry scanFolder("folder", "test", "test", 0);
        m_stateData->SetScanFolder(scanFolder);

        SourceDatabaseEntry source1, source2;
        JobDatabaseEntry job1, job2;
        ProductDatabaseEntry product1, product2, product3;

        Util::CreateSourceJobAndProduct(
            m_stateData.get(), scanFolder.m_scanFolderID, source1, job1, product1, "source1.txt", "product1.jpg");

        AssetBuilderSDK::ProductPathDependencySet set;
        set.insert(AssetBuilderSDK::ProductPathDependency("*.xml", AssetBuilderSDK::ProductPathDependencyType::ProductFile));

        manager.SaveUnresolvedDependenciesToDatabase(set, product1, "pc");

        Util::CreateSourceJobAndProduct(
            m_stateData.get(), scanFolder.m_scanFolderID, source2, job2, product2, "source2.xml", "source2.xml");

        // Create a 2nd product for this source
        product3 = ProductDatabaseEntry{job2.m_jobID, product2.m_subID + 1, "source2.txt", AZ::Data::AssetType::CreateRandom()};
        ASSERT_TRUE(m_stateData->SetProduct(product3));

        manager.QueueSourceForDependencyResolution(source2);
        manager.ProcessQueuedDependencyResolves();

        ProductDependencyDatabaseEntryContainer productDependencies;
        m_stateData->GetProductDependencies(productDependencies);

        EXPECT_EQ(productDependencies.size(), 2);
    }

    struct PathDependencyBenchmarks
        : ::UnitTest::ScopedAllocatorFixture
          , PathDependencyBase
    {
        static inline constexpr int NumTestDependencies = 4; // Must be a multiple of 4
        static inline constexpr int NumTestProducts = 2; // Must be a multiple of 2

        AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer m_products;
        AzToolsFramework::AssetDatabase::SourceDatabaseEntry m_source1, m_source2, m_source4;
        AzToolsFramework::AssetDatabase::JobDatabaseEntry m_job1, m_job2, m_job4;
        AzToolsFramework::AssetDatabase::ProductDatabaseEntry m_product1, m_product2, m_product4;
        AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer m_dependencies;

        void SetupTestData()
        {
            using namespace AzToolsFramework::AssetDatabase;

            ScanFolderDatabaseEntry scanFolder("folder", "test", "test", 0);
            ASSERT_TRUE(m_stateData->SetScanFolder(scanFolder));

            Util::CreateSourceJobAndProduct(
                m_stateData.get(), scanFolder.m_scanFolderID, m_source1, m_job1, m_product1, "source1.txt", "product1.jpg");

            Util::CreateSourceJobAndProduct(
                m_stateData.get(), scanFolder.m_scanFolderID, m_source4, m_job4, m_product4, "source4.txt", "product4.jpg");

            for (int i = 0; i < NumTestDependencies / 2; ++i)
            {
                m_dependencies.emplace_back(
                    m_product1.m_productID, AZ::Uuid::CreateNull(), 0, 0, "pc", 0,
                    AZStd::string::format("folder/folder2/%d_*2.jpg", i).c_str());
                ++i;
                m_dependencies.emplace_back(
                    m_product1.m_productID, AZ::Uuid::CreateNull(), 0, 0, "mac", 0,
                    AZStd::string::format("folder/folder2/%d_*2.jpg", i).c_str());
            }

            for (int i = 0; i < NumTestDependencies / 2; ++i)
            {
                m_dependencies.emplace_back(
                    m_product4.m_productID, AZ::Uuid::CreateNull(), 0, 0, "pc", 0,
                    AZStd::string::format("folder/folder2/%d_*2.jpg", i).c_str());
                ++i;
                m_dependencies.emplace_back(
                    m_product4.m_productID, AZ::Uuid::CreateNull(), 0, 0, "mac", 0,
                    AZStd::string::format("folder/folder2/%d_*2.jpg", i).c_str());
            }

            ASSERT_TRUE(m_stateData->SetProductDependencies(m_dependencies));

            Util::CreateSourceJobAndProduct(
                m_stateData.get(), scanFolder.m_scanFolderID, m_source2, m_job2, m_product2, "source2.txt", "product2.jpg");

            auto job3 = JobDatabaseEntry(
                m_source2.m_sourceID, "jobkey", 1111, "mac", AZ::Uuid::CreateRandom(), AzToolsFramework::AssetSystem::JobStatus::Completed,
                4444);
            ASSERT_TRUE(m_stateData->SetJob(job3));

            for (int i = 0; i < NumTestProducts; ++i)
            {
                m_products.emplace_back(
                    m_job2.m_jobID, i, AZStd::string::format("pc/folder/folder2/%d_product2.jpg", i).c_str(),
                    AZ::Data::AssetType::CreateRandom());
                ++i;
                m_products.emplace_back(
                    job3.m_jobID, i, AZStd::string::format("mac/folder/folder2/%d_product2.jpg", i).c_str(),
                    AZ::Data::AssetType::CreateRandom());
            }

            ASSERT_TRUE(m_stateData->SetProducts(m_products));
        }

        void DoTest()
        {
            AssetProcessor::PathDependencyManager manager(m_stateData, m_platformConfig.get());

            manager.QueueSourceForDependencyResolution(m_source2);
            manager.ProcessQueuedDependencyResolves();
        }

        void VerifyResult()
        {
            using namespace AzToolsFramework::AssetDatabase;

            ProductDependencyDatabaseEntryContainer productDependencies;
            m_stateData->GetProductDependencies(productDependencies);

            for (int i = 0; i < NumTestDependencies / 2 && i < NumTestProducts; ++i)
            {
                const auto& product = m_products[i];
                int found = 0;

                for (const auto& unresolvedProductDependency : productDependencies)
                {
                    if (unresolvedProductDependency.m_dependencySourceGuid == m_source2.m_sourceGuid &&
                        unresolvedProductDependency.m_dependencySubID == product.m_subID &&
                        unresolvedProductDependency.m_productPK == m_product1.m_productID)
                    {
                        ++found;
                    }

                    if (unresolvedProductDependency.m_dependencySourceGuid == m_source2.m_sourceGuid &&
                        unresolvedProductDependency.m_dependencySubID == product.m_subID &&
                        unresolvedProductDependency.m_productPK == m_product4.m_productID)
                    {
                        ++found;
                    }

                    if (found == 2)
                        break;
                }

                EXPECT_TRUE(found == 2) << product.m_productName.c_str() << " was not found";
            }

            EXPECT_EQ(productDependencies.size(), NumTestDependencies * 2);
        }
    };

    // For some reason, BENCHMARK_F doesn't seem to call the destructor
    // So we'll wrap the class and handle the new/delete ourselves
    struct PathDependencyBenchmarksWrapperClass : public ::benchmark::Fixture
    {
        void SetUp([[maybe_unused]] const benchmark::State& st) override
        {
            m_benchmarks = new PathDependencyBenchmarks();
            m_benchmarks->Init();
            m_benchmarks->SetupTestData();
        }

        void SetUp([[maybe_unused]] benchmark::State& st) override
        {
            m_benchmarks = new PathDependencyBenchmarks();
            m_benchmarks->Init();
            m_benchmarks->SetupTestData();
        }

        void TearDown([[maybe_unused]] benchmark::State& st) override
        {
            m_benchmarks->Destroy();
            delete m_benchmarks;
        }

        void TearDown([[maybe_unused]] const benchmark::State& st) override
        {
            m_benchmarks->Destroy();
            delete m_benchmarks;
        }

        PathDependencyBenchmarks* m_benchmarks = {};
    };

    struct PathDependencyTestValidation
        : PathDependencyBenchmarks, ::testing::Test
    {
        void SetUp() override
        {
            PathDependencyBase::Init();
        }
        void TearDown() override
        {
            PathDependencyBase::Destroy();
        }
    };

    TEST_F(PathDependencyTestValidation, DeferredWildcardDependencyResolution)
    {
        SetupTestData();
        DoTest();
        VerifyResult();
    }

    BENCHMARK_F(PathDependencyBenchmarksWrapperClass, BM_DeferredWildcardDependencyResolution)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto unused : state)
        {
            m_benchmarks->m_stateData->SetProductDependencies(m_benchmarks->m_dependencies);

            m_benchmarks->DoTest();
        }
    }

}
