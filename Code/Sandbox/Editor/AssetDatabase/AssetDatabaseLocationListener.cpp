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

    bool AssetDatabaseLocationListener::GetAssetDatabaseLocation( AZStd::string& result )
    {
        result = gEnv->pFileIO->GetAlias( "@devroot@" );
        result += "/Cache/";
        ICVar * pCvar = gEnv->pConsole->GetCVar( "sys_game_folder" );
        if( pCvar && pCvar->GetString() )
        {
            result += pCvar->GetString();
        }
        result += "/assetdb.sqlite";
        return true;
    }
}//namespace AssetDatabase
