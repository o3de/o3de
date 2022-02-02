/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <utilities/BatchApplicationManager.h>

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
}
