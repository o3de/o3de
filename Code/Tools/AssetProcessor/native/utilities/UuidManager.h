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
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/string/string.h>
#include <native/AssetManager/SourceAssetReference.h>

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

        //! Gets the canonical UUID for a given source asset.  A new metadata file may be created with a randomly generated UUID if
        //! generation is enabled for the file type.  Otherwise the UUID returned will follow the legacy system based on the file path.
        virtual AZ::Uuid GetUuid(const SourceAssetReference& sourceAsset) = 0;
        //! Gets the set of legacy UUIDs for the given source asset.  These are all the UUIDs that may have been used to reference this asset based on previous generation methods.
        virtual AZStd::unordered_set<AZ::Uuid> GetLegacyUuids(const SourceAssetReference& sourceAsset) = 0;

        //! Notifies the manager a metadata file has changed so the cache can be cleared.
        //! @param file Absolute path to the metadata file that changed.
        virtual void FileChanged(AZ::IO::Path file) = 0;
        //! Notifies the manager a metadata file has been removed so the cache can be cleared.
        //! @param file Absolute path to the metadata file that was removed.
        virtual void FileRemoved(AZ::IO::Path file) = 0;
        //! Sets the file types (based on file extension) which the manager will generate random UUIDs and store in a metadata file.  Types
        //! which are not enabled will use legacy path-based UUIDs.
        //! @param types Set of file extensions to enable generation for.
        virtual void EnableGenerationForTypes(AZStd::unordered_set<AZStd::string> types) = 0;
    };

    //! Serialized settings type for storing the user preferences for the UUID manager
    struct UuidSettings
    {
        AZ_TYPE_INFO(UuidSettings, "{0E4FD61F-1BB3-4FFF-90DA-E583D75BF948}");

        static void Reflect(AZ::ReflectContext* context);

        AZStd::unordered_set<AZStd::string> m_enabledTypes;
    };

    //! Handles all UUID lookup (and generation) requests for the AssetProcessor
    class UuidManager : public AZ::Interface<IUuidRequests>::Registrar
    {
    public:
        AZ_RTTI(UuidManager, "{49FA0129-7272-4256-A5C6-D789C156E6BA}", IUuidRequests);

        static constexpr const char* UuidKey = "/UUID";

        static void Reflect(AZ::ReflectContext* context);

        AZ::Uuid GetUuid(const SourceAssetReference& sourceAsset) override;
        AZStd::unordered_set<AZ::Uuid> GetLegacyUuids(const SourceAssetReference& sourceAsset) override;
        void FileChanged(AZ::IO::Path file) override;
        void FileRemoved(AZ::IO::Path file) override;
        void EnableGenerationForTypes(AZStd::unordered_set<AZStd::string> types) override;

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
        UuidEntry GetOrCreateUuidEntry(const SourceAssetReference& sourceAsset);
        AzToolsFramework::IMetadataRequests* GetMetadataManager();
        UuidEntry CreateUuidEntry(const SourceAssetReference& sourceAsset, bool enabledType);
        AZ::Uuid CreateUuid();
        AZStd::unordered_set<AZ::Uuid> CreateLegacyUuids(const AZStd::string& file);
        void InvalidateCacheEntry(AZ::IO::Path file);

        AZStd::recursive_mutex m_uuidMutex;
        // Cache of uuids.  AbsPath -> UUIDEntry
        AZStd::unordered_map<AZStd::string, UuidEntry> m_uuids;
        // Types which should use randomly generated UUIDs
        AZStd::unordered_set<AZStd::string> m_enabledTypes;
        AzToolsFramework::IMetadataRequests* m_metadataManager{};
    };
} // namespace AssetProcessor
