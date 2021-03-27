/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
