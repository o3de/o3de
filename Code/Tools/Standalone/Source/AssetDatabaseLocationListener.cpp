/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AssetDatabaseLocationListener.h"
#include <AzToolsFramework/AssetDatabase/AssetDatabaseConnection.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/API/AssetDatabaseBus.h>
#include <AzCore/std/string/string.h>
#include <AzCore/IO/FileIO.h>


namespace LUAEditor
{
    AssetDatabaseLocationListener::AssetDatabaseLocationListener() :
        m_assetDatabaseConnection(new AzToolsFramework::AssetDatabase::AssetDatabaseConnection())
    {
        BusConnect();
    }

    AssetDatabaseLocationListener::~AssetDatabaseLocationListener()
    {
        BusDisconnect();
    }

    void AssetDatabaseLocationListener::Init(const char * root)
    {
        m_root = root;
        using requestsBus = AzToolsFramework::AssetBrowser::AssetDatabaseLocationNotificationBus;
        using requests = AzToolsFramework::AssetBrowser::AssetDatabaseLocationNotifications;
        requestsBus::Broadcast(&requests::OnDatabaseInitialized);
    }
    
    bool AssetDatabaseLocationListener::GetAssetDatabaseLocation(AZStd::string& result)
    {
        result = m_root;
        return true;
    }
}//namespace AssetBrowserTester
