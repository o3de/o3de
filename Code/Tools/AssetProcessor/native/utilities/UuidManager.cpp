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

    AZ::Uuid UuidManager::GetUuid(const SourceAssetReference& sourceAsset)
    {
        auto entry = GetOrCreateUuidEntry(sourceAsset);

        return entry.m_uuid;
    }

    AZStd::unordered_set<AZ::Uuid> UuidManager::GetLegacyUuids(const SourceAssetReference& sourceAsset)
    {
        auto entry = GetOrCreateUuidEntry(sourceAsset);

        return entry.m_legacyUuids;
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

    AzToolsFramework::UuidEntry UuidManager::GetOrCreateUuidEntry(const SourceAssetReference& sourceAsset)
    {
        AZStd::scoped_lock scopeLock(m_uuidMutex);

        auto normalizedPath = GetCanonicalPath(sourceAsset.AbsolutePath());
        auto itr = m_uuids.find(normalizedPath);

        // Check if we already have the UUID loaded into memory
        if (itr != m_uuids.end())
        {
            return itr->second;
        }

        auto* fileStateInterface = AZ::Interface<IFileStateRequests>::Get();

        if (!fileStateInterface)
        {
            AZ_Assert(false, "Programmer Error - IFileStateRequests interface is not available");
            return {};
        }

        const bool fileExists = fileStateInterface->Exists(AzToolsFramework::MetadataManager::ToMetadataPath(sourceAsset.AbsolutePath().c_str()).c_str());
        const bool isEnabledType = m_enabledTypes.contains(sourceAsset.AbsolutePath().Extension().Native());

        // Metadata manager can't use the file state cache since it is in AzToolsFramework, so it's faster to do an Exists check up-front.
        if (fileExists)
        {
            AzToolsFramework::UuidEntry uuidInfo;

            // Check if there's a metadata file that already contains a saved UUID
            if (GetMetadataManager()->GetValue(sourceAsset.AbsolutePath(), AzToolsFramework::UuidUtilComponent::UuidKey, uuidInfo))
            {
                // Validate the entry - a null UUID is not ok
                if (uuidInfo.m_uuid.IsNull())
                {
                    AZ_Error("UuidManager", false, "Metadata file exists for %s but UUID is missing or invalid", sourceAsset.AbsolutePath().c_str());
                    return {};
                }

                // Missing other entries is ok, just generate them now and update the metadata file
                if (uuidInfo.m_legacyUuids.empty() || uuidInfo.m_originalPath.empty() || uuidInfo.m_millisecondsSinceUnixEpoch == 0)
                {
                    AzToolsFramework::UuidEntry regeneratedEntry = CreateUuidEntry(sourceAsset, isEnabledType);

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

                m_uuids[normalizedPath] = uuidInfo;

                return uuidInfo;
            }
        }

        if (!fileStateInterface->Exists(sourceAsset.AbsolutePath().c_str()))
        {
            AZ_Error(
                "UuidManager",
                false,
                "Programmer Error - cannot request UUID for file which does not exist - %s",
                sourceAsset.AbsolutePath().c_str());
            return {};
        }

        // Last resort - generate a new UUID and save it to the metadata file
        AzToolsFramework::UuidEntry newUuid = CreateUuidEntry(sourceAsset, isEnabledType);

        if (!isEnabledType ||
            GetMetadataManager()->SetValue(sourceAsset.AbsolutePath(), AzToolsFramework::UuidUtilComponent::UuidKey, newUuid))
        {
            m_uuids[normalizedPath] = newUuid;

            return newUuid;
        }

        return {};
    }

    AzToolsFramework::IMetadataRequests* UuidManager::GetMetadataManager()
    {
        if (!m_metadataManager)
        {
            m_metadataManager = AZ::Interface<AzToolsFramework::IMetadataRequests>::Get();
        }

        return m_metadataManager;
    }

    AzToolsFramework::UuidEntry UuidManager::CreateUuidEntry(const SourceAssetReference& sourceAsset, bool enabledType)
    {
        AzToolsFramework::UuidEntry newUuid;

        newUuid.m_uuid = enabledType ? CreateUuid() : AssetUtilities::CreateSafeSourceUUIDFromName(sourceAsset.RelativePath().c_str());
        newUuid.m_legacyUuids = CreateLegacyUuids(sourceAsset.RelativePath().c_str());
        newUuid.m_originalPath = sourceAsset.RelativePath().c_str();
        newUuid.m_millisecondsSinceUnixEpoch = aznumeric_cast<AZ::u64>(QDateTime::currentMSecsSinceEpoch());

        return newUuid;
    }

    AZ::Uuid UuidManager::CreateUuid()
    {
        return AZ::Uuid::CreateRandom();
    }

    AZStd::unordered_set<AZ::Uuid> UuidManager::CreateLegacyUuids(const AZStd::string& relativePath)
    {
        return { AssetUtilities::CreateSafeSourceUUIDFromName(relativePath.c_str()),
                 AssetUtilities::CreateSafeSourceUUIDFromName(relativePath.c_str(), false) };
    }
} // namespace AssetProcessor
