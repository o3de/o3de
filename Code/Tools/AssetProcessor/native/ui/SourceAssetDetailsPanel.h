/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
