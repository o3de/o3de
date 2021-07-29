/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef GM_REPLICA_INTERESTEVENTS_H
#define GM_REPLICA_INTERESTEVENTS_H

#if defined(GM_INTEREST_MANAGER)

#include <AzCore/base.h>
#include <GridMate/Replica/ReplicaCommon.h>

#include <GridMate/containers/unordered_set.h>
#include <GridMate/containers/unordered_map.h>
#include <GridMate/EBus.h>

namespace GridMate
{
    /**
    *  EBus for interest manager's events.
    *  Notifies subscribers about new interest matches and new mismatches happened.
    */
    class InterestManagerEvents
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        typedef AZStd::recursive_mutex MutexType;
        typedef void* BusIdType;
        typedef SysContAlloc AllocatorType;

        virtual ~InterestManagerEvents() {}

        /**
        * Called when new pair of replica and peer matched their interest
        */
        virtual void OnInterestMatched(ReplicaId replicaId, PeerId peerId) { (void) replicaId; (void) peerId; }

        /**
        * Called when pair of replica and peer mismatched interest (only called if the pair was previously matching)
        */
        virtual void OnInterestUnmatched(ReplicaId replicaId, PeerId peerId) { (void) replicaId; (void) peerId; }
    };

    typedef AZ::EBus<InterestManagerEvents> InterestManagerEventsBus;
}

#endif // GM_INTEREST_MANAGER
#endif
