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
            serializeContext->Class<UuidSettings>()->Version(1)->Field("EnabledTypes", &UuidSettings::m_enabledTypes);
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

        // Only metadata files are cached, so make sure the file is a metadata file before continuing
        if (extension != AzToolsFramework::MetadataManager::MetadataFileExtension)
        {
            return;
        }

        // Remove the metadata part of the extension since the cache is actually keyed by the source file path
        file.ReplaceExtension("");

        AZStd::scoped_lock scopeLock(m_uuidMutex);

        auto normalizedPath = GetCanonicalPath(file);
        auto itr = m_uuids.find(normalizedPath);

        if (itr != m_uuids.end())
        {
            m_existingUuids.erase(itr->second.m_uuid);
            m_uuids.erase(itr);
        }
    }

    bool UuidManager::IsGenerationEnabledForFile(AZ::IO::PathView file)
    {
        return m_enabledTypes.contains(file.Extension().Native());
    }

    void UuidManager::EnableGenerationForTypes(AZStd::unordered_set<AZStd::string> types)
    {
        m_enabledTypes = AZStd::move(types);
    }

    AZStd::string UuidManager::GetCanonicalPath(AZ::IO::PathView file)
    {
        return file.LexicallyNormal().FixedMaxPathStringAsPosix().c_str();
    }

    AZ::Outcome<AzToolsFramework::MetaUuidEntry, AZStd::string> UuidManager::GetOrCreateUuidEntry(const SourceAssetReference& sourceAsset)
    {
        AZStd::scoped_lock scopeLock(m_uuidMutex);

        auto normalizedPath = GetCanonicalPath(sourceAsset.AbsolutePath());
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

        const bool fileExists = fileStateInterface->Exists(AzToolsFramework::MetadataManager::ToMetadataPath(sourceAsset.AbsolutePath().c_str()).c_str());
        const bool isEnabledType = m_enabledTypes.contains(sourceAsset.AbsolutePath().Extension().Native());

        // Metadata manager can't use the file state cache since it is in AzToolsFramework, so it's faster to do an Exists check up-front.
        if (fileExists)
        {
            AzToolsFramework::MetaUuidEntry uuidInfo;

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

        if (!fileStateInterface->Exists(sourceAsset.AbsolutePath().c_str()))
        {
            AZ_Error(
                "UuidManager",
                false,
                "Programmer Error - cannot request UUID for file which does not exist - %s",
                sourceAsset.AbsolutePath().c_str());
            return AZ::Failure(AZStd::string("Programmer Error - cannot request UUID for file which does not exist"));
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
        newUuid.m_millisecondsSinceUnixEpoch = aznumeric_cast<AZ::u64>(QDateTime::currentMSecsSinceEpoch());

        return newUuid;
    }

    AZ::Outcome<void, AZStd::string> UuidManager::CacheUuidEntry(AZStd::string_view normalizedPath, AzToolsFramework::MetaUuidEntry entry, bool enabledType)
    {
        if (enabledType)
        {
            auto result = m_existingUuids.emplace(entry.m_uuid, normalizedPath);

            if (!result.second)
            {
                // Insertion failure means this UUID is duplicated
                return AZ::Failure(AZStd::string::format(
                    "Source " AZ_STRING_FORMAT " has duplicate UUID " AZ_STRING_FORMAT " which is already assigned to another asset " AZ_STRING_FORMAT ". "
                    "Every asset must have a unique ID.  Please change the UUID for one of these assets to resolve the conflict.",
                    AZ_STRING_ARG(normalizedPath),
                    AZ_STRING_ARG(entry.m_uuid.ToFixedString()),
                    AZ_STRING_ARG(result.first->second)));
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
