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
