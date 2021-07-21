/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/std/containers/vector.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>

#include <AzFramework/StringFunc/StringFunc.h>

#include <AzCore/IO/FileIO.h>

#include <AzToolsFramework/AssetBrowser/Entries/RootAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/FolderAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/SourceAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/ProductAssetBrowserEntry.h>
#include <AzToolsFramework/AssetDatabase/AssetDatabaseConnection.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntryCache.h>

#include <QVariant>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        const char* GEMS_FOLDER_NAME = "Gems";

        RootAssetBrowserEntry::RootAssetBrowserEntry()
            : AssetBrowserEntry()
        {
            EntryCache::CreateInstance();
        }

        void RootAssetBrowserEntry::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<RootAssetBrowserEntry>()
                    ->Version(1);
            }
        }

        AssetBrowserEntry::AssetEntryType RootAssetBrowserEntry::GetEntryType() const
        {
            return AssetEntryType::Root;
        }

        void RootAssetBrowserEntry::Update(const char* enginePath)
        {
            RemoveChildren();
            EntryCache::GetInstance()->Clear();

            m_enginePath = enginePath;

            // there is no "Gems" scan folder registered in db, create one manually
            auto gemFolder = aznew FolderAssetBrowserEntry();
            gemFolder->m_name = m_enginePath + AZ_CORRECT_DATABASE_SEPARATOR + GEMS_FOLDER_NAME;
            gemFolder->m_displayName = GEMS_FOLDER_NAME;
            gemFolder->m_isGemsFolder = true;
            AddChild(gemFolder);
        }

        bool RootAssetBrowserEntry::IsInitialUpdate() const
        {
            return m_isInitialUpdate;
        }

        void RootAssetBrowserEntry::SetInitialUpdate(bool newValue)
        {
            m_isInitialUpdate = newValue;
        }


        void RootAssetBrowserEntry::AddScanFolder(const AssetDatabase::ScanFolderDatabaseEntry& scanFolderDatabaseEntry)
        {
            // if it doesn't exist on disk yet, don't create it on the gui, yet.  We cache its info for later.
            EntryCache::GetInstance()->m_knownScanFolders[scanFolderDatabaseEntry.m_scanFolderID] = scanFolderDatabaseEntry.m_scanFolder;

            if (AZ::IO::FileIOBase::GetInstance()->IsDirectory(scanFolderDatabaseEntry.m_scanFolder.c_str()))
            {
                const auto scanFolder = CreateFolders(scanFolderDatabaseEntry.m_scanFolder.c_str(), this);
                scanFolder->m_displayName = QString::fromUtf8(scanFolderDatabaseEntry.m_displayName.c_str());
                EntryCache::GetInstance()->m_scanFolderIdMap[scanFolderDatabaseEntry.m_scanFolderID] = scanFolder;
            }
        }

        void RootAssetBrowserEntry::AddFile(const AssetDatabase::FileDatabaseEntry& fileDatabaseEntry)
        {
            using namespace AzFramework;
            
            AssetBrowserEntry* scanFolder = nullptr;

            auto itScanFolder = EntryCache::GetInstance()->m_scanFolderIdMap.find(fileDatabaseEntry.m_scanFolderPK);
            if (itScanFolder == EntryCache::GetInstance()->m_scanFolderIdMap.end())
            {
                // this scanfolder hasn't been created it, it probably just now popped into existence, so create the element now:
                // the only thing we need to know is what the path to the scanfolder is.
                auto scanFolderDetailsIt = EntryCache::GetInstance()->m_knownScanFolders.find(fileDatabaseEntry.m_scanFolderPK);
                if (scanFolderDetailsIt == EntryCache::GetInstance()->m_knownScanFolders.end())
                {
                    // we can't even find the details.
                    AZ_Assert(false, "No scan folder with id %d", fileDatabaseEntry.m_scanFolderPK);
                    return;
                }

                scanFolder = CreateFolders(scanFolderDetailsIt->second.c_str(), this);
                EntryCache::GetInstance()->m_scanFolderIdMap[fileDatabaseEntry.m_scanFolderPK] = scanFolder;
            }
            else
            {
                scanFolder = itScanFolder->second;
            }

            // verify that file does not already exist
            const auto itFile = EntryCache::GetInstance()->m_fileIdMap.find(fileDatabaseEntry.m_fileID);
            if (itFile != EntryCache::GetInstance()->m_fileIdMap.end())
            {
                AZ_Assert(false, "File %d already exists", fileDatabaseEntry.m_fileID);
                return;
            }

            const char* filePath = fileDatabaseEntry.m_fileName.c_str();

            AssetBrowserEntry* file;
            // file can be either folder or actual file
            if (fileDatabaseEntry.m_isFolder)
            {
                file = CreateFolders(filePath, scanFolder);
            }
            else
            {
                AZStd::string sourcePath;
                AZStd::string sourceName;
                AZStd::string sourceExtension;
                StringFunc::Path::Split(filePath, nullptr, &sourcePath, &sourceName, &sourceExtension);
                // if missing create folders leading to file's location and get immediate parent
                // (we don't need to have fileIds for any folders created yet, they will be added later)
                auto parent = CreateFolders(sourcePath.c_str(), scanFolder);
                // for simplicity in AB, files are represented as sources, but they are missing SourceDatabaseEntry-specific information such as SourceUuid
                auto source = aznew SourceAssetBrowserEntry();
                source->m_name = (sourceName + sourceExtension).c_str();
                source->m_fileId = fileDatabaseEntry.m_fileID;
                source->m_displayName = QString::fromUtf8(source->m_name.c_str());
                source->m_scanFolderId = fileDatabaseEntry.m_scanFolderPK;
                source->m_extension = sourceExtension.c_str();
                parent->AddChild(source);
                file = source;
            }

            EntryCache::GetInstance()->m_fileIdMap[fileDatabaseEntry.m_fileID] = file;
            AZStd::string fullPath = file->m_fullPath;
            AzFramework::StringFunc::Path::Normalize(fullPath);
            EntryCache::GetInstance()->m_absolutePathToFileId[fullPath] = fileDatabaseEntry.m_fileID;
        }

        bool RootAssetBrowserEntry::RemoveFile(const AZ::s64& fileId) const
        {
            auto fileIdNodeHandle = EntryCache::GetInstance()->m_fileIdMap.extract(fileId);
            if (fileIdNodeHandle.empty())
            {
                // file may have previously been removed if its parent folder was deleted
                // the order of messages received from AP is not always guaranteed,
                // so if we receive "remove folder" before "remove file" then this file would no longer exist in cache
                return true;
            }

            AssetBrowserEntry* entryToRemove = fileIdNodeHandle.mapped();
            AZStd::string fullPath = entryToRemove->GetFullPath();
            auto* source = azrtti_cast<SourceAssetBrowserEntry*>(entryToRemove);
            if (source && source->m_sourceId != -1)
            {
                RemoveSource(source->m_sourceUuid);
            }

            if (auto parent = entryToRemove->GetParent())
            {
                parent->RemoveChild(entryToRemove);
            }

            AzFramework::StringFunc::Path::Normalize(fullPath);
            EntryCache::GetInstance()->m_absolutePathToFileId.erase(fullPath);
            return true;
        }

        bool RootAssetBrowserEntry::AddSource(const SourceWithFileID& sourceWithFileIdEntry) const
        {
            const auto itFile = EntryCache::GetInstance()->m_fileIdMap.find(sourceWithFileIdEntry.first);
            if (itFile == EntryCache::GetInstance()->m_fileIdMap.end())
            {
                AZ_Warning("Asset Browser", false, "Add source failed: file %d not found, retrying later", sourceWithFileIdEntry.first);
                return false;
            }

            auto source = azrtti_cast<SourceAssetBrowserEntry*>(itFile->second);
            source->m_sourceId = sourceWithFileIdEntry.second.m_sourceID;
            source->m_sourceUuid = sourceWithFileIdEntry.second.m_sourceGuid;
            source->PathsUpdated(); // update thumbnailkey to valid uuid
            EntryCache::GetInstance()->m_sourceUuidMap[source->m_sourceUuid] = source;
            EntryCache::GetInstance()->m_sourceIdMap[source->m_sourceId] = source;
            return true;
        }

        void RootAssetBrowserEntry::RemoveSource(const AZ::Uuid& sourceUuid) const
        {
            const auto itSource = EntryCache::GetInstance()->m_sourceUuidMap.find(sourceUuid);
            if (itSource == EntryCache::GetInstance()->m_sourceUuidMap.end())
            {
                return;
            }

            AZStd::vector<const ProductAssetBrowserEntry*> products;
            itSource->second->GetChildren<ProductAssetBrowserEntry>(products);
            for (const ProductAssetBrowserEntry* product : products)
            {
                RemoveProduct(product->m_assetId);
            }

            EntryCache::GetInstance()->m_sourceIdMap.erase(itSource->second->m_sourceId);
            itSource->second->m_sourceId = -1;
            itSource->second->m_sourceUuid = AZ::Uuid::CreateNull();

            EntryCache::GetInstance()->m_sourceUuidMap.erase(itSource);
        }

        bool RootAssetBrowserEntry::AddProduct(const ProductWithUuid& productWithUuidDatabaseEntry)
        {
            auto itSource = EntryCache::GetInstance()->m_sourceUuidMap.find(productWithUuidDatabaseEntry.first);
            if (itSource == EntryCache::GetInstance()->m_sourceUuidMap.end())
            {
                return false;
            }

            auto source = itSource->second;
            
            if (!source)
            {
                AZ_Assert(false, "Source is invalid");
                return false;
            }

            const AZ::Data::AssetId assetId(AZ::Data::AssetId(productWithUuidDatabaseEntry.first, productWithUuidDatabaseEntry.second.m_subID));
            ProductAssetBrowserEntry* product;
            const auto itProduct = EntryCache::GetInstance()->m_productAssetIdMap.find(assetId);
            bool needsAdd = false;
            if (itProduct != EntryCache::GetInstance()->m_productAssetIdMap.end())
            {
                product = itProduct->second;
            }
            else
            {
                product = aznew ProductAssetBrowserEntry();
                needsAdd = true;
            }

            AZStd::string productPath;
            AZStd::string productName;
            AZStd::string productExtension;
            AzFramework::StringFunc::Path::Split(productWithUuidDatabaseEntry.second.m_productName.c_str(),
                nullptr, &productPath, &productName, &productExtension);

            AZStd::string assetTypeName;
            AZ::AssetTypeInfoBus::EventResult(assetTypeName, productWithUuidDatabaseEntry.second.m_assetType, &AZ::AssetTypeInfo::GetAssetTypeDisplayName);

            AZStd::string displayName;
            if (!assetTypeName.empty())
            {
                displayName = AZStd::string::format("%s (%s)", productName.c_str(), assetTypeName.c_str());
            }
            else
            {
                displayName = productName;
            }
            productName += productExtension;
            product->m_name = productName;
            product->m_displayName = QString::fromUtf8(displayName.c_str());
            product->m_productId = productWithUuidDatabaseEntry.second.m_productID;
            product->m_jobId = productWithUuidDatabaseEntry.second.m_jobPK;
            product->m_assetId = assetId;
            product->m_assetType = productWithUuidDatabaseEntry.second.m_assetType;
            product->m_assetType.ToString(product->m_assetTypeString);
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(product->m_relativePath, &AZ::Data::AssetCatalogRequests::GetAssetPathById, assetId);
            EntryCache::GetInstance()->m_productAssetIdMap[assetId] = product;

            if (needsAdd)
            {
                // save this for last since it actually causes other lookups (like thumbnail lookups) to occur.
                source->AddChild(product);
            }

            return true;
        }

        void RootAssetBrowserEntry::RemoveProduct(const AZ::Data::AssetId& assetId) const
        {
            const auto itProduct = EntryCache::GetInstance()->m_productAssetIdMap.find(assetId);
            if (itProduct == EntryCache::GetInstance()->m_productAssetIdMap.end())
            {
                return;
            }
            if (auto parent = itProduct->second->GetParent())
            {
                parent->RemoveChild(itProduct->second);
            }
        }

        FolderAssetBrowserEntry* RootAssetBrowserEntry::CreateFolder(const char* folderName, AssetBrowserEntry* parent)
        {
            auto it = AZStd::find_if(parent->m_children.begin(), parent->m_children.end(), [folderName](AssetBrowserEntry* entry)
                    {
                        if (!azrtti_istypeof<FolderAssetBrowserEntry*>(entry))
                        {
                            return false;
                        }
                        return AzFramework::StringFunc::Equal(entry->m_name.c_str(), folderName);
                    });
            if (it != parent->m_children.end())
            {
                return azrtti_cast<FolderAssetBrowserEntry*>(*it);
            }
            const auto folder = aznew FolderAssetBrowserEntry();
            folder->m_name = folderName;
            folder->m_displayName = folderName;
            parent->AddChild(folder);
            return folder;
        }

        AssetBrowserEntry* RootAssetBrowserEntry::CreateFolders(const char* relativePath, AssetBrowserEntry* parent)
        {
            auto children(parent->m_children);
            int n = 0;

            // check if folder with the same name already exists
            // step through every character in relativePath and compare to each child's relative path of suggested parent
            // if a character @n in child's rel path mismatches character at n in relativePath, remove that child from further search
            while (!children.empty() && relativePath[n])
            {
                AZStd::vector<AssetBrowserEntry*> toRemove;
                for (auto child : children)
                {
                    auto& childPath = azrtti_istypeof<RootAssetBrowserEntry*>(parent) ? child->m_fullPath : child->m_relativePath;

                    // child's path mismatched, remove it from search candidates
                    if (childPath.length() == n || childPath[n] != relativePath[n])
                    {
                        toRemove.push_back(child);

                        // it is possible that child may be a closer parent, substitute it as new potential parent
                        // e.g. child->m_relativePath = 'Gems', relativePath = 'Gems/Assets', old parent = root, new parent = Gems
                        if (childPath.length() == n && relativePath[n] == AZ_CORRECT_DATABASE_SEPARATOR)
                        {
                            parent = child;
                            relativePath += n; // advance relative path n characters since the parent has changed
                            n = 0; // Once the relative path pointer is advanced, reset n
                        }
                    }
                }
                for (auto entry : toRemove)
                {
                    children.erase(AZStd::remove(children.begin(), children.end(), entry), children.end());
                }
                n++;
            }

            // filter out the remaining children that don't end with '/' or '\0'
            // for example if folderName = "foo", while children may still remain with names like "foo123",
            // which is not the same folder
            AZStd::vector<AssetBrowserEntry*> toRemove;
            for (auto child : children)
            {
                auto& childPath = azrtti_istypeof<RootAssetBrowserEntry*>(parent) ? child->m_fullPath : child->m_relativePath;
                // check if there are non-null characters remaining @n
                if (childPath.length() > n)
                {
                    toRemove.push_back(child);
                }
            }
            for (auto entry : toRemove)
            {
                children.erase(AZStd::remove(children.begin(), children.end(), entry), children.end());
            }

            // at least one child remains, this means the folder with this name already exists, return it
            if (!children.empty())
            {
                parent = children.front();
            }
            // if it's a scanfolder, then do not create folders leading to it
            // e.g. instead of 'C:\dev\SampleProject' just create 'SampleProject'
            else if (parent->GetEntryType() == AssetEntryType::Root)
            {
                AZStd::string folderName;
                AzFramework::StringFunc::Path::Split(relativePath, nullptr, nullptr, &folderName);
                parent = CreateFolder(folderName.c_str(), parent);
                parent->m_fullPath = relativePath;
            }
            // otherwise create all missing folders
            else
            {
                n = 0;
                AZStd::string folderName(strlen(relativePath) + 1, '\0');
                // iterate through relativePath until the first '/'
                while (relativePath[n] && relativePath[n] != AZ_CORRECT_DATABASE_SEPARATOR)
                {
                    folderName[n] = relativePath[n];
                    n++;
                }
                if (n > 0)
                {
                    parent = CreateFolder(folderName.c_str(), parent);
                }
                // n+1 also skips the '/' character
                if (relativePath[n] && relativePath[n + 1])
                {
                    parent = CreateFolders(relativePath + n + 1, parent);
                }
            }
            return parent;
        }

        void RootAssetBrowserEntry::UpdateChildPaths(AssetBrowserEntry* child) const
        {
            child->m_relativePath = child->m_name;
            child->m_fullPath = child->m_name;
            AssetBrowserEntry::UpdateChildPaths(child);
        }

        SharedThumbnailKey RootAssetBrowserEntry::CreateThumbnailKey()
        {
            return MAKE_TKEY(ThumbnailKey);
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework
