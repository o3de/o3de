/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_REPLICA_RPC_H
#define GM_REPLICA_RPC_H

#include <AzCore/std/function/function_fwd.h>
#include <AzCore/Preprocessor/Sequences.h>
#include <AzCore/std/typetraits/is_base_of.h>
#include <AzCore/std/typetraits/remove_const.h>
#include <AzCore/std/typetraits/remove_reference.h>

#include <GridMate/Replica/Replica.h>
#include <GridMate/Replica/ReplicaChunk.h>
#include <GridMate/Replica/ReplicaChunkDescriptor.h>
#include <GridMate/Replica/ReplicaCommon.h>
#include <GridMate/Replica/ReplicaDefs.h>

#include <GridMate/Serialize/Buffer.h>
#include <GridMate/Serialize/DataMarshal.h>

namespace GridMate
{
    class ReplicaMarshalTaskBase;

    namespace Internal
    {
        struct RpcRequest;
        struct InterfaceResolver;
    }

    struct RpcDefaultTraits
    {
        static const bool s_isReliable = true;
        static const bool s_isPostAttached = true;
        static const bool s_alwaysForwardSourcePeer = false;
        static const bool s_allowNonAuthoritativeRequests = true;
        static const bool s_allowNonAuthoritativeRequestRelay = true;
    };

    struct RpcAuthoritativeTraits : public RpcDefaultTraits
    {
        static const bool s_isReliable = true;
        static const bool s_isPostAttached = true;
        static const bool s_allowNonAuthoritativeRequests = false;
    };

    struct RpcUnreliable
        : public RpcDefaultTraits
    {
        static const bool s_isReliable = false;
    };

    //--------------------------------------------------------------------------
    // RpcContext
    //--------------------------------------------------------------------------
    struct RpcContext
    {
        unsigned int m_realTime;
        unsigned int m_localTime;
        AZ::u32 m_timestamp;

        // The source peer is derived from the connection id, unless s_alwaysForwardSourcePeer is set for the RPC.
        // This will be the 'expected' peer for a direct connection, but if forwarding is involved between
        // peers, then the s_alwaysForwardSourcePeer should be set to ensure the source value is propagated
        // across the network.
        PeerId m_sourcePeer;

        RpcContext(unsigned int realTime = 0,
            unsigned int localTime = 0,
            unsigned int timestamp = 0,
            PeerId sourcePeer = InvalidReplicaPeerId)
            : m_realTime(realTime)
            , m_localTime(localTime)
            , m_timestamp(timestamp)
            , m_sourcePeer(sourcePeer)
        { }
    };
    //--------------------------------------------------------------------------

    /**
    * RPC argument container, used like Rpc<RpcArg<...>, RpcArg<...>>::BindInterface<Class, Method>...
    */
    struct RpcArgBase { };
    template<class T, class M = Marshaler<typename AZStd::remove_const<typename AZStd::remove_reference<T>::type>::type> >
    struct RpcArg
        : public RpcArgBase
    {
        typedef T Type;
        typedef M MarshalerType;
    };

    /**
    * RPC base class
    */
    class RpcBase
    {
        friend Replica;
        friend ReplicaChunkBase;
        friend ReplicaChunkDescriptor;
        friend ReplicaMarshalTaskBase;
        friend Internal::InterfaceResolver;

    protected:
        RpcBase(const char* debugName);
    public:
        virtual ~RpcBase() { }

    protected:
        virtual void Marshal(WriteBuffer& wb, Internal::RpcRequest* request) = 0;
        virtual Internal::RpcRequest* Unmarshal(ReadBuffer& rb) = 0;
        virtual bool Invoke(Internal::RpcRequest* rpc) const = 0;
        virtual bool IsPostAttached() const = 0;                //Requires Data Sets updated before executing RPC
        virtual bool IsAllowNonAuthoritativeRequests() const = 0;
        virtual bool IsAllowNonAuthoritativeRequestsRelay() const = 0;

        PeerId GetSourcePeerId();

        void Queue(Internal::RpcRequest* context);
        void OnRpcRequest(Internal::RpcRequest* rpc) const;
        void OnRpcInvoke(Internal::RpcRequest* rpc) const;

        ReplicaChunkBase* m_replicaChunk;
    };

    namespace Internal
    {
        // -- Internal RPC data ---------------------------
        struct RpcRequest
            : public RpcContext
        {
            GM_CLASS_ALLOCATOR(RpcRequest);
            bool m_authoritative;
            bool m_processed;
            bool m_relayed;
            bool m_reliable; // need to save reliability state as unreliable rpcs might be promoted to reliable (e.g. when unreliable rpc, called before reliable on the same frame)
            RpcBase* m_rpc;

            RpcRequest(RpcBase* rpc, unsigned int timestamp = 0, unsigned int realTime = 0, unsigned int localTime = 0, PeerId sourcePeer = InvalidReplicaPeerId)
                : RpcContext(realTime, localTime, timestamp, sourcePeer)
                , m_authoritative(false)
                , m_processed(false)
                , m_relayed(false)
                , m_reliable(true)
                , m_rpc(rpc)
            {
                AZ_Assert(m_rpc, "We require a valid RPCBase pointer!");
            }

            virtual ~RpcRequest() { }
        };
        // ------------------------------------------------

        struct InterfaceResolver
        {
            template<class C, class T>
            static C* GetInterface(T* object)
            {
                static_assert(AZStd::is_base_of<ReplicaChunkInterface, C>::value, "Class must inherit from ReplicaChunkInterface");
                AZ_Assert(object, "Invalid handler");
                AZ_Assert(object->m_replicaChunk, "Invalid replica chunk");
                return static_cast<C*>(object->m_replicaChunk->GetHandler());
            }
        };
    }

    namespace Internal
    {
        // -- Static integer sequence ---------------------
        // Used to unpack all values from a tuple at once
        template<size_t ...>
        struct StaticSequence { };

        template<size_t N, size_t ... Indices>
        struct GenerateStaticSequence
            : GenerateStaticSequence<N - 1, N - 1, Indices...> { };

        template<size_t ... Indices>
        struct GenerateStaticSequence<0, Indices...>
        {
            typedef StaticSequence<Indices...> Type;
        };
        // ------------------------------------------------

        // -- Convenience structures for extracting a type without wrapping it in RpcArg<T,M> ---------------------
        template<typename T, typename = void>
        struct RpcTypeExtraction;
        template<typename T>
        struct RpcTypeExtraction<T, typename AZStd::Utils::enable_if_c<AZStd::is_base_of<RpcArgBase, T>::value>::type>
        {
            using Type = typename T::Type;
            using MarshalerType = typename T::MarshalerType;
        };

        template<typename T>
        struct RpcTypeExtraction<T, typename AZStd::Utils::enable_if_c<!AZStd::is_base_of<RpcArgBase, T>::value>::type>
        {
            using Type = T;
            using MarshalerType = Marshaler<T>;
        };
        // ------------------------------------------------

        // -- Storage for Marshalers ---------------------
        template<class ... Args>
        struct VariadicMarshaler;

        template<>
        struct VariadicMarshaler<>
        {
            GM_CLASS_ALLOCATOR(VariadicMarshaler);

            VariadicMarshaler() { }
        };

        template<class This, class ... Rest>
        struct VariadicMarshaler<This, Rest...>
            : public VariadicMarshaler<Rest...>
        {
            GM_CLASS_ALLOCATOR(VariadicMarshaler);

            using ParentType = VariadicMarshaler<Rest...>;
            using MarshalerType = typename RpcTypeExtraction<This>::MarshalerType;

            VariadicMarshaler()
                : ParentType() { }

            template<typename ThisArg, typename ... RestArgs>
            VariadicMarshaler(ThisArg&& marshaler, RestArgs&& ... args)
                : ParentType(args ...)
                , m_marshaler(marshaler) { }

            MarshalerType m_marshaler;
        };
        // ------------------------------------------------

        // -- Storage for RPC -----------------------------
        template<class ... Args>
        struct VariadicStorage;

