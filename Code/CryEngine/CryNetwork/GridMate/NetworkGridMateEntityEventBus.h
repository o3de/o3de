/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or 
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,  
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
*
*/
#ifndef CRYNETWORK_NETWORKGRIDMATEENTITYEVENTBUS
#define CRYNETWORK_NETWORKGRIDMATEENTITYEVENTBUS

#include <AzCore/EBus/EBus.h>
#include <GridMate/Replica/Replica.h>

namespace GridMate
{
    /*
    * This is a helper bus to bind/unbind legacy cryengine's entities with gridmate's replicas.
    * Every networked cry-entity has an EntityReplica associated with it that provides legacy aspects and RMI support
    * This bus is to help developers using crynetwork shim to add their custom chunks on the entity replica 
    * and to bind custom game obj extensions with those chunks. 
    */
    class NetworkGridMateEntityEvents : public AZ::EBusTraits
    {
    public:
        // Called when new master entity replica is created for a given entityId
        virtual void OnEntityBoundToNetwork(ReplicaPtr replica) { (void)replica; }
        
        // Called when new proxy entity replica is received from the network for a given entityId
        virtual void OnEntityBoundFromNetwork(ReplicaPtr replica) { (void)replica; }
        
        // Called when entity replica is deactivated
        virtual void OnEntityUnboundFromNetwork(ReplicaPtr replica) { (void)replica; }
        
        
        // EBus settings
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        typedef EntityId BusIdType; // This is bus callbacks per each entityId
    };
    
    // Actual bus
    typedef AZ::EBus<NetworkGridMateEntityEvents> NetworkGridMateEntityEventBus;
}

#endif
