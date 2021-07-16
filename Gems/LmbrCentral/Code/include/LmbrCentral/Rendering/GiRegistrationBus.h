/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Math/Aabb.h>
#include <LmbrCentral/Rendering/MeshAsset.h>

namespace LmbrCentral
{
    /*!
    * Messages for handling SVOGI registration. 
    */
    class GiRegistration
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        using MutexType = AZStd::recursive_mutex;
        //    static const bool LocklessDispatch = true

        //Upsert will unpack the mesh, transform, world aabb and material and upsert to the gi system. 
        //If something is already registered on this entityId it will be removed then reinserted.
        virtual void UpsertToGi(AZ::EntityId entityId, AZ::Transform transform, AZ::Aabb worldAabb, 
            AZ::Data::Asset<MeshAsset> meshAsset, _smart_ptr<IMaterial> material) = 0;
        //Remove will remove any data associated with the entityId. 
        virtual void RemoveFromGi(AZ::EntityId entityId) = 0;
    };

    using GiRegistrationBus = AZ::EBus<GiRegistration>;

} // namespace LmbrCentral