        template<>
        struct VariadicStorage<>
            : public Internal::RpcRequest
        {
            GM_CLASS_ALLOCATOR(VariadicStorage);

            using MarshalerTuple = VariadicMarshaler<>;
            using StaticArgSequence = Internal::GenerateStaticSequence<0>::Type;

            VariadicStorage(RpcBase* rpc)
                : RpcRequest(rpc) { }
            VariadicStorage(RpcBase* rpc, const RpcContext& ctx)
                : RpcRequest(rpc, ctx.m_timestamp, ctx.m_realTime, ctx.m_localTime, ctx.m_sourcePeer) { }

            bool Marshal(WriteBuffer&, const VariadicMarshaler<>&) const
            {
                return true;
            }
            bool Unmarshal(ReadBuffer&, const VariadicMarshaler<>&)
            {
                return true;
            }
        };

        template<class This, class ... Rest>
        struct VariadicStorage<This, Rest...>
            : public VariadicStorage<Rest...>
        {
            GM_CLASS_ALLOCATOR(VariadicStorage);

            using ParentType = VariadicStorage<Rest...>;
            using ExtractedType = typename RpcTypeExtraction<This>::Type;
            using ThisType = typename AZStd::remove_const<typename AZStd::remove_reference<ExtractedType>::type>::type;
            using MarshalerTuple = VariadicMarshaler<This, Rest...>;
            using StaticArgSequence = typename Internal::GenerateStaticSequence<sizeof ... (Rest) +1>::Type;

            VariadicStorage(RpcBase* rpc)
                : ParentType(rpc) { }

            template<typename ThisArg, typename ... RestArgs>
            VariadicStorage(RpcBase* rpc, const RpcContext& ctx, ThisArg&& val, RestArgs&& ... args)
                : ParentType(rpc, ctx, args ...)
                , m_val(val) { }

            void Marshal(WriteBuffer& wb, MarshalerTuple& marshaler)
            {
                ParentType::Marshal(wb, marshaler);

                wb.Write(m_val, marshaler.m_marshaler);
            }

            bool Unmarshal(ReadBuffer& rb, MarshalerTuple& marshaler)
            {
                if (!ParentType::Unmarshal(rb, marshaler))
                {
                    return false;
                }

                return rb.Read(m_val, marshaler.m_marshaler);
            }

            template<size_t Index, class ... Types>
            struct ExtractIndexContainerType;

            template<class First, class ... Types>
            struct ExtractIndexContainerType<0, First, Types...>
            {
                typedef VariadicStorage<First, Types...> ContainerType;
            };

            template<size_t Index, class First, class ... Types>
            struct ExtractIndexContainerType<Index, First, Types...>
                : ExtractIndexContainerType<Index - 1, Types...>
            { };

            template<size_t Index, class First, class ... Types>
            struct ExtractIndexReturnType
            {
                typedef typename ExtractIndexContainerType<Index, This, Rest...>::ContainerType ContainerType;
                typedef typename ContainerType::ThisType ThisType;
            };

            ThisType m_val;
        };
        // ------------------------------------------------

        // -- GetStorageFromIndex -------------------------
        template<size_t index, typename Storage, class ... Args>
        const typename Storage::template ExtractIndexReturnType<index, Args...>::ThisType & GetStorageFromIndex(const Storage * storage)
        {
            typedef typename Storage::template ExtractIndexContainerType<index, Args...>::ContainerType ContainerType;

            // Cast to Nth parent type to extract the contained value
            return static_cast<const ContainerType*>(storage)->m_val;
        }
        // ------------------------------------------------

