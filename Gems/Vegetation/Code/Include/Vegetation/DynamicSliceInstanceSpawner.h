/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Vegetation/InstanceSpawner.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Slice/SliceAsset.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzFramework/Slice/SliceInstantiationBus.h>

namespace Vegetation
{
    /**
    * Instance spawner of dynamic slice instances.
    */
    class DynamicSliceInstanceSpawner
        : public InstanceSpawner
        , private AZ::Data::AssetBus::MultiHandler
        , private AzFramework::SliceInstantiationResultBus::MultiHandler
    {
    public:
        AZ_RTTI(DynamicSliceInstanceSpawner, "{BBA5CC1E-B4CA-4792-89F7-93711E98FBD1}", InstanceSpawner);
        AZ_CLASS_ALLOCATOR(DynamicSliceInstanceSpawner, AZ::SystemAllocator, 0);
        static void Reflect(AZ::ReflectContext* context);

        DynamicSliceInstanceSpawner();
        virtual ~DynamicSliceInstanceSpawner();

        //! Start loading any assets that the spawner will need.
        void LoadAssets() override;

        //! Unload any assets that the spawner loaded.
        void UnloadAssets() override;

        //! Perform any extra initialization needed at the point of registering with the vegetation system.
        void OnRegisterUniqueDescriptor() override;

        //! Perform any extra cleanup needed at the point of unregistering with the vegetation system.
        void OnReleaseUniqueDescriptor() override;

        //! Does this exist but have empty asset references?
        bool HasEmptyAssetReferences() const override;

        //! Has this finished loading any assets that are needed?
        bool IsLoaded() const override;

        //! Are the assets loaded, initialized, and spawnable?
        bool IsSpawnable() const override;

        //! Display name of the instances that will be spawned.
        AZStd::string GetName() const override;

        //! Create a single instance.
        InstancePtr CreateInstance(const InstanceData& instanceData) override;

        //! Destroy a single instance.
        void DestroyInstance(InstanceId id, InstancePtr instance) override;

        AZStd::string GetSliceAssetPath() const;
        void SetSliceAssetPath(const AZStd::string& assetPath);

    private:
        bool DataIsEquivalent(const InstanceSpawner& rhs) const override;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Data::AssetBus::Handler
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

        //////////////////////////////////////////////////////////////////////////
        // SliceInstantiationResultBus::MultiHandler
        void OnSliceInstantiated(const AZ::Data::AssetId& sliceAssetId, const AZ::SliceComponent::SliceInstanceAddress& sliceAddress) override;
        void OnSliceInstantiationFailedOrCanceled(const AZ::Data::AssetId& sliceAssetId, bool cancelled) override;

        AZ::u32 SliceAssetChanged();
        void ResetSliceAsset();

        void UpdateCachedValues();

        //! Verify that the slice only contains data compatible with the dynamic vegetation system.
        bool ValidateSliceContents(const AZ::Data::Asset<AZ::Data::AssetData> asset) const;

        //! Delete an instance of a dynamic slice
        void DeleteSliceInstance(const AzFramework::SliceInstantiationTicket& ticket, AZ::EntityId firstEntityInSlice);

        //! Cached values so that asset isn't accessed on other threads
        bool m_sliceLoadedAndSpawnable = false;

        //! Map from a slice instance ticket to the first entity spawned, needed for destroying the instances.
        AZStd::unordered_map<AzFramework::SliceInstantiationTicket, AZ::EntityId> m_ticketToEntityMap; 

        //! asset data
        AZ::Data::Asset<AZ::DynamicSliceAsset> m_sliceAsset;
    };

} // namespace Vegetation
