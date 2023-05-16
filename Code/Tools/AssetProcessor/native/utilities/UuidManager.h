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
#include <AzToolsFramework/Metadata/UuidUtils.h>
#include <native/AssetManager/SourceAssetReference.h>
#include <AzToolsFramework/Metadata/MetaUuidEntry.h>

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
        virtual AZ::Outcome<AZ::Uuid, AZStd::string> GetUuid(const SourceAssetReference& sourceAsset) = 0;
        //! Gets the set of legacy UUIDs for the given source asset.  These are all the UUIDs that may have been used to reference this asset based on previous generation methods.
        virtual AZ::Outcome<AZStd::unordered_set<AZ::Uuid>, AZStd::string> GetLegacyUuids(const SourceAssetReference& sourceAsset) = 0;
        //! Returns the full set of UUID entry details for the given asset.
        virtual AZ::Outcome<AzToolsFramework::MetaUuidEntry, AZStd::string> GetUuidDetails(const SourceAssetReference& sourceAsset) = 0;
        //! Returns the file(s) matching the provided UUID.  If the UUID provided is a legacy UUID there may be multiple matches.
        //! Note this API relies on the internal cache and expects all files to have had Get(Legacy)Uuid(s) called at least once already.
        //! This is expected to happened already as part of normal AP operations.
        virtual AZStd::vector<AZ::IO::Path> FindFilesByUuid(AZ::Uuid uuid) = 0;
        //! Returns the highest priority file matching the provided UUID.
        //! In the case of a legacy UUID provided which matches multiple files, the oldest file in the highest priority scanfolder will be returned.
        //! Note this API relies on the internal cache and expects all files to have had Get(Legacy)Uuid(s) called at least once already.
        //! This is expected to happened already as part of normal AP operations.
        virtual AZStd::optional<AZ::IO::Path> FindHighestPriorityFileByUuid(AZ::Uuid uuid) = 0;
        //! Upgrades a potentially legacy UUID to the canonical UUID associated with the asset.
        //! Note this API relies on the internal cache and expects all files to have had Get(Legacy)Uuid(s) called at least once already.
        //! This is expected to happened already as part of normal AP operations.
        virtual AZStd::optional<AZ::Uuid> GetCanonicalUuid(AZ::Uuid legacyUuid) = 0;

        //! Notifies the manager a metadata file has changed so the cache can be cleared.
        //! @param file Absolute path to the metadata file that changed.
        virtual void FileChanged(AZ::IO::PathView file) = 0;
        //! Notifies the manager a metadata file has been removed so the cache can be cleared.
        //! @param file Absolute path to the metadata file that was removed.
        virtual void FileRemoved(AZ::IO::PathView file) = 0;
        //! Sets the file types (based on file extension) which the manager will generate random UUIDs and store in a metadata file.  Types
        //! which are not enabled will use legacy path-based UUIDs.
        //! @param types Set of file extensions to enable generation for.
        virtual void EnableGenerationForTypes(AZStd::unordered_set<AZStd::string> types) = 0;
        //! Returns true if UUID generation is enabled for the type (based on file extension), false otherwise.
        virtual bool IsGenerationEnabledForFile(AZ::IO::PathView file) = 0;

        //! Returns the list of file types that UUID generation is enabled for. This is useful over checking the registry setting
        //! directly, because other types may be enabled via code.
        virtual AZStd::unordered_set<AZStd::string> GetEnabledTypes() = 0;
    };

    //! Serialized settings type for storing the user preferences for the UUID manager
    struct UuidSettings
    {
        AZ_TYPE_INFO(UuidSettings, "{0E4FD61F-1BB3-4FFF-90DA-E583D75BF948}");

        static void Reflect(AZ::ReflectContext* context);

        AZ::u32 m_metaCreationDelayMs{};
        AZStd::unordered_set<AZStd::string> m_enabledTypes;
    };

    //! Handles all UUID lookup (and generation) requests for the AssetProcessor
    class UuidManager : public AZ::Interface<IUuidRequests>::Registrar
    {
    public:
        AZ_RTTI(UuidManager, "{49FA0129-7272-4256-A5C6-D789C156E6BA}", IUuidRequests);

        static void Reflect(AZ::ReflectContext* context);

        AZ::Outcome<AZ::Uuid, AZStd::string> GetUuid(const SourceAssetReference& sourceAsset) override;
        AZ::Outcome<AZStd::unordered_set<AZ::Uuid>, AZStd::string> GetLegacyUuids(const SourceAssetReference& sourceAsset) override;
        AZ::Outcome<AzToolsFramework::MetaUuidEntry, AZStd::string> GetUuidDetails(const SourceAssetReference& sourceAsset) override;
        AZStd::vector<AZ::IO::Path> FindFilesByUuid(AZ::Uuid uuid) override;
        AZStd::optional<AZ::IO::Path> FindHighestPriorityFileByUuid(AZ::Uuid uuid) override;
        AZStd::optional<AZ::Uuid> GetCanonicalUuid(AZ::Uuid legacyUuid) override;
        void FileChanged(AZ::IO::PathView file) override;
        void FileRemoved(AZ::IO::PathView file) override;
        void EnableGenerationForTypes(AZStd::unordered_set<AZStd::string> types) override;
        bool IsGenerationEnabledForFile(AZ::IO::PathView file) override;
        AZStd::unordered_set<AZStd::string> GetEnabledTypes() override;

    private:
        AZ::IO::Path GetCanonicalPath(AZ::IO::PathView file);
        AZ::Outcome<AzToolsFramework::MetaUuidEntry, AZStd::string> GetOrCreateUuidEntry(const SourceAssetReference& sourceAsset);
        AzToolsFramework::IMetadataRequests* GetMetadataManager();
        AzToolsFramework::MetaUuidEntry CreateUuidEntry(const SourceAssetReference& sourceAsset, bool enabledType);
        AZ::Outcome<void, AZStd::string> CacheUuidEntry(AZ::IO::PathView normalizedPath, AzToolsFramework::MetaUuidEntry entry, bool enabledType);
        AZ::Uuid CreateUuid();
        AZStd::unordered_set<AZ::Uuid> CreateLegacyUuids(const AZStd::string& file);
        void InvalidateCacheEntry(AZ::IO::FixedMaxPath file);

        AZStd::recursive_mutex m_uuidMutex;
        // Cache of uuids.  AbsPath -> UUIDEntry
        AZStd::unordered_map<AZ::IO::Path, AzToolsFramework::MetaUuidEntry> m_uuids;
        // Types which should use randomly generated UUIDs
        AZStd::unordered_set<AZStd::string> m_enabledTypes;
        // Map of already existing UUIDs -> File path
        AZStd::unordered_map<AZ::Uuid, AZ::IO::Path> m_existingUuids;
        // Map of legacy UUID -> File path.  There may be multiple files with the same legacy UUID
        AZStd::unordered_multimap<AZ::Uuid, AZ::IO::Path> m_existingLegacyUuids;
        AzToolsFramework::IMetadataRequests* m_metadataManager{};
    };
} // namespace AssetProcessor
