/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Entity/SliceEntityOwnershipService.h>
#include <AzFramework/Entity/SliceGameEntityOwnershipServiceBus.h>

namespace AzFramework
{
    class SliceGameEntityOwnershipService
        : public SliceEntityOwnershipService
        , private SliceGameEntityOwnershipServiceRequestBus::Handler
        , private SliceInstantiationResultBus::MultiHandler
    {
    public:
        AZ_CLASS_ALLOCATOR(SliceGameEntityOwnershipService, AZ::SystemAllocator);
        explicit SliceGameEntityOwnershipService(const EntityContextId& entityContextId, AZ::SerializeContext* serializeContext);

        virtual ~SliceGameEntityOwnershipService();

        static void Reflect(AZ::ReflectContext* context);

        void Reset() override;

        //////////////////////////////////////////////////////////////////////////
        // SliceGameEntityOwnershipServiceRequestBus::Handler
        SliceInstantiationTicket InstantiateDynamicSlice(const AZ::Data::Asset<AZ::Data::AssetData>& sliceAsset,
            const AZ::Transform& worldTransform, const AZ::IdUtils::Remapper<AZ::EntityId>::IdMapper& customIdMapper) override;
        void CancelDynamicSliceInstantiation(const SliceInstantiationTicket& ticket) override;
        bool DestroyDynamicSliceByEntity(const AZ::EntityId&) override;
        //////////////////////////////////////////////////////////////////////////
        
        //////////////////////////////////////////////////////////////////////////
        // SliceInstantiationResultBus::MultiHandler
        void OnSlicePreInstantiate(const AZ::Data::AssetId& sliceAssetId, const AZ::SliceComponent::SliceInstanceAddress& instance) override;
        void OnSliceInstantiated(const AZ::Data::AssetId& sliceAssetId, const AZ::SliceComponent::SliceInstanceAddress& instance) override;
        void OnSliceInstantiationFailed(const AZ::Data::AssetId& sliceAssetId) override;
        //////////////////////////////////////////////////////////////////////////
    protected:
        void CreateRootSlice() override;

    private:
        void FlushDynamicSliceDeletionList();

        /**
         * Specifies that a given entity should not be activated by default
         * after it is created.
         * @param entityId The entity that should not be activated by default.
         */
        void MarkEntityForNoActivation(AZ::EntityId entityId);

        struct InstantiatingDynamicSliceInfo
        {
            AZ::Data::Asset<AZ::Data::AssetData> m_asset;
            AZ::Transform m_transform;
        };

        AZStd::unordered_map<SliceInstantiationTicket, InstantiatingDynamicSliceInfo> m_instantiatingDynamicSlices;

        SliceInstanceUnorderedSet m_dynamicSlicesToDestroy;
    };
}
