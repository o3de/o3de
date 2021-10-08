/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_REPLICA_REPLICATARGET_H
#define GM_REPLICA_REPLICATARGET_H

#include <GridMate/Replica/ReplicaCommon.h>

#include <AzCore/std/containers/intrusive_list.h>

namespace GridMate
{
    class ReplicaPeer;
    class TargetCallback final : public TargetCallbackBase
    {
        friend ReplicaTarget;
    public:
        GM_CLASS_ALLOCATOR(TargetCallback);
        TargetCallback(AZ::u64 revision, AZ::u64 *replicaStamp)
            : m_revision(revision)
            , m_currentRevision(replicaStamp) {}

        AZ_FORCE_INLINE void operator()() override
        {
            AZ_Warning("GridMate", m_revision >= *m_currentRevision, "Cannot decrease Replica revision. Possible network re-ordering: %u<%u.", m_revision, m_currentRevision);

            if (m_revision > *m_currentRevision)
            {
                *m_currentRevision = m_revision;
            }
        }
    private:
        AZ::u64     m_revision;
        AZ::u64     *m_currentRevision;
    };
    /**
    *  ReplicaTarget: keeps replica's marshaling target (peer) and related meta data,
    *  Replica itself keeps an intrusive list of targets it needs to be forwarded to
    *  Peers keep all their associated replica targets as well
    *  Once target is removed from replica it is automatically removed from the corresponding peer and vice versa
    *  Once replica is destroyed - all its targets are automatically removed from peers, same goes for peers
    */
    class ReplicaTarget
    {
        friend class InterestManager;

    public:
        static ReplicaTarget* AddReplicaTarget(ReplicaPeer* peer, Replica* replica);

        void SetNew(bool isNew);
        bool IsNew() const;
        bool IsRemoved() const;

        // Returns ReplicaPeer associated with given replica
        ReplicaPeer* GetPeer() const;

        // Destroys current target. Target will be removed both from peer and replica
        void Destroy();

        // Create Callback
        AZStd::weak_ptr<TargetCallbackBase> CreateCallback(AZ::u64 revision)
        {
            AZ_Assert(IsAckEnabled(), "ACK disabled.");  //Shouldn't happen
            AZ_Assert(m_replicaRevision <= revision, "Cannot decrease replica revision");

            if(!m_callback || m_callback->m_revision != revision)
            {
                m_callback = AZStd::make_shared<TargetCallback>(revision, &m_replicaRevision);
            }
            //else, the version hasn't changed so re-use the callback

            return m_callback;
        }

        // Checks replica stamp
        AZ_FORCE_INLINE AZ::u64 GetRevision() const
        {
            return m_replicaRevision;
        }

        // Checks replica stamp
        AZ_FORCE_INLINE bool HasOldRevision(AZ::u64 newRevision) const
        {
            return m_replicaRevision < newRevision;
        }

        AZ_FORCE_INLINE static bool IsAckEnabled()
        {
            return k_enableAck;
        }

        // Intrusive hooks to keep this target node both in replica and peer
        AZStd::intrusive_list_node<ReplicaTarget> m_replicaHook;
        AZStd::intrusive_list_node<ReplicaTarget> m_peerHook;

    private:
        GM_CLASS_ALLOCATOR(ReplicaTarget);

        ReplicaTarget();
        ~ReplicaTarget();
        ReplicaTarget(const ReplicaTarget&) = delete;
        ReplicaTarget& operator=(const ReplicaTarget&) = delete;

        enum TargetStatus
        {
            TargetNone       =      0,
            TargetNew        = 1 << 0, // it's a newly added target
            TargetRemoved    = 1 << 1, // target was removed
        };

        ReplicaPeer*                        m_peer;             ///< Holds peer ptr for marshaling until marshaling is fully moved under peers,
                                                                ///  it is safe to keep raw ptr as this node will be auto-destroyed when its peer goes away
        AZ::u32                             m_flags;

        AZ::u32                             m_slotMask;

        static bool                         k_enableAck;
        AZStd::shared_ptr<TargetCallback>   m_callback;
        AZ::u64                             m_replicaRevision; ///< Last ACK'd replica stamp; 0 means NULL
    };


    /**
    *  Intrusive list for replica targets, destroys targets when cleared
    *  Cannot use AZStd::intrusive_list directly because it does not support auto-unlinking of nodes
    */
    template<class T, class Hook>
    class ReplicaTargetAutoDestroyList
        : private AZStd::intrusive_list<T, Hook>
    {
        typedef typename AZStd::intrusive_list<T, Hook> BaseListType;

    public:

        using BaseListType::begin;
        using BaseListType::end;
        using BaseListType::push_back;

        AZ_FORCE_INLINE ~ReplicaTargetAutoDestroyList()
        {
            clear();
        }

        AZ_FORCE_INLINE void clear()
        {
            while (BaseListType::begin() != BaseListType::end())
            {
                BaseListType::begin()->Destroy();
            }
        }
    };

    typedef ReplicaTargetAutoDestroyList<ReplicaTarget, AZStd::list_member_hook<ReplicaTarget, & ReplicaTarget::m_replicaHook> > ReplicaTargetList;
    typedef ReplicaTargetAutoDestroyList<ReplicaTarget, AZStd::list_member_hook<ReplicaTarget, & ReplicaTarget::m_peerHook> > PeerTargetList;
}

#endif // GM_REPLICA_REPLICATARGET_H
