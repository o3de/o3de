/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <tests/assetmanager/AssetProcessorManagerTest.h>

namespace UnitTests
{
    struct ModtimeScanningTest : AssetProcessorManagerTest
    {
        void SetUpAssetProcessorManager();
        void PopulateDatabase() override;
        void SetUp() override;
        void TearDown() override;

        void ProcessAssetJobs();
        void SimulateAssetScanner(QSet<AssetProcessor::AssetFileInfo> filePaths);
        QSet<AssetProcessor::AssetFileInfo> BuildFileSet();
        void ExpectWork(int createJobs, int processJobs);
        void ExpectNoWork();
        void SetFileContents(QString filePath, QString contents);

        struct StaticData
        {
            AZStd::vector<AssetProcessor::SourceAssetReference> m_sourcePaths;
            AZStd::vector<AssetProcessor::JobDetails> m_processResults;
            AZStd::unordered_multimap<AZStd::string, QString> m_productPaths;
            AZStd::vector<AssetProcessor::SourceAssetReference> m_deletedSources;
            AZStd::shared_ptr<AssetProcessor::InternalMockBuilder> m_builderTxtBuilder;
            MockMultiBuilderInfoHandler m_mockBuilderInfoHandler;
        };

        AZStd::unique_ptr<StaticData> m_data;
    };

    struct DeleteTest : ModtimeScanningTest
    {
        void SetUp() override;
    };


    struct LockedFileTest
        : ModtimeScanningTest
        , AssetProcessor::ConnectionBus::Handler
    {
        MOCK_METHOD3(SendRaw, size_t(unsigned, unsigned, const QByteArray&));
        MOCK_METHOD3(SendPerPlatform, size_t(unsigned, const AzFramework::AssetSystem::BaseAssetProcessorMessage&, const QString&));
        MOCK_METHOD4(SendRawPerPlatform, size_t(unsigned, unsigned, const QByteArray&, const QString&));
        MOCK_METHOD2(SendRequest, unsigned(const AzFramework::AssetSystem::BaseAssetProcessorMessage&, const ResponseCallback&));
        MOCK_METHOD2(SendResponse, size_t(unsigned, const AzFramework::AssetSystem::BaseAssetProcessorMessage&));
        MOCK_METHOD1(RemoveResponseHandler, void(unsigned));

        size_t Send(unsigned, const AzFramework::AssetSystem::BaseAssetProcessorMessage& message) override
        {
            using SourceFileNotificationMessage = AzToolsFramework::AssetSystem::SourceFileNotificationMessage;
            switch (message.GetMessageType())
            {
            case SourceFileNotificationMessage::MessageType:
                if (const auto sourceFileMessage = azrtti_cast<const SourceFileNotificationMessage*>(&message);
                    sourceFileMessage != nullptr &&
                    sourceFileMessage->m_type == SourceFileNotificationMessage::NotificationType::FileRemoved)
                {
                    // The File Remove message will occur before an attempt to delete the file
                    // Wait for more than 1 File Remove message.
                    // This indicates the AP has attempted to delete the file once, failed to do so and is now retrying
                    ++m_deleteCounter;

                    if (m_deleteCounter > 1 && m_callback)
                    {
                        m_callback();
                        m_callback = {}; // Unset it to be safe, we only intend to run the callback once
                    }
                }
                break;
            default:
                break;
            }

            return 0;
        }

        void SetUp() override
        {
            ModtimeScanningTest::SetUp();

            AssetProcessor::ConnectionBus::Handler::BusConnect(0);
        }

        void TearDown() override
        {
            AssetProcessor::ConnectionBus::Handler::BusDisconnect();

            ModtimeScanningTest::TearDown();
        }

        AZStd::atomic_int m_deleteCounter{ 0 };
        AZStd::function<void()> m_callback;
    };
}
