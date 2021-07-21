/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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

class QMimeData;

namespace AZ
{
    class ReflectContext;
}

namespace AzToolsFramework
{
    using namespace Thumbnailer;

    namespace AssetDatabase
    {
        class ScanFolderDatabaseEntry;
        class FileDatabaseEntry;
        class SourceDatabaseEntry;
        class ProductDatabaseEntry;
        class CombinedDatabaseEntry;
    }
    
    namespace AssetBrowser
    {
        using ProductWithUuid = AZStd::pair<AZ::Uuid, AssetDatabase::ProductDatabaseEntry>;
        using SourceWithFileID = AZStd::pair<AZ::s64, AssetDatabase::SourceDatabaseEntry>;

        //! RootAssetBrowserEntry is a root node for Asset Browser tree view, it's not related to any asset path.
        class RootAssetBrowserEntry
            : public AssetBrowserEntry
        {
        public:
            AZ_RTTI(RootAssetBrowserEntry, "{A35CA80E-E1EB-420B-8BFE-B7792E3CCEDB}");
            AZ_CLASS_ALLOCATOR(RootAssetBrowserEntry, AZ::SystemAllocator, 0);

            RootAssetBrowserEntry();

            static void Reflect(AZ::ReflectContext* context);

            AssetEntryType GetEntryType() const override;

            //! Update root node to new engine location
            void Update(const char* enginePath);

            void AddScanFolder(const AssetDatabase::ScanFolderDatabaseEntry& scanFolderDatabaseEntry);
            void AddFile(const AssetDatabase::FileDatabaseEntry& fileDatabaseEntry);
            bool RemoveFile(const AZ::s64& fileId) const;
            bool AddSource(const SourceWithFileID& sourceWithFileIdEntry) const;
            void RemoveSource(const AZ::Uuid& sourceUuid) const;
            bool AddProduct(const ProductWithUuid& productWithUuidEntry);
            void RemoveProduct(const AZ::Data::AssetId& assetId) const;

            SharedThumbnailKey CreateThumbnailKey() override;

            bool IsInitialUpdate() const;
            void SetInitialUpdate(bool newValue);

        protected:
            void UpdateChildPaths(AssetBrowserEntry* child) const override;

        private:
            AZ_DISABLE_COPY_MOVE(RootAssetBrowserEntry);

            AZStd::string m_enginePath;

            //! Create folder entry child
            FolderAssetBrowserEntry* CreateFolder(const char* folderName, AssetBrowserEntry* parent);
            //! Recursively create folder structure leading to relative path from parent
            AssetBrowserEntry* CreateFolders(const char* relativePath, AssetBrowserEntry* parent);

            bool m_isInitialUpdate = false;
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework
