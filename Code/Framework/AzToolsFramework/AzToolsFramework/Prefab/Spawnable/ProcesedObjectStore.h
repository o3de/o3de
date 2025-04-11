/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/utils.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/typetraits/typetraits.h>

namespace AzToolsFramework::Prefab::PrefabConversionUtils
{
    //! Storage for objects created through the Prefab processing pipeline.
    //! These typically store the created object for immediate use in the editor plus additional information
    //! to allow the Prefab Builder to convert the object into a serialized form and register it with the
    //! Asset Database.
    class ProcessedObjectStore
    {
    public:
        using SerializerFunction = AZStd::function<bool(AZStd::vector<uint8_t>&, const ProcessedObjectStore&)>;

        struct AssetSmartPtrDeleter
        {
            void operator()(AZ::Data::AssetData* asset);
        };
        using AssetSmartPtr = AZStd::unique_ptr<AZ::Data::AssetData, AssetSmartPtrDeleter>; 

        //! Constructs a new instance.
        //! @param uniqueId A name for the object that's unique within the scope of the Prefab. This name will be used to generate a sub id for the product
        //!     which requires that the name to be stable between runs.
        //! @param sourceId The uuid for the source file.
        //! @param assetSerializer The callback used to convert the provided asset into a binary version.
        template<typename T>
        static AZStd::pair<ProcessedObjectStore, T*> Create(AZStd::string uniqueId, const AZ::Uuid& sourceId,
            SerializerFunction assetSerializer);
        
        bool Serialize(AZStd::vector<uint8_t>& output) const;
        static uint32_t BuildSubId(AZStd::string_view id);
        uint32_t GetSubId() const;

        bool HasAsset() const;
        AZ::Data::AssetType GetAssetType() const;
        const AZ::Data::AssetData& GetAsset() const;
        AZ::Data::AssetData& GetAsset();
        AssetSmartPtr ReleaseAsset();

        AZStd::vector<AZ::Data::Asset<AZ::Data::AssetData>>& GetReferencedAssets();
        const AZStd::vector<AZ::Data::Asset<AZ::Data::AssetData>>& GetReferencedAssets() const;

        const AZStd::string& GetId() const;

    private:
        ProcessedObjectStore(AZStd::string uniqueId, AssetSmartPtr asset, SerializerFunction assetSerializer);

        SerializerFunction m_assetSerializer;
        AssetSmartPtr m_asset;
        AZStd::vector<AZ::Data::Asset<AZ::Data::AssetData>> m_referencedAssets;
        AZStd::string m_uniqueId;
    };

    template<typename T>
    AZStd::pair<ProcessedObjectStore, T*> ProcessedObjectStore::Create(AZStd::string uniqueId, const AZ::Uuid& sourceId,
        SerializerFunction assetSerializer)
    {
        static_assert(AZStd::is_base_of_v<AZ::Data::AssetData, T>,
            "ProcessedObjectStore can only be created from a class that derives from AZ::Data::AssetData.");
        AZ::Data::AssetId assetId(sourceId, BuildSubId(uniqueId));
        auto instance = AssetSmartPtr(aznew T(assetId, AZ::Data::AssetData::AssetStatus::Ready));
        ProcessedObjectStore resultLeft(AZStd::move(uniqueId), AZStd::move(instance), AZStd::move(assetSerializer));
        T* resultRight = static_cast<T*>(&resultLeft.GetAsset());
        return AZStd::make_pair(AZStd::move(resultLeft), resultRight);
    }
} // namespace AzToolsFramework::Prefab::PrefabConversionUtils
