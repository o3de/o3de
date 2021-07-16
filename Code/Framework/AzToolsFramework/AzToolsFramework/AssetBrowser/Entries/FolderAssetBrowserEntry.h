/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Math/Uuid.h>

#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntry.h>
#include <AzToolsFramework/Thumbnails/Thumbnail.h>

namespace AZ
{
    class ReflectContext;
}

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        //! FolderAssetBrowserEntry is a class for any folder.
        class FolderAssetBrowserEntry
            : public AssetBrowserEntry
        {
            friend class RootAssetBrowserEntry;

        public:
            AZ_RTTI(FolderAssetBrowserEntry, "{938E6FCD-1582-4B63-A7EA-5C4FD28CABDC}", AssetBrowserEntry);
            AZ_CLASS_ALLOCATOR(FolderAssetBrowserEntry, AZ::SystemAllocator, 0);

            FolderAssetBrowserEntry() = default;
            ~FolderAssetBrowserEntry() override = default;

            static void Reflect(AZ::ReflectContext* context);

            AssetEntryType GetEntryType() const override;
            bool IsGemsFolder() const;

            SharedThumbnailKey CreateThumbnailKey() override;

        protected:
            void UpdateChildPaths(AssetBrowserEntry* child) const override;

        private:
            bool m_isGemsFolder = false;

            AZ_DISABLE_COPY_MOVE(FolderAssetBrowserEntry);
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework
