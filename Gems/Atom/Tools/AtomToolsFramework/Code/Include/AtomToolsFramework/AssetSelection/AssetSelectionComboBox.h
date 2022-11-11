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
        using FilterFn = AZStd::function<bool(const AZStd::string&)>;

        AssetSelectionComboBox(const FilterFn& filterFn, QWidget* parent = 0);
        ~AssetSelectionComboBox();

        void Reset();
        void AddPath(const AZStd::string& path);
        void RemovePath(const AZStd::string& path);
        void SetFilter(const FilterFn& filterFn);
        void SelectPath(const AZStd::string& path);
        AZStd::string GetSelectedPath() const;

        void SetThumbnailsEnabled(bool enabled);
        void SetThumbnailDelayMs(AZ::u32 delay);

    Q_SIGNALS:
        void PathSelected(const AZStd::string& path);

    private:
        AZ_DISABLE_COPY_MOVE(AssetSelectionComboBox);

        // AzFramework::AssetCatalogEventBus::Handler overrides ...
        void OnCatalogAssetAdded(const AZ::Data::AssetId& assetId) override;
        void OnCatalogAssetRemoved(const AZ::Data::AssetId& assetId, const AZ::Data::AssetInfo& assetInfo) override;

        void RegisterThumbnail(const AZStd::string& path);
        void UpdateThumbnail(const AZStd::string& path);
        void QueueUpdateThumbnail(const AZStd::string& path);
        void QueueSort();

        FilterFn m_filterFn;

        bool m_thumbnailsEnabled = false;
        AZ::u32 m_thumbnailDelayMs = 2000;
        AZStd::unordered_map<AZStd::string, AzToolsFramework::Thumbnailer::SharedThumbnailKey> m_thumbnailKeys;
        bool m_queueSort = false;
    };
} // namespace AtomToolsFramework
