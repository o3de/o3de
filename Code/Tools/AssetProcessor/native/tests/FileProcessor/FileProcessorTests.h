/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzTest/AzTest.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <native/tests/AssetProcessorTest.h>
#include <native/tests/MockAssetDatabaseRequestsHandler.h>
#include "AzToolsFramework/API/AssetDatabaseBus.h"
#include "AssetDatabase/AssetDatabase.h"
#include "FileProcessor/FileProcessor.h"
#include "utilities/PlatformConfiguration.h"
#include <QCoreApplication>
#include <utilities/AssetUtilEBusHelper.h>

namespace UnitTests
{
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

    class FileProcessorTests
        : public AssetProcessorTest,
        public ConnectionBus::Handler
    {
    public:
        FileProcessorTests() : m_coreApp(m_argc, nullptr), AssetProcessorTest()
        {

        }


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
        QDir m_assetRootSourceDir;

        AZStd::string m_databaseLocation;
        AssetProcessor::MockAssetDatabaseRequestsHandler m_databaseLocationListener;
        AssetProcessor::AssetDatabaseConnection m_connection;

        AZStd::unique_ptr<AssetProcessor::PlatformConfiguration> m_config;

        ScanFolderDatabaseEntry m_scanFolder;
        ScanFolderDatabaseEntry m_scanFolder2;

        AZStd::unique_ptr<FileProcessor> m_fileProcessor;

        FileDatabaseEntryContainer m_fileEntries;
        QCoreApplication m_coreApp;
        int m_argc = 0;
        int m_messagesSent = 0;
    };
}
