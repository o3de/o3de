/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Math/Uuid.h>

#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntry.h>
#include <AzToolsFramework/Thumbnails/Thumbnail.h>

#include <QObject>
#include <QModelIndex>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {       
        //! ProductAssetBrowserEntry represents product entry.
        class ProductAssetBrowserEntry
            : public AssetBrowserEntry
        {
            friend class RootAssetBrowserEntry;
        public:
            AZ_RTTI(ProductAssetBrowserEntry, "{52C02087-D68B-4E9D-BB8A-01E43CE51BA2}", AssetBrowserEntry);
            AZ_CLASS_ALLOCATOR(ProductAssetBrowserEntry, AZ::SystemAllocator);

            ProductAssetBrowserEntry() = default;
            ~ProductAssetBrowserEntry() override;

            QVariant data(int column) const override;
            AssetEntryType GetEntryType() const override;

            AZ::s64 GetProductID() const;
            AZ::s64 GetJobID() const;
            const AZ::Data::AssetId& GetAssetId() const;
            const AZ::Data::AssetType& GetAssetType() const;
            const AZStd::string& GetAssetTypeString() const;
            SharedThumbnailKey GetThumbnailKey() const override;
            SharedThumbnailKey CreateThumbnailKey() override;

            static ProductAssetBrowserEntry* GetProductByAssetId(const AZ::Data::AssetId& assetId);

            void SetThumbnailDirty() override;

        private:
            AZ::s64 m_productId = -1;
            AZ::s64 m_jobId = -1;
            AZ::Data::AssetId m_assetId;
            AZ::Data::AssetType m_assetType;
            AZStd::string m_assetTypeString;

            AZ_DISABLE_COPY_MOVE(ProductAssetBrowserEntry);
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework
