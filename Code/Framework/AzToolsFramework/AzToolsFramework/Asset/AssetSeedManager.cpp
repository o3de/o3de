
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Asset/AssetSeedManager.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Math/Sha1.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/Slice/SliceAsset.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzFramework/FileTag/FileTagBus.h>
#include <AzFramework/FileTag/FileTag.h>
#include <AzToolsFramework/Asset/AssetDebugInfo.h>
#include <AzToolsFramework/AssetCatalog/PlatformAddressedAssetCatalogBus.h>
#include <AzToolsFramework/AssetCatalog/PlatformAddressedAssetCatalog.h>


const char SeedFileExtension[] = "seed";
const char AssetListFileExtension[] = "assetlist";
const char ScriptCanvas[] = "scriptcanvas";
const char ScriptCanvasCompiled[] = "scriptcanvas_compiled";
const char ScriptCanvasFunction[] = "scriptcanvas_fn";
const char ScriptCanvasFunctionCompiled[] = "scriptcanvas_fn_compiled";

namespace AzToolsFramework
{
    AZStd::string GetSeedPath(AZ::Data::AssetId assetId, AzFramework::PlatformFlags platformFlags)
    {
        using namespace AzToolsFramework;
        auto platformIndices = AzFramework::PlatformHelper::GetPlatformIndicesInterpreted(platformFlags);
        for (const auto& platformId : platformIndices)
        {
            AZStd::string assetPath;
            AssetCatalog::PlatformAddressedAssetCatalogRequestBus::EventResult(assetPath, platformId, &AssetCatalog::PlatformAddressedAssetCatalogRequestBus::Events::GetAssetPathById, assetId);
            if (!assetPath.empty())
            {
                return assetPath;
            }
        }

        AZ_Warning("AssetSeedManager", false, "Unable to resolve path of Seed asset (%s) for the given platforms (%s).\n", assetId.ToString<AZStd::string>().c_str(), AzFramework::PlatformHelper::GetCommaSeparatedPlatformList(platformFlags).c_str());

        return {};
    }

    AZ::Data::AssetId GetAssetIdFromString(const AZStd::string& assetKey)
    {
        AZ::Data::AssetId assetId;
        auto found = assetKey.find(":");
        if (found != AZStd::string::npos)
        {
            assetId.m_subId = static_cast<AZ::u32>(atoi(assetKey.substr(found + 1).c_str()));
            assetId.m_guid = AZ::Uuid(assetKey.substr(0, found).c_str());
            return assetId;
        }

        return AZ::Data::AssetId();
    }

    AssetSeedManager::~AssetSeedManager()
    {
        m_sourceAssetTypeToRuntimeAssetTypeMap.clear();
    }

    bool AssetSeedManager::AddSeedAsset(AZ::Data::AssetId assetId, AzFramework::PlatformFlags platformFlags, AZStd::string path, const AZStd::string& seedListFilePath)
    {
        for (auto iter = m_assetSeedList.begin(); iter != m_assetSeedList.end(); ++iter)
        {
            if (iter->m_assetId == assetId)
            {
                if (iter->m_platformFlags == platformFlags)
                {
                    AZ_TracePrintf("AssetSeedManager", "Seed Asset ( %s ) is already present in the asset seed list.\n", assetId.ToString<AZStd::string>().c_str());
                    return false;
                }

                iter->m_platformFlags = iter->m_platformFlags | platformFlags;
                return true;
            }
        }

        if (path.empty())
        {
            path = GetSeedPath(assetId, platformFlags);
        }
        m_assetSeedList.emplace_back(AzFramework::SeedInfo(assetId, platformFlags, path, seedListFilePath));
        return true;
    }

    void AssetSeedManager::RemoveSeedAsset(AZ::Data::AssetId assetId, AzFramework::PlatformFlags platformFlags)
    {
        for (auto iter = m_assetSeedList.begin(); iter != m_assetSeedList.end(); ++iter)
        {
            if (iter->m_assetId == assetId)
            {
                if (iter->m_platformFlags == platformFlags)
                {
                    m_assetSeedList.erase(iter);
                }
                else
                {
                    iter->m_platformFlags = iter->m_platformFlags  & (~platformFlags);
                }
                return;
            }
        }

        AZ_TracePrintf("AssetSeedManager", "Seed Asset ( %s ) is not present in the asset seed list.\n", assetId.ToString<AZStd::string>().c_str());
    }

