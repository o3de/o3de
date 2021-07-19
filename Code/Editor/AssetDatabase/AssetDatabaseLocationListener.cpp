/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include "AssetDatabaseLocationListener.h"

#include <AzCore/Settings/SettingsRegistryMergeUtils.h>

// AzToolsFramework
#include <AzToolsFramework/AssetDatabase/AssetDatabaseConnection.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/API/AssetDatabaseBus.h>

namespace AssetDatabase
{
    AssetDatabaseLocationListener::AssetDatabaseLocationListener()
    {
        BusConnect();

        m_assetDatabaseConnection = new AzToolsFramework::AssetDatabase::AssetDatabaseConnection();
        m_assetDatabaseConnection->OpenDatabase();

        AzToolsFramework::AssetBrowser::AssetDatabaseLocationNotificationBus::Broadcast(&AzToolsFramework::AssetBrowser::AssetDatabaseLocationNotifications::OnDatabaseInitialized);
    }

    AssetDatabaseLocationListener::~AssetDatabaseLocationListener()
    {
        BusDisconnect();

        delete m_assetDatabaseConnection;
        m_assetDatabaseConnection = nullptr;
    }

    AzToolsFramework::AssetDatabase::AssetDatabaseConnection* AssetDatabaseLocationListener::GetAssetDatabaseConnection() const
    {
        return m_assetDatabaseConnection;
    }

    bool AssetDatabaseLocationListener::GetAssetDatabaseLocation(AZStd::string& result)
    {
        if (auto registry = AZ::SettingsRegistry::Get(); registry != nullptr)
        {
            AZ::SettingsRegistryInterface::FixedValueString projectCacheRootValue;
            if (registry->Get(projectCacheRootValue, AZ::SettingsRegistryMergeUtils::FilePathKey_CacheProjectRootFolder);
                !projectCacheRootValue.empty())
            {
                result = projectCacheRootValue;
                result += "/assetdb.sqlite";
                return true;
            }
        }

        return false;
    }
}//namespace AssetDatabase
