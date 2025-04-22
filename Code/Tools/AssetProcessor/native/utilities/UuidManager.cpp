/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/parallel/scoped_lock.h>
#include <native/utilities/UuidManager.h>
#include <native/utilities/assetUtils.h>
#include <Metadata/MetadataManager.h>
#include <native/AssetManager/FileStateCache.h>
#include "UuidManager.h"
#include <QDir>

namespace AssetProcessor
{
    void UuidManager::Reflect(AZ::ReflectContext* context)
    {
        UuidSettings::Reflect(context);
    }

    void UuidSettings::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<UuidSettings>()
                ->Version(2)
                ->Field("EnabledTypes", &UuidSettings::m_enabledTypes)
                ->Field("MetaCreationDelayMs", &UuidSettings::m_metaCreationDelayMs);
        }
    }

    AZ::Outcome<AZ::Uuid, AZStd::string> UuidManager::GetUuid(const SourceAssetReference& sourceAsset)
    {
        auto entry = GetOrCreateUuidEntry(sourceAsset);

        if (entry.IsSuccess())
        {
            return AZ::Success(entry.GetValue().m_uuid);
        }

        return AZ::Failure(entry.GetError());
    }

    AZ::Outcome<AZStd::unordered_set<AZ::Uuid>, AZStd::string> UuidManager::GetLegacyUuids(const SourceAssetReference& sourceAsset)
    {
        auto entry = GetOrCreateUuidEntry(sourceAsset);

        if (entry.IsSuccess())
        {
            return AZ::Success(entry.GetValue().m_legacyUuids);
        }

        return AZ::Failure(entry.GetError());
    }

    AZStd::optional<AZ::IO::Path> UuidManager::FindHighestPriorityFileByUuid(AZ::Uuid uuid)
    {
        if (auto sources = FindFilesByUuid(uuid); !sources.empty())
        {
            if (sources.size() == 1)
            {
                return sources[0];
            }
            else
            {
                // There are multiple files with the same legacy UUID, resolve to the highest priority one (highest priority scanfolder,
                // oldest creation time)

                // Convert all the paths into SourceAssetReferences which will get the scanfolder ID
                AZStd::vector<SourceAssetReference> sourceReferences;
                for (const auto& filePath : sources)
                {
                    sourceReferences.emplace_back(filePath);
                }

                // Sort the list based on scanfolder ID
                std::stable_sort(
                    sourceReferences.begin(),
                    sourceReferences.end(),
                    [](const SourceAssetReference& left, const SourceAssetReference& right)
                    {
                        return left.ScanFolderId() < right.ScanFolderId();
                    });

                // Get the range of files from the highest priority scanfolder (having the same scanfolder ID)
                AZ::s64 highestPriorityScanFolder = sourceReferences.front().ScanFolderId();

                AZ::u64 oldestFileTime = AZStd::numeric_limits<AZ::u64>::max();
                SourceAssetReference* oldestFile{};

                // From the files in the highest priority scanfolder, pick the oldest one
                for (auto& source : sourceReferences)
                {
                    if (source.ScanFolderId() > highestPriorityScanFolder)
                    {
                        // Only consider sources from the first, highest priority scanfolder
                        break;
                    }

                    auto entryDetails = GetUuidDetails(source);

                    if (entryDetails)
                    {
                        if (entryDetails.GetValue().m_millisecondsSinceUnixEpoch <= oldestFileTime)
                        {
                            oldestFile = &source;
                            oldestFileTime = entryDetails.GetValue().m_millisecondsSinceUnixEpoch;
                        }
                    }
                }

                return oldestFile->AbsolutePath().c_str();
            }
        }

        return AZStd::nullopt;
    }

    AZStd::optional<AZ::Uuid> UuidManager::GetCanonicalUuid(AZ::Uuid legacyUuid)
    {
        if (auto result = FindHighestPriorityFileByUuid(legacyUuid); result)
        {
            if (auto details = GetUuidDetails(SourceAssetReference(result.value())); details)
            {
                return details.GetValue().m_uuid;
            }
        }

        return AZStd::nullopt;
    }

    AZ::Outcome<AzToolsFramework::MetaUuidEntry, AZStd::string> UuidManager::GetUuidDetails(const SourceAssetReference& sourceAsset)
    {
        return GetOrCreateUuidEntry(sourceAsset);
    }

    AZStd::vector<AZ::IO::Path> UuidManager::FindFilesByUuid(AZ::Uuid uuid)
    {
        AZStd::scoped_lock scopeLock(m_uuidMutex);
        auto itr = m_existingUuids.find(uuid);

        // First check if the UUID matches a canonical UUID.
        // These always have highest priority.
        if (itr != m_existingUuids.end())
        {
            return { itr->second };
        }

        // UUID doesn't match a canonical UUID, see if there are any matching legacy UUIDs.
        // In this case there could be multiple files with the same legacy UUID, so return all of them.
        auto range = m_existingLegacyUuids.equal_range(uuid);
        AZStd::vector<AZ::IO::Path> foundFiles;

        for (auto legacyItr = range.first; legacyItr != range.second; ++legacyItr)
        {
            foundFiles.emplace_back(legacyItr->second);
        }

        return foundFiles;
    }

    void UuidManager::FileChanged(AZ::IO::PathView file)
    {
        InvalidateCacheEntry(file);
    }

    void UuidManager::FileRemoved(AZ::IO::PathView file)
    {
        InvalidateCacheEntry(file);
    }

    void UuidManager::InvalidateCacheEntry(AZ::IO::FixedMaxPath file)
    {
        AZStd::string extension = file.Extension().Native();

        if (extension == AzToolsFramework::MetadataManager::MetadataFileExtension)
        {
            // Remove the metadata part of the extension since the cache is actually keyed by the source file path
            file.ReplaceExtension("");
        }

        AZStd::scoped_lock scopeLock(m_uuidMutex);

        auto normalizedPath = GetCanonicalPath(file);
        auto itr = m_uuids.find(normalizedPath);

        if (itr != m_uuids.end())
        {
            m_existingUuids.erase(itr->second.m_uuid);

            for (auto legacyUuid : itr->second.m_legacyUuids)
            {
                auto range = m_existingLegacyUuids.equal_range(legacyUuid);

                for (auto legacyItr = range.first; legacyItr != range.second; ++legacyItr)
                {
                    if (legacyItr->second == file)
                    {
                        m_existingLegacyUuids.erase(legacyItr);
                        break;
                    }
                }
            }
            m_uuids.erase(itr);
        }
    }

    bool UuidManager::IsGenerationEnabledForFile(AZ::IO::PathView file)
    {
        return m_enabledTypes.contains(file.Extension().Native());
    }

    AZStd::unordered_set<AZStd::string> UuidManager::GetEnabledTypes()
    {
        return m_enabledTypes;
    }

    void UuidManager::EnableGenerationForTypes(AZStd::unordered_set<AZStd::string> types)
    {
        m_enabledTypes = AZStd::move(types);
    }

    AZ::IO::Path UuidManager::GetCanonicalPath(AZ::IO::PathView file)
    {
        return file.LexicallyNormal().FixedMaxPathStringAsPosix().c_str();
    }

    AZ::Outcome<AzToolsFramework::MetaUuidEntry, AZStd::string> UuidManager::GetOrCreateUuidEntry(const SourceAssetReference& sourceAsset)
    {
        AZStd::scoped_lock scopeLock(m_uuidMutex);

        AZ::IO::Path normalizedPath = GetCanonicalPath(sourceAsset.AbsolutePath());
        auto itr = m_uuids.find(normalizedPath);

        // Check if we already have the UUID loaded into memory
        if (itr != m_uuids.end())
        {
            return AZ::Success(itr->second);
        }

        auto* fileStateInterface = AZ::Interface<IFileStateRequests>::Get();

        if (!fileStateInterface)
        {
            AZ_Assert(false, "Programmer Error - IFileStateRequests interface is not available");
            return AZ::Failure(AZStd::string("Programmer Error - IFileStateRequests interface is not available"));
        }

        if (!fileStateInterface->Exists(sourceAsset.AbsolutePath().c_str()))
        {
            AZ_Error(
                "UuidManager",
                false,
                "Programmer Error - cannot request UUID for file which does not exist - %s",
                sourceAsset.AbsolutePath().c_str());
            return AZ::Failure(AZStd::string("Programmer Error - cannot request UUID for file which does not exist"));
        }

        const AZ::IO::Path metadataFilePath = AzToolsFramework::MetadataManager::ToMetadataPath(sourceAsset.AbsolutePath().c_str());
        AssetProcessor::FileStateInfo metadataFileInfo;
        bool metadataFileExists = fileStateInterface->GetFileInfo(metadataFilePath.c_str(), &metadataFileInfo);
        const bool isEnabledType = m_enabledTypes.contains(sourceAsset.AbsolutePath().Extension().Native());

        if constexpr (ASSETPROCESSOR_TRAIT_CASE_SENSITIVE_FILESYSTEM)
        {
            // On case sensitive filesystems, the above exists check will fail if the case is not correct
            // In that case, try to update the case to determine if the file actually exists
            if (!metadataFileExists)
            {
                QString parentPath = QString::fromStdString(AZStd::string(metadataFilePath.ParentPath().Native()).c_str());
                QString caseCorrectedMetadataRelPath = QString::fromStdString(AZStd::string(metadataFilePath.Filename().Native()).c_str());

                // in this case, we got the file name and path from a real existing file that has already got the correct case
                // so the only case correction we may need to do is for the last part (the meta file name)
                // so we can set the checkEntirePath param to false.
                metadataFileExists = AssetUtilities::UpdateToCorrectCase(parentPath, caseCorrectedMetadataRelPath, false);

                if (metadataFileExists)
                {
                    AZ::IO::FixedMaxPath correctAbsolutePath(parentPath.toUtf8().constData());
                    correctAbsolutePath /= caseCorrectedMetadataRelPath.toUtf8().constData();
                    fileStateInterface->GetFileInfo(correctAbsolutePath.c_str(), &metadataFileInfo);
                }
            }
        }

        // Metadata manager can't use the file state cache since it is in AzToolsFramework, so it's faster to do an Exists check up-front.
        if (metadataFileExists)
        {
            AzToolsFramework::MetaUuidEntry uuidInfo;

            // Check if the path computed based on the source asset's filename is different from the on-disk path
            if (metadataFileInfo.m_absolutePath.compare(metadataFilePath.c_str()) != 0)
            {
                // Metadata filename case does not match source filename case
                // Rename the metadata file to match
                AZ::IO::FileIOBase::GetInstance()->Rename(
                    metadataFileInfo.m_absolutePath.toUtf8().constData(), metadataFilePath.c_str());
            }

            // Check if there's a metadata file that already contains a saved UUID
            if (GetMetadataManager()->GetValue(sourceAsset.AbsolutePath(), AzToolsFramework::UuidUtilComponent::UuidKey, uuidInfo))
            {
                // Validate the entry - a null UUID is not ok
                if (uuidInfo.m_uuid.IsNull())
                {
                    return AZ::Failure(AZStd::string::format(
                        "Metadata file exists for %s but UUID is missing or invalid", sourceAsset.AbsolutePath().c_str()));
                }

                // Missing other entries is ok, just generate them now and update the metadata file
                if (uuidInfo.m_legacyUuids.empty() || uuidInfo.m_originalPath.empty() || uuidInfo.m_millisecondsSinceUnixEpoch == 0)
                {
                    AzToolsFramework::MetaUuidEntry regeneratedEntry = CreateUuidEntry(sourceAsset, isEnabledType);

                    if (uuidInfo.m_legacyUuids.empty())
                    {
                       uuidInfo.m_legacyUuids = regeneratedEntry.m_legacyUuids;
                    }

                    if (uuidInfo.m_originalPath.empty())
                    {
                        uuidInfo.m_originalPath = regeneratedEntry.m_originalPath;
                    }

                    if (uuidInfo.m_millisecondsSinceUnixEpoch == 0)
                    {
                        uuidInfo.m_millisecondsSinceUnixEpoch = regeneratedEntry.m_millisecondsSinceUnixEpoch;
                    }

                    // Update the metadata file
                    GetMetadataManager()->SetValue(sourceAsset.AbsolutePath(), AzToolsFramework::UuidUtilComponent::UuidKey, uuidInfo);
                }

                auto outcome = CacheUuidEntry(normalizedPath, uuidInfo, isEnabledType);

                if (outcome)
                {
                    return uuidInfo;
                }
                else
                {
                    return AZ::Failure(outcome.GetError());
                }
            }
        }

        // Last resort - generate a new UUID and save it to the metadata file
        AzToolsFramework::MetaUuidEntry newUuid = CreateUuidEntry(sourceAsset, isEnabledType);

        if (!isEnabledType ||
            GetMetadataManager()->SetValue(sourceAsset.AbsolutePath(), AzToolsFramework::UuidUtilComponent::UuidKey, newUuid))
        {
            auto outcome = CacheUuidEntry(normalizedPath, newUuid, isEnabledType);

            if (outcome)
            {
                return newUuid;
            }
            else
            {
                return AZ::Failure(outcome.GetError());
            }
        }

        return AZ::Failure(AZStd::string::format("Failed to save UUID to metadata file - %s", sourceAsset.AbsolutePath().c_str()));
    }

    AzToolsFramework::IMetadataRequests* UuidManager::GetMetadataManager()
    {
        if (!m_metadataManager)
        {
            m_metadataManager = AZ::Interface<AzToolsFramework::IMetadataRequests>::Get();
        }

        return m_metadataManager;
    }

    AzToolsFramework::MetaUuidEntry UuidManager::CreateUuidEntry(const SourceAssetReference& sourceAsset, bool enabledType)
    {
        AzToolsFramework::MetaUuidEntry newUuid;

        newUuid.m_uuid = enabledType ? CreateUuid() : AssetUtilities::CreateSafeSourceUUIDFromName(sourceAsset.RelativePath().c_str());
        newUuid.m_legacyUuids = CreateLegacyUuids(sourceAsset.RelativePath().c_str());
        newUuid.m_originalPath = sourceAsset.RelativePath().c_str();
        newUuid.m_millisecondsSinceUnixEpoch = enabledType ? aznumeric_cast<AZ::u64>(QDateTime::currentMSecsSinceEpoch()) : 0;

        return newUuid;
    }

    AZ::Outcome<void, AZStd::string> UuidManager::CacheUuidEntry(AZ::IO::PathView normalizedPath, AzToolsFramework::MetaUuidEntry entry, bool enabledType)
    {
        if (enabledType)
        {
            auto result = m_existingUuids.emplace(entry.m_uuid, normalizedPath);

            if (!result.second)
            {
                // Insertion failure means this UUID is duplicated
                return AZ::Failure(AZStd::string::format(
                    "Source " AZ_STRING_FORMAT " has duplicate UUID " AZ_STRING_FORMAT
                    " which is already assigned to another asset " AZ_STRING_FORMAT ". "
                    "Every asset must have a unique ID.  Please change the UUID for one of these assets to resolve the conflict.",
                    AZ_STRING_ARG(normalizedPath.Native()),
                    AZ_STRING_ARG(entry.m_uuid.ToFixedString()),
                    AZ_STRING_ARG(result.first->second.Native())));
            }

            for (const auto& legacyUuid : entry.m_legacyUuids)
            {
                m_existingLegacyUuids.emplace(legacyUuid, normalizedPath);
            }
        }

        m_uuids[normalizedPath] = AZStd::move(entry);
        return AZ::Success();
    }

    AZ::Uuid UuidManager::CreateUuid()
    {
        constexpr int MaxRetry = 50;

        auto uuid = AZ::Uuid::CreateRandom();
        int retry = 0;

        while (m_existingUuids.contains(uuid) && retry < MaxRetry)
        {
            uuid = AZ::Uuid::CreateRandom();
            ++retry;
        }

        if (retry >= MaxRetry)
        {
            AZ_Error("UuidManager", false, "Failed to randomly generate a unique UUID after %d attempts.  UUID not assigned.", retry);
            return AZ::Uuid::CreateNull();
        }

        return uuid;
    }

    AZStd::unordered_set<AZ::Uuid> UuidManager::CreateLegacyUuids(const AZStd::string& relativePath)
    {
        return { AssetUtilities::CreateSafeSourceUUIDFromName(relativePath.c_str()),
                 AssetUtilities::CreateSafeSourceUUIDFromName(relativePath.c_str(), false) };
    }
} // namespace AssetProcessor
