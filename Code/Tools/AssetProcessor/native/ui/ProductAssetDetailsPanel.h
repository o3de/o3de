/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <native/ui/AssetDetailsPanel.h>
#include <native/ui/ProductDependencyTreeModel.h>
#include <native/ui/ProductDependencyTreeDelegate.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <QDateTime>
#include <QHash>
#include <QPointer>
#include <QScopedPointer>
#endif

class QItemSelection;
class QLabel;
class QListWidget;
class QListWidgetItem;

namespace AZ
{
    namespace Data
    {
        struct AssetId;
    }
}

namespace Ui
{
    class ProductAssetDetailsPanel;
}

namespace AssetProcessor
{
    class AssetTreeItem;
    class AssetDatabaseConnection;
    class ProductAssetTreeItemData;

    class ProductAssetDetailsPanel
        : public AssetDetailsPanel
    {
        Q_OBJECT
    public:
        explicit ProductAssetDetailsPanel(QWidget* parent = nullptr);
        ~ProductAssetDetailsPanel() override;

        // The scan results widget is in a separate section of the UI, but updates when scans are added / completed.
        void SetScannerInformation(QPointer<QListWidget> missingDependencyScanResults, AZStd::shared_ptr<AssetDatabaseConnection> assetDatabaseConnection)
        {
            m_missingDependencyScanResults = missingDependencyScanResults;
            m_assetDatabaseConnection = assetDatabaseConnection;
        }

        void SetScanQueueEnabled(bool enabled);
        void SetupDependencyGraph(QTreeView* productAssetsTreeView, AZStd::shared_ptr<AssetDatabaseConnection> assetDatabaseConnection);

        QTreeView* GetOutgoingProductDependenciesTreeView() const;
        QTreeView* GetIncomingProductDependenciesTreeView() const;

    public Q_SLOTS:
        void AssetDataSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

    protected:
        struct MissingDependencyScanGUIInfo
        {
            QListWidgetItem* m_scanWidgetRow = nullptr;
            size_t m_remainingFiles = 0;
            QDateTime m_scanTimeStart;
        };

        enum class MissingDependencyTableColumns
        {
            GoToButton,
            ScanTime,
            Dependency,
            Max
        };

        void ResetText();
        void SetDetailsVisible(bool visible);
        void OnSupportClicked(bool checked);
        void OnScanFileClicked(bool checked);
        void OnScanFolderClicked(bool checked);
        void OnClearScanFileClicked(bool checked);
        void OnClearScanFolderClicked(bool checked);

        void ScanFolderForMissingDependencies(QString scanName, AssetTreeItem& folder);
        void ScanFileForMissingDependencies(QString scanName, const AZStd::shared_ptr<const ProductAssetTreeItemData> productItemData);

        void ClearMissingDependenciesForFile(const AZStd::shared_ptr<const ProductAssetTreeItemData> productItemData);
        void ClearMissingDependenciesForFolder(AssetTreeItem& folder);

        void RefreshUI();

        void BuildOutgoingProductDependencies(
            const AZStd::shared_ptr<const ProductAssetTreeItemData> productItemData,
            const AZStd::string& platform);

        void BuildIncomingProductDependencies(
            const AZStd::shared_ptr<const ProductAssetTreeItemData> productItemData,
            const AZ::Data::AssetId& assetId,
            const AZStd::string& platform);

        void BuildMissingProductDependencies(
            const AZStd::shared_ptr<const ProductAssetTreeItemData> productItemData);

        void AddProductIdToScanCount(AZ::s64 scannedProductId, QString scanName);
        void RemoveProductIdFromScanCount(AZ::s64 scannedProductId, QString scanName);
        void UpdateScannerUI(MissingDependencyScanGUIInfo& scannerUIInfo, QString scanName);

        QScopedPointer<Ui::ProductAssetDetailsPanel> m_ui;
        AssetTreeItem* m_currentItem = nullptr;
        // Track how many files are being scanned in the UI.
        QHash<AZ::s64, QString> m_productIdToScanName;
        QHash<QString, MissingDependencyScanGUIInfo> m_scanNameToScanGUIInfo;
        mutable AZStd::recursive_mutex m_scanCountMutex;
        QPointer<QListWidget> m_missingDependencyScanResults;
        // The asset database connection in the AzToolsFramework namespace is read only. The AssetProcessor connection allows writing.
        AZStd::shared_ptr<AssetDatabaseConnection> m_assetDatabaseConnection;

        ProductDependencyTreeModel* m_outgoingDependencyTreeModel = nullptr;
        ProductDependencyTreeModel* m_incomingDependencyTreeModel = nullptr;
    };
} // namespace AssetProcessor
