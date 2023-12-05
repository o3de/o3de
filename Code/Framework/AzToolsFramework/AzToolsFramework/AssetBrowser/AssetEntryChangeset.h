/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzToolsFramework/AssetBrowser/Entries/RootAssetBrowserEntry.h>

namespace AzToolsFramework
{
    namespace AssetDatabase
    {
        class AssetDatabaseConnection;
        class FileDatabaseEntry;
        class ScanFolderDatabaseEntry;
        class SourceDatabaseEntry;
        class CombinedDatabaseEntry;
        class ProductDatabaseEntry;
    }

    namespace AssetBrowser
    {
        class AssetEntryChange;

        class AssetEntryChangeset
        {
        public:
            AssetEntryChangeset(
                AZStd::shared_ptr<AssetDatabase::AssetDatabaseConnection> databaseConnection,
                AZStd::shared_ptr<RootAssetBrowserEntry> rootEntry);
            ~AssetEntryChangeset();

            void PopulateEntries();
            void Update();
            void Synchronize();

            void AddFile(const AZ::s64& fileId);
            void RemoveFile(const AZ::s64& fileId);
            void AddSource(const AZ::Uuid& sourceUuid);
            void RemoveSource(const AZ::Uuid& sourceUuid);
            void AddProduct(const AZ::Data::AssetId& assetId);
            void RemoveProduct(const AZ::Data::AssetId& assetId);

        private:
            AZStd::shared_ptr<AssetDatabase::AssetDatabaseConnection> m_databaseConnection;

            AZStd::shared_ptr<RootAssetBrowserEntry> m_rootEntry;

            //! protects read/write to m_entries
            AZStd::mutex m_mutex;

            AZStd::atomic_bool m_fullUpdate;
            bool m_updated;

            AZStd::vector<AZStd::shared_ptr<AssetEntryChange>> m_changes;
            AZStd::unordered_set<AZ::s64> m_fileIdsToAdd;
            AZStd::unordered_set<AZ::Uuid> m_sourceUuidsToAdd;
            AZStd::unordered_set<AZ::Data::AssetId> m_productAssetIdsToAdd;

            AZStd::string m_relativePath;

            void QueryEntireDatabase();
            void QueryChangeset();
            
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework
