/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include "AssetDetailsPanel.h"
#include <QScopedPointer>
#endif

class QItemSelection;

namespace Ui
{
    class SourceAssetDetailsPanel;
}

namespace AssetProcessor
{
    class AssetDatabaseConnection;
    class SourceAssetTreeItemData;

    class SourceAssetDetailsPanel
        : public AssetDetailsPanel
    {
        Q_OBJECT
    public:
        explicit SourceAssetDetailsPanel(QWidget* parent = nullptr);
        ~SourceAssetDetailsPanel() override;

    public Q_SLOTS:
        void AssetDataSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

    protected:
        void ResetText();
        void SetDetailsVisible(bool visible);

        void BuildProducts(
            AssetDatabaseConnection& assetDatabaseConnection,
            const AZStd::shared_ptr<const SourceAssetTreeItemData> sourceItemData);
        void BuildOutgoingSourceDependencies(
            AssetDatabaseConnection& assetDatabaseConnection,
            const AZStd::shared_ptr<const SourceAssetTreeItemData> sourceItemData);
        void BuildIncomingSourceDependencies(
            AssetDatabaseConnection& assetDatabaseConnection,
            const AZStd::shared_ptr<const SourceAssetTreeItemData> sourceItemData);

        QScopedPointer<Ui::SourceAssetDetailsPanel> m_ui;
    };
} // namespace AssetProcessor
