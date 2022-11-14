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
#include <native/tests/MockAssetDatabaseRequestsHandler.h>
#include "assetmanager/MockAssetProcessorManager.h"
#include "assetmanager/MockFileProcessor.h"

namespace UnitTests
{
    struct MockBatchApplicationManager : BatchApplicationManager
    {
        using ApplicationManagerBase::InitFileMonitor;
        using ApplicationManagerBase::InitFileStateCache;
        using ApplicationManagerBase::m_assetProcessorManager;
        using ApplicationManagerBase::m_fileProcessor;
        using ApplicationManagerBase::m_fileStateCache;
        using ApplicationManagerBase::m_platformConfiguration;
        using BatchApplicationManager::BatchApplicationManager;
    };

    struct ApplicationManagerTest : ::UnitTest::ScopedAllocatorSetupFixture
    {
    protected:
        void SetUp() override;
        void TearDown() override;

        AssetProcessor::MockAssetDatabaseRequestsHandler m_databaseLocationListener;

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
