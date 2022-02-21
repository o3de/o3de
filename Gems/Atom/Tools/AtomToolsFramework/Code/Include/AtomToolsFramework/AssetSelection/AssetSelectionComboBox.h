/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <QComboBox>
#endif

namespace AtomToolsFramework
{
    class AssetSelectionComboBox
        : public QComboBox
        , private AzFramework::AssetCatalogEventBus::Handler
    {
        Q_OBJECT
    public:
        AssetSelectionComboBox(QWidget* parent = 0);
        ~AssetSelectionComboBox();

        void Reset();
        void SetFilterCallback(const AZStd::function<bool(const AZ::Data::AssetInfo&)>& filterCallback);
        void SelectAsset(const AZ::Data::AssetId& assetId);

    Q_SIGNALS:
        void AssetSelected(const AZ::Data::AssetId& assetId);

    private:
        AZ_DISABLE_COPY_MOVE(AssetSelectionComboBox);

        // AzFramework::AssetCatalogEventBus::Handler overrides ...
        void OnCatalogAssetAdded(const AZ::Data::AssetId& assetId) override;
        void OnCatalogAssetRemoved(const AZ::Data::AssetId& assetId, const AZ::Data::AssetInfo& assetInfo) override;

        AZStd::function<bool(const AZ::Data::AssetInfo&)> m_filterCallback;
    };
} // namespace AtomToolsFramework
