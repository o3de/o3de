/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Serialization/Json/JsonSerializationSettings.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Settings/SettingsRegistryVisitorUtils.h>
#include <AzCore/UnitTest/Mocks/MockSettingsRegistry.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/Utils.h>
#include <AzToolsFramework/Archive/ArchiveAPI.h>
#include <native/resourcecompiler/rcjob.h>
#include <native/utilities/AssetServerHandler.h>
#include <native/utilities/assetUtils.h>
#include <native/unittests/UnitTestUtils.h>
#include <QStandardPaths>
#include <QDir>

namespace UnitTest
{
    struct MockArchiveCommandsBusHandler
        : public AzToolsFramework::ArchiveCommandsBus::Handler
    {
        MockArchiveCommandsBusHandler()
        {
            AzToolsFramework::ArchiveCommandsBus::Handler::BusConnect();
        }

        ~MockArchiveCommandsBusHandler()
        {
            AzToolsFramework::ArchiveCommandsBus::Handler::BusDisconnect();
        }

        MOCK_METHOD2(CreateArchive, std::future<bool>(const AZStd::string&, const AZStd::string&));
        MOCK_METHOD2(ExtractArchive, std::future<bool>(const AZStd::string&, const AZStd::string&));
        MOCK_METHOD3(ExtractFile, std::future<bool>(const AZStd::string&, const AZStd::string&, const AZStd::string&));
        MOCK_METHOD2(ListFilesInArchive, bool(const AZStd::string&, AZStd::vector<AZStd::string>&));
        MOCK_METHOD3(AddFileToArchive, std::future<bool>(const AZStd::string&, const AZStd::string&, const AZStd::string&));
        MOCK_METHOD3(AddFilesToArchive, std::future<bool>(const AZStd::string&, const AZStd::string&, const AZStd::string&));
    };

    class AssetServerHandlerUnitTest
        : public AllocatorsFixture
    {

    public:
        AssetServerHandlerUnitTest()
        {
            AZ::SettingsRegistry::Register(&m_mockSettingsRegistry);
            m_tempFolder = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
            m_fakeSourceFile = QString(m_tempFolder + m_fakeFullname + ".zip");

            using namespace AssetProcessor;
            AssetServerBus::Broadcast(&AssetServerBus::Events::SetServerAddress, "");
            AssetServerBus::Broadcast(&AssetServerBus::Events::SetRemoteCachingMode, AssetServerMode::Inactive);
        }

        ~AssetServerHandlerUnitTest()
        {
            RemoveMockAssetArchive();
            using namespace AssetProcessor;
            AssetServerBus::Broadcast(&AssetServerBus::Events::SetServerAddress, "");
            AssetServerBus::Broadcast(&AssetServerBus::Events::SetRemoteCachingMode, AssetServerMode::Inactive);
            AZ::SettingsRegistry::Unregister(&m_mockSettingsRegistry);
        }

        void MockSettingsRegistry()
        {
            auto getString = [this](AZStd::string& result, AZStd::string_view input) -> bool
            {
                if (input == "/O3DE/AssetProcessor/Settings/Server/cacheServerAddress")
                {
                    result = this->m_tempFolder.toUtf8().toStdString().c_str();
                }
                return true;
            };
            ON_CALL(m_mockSettingsRegistry, Get(::testing::An<AZStd::string&>(), ::testing::_)).WillByDefault(getString);

            auto getBool = [this](bool& result, AZStd::string_view input) -> bool
            {
                if (input == "/O3DE/AssetProcessor/Settings/Server/enableCacheServer")
                {
                    result = this->m_enableServer;
                }
                return true;
            };
            ON_CALL(m_mockSettingsRegistry, Get(::testing::An<bool&>(), ::testing::_)).WillByDefault(getBool);
        }

        void CreateMockAssetArchive()
        {
            QFileInfo fileInfo(m_fakeSourceFile);
            if (!fileInfo.absoluteDir().exists())
            {
                EXPECT_TRUE(fileInfo.absoluteDir().mkpath("."));
            }
            QFile tempFile(m_fakeSourceFile);
            tempFile.open(QIODevice::OpenModeFlag::NewOnly);
        }

        void RemoveMockAssetArchive()
        {
            QFile tempFile(m_fakeSourceFile);
            tempFile.remove();
        }

        UnitTestUtils::AssertAbsorber m_assertAbsorber;
        AZ::MockSettingsRegistry m_mockSettingsRegistry;
        MockArchiveCommandsBusHandler m_mockArchiveCommandsBusHandler;
        QString m_tempFolder;
        QString m_fakeSourceFile;
        bool m_enableServer = false;
        const char* m_fakeFullname = "/mock_cache/asset_server_key";
        const char* m_fakeFilename = "asset_server_key";
    };

    TEST_F(AssetServerHandlerUnitTest, AssetCacheServer_UnConfiguredToRunAsServer_SetsFalse)
    {
        EXPECT_CALL(m_mockSettingsRegistry, Get(::testing::An<AZStd::string&>(), ::testing::_)).Times(2);
        EXPECT_CALL(m_mockSettingsRegistry, Get(::testing::An<bool&>(), ::testing::_)).Times(1);

        AssetProcessor::AssetServerHandler assetServerHandler;
        EXPECT_FALSE(assetServerHandler.IsServerAddressValid());
    }

