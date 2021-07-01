/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzTest/AzTest.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include "native/tests/AssetProcessorTest.h"
#include "AzToolsFramework/API/AssetDatabaseBus.h"
#include "AssetDatabase/AssetDatabase.h"
#include "FileProcessor/FileProcessor.h"
#include "utilities/PlatformConfiguration.h"
#include <QCoreApplication>
#include <utilities/AssetUtilEBusHelper.h>

namespace UnitTests
{
    using namespace testing;
    using ::testing::NiceMock;
    using namespace AssetProcessor;

    using AzToolsFramework::AssetDatabase::ProductDatabaseEntry;
    using AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry;
    using AzToolsFramework::AssetDatabase::SourceDatabaseEntry;
    using AzToolsFramework::AssetDatabase::SourceFileDependencyEntry;
    using AzToolsFramework::AssetDatabase::SourceFileDependencyEntryContainer;
    using AzToolsFramework::AssetDatabase::JobDatabaseEntry;
    using AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer;
    using AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry;
    using AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer;
    using AzToolsFramework::AssetDatabase::AssetDatabaseConnection;
    using AzToolsFramework::AssetDatabase::FileDatabaseEntry;
    using AzToolsFramework::AssetDatabase::FileDatabaseEntryContainer;

    class FileProcessorTestsMockDatabaseLocationListener : public AzToolsFramework::AssetDatabase::AssetDatabaseRequests::Bus::Handler
    {
    public:
        MOCK_METHOD1(GetAssetDatabaseLocation, bool(AZStd::string&));
    };

    class FileProcessorTests 
        : public AssetProcessorTest,
        public ConnectionBus::Handler
    {
    public:
        void SetUp() override;
        void TearDown() override;

        //////////////////////////////////////////////////////////////////////////

        // Sends an unsolicited message to the connection
        size_t Send(unsigned int serial, const AzFramework::AssetSystem::BaseAssetProcessorMessage& message) override;
        // Sends a raw buffer to the connection
        size_t SendRaw([[maybe_unused]] unsigned int type, [[maybe_unused]] unsigned int serial, [[maybe_unused]] const QByteArray& data) override { return 0; };

        // Sends a message to the connection if the platform match
        size_t SendPerPlatform([[maybe_unused]] unsigned int serial, [[maybe_unused]] const AzFramework::AssetSystem::BaseAssetProcessorMessage& message, [[maybe_unused]] const QString& platform) override { return 0; };
        // Sends a raw buffer to the connection if the platform match
        size_t SendRawPerPlatform([[maybe_unused]] unsigned int type, [[maybe_unused]] unsigned int serial, [[maybe_unused]] const QByteArray& data, [[maybe_unused]] const QString& platform) override { return 0; };

        // Sends a message to the connection which expects a response.
        unsigned int SendRequest([[maybe_unused]] const AzFramework::AssetSystem::BaseAssetProcessorMessage& message, [[maybe_unused]] const ResponseCallback& callback) override { return 0; };

        // Sends a response to the connection
        size_t SendResponse([[maybe_unused]] unsigned int serial, [[maybe_unused]] const AzFramework::AssetSystem::BaseAssetProcessorMessage& message) override { return 0; };

        // Removes a response handler that is no longer needed
        void RemoveResponseHandler([[maybe_unused]] unsigned int serial) override {};

    protected:
        struct StaticData
        {
            QTemporaryDir m_temporaryDir;
            QDir m_temporarySourceDir;

            // these variables are created during SetUp() and destroyed during TearDown() and thus are always available during tests using this fixture:
            AZStd::string m_databaseLocation;
            NiceMock<FileProcessorTestsMockDatabaseLocationListener> m_databaseLocationListener;
            AssetProcessor::AssetDatabaseConnection m_connection;

            AZStd::unique_ptr<AssetProcessor::PlatformConfiguration> m_config;

            // The following database entry variables are initialized only when you call coverage test data CreateCoverageTestData().
            // Tests which don't need or want a pre-made database should not call CreateCoverageTestData() but note that in that case
            // these entries will be empty and their identifiers will be -1.
            ScanFolderDatabaseEntry m_scanFolder;

            AZStd::unique_ptr<FileProcessor> m_fileProcessor;

            FileDatabaseEntryContainer m_fileEntries;
            QCoreApplication m_coreApp;
            int m_argc = 0;
            int m_messagesSent = 0;

            StaticData() : m_coreApp(m_argc, nullptr)
            {

            }
        };

        // we store the above data in a unique_ptr so that its memory can be cleared during TearDown() in one call, before we destroy the memory
        // allocator, reducing the chance of missing or forgetting to destroy one in the future.
        AZStd::unique_ptr<StaticData> m_data;
    };
}
