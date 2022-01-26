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
#endif

#include <QDialog>
#include <QString>

class QListWidgetItem;

namespace Ui
{
    class AssetGridDialog;
}

namespace AtomToolsFramework
{
    //! Widget for managing and selecting from a library of assets
    class AssetGridDialog : public QDialog
    {
        Q_OBJECT
    public:
        struct SelectableAsset
        {
            AZ::Data::AssetId m_assetId;
            QString m_title;
        };

        using SelectableAssetVector = AZStd::vector<SelectableAsset>;

        AssetGridDialog(
            const QString& title,
            const SelectableAssetVector& selectableAssets,
            const AZ::Data::AssetId& selectedAsset,
            const QSize& tileSize,
            QWidget* parent = nullptr);

        ~AssetGridDialog();

    Q_SIGNALS:
        void AssetSelected(const AZ::Data::AssetId& assetId);

    private:
        AZ_DISABLE_COPY_MOVE(AssetGridDialog);

        QListWidgetItem* CreateListItem(const SelectableAsset& selectableAsset);
        void SetupAssetList();
        void SetupSearchWidget();
        void SetupDialogButtons();
        void ApplySearchFilter();
        void ShowSearchMenu(const QPoint& pos);
        void SelectCurrentAsset();
        void SelectInitialAsset();

        QSize m_tileSize;
        AZ::Data::AssetId m_initialSelectedAsset;
        QScopedPointer<Ui::AssetGridDialog> m_ui;
    };
} // namespace AtomToolsFramework
