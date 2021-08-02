/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "NeighborhoodAPI.h"
#include <GridMate/Session/LANSession.h>
#include <GridMate/Replica/ReplicaFunctions.h>

using namespace GridMate;

namespace Neighborhood {
    //---------------------------------------------------------------------
    NeighborReplica::NeighborReplica()
        : m_capabilities("Capabilities")
        , m_owner("Owner")
        , m_persistentName("PersistentName")
        , m_displayName("DisplayName")
    {
    }
    //---------------------------------------------------------------------
    NeighborReplica::NeighborReplica(GridMate::MemberIDCompact owner, const char* persistentName, NeighborCaps capabilities)
        : m_capabilities("Capabilities", capabilities)
        , m_owner("Owner", owner)
        , m_persistentName("PersistentName", persistentName)
        , m_displayName("DisplayName")
    {
    }
    //---------------------------------------------------------------------
    void NeighborReplica::UpdateChunk(const GridMate::ReplicaContext& rc)
    {
        (void)rc;
    }
    //---------------------------------------------------------------------
    void NeighborReplica::OnReplicaActivate(const GridMate::ReplicaContext& /*rc*/)
    {
        // TODO: Should we send the message to ourselves as well?
        if (IsProxy())
        {
            AZ_Assert(m_persistentName.Get().c_str(), "Received NeighborReplica with missing persistent name!");
            AZ_Assert(m_displayName.Get().c_str(), "Received NeighborReplica with missing display name!");
            EBUS_EVENT(NeighborhoodBus, OnNodeJoined, *this);
        }
    }
    //---------------------------------------------------------------------
    void NeighborReplica::OnReplicaDeactivate(const GridMate::ReplicaContext& /*rc*/)
    {
        // TODO: Should we send the message to ourselves as well?
        if (IsProxy())
        {
            EBUS_EVENT(NeighborhoodBus, OnNodeLeft, *this);
        }
    }
    //---------------------------------------------------------------------
    void NeighborReplica::UpdateFromChunk(const GridMate::ReplicaContext& rc)
    {
        (void)rc;
    }
    //---------------------------------------------------------------------
    bool NeighborReplica::IsReplicaMigratable()
    {
        return false;
    }
    //---------------------------------------------------------------------
    NeighborCaps NeighborReplica::GetCapabilities() const
    {
        return m_capabilities.Get();
    }
    //---------------------------------------------------------------------
    GridMate::MemberIDCompact NeighborReplica::GetTargetMemberId() const
    {
        return m_owner.Get();
    }
    //---------------------------------------------------------------------
    const char* NeighborReplica::GetPersistentName() const
    {
        return m_persistentName.Get().c_str();
    }
    //---------------------------------------------------------------------
    void NeighborReplica::SetDisplayName(const char* displayName)
    {
        m_displayName.Set(AZStd::string(displayName));
    }
    //---------------------------------------------------------------------
    const char* NeighborReplica::GetDisplayName() const
    {
        return m_displayName.Get().c_str();
    }

    NeighborReplica::Desc::Desc()
        : GridMate::ReplicaChunkDescriptor(NeighborReplica::GetChunkName(), sizeof(Desc))
    {
    }

    ReplicaChunkBase* NeighborReplica::Desc::CreateFromStream(GridMate::UnmarshalContext& mc)
    {
        GridMate::MemberIDCompact member;
        mc.m_iBuf->Read(member);
        AZStd::string name;
        mc.m_iBuf->Read(name);
        NeighborCaps caps;
        mc.m_iBuf->Read(caps);

        return CreateReplicaChunk<NeighborReplica>(member, name.c_str(), caps);
    }

    void NeighborReplica::Desc::DiscardCtorStream(GridMate::UnmarshalContext& mc)
    {
        GridMate::MemberIDCompact member;
        mc.m_iBuf->Read(member);
        AZStd::string name;
        mc.m_iBuf->Read(name);
        NeighborCaps caps;
        mc.m_iBuf->Read(caps);
    }

    void NeighborReplica::Desc::DeleteReplicaChunk(ReplicaChunkBase* chunkInstance)
    {
        delete chunkInstance;
    }

    void NeighborReplica::Desc::MarshalCtorData(ReplicaChunkBase* chunkInstance, GridMate::WriteBuffer& wb)
    {
        if (NeighborReplica* member = static_cast<NeighborReplica*>(chunkInstance))
        {
            wb.Write(member->GetTargetMemberId());
            wb.Write(member->m_persistentName.Get());
            wb.Write(member->GetCapabilities());
        }
    }

    //---------------------------------------------------------------------
}   // namespace Neighborhood
