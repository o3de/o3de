/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/containers/vector.h>
#include <AzFramework/Asset/AssetCatalogBus.h>
#endif

#include <QDialog>
#include <QString>

class QListWidgetItem;

namespace Ui
{
    class AssetSelectionGrid;
}

namespace AtomToolsFramework
{
    //! Widget for managing and selecting from a library of assets
    class AssetSelectionGrid
        : public QDialog
        , private AzFramework::AssetCatalogEventBus::Handler
    {
        Q_OBJECT
    public:
        AssetSelectionGrid(
            const QString& title,
            const AZStd::function<bool(const AZ::Data::AssetInfo&)>& filterCallback,
            const QSize& tileSize,
            QWidget* parent = nullptr);

        ~AssetSelectionGrid();

        void Reset();
        void SetFilterCallback(const AZStd::function<bool(const AZ::Data::AssetInfo&)>& filterCallback);
        void SelectAsset(const AZ::Data::AssetId& assetId);
        AZ::Data::AssetId GetSelectedAsset() const;
        AZStd::string GetSelectedAssetSourcePath() const;
        AZStd::string GetSelectedAssetProductPath() const;

    Q_SIGNALS:
        void AssetSelected(const AZ::Data::AssetId& assetId);
        void AssetRejected();

    private:
        AZ_DISABLE_COPY_MOVE(AssetSelectionGrid);

        // AzFramework::AssetCatalogEventBus::Handler overrides ...
        void OnCatalogAssetAdded(const AZ::Data::AssetId& assetId) override;
        void OnCatalogAssetRemoved(const AZ::Data::AssetId& assetId, const AZ::Data::AssetInfo& assetInfo) override;

        QListWidgetItem* CreateListItem(const AZ::Data::AssetId& assetId, const QString& title);
        void SetupAssetList();
        void SetupSearchWidget();
        void SetupDialogButtons();
        void ApplySearchFilter();
        void ShowSearchMenu(const QPoint& pos);

        QSize m_tileSize;
        QScopedPointer<Ui::AssetSelectionGrid> m_ui;
        AZStd::function<bool(const AZ::Data::AssetInfo&)> m_filterCallback;
    };
} // namespace AtomToolsFramework
