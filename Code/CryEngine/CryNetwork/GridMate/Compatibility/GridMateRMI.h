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
#ifndef INCLUDE_GRIDMATERMI_HEADER
#define INCLUDE_GRIDMATERMI_HEADER

#pragma once

#include <INetwork.h>

#include "../NetworkGridMateCommon.h"
#include "GridMateNetSerialize.h"

#include <GridMate/Replica/Replica.h>

enum ERMInvocation
{
    eRMI_ToClientChannel        =    0x01,  // Send RMI from server to a specific client
    eRMI_ToOwningClient         =    0x04,  // Send RMI from server to client that owns the actor
    eRMI_ToOtherClients         =    0x08,  // Send RMI from server to all clients except the specified client
    eRMI_ToOtherRemoteClients   =    0x10,  // Send RMI from server to all remote clients except the specified client
    eRMI_ToAllClients           =    0x20,  // Send RMI from server to all clients

    eRMI_ToServer               =   0x100,  // Send RMI from client to server

    eRMI_NoLocalCalls           =  0x1000,  // For internal use only

    // IMPORTANT: Using the RMI shim through GridMate, do not exceed 16 bits or flags will be lost in transit.

    eRMI_ToRemoteClients = eRMI_NoLocalCalls | eRMI_ToAllClients,   // Send RMI from server to all remote clients

    // Mask aggregating all bits that require dispatching to non-server clients.
    eRMI_ClientsMask     =  eRMI_ToAllClients | eRMI_ToOtherClients | eRMI_ToOtherRemoteClients | eRMI_ToOwningClient | eRMI_ToClientChannel,
};

namespace GridMate
{
    namespace RMI
    {
        typedef uint16 WhereType;

        enum
        {
            kInvocationBufferBaseSize = 128
        };

        /*!
         * Flushes any RMIs invoked by the game this frame, by dispatching via RPCs.
         */
        void FlushQueue();

        /*!
         * Empties pending RMI queue, does not dispatch.
         */
        void EmptyQueue();

        /*!
         * Used to limit client RMIs to be callable from the host
         */
        struct ClientRMITraits : public GridMate::RpcDefaultTraits
        {
        };

        /*!
         * Base helper template for wrapping CryEngine RMI invocations.
         * This object is managed by a smart ptr to ensure the memory it owns
         * is properly resource-managed while its queued in GridMate as an RPC.
         */
        template<typename DerivedType>
        class InvocationWrapperBase
            : public _i_multithread_reference_target_t
        {
        public:

            typedef _smart_ptr<DerivedType> Ptr;
            WhereType       m_where;
            ChannelId       m_sentFromChannel;
            ChannelId       m_targetChannelFilter;  // contains channel to include or exclude, depending on WhereType

            typedef FlexibleBuffer<kInvocationBufferBaseSize> ParamsBuffer;
            ParamsBuffer    m_paramsBuffer;

            InvocationWrapperBase()
                : m_where(0)
                , m_sentFromChannel(kInvalidChannelId)
                , m_targetChannelFilter(kInvalidChannelId)
            {
            }

            InvocationWrapperBase(ChannelId sentFromChannel,
                ChannelId targetChannelFilter,
                WhereType where,
                const char* paramsBuffer,
                uint32 paramsBufferSize)
                : m_where(where)
                , m_sentFromChannel(sentFromChannel)
                , m_targetChannelFilter(targetChannelFilter)
                , m_paramsBuffer(paramsBuffer, paramsBufferSize)
            {
            }

            virtual ~InvocationWrapperBase() {}
        };

        /*!
         * Marshaler base class handling base class' members.
         */
        template<typename DerivedType>
        class InvocationWrapperMarshalerBase
        {
        public:

            static bool RequiresFromChannel(WhereType where)
            {
                return !!(where & (eRMI_ToOtherRemoteClients | eRMI_NoLocalCalls));
            }

            static bool RequiresTargetChannelFilter(WhereType where)
            {
                return !!(where & (eRMI_ToClientChannel | eRMI_ToOtherClients | eRMI_ToOtherRemoteClients));
            }

            void Marshal(GridMate::WriteBuffer& wb, const typename DerivedType::Ptr& value)
            {
                wb.Write(value->m_where);

                if (RequiresFromChannel(value->m_where))
                {
                    wb.Write(value->m_sentFromChannel);
                }

                if (RequiresTargetChannelFilter(value->m_where))
                {
                    wb.Write(value->m_targetChannelFilter);
                }

                typename DerivedType::ParamsBuffer::Marshaler().Marshal(wb, value->m_paramsBuffer);
            }

            void Unmarshal(typename DerivedType::Ptr& value, GridMate::ReadBuffer& rb)
            {
                DerivedType* invocation = new DerivedType();

                rb.Read(invocation->m_where);

                if (RequiresFromChannel(invocation->m_where))
                {
                    rb.Read(invocation->m_sentFromChannel);
                }

                if (RequiresTargetChannelFilter(invocation->m_where))
                {
                    rb.Read(invocation->m_targetChannelFilter);
                }

                typename DerivedType::ParamsBuffer::Marshaler().Unmarshal(invocation->m_paramsBuffer, rb);

                value = std::move(invocation);
            }
        };

        /*!
         * Wrapper for legacy (GameObject / GameObjectExtension) RMIs.
         */
        class LegacyInvocationWrapper
            : public InvocationWrapperBase < LegacyInvocationWrapper >
        {
        public:

            LegacyInvocationWrapper() {}
            LegacyInvocationWrapper(ChannelId sentFromChannel,
                uint32 repId,
                ChannelId targetChannelFilter,
                WhereType where,
                const char* paramsBuffer,
                uint32 paramsBufferSize)
                : InvocationWrapperBase(sentFromChannel, targetChannelFilter, where, paramsBuffer, paramsBufferSize)
                , m_repId(repId)
            {
            }

            uint32 m_repId;

            class Marshaler
                : public InvocationWrapperMarshalerBase < LegacyInvocationWrapper >
            {
            public:

                typedef InvocationWrapperMarshalerBase<LegacyInvocationWrapper> Super;

                void Marshal(GridMate::WriteBuffer& wb, const LegacyInvocationWrapper::Ptr& value)
                {
                    Super::Marshal(wb, value);
                    wb.Write(value->m_repId);
                }

                void Unmarshal(LegacyInvocationWrapper::Ptr& value, GridMate::ReadBuffer& rb)
                {
                    Super::Unmarshal(value, rb);
                    rb.Read(value->m_repId);
                }
            };
        };

        /*!
         * Wrapper for actor (GameCore / RPGSample) RMIs.
         */
        class ActorInvocationWrapper
            : public InvocationWrapperBase < ActorInvocationWrapper >
        {
        public:

            ActorInvocationWrapper() {}
            ActorInvocationWrapper(ChannelId sentFromChannel,
                uint8 actorExtensionId,
                uint32 repId,
                ChannelId targetChannelFilter,
                WhereType where,
                const char* paramsBuffer,
                uint32 paramsBufferSize)
                : InvocationWrapperBase(sentFromChannel, targetChannelFilter, where, paramsBuffer, paramsBufferSize)
                , m_repId(repId)
                , m_actorExtensionId(actorExtensionId)
            {
            }

            uint32 m_repId;
            uint8 m_actorExtensionId;

            class Marshaler
                : public InvocationWrapperMarshalerBase < ActorInvocationWrapper >
            {
            public:

                typedef InvocationWrapperMarshalerBase<ActorInvocationWrapper> Super;

                void Marshal(GridMate::WriteBuffer& wb, const ActorInvocationWrapper::Ptr& value)
                {
                    // \todo: Investigate reduction of repId size, or combining to reduce per-RMI overhead.
                    Super::Marshal(wb, value);
                    wb.Write(value->m_repId);
                    wb.Write(value->m_actorExtensionId);
                }

                void Unmarshal(ActorInvocationWrapper::Ptr& value, GridMate::ReadBuffer& rb)
                {
                    Super::Unmarshal(value, rb);
                    rb.Read(value->m_repId);
                    rb.Read(value->m_actorExtensionId);
                }
            };
        };

        /*!
         * Wrapper for lua script entity RMIs.
         */
        class ScriptInvocationWrapper
            : public _i_reference_target_t
        {
        public:

            typedef _smart_ptr<ScriptInvocationWrapper> Ptr;

            ScriptInvocationWrapper() {}
            ScriptInvocationWrapper(bool isServerRMI,
                ChannelId toChannelId,
                ChannelId avoidChannelId,
                const char* serializedData,
                size_t serializedDataSize)
                : m_toChannelId(toChannelId)
                , m_avoidChannelId(avoidChannelId)
                , m_isServerRMI(isServerRMI)
                , m_serializedData(serializedData, serializedDataSize)
            {
            }

            ~ScriptInvocationWrapper() {}

            ChannelId m_toChannelId;
            ChannelId m_avoidChannelId;
            bool m_isServerRMI;

            typedef FlexibleBuffer<128> DataBuffer;
            DataBuffer m_serializedData;

            class Marshaler
            {
            public:

                void Marshal(GridMate::WriteBuffer& wb, const ScriptInvocationWrapper::Ptr& value)
                {
                    wb.Write(value->m_toChannelId);
                    wb.Write(value->m_avoidChannelId);
                    DataBuffer::Marshaler().Marshal(wb, value->m_serializedData);
                }

                void Unmarshal(ScriptInvocationWrapper::Ptr& value, GridMate::ReadBuffer& rb)
                {
                    ScriptInvocationWrapper* invocation = new ScriptInvocationWrapper();

                    rb.Read(invocation->m_toChannelId);
                    rb.Read(invocation->m_avoidChannelId);
                    DataBuffer::Marshaler().Unmarshal(invocation->m_serializedData, rb);
                    value = std::move(invocation);
                }
            };
        };

        //! Handles invocation for actor (GameCore) RMIs.
        void InvokeActor(EntityId entityId, uint8 actorExtensionId, ChannelId targetChannelFilter, IActorRMIRep& rep);

        //! Handles invocation for lua script entity RMIs.
        void InvokeScript(ISerializable* serializable, bool isServerRMI, ChannelId toChannelId, ChannelId avoidChannelId);

        //! Handles deciphering and dispatching of legacy GameObject RMIs.
        bool HandleLegacy(EntityId entityId, LegacyInvocationWrapper::Ptr invocation, const GridMate::RpcContext& rc);

        //! Handles deciphering and dispatching of actor (GameCore) RMIs.
        bool HandleActor(EntityId entityId, ActorInvocationWrapper::Ptr invocation, const GridMate::RpcContext& rc);

        //! Handles deciphering and dispatching of lua script entity RMIs.
        bool HandleScript(ScriptInvocationWrapper::Ptr invocation, const GridMate::RpcContext& rc);

        //! Register/Unregister an actor RMI sink for dispatching upon receipt.
        void RegisterActorRMI(IActorRMIRep* rep);
        void UnregisterActorRMI(IActorRMIRep* rep);

        IActorRMIRep* FindActorRMIRep(uint32 repId);

        //! Convenience typedefs for RMI param serializers.
        typedef NetSerialize::EntityNetSerializerCollectState RMIParamsSerializerStoreParams;
        typedef NetSerialize::EntityNetSerializerDispatchState RMIParamsSerializerUnwindParams;
    } // namespace RMI
} // namespace GridMate

#endif // INCLUDE_GRIDMATERMI_HEADER