        // -- Base binding class for common functionality --
        template<class Traits, class TypeTuple, class ForwardHandler>
        class RpcBindBase
            : public RpcBase
        {
        public:
            typedef typename TypeTuple::MarshalerTuple MarshalerSetType;

            template<typename ... LocalArgs>
            RpcBindBase(const char* debugName, LocalArgs&& ... marshalers)
                : RpcBase(debugName)
                , m_marshalers(AZStd::forward<LocalArgs>(marshalers) ...) { }

            virtual bool InvokeImpl(Internal::RpcRequest* request) const = 0;

            bool IsPostAttached() const override { return Traits::s_isPostAttached; }
            bool IsAllowNonAuthoritativeRequests() const override { return Traits::s_allowNonAuthoritativeRequests; }
            bool IsAllowNonAuthoritativeRequestsRelay() const override { return Traits::s_allowNonAuthoritativeRequestRelay; }

            template<typename ... LocalArgs>
            void operator()(LocalArgs&& ... args)
            {
                AZ_Assert(m_replicaChunk, "Cannot call an RPC that is not bound to a ReplicaChunk");
                Replica* replica = m_replicaChunk->GetReplica();
                ReplicaContext rc = replica ? replica->GetMyContext() : ReplicaContext(nullptr, TimeContext());
                PeerId sourcePeerId = GetSourcePeerId();
                bool shouldQueue = true;
                bool processed = false;
                bool isPrimary = m_replicaChunk->IsPrimary();
                if (isPrimary)
                {
                    // We are authoritative so execute the RPC immediately, forwarding the args along
                    RpcRequest localRequest(this, rc.m_realTime, rc.m_realTime, rc.m_localTime);
                    OnRpcRequest(&localRequest);
                    OnRpcInvoke(&localRequest);
                    bool isReplicaActive = replica && replica->IsActive(); // cache this because it could get changed during the RPC call.
                    bool isForward = static_cast<ForwardHandler*>(this)->Forward(RpcContext(rc.m_realTime, rc.m_localTime, rc.m_realTime, sourcePeerId), AZStd::forward<LocalArgs>(args) ...);
                    shouldQueue = isForward && isReplicaActive;
                    processed = true;
                }
                if (shouldQueue)
                {
                    TypeTuple* storage = aznew TypeTuple(this, RpcContext(rc.m_realTime, rc.m_realTime, rc.m_localTime, sourcePeerId), AZStd::forward<LocalArgs>(args) ...);
                    storage->m_authoritative = isPrimary;
                    storage->m_processed = processed;
                    storage->m_reliable = Traits::s_isReliable;
                    OnRpcRequest(storage);
                    Queue(storage);
                }
            }

            bool Invoke(Internal::RpcRequest* rpc) const override
            {
                OnRpcInvoke(rpc);
                return InvokeImpl(rpc);
            }

        protected:
            void Marshal(WriteBuffer& wb, Internal::RpcRequest* request) override
            {
                TypeTuple* storage = static_cast<TypeTuple*>(request);

                wb.Write(storage->m_timestamp);
                wb.Write(storage->m_authoritative);
                if (Traits::s_alwaysForwardSourcePeer)
                {
                    wb.Write(storage->m_sourcePeer);
                }

                // Pass the marshal onto the storage which unwraps the marshaling of each RPC value
                storage->Marshal(wb, m_marshalers);
            }

            Internal::RpcRequest* Unmarshal(ReadBuffer& rb) override
            {
                TypeTuple* storage = aznew TypeTuple(this);

                rb.Read(storage->m_timestamp);
                rb.Read(storage->m_authoritative);
                if (Traits::s_alwaysForwardSourcePeer)
                {
                    rb.Read(storage->m_sourcePeer);
                }

                // Pass the unmarshal onto the storage which unwraps the marshaling of each RPC value
                if (!storage->Unmarshal(rb, m_marshalers))
                {
                    delete storage;
                    return nullptr;
                }
                return storage;
            }

            MarshalerSetType m_marshalers; // Set of marshalers, one for each type specified in the RPC
        };
        // -----------------------------------------

