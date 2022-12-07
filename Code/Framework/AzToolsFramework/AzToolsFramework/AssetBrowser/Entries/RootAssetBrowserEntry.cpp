/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/std/containers/vector.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>

#include <AzFramework/StringFunc/StringFunc.h>

#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Path/Path.h>

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
        RootAssetBrowserEntry::RootAssetBrowserEntry()
            : AssetBrowserEntry()
        {
            EntryCache::CreateInstance();
        }

        AssetBrowserEntry::AssetEntryType RootAssetBrowserEntry::GetEntryType() const
        {
            return AssetEntryType::Root;
        }

        void RootAssetBrowserEntry::Update(const char* enginePath)
        {
            RemoveChildren();
            EntryCache::GetInstance()->Clear();

            m_enginePath = AZ::IO::Path(enginePath).LexicallyNormal();
            m_fullPath = m_enginePath;
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
                const auto scanFolder = CreateFolders(scanFolderDatabaseEntry.m_scanFolder, this, true);
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

                scanFolder = CreateFolders(scanFolderDetailsIt->second.c_str(), this, true);
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

            AZ::IO::FixedMaxPath absoluteFilePath = AZ::IO::FixedMaxPath(AZStd::string_view{ scanFolder->GetFullPath() })
                / fileDatabaseEntry.m_fileName.c_str();

            AssetBrowserEntry* file;
            // file can be either folder or actual file
            if (fileDatabaseEntry.m_isFolder)
            {
                file = CreateFolders(absoluteFilePath.Native(), scanFolder, false);
            }
            else
            {
                // if missing create folders leading to file's location and get immediate parent
                // (we don't need to have fileIds for any folders created yet, they will be added later)
                auto parent = CreateFolders(absoluteFilePath.ParentPath().Native(), scanFolder, false);
                // for simplicity in AB, files are represented as sources, but they are missing SourceDatabaseEntry-specific information such as SourceUuid
                auto source = aznew SourceAssetBrowserEntry();
                source->m_name = absoluteFilePath.Filename().Native();
                source->m_fileId = fileDatabaseEntry.m_fileID;
                source->m_displayName = QString::fromUtf8(source->m_name.c_str());
                source->m_scanFolderId = fileDatabaseEntry.m_scanFolderPK;
                source->m_extension = absoluteFilePath.Extension().Native();
                parent->AddChild(source);
                file = source;
            }

            EntryCache::GetInstance()->m_fileIdMap[fileDatabaseEntry.m_fileID] = file;
            AZStd::string filePath = AZ::IO::PathView(file->m_fullPath).LexicallyNormal().String();
            EntryCache::GetInstance()->m_absolutePathToFileId[filePath] = fileDatabaseEntry.m_fileID;
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

            product->m_name = productName + productExtension;
            product->m_productId = productWithUuidDatabaseEntry.second.m_productID;
            product->m_jobId = productWithUuidDatabaseEntry.second.m_jobPK;
            product->m_assetId = assetId;
            product->m_assetType = productWithUuidDatabaseEntry.second.m_assetType;
            product->m_assetType.ToString(product->m_assetTypeString);

            // note that products from the database come in with a path relative to the cache root
            // and includes the platform name.  For example, "pc/textures/blah.tex", not just "textures/blah.tex"
            // this path is relative to the cache root.  What we actually want is a path relative to the 'products'
            // alias.
            // what this boils down to, is, always remove the first element to form a relative path.
            // the absolute path is just the relative path with cache root prepended.
            AZ::IO::Path pathFromDatabase(productWithUuidDatabaseEntry.second.m_productName.c_str());
            AZ::IO::PathView cleanedRelative = pathFromDatabase.RelativePath();
            AZ::IO::FixedMaxPath storageForLexicallyRelative{};
            // remove the first element from the path if you can:
            if (!cleanedRelative.empty())
            {
                storageForLexicallyRelative = cleanedRelative.LexicallyRelative(*cleanedRelative.begin());
                cleanedRelative = storageForLexicallyRelative;
            }
            product->m_relativePath = cleanedRelative;
            product->m_fullPath = (AZ::IO::Path("@products@") / cleanedRelative).LexicallyNormal();

            // compute the display data from the above data.
            // does someone have information about a more friendly name for this type?
            AZStd::string assetTypeName;
            AZ::AssetTypeInfoBus::EventResult(
                assetTypeName, productWithUuidDatabaseEntry.second.m_assetType, &AZ::AssetTypeInfo::GetAssetTypeDisplayName);

            AZStd::string displayName;
            if (!assetTypeName.empty())
            {
                // someone has more friendly data - use the friendly name
                displayName = AZStd::string::format("%s (%s)", productName.c_str(), assetTypeName.c_str());
            }
            else
            {
                // just use the product name (with extension)
                displayName = product->m_name; 
            }
            product->m_displayName = QString::fromUtf8(displayName.c_str());
            product->m_displayPath = QString::fromUtf8(AZ::IO::Path(cleanedRelative.ParentPath()).c_str());
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

        AssetBrowserEntry* RootAssetBrowserEntry::GetNearestAncestor(AZ::IO::PathView absolutePathView, AssetBrowserEntry* parent,
            AZStd::unordered_set<AssetBrowserEntry*>& visitedSet)
        {
            auto IsPathRelativeToEntry = [absolutePathView](AssetBrowserEntry* assetBrowserEntry)
            {
                auto& childPath = assetBrowserEntry->m_fullPath;
                return absolutePathView.IsRelativeTo(AZ::IO::PathView(childPath));
            };

            if (visitedSet.contains(parent))
            {
                return {};
            }

            visitedSet.insert(parent);

            AssetBrowserEntry* nearestAncestor{};
            for (AssetBrowserEntry* childBrowserEntry : parent->m_children)
            {
                if (IsPathRelativeToEntry(childBrowserEntry))
                {
                    // Walk the AssetBrowserEntry Tree looking for a nearer ancestor to the absolute path
                    // If one is not found in the recursive call to GetNearestAncestor, then the childBrowserEntry
                    // is the current best candidate
                    AssetBrowserEntry* candidateAncestor = GetNearestAncestor(absolutePathView, childBrowserEntry, visitedSet);
                    candidateAncestor = candidateAncestor != nullptr ? candidateAncestor : childBrowserEntry;
                    AZ::IO::PathView candidatePathView(candidateAncestor->m_fullPath);
                    // If the candidate is relative to the current nearest ancestor, then it is even nearer to the path
                    if (!nearestAncestor || candidatePathView.IsRelativeTo(nearestAncestor->m_fullPath))
                    {
                        nearestAncestor = candidateAncestor;
                        // If the full path compares equal to the AssetBrowserEntry path, then no need to proceed any further
                        if (AZ::IO::PathView(nearestAncestor->m_fullPath) == absolutePathView)
                        {
                            break;
                        }
                    }
                }
            }

            return nearestAncestor;
        }

        FolderAssetBrowserEntry* RootAssetBrowserEntry::CreateFolder(AZStd::string_view folderName, AssetBrowserEntry* parent, bool isScanFolder)
        {
            auto it = AZStd::find_if(parent->m_children.begin(), parent->m_children.end(), [folderName](AssetBrowserEntry* entry)
            {
                if (!azrtti_istypeof<FolderAssetBrowserEntry*>(entry))
                {
                    return false;
                }
                return AZ::IO::PathView(entry->m_name) == AZ::IO::PathView(folderName);
            });
            if (it != parent->m_children.end())
            {
                return azrtti_cast<FolderAssetBrowserEntry*>(*it);
            }
            const auto folder = aznew FolderAssetBrowserEntry();
            folder->m_name = folderName;
            folder->m_displayName = QString::fromUtf8(folderName.data(), aznumeric_caster(folderName.size()));
            folder->m_isScanFolder = isScanFolder;
            parent->AddChild(folder);
            return folder;
        }

        AssetBrowserEntry* RootAssetBrowserEntry::CreateFolders(AZStd::string_view absolutePath, AssetBrowserEntry* parent, bool isScanFolder)
        {
            AZ::IO::PathView absolutePathView(absolutePath);
            // Find the nearest ancestor path to the absolutePath
            AZStd::unordered_set<AssetBrowserEntry*> visitedSet;

            if (AssetBrowserEntry* nearestAncestor = GetNearestAncestor(absolutePathView, parent, visitedSet);
                nearestAncestor != nullptr)
            {
                parent = nearestAncestor;
            }

            // If the nearest ancestor is the absolutePath, then it is already created
            if (absolutePathView == AZ::IO::PathView(parent->GetFullPath()))
            {
                return parent;
            }

            // create all missing folders
            auto proximateToPath = absolutePathView.IsRelativeTo(parent->m_fullPath)
                ? absolutePathView.LexicallyProximate(parent->m_fullPath)
                : AZ::IO::FixedMaxPath(absolutePathView);

            AZ::IO::FixedMaxPath constructor;
            for (AZ::IO::FixedMaxPath scanFolderSegment : proximateToPath)
            {
                constructor = constructor / scanFolderSegment;
                // if this is the final segment, and we are in a scan folder, this represents the actual scan folder.
                bool isTheScanFolder = isScanFolder && (constructor == proximateToPath);
                parent = CreateFolder(scanFolderSegment.c_str(), parent, isTheScanFolder);
            }

            return parent;
        }

        void RootAssetBrowserEntry::UpdateChildPaths(AssetBrowserEntry* child) const
        {
            // note that the children of roots have the same fullpath of the child itself
            // they don't inherit a path from the root because the root is an invisible no-path not
            // shown root.
            child->m_fullPath = m_fullPath / child->m_name;
            child->m_relativePath = child->m_name;

            // the display path is the relative path without the child's name.  So it is blank here.
            child->m_displayPath = QString();

            AssetBrowserEntry::UpdateChildPaths(child);
        }

        SharedThumbnailKey RootAssetBrowserEntry::CreateThumbnailKey()
        {
            return MAKE_TKEY(ThumbnailKey);
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework
