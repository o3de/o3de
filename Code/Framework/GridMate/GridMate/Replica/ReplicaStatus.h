/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_REPLICA_STATUS_H
#define GM_REPLICA_STATUS_H

#include <GridMate/Replica/DataSet.h>
#include <GridMate/Replica/RemoteProcedureCall.h>
#include <GridMate/Replica/ReplicaChunk.h>
#include <GridMate/Replica/ReplicaStatusInterface.h>
#include <GridMate/String/string.h>
#include <GridMate/Serialize/ContainerMarshal.h>

namespace GridMate
{
    //-------------------------------------------------------------------------
    // ReplicaStatus - Replica management chunk
    //-------------------------------------------------------------------------
    class ReplicaStatus
        : public ReplicaChunkBase
    {
    public:
        typedef AZStd::intrusive_ptr<ReplicaStatus> Ptr;

        GM_CLASS_ALLOCATOR(ReplicaStatus);

        ReplicaStatus();

        static const char* GetChunkName() { return "GridMateReplicaStatus"; }
        static void RegisterType();

        void OnAttachedToReplica(Replica* replica) override;
        void OnDetachedFromReplica(Replica* replica) override;
        bool IsReplicaMigratable() override;

        const char* GetDebugName() const;
        void SetDebugName(const char* debugName);

        void SetUpstreamSuspended(bool isSuspended);
        bool IsUpstreamSuspended() const;

        //! Called on the originator node to request replica migration.
        Rpc<RpcArg<PeerId> >::BindInterface<ReplicaStatusInterface, & ReplicaStatusInterface::RequestOwnershipFn> RequestOwnership;
        
        //! Called by the primary to suspend upstream requests during replica migration.
        Rpc<RpcArg<PeerId>, RpcArg<AZ::u32> >::BindInterface<ReplicaStatusInterface, & ReplicaStatusInterface::MigrationSuspendUpstreamFn, RpcAuthoritativeTraits> MigrationSuspendUpstream;
        
        //! Called by the primary to signal downstream flush during replica migration.
        Rpc<RpcArg<PeerId>, RpcArg<AZ::u32> >::BindInterface<ReplicaStatusInterface, & ReplicaStatusInterface::MigrationRequestDownstreamAckFn, RpcAuthoritativeTraits> MigrationRequestDownstreamAck;

        struct ReplicaOptions
        {
            ReplicaOptions()
                : m_flags(0)
            {}

            AZ_FORCE_INLINE bool IsUpstreamSuspended() const { return !!(m_flags & ReplicaUpstreamSuspended); }
            AZ_FORCE_INLINE void SetUpstreamSuspended(bool isSuspended) { if (isSuspended) m_flags |= ReplicaUpstreamSuspended; else m_flags &= ~ReplicaUpstreamSuspended; }

            AZ_FORCE_INLINE bool HasDebugName() const { return !!(m_flags & ReplicaHasDebugName); }
            AZ_FORCE_INLINE void SetDebugName(const char* debugName) { m_flags |= ReplicaHasDebugName; m_replicaName = debugName; }
            AZ_FORCE_INLINE void UnsetDebugName() { m_flags &= ~ReplicaHasDebugName; m_replicaName.clear(); }

            AZ_FORCE_INLINE bool operator==(const ReplicaOptions& rhs) const
            {
                if (m_flags != rhs.m_flags)
                    return false;

                return !HasDebugName() || m_replicaName == rhs.m_replicaName;
            }

            enum ReplicaStatusFlags
            {
                ReplicaUpstreamSuspended   = 1 << 0,
                ReplicaHasDebugName        = 1 << 1
            };

            struct Marshaler
            {
                void Marshal(WriteBuffer& wb, const ReplicaOptions& value)
                {
                    wb.Write(value.m_flags);
                    if (value.HasDebugName())
                    {
                        wb.Write(value.m_replicaName);
                    }
                }

                void Unmarshal(ReplicaOptions& value, GridMate::ReadBuffer& rb)
                {
                    rb.Read(value.m_flags);
                    value.m_replicaName.clear();
                    if (value.HasDebugName())
                    {
                        rb.Read(value.m_replicaName);
                    }
                }
            };

            AZ::u8 m_flags;
            string m_replicaName;
        };

        DataSet<ReplicaOptions, ReplicaOptions::Marshaler> m_options; // Flags and debug info
        DataSet<AZ::u32> m_ownerSeq; // used to determine who is the most recent owner when we learn about proxies as it is being migrated.
    };
} // namespace GridMate

#endif // GM_REPLICA_STATUS_H
