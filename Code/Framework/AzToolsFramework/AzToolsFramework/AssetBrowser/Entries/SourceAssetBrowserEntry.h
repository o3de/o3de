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
        //! SourceAssetBrowserEntry represents source entry.
        class SourceAssetBrowserEntry
            : public AssetBrowserEntry
        {
            friend class RootAssetBrowserEntry;

        public:
            AZ_RTTI(SourceAssetBrowserEntry, "{9FD4FF76-4CC3-4E96-953F-5BF63C2E1F1D}", AssetBrowserEntry);
            AZ_CLASS_ALLOCATOR(SourceAssetBrowserEntry, AZ::SystemAllocator);

            SourceAssetBrowserEntry() = default;
            ~SourceAssetBrowserEntry() override;

            QVariant data(int column) const override;
            AssetEntryType GetEntryType() const override;

            const AZStd::string GetExtension() const;
            const AZStd::string GetFileName() const;
            AZ::s64 GetFileID() const;
            AZ::s64 GetSourceID() const;
            AZ::s64 GetScanFolderID() const;

            //! returns the asset type of the first child (product) that isn't an invalid type.
            AZ::Data::AssetType GetPrimaryAssetType() const;

            //! Returns true if any children (products) are the given asset type
            bool HasProductType(const AZ::Data::AssetType& assetType) const;
            SharedThumbnailKey CreateThumbnailKey() override;
            SharedThumbnailKey GetSourceControlThumbnailKey() const;
            const AZ::Uuid& GetSourceUuid() const;

            static const SourceAssetBrowserEntry* GetSourceByUuid(const AZ::Uuid& sourceUuid);

        protected:
            void PathsUpdated() override;

        private:
            AZStd::string m_extension;
            AZ::s64 m_fileId = -1;
            AZ::s64 m_sourceId = -1;
            AZ::s64 m_scanFolderId = -1;
            AZ::Uuid m_sourceUuid;
            SharedThumbnailKey m_sourceControlThumbnailKey;

            void UpdateSourceControlThumbnail();

            AZ_DISABLE_COPY_MOVE(SourceAssetBrowserEntry);
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework
