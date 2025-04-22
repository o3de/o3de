/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QCoreApplication>
#include <native/tests/assetmanager/SourceDependencyTests.h>
#include <native/unittests/UnitTestUtils.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzCore/Utils/Utils.h>

namespace UnitTests
{
    void SourceDependencyTests::SetUp()
    {
        AssetManagerTestingBase::SetUp();

        AZ::Data::AssetId::Reflect(m_serializeContext.get());
        AZ::Data::AssetData::Reflect(m_serializeContext.get());

        using namespace AssetBuilderSDK;

        m_uuidInterface = AZ::Interface<AssetProcessor::IUuidRequests>::Get();
        ASSERT_TRUE(m_uuidInterface);

        m_uuidInterface->EnableGenerationForTypes({ ".stage1" });

        m_assetProcessorManager->SetMetaCreationDelay(MetadataProcessingDelayMs);

        CreateBuilder("stage1", "*.stage1", "stage2", false, ProductOutputFlags::ProductAsset);
        ProcessFileMultiStage(1, true);
        QCoreApplication::processEvents();
    }

    TEST_F(SourceDependencyTests, ExistingSourceAndProductDependency_OnCatalogStartup_LegacyUuidsUpgraded)
    {
        // Test that source and product dependencies using legacy UUIDs are upgraded during catalog setup
        using namespace AssetProcessor;
        using namespace AzToolsFramework::AssetDatabase;

        SourceAssetReference testA = SourceAssetReference(m_testFilePath.c_str());

        AZ::IO::Path scanFolderDir(m_scanfolder.m_scanFolder);
        AZStd::string testFilename = "testB.stage1";
        SourceAssetReference testB(scanFolderDir / testFilename);

        AZ::Utils::WriteFile("unit test file", testB.AbsolutePath().c_str());

        auto testAUuids = m_uuidInterface->GetLegacyUuids(testA);
        ASSERT_TRUE(testAUuids);

        auto testBUuids = m_uuidInterface->GetLegacyUuids(testB);
        ASSERT_TRUE(testBUuids);

        // Inject a source dependency into the db using legacy UUIDs on both ends
        // A depends on -> B
        auto builderId = AZ::Uuid::CreateRandom();
        SourceFileDependencyEntry dep{ builderId,
                                       *testAUuids.GetValue().begin(),
                                       PathOrUuid(*testBUuids.GetValue().begin()),
                                       SourceFileDependencyEntry::DEP_SourceToSource,
                                       true,
                                       "" };

        ASSERT_TRUE(m_stateData->SetSourceFileDependency(dep));

        ProductDatabaseEntryContainer products;
        EXPECT_TRUE(m_stateData->GetProductsBySourceNameScanFolderID(testA.RelativePath().c_str(), testA.ScanFolderId(), products));
        ASSERT_EQ(products.size(), 1);

        // Inject a product dependency with B's legacy UUID.  A depends on -> B
        ProductDependencyDatabaseEntry productDep{ products[0].m_productID, *testBUuids.GetValue().begin(), 0, {}, "pc", 1 };
        EXPECT_TRUE(m_stateData->SetProductDependency(productDep));

        // Run the catalog startup which handles the updating
        auto catalog = AZStd::make_unique<AssetCatalog>(nullptr, m_platformConfig.get());
        catalog->BuildRegistry();

        // Check that the source dependency db entry has been updated
        auto testAUuid = m_uuidInterface->GetUuid(testA).GetValue();
        auto testBUuid = m_uuidInterface->GetUuid(testB).GetValue();

        SourceFileDependencyEntryContainer updatedEntry;
        EXPECT_TRUE(m_stateData->GetSourceFileDependenciesByBuilderGUIDAndSource(builderId, testAUuid, SourceFileDependencyEntry::DEP_Any, updatedEntry));

        ASSERT_EQ(updatedEntry.size(), 1);
        EXPECT_STREQ(updatedEntry[0].m_dependsOnSource.ToString().c_str(), testBUuid.ToFixedString(false, false).c_str());
        EXPECT_STREQ(updatedEntry[0].m_sourceGuid.ToFixedString().c_str(), testAUuid.ToFixedString().c_str());

        // Check that the product dependency db entry has been updated
        ProductDependencyDatabaseEntryContainer productDependencies;
        EXPECT_TRUE(m_stateData->GetProductDependencies(productDependencies));
        ASSERT_EQ(productDependencies.size(), 1);

        EXPECT_STREQ(productDependencies[0].m_dependencySourceGuid.ToFixedString().c_str(), testBUuid.ToFixedString().c_str());
    }

    TEST_F(SourceDependencyTests, NewlyCreatedSourceAndProductDependency_UpgradedBeforeSaving)
    {
        // Test that a Source dependency using a legacy UUID reference is upgraded before being saved to the database during processing
        using namespace AssetProcessor;
        using namespace AzToolsFramework::AssetDatabase;
        using namespace AssetBuilderSDK;

        SourceAssetReference testA = SourceAssetReference(m_testFilePath.c_str());

        AZ::IO::Path scanFolderDir(m_scanfolder.m_scanFolder);
        AZStd::string testFilename = "testB.src";
        SourceAssetReference testB(scanFolderDir / testFilename);

        AZ::Utils::WriteFile("unit test file", testB.AbsolutePath().c_str());

        auto testAUuids = m_uuidInterface->GetLegacyUuids(testA);
        ASSERT_TRUE(testAUuids);

        auto testBUuids = m_uuidInterface->GetLegacyUuids(testB);
        ASSERT_TRUE(testBUuids);

        // Builder which will say B depends on legacy A with a source dependency and a product dependency
        const auto testASubId = 0;
        auto builderId = AZ::Uuid::CreateRandom();
        m_builderInfoHandler.CreateBuilderDesc(
            "DependencyBuilder",
            builderId.ToFixedString().c_str(),
            { AssetBuilderPattern{ "*.src", AssetBuilderPattern::Wildcard } },
            CreateJobStage("DependencyBuilder", false, PathOrUuid(*testAUuids.GetValue().begin())),
            ProcessJobStage("bin", ProductOutputFlags::ProductAsset, false, AZ::Data::AssetId(*testAUuids.GetValue().begin(), testASubId)),
            "fingerprint");

        // Process the file
        ProcessFileMultiStage(1, false, testB);

        // Fetch and check the source dependency
        SourceFileDependencyEntryContainer dependencies;
        EXPECT_TRUE(m_stateData->GetSourceFileDependenciesByBuilderGUIDAndSource(
            builderId, m_uuidInterface->GetUuid(testB).GetValue(), SourceFileDependencyEntry::DEP_Any, dependencies));

        auto testAUuid = m_uuidInterface->GetUuid(testA).GetValue();

        ASSERT_EQ(dependencies.size(), 1);
        EXPECT_STREQ(
            dependencies[0].m_dependsOnSource.GetUuid().ToFixedString().c_str(),
            testAUuid.ToFixedString().c_str());

        // Fetch and check the product dependency
        ProductDependencyDatabaseEntryContainer productDependencies;
        EXPECT_TRUE(m_stateData->GetProductDependencies(productDependencies));

        ASSERT_EQ(productDependencies.size(), 1);
        EXPECT_STREQ(productDependencies[0].m_dependencySourceGuid.ToFixedString().c_str(), testAUuid.ToFixedString().c_str());
    }
} // namespace UnitTests
