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