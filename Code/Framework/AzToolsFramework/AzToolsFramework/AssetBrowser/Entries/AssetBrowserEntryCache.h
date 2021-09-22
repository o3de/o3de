/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntry.h>

namespace AZ
{
    namespace Data
    {
        struct AssetId;
    }
    struct Uuid;
    template<class T>
    class EnvironmentVariable;
}

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class ProductAssetBrowserEntry;
        class SourceAssetBrowserEntry;
        /**
        * This exists to handle memory caches that the AssetBrowser system needs
        * which need to be available across DLLs but still with managed life cycles and not leaking memory
        */
        class EntryCache
        {
            AZ_CLASS_ALLOCATOR(EntryCache, AZ::SystemAllocator, 0);
        public:
            static EntryCache* GetInstance();

            // life cycle management:
            static void CreateInstance();
            static void DestroyInstance();

            AZStd::unordered_map<AZ::s64, AssetBrowserEntry*> m_scanFolderIdMap;
            AZStd::unordered_map<AZ::s64, AssetBrowserEntry*> m_fileIdMap;
            AZStd::unordered_map<AZ::Uuid, SourceAssetBrowserEntry*> m_sourceUuidMap;
            AZStd::unordered_map<AZ::s64, SourceAssetBrowserEntry*> m_sourceIdMap;
            AZStd::unordered_map<AZ::Data::AssetId, ProductAssetBrowserEntry*> m_productAssetIdMap;
            AZStd::unordered_map<AZ::s64, AZStd::string> m_knownScanFolders;
            AZStd::unordered_map<AZStd::string, AZ::s64> m_absolutePathToFileId;

            AZStd::unordered_set<AssetBrowserEntry*> m_dirtyThumbnailsSet;
            static const char* s_environmentVariableName;
            static AZ::EnvironmentVariable<EntryCache*> g_globalInstance;

            void Clear();
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework
