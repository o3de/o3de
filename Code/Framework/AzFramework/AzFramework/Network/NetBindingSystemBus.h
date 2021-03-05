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

#pragma once

#ifndef AZFRAMEWORK_NET_BINDING_SYSTEM_BUS_H
#define AZFRAMEWORK_NET_BINDING_SYSTEM_BUS_H

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Asset/AssetCommon.h>
#include <GridMate/Replica/ReplicaCommon.h>
#include <GridMate/Session/Session.h>
#include <AzCore/Slice/SliceComponent.h>

namespace AZ
{
    namespace IO
    {
        class GenericStream;
    }
}

namespace AzFramework
{
    const AZ::SliceComponent::SliceInstanceId UnspecifiedSliceInstanceId = AZ::Uuid::CreateNull();

    /**
    */
    typedef AZ::u32 NetBindingContextSequence;
    const NetBindingContextSequence UnspecifiedNetBindingContextSequence = 0;

    /**
    */
    struct NetBindingSliceContext
    {
        NetBindingContextSequence m_contextSequence;
        AZ::Data::AssetId m_sliceAssetId;
        AZ::EntityId m_staticEntityId;
        AZ::EntityId m_runtimeEntityId;
        /**
         * \brief uniquely identifies the slice instance that this entity is being replicated from
         */
        AZ::SliceComponent::SliceInstanceId m_sliceInstanceId;
    };

    /**
    * The net binding system implements this interface and listens on the NetBindingSystemBus.
    *
    * Network binding is activated when OnNetworkSessionActivated event is received with the binding session,
    * and is deactivated by the OnNetworkSessionDeactivated event.
    */
    class NetBindingSystemInterface
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        virtual ~NetBindingSystemInterface() {}

        //! Returns true if a network session is available and entities should bind themselves to the network.
        virtual bool ShouldBindToNetwork() = 0;

        //! Returns the current entity context sequence
        virtual NetBindingContextSequence GetCurrentContextSequence() = 0;

        //! Get a level entity's static id.
        virtual AZ::EntityId GetStaticIdFromEntityId(AZ::EntityId entity) = 0;

        //! Get a level entity's id based on the static id
        virtual AZ::EntityId GetEntityIdFromStaticId(AZ::EntityId staticEntityId) = 0;

        //! Adds a bound replica to the network session as master.
        virtual void AddReplicaMaster(AZ::Entity* entity, GridMate::ReplicaPtr replica) = 0;

        //! Spawn and bind an entity from a slice
        virtual void SpawnEntityFromSlice(GridMate::ReplicaId bindTo, const NetBindingSliceContext& bindToContext) = 0;

        //! Spawn and bind an entity from stream
        virtual void SpawnEntityFromStream(AZ::IO::GenericStream& spawnData, AZ::EntityId useEntityId, GridMate::ReplicaId bindTo, NetBindingContextSequence addToContext) = 0;

        //! De-spawn an entity: deactivates or removes the entity.
        /**
         * /note @sliceInstanceId is the slice instance that the entity belongs to. If it's a level entity, then this should be AZ::Uuid::CreateNull()
         */
        virtual void UnbindGameEntity(AZ::EntityId entity, const AZ::SliceComponent::SliceInstanceId& sliceInstanceId) = 0;
    };
    typedef AZ::EBus<NetBindingSystemInterface> NetBindingSystemBus;

    class NetBindingSystemEvents
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        //! Notification that a network session is created
        virtual void OnNetworkSessionCreated(GridMate::GridSession* session) { (void)session; }

        //! Notification that a network session is ready
        virtual void OnNetworkSessionActivated(GridMate::GridSession* session) { (void)session; }

        //! Notification that a network session is no longer available
        virtual void OnNetworkSessionDeactivated(GridMate::GridSession* session) { (void)session; }
    };
    typedef AZ::EBus<NetBindingSystemEvents> NetBindingSystemEventsBus;
}   // namespace AzFramework

#endif // AZFRAMEWORK_NET_BINDING_SYSTEM_BUS_H
