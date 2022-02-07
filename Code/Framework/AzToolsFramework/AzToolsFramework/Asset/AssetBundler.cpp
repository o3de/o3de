/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Asset/AssetBundler.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/std/hash.h>
#include <AzCore/std/string/wildcard.h>
#include <AzCore/std/string/regex.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/IO/FileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace
{
    //! These struct are used because we have a use case where we only need to hash and
    //! override the equals to method for AssetFileInfo class based only on AssetId, which might not be true for other usages of AssetFileInfo class.
    struct AssetFileInfoComparatorByAssetId
    {
        bool operator()(const AzToolsFramework::AssetFileInfo& left, const AzToolsFramework::AssetFileInfo& right) const
        {
            return left.m_assetId == right.m_assetId;
        }
    };

    struct AssetFileInfoHasherByAssetId
    {
        size_t operator() (const AzToolsFramework::AssetFileInfo& assetFileInfo) const
        {
            size_t hashValue = 0;
            AZStd::hash_combine(hashValue, assetFileInfo.m_assetId);
            return hashValue;
        }
    };
}
namespace AzToolsFramework
{
    const char AssetBundleSettingsFileExtension[] = "bundlesettings";
    const char BundleFileExtension[] = "pak";
    const char ComparisonRulesFileExtension[] = "rules";
    [[maybe_unused]] const char ErrorWindowName[] = "AssetBundler";
    const char* AssetFileInfoListComparison::ComparisonTypeNames[] = { "delta", "union", "intersection", "complement", "filepattern", "intersectioncount" };
    const char* AssetFileInfoListComparison::FilePatternTypeNames[] = { "wildcard", "regex" };
    const char DefaultTypeName[] = "default";
    const char TokenIdentifier = '$';

