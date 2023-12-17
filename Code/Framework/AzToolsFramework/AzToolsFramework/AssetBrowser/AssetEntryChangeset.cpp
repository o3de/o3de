/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetEntryChangeset.h>
#include <AzToolsFramework/AssetDatabase/AssetDatabaseConnection.h>
#include <AzToolsFramework/AssetBrowser/Entries/RootAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/AssetEntryChange.h>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        AssetEntryChangeset::AssetEntryChangeset(
            AZStd::shared_ptr<AssetDatabase::AssetDatabaseConnection> databaseConnection,
            AZStd::shared_ptr<RootAssetBrowserEntry> rootEntry)
            : m_databaseConnection(databaseConnection)
            , m_rootEntry(rootEntry)
            , m_fullUpdate(false)
            , m_updated(false)
        {}

        AssetEntryChangeset::~AssetEntryChangeset()
        {
        }

        void AssetEntryChangeset::PopulateEntries()
        {
            m_fullUpdate = true;
        }

        void AssetEntryChangeset::Update()
        {
            if (m_fullUpdate)
            {
                AZStd::lock_guard<AZStd::mutex> locker(m_mutex);

                m_fullUpdate = false;
                m_updated = true;
                QueryEntireDatabase();
                m_fileIdsToAdd.clear();
                m_sourceUuidsToAdd.clear();
                m_productAssetIdsToAdd.clear();
            }
            else
            {
                QueryChangeset();
            }
        }

        void AssetEntryChangeset::Synchronize()
        {
            using namespace AssetDatabase;

            AZStd::lock_guard<AZStd::mutex> locker(m_mutex);

            if (m_updated)
            {
                m_rootEntry->SetInitialUpdate(true);
                m_rootEntry->Update(m_relativePath.c_str());
                m_updated = false;
            }

            // iterate through new changes and try to apply them
            // if application of change fails, try them again next tick
            AZStd::vector<AZStd::shared_ptr<AssetEntryChange>> changesFailed;
            changesFailed.reserve(m_changes.size());

            for (auto& change : m_changes)
            {
                if (!change->Apply(m_rootEntry))
                {
                    changesFailed.emplace_back(AZStd::move(change));
                }
            }

#if AZ_DEBUG_BUILD
            if (!m_changes.empty())
            {
                AZ_TracePrintf("Asset Browser DEBUG", "%d/%d data changes applied\n", m_changes.size() - changesFailed.size(), m_changes.size());
            }
#endif
            // try again next time.
            AZStd::swap(m_changes, changesFailed);

            if (m_rootEntry->IsInitialUpdate())
            {
                m_rootEntry->SetInitialUpdate(false);
            }
        }

        void AssetEntryChangeset::AddFile(const AZ::s64& fileId)
        {
            AZStd::lock_guard<AZStd::mutex> locker(m_mutex);
            m_fileIdsToAdd.insert(fileId);
        }

        void AssetEntryChangeset::RemoveFile(const AZ::s64& fileId)
        {
            AZStd::lock_guard<AZStd::mutex> locker(m_mutex);
            m_changes.emplace_back(aznew RemoveFileChange(fileId));
        }

        void AssetEntryChangeset::AddSource(const AZ::Uuid& sourceUuid)
        {
            AZStd::lock_guard<AZStd::mutex> locker(m_mutex);
            m_sourceUuidsToAdd.insert(sourceUuid);
        }

        void AssetEntryChangeset::RemoveSource(const AZ::Uuid& sourceUuid)
        {
            AZStd::lock_guard<AZStd::mutex> locker(m_mutex);
            m_changes.emplace_back(aznew RemoveSourceChange(sourceUuid));
        }

        void AssetEntryChangeset::AddProduct(const AZ::Data::AssetId& assetId)
        {
            AZStd::lock_guard<AZStd::mutex> locker(m_mutex);
            m_productAssetIdsToAdd.insert(assetId);
        }

        void AssetEntryChangeset::RemoveProduct(const AZ::Data::AssetId& assetId)
        {
            AZStd::lock_guard<AZStd::mutex> locker(m_mutex);

            // make sure any pending "add" commands are erased.
            m_productAssetIdsToAdd.erase(assetId);

            // remove the still-pending "add Product" from the list, no longer necesary.
            AZStd::erase_if(
                m_changes,
                [assetId](const auto& change)
                {
                    auto addProductChange = azrtti_cast<AddProductChange*>(change);
                    return addProductChange && addProductChange->GetAssetId() == assetId;
                });

            m_changes.emplace_back(aznew RemoveProductChange(assetId));
            

        }

        void AssetEntryChangeset::QueryEntireDatabase()
        {
            using namespace AssetDatabase;

            // querying the asset database for the root folder
            m_databaseConnection->QueryScanFolderByDisplayName(
                "root",
                [=](ScanFolderDatabaseEntry& scanFolderDatabaseEntry)
                {
                    m_relativePath = scanFolderDatabaseEntry.m_scanFolder.c_str();
                    return true;
                });
        
            // query all scanfolders
            m_databaseConnection->QueryScanFoldersTable(
                [&](ScanFolderDatabaseEntry& scanFolder)
                {
                    // ignore scanfolders that are non-recursive (e.g. dev folder), as they are used generally for system assets
                    if (scanFolder.m_isRoot)
                    {
                        return true;
                    }

                    m_changes.emplace_back(aznew AddScanFolderChange(scanFolder));
                    return m_databaseConnection->QueryFilesByScanFolderID(scanFolder.m_scanFolderID,
                        [&](FileDatabaseEntry& file)
                        {
                            m_changes.emplace_back(aznew AddFileChange(file));
                            return m_databaseConnection->QuerySourceBySourceNameScanFolderID(file.m_fileName.c_str(), scanFolder.m_scanFolderID,
                                [&](SourceDatabaseEntry& source)
                                {
                                    m_changes.emplace_back(aznew AddSourceChange({ file.m_fileID, source }));
                                    return m_databaseConnection->QueryProductBySourceID(source.m_sourceID,
                                        [&](ProductDatabaseEntry& product)
                                        {
                                            m_changes.emplace_back(aznew AddProductChange({ source.m_sourceGuid, product }));
                                            return true;
                                        });
                                });
                        });
                });
        }

        // this function translates from a series of incoming change notifies into an actual
        // changeset that can then be applied to the model.  Change notifies are brief and may contain
        // only minimal information such as fileId.  This transforms them into larger sequences of changes
        // that include the creation of intermediate parent(s) if necessary.
        void AssetEntryChangeset::QueryChangeset()
        {
            using namespace AssetDatabase;

            AZStd::unordered_set<AZ::s64> fileIdsToAdd;
            AZStd::unordered_set<AZ::Uuid> sourceUuidsToAdd;
            AZStd::unordered_set<AZ::Data::AssetId> productAssetIdsToAdd;

            {
                AZStd::lock_guard<AZStd::mutex> locker(m_mutex);
                AZStd::swap(fileIdsToAdd, m_fileIdsToAdd);
                AZStd::swap(sourceUuidsToAdd, m_sourceUuidsToAdd);
                AZStd::swap(productAssetIdsToAdd, m_productAssetIdsToAdd);
            }

            for (const AZ::s64& fileId : fileIdsToAdd)
            {
                m_databaseConnection->QueryFileByFileID(fileId,
                    [&](FileDatabaseEntry& file)
                    {
                        return m_databaseConnection->QueryScanFolderByScanFolderID(file.m_scanFolderPK,
                            [&](ScanFolderDatabaseEntry& scanFolder)
                            {
                                // ignore scanfolders that are non-recursive (e.g. dev folder), as they are used generally for system assets
                                if (!scanFolder.m_isRoot)
                                {
                                    AZStd::lock_guard<AZStd::mutex> locker(m_mutex);
                                    m_changes.emplace_back(aznew AddFileChange(file));
                                }
                                return true;
                            });
                    });
            }

            for (const AZ::Data::AssetId& assetId : productAssetIdsToAdd)
            {
                sourceUuidsToAdd.insert(assetId.m_guid);

                m_databaseConnection->QueryProductBySourceGuidSubID(assetId.m_guid, assetId.m_subId,
                    [&](ProductDatabaseEntry& product)
                    {
                        AZStd::lock_guard<AZStd::mutex> locker(m_mutex);
                        m_changes.emplace_back(aznew AddProductChange({ assetId.m_guid, product }));
                        return true;
                    });
            }

            for (const AZ::Uuid& sourceUuid : sourceUuidsToAdd)
            {
                m_databaseConnection->QuerySourceBySourceGuid(sourceUuid,
                    [&](SourceDatabaseEntry& source)
                    {
                        return m_databaseConnection->QueryFileByFileNameScanFolderID(source.m_sourceName.c_str(), source.m_scanFolderPK,
                            [&](FileDatabaseEntry& file)
                            {
                                return m_databaseConnection->QueryScanFolderByScanFolderID(file.m_scanFolderPK,
                                    [&](ScanFolderDatabaseEntry& scanFolder)
                                    {
                                        AZStd::lock_guard<AZStd::mutex> locker(m_mutex);
                                        // ignore scanfolders that are non-recursive (e.g. dev folder), as they are used generally for system assets
                                        if (!scanFolder.m_isRoot)
                                        {
                                            m_changes.emplace_back(new AddSourceChange({ file.m_fileID, source }));
                                        }
                                        else
                                        {
                                            // if products belonging to entry in root folder are considered, remove them from changes
                                            AZStd::erase_if(
                                                m_changes,
                                                [sourceUuid](const auto& change)
                                                {
                                                    auto addProductChange = azrtti_cast<AddProductChange*>(change);
                                                    return addProductChange && addProductChange->GetUuid() == sourceUuid;
                                                });
                                        }
                                        return true;
                                    });
                            });
                    });
            }
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework
