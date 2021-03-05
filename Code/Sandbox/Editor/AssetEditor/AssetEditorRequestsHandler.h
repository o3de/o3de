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

#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/AssetEditor/AssetEditorBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

class AssetEditorRequestsHandler 
    : public AzToolsFramework::AssetEditor::AssetEditorRequestsBus::Handler
    , public AzToolsFramework::EditorEvents::Bus::Handler
{
public:
    AZ_CLASS_ALLOCATOR(AssetEditorRequestsHandler, AZ::SystemAllocator, 0);

    AssetEditorRequestsHandler();
    ~AssetEditorRequestsHandler() override;

    //////////////////////////////////////////////////////////////////////////
    // AssetEditorRequests
    //////////////////////////////////////////////////////////////////////////
    void CreateNewAsset(const AZ::Data::AssetType& assetType) override;
    void OpenAssetEditor(const AZ::Data::Asset<AZ::Data::AssetData>& asset) override;
    void OpenAssetEditorById(const AZ::Data::AssetId assetId) override;

    //////////////////////////////////////////////////////////////////////////
    // AzToolsFramework::EditorEvents::Bus::Handler
    //////////////////////////////////////////////////////////////////////////
    void NotifyRegisterViews() override;
};
