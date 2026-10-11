/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include "AssetEditorRequestsHandler.h"

// AzCore
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerBus.h>

// Editor
#include "AssetEditorWindow.h"
#include "QtViewPaneManager.h"

AssetEditorRequestsHandler::AssetEditorRequestsHandler()
{
    AzToolsFramework::AssetEditor::AssetEditorRequestsBus::Handler::BusConnect();
    AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
}

AssetEditorRequestsHandler::~AssetEditorRequestsHandler()
{
    AzToolsFramework::AssetEditor::AssetEditorRequestsBus::Handler::BusDisconnect();
    AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
}

void AssetEditorRequestsHandler::NotifyRegisterViews()
{
    using namespace AzToolsFramework::AssetEditor;
    if (auto assetsWindowsToRestore = AZ::UserSettings::CreateFind<AssetEditorWindowSettings>(AZ::Crc32(AssetEditorWindowSettings::s_name), AZ::UserSettings::CT_GLOBAL))
    {
        //copy the current list and clear it since they will be re-added as we request to open them
        auto windowsToOpen = assetsWindowsToRestore->m_openAssets;
        assetsWindowsToRestore->m_openAssets.clear();
        for (auto&& assetRef : windowsToOpen)
        {
            AssetEditorWindow::RegisterViewClass(assetRef);
        }
    }
}

void AssetEditorRequestsHandler::CreateNewAsset(const AZ::Data::AssetType& assetType, const AZ::Uuid& observerId)
{
    using namespace AzToolsFramework::AssetEditor;
    AzToolsFramework::OpenViewPane(LyViewPane::AssetEditor);

    AssetEditorWidgetRequestsBus::Broadcast(&AssetEditorWidgetRequests::CreateAsset, assetType, observerId);
}

void AssetEditorRequestsHandler::OpenAssetEditor(const AZ::Data::Asset<AZ::Data::AssetData>& asset)
{
    using namespace AzToolsFramework::AssetEditor;

    // Open the AssetEditor if it isn't open already.
    QtViewPaneManager::instance()->OpenPane(LyViewPane::AssetEditor, QtViewPane::OpenMode::RestoreLayout);

    AssetEditorWidgetRequestsBus::Broadcast(&AssetEditorWidgetRequests::OpenAsset, asset);
}

void AssetEditorRequestsHandler::OpenAssetEditorById(const AZ::Data::AssetId assetId)
{
    // The catalog knows the real asset type; GetAsset<AssetData> would ask for a type that has no handler
    AZ::Data::AssetInfo assetInfo;
    AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, assetId);
    OpenAssetEditor(AZ::Data::AssetManager::Instance().GetAsset(assetId, assetInfo.m_assetType, AZ::Data::AssetLoadBehavior::NoLoad));
}

void AssetEditorRequestsHandler::OpenAssetEditorByPath(const AZStd::string& path)
{
    using namespace AzToolsFramework::AssetEditor;

    QtViewPaneManager::instance()->OpenPane(LyViewPane::AssetEditor, QtViewPane::OpenMode::RestoreLayout);

    AssetEditorWidgetRequestsBus::Broadcast(&AssetEditorWidgetRequests::OpenAssetByPath, path);
}