    void AssetBundleSettings::Reflect(AZ::ReflectContext * context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AssetBundleSettings>()
                ->Version(3)
                ->Field("AssetFileInfoListPath", &AssetBundleSettings::m_assetFileInfoListPath)
                ->Field("BundleFilePath", &AssetBundleSettings::m_bundleFilePath)
                ->Field("BundleVersion", &AssetBundleSettings::m_bundleVersion)
                ->Field("maxBundleSize", &AssetBundleSettings::m_maxBundleSizeInMB)
                ->Field("comment", &AssetBundleSettings::m_comment);
        }
    }

    AZ::Outcome<AssetBundleSettings, AZStd::string> AssetBundleSettings::Load(const AZStd::string& filePath)
    {
        auto fileExtensionOutcome = ValidateBundleSettingsFileExtension(filePath);
        if (!fileExtensionOutcome.IsSuccess())
        {
            return AZ::Failure(fileExtensionOutcome.GetError());
        }

        AssetBundleSettings assetBundleSettings;

        if (!AZ::Utils::LoadObjectFromFileInPlace(filePath.c_str(), assetBundleSettings))
        {
            return AZ::Failure(AZStd::string::format("Failed to load AssetBundleSettings file (%s) from disk.\n", filePath.c_str()));
        }
        
        assetBundleSettings.m_platform = GetPlatformFromAssetInfoFilePath(assetBundleSettings);


        return AZ::Success(assetBundleSettings);
    }

    bool AssetBundleSettings::Save(const AssetBundleSettings& assetBundleSettings, const AZStd::string& destinationFilePath)
    {
        auto fileExtensionOutcome = ValidateBundleSettingsFileExtension(destinationFilePath);
        if (!fileExtensionOutcome.IsSuccess())
        {
            AZ_Error(ErrorWindowName, false, fileExtensionOutcome.GetError().c_str());
            return false;
        }

        if (AZ::IO::FileIOBase::GetInstance()->Exists(destinationFilePath.c_str()) && AZ::IO::FileIOBase::GetInstance()->IsReadOnly(destinationFilePath.c_str()))
        {
            AZ_Error(ErrorWindowName, false, "Unable to save bundle settings file (%s): file is marked Read-Only.\n", destinationFilePath.c_str());
            return false;
        }

        if (!IsBundleSettingsFile(destinationFilePath))
        {
            AZ_Error(ErrorWindowName, false, "Failed to save file (%s) to disk. AssetBundleSettings files must have the extension: %s\n", destinationFilePath.c_str(), AssetBundleSettingsFileExtension);
            return false;
        }

        if (AZ::Utils::SaveObjectToFile(destinationFilePath.c_str(), AZ::DataStream::StreamType::ST_XML, &assetBundleSettings))
        {
            return true;
        }

        AZ_Error(ErrorWindowName, false, "Failed to save file (%s) to disk.", destinationFilePath.c_str());
        return false;
    }


    bool AssetBundleSettings::IsBundleSettingsFile(const AZStd::string& filePath)
    {
        return AzFramework::StringFunc::EndsWith(filePath, AssetBundleSettingsFileExtension);
    }

    const char* AssetBundleSettings::GetBundleSettingsFileExtension()
    {
        return AssetBundleSettingsFileExtension;
    }

    AZ::Outcome<void, AZStd::string> AssetBundleSettings::ValidateBundleSettingsFileExtension(const AZStd::string& path)
    {
        if (!AzFramework::StringFunc::EndsWith(path, AssetBundleSettingsFileExtension))
        {
            return AZ::Failure(AZStd::string::format(
                "Invalid Bundle Settings file path ( %s ). Invalid file extension, Bundle Settings files can only have ( .%s ) extension.\n",
                path.c_str(),
                AssetBundleSettingsFileExtension));
        }

        return AZ::Success();
    }

    const char* AssetBundleSettings::GetBundleFileExtension()
    {
        return BundleFileExtension;
    }

    AZ::Outcome<void, AZStd::string> AssetBundleSettings::ValidateBundleFileExtension(const AZStd::string& path)
    {
        if (!AzFramework::StringFunc::EndsWith(path, BundleFileExtension))
        {
            return AZ::Failure(AZStd::string::format(
                "Invalid Bundle file path ( %s ). Invalid file extension, Bundles can only have ( .%s ) extension.\n",
                path.c_str(),
                BundleFileExtension));
        }

        return AZ::Success();
    }

    AZ::u64 AssetBundleSettings::GetMaxBundleSizeInMB()
    {
        return MaxBundleSizeInMB;
    }

    AZStd::string AssetBundleSettings::GetPlatformFromAssetInfoFilePath(const AssetBundleSettings& assetBundleSettings)
    {
        return GetPlatformIdentifier(assetBundleSettings.m_assetFileInfoListPath);
    }

    bool AssetFileInfoListComparison::IsOutputPath(const AZStd::string& filePath)
    {
        return !filePath.empty() && !IsTokenFile(filePath);
    }

    bool AssetFileInfoListComparison::IsTokenFile(const AZStd::string& filePath)
    {
        return !filePath.empty() && filePath[0] == TokenIdentifier;
    }

    void AssetFileInfoListComparison::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            ComparisonData::Reflect(serializeContext);
            serializeContext->Class<AssetFileInfoListComparison>()
                ->Version(1)
                ->Field("ComparisonDataListType", &AssetFileInfoListComparison::m_comparisonDataList);
        }
    }

    bool AssetFileInfoListComparison::Save(const AZStd::string& destinationFilePath) const
    {
        if (!AzFramework::StringFunc::EndsWith(destinationFilePath, ComparisonRulesFileExtension))
        {
            AZ_Error(ErrorWindowName, false, "Unable to save comparison file (%s). Invalid file extension, comparison files can only have (.%s) extension.\n", destinationFilePath.c_str(), ComparisonRulesFileExtension);
            return false;
        }

        if (AZ::IO::FileIOBase::GetInstance()->Exists(destinationFilePath.c_str()) && AZ::IO::FileIOBase::GetInstance()->IsReadOnly(destinationFilePath.c_str()))
        {
            AZ_Error(ErrorWindowName, false, "Unable to save comparison file (%s): file is marked Read-Only.\n", destinationFilePath.c_str());
            return false;
        }
        return AZ::Utils::SaveObjectToFile(destinationFilePath, AZ::DataStream::StreamType::ST_XML, this);
    }

    AZ::Outcome<AssetFileInfoListComparison, AZStd::string> AssetFileInfoListComparison::Load(const AZStd::string& filePath)
    {
        if (!AzFramework::StringFunc::EndsWith(filePath, ComparisonRulesFileExtension))
        {
            return AZ::Failure(AZStd::string::format("Unable to load comparison file (%s). Invalid file extension, comparison files can only have (.%s) extension.\n", filePath.c_str(), ComparisonRulesFileExtension));
        }

        AssetFileInfoListComparison assetFileInfoListComparison;
        if (!AZ::Utils::LoadObjectFromFileInPlace(filePath.c_str(), assetFileInfoListComparison))
        {
            return AZ::Failure(AZStd::string::format("Failed to load AssetFileInfoComparison file (%s) from disk.\n", filePath.c_str()));
        }
        return AZ::Success(AZStd::move(assetFileInfoListComparison));
    }

    AZ::Outcome<void, AZStd::string> AssetFileInfoListComparison::CompareAndSaveResults(const AZStd::vector<AZStd::string>& intersectionCountAssetListFiles)
    {
        AZ::Outcome<AssetFileInfoList, AZStd::string> result = Compare(intersectionCountAssetListFiles);

        if (!result.IsSuccess())
        {
            return AZ::Failure(result.TakeError());
        }

        return SaveResults();
    }

    AZ::Outcome<void, AZStd::string> AssetFileInfoListComparison::SaveResults() const
    {
        for (auto iter = m_assetFileInfoMap.begin(); iter != m_assetFileInfoMap.end(); iter++)
        {
            if (IsOutputPath(iter->first))
            {
                if (!AssetFileInfoList::Save(iter->second, iter->first))
                {
                    return AZ::Failure(AZStd::string::format("Failed to save result of comparison operation for file %s.\n", iter->first.c_str()));
                }
            }
        }
        return AZ::Success();
    }

    AZStd::vector<AZStd::string> AssetFileInfoListComparison::GetDestructiveOverwriteFilePaths()
    {
        AZStd::vector<AZStd::string> existingPaths;

        for (auto iter = m_assetFileInfoMap.begin(); iter != m_assetFileInfoMap.end(); iter++)
        {
            if (IsOutputPath(iter->first) && AZ::IO::FileIOBase::GetInstance()->Exists(iter->first.c_str()))
            {
                existingPaths.emplace_back(iter->first);
            }
        }

        return existingPaths;
    }

    AssetFileInfoList AssetFileInfoListComparison::GetComparisonResults(const AZStd::string& comparisonKey)
    {
        auto comparisonResults = m_assetFileInfoMap.find(comparisonKey);
        if (comparisonResults == m_assetFileInfoMap.end())
        {
            return AssetFileInfoList();
        }
        return comparisonResults->second;
    }


    const char* AssetFileInfoListComparison::GetComparisonRulesFileExtension()
    {
        return ComparisonRulesFileExtension;
    }

    const char* AssetFileInfoListComparison::GetComparisonTypeName(ComparisonType comparisonType)
    {
        if (comparisonType == ComparisonType::Default)
        {
            return DefaultTypeName;
        }

        return ComparisonTypeNames[aznumeric_cast<AZ::u8>(comparisonType)];
    }

    const char* AssetFileInfoListComparison::GetFilePatternTypeName(FilePatternType filePatternType)
    {
        if (filePatternType == FilePatternType::Default)
        {
            return DefaultTypeName;
        }

        return FilePatternTypeNames[aznumeric_cast<AZ::u8>(filePatternType)];
    }

    const char AssetFileInfoListComparison::GetTokenIdentifier()
    {
        return TokenIdentifier;
    }

    AZ::Outcome<AssetFileInfoList, AZStd::string>  AssetFileInfoListComparison::PopulateAssetFileInfo(const AZStd::string& assetFileInfoPath) const
    {
        AssetFileInfoList assetFileInfoList;
        if (assetFileInfoPath.empty())
        {
            return AZ::Failure(AZStd::string::format("File path for the first asset file info list is empty.\n"));
        }
        if (IsOutputPath(assetFileInfoPath))
        {
            if (!AZ::IO::FileIOBase::GetInstance()->Exists(assetFileInfoPath.c_str()))
            {
                return AZ::Failure(AZStd::string::format("File ( %s ) does not exists on disk.\n", assetFileInfoPath.c_str()));
            }

            if (!AZ::Utils::LoadObjectFromFileInPlace(assetFileInfoPath.c_str(), assetFileInfoList))
            {
                return AZ::Failure(AZStd::string::format("Failed to deserialize file ( %s ).\n", assetFileInfoPath.c_str()));
            }
        }
        else
        {
            auto found = m_assetFileInfoMap.find(assetFileInfoPath);
            if (found != m_assetFileInfoMap.end())
            {
                assetFileInfoList = found->second;
            }
            else
            {
                return AZ::Failure(AZStd::string::format("Failed to find AssetFileInfoList that matches the TAG ( %s ).\n", assetFileInfoPath.c_str()));
            }
        }

        if (!assetFileInfoList.m_fileInfoList.size())
        {
            return AZ::Failure(AZStd::string::format("File ( %s ) does not contain any assets.\n", assetFileInfoPath.c_str()));
        }

        return AZ::Success(assetFileInfoList);
    }

    AZ::Outcome<AssetFileInfoList, AZStd::string> AssetFileInfoListComparison::Compare(const AZStd::vector<AZStd::string>& intersectionCountAssetListFiles)
    {
        AssetFileInfoList lastAssetFileInfoList;
        AZ::Outcome<AssetFileInfoList, AZStd::string> result = AZ::Failure(AZStd::string());

        if (m_comparisonDataList.empty())
        {
            return AZ::Failure(AZStd::string("Comparison failed: no Comparison Steps were provided."));
        }

        //IntersectionCount Operation cannot be combined with other compare operations
        if (m_comparisonDataList[0].m_comparisonType == ComparisonType::IntersectionCount)
        {
            result = IntersectionCount(intersectionCountAssetListFiles);
            if (result.IsSuccess())
            {
                lastAssetFileInfoList = result.GetValue();
                m_assetFileInfoMap[m_comparisonDataList[0].m_output] = result.TakeValue();
            }
            return AZ::Success(lastAssetFileInfoList);
        }

        for (const ComparisonData& comparisonStep : m_comparisonDataList)
        {
            AZ::Outcome<AssetFileInfoList, AZStd::string> assetListOutcome = PopulateAssetFileInfo(comparisonStep.m_firstInput);
            if (!assetListOutcome.IsSuccess())
            {
                return AZ::Failure(assetListOutcome.TakeError());
            }
            AssetFileInfoList firstAssetList = assetListOutcome.TakeValue();
            AssetFileInfoList secondAssetList;

            if (comparisonStep.m_comparisonType != ComparisonType::FilePattern)
            {
                assetListOutcome = PopulateAssetFileInfo(comparisonStep.m_secondInput);
                if (!assetListOutcome.IsSuccess())
                {
                    return AZ::Failure(assetListOutcome.TakeError());
                }
                secondAssetList = assetListOutcome.TakeValue();
            }

            switch (comparisonStep.m_comparisonType)
            {
            case ComparisonType::Delta:
            {
                result = Delta(firstAssetList, secondAssetList);
                break;
            }
            case ComparisonType::Union:
            {
                result = Union(firstAssetList, secondAssetList);
                break;
            }
            case ComparisonType::Intersection:
            {
                result = Intersection(firstAssetList, secondAssetList);
                break;
            }
            case ComparisonType::Complement:
            {
                result = Complement(firstAssetList, secondAssetList);
                break;
            }
            case ComparisonType::FilePattern:
            {
                result = FilePattern(firstAssetList, comparisonStep);
                break;
            }
            default:
                return AZ::Failure(AZStd::string::format("Invalid comparison type ( %s ) specified.\n", ComparisonTypeNames[static_cast<int>(comparisonStep.m_comparisonType)]));
            }

            if (!result.IsSuccess())
            {
                return result;
            }

            lastAssetFileInfoList = result.GetValue();
            m_assetFileInfoMap[comparisonStep.m_output] = result.TakeValue();
        }

        return AZ::Success(lastAssetFileInfoList);
    }

    AssetFileInfoList AssetFileInfoListComparison::Delta(const AssetFileInfoList& firstAssetFileInfoList, const AssetFileInfoList& secondAssetFileInfoList) const
    {
        AZStd::unordered_map<AZ::Data::AssetId, AzToolsFramework::AssetFileInfo> assetIdToAssetFileInfoMap;

        // Populate the map with entries from the secondAssetFileInfoList
        for (const AssetFileInfo& assetFileInfo : secondAssetFileInfoList.m_fileInfoList)
        {
            assetIdToAssetFileInfoMap[assetFileInfo.m_assetId] = assetFileInfo;
        }

        for (const AssetFileInfo& assetFileInfo : firstAssetFileInfoList.m_fileInfoList)
        {
            auto found = assetIdToAssetFileInfoMap.find(assetFileInfo.m_assetId);
            if (found != assetIdToAssetFileInfoMap.end())
            {
                bool isHashEqual = true;
                // checking the file hash
                for (int idx = 0; idx < AzToolsFramework::AssetFileInfo::s_arraySize; idx++)
                {
                    if (found->second.m_hash[idx] != assetFileInfo.m_hash[idx])
                    {
                        isHashEqual = false;
                        break;
                    }
                }

                if (isHashEqual)
                {
                    assetIdToAssetFileInfoMap.erase(found);
                }
            }
        }

        AssetFileInfoList assetFileInfoList;
        for (auto iter = assetIdToAssetFileInfoMap.begin(); iter != assetIdToAssetFileInfoMap.end(); iter++)
        {
            assetFileInfoList.m_fileInfoList.emplace_back(AZStd::move(iter->second));
        }

        return assetFileInfoList;
    }

    AssetFileInfoList AssetFileInfoListComparison::Union(const AssetFileInfoList& firstAssetFileInfoList, const AssetFileInfoList& secondAssetFileInfoList) const
    {
        AZStd::unordered_map<AZ::Data::AssetId, AzToolsFramework::AssetFileInfo> assetIdToAssetFileInfoMap;

        // Populate the map with entries from the secondAssetFileInfoList
        for (const AssetFileInfo& assetFileInfo : secondAssetFileInfoList.m_fileInfoList)
        {
            assetIdToAssetFileInfoMap[assetFileInfo.m_assetId] = assetFileInfo;
        }

        for (const AssetFileInfo& assetFileInfo : firstAssetFileInfoList.m_fileInfoList)
        {
            auto found = assetIdToAssetFileInfoMap.find(assetFileInfo.m_assetId);
            if (found == assetIdToAssetFileInfoMap.end())
            {
                assetIdToAssetFileInfoMap[assetFileInfo.m_assetId] = assetFileInfo;
            }
        }

        //populate AssetFileInfoList from map
        AssetFileInfoList assetFileInfoList;
        for (auto iter = assetIdToAssetFileInfoMap.begin(); iter != assetIdToAssetFileInfoMap.end(); iter++)
        {
            assetFileInfoList.m_fileInfoList.emplace_back(AZStd::move(iter->second));
        }

        return assetFileInfoList;
    }

    AssetFileInfoList AssetFileInfoListComparison::Intersection(const AssetFileInfoList& firstAssetFileInfoList, const AssetFileInfoList& secondAssetFileInfoList) const
    {
        AZStd::unordered_map<AZ::Data::AssetId, AzToolsFramework::AssetFileInfo> assetIdToAssetFileInfoMap;

        // Populate the map with entries from the secondAssetFileInfoList
        for (const AssetFileInfo& assetFileInfo : secondAssetFileInfoList.m_fileInfoList)
        {
            assetIdToAssetFileInfoMap[assetFileInfo.m_assetId] = assetFileInfo;
        }

        AssetFileInfoList assetFileInfoList;
        for (const AssetFileInfo& assetFileInfo : firstAssetFileInfoList.m_fileInfoList)
        {
            auto found = assetIdToAssetFileInfoMap.find(assetFileInfo.m_assetId);
            if (found != assetIdToAssetFileInfoMap.end())
            {
                assetFileInfoList.m_fileInfoList.emplace_back(AZStd::move(found->second));
            }
        }

        return assetFileInfoList;
    }

    AssetFileInfoList AssetFileInfoListComparison::Complement(const AssetFileInfoList& firstAssetFileInfoList, const AssetFileInfoList& secondAssetFileInfoList) const
    {
        AZStd::unordered_map<AZ::Data::AssetId, AzToolsFramework::AssetFileInfo> assetIdToAssetFileInfoMap;
        
        // Populate the map with entries from the firstAssetFileInfoList
        for (const AssetFileInfo& assetFileInfo : firstAssetFileInfoList.m_fileInfoList)
        {
            assetIdToAssetFileInfoMap[assetFileInfo.m_assetId] = assetFileInfo;
        }

        AssetFileInfoList assetFileInfoList;
        for (const AssetFileInfo& assetFileInfo : secondAssetFileInfoList.m_fileInfoList)
        {
            auto found = assetIdToAssetFileInfoMap.find(assetFileInfo.m_assetId);
            if (found == assetIdToAssetFileInfoMap.end())
            {
                assetFileInfoList.m_fileInfoList.emplace_back(AZStd::move(assetFileInfo));
            }
        }

        return assetFileInfoList;
    }

    AZ::Outcome<AssetFileInfoList, AZStd::string> AssetFileInfoListComparison::FilePattern(const AssetFileInfoList& assetFileInfoList, const ComparisonData& comparisonData) const
    {
        if (comparisonData.m_filePattern.empty())
        {
            return AZ::Failure(AZStd::string::format("Invalid Comparison Step: %s File Pattern value cannot be empty.\n", GetFilePatternTypeName(comparisonData.m_filePatternType)));
        }

        bool isWildCard = comparisonData.m_filePatternType == FilePatternType::Wildcard;

        AssetFileInfoList assetFileInfoListResult;
        for (const AssetFileInfo& assetFileInfo : assetFileInfoList.m_fileInfoList)
        {
            if (isWildCard)
            {
                if (AZStd::wildcard_match(comparisonData.m_filePattern, assetFileInfo.m_assetRelativePath))
                {
                    assetFileInfoListResult.m_fileInfoList.push_back(assetFileInfo);
                }
            }
            else
            {
                AZStd::regex regex(comparisonData.m_filePattern.c_str(), AZStd::regex::extended);
                if (AZStd::regex_match(assetFileInfo.m_assetRelativePath.c_str(), regex))
                {
                    assetFileInfoListResult.m_fileInfoList.push_back(assetFileInfo);
                }
            }
        }

        return AZ::Success(assetFileInfoListResult);
    }

    AZ::Outcome<AssetFileInfoList, AZStd::string> AssetFileInfoListComparison::IntersectionCount(const AZStd::vector<AZStd::string>& assetFileInfoPathList) const
    {
        AZStd::unordered_map<AssetFileInfo, unsigned int, AssetFileInfoHasherByAssetId, AssetFileInfoComparatorByAssetId> assetCountMap;
        for (const auto& absoluteAssetFileInfoPath : assetFileInfoPathList)
        {
            AZ::Outcome<AssetFileInfoList, AZStd::string> firstResult = PopulateAssetFileInfo(absoluteAssetFileInfoPath);
            if (!firstResult.IsSuccess())
            {
                return AZ::Failure(firstResult.GetError());
            }
            AssetFileInfoList firstAssetFileInfoList = firstResult.TakeValue();

            for (const AssetFileInfo& assetFileInfo : firstAssetFileInfoList.m_fileInfoList)
            {
                auto assetFound = assetCountMap.find(assetFileInfo);

                if (assetFound == assetCountMap.end())
                {
                    assetCountMap.insert(AZStd::make_pair(assetFileInfo, 1));
                }
                else
                {
                    assetFound->second++;
                }
            }
        }


        // Loop over the map and create a assetFileInfo of assets that appeared at least the number of times specified by the user    
        AssetFileInfoList outputAssetFileInfoList;
        for (auto iter = assetCountMap.begin(); iter != assetCountMap.end(); iter++)
        {
            if (iter->second >= m_comparisonDataList[0].m_intersectionCount)
            {
                outputAssetFileInfoList.m_fileInfoList.emplace_back(iter->first);
            }
        }

        return AZ::Success(outputAssetFileInfoList);
    }
    bool AssetFileInfoListComparison::AddComparisonStep(const ComparisonData& comparisonData)
    {
        return AddComparisonStep(comparisonData, GetNumComparisonSteps());
    }

    bool AssetFileInfoListComparison::AddComparisonStep(const ComparisonData& comparisonData, size_t destinationIndex)
    {
        if (destinationIndex >= m_comparisonDataList.size())
        {
            m_comparisonDataList.emplace_back(comparisonData);
        }
        else
        {
            m_comparisonDataList.insert(&m_comparisonDataList.at(destinationIndex), comparisonData);
        }
        return true;
    }

    bool AssetFileInfoListComparison::RemoveComparisonStep(size_t index)
    {
        if (index >= m_comparisonDataList.size())
        {
            AZ_Error(ErrorWindowName, false, "Input index ( %u ) is invalid.", index);
            return false;
        }

        m_comparisonDataList.erase(&m_comparisonDataList.at(index));
        return true;
    }

    bool AssetFileInfoListComparison::MoveComparisonStep(size_t initialIndex, size_t destinationIndex)
    {
        size_t comparisonDataListSize = m_comparisonDataList.size();
        // No need to check the destinationIndex, if it is out of bounds it will just be appended to the end
        if (initialIndex >= comparisonDataListSize)
        {
            AZ_Error(ErrorWindowName, false, "Input indices ( %u, %u ) are invalid.", initialIndex, destinationIndex);
            return false;
        }

        auto comparisonData = m_comparisonDataList.at(initialIndex);
        m_comparisonDataList.erase(&m_comparisonDataList.at(initialIndex));

        // Since we have modified the list, the indecies after the initialIndex have all shifted
        size_t modifiedDestinationIndex = destinationIndex;
        if (destinationIndex > initialIndex)
        {
            --modifiedDestinationIndex;
        }

        return AddComparisonStep(comparisonData, modifiedDestinationIndex);
    }

    size_t AssetFileInfoListComparison::GetNumComparisonSteps() const
    {
        return m_comparisonDataList.size();
    }

    AZStd::vector<AssetFileInfoListComparison::ComparisonData> AssetFileInfoListComparison::GetComparisonList() const
    {
        return m_comparisonDataList;
    }

    bool AssetFileInfoListComparison::SetComparisonType(size_t index, const ComparisonType comparisonType)
    {
        if (index >= m_comparisonDataList.size())
        {
            AZ_Error(ErrorWindowName, false, "Input index ( %u ) is invalid.", index);
            return false;
        }

        m_comparisonDataList[index].m_comparisonType = comparisonType;

        if (comparisonType != ComparisonType::FilePattern)
        {
            // Only FilePattern operations are allowed to have FilePatternType and FilePattern values
            m_comparisonDataList[index].m_filePatternType = FilePatternType::Default;
            m_comparisonDataList[index].m_filePattern.clear();
        }
        else
        {
            // FilePattern operations only take one input
            m_comparisonDataList[index].m_secondInput.clear();
        }

        return true;
    }

    bool AssetFileInfoListComparison::SetFilePatternType(size_t index, const FilePatternType filePatternType)
    {
        if (index >= m_comparisonDataList.size())
        {
            AZ_Error(ErrorWindowName, false, "Input index ( %u ) is invalid.", index);
            return false;
        }

        if (m_comparisonDataList[index].m_comparisonType != ComparisonType::FilePattern)
        {
            AZ_Error(ErrorWindowName, false,
                "Unable to set File Pattern Type: Comparison Step must be of type ( %s ). Current Comparison Type is: %s",
                ComparisonTypeNames[static_cast<int>(ComparisonType::FilePattern)],
                ComparisonTypeNames[static_cast<int>(m_comparisonDataList[index].m_comparisonType)]);
            return false;
        }

        m_comparisonDataList[index].m_filePatternType = filePatternType;
        return true;
    }

    bool AssetFileInfoListComparison::SetFilePattern(size_t index, const AZStd::string& filePattern)
    {
        if (index >= m_comparisonDataList.size())
        {
            AZ_Error(ErrorWindowName, false, "Input index ( %i ) is invalid.", index);
            return false;
        }

        if (m_comparisonDataList[index].m_comparisonType != ComparisonType::FilePattern)
        {
            AZ_Error(ErrorWindowName, false,
                "Unable to set File Pattern: Comparison Step must be of type ( %s ). Current Comparison Type is: %s",
                ComparisonTypeNames[static_cast<int>(ComparisonType::FilePattern)],
                ComparisonTypeNames[static_cast<int>(m_comparisonDataList[index].m_comparisonType)]);
            return false;
        }

        m_comparisonDataList[index].m_filePattern = filePattern;
        return true;
    }

    bool AssetFileInfoListComparison::SetFirstInput(size_t index, const AZStd::string& firstInput)
    {
        if (index >= m_comparisonDataList.size())
        {
            AZ_Error(ErrorWindowName, false, "Input index ( %i ) is invalid.", index);
            return false;
        }

        m_comparisonDataList[index].m_firstInput = firstInput;
        return true;
    }

    bool AssetFileInfoListComparison::SetSecondInput(size_t index, const AZStd::string& secondInput)
    {
        if (index >= m_comparisonDataList.size())
        {
            AZ_Error(ErrorWindowName, false, "Input index ( %i ) is invalid.", index);
            return false;
        }

        if (m_comparisonDataList[index].m_comparisonType == ComparisonType::FilePattern)
        {
            AZ_Error(ErrorWindowName, false,
                "Unable to set Second Input value: Comparison Step is of type ( %s ), which only allows for one input.",
                ComparisonTypeNames[static_cast<int>(ComparisonType::FilePattern)]);
            return false;
        }

        m_comparisonDataList[index].m_secondInput = secondInput;
        return true;
    }

    bool AssetFileInfoListComparison::SetOutput(size_t index, const AZStd::string& output)
    {
        if (index >= m_comparisonDataList.size())
        {
            AZ_Error(ErrorWindowName, false, "Input index ( %u ) is invalid.", index);
            return false;
        }

        m_comparisonDataList[index].m_output = output;
        return true;
    }

    bool AssetFileInfoListComparison::SetCachedFirstInputPath(size_t index, const AZStd::string& firstInputPath)
    {
        if (index >= m_comparisonDataList.size())
        {
            AZ_Error(ErrorWindowName, false, "Input index ( %u ) is invalid.", index);
            return false;
        }

        m_comparisonDataList[index].m_cachedFirstInputPath = firstInputPath;
        return true;
    }

    bool AssetFileInfoListComparison::SetCachedSecondInputPath(size_t index, const AZStd::string& secondInputPath)
    {
        if (index >= m_comparisonDataList.size())
        {
            AZ_Error(ErrorWindowName, false, "Input index ( %u ) is invalid.", index);
            return false;
        }

        m_comparisonDataList[index].m_cachedSecondInputPath = secondInputPath;
        return true;
    }

    void AssetFileInfoListComparison::FormatOutputToken(AZStd::string& tokenName)
    {
        if (!tokenName.starts_with(TokenIdentifier) && !tokenName.empty())
        {
            AzFramework::StringFunc::Prepend(tokenName, TokenIdentifier);
        }
    }

    AssetFileInfoListComparison::ComparisonData::ComparisonData(const ComparisonType& type, const AZStd::string& destinationPath, const AZStd::string& filePattern, FilePatternType filePatternType, unsigned int intersectionCount)
        : m_comparisonType(type)
        , m_output(destinationPath)
        , m_filePattern(filePattern)
        , m_filePatternType(filePatternType)
        , m_intersectionCount(intersectionCount)
    {
    }

    void AssetFileInfoListComparison::ComparisonData::Reflect(AZ::ReflectContext * context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ComparisonData>()
                ->Version(3)
                ->Field("comparisonType", &ComparisonData::m_comparisonType)
                ->Field("firstInput", &ComparisonData::m_firstInput)
                ->Field("secondInput", &ComparisonData::m_secondInput)
                ->Field("filePattern", &ComparisonData::m_filePattern)
                ->Field("filePatternType", &ComparisonData::m_filePatternType)
                ->Field("destinationPath", &ComparisonData::m_output)
                ->Field("intersectionCount", &ComparisonData::m_intersectionCount);
        }
    }

    /*
    * Asset bundler Path and file name utils
    */
    void SplitFilename(const AZStd::string& filePath, AZStd::string& baseFileName, AZStd::string& platformIdentifier)
    {
        AZStd::string fileName;
        AzFramework::StringFunc::Path::GetFileName(filePath.c_str(), fileName);

        for (AZStd::string_view platformName : AzFramework::PlatformHelper::GetPlatformsInterpreted(AzFramework::PlatformFlags::AllNamedPlatforms))
        {
            AZStd::string appendedPlatform = AZStd::string::format("_%.*s", aznumeric_cast<int>(platformName.size()), platformName.data());
            if (AzFramework::StringFunc::EndsWith(fileName, appendedPlatform))
            {
                AzFramework::StringFunc::RChop(fileName, appendedPlatform.size());
                baseFileName = fileName;
                platformIdentifier = platformName;
                return;
            }
        }
    }

    void RemovePlatformIdentifier(AZStd::string& filePath)
    {
        AZStd::string baseFileName;
        AZStd::string platform;
        SplitFilename(filePath, baseFileName, platform);

        if (platform.empty())
        {
            return;
        }

        AZStd::string extension;
        AzFramework::StringFunc::Path::GetExtension(filePath.c_str(), extension);

        AzFramework::StringFunc::Path::ReplaceFullName(filePath, baseFileName.c_str(), extension.c_str());
    }

    AZStd::string GetPlatformIdentifier(const AZStd::string& filePath)
    {
        AZStd::string baseFileName;
        AZStd::string platform;
        SplitFilename(filePath, baseFileName, platform);
        return platform;
    }
} // namespace AzToolsFramework
