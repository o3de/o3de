/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GridMate/Replica/Interest/InterestManager.h>
#include <GridMate/Replica/Interest/RulesHandler.h>
#include <GridMate/Replica/Interest/InterestQueryResult.h>

#include <GridMate/Replica/ReplicaMgr.h>

#include <AzCore/std/containers/array.h>

namespace GridMate
{
    static const unsigned k_maxHandlers = sizeof(GridMate::InterestHandlerSlot) * CHAR_BIT;

    /**
    * Hashing utils
    */
    struct ReplicaHashByPeer
    {
        AZ_FORCE_INLINE AZStd::size_t operator()(const ReplicaTarget* t) const
        {
            static_assert(sizeof(AZStd::size_t) >= sizeof(ReplicaPeer*), "Types sizes mismatch");
            return reinterpret_cast<AZStd::size_t>(t->GetPeer());
        }
    };

    struct ReplicaEqualToByPeer 
    {
        AZ_FORCE_INLINE bool operator()(const ReplicaTarget* left, const ReplicaTarget* right) const
        {
            return left->GetPeer() == right->GetPeer();
        }
    };

    struct ReplicaHashByPeerId
    {
        AZ_FORCE_INLINE AZStd::size_t operator()(PeerId peerId) const
        {
            static_assert(sizeof(AZStd::size_t) >= sizeof(PeerId), "Types sizes mismatch");
            return static_cast<AZStd::size_t>(peerId);
        }
    };

    struct ReplicaEqualToByPeerId
    {
        AZ_FORCE_INLINE bool operator()(PeerId peerId, const ReplicaTarget* right) const
        {
            return peerId == right->GetPeer()->GetId();
        }
    };
    ///////////////////////////////////////////////////////////////////////////

    /**
    * InterestManager
    */
    InterestManager::InterestManager()
        : m_rm(nullptr)
        , m_freeSlots(~0u)
    {
    }

    void InterestManager::Init(const InterestManagerDesc& desc)
    {
        m_rm = desc.m_rm;
        AZ_Assert(m_rm, "Invalid replica manager");
    }

    bool InterestManager::IsReady() const
    {
        return m_rm != nullptr;
    }

    InterestManager::~InterestManager()
    {
        while (!m_handlers.empty())
        {
            m_handlers.back()->OnRulesHandlerUnregistered(this);
            m_handlers.pop_back();
        }
    }

    void InterestManager::RegisterHandler(BaseRulesHandler* handler)
    {
        AZ_Assert(handler, "Invalid rules handler");

        for (BaseRulesHandler* h : m_handlers)
        {
            if (h == handler)
            {
                AZ_TracePrintf("GridMate", "Rules handler %p is already registered", handler);
                return;
            }
        }

        InterestHandlerSlot slot = GetNewSlot();
        if (!slot)
        {
            AZ_TracePrintf("GridMate", "Too many rules handlers, max=%u\n", k_maxHandlers);
            return;
        }

        handler->m_slot = slot;
        m_handlers.push_back(handler);
        handler->OnRulesHandlerRegistered(this);
    }

    void InterestManager::UnregisterHandler(BaseRulesHandler* handler)
    {
        AZ_Assert(handler, "Invalid rules handler");

        for (auto it = m_handlers.begin(); it != m_handlers.end(); ++it)
        {
            if (*it == handler)
            {
                handler->OnRulesHandlerUnregistered(this);
                m_handlers.erase(it);
                return;
            }
        }

        AZ_Assert(false, "Handler was not registered");
    }

    void InterestManager::Update()
    {
        // Updating all handlers
        for (BaseRulesHandler* handler : m_handlers)
        {
            handler->Update();
        }

        // merging results from every handler
        for (BaseRulesHandler* handler : m_handlers)
        {
            const InterestMatchResult& result = handler->GetLastResult();

            for (auto& match : result)
            {
                ReplicaPtr replica = m_rm->FindReplica(match.first);
                if (!replica) // replica was destroyed: ignoring this match
                {
                    continue;
                }

                unordered_set<ReplicaTarget*, ReplicaHashByPeer, ReplicaEqualToByPeer> targets;

                for (ReplicaTarget& targetObj : replica->m_targets)
                {
                    targets.insert(&targetObj);
                    if (!match.second.count(targetObj.GetPeer()->GetId()))
                    {
                        targetObj.m_slotMask &= ~handler->m_slot;
                        if (!targetObj.m_slotMask)
                        {
                            targetObj.m_flags |= ReplicaTarget::TargetRemoved;
                            m_rm->OnReplicaChanged(replica);
                        }
                    }
                }

                for (const PeerId& peerId : match.second)
                {
                    ReplicaTarget* rt = nullptr;

                    auto it = targets.find_as(peerId, ReplicaHashByPeerId(), ReplicaEqualToByPeerId());
                    if (it == targets.end())
                    {
                        ReplicaPeer* peer = m_rm->FindPeer(peerId);

                        if (!ShouldForward(replica.get(), peer))
                        {
                            continue;
                        }

                        rt = ReplicaTarget::AddReplicaTarget(peer, replica.get());
                        rt->SetNew(true);
                        m_rm->OnReplicaChanged(replica);
                    }
                    else
                    {
                        rt = *it;
                    }

                    rt->m_slotMask |= handler->m_slot;
                    rt->m_flags &= ~ReplicaTarget::TargetRemoved;
                }
            }
        }
    }


    bool InterestManager::ShouldForward(Replica* replica, ReplicaPeer* peer) const
    {
        if (!peer) // invalid peer
        {
            return false;
        }

        if (replica->IsPrimary()) // own the replica?
        {
            return true;
        }

        if (m_rm->GetLocalPeerId() == peer->GetId() || peer->GetId() == replica->m_upstreamHop->GetId()) // forwarding to local peer or to owner?
        {
            return false;
        }

        if (m_rm->IsSyncHost() && !(replica->m_upstreamHop->GetMode() == Mode_Peer && peer->GetMode() == Mode_Peer)) // we are host and replica' owner and target are not connected
        {
            return true;
        }

        return false;
    }

    InterestHandlerSlot InterestManager::GetNewSlot()
    {
        InterestHandlerSlot s = m_freeSlots;
        for (unsigned i = 0; s && i < k_maxHandlers; ++i, s >>= 1)
        {
            if (s & 1)
            {
                InterestHandlerSlot slot = (1 << i);
                m_freeSlots &= ~slot;
                return slot;
            }
        }
        return 0;
    }

    void InterestManager::FreeSlot(InterestHandlerSlot slot)
    {
        m_freeSlots |= slot;
    }
    ///////////////////////////////////////////////////////////////////////////
}