    void AssetSeedManager::RemoveSeedAsset(const AZStd::string& assetKey, AzFramework::PlatformFlags platformFlags)
    {
        bool validAssetId = false;
        AZStd::string assetPathHint;
        AZ::Data::AssetId assetId = GetAssetIdFromString(assetKey);
        if (assetId.IsValid())
        {
            validAssetId = true;
        }
        else
        {
            AZStd::string normalizedAssetKey = assetKey;
            AZ::StringFunc::Path::Normalize(normalizedAssetKey);
            if (AZ::StringFunc::Path::IsValid(normalizedAssetKey.c_str()))
            {
                assetPathHint = assetKey;
                AZ::StringFunc::Path::Normalize(assetPathHint);
            }
        }

        if (assetPathHint.empty() && !validAssetId)
        {
            AZ_Warning("AssetSeedManager", false, "Invalid asset key ( %s ). It is neither a valid assetId nor a valid relative path.\n", assetKey.c_str());
        }

        bool assetFound = false;

        for (auto iter = m_assetSeedList.begin(); iter != m_assetSeedList.end(); ++iter)
        {
            if (validAssetId && iter->m_assetId == assetId)
            {
                iter->m_platformFlags = iter->m_platformFlags & (~platformFlags);
                if (iter->m_platformFlags == AzFramework::PlatformFlags::Platform_NONE)
                {
                    m_assetSeedList.erase(iter);
                }
                assetFound = true;
                break;
            }
            else if(!assetPathHint.empty())
            {
                // if we are here it implies that we need to search based on path hint
                AZ::StringFunc::Path::Normalize(iter->m_assetRelativePath);

                if (iter->m_assetRelativePath == assetPathHint)
                {
                    iter->m_platformFlags = iter->m_platformFlags & (~platformFlags);
                    if (iter->m_platformFlags == AzFramework::PlatformFlags::Platform_NONE)
                    {
                        m_assetSeedList.erase(iter);
                    }
                    assetFound = true;
                    break;
                }
            }
        }

        if (!assetFound)
        {
            AZ_Warning("AssetSeedManager", false, "Unable to remove asset ( %s ). Please ensure that this asset exists in the seed list file(s).\n", assetKey.c_str());
        }
    }

    bool AssetSeedManager::AddSeedAsset(const AZStd::string& assetPath, AzFramework::PlatformFlags platformFlags, const AZStd::string& seedListFilePath)
    {
        AZStd::string seedPath = AddAssetToSeedListHelper(assetPath);
        if (seedPath.empty())
        {
            // Error has already been thrown
            return false;
        }

        AZ::Data::AssetId assetId = GetAssetIdByPath(seedPath, platformFlags);
        if (assetId.IsValid())
        {
            return AddSeedAsset(assetId, platformFlags, seedPath, seedListFilePath);
        }

        AZ_Warning("AssetSeedManager", false, "Unable to add asset ( %s ) to the seed list, could not find it on all requested platforms.\n", seedPath.c_str());
        return false;
    }

    AZStd::pair<AZ::Data::AssetId, AzFramework::PlatformFlags> AssetSeedManager::AddSeedAssetForValidPlatforms(const AZStd::string& assetPath, AzFramework::PlatformFlags platformFlags)
    {
        using namespace AzFramework;

        AZStd::pair<AZ::Data::AssetId, AzFramework::PlatformFlags> result = AZStd::make_pair(AZ::Data::AssetId(), AzFramework::PlatformFlags::Platform_NONE);

        if (platformFlags == AzFramework::PlatformFlags::Platform_NONE)
        {
            AZ_Error("AssetSeedManager", false, "Unable to add asset ( %s ) to the seed list, no platforms were defined.\n", assetPath.c_str());
            return result;
        }

        AZStd::string seedPath = AddAssetToSeedListHelper(assetPath);
        if (seedPath.empty())
        {
            // Error has already been thrown
            return result;
        }

        // Try to add it one platform at a time, that way if any of them fail, the Seed is still added
        //    to the list for the successes
        for (AzFramework::PlatformId platformId : AzFramework::PlatformHelper::GetPlatformIndicesInterpreted(platformFlags))
        {
            PlatformFlags singlePlatformFlag = AzFramework::PlatformHelper::GetPlatformFlagFromPlatformIndex(platformId);

            auto tempId = GetAssetIdByPath(seedPath, singlePlatformFlag);
            if (!tempId.IsValid())
            {
                // Try another platform
                continue;
            }

            result.first = tempId;

            if (AddSeedAsset(tempId, singlePlatformFlag))
            {
                result.second = result.second | singlePlatformFlag;
            }
        }

        return result;
    }

