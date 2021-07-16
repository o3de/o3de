/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <GridMate/Replica/ReplicaTarget.h>

#include <GridMate/Replica/ReplicaMgr.h>

namespace
{
    // Intrusive list helpers
    template<class T>
    void InitNode(AZStd::intrusive_list_node<T>& node)
    {
        node.m_next = node.m_prev = nullptr;
    }

    template<class T, AZStd::intrusive_list_node<T> T::* PtrToMember>
    void UnlinkNode(AZStd::intrusive_list_node<T>& node)
    {
        if (node.m_prev)
        {
            (node.m_prev->*PtrToMember).m_next = node.m_next;
        }

        if (node.m_next)
        {
            (node.m_next->*PtrToMember).m_prev = node.m_prev;
        }

        node.m_next = node.m_prev = nullptr;
    }
}

namespace GridMate
{
    bool ReplicaTarget::k_enableAck = false;

    ReplicaTarget::ReplicaTarget()
        : m_peer(nullptr)
        , m_flags(0)
        , m_slotMask(0)
        , m_replicaRevision(0)
    {
        InitNode<ReplicaTarget>(m_replicaHook);
        InitNode<ReplicaTarget>(m_peerHook);
    }

    ReplicaTarget::~ReplicaTarget()
    {
        UnlinkNode<ReplicaTarget, & ReplicaTarget::m_replicaHook>(m_replicaHook);
        UnlinkNode<ReplicaTarget, & ReplicaTarget::m_peerHook>(m_peerHook);
    }

    ReplicaTarget* ReplicaTarget::AddReplicaTarget(ReplicaPeer* peer, Replica* replica)
    {
        ReplicaTarget* newTarget = aznew ReplicaTarget();
        newTarget->m_peer = peer;
        newTarget->SetNew(peer->IsNew() || replica->IsNew());
        replica->m_targets.push_back(*newTarget);
        peer->m_targets.push_back(*newTarget);
        return newTarget;
    }

    void ReplicaTarget::SetNew(bool isNew)
    {
        if (isNew)
        {
            m_flags |= TargetNew;
        }
        else
        {
            m_flags &= ~TargetNew;
        }
    }

    bool ReplicaTarget::IsNew() const
    {
        return !!(m_flags & TargetNew);
    }

    bool ReplicaTarget::IsRemoved() const
    {
        return !!(m_flags & TargetRemoved);
    }

    ReplicaPeer* ReplicaTarget::GetPeer() const
    {
        return m_peer;
    }

    void ReplicaTarget::Destroy()
    {
        delete this;
    }
}
