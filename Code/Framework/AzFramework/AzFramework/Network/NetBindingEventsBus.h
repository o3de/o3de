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

#ifndef AZFRAMEWORK_NET_BINDING_EVENTS_BUS_H
#define AZFRAMEWORK_NET_BINDING_EVENTS_BUS_H

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/EntityId.h>
#include <GridMate/Replica/ReplicaCommon.h>

namespace AzFramework
{
    /**
    * NetBindingEventsBus
    * Throws networking related entity events
    */
    class NetBindingEvents
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        typedef AZ::EntityId BusIdType;

        virtual ~NetBindingEvents() {}

        /**
        * Called on authoritative(Master) entity when ownership of this entity is about to be transferred to another peer
        * Returning false from this call will result in denying request for ownership transfer
        */
        virtual bool OnEntityAcceptChangeOwnership(GridMate::PeerId requestor, const GridMate::ReplicaContext& rc) { (void)requestor; (void)rc; return true; }

        /**
        * Called when ownership transfer of an entity is finished.
        */
        virtual void OnEntityChangeOwnership(const GridMate::ReplicaContext& rc) { (void)rc; }
    };

    typedef AZ::EBus<NetBindingEvents> NetBindingEventsBus;
}   // namespace AzFramework

#endif // AZFRAMEWORK_NET_BINDING_EVENTS_BUS_H
#pragma once