    TEST_F(AssetServerHandlerUnitTest, AssetCacheServer_ConfiguredToRunAsServer_Works)
    {
        m_enableServer = true;
        MockSettingsRegistry();

        EXPECT_CALL(m_mockSettingsRegistry, Get(::testing::An<AZStd::string&>(), ::testing::_)).Times(2);
        EXPECT_CALL(m_mockSettingsRegistry, Get(::testing::An<bool&>(), ::testing::_)).Times(1);

        AssetProcessor::AssetServerHandler assetServerHandler;
        EXPECT_TRUE(assetServerHandler.IsServerAddressValid());
        EXPECT_TRUE(assetServerHandler.GetRemoteCachingMode() == AssetProcessor::AssetServerMode::Server);
    }

    TEST_F(AssetServerHandlerUnitTest, AssetCacheServer_ConfiguredToRunAsClient_Works)
    {
        m_enableServer = false;
        MockSettingsRegistry();

        EXPECT_CALL(m_mockSettingsRegistry, Get(::testing::An<AZStd::string&>(), ::testing::_)).Times(2);
        EXPECT_CALL(m_mockSettingsRegistry, Get(::testing::An<bool&>(), ::testing::_)).Times(1);

        AssetProcessor::AssetServerHandler assetServerHandler;
        EXPECT_TRUE(assetServerHandler.IsServerAddressValid());
        EXPECT_TRUE(assetServerHandler.GetRemoteCachingMode() == AssetProcessor::AssetServerMode::Client);
    }

    TEST_F(AssetServerHandlerUnitTest, AssetCacheServer_ServerStoresZipFile_Works)
    {
        m_enableServer = true;
        MockSettingsRegistry();
        RemoveMockAssetArchive();

        auto createArchive = [this](const AZStd::string& archivePath, const AZStd::string&) -> std::future<bool>
        {
            AZStd::string targetFilename = AZStd::string(this->m_fakeFilename) + ".zip";
            EXPECT_TRUE(archivePath.ends_with(targetFilename));
            return std::async(std::launch::async, [] { return true; });
        };
        ON_CALL(m_mockArchiveCommandsBusHandler, CreateArchive(::testing::_, ::testing::_)).WillByDefault(createArchive);

        EXPECT_CALL(m_mockSettingsRegistry, Get(::testing::An<AZStd::string&>(), ::testing::_)).Times(2);
        EXPECT_CALL(m_mockSettingsRegistry, Get(::testing::An<bool&>(), ::testing::_)).Times(1);
        EXPECT_CALL(m_mockArchiveCommandsBusHandler, CreateArchive(::testing::_, ::testing::_)).Times(1);

        QObject parent{};
        AssetProcessor::JobDetails jobDetails;
        jobDetails.m_jobEntry.m_jobKey = "ACS_Test";
        AssetProcessor::RCJob rcJob {&parent};
        rcJob.Init(jobDetails);
        AZ::ComponentDescriptor::StringWarningArray sourceFileList;
        AssetProcessor::BuilderParams builderParams {&rcJob};
        builderParams.m_serverKey = m_fakeFilename;
        builderParams.m_processJobRequest.m_sourceFile = (m_tempFolder + m_fakeFullname).toUtf8().toStdString().c_str();

        AssetProcessor::AssetServerHandler assetServerHandler;

        using namespace AssetProcessor;
        AssetServerMode mode = {};
        AssetServerBus::BroadcastResult(mode , &AssetServerBus::Events::GetRemoteCachingMode);

        EXPECT_TRUE(assetServerHandler.IsServerAddressValid());
        EXPECT_EQ(mode, AssetServerMode::Server);
        EXPECT_TRUE(assetServerHandler.StoreJobResult(builderParams, sourceFileList));
    }

    TEST_F(AssetServerHandlerUnitTest, AssetCacheServer_ClientReadsZipFile_Works)
    {
        m_enableServer = false;
        MockSettingsRegistry();
        CreateMockAssetArchive();

        auto extractArchive = [this](const AZStd::string& archivePath, const AZStd::string& tempFolder) -> std::future<bool>
        {
            AZStd::string targetFilename = AZStd::string(this->m_fakeFilename) + ".zip";
            EXPECT_TRUE(archivePath.ends_with(targetFilename));
            AZ_UNUSED(tempFolder);
            return std::async(std::launch::async, [] { return true; });
        };
        ON_CALL(m_mockArchiveCommandsBusHandler, ExtractArchive(::testing::_, ::testing::_)).WillByDefault(extractArchive);

        EXPECT_CALL(m_mockSettingsRegistry, Get(::testing::An<AZStd::string&>(), ::testing::_)).Times(2);
        EXPECT_CALL(m_mockSettingsRegistry, Get(::testing::An<bool&>(), ::testing::_)).Times(1);
        EXPECT_CALL(m_mockArchiveCommandsBusHandler, ExtractArchive(::testing::_, ::testing::_)).Times(1);

        QObject parent{};
        AssetProcessor::JobDetails jobDetails;
        jobDetails.m_jobEntry.m_jobKey = "ACS_Test";
        AssetProcessor::RCJob rcJob{ &parent };
        rcJob.Init(jobDetails);
        AssetProcessor::BuilderParams builderParams{ &rcJob };
        builderParams.m_serverKey = m_fakeFilename;
        builderParams.m_processJobRequest.m_sourceFile = (m_tempFolder + m_fakeFullname).toUtf8().toStdString().c_str();

        AssetProcessor::AssetServerHandler assetServerHandler;

        using namespace AssetProcessor;
        AssetServerMode mode = {};
        AssetServerBus::BroadcastResult(mode, &AssetServerBus::Events::GetRemoteCachingMode);

        EXPECT_TRUE(assetServerHandler.IsServerAddressValid());
        EXPECT_EQ(mode, AssetServerMode::Client);
        EXPECT_TRUE(assetServerHandler.RetrieveJobResult(builderParams));
    }
}
