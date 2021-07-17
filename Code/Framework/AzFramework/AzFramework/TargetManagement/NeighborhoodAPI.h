/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef AZFRAMEWORK_NEIGHBORHOODAPI_H
#define AZFRAMEWORK_NEIGHBORHOODAPI_H

/**
 * Neighborhood provides the API for apps to join and advertise their presence
 * to a particular development neighborhood.
 * A neighborhood is a peer-to-peer network of nodes (game, editor, hub, etc) so
 * that they can talk to each other during development.
 *
 * THIS FILE IS TO BE INCLUDED By THE TARGET MANAGER ONLY!!!
 */

#include <GridMate/GridMate.h>
#include <GridMate/Session/Session.h>
#include <GridMate/Replica/Replica.h>
#include <AzCore/EBus/EBus.h>
#include <GridMate/Replica/ReplicaChunkDescriptor.h>

// neighborhood common
namespace Neighborhood {
    enum NeighborCapability
    {
        NEIGHBOR_CAP_NONE           = 0,
        NEIGHBOR_CAP_LUA_VM         = 1 << 0,
        NEIGHBOR_CAP_LUA_DEBUGGER   = 1 << 1,
    };
    typedef AZ::u32 NeighborCaps;

    class NeighborReplica;

    /*
     * Neighborhood changes are broadcast at each node through this bus
     */
    class NeighborhoodEvents
        : public AZ::EBusTraits
    {
    public:
        virtual ~NeighborhoodEvents() {}

        // used to advertise a node and its capabilities
        virtual void    OnNodeJoined(const NeighborReplica& /*node*/) {}
        // used to notify that a node is no longer available
        virtual void    OnNodeLeft(const NeighborReplica& /*node*/) {}
    };
    typedef AZ::EBus<NeighborhoodEvents> NeighborhoodBus;

    /*
     * The NeighborReplica is used to advertise features present at each node
     */
    class NeighborReplica
        : public GridMate::ReplicaChunk
    {
    public:
        GM_CLASS_ALLOCATOR(NeighborReplica);

        NeighborReplica();
        NeighborReplica(GridMate::MemberIDCompact owner, const char* persistentName, NeighborCaps capabilities = NEIGHBOR_CAP_NONE);

        //////////////////////////////////////////////////////////////////////
        //! GridMate::ReplicaChunk overrides.
        static const char* GetChunkName() { return "NeighborReplica"; }
        void UpdateChunk(const GridMate::ReplicaContext& rc) override;
        void OnReplicaActivate(const GridMate::ReplicaContext& rc) override;
        void OnReplicaDeactivate(const GridMate::ReplicaContext& rc) override;
        void UpdateFromChunk(const GridMate::ReplicaContext& rc) override;
        bool IsReplicaMigratable() override;

        NeighborCaps                GetCapabilities() const;
        GridMate::MemberIDCompact   GetTargetMemberId() const;
        const char*                 GetPersistentName() const;
        void                        SetDisplayName(const char* displayName);
        const char*                 GetDisplayName() const;

        class Desc
            : public GridMate::ReplicaChunkDescriptor
        {
        public:
            Desc();

            ReplicaChunkBase* CreateFromStream(GridMate::UnmarshalContext& mc) override;
            void DiscardCtorStream(GridMate::UnmarshalContext& mc) override;
            void DeleteReplicaChunk(ReplicaChunkBase* chunkInstance) override;
            void MarshalCtorData(ReplicaChunkBase* chunkInstance, GridMate::WriteBuffer& wb) override;
        };

    protected:
        GridMate::DataSet<NeighborCaps>                 m_capabilities;
        GridMate::DataSet<GridMate::MemberIDCompact>    m_owner;
        GridMate::DataSet<AZStd::string>                m_persistentName;
        GridMate::DataSet<AZStd::string>                m_displayName;
    };
    typedef AZStd::intrusive_ptr<NeighborReplica> NeighborReplicaPtr;
    //---------------------------------------------------------------------
}   // namespace Neighborhood


#endif  // AZFRAMEWORK_NEIGHBORHOODAPI_H
#pragma once