    void AssetSeedManager::AddPlatformToAllSeeds(AzFramework::PlatformId platform)
    {
        using namespace AzToolsFramework::AssetCatalog;

        AzFramework::PlatformFlags platformFlag = AzFramework::PlatformHelper::GetPlatformFlagFromPlatformIndex(platform);
        AZ::Data::AssetInfo assetInfo;
        bool isSpecialPlatform = AzFramework::PlatformHelper::IsSpecialPlatform(platformFlag);

        for (auto& seed : m_assetSeedList)
        {
            if (isSpecialPlatform)
            {
                seed.m_platformFlags |= platformFlag;
            }
            else
            {
                assetInfo = GetAssetInfoById(seed.m_assetId, platform, seed.m_seedListFilePath, seed.m_assetRelativePath);

                if (assetInfo.m_assetId.IsValid())
                {
                    seed.m_platformFlags |= platformFlag;
                }
            }
        }
    }

    void AssetSeedManager::RemovePlatformFromAllSeeds(AzFramework::PlatformId platform)
    {
        using namespace AzToolsFramework::AssetCatalog;
        AzFramework::PlatformFlags platformFlag = AzFramework::PlatformHelper::GetPlatformFlagFromPlatformIndex(platform);

        for (auto& seed : m_assetSeedList)
        {
            if ((seed.m_platformFlags & (~platformFlag)) == AzFramework::PlatformFlags::Platform_NONE)
            {
                AZ_Warning("AssetSeedManager", false, "Cannot remove platform ( %s ) from Seed ( %s ): Seed only has one platform", AzFramework::PlatformHelper::GetPlatformName(platform), seed.m_assetId.ToString<AZStd::string>().c_str());
            }
            else
            {
                seed.m_platformFlags &= (~platformFlag);
            }
        }
    }

    AZ::Data::AssetId AssetSeedManager::FindAssetIdByPathHint(const AZStd::string& pathHint) const
    {
        for (auto iter = m_assetSeedList.begin(); iter != m_assetSeedList.end(); ++iter)
        {
            if (AZ::StringFunc::Equal(iter->m_assetRelativePath.c_str(), pathHint.c_str()))
            {
                return iter->m_assetId;
            }
        }
        return AZ::Data::AssetId();
    }

    // Returns the AssetId if it was valid for all the platforms in platformFlags
    AZ::Data::AssetId AssetSeedManager::GetAssetIdByPath(const AZStd::string& assetPath, const AzFramework::PlatformFlags& platformFlags) const
    {
        using namespace AzToolsFramework::AssetCatalog;
        AZ::Data::AssetId assetId;
        auto platformIndices = AzFramework::PlatformHelper::GetPlatformIndicesInterpreted(platformFlags);
        bool foundInvalid = false;

        for (const auto& platformNum : platformIndices)
        {
            AZ::Data::AssetId foundAssetId;
            PlatformAddressedAssetCatalogRequestBus::EventResult(foundAssetId, platformNum, &PlatformAddressedAssetCatalogRequestBus::Events::GetAssetIdByPath, assetPath.c_str(), AZ::Data::s_invalidAssetType, false);
            if (!foundAssetId.IsValid())
            {
                AZ_Warning("AssetSeedManager", false, "Asset catalog does not know about the asset ( %s ) on platform ( %s ).", assetPath.c_str(), AzFramework::PlatformHelper::GetPlatformName(platformNum));
                foundInvalid = true;
            }
            else
            {
                assetId = foundAssetId;
            }
        }
        if (foundInvalid)
        {
            return AZ::Data::AssetId();
        }
        return assetId;
    }

    AZ::Data::AssetId AssetSeedManager::GetAssetIdByAssetKey(const AZStd::string& assetKey, const AzFramework::PlatformFlags& platformFlags) const
    {
        AZ::Data::AssetId assetId = GetAssetIdFromString(assetKey);
        if (assetId.IsValid())
        {
            return assetId;
        }

        // if we are here we will first try to query the asset catalog about the asset,
        // if we are unable to find the asset in the catalog than we will try to search it based on path hints.
        assetId = GetAssetIdByPath(assetKey, platformFlags);

        if (!assetId.IsValid())
        {
            assetId = FindAssetIdByPathHint(assetKey);
        }

        return assetId;
    }

    // Returns the asset info if it exists and is the same for the platform specified by platformIndex
    AZ::Data::AssetInfo AssetSeedManager::GetAssetInfoById(const AZ::Data::AssetId& assetId, const AzFramework::PlatformId& platformIndex, const AZStd::string& seedListfilePath, const AZStd::string& assetHintPath)
    {
        using namespace AzToolsFramework::AssetCatalog;
        AZ::Data::AssetInfo assetInfo;
        PlatformAddressedAssetCatalogRequestBus::EventResult(assetInfo, static_cast<AzFramework::PlatformId>(platformIndex), &PlatformAddressedAssetCatalogRequestBus::Events::GetAssetInfoById, assetId);
        if (!assetInfo.m_assetId.IsValid())
        {
            AZStd::string errorMessage = AZStd::string::format("Could not find asset with id (%s) on platform (%s)", assetId.ToString<AZStd::string>().c_str(), AzFramework::PlatformHelper::GetPlatformName(platformIndex));

            if (!seedListfilePath.empty() || !assetHintPath.empty())
            {
                errorMessage = AZStd::string::format("%s from Seed List (%s) Asset Hint (%s)", errorMessage.c_str(), seedListfilePath.c_str(), assetHintPath.c_str());
            }

            AZ_Error("AssetSeedManager", false, errorMessage.c_str());
        }
        return assetInfo;
    }

    AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string> AssetSeedManager::GetAllProductDependencies(
        const AZ::Data::AssetId& assetId,
        const AzFramework::PlatformId& platformIndex,
        const AZStd::unordered_set<AZ::Data::AssetId>& exclusionList,
        AssetFileDebugInfoList* optionalDebugList,
        AZStd::unordered_set<AZ::Data::AssetId>* cyclicalDependencySet,
        const AZStd::vector<AZStd::string>& wildcardPatternExclusionList) const
    {
        using namespace AzToolsFramework::AssetCatalog;

        if (optionalDebugList)
        {
            return AssetFileDebugInfoList::GetAllProductDependenciesDebug(assetId, platformIndex, optionalDebugList, cyclicalDependencySet, exclusionList, wildcardPatternExclusionList);
        }

        // If not gathering debug info, then the recursion can happen in SQL. Call GetAllProductDependencies for the faster method of gathering.
        AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string> getDependenciesResult = AZ::Failure(AZStd::string());
        PlatformAddressedAssetCatalogRequestBus::EventResult(getDependenciesResult, platformIndex, &PlatformAddressedAssetCatalogRequestBus::Events::GetAllProductDependenciesFilter, assetId, exclusionList, wildcardPatternExclusionList);
        return getDependenciesResult;
    }

    AZStd::string AssetSeedManager::AddAssetToSeedListHelper(const AZStd::string& assetPath)
    {
        if (!m_sourceAssetTypeToRuntimeAssetTypeMap.size())
        {
            PopulateAssetTypeMap();
        }

        using namespace AzFramework::FileTag;
        AZStd::vector<AZStd::string> editorTagsList = { FileTags[static_cast<unsigned int>(FileTagsIndex::EditorOnly)] };
        bool editorOnlyAsset = false;
        QueryFileTagsEventBus::EventResult(editorOnlyAsset, FileTagType::Exclude,
            &QueryFileTagsEventBus::Events::Match, assetPath, editorTagsList);

        AZStd::string seedPath = assetPath;

        if (editorOnlyAsset)
        {
            // If we are here it implies that this is an editor only asset type.
            // Please note that in those cases where we are certain about what the runtime asset type should be for this
            // source asset type, we will fix the fileextension.

            AZStd::string fileExtension;
            AZ::StringFunc::Path::GetExtension(seedPath.c_str(), fileExtension, false);
            auto found = m_sourceAssetTypeToRuntimeAssetTypeMap.find(fileExtension);

            if (found != m_sourceAssetTypeToRuntimeAssetTypeMap.end())
            {
                AZ::StringFunc::Path::ReplaceExtension(seedPath, found->second.c_str());
                AZ_Warning("AssetSeedManager", false, "( %s ) is an editor only asset. We wil use seed asset( %s ) instead.\n", assetPath.c_str(), seedPath.c_str());
            }
            else
            {
                AZ_Warning("AssetSeedManager", false, "Invalid seed asset ( %s ). This is an editor only asset. Please note that you can open the asset processor database and find \
                    all assets associated with this source asset and add those products instead. \n", seedPath.c_str());
                return {};
            }
        }

        return seedPath;
    }

    void AssetSeedManager::PopulateAssetTypeMap()
    {
        AZStd::string sliceFileExtension;
        AZ::StringFunc::Path::GetExtension(AZ::SliceAsset::GetFileFilter(), sliceFileExtension, false);

        AZStd::string dynamicSliceFileExtension;
        AZ::StringFunc::Path::GetExtension(AZ::DynamicSliceAsset::GetFileFilter(), dynamicSliceFileExtension, false);

        m_sourceAssetTypeToRuntimeAssetTypeMap[sliceFileExtension] = dynamicSliceFileExtension;
        m_sourceAssetTypeToRuntimeAssetTypeMap[ScriptCanvas] = ScriptCanvasCompiled;
        m_sourceAssetTypeToRuntimeAssetTypeMap[ScriptCanvasFunction] = ScriptCanvasFunctionCompiled;
    }

    const AzFramework::AssetSeedList& AssetSeedManager::GetAssetSeedList() const
    {
        return m_assetSeedList;
    }

