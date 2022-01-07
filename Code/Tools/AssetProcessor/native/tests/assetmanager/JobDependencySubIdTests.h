/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/UnitTest/TestTypes.h>
#include <API/AssetDatabaseBus.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <native/AssetManager/assetProcessorManager.h>
#include <tests/UnitTestUtilities.h>
#include <Tests/Utils/Utils.h>

namespace UnitTests
{
    class DatabaseLocationListener : public AzToolsFramework::AssetDatabase::AssetDatabaseRequests::Bus::Handler
    {
    public:
        DatabaseLocationListener() { BusConnect(); }
        ~DatabaseLocationListener() override { BusDisconnect(); }

        bool GetAssetDatabaseLocation(AZStd::string& location) override;

        AZStd::string m_databaseLocation;
    };

    struct JobDependencySubIdTest;

    struct MockAssetProcessorManager : AssetProcessor::AssetProcessorManager
    {
        friend struct JobDependencySubIdTest;

        MockAssetProcessorManager(AssetProcessor::PlatformConfiguration* config, QObject* parent = nullptr)
            : AssetProcessorManager(config, parent) {}

        void CheckActiveFiles(int count);
        void CheckFilesToExamine(int count);
        void CheckJobEntries(int count);
    };

    struct JobDependencySubIdTest : UnitTest::ScopedAllocatorSetupFixture
    {
        void SetUp() override;
        void TearDown() override;
        void CreateTestData(AZ::u64 hashA, AZ::u64 hashB, bool useSubId);
        void RunTest(bool firstProductChanged, bool secondProductChanged);

        int m_argc = 0;
        char** m_argv{};

        AssetProcessor::FileStatePassthrough m_fileStateCache;

        AZStd::unique_ptr<QCoreApplication> m_qApp;
        AZStd::unique_ptr<MockAssetProcessorManager> m_assetProcessorManager;
        AZStd::unique_ptr<AssetProcessor::PlatformConfiguration> m_platformConfig;
        AZStd::unique_ptr<AZ::SettingsRegistryImpl> m_settingsRegistry;
        AZStd::shared_ptr<AssetProcessor::AssetDatabaseConnection> m_stateData;
        AZ::Test::ScopedAutoTempDirectory m_tempDir;
        DatabaseLocationListener m_databaseLocationListener;
        AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry m_scanfolder;
        AZ::IO::Path m_parentFile, m_childFile;
        MockBuilderInfoHandler m_builderInfoHandler;
        AZ::IO::LocalFileIO* m_localFileIo;

        AZ::Uuid m_assetType = AZ::Uuid::CreateName("test");
    };
}
