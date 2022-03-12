/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_REPLICA_INTERESTDEFS_H
#define GM_REPLICA_INTERESTDEFS_H

#include <AzCore/base.h>
#include <GridMate/Replica/ReplicaCommon.h>
#include <GridMate/Containers/vector.h>
#include <GridMate/Containers/unordered_set.h>
#include <GridMate/Containers/unordered_map.h>
#include <AzCore/std/sort.h>

namespace GridMate
{
    /**
    * Bitmask used internally in InterestManager to check which handler is responsible for a given interest match
    */
    using InterestHandlerSlot = AZ::u32;

    /**
    * Rule identifier (unique within the session)
    */
    using RuleNetworkId = AZ::u64;
    ///////////////////////////////////////////////////////////////////////////

    using InterestPeerSet = unordered_set<PeerId>;

    /**
     * InterestMatchResult: a structure to gather new matches from handlers.
     * Passed to handler within matching context when handler's Match method is invoked.
     * User must fill the structure with changes that handler recalculated.
     *
     * Specifically, the changes should have all the replicas that had their list of associated peers modified.
     * Each entry replica - new full list of associated peers.
     */
    class InterestMatchResult : public unordered_map<ReplicaId, InterestPeerSet>
    {
    public:
        using unordered_map::unordered_map;

        /*
        * An expensive debug trace helper, prints sorted mapping between replica id's and associated peers.
        */
        void PrintMatchResult(const char* name) const;
    };
    ///////////////////////////////////////////////////////////////////////////

    /**
    *  Base class for interest rules
    */
    class InterestRule
    {
    public:
        explicit InterestRule(PeerId peerId, RuleNetworkId netId)
            : m_peerId(peerId)
            , m_netId(netId)
        {}

        PeerId GetPeerId() const { return m_peerId; }
        RuleNetworkId GetNetworkId() const { return m_netId; }

    protected:
        PeerId m_peerId; ///< the peer this rule is bound to
        RuleNetworkId m_netId; ///< network id
    };
    ///////////////////////////////////////////////////////////////////////////


    /**
    *  Base class for interest attributes
    */
    class InterestAttribute
    {
    public:
        explicit InterestAttribute(ReplicaId replicaId)
            : m_replicaId(replicaId)
        {}

        ReplicaId GetReplicaId() const { return m_replicaId; }

    protected:
        ReplicaId m_replicaId; ///< Replica id this attribute is bound to
    };
    ///////////////////////////////////////////////////////////////////////////

#if !defined(AZ_DEBUG_BUILD)
    AZ_INLINE void InterestMatchResult::PrintMatchResult(const char*) const {}
#else
    AZ_INLINE void InterestMatchResult::PrintMatchResult(const char* name) const
    {
        if (size() == 0)
        {
            AZ_TracePrintf("GridMate", "InterestMatchResult %s empty \n", name);
            return;
        }

        AZStd::vector<value_type> sorted;
        for (auto& r : *this)
        {
            sorted.push_back(r);
        }

        auto sortByReplicaId = [](const value_type& one, const value_type& another)
        {
            return one.first < another.first;
        };
        AZStd::sort(sorted.begin(), sorted.end(), sortByReplicaId);

        AZ_TracePrintf("GridMate", "InterestMatchResult %s \n", name);
        for (auto& match : sorted)
        {
            auto repId = match.first;
            AZ_TracePrintf("GridMate", "\t\t\t for repId %d ", repId);

            // unsorted list of peers
            for (auto& peerId : match.second)
            {
                AZ_TracePrintf("", "peer %d", peerId);
            }

            AZ_TracePrintf("", "\n");
        }
    }
#endif
} // GridMate

#endif // GM_REPLICA_INTERESTDEFS_H