    AZ::Outcome<void, AZStd::string> AssetSeedManager::SetSeedPlatformFlags(int index, AzFramework::PlatformFlags platformFlags)
    {
        if (index >= m_assetSeedList.size() || index < 0)
        {
            return AZ::Failure(AZStd::string("Index is out of range"));
        }

        m_assetSeedList[index].m_platformFlags = platformFlags;
        return AZ::Success();
    }

    AssetSeedManager::AssetsInfoList AssetSeedManager::GetDependenciesInfo(AzFramework::PlatformId platformIndex, const AZStd::unordered_set<AZ::Data::AssetId>& exclusionList, AssetFileDebugInfoList* optionalDebugList, const AZStd::vector<AZStd::string>& wildcardPatternExclusionList) const
    {
        if (!m_assetSeedList.size())
        {
            AZ_TracePrintf("AssetSeedManager", "Asset Seed list is empty.\n");
            return {};
        }

        if (!AzToolsFramework::PlatformAddressedAssetCatalog::CatalogExists(platformIndex))
        {
            // There's no catalog loaded for this platform, so there won't be any assets
            return {};
        }

        AssetSeedManager::AssetsInfoList assetsInfoList;
        AZStd::unordered_set<AZ::Data::AssetId> assetIdSet;
        AZStd::unordered_set<AZ::Data::AssetId> cyclicalDependencySet;
        for (int idx = 0; idx < m_assetSeedList.size(); idx++)
        {
            if(!AzFramework::PlatformHelper::HasPlatformFlag(m_assetSeedList[idx].m_platformFlags,platformIndex))
            {
                // This asset is not valid for the platformIndex
                continue;
            }

            AZ::Data::AssetInfo seedAssetInfo = GetAssetInfoById(m_assetSeedList[idx].m_assetId, platformIndex, m_assetSeedList[idx].m_seedListFilePath, m_assetSeedList[idx].m_assetRelativePath);

            if (optionalDebugList &&
                optionalDebugList->m_fileDebugInfoList.find(seedAssetInfo.m_assetId) == optionalDebugList->m_fileDebugInfoList.end())
            {
                optionalDebugList->m_fileDebugInfoList[seedAssetInfo.m_assetId].m_assetId = seedAssetInfo.m_assetId;
            }

            if (assetIdSet.find(seedAssetInfo.m_assetId) != assetIdSet.end())
            {
                // do not want duplicate enteries in the assets info list
                continue;
            }

            if(exclusionList.find(seedAssetInfo.m_assetId) != exclusionList.end())
            {
                continue;
            }

            bool wildcardPatternMatch = false;
            for (const AZStd::string& wildcardPattern : wildcardPatternExclusionList)
            {
                AzToolsFramework::AssetCatalog::PlatformAddressedAssetCatalogRequestBus::EventResult(wildcardPatternMatch, platformIndex, &AzToolsFramework::AssetCatalog::PlatformAddressedAssetCatalogRequestBus::Events::DoesAssetIdMatchWildcardPattern, seedAssetInfo.m_assetId, wildcardPattern);
                if (wildcardPatternMatch)
                {
                    break;
                }
            }
            if (wildcardPatternMatch)
            {
                continue;
            }

            assetsInfoList.emplace_back(AZStd::move(seedAssetInfo));
            assetIdSet.insert(seedAssetInfo.m_assetId);

            cyclicalDependencySet.clear();
            AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string> getDependenciesResult =
                GetAllProductDependencies(m_assetSeedList[idx].m_assetId, platformIndex, exclusionList, optionalDebugList, &cyclicalDependencySet, wildcardPatternExclusionList);
            if (getDependenciesResult.IsSuccess())
            {
                AZStd::vector<AZ::Data::ProductDependency> entries = getDependenciesResult.TakeValue();

                for (const AZ::Data::ProductDependency& productDependency : entries)
                {
                    if (productDependency.m_assetId.IsValid() && assetIdSet.find(productDependency.m_assetId) == assetIdSet.end())
                    {
                        assetIdSet.insert(productDependency.m_assetId);

                        AZ::Data::AssetInfo assetInfo = GetAssetInfoById(productDependency.m_assetId, platformIndex, m_assetSeedList[idx].m_seedListFilePath);
                        assetsInfoList.emplace_back(AZStd::move(assetInfo));
                    }
                }
            }
            else
            {
                AZ_Error("AssetSeedManager", false, "Unable to retrieve all product dependencies for asset ( %s ) with asset id ( %s ).\n", seedAssetInfo.m_relativePath.c_str(), m_assetSeedList[idx].m_assetId.ToString<AZStd::string>().c_str());
            }
        }

        return assetsInfoList;
    }