        template<class ... Args>
        struct RpcBindArgsWrapper
        {
            /**
            * Bind a class method to the RPC
            */
            template<class Traits, class TypeTuple, class ThisResolver, class C, bool (C::* FuncPtr)(typename Args::Type..., const RpcContext&)>
            class BindInterface
                : public RpcBindBase<Traits, TypeTuple, BindInterface<Traits, TypeTuple, ThisResolver, C, FuncPtr> >
            {
            public:
                template<typename ... LocalArgs>
                BindInterface(const char* debugName, LocalArgs&& ... marshalers)
                    : RpcBindBase<Traits, TypeTuple, BindInterface<Traits, TypeTuple, ThisResolver, C, FuncPtr> >(debugName, AZStd::forward<LocalArgs>(marshalers) ...) { }

                // Invoke from storage
                bool InvokeImpl(Internal::RpcRequest* request) const override
                {
                    TypeTuple* storage = static_cast<TypeTuple*>(request);
                    return InvokeWithArgs(storage, typename TypeTuple::StaticArgSequence());
                }

                // Forward from a direct in-place call
                template<typename ... LocalArgs>
                bool Forward(const RpcContext& context, LocalArgs&& ... args)
                {
                    C* c = ThisResolver::template GetInterface<C>(this);
                    if (c)
                    {
                        return (*c.*FuncPtr)(AZStd::forward<LocalArgs>(args) ..., context);
                    }

                    return false;
                }

            protected:
                // Invoke and unwrap the args
                template<size_t ... Indices>
                bool InvokeWithArgs(TypeTuple* storage, Internal::StaticSequence<Indices...>) const
                {
                    C* c = ThisResolver::template GetInterface<C>(this);
                    if (c)
                    {
                        return (*c.*FuncPtr)(GetStorageFromIndex<Indices, TypeTuple, Args...>(storage) ..., *storage);
                    }
                     
                    return false;
                }
            };

            /**
            * Bind a class method to the RPC (0 args version)
            */
            template<class Traits, class TypeTuple, class ThisResolver, class C, bool (C::* FuncPtr)(const RpcContext&)>
            class BindInterface0
                : public RpcBindBase<Traits, TypeTuple, BindInterface0<Traits, TypeTuple, ThisResolver, C, FuncPtr> >
            {
            public:
                BindInterface0(const char* debugName)
                    : RpcBindBase<Traits, TypeTuple, BindInterface0<Traits, TypeTuple, ThisResolver, C, FuncPtr> >(debugName) { }

                // Invoke from storage
                bool InvokeImpl(Internal::RpcRequest* request) const override
                {
                    C* c = ThisResolver::template GetInterface<C>(this);
                    if (c)
                    {
                        TypeTuple* storage = static_cast<TypeTuple*>(request);
                        return (*c.*FuncPtr)(*storage);
                    }
                     
                    return false;
                }

                // Forward from a direct in-place call
                bool Forward(const RpcContext& context)
                {
                    C* c = ThisResolver::template GetInterface<C>(this);
                    if (c)
                    {
                        return (*c.*FuncPtr)(context);
                    }

                    return false;
                }
            };

            RpcBindArgsWrapper() = delete;
        };
    }

    /**
    * Public interface for declaring an RPC
    */
    template<typename ... Args>
    class Rpc;

    /**
    * 0 args specialisation
    */
    template<>
    class Rpc<>
    {
        typedef Internal::VariadicStorage<> TypeTuple;

    public:
        template<class C, bool (C::* FuncPtr)(const RpcContext&), class Traits = RpcDefaultTraits>
        using BindInterface = Internal::RpcBindArgsWrapper<>::BindInterface0<Traits, TypeTuple, Internal::InterfaceResolver, C, FuncPtr>;

        Rpc() = delete;
    };

    /**
    * Any args specialisation
    */
    template<typename ... Args>
    class Rpc
    {
        typedef Internal::VariadicStorage<Args...> TypeTuple;

    public:
        template<class C, bool (C::* FuncPtr)(typename Args::Type..., const RpcContext&), class Traits = RpcDefaultTraits>
        struct BindInterface
            : public Internal::RpcBindArgsWrapper<Args...>::template BindInterface<Traits, TypeTuple, Internal::InterfaceResolver, C, FuncPtr>
        {
            template<typename ... LocalArgs>
            BindInterface(const char* debugName, LocalArgs&& ... marshalers)
                : Internal::RpcBindArgsWrapper<Args...>::template BindInterface<Traits, TypeTuple, Internal::InterfaceResolver, C, FuncPtr>(debugName, AZStd::forward<LocalArgs>(marshalers) ...) { }
        };

        Rpc() = delete;
    };
} // namespace GridMate

#endif // GM_REPLICA_RPC_H
