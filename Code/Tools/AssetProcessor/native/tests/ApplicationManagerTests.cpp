/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ApplicationManagerTests.h"

#include <QCoreApplication>
#include <tests/assetmanager/MockAssetProcessorManager.h>
#include <tests/assetmanager/MockFileProcessor.h>

namespace UnitTests
{
    bool DatabaseLocationListener::GetAssetDatabaseLocation(AZStd::string& location)
    {
        location = m_databaseLocation;
        return true;
    }

    void ApplicationManagerTest::SetUp()
    {
        ScopedAllocatorSetupFixture::SetUp();
        
        AZ::IO::Path tempDir(m_tempDir.GetDirectory());
        m_databaseLocationListener.m_databaseLocation = (tempDir / "test_database.sqlite").Native();

        // We need a QCoreApplication to run the event loop
        int argc = 0;
        m_coreApplication = AZStd::make_unique<QCoreApplication>(argc, nullptr);

        m_applicationManager = AZStd::make_unique<MockBatchApplicationManager>(&argc, nullptr);
        m_applicationManager->m_platformConfiguration = new AssetProcessor::PlatformConfiguration{ nullptr };
        m_applicationManager->m_fileStateCache = AZStd::make_unique<AssetProcessor::FileStateCache>();

        m_mockAPM = AZStd::make_unique<MockAssetProcessorManager>(m_applicationManager->m_platformConfiguration, nullptr);
        m_applicationManager->m_assetProcessorManager = m_mockAPM.get();

        AZStd::vector<AssetBuilderSDK::PlatformInfo> platforms;
        m_applicationManager->m_platformConfiguration->EnablePlatform(AssetBuilderSDK::PlatformInfo{ "pc", { "tag" } });
        m_applicationManager->m_platformConfiguration->PopulatePlatformsForScanFolder(platforms);
        m_applicationManager->m_platformConfiguration->AddScanFolder(
            AssetProcessor::ScanFolderInfo{ tempDir.c_str(), "test", "test", true, true, platforms });
        
        m_apmThread = AZStd::make_unique<QThread>(nullptr);
        m_apmThread->setObjectName("APM Thread");
        m_applicationManager->m_assetProcessorManager->moveToThread(m_apmThread.get());
        m_apmThread->start();

        m_fileProcessorThread = AZStd::make_unique<QThread>(nullptr);
        m_fileProcessorThread->setObjectName("File Processor Thread");
        auto fileProcessor = AZStd::make_unique<MockFileProcessor>(m_applicationManager->m_platformConfiguration);
        fileProcessor->moveToThread(m_fileProcessorThread.get());
        m_mockFileProcessor = fileProcessor.get();
        m_applicationManager->m_fileProcessor = AZStd::move(fileProcessor); // The manager is taking ownership
        m_fileProcessorThread->start();

        auto fileWatcher = AZStd::make_unique<FileWatcher>();
        m_fileWatcher = fileWatcher.get();

        // This is what we're testing, it will set up connections between the fileWatcher and the 2 QObject handlers we'll check
        m_applicationManager->InitFileMonitor(AZStd::move(fileWatcher)); // The manager is going to take ownership of the file watcher
    }

    void ApplicationManagerTest::TearDown()
    {
        m_apmThread->exit();
        m_fileProcessorThread->exit();
        m_mockAPM = nullptr;

        ScopedAllocatorSetupFixture::TearDown();
    }

    TEST_F(ApplicationManagerTest, FileWatcherEventsTriggered_ProperlySignalledOnCorrectThread)
    {
        AZ::IO::Path tempDir(m_tempDir.GetDirectory());

        Q_EMIT m_fileWatcher->fileAdded((tempDir / "test").c_str());
        Q_EMIT m_fileWatcher->fileModified((tempDir / "test2").c_str());
        Q_EMIT m_fileWatcher->fileRemoved((tempDir / "test3").c_str());
        
        EXPECT_TRUE(m_mockAPM->m_events[Added].WaitAndCheck()) << "APM Added event failed";
        EXPECT_TRUE(m_mockAPM->m_events[Modified].WaitAndCheck()) << "APM Modified event failed";
        EXPECT_TRUE(m_mockAPM->m_events[Deleted].WaitAndCheck()) << "APM Deleted event failed";

        EXPECT_TRUE(m_mockFileProcessor->m_events[Added].WaitAndCheck()) << "File Processor Added event failed";
        EXPECT_TRUE(m_mockFileProcessor->m_events[Deleted].WaitAndCheck()) << "File Processor Deleted event failed";
    }
}
