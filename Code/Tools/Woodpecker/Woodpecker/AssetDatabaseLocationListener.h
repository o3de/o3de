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
#pragma once
#include <AzCore/std/string/string.h>
#include <AzToolsFramework/API/AssetDatabaseBus.h>
#include <AzToolsFramework/AssetDatabase/AssetDatabaseConnection.h>

namespace LUAEditor
{
    class AssetDatabaseLocationListener
        : protected AzToolsFramework::AssetDatabase::AssetDatabaseRequestsBus::Handler
    {
    public:
        AssetDatabaseLocationListener();
        ~AssetDatabaseLocationListener();

        void Init(const char* root);

    protected:
        //////////////////////////////////////////////////////////////////////////
        //AzToolsFramework::AssetDatabase::AssetDatabaseRequests::Bus::Listener
        bool GetAssetDatabaseLocation(AZStd::string& result) override;
        //////////////////////////////////////////////////////////////////////////

    private:
        AZStd::unique_ptr<AzToolsFramework::AssetDatabase::AssetDatabaseConnection> m_assetDatabaseConnection;
        AZStd::string m_root;
    };
}//namespace AssetBrowserTester
