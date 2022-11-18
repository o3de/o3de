/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/IO/Path/Path.h>
#include <AzCore/Interface/Interface.h>

namespace AzToolsFramework
{
    struct IMetadataRequests;
}

namespace AssetProcessor
{
    struct IUuidRequests
    {
        AZ_RTTI(IUuidRequests, "{4EA7E0F6-CB4E-4F9C-ADBC-807676D51772}");

        virtual ~IUuidRequests() = default;

        virtual AZ::Uuid GetUuid(AZ::IO::PathView file) = 0;
        virtual AZStd::unordered_set<AZ::Uuid> GetLegacyUuids(AZ::IO::PathView file) = 0;
        virtual void FileChanged(AZ::IO::Path file) = 0;
        virtual void FileRemoved(AZ::IO::Path file) = 0;
    };

    class UuidManager : public AZ::Interface<IUuidRequests>::Registrar
    {
    public:
        AZ_RTTI(UuidManager, "{49FA0129-7272-4256-A5C6-D789C156E6BA}", IUuidRequests);

        static constexpr const char* UuidKey = "/UUID";

        static void Reflect(AZ::ReflectContext* context);

        AZ::Uuid GetUuid(AZ::IO::PathView file) override;
        AZStd::unordered_set<AZ::Uuid> GetLegacyUuids(AZ::IO::PathView file) override;
        void FileChanged(AZ::IO::Path file) override;
        void FileRemoved(AZ::IO::Path file) override;

    private:
        struct UuidEntry
        {
            AZ_TYPE_INFO(UuidEntry, "{FAD60D80-9B1D-421D-A4CA-DD2CA2EA80BB}");

            static void Reflect(AZ::ReflectContext* context);

            // The canonical UUID
            AZ::Uuid m_uuid;
            // A list of UUIDs that used to refer to this file
            AZStd::unordered_set<AZ::Uuid> m_legacyUuids;
            // The relative path of the file when it was originally created
            AZStd::string m_originalPath;
            // Creation time of the UUID entry
            AZ::u64 m_millisecondsSinceUnixEpoch;
        };

        AZStd::string GetCanonicalPath(AZ::IO::PathView file);
        UuidEntry GetOrCreateUuidEntry(AZ::IO::PathView file);
        AzToolsFramework::IMetadataRequests* GetMetadataManager();
        UuidEntry CreateUuidEntry(const AZStd::string& file);
        AZ::Uuid CreateUuid();
        AZStd::unordered_set<AZ::Uuid> CreateLegacyUuids(const AZStd::string& file);

        AZStd::recursive_mutex m_uuidMutex;
        AZStd::unordered_map<AZStd::string, UuidEntry> m_uuids;

        AzToolsFramework::IMetadataRequests* m_metadataManager{};
    };
} // namespace AssetProcessor
