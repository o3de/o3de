/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Asset/AssetManager.h>
#include <AzToolsFramework/AssetEditor/AssetEditorBus.h>

namespace AzToolsFramework
{
    inline void OpenGenericAssetEditor(const AZ::Data::Asset<AZ::Data::AssetData>& asset)
    {
        AzToolsFramework::AssetEditor::AssetEditorRequestsBus::Broadcast(&AzToolsFramework::AssetEditor::AssetEditorRequests::OpenAssetEditor, asset);
    }

    inline bool OpenGenericAssetEditor(const AZ::Data::AssetType& assetType, const AZ::Data::AssetId& assetId)
    {
        AZ::Data::Asset<AZ::Data::AssetData> asset = AZ::Data::AssetManager::Instance().FindOrCreateAsset(assetId, assetType, AZ::Data::AssetLoadBehavior::Default);
        
        if (asset)
        {
            OpenGenericAssetEditor(asset);
            return true;
        }
        
        return false;
    }
}