    AssetFileInfoList AssetSeedManager::GetDependencyList(AzFramework::PlatformId platformIndex, const AZStd::unordered_set<AZ::Data::AssetId>& exclusionList, AssetFileDebugInfoList* optionalDebugList, const AZStd::vector<AZStd::string>& wildcardPatternExclusionList) const
    {
        AssetSeedManager::AssetsInfoList  assetInfoList = AZStd::move(GetDependenciesInfo(platformIndex, exclusionList, optionalDebugList, wildcardPatternExclusionList));

        AssetFileInfoList assetFileInfoList;
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        AZ_Assert(fileIO != nullptr, "AZ::IO::FileIOBase must be ready for use.\n");
        AZStd::string assetRoot = PlatformAddressedAssetCatalog::GetAssetRootForPlatform(platformIndex);

        for (const AZ::Data::AssetInfo& assetInfo : assetInfoList)
        {
            if (assetInfo.m_assetId.IsValid())
            {
                if (!assetInfo.m_relativePath.empty())
                {
                    AZStd::string assetPath;
                    AZ::StringFunc::Path::Join(assetRoot.c_str(), assetInfo.m_relativePath.c_str(), assetPath);
                    if (!fileIO->Exists(assetPath.c_str()))
                    {
                        AZ_Warning("AssetSeedManager", false, "Asset ( %s ) does not exist in the cache folder.\n", assetPath.c_str());
                        continue;
                    }

                    AZ::IO::FileIOStream fileStream;
                    if (!fileStream.Open(assetPath.c_str(), AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary))
                    {
                        AZ_Warning("AssetSeedManager", false, "Failed to open asset ( %s ).\n", assetPath.c_str());
                        continue;
                    }

                    AZ::IO::SizeType length = fileStream.GetLength();
                    AZ::Sha1 hash;
                    AZStd::array<AZ::u32, AssetFileInfo::s_arraySize> digest = { {0} };
                    AZ::u32 digestArray[AssetFileInfo::s_arraySize] = { 0 };
                    // If there's no length, there's no data to hash.
                    // It's valid to have 0 length files, these can be used as markers for the file system.
                    if (length)
                    {
                        AZStd::vector<uint8_t> buffer;
                        buffer.resize_no_construct(length);

                        if (fileStream.Read(length, buffer.data()) != length)
                        {
                            AZ_Warning("AssetSeedManager", false, "Failed to read entire asset file ( %s ).\n", assetPath.c_str());
                            continue;
                        }

                        hash.ProcessBytes(AZStd::as_bytes(AZStd::span(buffer)));
                        hash.GetDigest(digestArray);

                        for (int idx = 0; idx < digest.size(); idx++)
                        {
                            digest[idx] = digestArray[idx];
                        }
                    }

                    uint64_t modTime = fileIO->ModificationTime(assetInfo.m_relativePath.c_str());

                    AssetFileInfo assetFileInfo(assetInfo.m_assetId, assetInfo.m_relativePath, modTime, digest);

                    if (optionalDebugList)
                    {
                        optionalDebugList->m_fileDebugInfoList[assetInfo.m_assetId].m_assetRelativePath = assetInfo.m_relativePath;
                        optionalDebugList->m_fileDebugInfoList[assetInfo.m_assetId].m_fileSize = length;
                    }

                    assetFileInfoList.m_fileInfoList.push_back(assetFileInfo);
                }
                else
                {
                    AZ_Warning("AssetSeedManager", false, "Asset with asset id ( %s ) is missing relative path information in the asset catalog.\n", assetInfo.m_assetId.ToString<AZStd::string>().c_str());
                }
            }
        }

        return assetFileInfoList;
    }

    bool AssetSeedManager::SaveAssetFileInfo(const AZStd::string& destinationFilePath, AzFramework::PlatformFlags platformFlags, const AZStd::unordered_set<AZ::Data::AssetId>& exclusionList, const AZStd::string& debugFilePath, const AZStd::vector<AZStd::string>& wildcardPatternExclusionList)
    {
        auto platformIndices = AzFramework::PlatformHelper::GetPlatformIndicesInterpreted(platformFlags);
        if (platformIndices.size() != 1)
        {
            AZ_Warning("AssetSeedManager", false, "AssetSeedManager::SaveAssetFileInfo can only operate on one platform at a time.\n");
            return false;
        }

        bool useDebugInfoList = !debugFilePath.empty();
        AssetFileDebugInfoList debugInfo;
        AssetFileInfoList assetFileInfoList = GetDependencyList(platformIndices[0], exclusionList, useDebugInfoList ? &debugInfo : nullptr, wildcardPatternExclusionList);

        if (!AssetFileInfoList::Save(assetFileInfoList, destinationFilePath))
        {
            // Error has already been thrown
            return false;
        }

        if (useDebugInfoList)
        {
            debugInfo.BuildHumanReadableString();
            return AZ::Utils::SaveObjectToFile(debugFilePath, AZ::DataStream::StreamType::ST_XML, &debugInfo);
        }

        return true;
    }

