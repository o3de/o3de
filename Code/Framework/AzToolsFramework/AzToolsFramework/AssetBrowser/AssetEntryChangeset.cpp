/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
            for (auto change : m_changes)
            {
                delete change;
            }
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
            AZStd::vector<AssetEntryChange*> changesFailed;
            for (auto change : m_changes)
            {
                if (change->Apply(m_rootEntry))
                {
                    delete change;
                }
                else
                {
                    changesFailed.push_back(change);
                }
            }

#if AZ_DEBUG_BUILD
            if (m_changes.size() > 0)
            {
                AZ_TracePrintf("Asset Browser DEBUG", "%d/%d data changes applied\n", m_changes.size() - changesFailed.size(), m_changes.size());
            }
#endif
            // try again next time.
            m_changes = changesFailed;

            if (m_rootEntry->IsInitialUpdate())
            {
                m_rootEntry->SetInitialUpdate(false);
            }
        }

        void AssetEntryChangeset::AddFile(const AZ::s64& fileId)
        {
            AZStd::lock_guard<AZStd::mutex> locker(m_mutex);
            m_fileIdsToAdd.push_back(fileId);
        }

        void AssetEntryChangeset::RemoveFile(const AZ::s64& fileId)
        {
            AZStd::lock_guard<AZStd::mutex> locker(m_mutex);
            m_changes.push_back(aznew RemoveFileChange(fileId));
        }


        void AssetEntryChangeset::AddSource(const AZ::Uuid& sourceUuid)
        {
            AZStd::lock_guard<AZStd::mutex> locker(m_mutex);
            m_sourceUuidsToAdd.push_back(sourceUuid);
        }

        void AssetEntryChangeset::RemoveSource(const AZ::Uuid& sourceUuid)
        {
            AZStd::lock_guard<AZStd::mutex> locker(m_mutex);
            m_changes.push_back(aznew RemoveSourceChange(sourceUuid));
        }

        void AssetEntryChangeset::AddProduct(const AZ::Data::AssetId& assetId)
        {
            AZStd::lock_guard<AZStd::mutex> locker(m_mutex);
            m_productAssetIdsToAdd.push_back(assetId);
        }

        void AssetEntryChangeset::RemoveProduct(const AZ::Data::AssetId& assetId)
        {
            AZStd::lock_guard<AZStd::mutex> locker(m_mutex);
            auto findPredicate = [&assetId](const AssetEntryChange* toCheck)
            {
                if (const AddProductChange* productChange = azrtti_cast<const AddProductChange*>(toCheck))
                {
                    return productChange->GetAssetId() == assetId;
                }

                return false;
            };

            // make sure any pending "add" commands are erased.
            m_productAssetIdsToAdd.erase(AZStd::remove(m_productAssetIdsToAdd.begin(), m_productAssetIdsToAdd.end(), assetId), m_productAssetIdsToAdd.end());
            auto foundExisting = AZStd::find_if(m_changes.begin(), m_changes.end(), findPredicate);
            if (foundExisting != m_changes.end())
            {
                // remove the still-pending "add Product" from the list, no longer necesary.
                m_changes.erase(foundExisting);
            }

            m_changes.push_back(aznew RemoveProductChange(assetId));
            

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
                    m_changes.push_back(aznew AddScanFolderChange(scanFolder));
                    return m_databaseConnection->QueryFilesByScanFolderID(scanFolder.m_scanFolderID,
                        [&](FileDatabaseEntry& file)
                        {
                            m_changes.push_back(aznew AddFileChange(file));
                            return m_databaseConnection->QuerySourceBySourceNameScanFolderID(file.m_fileName.c_str(), scanFolder.m_scanFolderID,
                                [&](SourceDatabaseEntry& source)
                                {
                                    m_changes.push_back(aznew AddSourceChange({ file.m_fileID, source }));
                                    return m_databaseConnection->QueryProductBySourceID(source.m_sourceID,
                                        [&](ProductDatabaseEntry& product)
                                        {
                                            m_changes.push_back(aznew AddProductChange({ source.m_sourceGuid, product }));
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

            AZStd::vector<AZ::s64> fileIdsToAdd;
            AZStd::vector<AZ::Uuid> sourceUuidsToAdd;
            AZStd::vector<AZ::Data::AssetId> productAssetIdsToAdd;

            {
                AZStd::lock_guard<AZStd::mutex> locker(m_mutex);
                fileIdsToAdd = AZStd::move(m_fileIdsToAdd);
                m_fileIdsToAdd.clear();
                sourceUuidsToAdd = AZStd::move(m_sourceUuidsToAdd);
                m_sourceUuidsToAdd.clear();
                productAssetIdsToAdd = AZStd::move(m_productAssetIdsToAdd);
                m_productAssetIdsToAdd.clear();
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
                                    m_changes.push_back(aznew AddFileChange(file));
                                }
                                return true;
                            });
                    });
            }

            for (const AZ::Data::AssetId& assetId : productAssetIdsToAdd)
            {
                if (AZStd::find(sourceUuidsToAdd.begin(), sourceUuidsToAdd.end(), assetId.m_guid) == sourceUuidsToAdd.end())
                {
                    sourceUuidsToAdd.push_back(assetId.m_guid);
                }

                m_databaseConnection->QueryProductBySourceGuidSubID(assetId.m_guid, assetId.m_subId,
                    [&](ProductDatabaseEntry& product)
                    {
                        AZStd::lock_guard<AZStd::mutex> locker(m_mutex);
                        m_changes.push_back(aznew AddProductChange({ assetId.m_guid, product }));
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
                                            m_changes.push_back(new AddSourceChange({ file.m_fileID, source }));
                                        }
                                        else
                                        {
                                            // if products belonging to entry in root folder are considered, remove them from changes
                                            m_changes.erase(AZStd::remove_if(m_changes.begin(), m_changes.end(),
                                                        [sourceUuid](AssetEntryChange* change)
                                                        {
                                                            auto addProductChange = azrtti_cast<AddProductChange*>(change);
                                                            if (addProductChange && addProductChange->GetUuid() == sourceUuid)
                                                            {
                                                                delete change;
                                                                return true;
                                                            }
                                                            return false;
                                                        }),
                                                    m_changes.end());
                                        }
                                        return true;
                                    });
                            });
                    });
            }
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework
