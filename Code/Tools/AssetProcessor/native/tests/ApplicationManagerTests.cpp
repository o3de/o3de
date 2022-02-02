/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ApplicationManagerTests.h"

#include <QCoreApplication>
#include <AzCore/UnitTest/TestTypes.h>
#include <tests/assetmanager/MockAssetProcessorManager.h>
#include <tests/assetmanager/MockFileProcessor.h>

namespace UnitTests
{
    bool DatabaseLocationListener::GetAssetDatabaseLocation(AZStd::string& location)
    {
        location = m_databaseLocation;
        return true;
    }

    using ApplicationManagerTest = ::UnitTest::ScopedAllocatorSetupFixture;

    TEST_F(ApplicationManagerTest, FileWatcherEventsTriggered_ProperlySignalledOnCorrectThread)
    {
        AZ::Test::ScopedAutoTempDirectory m_tempDir;
        AZ::IO::Path tempDir(m_tempDir.GetDirectory());
        DatabaseLocationListener m_databaseLocationListener;
        m_databaseLocationListener.m_databaseLocation = (tempDir / "test_database.sqlite").Native();

        // We need a QCoreApplication to run the event loop
        int argc = 0;
        QCoreApplication app{ argc, nullptr };

        MockBatchApplicationManager applicationManager(&argc, nullptr);
        applicationManager.m_platformConfiguration = new AssetProcessor::PlatformConfiguration{ nullptr };
        applicationManager.m_fileStateCache = AZStd::make_unique<AssetProcessor::FileStateCache>();
        auto mockAPM = new MockAssetProcessorManager{ applicationManager.m_platformConfiguration, nullptr };
        applicationManager.m_assetProcessorManager = mockAPM;

        QThread apmThread(nullptr);
        applicationManager.m_assetProcessorManager->moveToThread(&apmThread);
        apmThread.start();

        QThread fileProcessorThread(nullptr);
        auto fileProcessor = AZStd::make_unique<MockFileProcessor>(applicationManager.m_platformConfiguration);
        fileProcessor->moveToThread(&fileProcessorThread);
        MockFileProcessor* mockFileProcessor = fileProcessor.get();
        applicationManager.m_fileProcessor = AZStd::move(fileProcessor); // The manager is taking ownership
        fileProcessorThread.start();

        auto fileWatcher = AZStd::make_unique<FileWatcher>();
        FileWatcher* mockFileWatcher = fileWatcher.get();

        // This is what we're testing, it will set up connections between the fileWatcher and the 2 QObject handlers we'll check
        applicationManager.InitFileMonitor(AZStd::move(fileWatcher)); // The manager is going to take ownership of the file watcher

        Q_EMIT mockFileWatcher->fileAdded("test");
        Q_EMIT mockFileWatcher->fileModified("test2");
        Q_EMIT mockFileWatcher->fileRemoved("test3");

        QCoreApplication::processEvents();

        mockAPM->m_events[Added].WaitAndCheck();
        mockAPM->m_events[Modified].WaitAndCheck();
        mockAPM->m_events[Deleted].WaitAndCheck();

        mockFileProcessor->m_events[Added].WaitAndCheck();
        mockFileProcessor->m_events[Deleted].WaitAndCheck();

        apmThread.exit();
        fileProcessorThread.exit();
        delete mockAPM;
    }
}
