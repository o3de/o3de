/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/std/containers/vector.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Gem/GemInfo.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Utils/Utils.h>

#include <AzToolsFramework/AssetBrowser/Entries/RootAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/FolderAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/SourceAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/SourceAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/ProductAssetBrowserEntry.h>
#include <AzToolsFramework/AssetDatabase/AssetDatabaseConnection.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntryCache.h>
#include <AzToolsFramework/AssetBrowser/Favorites/AssetBrowserFavoritesManager.h>
#include <QVariant>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        inline QString AzToQtUtf8String(AZStd::string_view str)
        {
            return QString::fromUtf8(str.data(), static_cast<int>(str.size()));
        }

        RootAssetBrowserEntry::RootAssetBrowserEntry()
            : AssetBrowserEntry()
        {
            EntryCache::CreateInstance();
            AssetBrowserFavoritesManager::CreateInstance();
        }

        AssetBrowserEntry::AssetEntryType RootAssetBrowserEntry::GetEntryType() const
        {
            return AssetEntryType::Root;
        }


        void RootAssetBrowserEntry::PrepareForReset()
        {
            RemoveChildren();
            auto entryCache = EntryCache::GetInstance();
            entryCache->Clear();
        }

        void RootAssetBrowserEntry::Update(const char* enginePath)
        {
            m_enginePath = AZ::IO::Path(enginePath).LexicallyNormal();
            m_projectPath = AZ::IO::Path(AZ::Utils::GetProjectPath()).LexicallyNormal();
            SetFullPath(m_enginePath);
            m_gemNames.clear();

            AZ::SettingsRegistryInterface* settingsRegistry = AZ::SettingsRegistry::Get();
            if (settingsRegistry != nullptr)
            {
                AZStd::vector<AzFramework::GemInfo> gemInfoList;
                AzFramework::GetGemsInfo(gemInfoList, *settingsRegistry);
                for (const AzFramework::GemInfo& gemInfo : gemInfoList)
                {
                    m_gemNames.insert(gemInfo.m_absoluteSourcePaths.begin(), gemInfo.m_absoluteSourcePaths.end());
                }
            }
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
            auto entryCache = EntryCache::GetInstance();

            // if it doesn't exist on disk yet, don't create it on the gui, yet.  We cache its info for later.
            entryCache->m_knownScanFolders[scanFolderDatabaseEntry.m_scanFolderID] = scanFolderDatabaseEntry.m_scanFolder;

            if (AZ::IO::FileIOBase::GetInstance()->IsDirectory(scanFolderDatabaseEntry.m_scanFolder.c_str()))
            {
                const auto& scanFolder = CreateFolders(scanFolderDatabaseEntry.m_scanFolder, this, true);
                entryCache->m_scanFolderIdMap[scanFolderDatabaseEntry.m_scanFolderID] = scanFolder;
            }
        }

        void RootAssetBrowserEntry::AddFile(const AssetDatabase::FileDatabaseEntry& fileDatabaseEntry)
        {
            using namespace AzFramework;
            
            AssetBrowserEntry* scanFolder = nullptr;

            auto entryCache = EntryCache::GetInstance();
            auto itScanFolder = entryCache->m_scanFolderIdMap.find(fileDatabaseEntry.m_scanFolderPK);
            if (itScanFolder == entryCache->m_scanFolderIdMap.end())
            {
                // this scanfolder hasn't been created. it probably just now popped into existence, so create the element now:
                // the only thing we need to know is what the path to the scanfolder is.
                auto scanFolderDetailsIt = entryCache->m_knownScanFolders.find(fileDatabaseEntry.m_scanFolderPK);
                if (scanFolderDetailsIt == entryCache->m_knownScanFolders.end())
                {
                    // we can't find the details.
                    return;
                }

                scanFolder = CreateFolders(scanFolderDetailsIt->second, this, true);
                entryCache->m_scanFolderIdMap[fileDatabaseEntry.m_scanFolderPK] = scanFolder;
            }
            else
            {
                scanFolder = itScanFolder->second;
            }

            // verify that file does not already exist
            const auto itFile = entryCache->m_fileIdMap.find(fileDatabaseEntry.m_fileID);
            if (itFile != entryCache->m_fileIdMap.end())
            {
                // The file already exists. This might occur if the content creator or other user is iterating and resaving source files and
                // assets.
                return;
            }

            AZ::IO::FixedMaxPath absoluteFilePath = AZ::IO::FixedMaxPath(AZStd::string_view{ scanFolder->GetFullPath() })
                / fileDatabaseEntry.m_fileName.c_str();

            AssetBrowserEntry* file{};
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
                source->m_displayName = AzToQtUtf8String(source->m_name);
                source->m_scanFolderId = fileDatabaseEntry.m_scanFolderPK;
                source->m_extension = absoluteFilePath.Extension().Native();
                source->m_diskSize = AZ::IO::SystemFile::Length(absoluteFilePath.c_str());
                source->m_modificationTime = AZ::IO::SystemFile::ModificationTime(absoluteFilePath.c_str());

                AZ::IO::FixedMaxPath assetPath;
                if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
                {
                    settingsRegistry->Get(assetPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_CacheRootFolder);
                    assetPath /= fileDatabaseEntry.m_fileName + ".abdata.json";

                    auto result = AZ::JsonSerializationUtils::ReadJsonFile(assetPath.Native());

                    if (result)
                    {
                        auto& doc = result.GetValue();

                        const rapidjson::Value& metadata = doc["metadata"];
                        if (metadata.HasMember("dimension"))
                        {
                            const rapidjson::Value& dimension = metadata["dimension"];
                            if (dimension.IsArray())
                            {
                                source->m_dimension.SetX(static_cast<float>(dimension[0].GetDouble()));
                                source->m_dimension.SetY(static_cast<float>(dimension[1].GetDouble()));
                                source->m_dimension.SetZ(static_cast<float>(dimension[2].GetDouble()));
                            }
                        }
                        if (metadata.HasMember("vertices"))
                        {
                            const rapidjson::Value& vertices = metadata["vertices"];
                            if (vertices.IsUint())
                            {
                                source->m_vertices = vertices.GetUint();
                            }
                        }
                    }
                }
                parent->AddChild(source);
                file = source;
            }

            entryCache->m_fileIdMap[fileDatabaseEntry.m_fileID] = file;

            const AZStd::string filePath = AZ::IO::PathView(file->m_fullPath).LexicallyNormal().String();
            entryCache->m_absolutePathToFileId[filePath] = fileDatabaseEntry.m_fileID;
        }

        bool RootAssetBrowserEntry::RemoveFile(const AZ::s64& fileId) const
        {
            auto entryCache = EntryCache::GetInstance();
            auto fileIdNodeHandle = entryCache->m_fileIdMap.extract(fileId);
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
            entryCache->m_absolutePathToFileId.erase(fullPath);
            return true;
        }

        bool RootAssetBrowserEntry::AddSource(const SourceWithFileID& sourceWithFileIdEntry) const
        {
            auto entryCache = EntryCache::GetInstance();
            const auto itFile = entryCache->m_fileIdMap.find(sourceWithFileIdEntry.first);
            if (itFile == entryCache->m_fileIdMap.end())
            {
                AZ_Warning("Asset Browser", false, "Add source failed: file %d not found, retrying later", sourceWithFileIdEntry.first);
                return false;
            }

            auto source = static_cast<SourceAssetBrowserEntry*>(itFile->second); // it MUST be a source to get here.
            source->m_sourceId = sourceWithFileIdEntry.second.m_sourceID;
            source->m_sourceUuid = sourceWithFileIdEntry.second.m_sourceGuid;
            source->PathsUpdated(); // update thumbnailkey to valid uuid
            entryCache->m_sourceUuidMap[source->m_sourceUuid] = source;
            entryCache->m_sourceIdMap[source->m_sourceId] = source;
            return true;
        }

        void RootAssetBrowserEntry::RemoveSource(const AZ::Uuid& sourceUuid) const
        {
            auto entryCache = EntryCache::GetInstance();
            const auto itSource = entryCache->m_sourceUuidMap.find(sourceUuid);
            if (itSource == entryCache->m_sourceUuidMap.end())
            {
                return;
            }

            AZStd::vector<const ProductAssetBrowserEntry*> products;
            itSource->second->GetChildren<ProductAssetBrowserEntry>(products);
            for (const ProductAssetBrowserEntry* product : products)
            {
                RemoveProduct(product->m_assetId);
            }

            entryCache->m_sourceIdMap.erase(itSource->second->m_sourceId);
            itSource->second->m_sourceId = -1;
            itSource->second->m_sourceUuid = AZ::Uuid::CreateNull();

            entryCache->m_sourceUuidMap.erase(itSource);
        }

        bool RootAssetBrowserEntry::AddProduct(const ProductWithUuid& productWithUuidDatabaseEntry)
        {
            auto entryCache = EntryCache::GetInstance();
            auto itSource = entryCache->m_sourceUuidMap.find(productWithUuidDatabaseEntry.first);
            if (itSource == entryCache->m_sourceUuidMap.end())
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
            const auto itProduct = entryCache->m_productAssetIdMap.find(assetId);
            bool needsAdd = false;
            if (itProduct != entryCache->m_productAssetIdMap.end())
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
            product->m_visiblePath = cleanedRelative;
            product->SetFullPath((AZ::IO::Path("@products@") / cleanedRelative).LexicallyNormal());

            AZStd::string assetGroupName;
            AZ::AssetTypeInfoBus::EventResult(
                assetGroupName, productWithUuidDatabaseEntry.second.m_assetType, &AZ::AssetTypeInfo::GetGroup);
            product->m_groupName = AzToQtUtf8String(assetGroupName);
            product->m_groupNameCrc = AZ::Crc32(assetGroupName);

            // compute the display data from the above data.
            AZStd::string assetTypeName;
            AZ::AssetTypeInfoBus::EventResult(
                assetTypeName, productWithUuidDatabaseEntry.second.m_assetType, &AZ::AssetTypeInfo::GetAssetTypeDisplayName);

            // Create a display name for the product that includes the asset type name, if it is available.
            product->m_displayName = AzToQtUtf8String(
                !assetTypeName.empty() ? AZStd::string::format("%s (%s)", productName.c_str(), assetTypeName.c_str()) : product->m_name);

            product->m_displayPath = AzToQtUtf8String(AZ::IO::Path(cleanedRelative.ParentPath()).Native());
            entryCache->m_productAssetIdMap[assetId] = product;

            if (needsAdd)
            {
                // save this for last since it actually causes other lookups (like thumbnail lookups) to occur.
                source->AddChild(product);
            }

            return true;
        }

        void RootAssetBrowserEntry::RemoveProduct(const AZ::Data::AssetId& assetId) const
        {
            auto entryCache = EntryCache::GetInstance();
            if (const auto itProduct = entryCache->m_productAssetIdMap.find(assetId); itProduct != entryCache->m_productAssetIdMap.end())
            {
                if (auto parent = itProduct->second->GetParent())
                {
                    parent->RemoveChild(itProduct->second);
                }
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
                return (entry->GetEntryType() == AssetEntryType::Folder) && AZ::IO::PathView(entry->m_name) == AZ::IO::PathView(folderName);
            });
            
            if (it != parent->m_children.end())
            {
                return static_cast<FolderAssetBrowserEntry*>(*it); // RTTI Cast is not necessary since find_if only returns folders.
            }

            auto folder = aznew FolderAssetBrowserEntry();
            folder->m_name = folderName;
            folder->m_displayName = AzToQtUtf8String(folderName);
            folder->m_isScanFolder = isScanFolder;
            parent->AddChild(folder);
            folder->m_isGemFolder = m_gemNames.contains(folder->GetFullPath());
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

            // If the project is not inside the root engine folder
            if (!m_projectPath.IsRelativeTo(m_enginePath))
            {
                // Update the parent to be the project directory if it isn't already
                if (absolutePathView.IsRelativeTo(m_projectPath) && !parent->m_fullPath.IsRelativeTo(m_projectPath))
                {
                    parent->SetFullPath(m_projectPath.ParentPath());
                }
                // Update the parent to be the o3de directory if it isn't already
                else if (absolutePathView.IsRelativeTo(m_enginePath) && !parent->m_fullPath.IsRelativeTo(m_enginePath))
                {
                    parent->SetFullPath(m_enginePath.ParentPath());
                }
            }

            // create all missing folders
            const auto proximateToPath = absolutePathView.IsRelativeTo(parent->m_fullPath)
                ? absolutePathView.LexicallyProximate(parent->m_fullPath)
                : AZ::IO::FixedMaxPath(absolutePathView);

            AZ::IO::FixedMaxPath constructor;
            for (const AZ::IO::FixedMaxPath& scanFolderSegment : proximateToPath)
            {
                constructor = constructor / scanFolderSegment;
                // if this is the final segment, and we are in a scan folder, this represents the actual scan folder.
                bool isTheScanFolder = isScanFolder && (constructor == proximateToPath);
                parent = CreateFolder(scanFolderSegment.Native(), parent, isTheScanFolder);
            }

            return parent;
        }

        void RootAssetBrowserEntry::UpdateChildPaths(AssetBrowserEntry* child) const
        {
            // note that the children of roots have the same fullpath of the child itself
            // they don't inherit a path from the root because the root is an invisible no-path not
            // shown root.
            child->SetFullPath(m_fullPath / child->m_name);
            child->m_relativePath = child->m_name;
            child->m_visiblePath = child->m_name;

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
