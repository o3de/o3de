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

namespace AssetProcessor
{
    void UuidManager::Reflect(AZ::ReflectContext* context)
    {
        UuidEntry::Reflect(context);
        UuidSettings::Reflect(context);
    }

    void UuidManager::UuidEntry::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<UuidEntry>()
                ->Version(0)
                ->Field("uuid", &UuidEntry::m_uuid)
                ->Field("legacyUuids", &UuidEntry::m_legacyUuids)
                ->Field("originalPath", &UuidEntry::m_originalPath)
                ->Field("creationUnixEpochMS", &UuidEntry::m_millisecondsSinceUnixEpoch);
        }
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

    void UuidManager::FileChanged(AZ::IO::Path file)
    {
        InvalidateCacheEntry(AZStd::move(file));
    }

    void UuidManager::FileRemoved(AZ::IO::Path file)
    {
        InvalidateCacheEntry(AZStd::move(file));
    }

    void UuidManager::InvalidateCacheEntry(AZ::IO::Path file)
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

    void UuidManager::EnableGenerationForTypes(AZStd::unordered_set<AZStd::string> types)
    {
        m_enabledTypes = AZStd::move(types);
    }

    AZStd::string UuidManager::GetCanonicalPath(AZ::IO::PathView file)
    {
        return file.LexicallyNormal().FixedMaxPathStringAsPosix().c_str();
    }

    UuidManager::UuidEntry UuidManager::GetOrCreateUuidEntry(const SourceAssetReference& sourceAsset)
    {
        AZStd::scoped_lock scopeLock(m_uuidMutex);

        auto normalizedPath = GetCanonicalPath(sourceAsset.AbsolutePath());
        auto itr = m_uuids.find(normalizedPath);

        // Check if we already have the UUID loaded into memory
        if (itr != m_uuids.end())
        {
            return itr->second;
        }

        UuidEntry uuidInfo;

        // Check if there's a metadata file that already contains a saved UUID
        if (GetMetadataManager()->GetValue(sourceAsset.AbsolutePath(), UuidKey, uuidInfo))
        {
            m_uuids[normalizedPath] = uuidInfo;

            return uuidInfo;
        }

        const bool isEnabledType = m_enabledTypes.contains(sourceAsset.AbsolutePath().Extension().Native());
        // Last resort - generate a new UUID and save it to the metadata file
        UuidEntry newUuid = CreateUuidEntry(sourceAsset, isEnabledType);

        if (!isEnabledType || GetMetadataManager()->SetValue(sourceAsset.AbsolutePath(), UuidKey, newUuid))
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

    UuidManager::UuidEntry UuidManager::CreateUuidEntry(const SourceAssetReference& sourceAsset, bool enabledType)
    {
        UuidEntry newUuid;

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
