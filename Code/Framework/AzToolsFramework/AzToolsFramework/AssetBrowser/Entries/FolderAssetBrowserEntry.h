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