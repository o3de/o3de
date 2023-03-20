/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
