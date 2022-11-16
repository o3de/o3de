/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzCore/Interface/Interface.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/parallel/scoped_lock.h>
#include <Metadata/MetadataManager.h>

#include "assetUtils.h"

namespace AssetProcessor
{
    struct IUuidRequests
    {
        AZ_RTTI(IUuidRequests, "{4EA7E0F6-CB4E-4F9C-ADBC-807676D51772}");

        virtual ~IUuidRequests() = default;

        virtual AZ::Uuid GetUuid(AZ::IO::PathView file) = 0;
        virtual AZStd::vector<AZ::Uuid> GetLegacyUuids(AZ::IO::PathView file) = 0;
    };

    class UuidManager :
        public AZ::Interface<IUuidRequests>::Registrar
    {
    public:
        AZ_RTTI(UuidManager, "{49FA0129-7272-4256-A5C6-D789C156E6BA}", IUuidRequests);

        static constexpr const char* UuidKey = "UUID";

        static void Reflect(AZ::ReflectContext* context)
        {
            UuidEntry::Reflect(context);
        }

        AZ::Uuid GetUuid(AZ::IO::PathView file) override
        {
            auto entry = GetOrCreateUuidEntry(file);

            return entry.m_uuid;
        }

        AZStd::vector<AZ::Uuid> GetLegacyUuids(AZ::IO::PathView file) override
        {
            auto entry = GetOrCreateUuidEntry(file);

            return entry.m_legacyUuids;
        }

    private:
        struct UuidEntry
        {
            AZ_TYPE_INFO(UuidEntry, "{FAD60D80-9B1D-421D-A4CA-DD2CA2EA80BB}");

            static void Reflect(AZ::ReflectContext* context)
            {
                if(auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
                {
                    serializeContext->Class<UuidEntry>()
                        ->Version(0)
                        ->Field("uuid", &UuidEntry::m_uuid)
                        ->Field("legacyUuids", &UuidEntry::m_legacyUuids)
                        ->Field("originalPath", &UuidEntry::m_originalPath)
                        ->Field("millisecondsSinceUnixEpoch", &UuidEntry::m_millisecondsSinceUnixEpoch);
                }
            }

            // The canonical UUID
            AZ::Uuid m_uuid;
            // A list of UUIDs that used to refer to this file
            AZStd::vector<AZ::Uuid> m_legacyUuids;
            // The relative path of the file when it was originally created
            AZStd::string m_originalPath;
            // Creation time of the UUID entry
            AZ::u64 m_millisecondsSinceUnixEpoch;
        };

        AZStd::string GetCanonicalPath(AZ::IO::PathView file)
        {
            return file.LexicallyNormal().FixedMaxPathStringAsPosix().c_str();
        }

        UuidEntry GetOrCreateUuidEntry(AZ::IO::PathView file)
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

        AzToolsFramework::IMetadataRequests* GetMetadataManager()
        {
            if(!m_metadataManager)
            {
                m_metadataManager = AZ::Interface<AzToolsFramework::IMetadataRequests>::Get();
            }

            return m_metadataManager;
        }

        UuidEntry CreateUuidEntry(const AZStd::string& file)
        {
            UuidEntry newUuid;

            newUuid.m_uuid = CreateUuid();
            newUuid.m_legacyUuids = CreateLegacyUuids(file);
            newUuid.m_originalPath = file;
            newUuid.m_millisecondsSinceUnixEpoch = aznumeric_cast<AZ::u64>(QDateTime::currentMSecsSinceEpoch());

            return newUuid;
        }

        AZ::Uuid CreateUuid()
        {
            return AZ::Uuid::CreateRandom();
        }

        AZStd::vector<AZ::Uuid> CreateLegacyUuids(const AZStd::string& file)
        {
            return { AssetUtilities::CreateSafeSourceUUIDFromName(file.c_str()),
                     AssetUtilities::CreateSafeSourceUUIDFromName(file.c_str(), false) };
        }

        AZStd::recursive_mutex m_uuidMutex;
        AZStd::unordered_map<AZStd::string, UuidEntry> m_uuids;

        AzToolsFramework::IMetadataRequests* m_metadataManager{};
    };
}
