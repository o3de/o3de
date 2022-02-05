/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <utilities/BatchApplicationManager.h>
#include <AzCore/UnitTest/TestTypes.h>
#include "assetmanager/MockAssetProcessorManager.h"
#include "assetmanager/MockFileProcessor.h"

namespace UnitTests
{
    struct MockBatchApplicationManager : BatchApplicationManager
    {
        using ApplicationManagerBase::InitFileMonitor;
        using ApplicationManagerBase::m_assetProcessorManager;
        using ApplicationManagerBase::m_fileProcessor;
        using ApplicationManagerBase::m_fileStateCache;
        using ApplicationManagerBase::m_platformConfiguration;
        using BatchApplicationManager::BatchApplicationManager;
    };

    class DatabaseLocationListener : public AzToolsFramework::AssetDatabase::AssetDatabaseRequests::Bus::Handler
    {
    public:
        DatabaseLocationListener()
        {
            BusConnect();
        }
        ~DatabaseLocationListener() override
        {
            BusDisconnect();
        }

        bool GetAssetDatabaseLocation(AZStd::string& location) override;

        AZStd::string m_databaseLocation;
    };

    struct ApplicationManagerTest : ::UnitTest::ScopedAllocatorSetupFixture
    {
    protected:
        void SetUp() override;
        void TearDown() override;

        AZ::Test::ScopedAutoTempDirectory m_tempDir;
        DatabaseLocationListener m_databaseLocationListener;

        AZStd::unique_ptr<QCoreApplication> m_coreApplication;
        AZStd::unique_ptr<MockBatchApplicationManager> m_applicationManager;
        AZStd::unique_ptr<QThread> m_apmThread;
        AZStd::unique_ptr<QThread> m_fileProcessorThread;
        AZStd::unique_ptr<MockAssetProcessorManager> m_mockAPM;

        // These are just aliases, no need to manage/delete them
        FileWatcher* m_fileWatcher{};
        MockFileProcessor* m_mockFileProcessor{};
    };
}
