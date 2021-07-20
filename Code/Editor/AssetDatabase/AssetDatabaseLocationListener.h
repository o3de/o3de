/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/string/string.h>
#include <AzToolsFramework/API/AssetDatabaseBus.h>
#include <AzToolsFramework/AssetDatabase/AssetDatabaseConnection.h>

namespace AssetDatabase
{
    class AssetDatabaseLocationListener
        : protected AzToolsFramework::AssetDatabase::AssetDatabaseRequestsBus::Handler
    {
    public:
        AssetDatabaseLocationListener();
        ~AssetDatabaseLocationListener();

        AzToolsFramework::AssetDatabase::AssetDatabaseConnection* GetAssetDatabaseConnection() const;

    protected:
        //////////////////////////////////////////////////////////////////////////
        //AzToolsFramework::AssetDatabase::AssetDatabaseRequests::Bus::Listener
        bool GetAssetDatabaseLocation(AZStd::string& result) override;

        //////////////////////////////////////////////////////////////////////////

    private:

        AzToolsFramework::AssetDatabase::AssetDatabaseConnection* m_assetDatabaseConnection = nullptr;
    };
}//namespace AssetDatabase