    AZ::Outcome<AssetFileInfoList, AZStd::string> AssetSeedManager::LoadAssetFileInfo(const AZStd::string& assetListFileAbsolutePath)
    {
        auto fileExtensionOutcome = AssetFileInfoList::ValidateAssetListFileExtension(assetListFileAbsolutePath);
        if (!fileExtensionOutcome.IsSuccess())
        {
            return AZ::Failure(fileExtensionOutcome.GetError());
        }

        if (!AZ::IO::FileIOBase::GetInstance()->Exists(assetListFileAbsolutePath.c_str()))
        {
            return AZ::Failure(AZStd::string::format("Unable to load Asset List file ( %s ): file does not exist.", assetListFileAbsolutePath.c_str()));
        }

        AssetFileInfoList assetFileInfoList;
        if (!AZ::Utils::LoadObjectFromFileInPlace(assetListFileAbsolutePath.c_str(), assetFileInfoList))
        {
            return AZ::Failure(AZStd::string::format("Unable to load Asset List file ( %s ).", assetListFileAbsolutePath.c_str()));
        }

        return AZ::Success(assetFileInfoList);
    }

    const char* AssetSeedManager::GetSeedFileExtension()
    {
        return SeedFileExtension;
    }

    AZ::Outcome<void, AZStd::string> AssetSeedManager::ValidateSeedFileExtension(const AZStd::string& path)
    {
        if (!AZ::StringFunc::EndsWith(path, SeedFileExtension))
        {
            return AZ::Failure(AZStd::string::format(
                "Invalid Seed List file path ( %s ). Invalid file extension, Seed List files can only have ( .%s ) extension.\n",
                path.c_str(),
                SeedFileExtension));
        }

        return AZ::Success();
    }

    const char* AssetSeedManager::GetAssetListFileExtension()
    {
        return AssetListFileExtension;
    }

    const AZStd::string& AssetSeedManager::GetReadablePlatformList(const AzFramework::SeedInfo& seed)
    {
        using namespace AzFramework;

        PlatformFlags visiblePlatforms = seed.m_platformFlags;
#ifndef AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS
        // don't include restricted platforms when they are not enabled
        visiblePlatforms &= PlatformFlags::UnrestrictedPlatforms;
#endif
        auto readablePlatformListIter = m_platformFlagsToReadablePlatformList.find(visiblePlatforms);
        if (readablePlatformListIter != m_platformFlagsToReadablePlatformList.end())
        {
            return readablePlatformListIter->second;
        }

        m_platformFlagsToReadablePlatformList[visiblePlatforms] = PlatformHelper::GetCommaSeparatedPlatformList(visiblePlatforms);
        return m_platformFlagsToReadablePlatformList.at(visiblePlatforms);
    }

    void AssetSeedManager::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AssetSeedManager>()
                ->Version(1)
                ->Field("assetSeedList", &AssetSeedManager::m_assetSeedList);

