/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#ifndef AZFRAMEWORK_NET_BINDING_HANDLER_BUS_H
#define AZFRAMEWORK_NET_BINDING_HANDLER_BUS_H

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/std/parallel/mutex.h>
#include <GridMate/Replica/ReplicaCommon.h>
#include <AzCore/Slice/SliceComponent.h>

namespace AzFramework
{
    /**
    * The NetBindingSystemComponent notifies net binding handlers of binding events on this bus.
    * The net binding component implements this interface and listens on the NetBindingHandlerBus.
    */
    class NetBindingHandlerInterface
        : public AZ::EBusTraits
    {
    public:
        AZ_RTTI(NetBindingHandlerInterface, "{9F84E9FE-81A0-4105-9C51-6C42C83FECAF}");

        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        typedef AZ::EntityId BusIdType;

        virtual ~NetBindingHandlerInterface() {}

        /**
        * Called to let the entity know that it should bind to the network.
        * If bindTo is set, it means that the entity is a proxy and the handler
        * should bind the entity to the specified
        * replica, otherwise it should bind to a new replica and add it via
        * NetBindingSystemBus::AddReplicaMaster.
        */
        virtual void BindToNetwork(GridMate::ReplicaPtr bindTo) = 0;

        /**
        * Called to let the entity know that it should unbind from the network.
        */
        virtual void UnbindFromNetwork() = 0;

        /**
        * Returns true if the entity is bound to the network.
        */
        virtual bool IsEntityBoundToNetwork() = 0;

        /**
        * Returns true if the entity is authoritative on the local node.
        */
        virtual bool IsEntityAuthoritative() = 0;

        /**
        * Flags the entity as part of the level slice.
        */
        virtual void MarkAsLevelSliceEntity() = 0;

        /**
         * Set the slice instance id that this entity was spawned by and belongs to.
         */
        virtual void SetSliceInstanceId(const AZ::SliceComponent::SliceInstanceId& sliceInstanceId) = 0;

        /**
        * Sets the Replica Priority
        */
        virtual void SetReplicaPriority(GridMate::ReplicaPriority replicaPriority) = 0;

        /**
        * Request entity ownership to a given peer (by default to local peer)
        */
        virtual void RequestEntityChangeOwnership(GridMate::PeerId peerId = GridMate::InvalidReplicaPeerId) = 0;

        /**
        * Gets the Replica Priority
        */
        virtual GridMate::ReplicaPriority GetReplicaPriority() const = 0;
    };
    typedef AZ::EBus<NetBindingHandlerInterface> NetBindingHandlerBus;

    /**
     * Set of queries that might want to be made about the networking system
     * mainly wraps up EBus calls to keep the implementing code a bit more readable
     */
    class NetQuery
    {
    public:
        AZ_RTTI(NetQuery, "{AA4C5699-889D-4A73-9AD2-53EB03D8BB99}");

        virtual ~NetQuery() = default;

        static AZ_FORCE_INLINE bool IsEntityAuthoritative(AZ::EntityId entityId)
        {
            bool result = true;
            EBUS_EVENT_ID_RESULT(result,entityId,NetBindingHandlerBus,IsEntityAuthoritative);
            return result;
        }
    };

}   // namespace AzFramework

#endif // AZFRAMEWORK_NET_BINDING_HANDLER_BUS_H
#pragma once
