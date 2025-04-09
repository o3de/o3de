/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include "AssetEditorWindow.h"

// Qt
#include <QMessageBox>

// AzCore
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzCore/Utils/Utils.h>

// AzToolsFramework
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/ViewPaneOptions.h>

// Editor
#include "LyViewPaneNames.h"

AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include <AssetEditor/ui_AssetEditorWindow.h>
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

namespace AssetEditorUtils
{
    AssetEditorWindow* CreateAssetEditorWithAsset(const AZ::Data::Asset<AZ::Data::AssetData> assetRef)
    {
        AssetEditorWindow* assetEditorWindow = new AssetEditorWindow();
        if (auto assetsWindowsToRestore = AZ::UserSettings::CreateFind<AzToolsFramework::AssetEditor::AssetEditorWindowSettings>(AZ::Crc32(AzToolsFramework::AssetEditor::AssetEditorWindowSettings::s_name), AZ::UserSettings::CT_GLOBAL))
        {
            assetsWindowsToRestore->m_openAssets.emplace(assetRef);
        }

        AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequestBus::Events::Save);

        auto&& loadedAsset = AZ::Data::AssetManager::Instance().GetAsset(assetRef.GetId(), assetRef.GetType(), assetRef.GetAutoLoadBehavior());
        loadedAsset.BlockUntilLoadComplete();
        assetEditorWindow->OpenAsset(loadedAsset);
        return assetEditorWindow;
    }
}

AssetEditorWindow::AssetEditorWindow(QWidget* parent)
    : QWidget(parent)
    , m_ui(new Ui::AssetEditorWindowClass())
{
    using namespace AzToolsFramework::AssetEditor;

    m_ui->setupUi(this);

    connect(m_ui->m_assetEditorWidget, &AssetEditorWidget::OnAssetSaveFailedSignal, this, &AssetEditorWindow::OnAssetSaveFailed);
    connect(m_ui->m_assetEditorWidget, &AssetEditorWidget::OnAssetOpenedSignal, this, &AssetEditorWindow::OnAssetOpened);

    BusConnect();
}

AssetEditorWindow::~AssetEditorWindow()
{
    using namespace AzToolsFramework::AssetEditor;
    BusDisconnect();
}

void AssetEditorWindow::RegisterViewClass(const AZ::Data::Asset<AZ::Data::AssetData>& asset)
{
    AzToolsFramework::ViewPaneOptions options;
    options.showInMenu = false;
    auto& assetName = asset.GetHint();
    const char* paneName = assetName.c_str();
    AzToolsFramework::RegisterViewPane<AssetEditorWindow>(paneName, LyViewPane::CategoryTools, options, [asset](QWidget*) {return AssetEditorUtils::CreateAssetEditorWithAsset(asset); });
}

void AssetEditorWindow::CreateAsset(const AZ::Data::AssetType& assetType, const AZ::Uuid& interestedComponentId)
{
    m_ui->m_assetEditorWidget->CreateAsset(assetType, interestedComponentId);
}

void AssetEditorWindow::OpenAsset(const AZ::Data::Asset<AZ::Data::AssetData>& asset)
{
    m_ui->m_assetEditorWidget->OpenAsset(asset);
}

void AssetEditorWindow::OpenAssetById(const AZ::Data::AssetId assetId)
{
    AZ::Data::Asset<AZ::Data::AssetData> asset = AZ::Data::AssetManager::Instance().GetAsset<AZ::Data::AssetData>(assetId, AZ::Data::AssetLoadBehavior::NoLoad);
    OpenAsset(asset);
}

void AssetEditorWindow::SaveAssetAs(const AZStd::string_view assetPath)
{
    if (assetPath.empty())
    {
        AZ_Warning("Asset Editor", false, "Could not save asset to empty path.");
        return;
    }

    auto absoluteAssetPath = AZ::IO::FixedMaxPath(AZ::Utils::GetEnginePath()) / assetPath;

    if (!m_ui->m_assetEditorWidget->SaveAssetToPath(absoluteAssetPath.Native()))
    {
        AZ_Warning("Asset Editor", false, "File was not saved correctly via SaveAssetAs.");
    }
}

void AssetEditorWindow::RegisterViewClass()
{
    AzToolsFramework::ViewPaneOptions options;
    options.preferedDockingArea = Qt::NoDockWidgetArea;
    options.showOnToolsToolbar = true;
    options.toolbarIcon = ":/Menu/asset_editor.svg";
    AzToolsFramework::RegisterViewPane<AssetEditorWindow>(LyViewPane::AssetEditor, LyViewPane::CategoryTools, options);
}

void AssetEditorWindow::OnAssetOpened(const AZ::Data::Asset<AZ::Data::AssetData>& asset)
{
    if (asset)
    {
        AZStd::string assetPath;
        AZStd::string assetName;
        AZStd::string extension;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequests::GetAssetPathById, asset.GetId());
        AzFramework::StringFunc::Path::Split(assetPath.c_str(), nullptr, nullptr, &assetName, &extension);
//        AZStd::string windowTitle = AZStd::string::format("Edit Asset: %s", (assetName + extension).c_str());

        AZStd::string windowTitle = asset.GetHint();

        qobject_cast<QWidget*>(parent())->setWindowTitle(tr(windowTitle.c_str()));
    }
    else
    {
        qobject_cast<QWidget*>(parent())->setWindowTitle(tr("Asset Editor"));
    }
}

void AssetEditorWindow::closeEvent(QCloseEvent* event)
{
    if (!m_ui->m_assetEditorWidget->SaveAllAndClose())
    {
        event->ignore();
    }
}

void AssetEditorWindow::OnAssetSaveFailed(const AZStd::string& error)
{
    QMessageBox::warning(this, tr("Unable to Save Asset"),
        tr(error.c_str()), QMessageBox::Ok, QMessageBox::Ok);
}

#include <AssetEditor/moc_AssetEditorWindow.cpp>
