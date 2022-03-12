/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "AzAssetBrowserWindow.h"
#include "AzAssetBrowser/ui_AssetBrowserWindow.h"

#include <AzToolsFramework/AssetBrowser/UI/AssetTreeView.h>
#include <AzToolsFramework/AssetBrowser/UI/SortFilterProxyModel.hxx>
#include <AzToolsFramework/AssetBrowser/UI/AssetBrowserModel.h>
#include <AzToolsFramework/AssetBrowser/AssetCache/AssetCacheBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserRequestBus.h>

const char* ASSET_BROWSER_PREVIEW_NAME = "Asset Browser (PREVIEW)";

AzAssetBrowserWindow::AzAssetBrowserWindow(const QString& name, QWidget* parent)
        : QDialog(parent)
        , m_ui(new Ui::AssetBrowserWindowClass())
        , m_assetDatabaseSortFilterProxyModel(new AssetBrowser::UI::SortFilterProxyModel(parent))
        , m_name(name)
        , m_assetBrowser(new AssetBrowser::UI::AssetTreeView(name, this))
    {
        EBUS_EVENT_RESULT(m_assetBrowserModel, AssetBrowser::AssetCache::AssetCacheRequestsBus, GetAssetBrowserModel);
        AZ_Assert(m_assetBrowserModel, "Failed to get filebrowser model");
        m_assetDatabaseSortFilterProxyModel->setSourceModel(m_assetBrowserModel);

        m_ui->setupUi(this);

        connect(m_ui->searchCriteriaWidget,
            &AzToolsFramework::SearchCriteriaWidget::SearchCriteriaChanged,
            m_assetDatabaseSortFilterProxyModel.data(),
            &AssetBrowser::UI::SortFilterProxyModel::OnSearchCriteriaChanged);

        connect(m_assetBrowser, &QTreeView::customContextMenuRequested, this, &AzAssetBrowserWindow::OnContextMenu);
    }

    AzAssetBrowserWindow::~AzAssetBrowserWindow()
    {
        m_assetBrowser->SaveState();
    }

    //////////////////////////////////////////////////////////////////////////
    const AZ::Uuid& AzAssetBrowserWindow::GetClassID()
    {
        return AZ::AzTypeInfo<AzAssetBrowserWindow>::Uuid();
    }

    void AzAssetBrowserWindow::OnContextMenu(const QPoint& point)
    {
        (void)point;
        //get the selected entries
        QModelIndexList sourceIndexes;
        for (const auto& index : m_assetBrowser->selectedIndexes())
        {
            sourceIndexes.push_back(m_assetDatabaseSortFilterProxyModel->mapToSource(index));
        }
        AZStd::vector<AssetBrowser::UI::Entry*> entries;
        m_assetBrowserModel->SourceIndexesToAssetDatabaseEntries(sourceIndexes, entries);

        if (entries.empty() || entries.size() > 1)
        {
            return;
        }
        auto entry = entries.front();

        EBUS_EVENT(AssetBrowser::AssetBrowserRequestBus::Bus, OnItemContextMenu, this, entry);
    }

#include <AzAssetBrowser/moc_AzAssetBrowserWindow.cpp>

