/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzToolsFramework/AssetEditor/AssetEditorBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

class AssetEditorRequestsHandler 
    : public AzToolsFramework::AssetEditor::AssetEditorRequestsBus::Handler
    , public AzToolsFramework::EditorEvents::Bus::Handler
{
public:
    AZ_CLASS_ALLOCATOR(AssetEditorRequestsHandler, AZ::SystemAllocator);

    AssetEditorRequestsHandler();
    ~AssetEditorRequestsHandler() override;

    //////////////////////////////////////////////////////////////////////////
    // AssetEditorRequests
    //////////////////////////////////////////////////////////////////////////
    void CreateNewAsset(const AZ::Data::AssetType& assetType, const AZ::Uuid& observerId) override;
    void OpenAssetEditor(const AZ::Data::Asset<AZ::Data::AssetData>& asset) override;
    void OpenAssetEditorById(const AZ::Data::AssetId assetId) override;

    //////////////////////////////////////////////////////////////////////////
    // AzToolsFramework::EditorEvents::Bus::Handler
    //////////////////////////////////////////////////////////////////////////
    void NotifyRegisterViews() override;
};
