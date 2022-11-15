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
        using FilterFn = AZStd::function<bool(const AZStd::string&)>;

        AssetSelectionGrid(
            const QString& title,
            const FilterFn& filterFn,
            const QSize& tileSize,
            QWidget* parent = nullptr);

        ~AssetSelectionGrid();

        void Reset();
        void SetFilter(const FilterFn& filterFn);
        void AddPath(const AZStd::string& path);
        void RemovePath(const AZStd::string& path);
        void SelectPath(const AZStd::string& path);
        AZStd::string GetSelectedPath() const;

    Q_SIGNALS:
        void PathSelected(const AZStd::string& path);
        void PathRejected();

    private:
        AZ_DISABLE_COPY_MOVE(AssetSelectionGrid);

        // AzFramework::AssetCatalogEventBus::Handler overrides ...
        void OnCatalogAssetAdded(const AZ::Data::AssetId& assetId) override;
        void OnCatalogAssetRemoved(const AZ::Data::AssetId& assetId, const AZ::Data::AssetInfo& assetInfo) override;

        void SetupAssetList();
        void SetupSearchWidget();
        void SetupDialogButtons();
        void ApplySearchFilter();
        void ShowSearchMenu(const QPoint& pos);

        QSize m_tileSize;
        QScopedPointer<Ui::AssetSelectionGrid> m_ui;
        FilterFn m_filterFn;
    };
} // namespace AtomToolsFramework
