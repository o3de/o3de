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
                ->Field("millisecondsSinceUnixEpoch", &UuidEntry::m_millisecondsSinceUnixEpoch);
        }
    }

    AZ::Uuid UuidManager::GetUuid(AZ::IO::PathView file)
    {
        auto entry = GetOrCreateUuidEntry(file);

        return entry.m_uuid;
    }

    AZStd::unordered_set<AZ::Uuid> UuidManager::GetLegacyUuids(AZ::IO::PathView file)
    {
        auto entry = GetOrCreateUuidEntry(file);

        return entry.m_legacyUuids;
    }

    void UuidManager::FileChanged(AZ::IO::Path file)
    {
        AZStd::string extension = file.Extension().Native();

        if (extension != AzToolsFramework::MetadataManager::MetadataFileExtension)
        {
            return;
        }

        auto lastDot = extension.find_last_of('.');
        extension = extension.substr(0, lastDot);
        file.ReplaceExtension(extension.c_str());

        AZStd::scoped_lock scopeLock(m_uuidMutex);

        auto normalizedPath = GetCanonicalPath(file);
        auto itr = m_uuids.find(normalizedPath);

        if (itr != m_uuids.end())
        {
            m_uuids.erase(itr);
        }
    }

    void UuidManager::FileRemoved(AZ::IO::Path file)
    {
        AZStd::string extension = file.Extension().Native();

        if (extension != AzToolsFramework::MetadataManager::MetadataFileExtension)
        {
            return;
        }

        file.ReplaceExtension("");

        AZStd::scoped_lock scopeLock(m_uuidMutex);

        auto normalizedPath = GetCanonicalPath(file);
        auto itr = m_uuids.find(normalizedPath);

        if (itr != m_uuids.end())
        {
            m_uuids.erase(itr);
        }
    }

    AZStd::string UuidManager::GetCanonicalPath(AZ::IO::PathView file)
    {
        return file.LexicallyNormal().FixedMaxPathStringAsPosix().c_str();
    }

    UuidManager::UuidEntry UuidManager::GetOrCreateUuidEntry(AZ::IO::PathView file)
    {
        AZStd::scoped_lock scopeLock(m_uuidMutex);

        auto normalizedPath = GetCanonicalPath(file);
        auto itr = m_uuids.find(normalizedPath);

        // Check if we already have the UUID loaded into memory
        if (itr != m_uuids.end())
        {
            return itr->second;
        }

        UuidEntry uuidInfo;

        // Check if there's a metadata file that already contains a saved UUID
        if (GetMetadataManager()->GetValue(file, UuidKey, uuidInfo))
        {
            m_uuids[normalizedPath] = uuidInfo;

            return uuidInfo;
        }

        // Last resort - generate a new UUID and save it to the metadata file
        UuidEntry newUuid = CreateUuidEntry(normalizedPath);

        if (GetMetadataManager()->SetValue(file, UuidKey, newUuid))
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

    UuidManager::UuidEntry UuidManager::CreateUuidEntry(const AZStd::string& file)
    {
        UuidEntry newUuid;

        newUuid.m_uuid = CreateUuid();
        newUuid.m_legacyUuids = CreateLegacyUuids(file);
        newUuid.m_originalPath = file;
        newUuid.m_millisecondsSinceUnixEpoch = aznumeric_cast<AZ::u64>(QDateTime::currentMSecsSinceEpoch());

        return newUuid;
    }

    AZ::Uuid UuidManager::CreateUuid()
    {
        return AZ::Uuid::CreateRandom();
    }

    AZStd::unordered_set<AZ::Uuid> UuidManager::CreateLegacyUuids(const AZStd::string& file)
    {
        return { AssetUtilities::CreateSafeSourceUUIDFromName(file.c_str()),
                 AssetUtilities::CreateSafeSourceUUIDFromName(file.c_str(), false) };
    }
} // namespace AssetProcessor
