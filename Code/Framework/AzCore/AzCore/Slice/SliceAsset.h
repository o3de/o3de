/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZCORE_SLICE_ASSET_H
#define AZCORE_SLICE_ASSET_H

#include <AzCore/Asset/AssetCommon.h>

namespace AZ
{
    class SliceComponent;
    class Entity;

    /**
     * Represents a Slice asset.
     */
    class SliceAsset
        : public Data::AssetData
    {
        friend class SliceAssetHandler;

    public:
        AZ_CLASS_ALLOCATOR(SliceAsset, AZ::SystemAllocator);
        AZ_RTTI(SliceAsset, "{C62C7A87-9C09-4148-A985-12F2C99C0A45}", AssetData);

        SliceAsset(const Data::AssetId& assetId = Data::AssetId());
        ~SliceAsset();

        /**
         * Overwrites the current asset data (if any) and set the asset ready (if not).
         * \param deleteExisting deletes the existing entity on set (if one exist)
         * \returns true of data was set, or false if the asset is currently loading.
         */
        bool SetData(Entity* entity, SliceComponent* component, bool deleteExisting = true);

        Entity* GetEntity()
        {
            return m_entity;
        }

        SliceComponent* GetComponent()
        {
            return m_component;
        }

        virtual SliceAsset* Clone();

        static const char* GetFileFilter()
        {
            return "*.slice";
        }

        static constexpr u32 GetAssetSubId()
        {
            return 1;
        }

        /**
        * Invoked by the AssetManager to determine if this SliceAsset data object should be reloaded
        * when a change to the asset on disk is detected.
        * Checks the state of m_ignoreNextAutoReload to determine this and sets m_ignoreNextAutoReload to false at the end.
        * During a Create Slice operation in Editor the reload is prevented as we have already built the asset in memory
        * and a reload is not needed.
        */
        bool HandleAutoReload() override;

        void SetIgnoreNextAutoReload(bool ignoreNextAutoReload)
        {
            m_ignoreNextAutoReload = ignoreNextAutoReload;
        }

    protected:
        Entity* m_entity; ///< Root entity that should contain only the slice component
        SliceComponent* m_component; ///< Slice component for this asset
        bool m_ignoreNextAutoReload;
    };

    /**
     * Represents an exported Dynamic Slice.
     */
    class DynamicSliceAsset
        : public SliceAsset
    {
    public:
        AZ_CLASS_ALLOCATOR(DynamicSliceAsset, AZ::SystemAllocator);
        AZ_RTTI(DynamicSliceAsset, "{78802ABF-9595-463A-8D2B-D022F906F9B1}", SliceAsset);

        DynamicSliceAsset(const Data::AssetId& assetId = Data::AssetId())
            : SliceAsset(assetId)
        {
        }
        ~DynamicSliceAsset() = default;

        static const char* GetFileFilter()
        {
            return "*.dynamicslice";
        }

        static constexpr u32 GetAssetSubId()
        {
            return 2;
        }
    };

    /// @deprecated Use SliceComponent.
    using PrefabComponent = SliceComponent;

    /// @deprecated Use SliceAsset.
    using PrefabAsset = SliceAsset;

    /// @deprecated Use DynamicSliceAsset.
    using DynamicPrefabAsset = DynamicSliceAsset;

    namespace Data
    {
        /// Asset filter helper for stripping all assets except slices.
        bool AssetFilterSourceSlicesOnly(const AssetFilterInfo& filterInfo);
    }

} // namespace AZ

namespace AZStd
{
    // hash specialization
    template <>
    struct hash<AZ::Data::Asset<AZ::SliceAsset>>
    {
        using argument_type = AZ::Uuid;
        using result_type = size_t;
        size_t operator()(const AZ::Data::Asset<AZ::SliceAsset>& asset) const
        {
            return asset.GetId().m_guid.GetHash();
        }
    };
}
#endif // AZCORE_SLICE_ASSET_H
#pragma once
