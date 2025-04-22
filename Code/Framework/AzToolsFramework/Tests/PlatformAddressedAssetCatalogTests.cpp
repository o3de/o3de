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
#include <AZTestShared/Utils/Utils.h>
#include <AzToolsFramework/UnitTest/ToolsTestApplication.h>
#include <AzFramework/Platform/PlatformDefaults.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzTest/AzTest.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>

namespace
{
    static const int s_totalAssets = 12;
}

namespace UnitTest
{
    class PlatformAddressedAssetCatalogManagerTest
        : public LeakDetectionFixture
    {
    public:

        void SetUp() override
        {
            using namespace AZ::Data;
            constexpr size_t MaxCommandArgsCount = 128;
            using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;
            using ArgumentContainer = AZStd::fixed_vector<char*, MaxCommandArgsCount>;
            // The first command line argument is assumed to be the executable name so add a blank entry for it
            ArgumentContainer argContainer{ {} };

            // Append Command Line override for the Project Cache Path
            auto cacheProjectRootFolder = AZ::IO::Path{ m_tempDir.GetDirectory() } / "Cache";
            auto projectPathOverride = FixedValueString::format(R"(--project-path="%s")", m_tempDir.GetDirectory());
            argContainer.push_back(projectPathOverride.data());
            m_application = new ToolsTestApplication("AddressedAssetCatalogManager", aznumeric_caster(argContainer.size()), argContainer.data());

            AZ::ComponentApplication::StartupParameters startupParameters;
            startupParameters.m_loadSettingsRegistry = false;
            m_application->Start(AzFramework::Application::Descriptor(), startupParameters);
            // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
            // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash
            // in the unit tests.
            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);

            // By default @products@ is setup to include the platform at the end. But this test is going to
            // loop over all platforms and it will be included as part of the relative path of the file.
            // So the asset folder for these tests have to point to the cache project root folder, which
            // doesn't include the platform.
            AZ::IO::FileIOBase::GetInstance()->SetAlias("@products@", cacheProjectRootFolder.c_str());

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
                    info.m_relativePath = AZStd::move((AZ::IO::Path(platformName) / AZStd::string::format("Asset%d.txt", idx)).Native());
                    info.m_assetId = m_assets[platformNum][idx];
                    assetRegistry->RegisterAsset(m_assets[platformNum][idx], info);
                    m_assetsPath[platformNum][idx] = AZStd::move((cacheProjectRootFolder / info.m_relativePath).Native());
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
            delete m_PlatformAddressedAssetCatalogManager;
            m_application->Stop();
            delete m_application;
        }


        AzToolsFramework::PlatformAddressedAssetCatalogManager* m_PlatformAddressedAssetCatalogManager = nullptr;
        ToolsTestApplication* m_application = nullptr;
        AZ::Test::ScopedAutoTempDirectory m_tempDir;
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
        AZ_CLASS_ALLOCATOR(PlatformAddressedAssetCatalogMessageTest, AZ::SystemAllocator)
        PlatformAddressedAssetCatalogMessageTest(AzFramework::PlatformId platformId) :  AzToolsFramework::PlatformAddressedAssetCatalog(platformId)
        {

        }
        MOCK_METHOD2(AssetChanged, void(const AZStd::vector<AzFramework::AssetSystem::AssetNotificationMessage>& message, bool isCatalogInitialize));
        MOCK_METHOD1(AssetRemoved, void(const AZStd::vector<AzFramework::AssetSystem::AssetNotificationMessage>& message));
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
        : public LeakDetectionFixture
    {
    public:
        void SetUp() override
        {
            constexpr size_t MaxCommandArgsCount = 128;
            using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;
            using ArgumentContainer = AZStd::fixed_vector<char*, MaxCommandArgsCount>;
            // The first command line argument is assumed to be the executable name so add a blank entry for it
            ArgumentContainer argContainer{ {} };

            // Append Command Line override for the Project Cache Path
            auto projectPathOverride = FixedValueString::format(R"(--project-path="%s")", m_tempDir.GetDirectory());
            argContainer.push_back(projectPathOverride.data());
            m_application = new ToolsTestApplication("MessageTest", aznumeric_caster(argContainer.size()), argContainer.data());

            m_platformAddressedAssetCatalogManager = AZStd::make_unique<AzToolsFramework::PlatformAddressedAssetCatalogManager>(AzFramework::PlatformId::Invalid);
        }
        void TearDown() override
        {
            m_platformAddressedAssetCatalogManager.reset();
            delete m_application;
        }
        ToolsTestApplication* m_application = nullptr;
        AZStd::unique_ptr<AzToolsFramework::PlatformAddressedAssetCatalogManager> m_platformAddressedAssetCatalogManager;
        AZ::Test::ScopedAutoTempDirectory m_tempDir;
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
        EXPECT_CALL(*mockCatalog, AssetChanged(testing::_, false)).Times(0);
        notificationInterface->AssetChanged({ testMessage });

        testMessage.m_platform = "android";
        EXPECT_CALL(*mockCatalog, AssetChanged(testing::_, false)).Times(1);
        notificationInterface->AssetChanged({ testMessage });

        testMessage.m_platform = "pc";
        EXPECT_CALL(*mockCatalog, AssetChanged(testing::_, false)).Times(0);
        notificationInterface->AssetChanged({ testMessage });

        EXPECT_CALL(*mockCatalog, AssetRemoved(testing::_)).Times(0);
        notificationInterface->AssetRemoved({ testMessage });

        testMessage.m_platform = "android";
        EXPECT_CALL(*mockCatalog, AssetRemoved(testing::_)).Times(1);
        notificationInterface->AssetRemoved({ testMessage });
    }

}