            AssetFileInfo::Reflect(serializeContext);
            AssetFileInfoList::Reflect(serializeContext);
            AssetFileDebugInfo::Reflect(serializeContext);
            AssetFileDebugInfoList::Reflect(serializeContext);
        }
    }

    bool AssetSeedManager::Save(const AZStd::string& destinationPath)
    {
        auto fileExtensionOutcome = ValidateSeedFileExtension(destinationPath);
        if (!fileExtensionOutcome.IsSuccess())
        {
            AZ_Error("AssetSeedManager", false, fileExtensionOutcome.GetError().c_str());
            return false;
        }

        if (AZ::IO::FileIOBase::GetInstance()->Exists(destinationPath.c_str()) && AZ::IO::FileIOBase::GetInstance()->IsReadOnly(destinationPath.c_str()))
        {
            AZ_Error("AssetSeedManager", false, "Unable to save seed file (%s): file is marked Read-Only.\n", destinationPath.c_str());
            return false;
        }

        return static_cast<bool>(AZ::JsonSerializationUtils::SaveObjectToFile(&m_assetSeedList, destinationPath));
    }

    void AssetSeedManager::UpdateSeedPath()
    {
        for (AzFramework::SeedInfo& seedInfo : m_assetSeedList)
        {
            AZStd::string assetPath = GetSeedPath(seedInfo.m_assetId, seedInfo.m_platformFlags);
            if (!assetPath.empty())
            {
                seedInfo.m_assetRelativePath = assetPath;
            }
        }
    }

    void AssetSeedManager::RemoveSeedPath()
    {
        for (AzFramework::SeedInfo& seedInfo : m_assetSeedList)
        {
            seedInfo.m_assetRelativePath.clear();
        }
    }

    bool AssetSeedManager::Load(const AZStd::string& sourceFilePath)
    {
        auto fileExtensionOutcome = ValidateSeedFileExtension(sourceFilePath);
        if (!fileExtensionOutcome.IsSuccess())
        {
            AZ_Error("AssetSeedManager", false, fileExtensionOutcome.GetError().c_str());
            return false;
        }

        AzFramework::AssetSeedList assetSeedList;

        // As the seed file can support both JSON and ObjectStream XML
        // It is opened and read into memory first
        AZ::IO::FileIOStream assetSeedStream;
        if (!assetSeedStream.Open(sourceFilePath.c_str(), AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary))
        {
            return false;
        }

        auto ReadAssetSeedData = [&assetSeedStream](char* buffer, size_t size) -> size_t
        {
            return assetSeedStream.Read(size, buffer);
        };

        AZStd::string assetSeedData;
        assetSeedData.resize_and_overwrite(assetSeedStream.GetLength(), ReadAssetSeedData);

        // If the asset seed file starts with the ObjectStream XML stream tag
        // then load the data using the ObjectStream
        constexpr AZ::u8 xmlObjectStreamTag = '<';
        if (assetSeedData.starts_with(xmlObjectStreamTag))
        {
            if (!AZ::Utils::LoadObjectFromBufferInPlace(assetSeedData.c_str(), assetSeedData.size(), assetSeedList))
            {
                return false;
            }
        }
        else
        {
            // Load the asset seed file using Json Serialization
            if (!AZ::JsonSerializationUtils::LoadObjectFromString(assetSeedList, assetSeedData))
            {
                return false;
            }
        }

        for (AzFramework::SeedInfo& seedInfo : assetSeedList)
        {
            AddSeedAsset(seedInfo.m_assetId, seedInfo.m_platformFlags, seedInfo.m_assetRelativePath, sourceFilePath.c_str());
        }

        return true;
    }

    AssetFileInfo::AssetFileInfo(AZ::Data::AssetId assetId, AZStd::string assetRelativePath, uint64_t modTime, AZStd::array<AZ::u32, s_arraySize> hash)
        : m_assetId(assetId)
        , m_modificationTime(modTime)
        , m_assetRelativePath(AZStd::move(assetRelativePath))
        , m_hash(AZStd::move(hash))
    {
    }

    void AssetFileInfo::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AssetFileInfo>()
                ->Version(1)
                ->Field("assetId", &AssetFileInfo::m_assetId)
                ->Field("assetRelativePath", &AssetFileInfo::m_assetRelativePath)
                ->Field("modificationTime", &AssetFileInfo::m_modificationTime)
                ->Field("hash", &AssetFileInfo::m_hash);
        }
    }

    void AssetFileInfoList::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AssetFileInfoList>()
                ->Version(1)
                ->Field("fileInfoList", &AssetFileInfoList::m_fileInfoList);
        }
    }

    bool AssetFileInfoList::Save(const AssetFileInfoList& assetFileInfoList, const AZStd::string& destinationFilePath)
    {
        if (assetFileInfoList.m_fileInfoList.empty())
        {
            // Don't save an empty list
            AZ_Warning("AssetFileInfoList", false, "Unable to save Asset List file (%s): list is empty.\n", destinationFilePath.c_str());
            return false;
        }

        auto fileExtensionOutcome = ValidateAssetListFileExtension(destinationFilePath);
        if (!fileExtensionOutcome.IsSuccess())
        {
            AZ_Error("AssetFileInfoList", false, fileExtensionOutcome.GetError().c_str());
            return false;
        }

        if (AZ::IO::FileIOBase::GetInstance()->Exists(destinationFilePath.c_str()) && AZ::IO::FileIOBase::GetInstance()->IsReadOnly(destinationFilePath.c_str()))
        {
            AZ_Error("AssetFileInfoList", false, "Unable to save Asset List file (%s): file is marked Read-Only.\n", destinationFilePath.c_str());
            return false;
        }

        return AZ::Utils::SaveObjectToFile(destinationFilePath, AZ::DataStream::StreamType::ST_XML, &assetFileInfoList);
    }

    AZ::Outcome<void, AZStd::string> AssetFileInfoList::ValidateAssetListFileExtension(const AZStd::string& path)
    {
        if (!AZ::StringFunc::EndsWith(path, AssetListFileExtension))
        {
            return AZ::Failure(AZStd::string::format(
                "Invalid Asset List file path ( %s ). Invalid file extension, Asset List files can only have ( .%s ) extension.\n",
                path.c_str(),
                AssetListFileExtension));
        }

        return AZ::Success();
    }

} // namespace AzToolsFramework
