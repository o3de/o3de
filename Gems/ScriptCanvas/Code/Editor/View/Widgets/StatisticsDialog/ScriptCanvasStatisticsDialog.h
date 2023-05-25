/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <QAbstractItemModel>
#include <QDialog>
#include <QSortFilterProxyModel>

#include <AzFramework/Asset/AssetCatalogBus.h>

#include <AzToolsFramework/AssetBrowser/AssetBrowserFilterModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Search/Filter.h>

#include <AzQtComponents/Components/StyledDockWidget.h>

#include <GraphCanvas/Widgets/GraphCanvasTreeModel.h>
#include <GraphCanvas/Widgets/GraphCanvasTreeItem.h>

#include <Editor/View/Widgets/NodePalette/NodePaletteModel.h>
#include <Editor/View/Widgets/StatisticsDialog/NodeUsageTreeItem.h>
#include <ScriptCanvas/Core/Core.h>
#endif

namespace Ui
{
    class ScriptCanvasStatisticsDialog;
}

namespace ScriptCanvasEditor
{
    class ScriptCanvasAssetNodeUsageFilterModel
        : public QSortFilterProxyModel
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptCanvasAssetNodeUsageFilterModel, AZ::SystemAllocator);

        ScriptCanvasAssetNodeUsageFilterModel();

        bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const;

        void SetFilter(const QString& filterName);
        void SetNodeTypeFilter(const ScriptCanvas::NodeTypeIdentifier& nodeType);

    private:

        QString m_filter;
        QRegExp m_regex;

        ScriptCanvas::NodeTypeIdentifier m_nodeIdentifier;
    };

    class StatisticsDialog
        : public QDialog
        , AzFramework::AssetCatalogEventBus::Handler
        , public NodePaletteModelNotificationBus::Handler
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(StatisticsDialog, AZ::SystemAllocator);

        StatisticsDialog(const NodePaletteModel& nodePaletteModel, AzToolsFramework::AssetBrowser::AssetBrowserFilterModel* scriptCanvasAssetModel, QWidget* widget = nullptr);
        ~StatisticsDialog();

        void InitStatisticsWindow();
        void ResetModel();

        // AssetSystemBus
        //! Called by the AssetCatalog when an asset has been modified
        void OnCatalogAssetChanged(const AZ::Data::AssetId& assetId) override;
        void OnCatalogAssetAdded(const AZ::Data::AssetId& /*assetId*/) override;

        //! Called by the AssetProcessor when an asset in the cache has been removed.
        void OnCatalogAssetRemoved(const AZ::Data::AssetId& assetId, const AZ::Data::AssetInfo& assetInfo) override;
        ////

        // NodePaletteModelNotificationBus
        void OnAssetModelRepopulated() override;

        void OnAssetNodeAdded(NodePaletteModelInformation* modelInformation) override;
        void OnAssetNodeRemoved(NodePaletteModelInformation* modelInformation) override;
        ////

        void OnScriptCanvasAssetClicked(const QModelIndex& modelIndex);

        void showEvent(QShowEvent* showEvent) override;

    public slots:
        void OnSelectionCleared();
        void OnItemSelected(const GraphCanvas::GraphCanvasTreeItem* treeItem);

        void OnFilterUpdated(const QString& filterText);
        void OnScriptCanvasAssetRowsInserted(QModelIndex modelIndex, int first, int last);

    private:

        void TraverseTree(QModelIndex index = QModelIndex());
        void ProcessAsset(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry);

        const NodePaletteModel& m_nodePaletteModel;

        AZStd::unique_ptr<Ui::ScriptCanvasStatisticsDialog>    m_ui;
        NodePaletteNodeUsageRootItem*                          m_treeRoot;

        AzToolsFramework::AssetBrowser::AssetBrowserFilterModel*     m_scriptCanvasAssetBrowserModel;

        ScriptCanvasAssetNodeUsageTreeItemRoot* m_scriptCanvasAssetTreeRoot;
        GraphCanvas::GraphCanvasTreeModel* m_scriptCanvasAssetTree;
        ScriptCanvasAssetNodeUsageFilterModel* m_scriptCanvasAssetFilterModel;

        AZStd::unordered_map< ScriptCanvas::NodeTypeIdentifier, GraphCanvas::GraphCanvasTreeItem* > m_leafMap;
    };
}
