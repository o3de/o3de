
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

namespace AZ
{
    class SerializeContext;
}

namespace AzFramework
{
    /**
    * Data storage for asset registry.
    * Maintained separate to facilitate easy serialization to/from disk.
    */
    class AssetRegistry
    {
        friend class AssetCatalog;
    public:
        AZ_TYPE_INFO(AssetRegistry, "{5DBC20D9-7143-48B3-ADEE-CCBD2FA6D443}");
        AZ_CLASS_ALLOCATOR(AssetRegistry, AZ::SystemAllocator);

        AssetRegistry() = default;

        void RegisterAsset(AZ::Data::AssetId id, const AZ::Data::AssetInfo& assetInfo);
        void UnregisterAsset(AZ::Data::AssetId id);

        void SetAssetDependencies(const AZ::Data::AssetId& id, const AZStd::vector<AZ::Data::ProductDependency>& dependencies);
        void RegisterAssetDependency(const AZ::Data::AssetId& id, const AZ::Data::ProductDependency& dependency);
        AZStd::vector<AZ::Data::ProductDependency> GetAssetDependencies(const AZ::Data::AssetId& id) const;

        //! LEGACY - do not use in new code unless interfacing with legacy systems.
        //! All new systems should be referring to assets by ID/Type only and should not need to look up by path/
        AZ::Data::AssetId GetAssetIdByPath(const char* assetPath) const;

        using AssetIdToInfoMap = AZStd::unordered_map < AZ::Data::AssetId, AZ::Data::AssetInfo >;
        AssetIdToInfoMap m_assetIdToInfo;

        AZStd::unordered_map<AZ::Data::AssetId, AZStd::vector<AZ::Data::ProductDependency>> m_assetDependencies;

        void Clear();

        static void ReflectSerialize(AZ::SerializeContext* serializeContext);

    private:
        // Add another registry to our existing registry data.  Intended to be called by AssetCatalog::AddDeltaCatalog
        void AddRegistry(AZStd::shared_ptr<AssetRegistry> assetRegistry);

        // use these only through the legacy getters/setters above.
        using AssetPathToIdMap = AZStd::unordered_map < AZ::Uuid, AZ::Data::AssetId >;

        AssetPathToIdMap m_assetPathToId; // for legacy lookups only

        //! LEGACY - do not use in new code unless interfacing with legacy systems.
        //! given an assetPath and AssetID, this stores it in the registry to use with the above GetAssetIdByPath function.
        //! Called automatically by RegisterAsset.
        void SetAssetIdByPath(const char* assetPath, const AZ::Data::AssetId& id);

    };

} // namespace AzFramework
