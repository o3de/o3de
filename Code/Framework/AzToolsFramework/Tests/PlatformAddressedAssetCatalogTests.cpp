/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzToolsFramework/AssetCatalog/PlatformAddressedAssetCatalogManager.h>
#include <AzFramework/Asset/AssetRegistry.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/IO/FileIO.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <Tests/AZTestShared/Utils/Utils.h>
#include <AzToolsFramework/UnitTest/ToolsTestApplication.h>
#include <AzFramework/Platform/PlatformDefaults.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzTest/AzTest.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <Utils/Utils.h>

namespace 
{
    static const int s_totalAssets = 12;
}

namespace UnitTest
{
    class PlatformAddressedAssetCatalogManagerTest
        : public AllocatorsFixture
    {
    public:

        void SetUp() override
        {
            using namespace AZ::Data;
            m_application = new ToolsTestApplication("AddressedAssetCatalogManager"); // Shorter name because Setting Registry
                                                                                      // specialization are 32 characters max.

            AZ::SettingsRegistryInterface* registry = AZ::SettingsRegistry::Get();

            auto projectPathKey =
                AZ::SettingsRegistryInterface::FixedValueString(AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey) + "/project_path";
            registry->Set(projectPathKey, "AutomatedTesting");
            AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddRuntimeFilePaths(*registry);

            m_application->Start(AzFramework::Application::Descriptor());
            // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
            // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash 
            // in the unit tests.
            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);

            const AZStd::string cacheProjectRootFolder = m_tempDir.GetDirectory();
            registry->Set(AZ::SettingsRegistryMergeUtils::FilePathKey_CacheProjectRootFolder, cacheProjectRootFolder);
            AZ::IO::FileIOBase::GetInstance()->SetAlias("@assets@", cacheProjectRootFolder.c_str());

            for (int platformNum = AzFramework::PlatformId::PC; platformNum < AzFramework::PlatformId::NumPlatformIds; ++platformNum)
            {
                const AZStd::string platformName{ AzFramework::PlatformHelper::GetPlatformName(static_cast<AzFramework::PlatformId>(platformNum)) };
                if (!platformName.length())
                {
                    // Do not test disabled platforms
                    continue;
                }

                AZStd::unique_ptr<AzFramework::AssetRegistry> assetRegistry = AZStd::make_unique<AzFramework::AssetRegistry>();
                for (int idx = 0; idx < s_totalAssets; idx++)
                {
                    m_assets[platformNum][idx] = AssetId(AZ::Uuid::CreateRandom(), 0);
                    AZ::Data::AssetInfo info;
                    info.m_relativePath = AZStd::string::format("%s%sAsset%d.txt", platformName.c_str(), AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING, idx);
                    info.m_assetId = m_assets[platformNum][idx];
                    assetRegistry->RegisterAsset(m_assets[platformNum][idx], info);
                    m_assetsPath[platformNum][idx] = cacheProjectRootFolder + AZ_CORRECT_FILESYSTEM_SEPARATOR + info.m_relativePath;
                    AZ_TEST_START_TRACE_SUPPRESSION;
                    if (m_fileStreams[platformNum][idx].Open(m_assetsPath[platformNum][idx].c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeBinary | AZ::IO::OpenMode::ModeCreatePath))
                    {
                        AZ::IO::SizeType bytesWritten = m_fileStreams[platformNum][idx].Write(info.m_relativePath.size(), info.m_relativePath.data());
                        EXPECT_EQ(bytesWritten, info.m_relativePath.size());
                        m_fileStreams[platformNum][idx].Close();
                        AZ_TEST_STOP_TRACE_SUPPRESSION(1); // writing to asset cache folder
                    }
                    else
                    {
                        GTEST_FATAL_FAILURE_(AZStd::string::format("Unable to create temporary file ( %s ) in PlatformAddressedAssetCatalogManagerTest unit tests.\n", m_assetsPath[platformNum][idx].c_str()).c_str());
                    }
                }

                bool useRequestBus = false;
                AzFramework::AssetCatalog assetCatalog(useRequestBus);
                AZStd::string catalogPath = AzToolsFramework::PlatformAddressedAssetCatalog::GetCatalogRegistryPathForPlatform(static_cast<AzFramework::PlatformId>(platformNum));

                if (!assetCatalog.SaveCatalog(catalogPath.c_str(), assetRegistry.get()))
                {
                    GTEST_FATAL_FAILURE_(AZStd::string::format("Unable to save the asset catalog file.\n").c_str());
                }
            }


            m_PlatformAddressedAssetCatalogManager = new AzToolsFramework::PlatformAddressedAssetCatalogManager();
        }

        void TearDown() override
        {
            AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
            for (int platformNum = AzFramework::PlatformId::PC; platformNum < AzFramework::PlatformId::NumPlatformIds; ++platformNum)
            {
                AZStd::string platformName{ AzFramework::PlatformHelper::GetPlatformName(static_cast<AzFramework::PlatformId>(platformNum)) };
                if (!platformName.length())
                {
                    // Do not test disabled platforms
                    continue;
                }
                AZStd::string catalogPath = AzToolsFramework::PlatformAddressedAssetCatalog::GetCatalogRegistryPathForPlatform(static_cast<AzFramework::PlatformId>(platformNum));

                if (fileIO->Exists(catalogPath.c_str()))
                {
                    AZ_TEST_START_TRACE_SUPPRESSION;
                    AZ::IO::Result result = fileIO->Remove(catalogPath.c_str());
                    EXPECT_EQ(result.GetResultCode(), AZ::IO::ResultCode::Success);
                    AZ_TEST_STOP_TRACE_SUPPRESSION(1); // removing from asset cache folder
                }
                // Deleting all the temporary files
                for (int idx = 0; idx < s_totalAssets; idx++)
                {
                    if (fileIO->Exists(m_assetsPath[platformNum][idx].c_str()))
                    {
                        AZ_TEST_START_TRACE_SUPPRESSION;
                        AZ::IO::Result result = fileIO->Remove(m_assetsPath[platformNum][idx].c_str());
                        EXPECT_EQ(result.GetResultCode(), AZ::IO::ResultCode::Success);
                        AZ_TEST_STOP_TRACE_SUPPRESSION(1); // removing from asset cache folder
                    }
                }
            }

            delete m_localFileIO;
            m_localFileIO = nullptr;
            AZ::IO::FileIOBase::SetInstance(m_priorFileIO);
            delete m_PlatformAddressedAssetCatalogManager;
            m_application->Stop();
            delete m_application;
        }


        AzToolsFramework::PlatformAddressedAssetCatalogManager* m_PlatformAddressedAssetCatalogManager;
        ToolsTestApplication* m_application;
        UnitTest::ScopedTemporaryDirectory m_tempDir;
        AZ::IO::FileIOBase* m_priorFileIO = nullptr;
        AZ::IO::FileIOBase* m_localFileIO = nullptr;
        AZ::IO::FileIOStream m_fileStreams[AzFramework::PlatformId::NumPlatformIds][s_totalAssets];

        AZ::Data::AssetId m_assets[AzFramework::PlatformId::NumPlatformIds][s_totalAssets];
        AZStd::string m_assetsPath[AzFramework::PlatformId::NumPlatformIds][s_totalAssets];
    };

    TEST_F(PlatformAddressedAssetCatalogManagerTest, PlatformAddressedAssetCatalogManager_AllCatalogsLoaded_Success)
    {
        for (int platformNum = AzFramework::PlatformId::PC; platformNum < AzFramework::PlatformId::NumPlatformIds; ++platformNum)
        {
            AZStd::string platformName{ AzFramework::PlatformHelper::GetPlatformName(static_cast<AzFramework::PlatformId>(platformNum)) };
            if (!platformName.length())
            {
                // Do not test disabled platforms
                continue;
            }
            for (int assetNum = 0; assetNum < s_totalAssets; ++assetNum)
            {

                AZ::Data::AssetInfo assetInfo;
                AzToolsFramework::AssetCatalog::PlatformAddressedAssetCatalogRequestBus::EventResult(assetInfo, static_cast<AzFramework::PlatformId>(platformNum), &AZ::Data::AssetCatalogRequests::GetAssetInfoById, m_assets[platformNum][assetNum]);

                EXPECT_EQ(m_assets[platformNum][assetNum], assetInfo.m_assetId);
            }
        }
    }

    TEST_F(PlatformAddressedAssetCatalogManagerTest, PlatformAddressedAssetCatalogManager_CatalogExistsChecks_Success)
    {
        EXPECT_EQ(AzToolsFramework::PlatformAddressedAssetCatalog::CatalogExists(AzFramework::PlatformId::ANDROID_ID), true);
        AZStd::string androidCatalogPath = AzToolsFramework::PlatformAddressedAssetCatalog::GetCatalogRegistryPathForPlatform(AzFramework::PlatformId::ANDROID_ID);
        if (AZ::IO::FileIOBase::GetInstance()->Exists(androidCatalogPath.c_str()))
        {
            AZ_TEST_START_TRACE_SUPPRESSION;
            AZ::IO::Result result = AZ::IO::FileIOBase::GetInstance()->Remove(androidCatalogPath.c_str());
            EXPECT_EQ(result.GetResultCode(), AZ::IO::ResultCode::Success);
            AZ_TEST_STOP_TRACE_SUPPRESSION(1); // removing from asset cache folder
        }
        EXPECT_EQ(AzToolsFramework::PlatformAddressedAssetCatalog::CatalogExists(AzFramework::PlatformId::ANDROID_ID), false);
    }

    class PlatformAddressedAssetCatalogMessageTest : public AzToolsFramework::PlatformAddressedAssetCatalog
    {
    public:
        PlatformAddressedAssetCatalogMessageTest(AzFramework::PlatformId platformId) :  AzToolsFramework::PlatformAddressedAssetCatalog(platformId)
        {

        }
        MOCK_METHOD1(AssetChanged, void(AzFramework::AssetSystem::AssetNotificationMessage message));
        MOCK_METHOD1(AssetRemoved, void(AzFramework::AssetSystem::AssetNotificationMessage message));
    };

    class PlatformAddressedAssetCatalogManagerMessageTest : public AzToolsFramework::PlatformAddressedAssetCatalogManager
    {
    public:
        PlatformAddressedAssetCatalogManagerMessageTest(AzFramework::PlatformId platformId) :
            AzToolsFramework::PlatformAddressedAssetCatalogManager(AzFramework::PlatformId::Invalid)
        {
            TakeSingleCatalog(AZStd::make_unique<PlatformAddressedAssetCatalogMessageTest>(platformId));
        }
    };

    class MessageTest
        : public AllocatorsFixture
    {
    public:
        void SetUp() override
        {
            m_application = new ToolsTestApplication("MessageTest");

            AZ::IO::FileIOBase::SetInstance(nullptr); // The API requires the old instance to be destroyed first
            AZ::IO::FileIOBase::SetInstance(new AZ::IO::LocalFileIO());

            auto registry = AZ::SettingsRegistry::Get();
            ASSERT_TRUE(registry != nullptr);

            const AZStd::string cacheProjectRootFolder = m_tempDir.GetDirectory();
            registry->Set(AZ::SettingsRegistryMergeUtils::FilePathKey_CacheProjectRootFolder, cacheProjectRootFolder);
            AZ::IO::FileIOBase::GetInstance()->SetAlias("@assets@", cacheProjectRootFolder.c_str());

            m_platformAddressedAssetCatalogManager = AZStd::make_unique<AzToolsFramework::PlatformAddressedAssetCatalogManager>(AzFramework::PlatformId::Invalid);
        }
        void TearDown() override
        {
            m_platformAddressedAssetCatalogManager.reset();
            delete m_application;
        }
        ToolsTestApplication* m_application;
        AZStd::unique_ptr<AzToolsFramework::PlatformAddressedAssetCatalogManager> m_platformAddressedAssetCatalogManager;
        UnitTest::ScopedTemporaryDirectory m_tempDir;
    };

    TEST_F(MessageTest, PlatformAddressedAssetCatalogManagerMessageTest_MessagesForwarded_CountsMatch)
    {
        AzFramework::AssetSystem::AssetNotificationMessage testMessage;
        AzFramework::AssetSystem::NetworkAssetUpdateInterface* notificationInterface = AZ::Interface<AzFramework::AssetSystem::NetworkAssetUpdateInterface>::Get();
        EXPECT_NE(notificationInterface, nullptr);

        AZ_TEST_START_TRACE_SUPPRESSION;
        auto* mockCatalog = new ::testing::NiceMock<PlatformAddressedAssetCatalogMessageTest>(AzFramework::PlatformId::ANDROID_ID);
        AZ_TEST_STOP_TRACE_SUPPRESSION(1); // Expected error not finding catalog
        AZStd::unique_ptr< ::testing::NiceMock<PlatformAddressedAssetCatalogMessageTest>> catalogHolder;
        catalogHolder.reset(mockCatalog);

        m_platformAddressedAssetCatalogManager->TakeSingleCatalog(AZStd::move(catalogHolder));
        EXPECT_CALL(*mockCatalog, AssetChanged(testing::_)).Times(0);
        notificationInterface->AssetChanged(testMessage);

        testMessage.m_platform = "android";
        EXPECT_CALL(*mockCatalog, AssetChanged(testing::_)).Times(1);
        notificationInterface->AssetChanged(testMessage);

        testMessage.m_platform = "pc";
        EXPECT_CALL(*mockCatalog, AssetChanged(testing::_)).Times(0);
        notificationInterface->AssetChanged(testMessage);

        EXPECT_CALL(*mockCatalog, AssetRemoved(testing::_)).Times(0);
        notificationInterface->AssetRemoved(testMessage);

        testMessage.m_platform = "android";
        EXPECT_CALL(*mockCatalog, AssetRemoved(testing::_)).Times(1);
        notificationInterface->AssetRemoved(testMessage);
    }

}
