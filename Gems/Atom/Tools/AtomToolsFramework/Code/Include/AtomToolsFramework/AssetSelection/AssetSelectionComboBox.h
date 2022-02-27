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
#include <AzToolsFramework/Thumbnails/Thumbnail.h>
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
        AssetSelectionComboBox(const AZStd::function<bool(const AZ::Data::AssetInfo&)>& filterCallback, QWidget* parent = 0);
        ~AssetSelectionComboBox();

        void Reset();
        void SetFilterCallback(const AZStd::function<bool(const AZ::Data::AssetInfo&)>& filterCallback);
        void SelectAsset(const AZ::Data::AssetId& assetId);
        AZ::Data::AssetId GetSelectedAsset() const;
        AZStd::string GetSelectedAssetSourcePath() const;
        AZStd::string GetSelectedAssetProductPath() const;

        void SetThumbnailsEnabled(bool enabled);
        void SetThumbnailDelayMs(AZ::u32 delay);

    Q_SIGNALS:
        void AssetSelected(const AZ::Data::AssetId& assetId);

    private:
        AZ_DISABLE_COPY_MOVE(AssetSelectionComboBox);

        // AzFramework::AssetCatalogEventBus::Handler overrides ...
        void OnCatalogAssetAdded(const AZ::Data::AssetId& assetId) override;
        void OnCatalogAssetRemoved(const AZ::Data::AssetId& assetId, const AZ::Data::AssetInfo& assetInfo) override;

        void AddAsset(const AZ::Data::AssetInfo& assetInfo);
        void RegisterThumbnail(const AZ::Data::AssetId& assetId);
        void UpdateThumbnail(const AZ::Data::AssetId& assetId);
        void QueueUpdateThumbnail(const AZ::Data::AssetId& assetId);

        AZStd::function<bool(const AZ::Data::AssetInfo&)> m_filterCallback;

        bool m_thumbnailsEnabled = false;
        AZ::u32 m_thumbnailDelayMs = 2000;
        AZStd::unordered_map<AZ::Data::AssetId, AzToolsFramework::Thumbnailer::SharedThumbnailKey> m_thumbnailKeys;
    };
} // namespace AtomToolsFramework
