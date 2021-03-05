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

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzFramework/Network/NetSystemBus.h>
#include <AzFramework/Network/NetBindable.h>

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/typetraits/is_base_of.h>
#include <AzCore/std/functional.h>

namespace AzFramework
{
    class NetBindable;

    using GridMate::ReplicaChunkInterface;
    using GridMate::ReplicaChunkBase;
    using GridMate::ReplicaChunk;
    using GridMate::ReplicaChunkDescriptor;
    using GridMate::DefaultReplicaChunkDescriptor;
    using GridMate::ReplicaChunkDescriptorTable;
    using GridMate::ReplicaChunkClassId;
    using GridMate::ReplicaChunkPtr;
    using GridMate::Rpc;
    using GridMate::ZoneMask;
    using GridMate::ZoneMask_All;
    using GridMate::UnmarshalContext;
    using GridMate::ReadBuffer;
    using GridMate::WriteBuffer;
    using GridMate::WriteBufferDynamic;

    ///////////////////////////////////////////////////////////////////////////
    // GridMate ReplicaChunk/ReplicaChunkDescriptors
    ///////////////////////////////////////////////////////////////////////////
    class ReflectedReplicaChunkBase
        : public ReplicaChunkBase
        , public ReplicaChunkInterface
    {
        friend NetworkContext;
    public:
        ReflectedReplicaChunkBase();
        bool IsReplicaMigratable() override { return true; }

        /// Returns the chunk type name, e.g. "ReflectedReplicaChunk<MyClass>"
        virtual const char* GetName() const = 0;
        /// Returns the linear size of the chunk including DataSets and RPCs
        virtual size_t GetSize() const = 0;
        /// Returns a pointer to the start of the DataSet/RPC storage allocated with the chunk
        virtual AZ::u8* GetDataStart() const = 0;
        /// Binds an instance of the reflected class to this chunk
        virtual void Bind(NetBindable* instance, NetworkContextBindMode mode) = 0;
        /// Removes network bindings from the bound NetBindable
        virtual void Unbind() = 0;

        WriteBufferDynamic   m_ctorBuffer; ///< Buffer to hold ctor data before the chunk is bound
    };

    /// This will be the header for a blob in memory:
    /// The layout looks like:
    /// * ReflectedReplicaChunk<T>
    /// * DataSets
    /// * RPCs
    template <class ClassType>
    class ReflectedReplicaChunk
        : public ReflectedReplicaChunkBase
    {
        friend NetworkContext;
    public:
        static const char* GetChunkName();
        static size_t GetChunkSize();

    public:
        AZ_CLASS_ALLOCATOR(ReflectedReplicaChunk, AZ::SystemAllocator, 0);
        ReflectedReplicaChunk()
            : m_dataSets(reinterpret_cast<AZ::u8*>(this) + sizeof(*this))
        {
        }

        const char* GetName() const override { return GetChunkName(); }
        size_t GetSize() const override { return GetChunkSize(); }
        AZ::u8* GetDataStart() const override { return const_cast<AZ::u8*>(m_dataSets); }
        void Bind(NetBindable* instance, NetworkContextBindMode mode) override;
        void Unbind() override;

    private:
        const AZ::u8*           m_dataSets; ///< Points to the beginning of the datasets for this chunk
    };

    class NetworkContextChunkDescriptor
        : public ReplicaChunkDescriptor
    {
    public:
        NetworkContextChunkDescriptor(const char* name, size_t size, const AZ::Uuid& typeId = AZ::Uuid());

        ReplicaChunkBase* CreateFromStream(UnmarshalContext& ctx) override;
        void DeleteReplicaChunk(ReplicaChunkBase* chunkInstance) override;
        void DiscardCtorStream(UnmarshalContext&) override;
        void MarshalCtorData(ReplicaChunkBase*, WriteBuffer&) override;

        void Bind(const AZ::Uuid& typeId) { m_typeId = typeId; }
        virtual bool IsAuto() const { return false; }

    private:
        AZ::Uuid m_typeId; ///< TypeId of the class this descriptor represents (not the chunk type)
    };

    template <class ClassType, ZoneMask mask = ZoneMask_All>
    class AutoChunkDescriptor
        : public NetworkContextChunkDescriptor
    {
    public:
        AutoChunkDescriptor()
            : NetworkContextChunkDescriptor(ReflectedReplicaChunk<ClassType>::GetChunkName(), ReflectedReplicaChunk<ClassType>::GetChunkSize(), AZ::RttiTypeId<ClassType>())
        {
        }

        ZoneMask GetZoneMask() const override { return mask; }

        bool IsAuto() const override { return true; }
    };

    template <class ChunkType, ZoneMask mask = ZoneMask_All>
    class ExternalChunkDescriptor
        : public NetworkContextChunkDescriptor
    {
    public:
        ExternalChunkDescriptor()
            : NetworkContextChunkDescriptor(ChunkType::GetChunkName(), sizeof(ChunkType))
        {}

        ZoneMask GetZoneMask() const override { return mask; }
    };

    ///////////////////////////////////////////////////////////////////////////
    /// NetworkContext can be used to reflect classes for network serialization
    ///  It will automatically generate ReplicaChunks and bind them to instances
    ///  when requested. It also serves as a binding registry for binding a class
    ///  to the ReplicaChunk that should be used to replicate it.
    ///////////////////////////////////////////////////////////////////////////
    class NetworkContext
        : public AZ::ReflectContext
    {
    public:
        /// @cond EXCLUDE_DOCS
        class ClassBuilder;
        class ClassDesc;
        using ClassDescPtr = AZStd::intrusive_ptr<ClassDesc>;
        using ClassBuilderPtr = AZStd::intrusive_ptr<ClassBuilder>;
        using ClassBindings = AZStd::unordered_map<AZ::Uuid, ClassDescPtr>;
        using ChunkBindings = AZStd::unordered_map<ReplicaChunkClassId, ClassDescPtr>;
        using ClassInfo = ClassBuilder; ///< @deprecated Use NetworkContext::ClassBuilder
        using ClassInfoPtr = ClassBuilderPtr; ///< @deprecated Use NetworkContext::ClassBuilderPtr
        /// @endcond

        class IntrusiveRefCounted
        {
        public:
            virtual ~IntrusiveRefCounted() {}
        private:
            // refcount
            template<class T>
            friend struct AZStd::IntrusivePtrCountPolicy;
            mutable unsigned int m_refCount = 0;
            AZ_FORCE_INLINE void add_ref() { ++m_refCount; }
            AZ_FORCE_INLINE void release()
            {
                AZ_Assert(m_refCount > 0, "Reference count logic error, trying to remove reference when refcount is 0");
                if (--m_refCount == 0)
                {
                    delete this;
                }
            }
        };

        /**
         * Interface for recording classes, chunks, and datasets
         * When destructed at the end of reflection, it will register/unregister the ChunkDescriptor
         */
        class ClassBuilder
            : public IntrusiveRefCounted
        {
            friend class NetworkContext;

        protected:
            AZ_CLASS_ALLOCATOR(ClassBuilder, AZ::SystemAllocator, 0);
            ClassBuilder(NetworkContext* context, ClassDescPtr binding);

        public:
            ~ClassBuilder();
            ClassBuilderPtr operator->() { return this; }

            /// Bind a ReplicaChunk type to this class for network serialization
            template <class ChunkType, typename DescriptorType = ExternalChunkDescriptor<ChunkType> >
            ClassBuilderPtr Chunk();

            /// Bind a NetBindable's Field
            template <class ClassType, typename FieldType>
            typename AZStd::enable_if<AZStd::is_base_of<NetBindableFieldBase, FieldType>::value, ClassBuilderPtr>::type
            Field(const char* name, FieldType ClassType::* address);

            /// Declare an external chunk's DataSet
            template <class ClassType, typename DataSetType>
            typename AZStd::enable_if<AZStd::is_base_of<DataSetBase, DataSetType>::value, ClassBuilderPtr>::type
            Field(const char* name, DataSetType ClassType::* address);

            /// Bind an Rpc::BindInterface for this chunk
            template <class ClassType,           // class this RPC is part of
                class InterfaceType = ClassType,   // class implementing the RPC, must derive from ReplicaChunkInterface
                typename ... Args,
                class Traits = RpcDefaultTraits,
                typename RpcBindType = typename Rpc<Args...>::template BindInterface<InterfaceType, bool (InterfaceType::*)(typename Args::Type..., const RpcContext&), Traits> >
            typename AZStd::enable_if<AZStd::is_base_of<RpcBase, RpcBindType>::value, ClassBuilderPtr>::type
            RPC(const char* name, RpcBindType ClassType::* rpc);

            /// Bind a NetBindable::Rpc for this NetBindable
            template <class ClassType,
                class InterfaceType = ClassType,
                typename ... Args,
                class Traits = RpcDefaultTraits,
                typename RpcBindType = typename NetBindable::Rpc<Args...>::template Bind<InterfaceType, bool (InterfaceType::*)(Args..., const RpcContext&), Traits> >
            typename AZStd::enable_if<AZStd::is_base_of<NetBindableRpcBase, RpcBindType>::value, ClassBuilderPtr>::type
            RPC(const char* name, RpcBindType ClassType::* rpc);

#define CTOR_DATA_OVERLOAD(_getsig, _setsig)                                                                   \
    template <class ClassType, class DataType, typename MarshalerType = Marshaler<DataType> >                  \
    ClassBuilderPtr CtorData(const char* name, _getsig, _setsig, const MarshalerType&marshaler = MarshalerType()) \
    {                                                                                                          \
        return CtorDataImpl<ClassType, DataType>(name, getter, setter, marshaler);                             \
    }

            /// Bind a getter/setter pair for data required during object construction
            // this has to be done via overload so that the user does not have to explicitly provide
            // the template arguments, they can be divined from the function call
            CTOR_DATA_OVERLOAD(DataType(ClassType::* getter)(), void (ClassType::* setter)(const DataType&));
            CTOR_DATA_OVERLOAD(DataType(ClassType::* getter)() const, void (ClassType::* setter)(const DataType&));
            CTOR_DATA_OVERLOAD(DataType & (ClassType::* getter)(), void (ClassType::* setter)(const DataType&));
            CTOR_DATA_OVERLOAD(DataType & (ClassType::* getter)() const, void (ClassType::* setter)(const DataType&));
            CTOR_DATA_OVERLOAD(const DataType&(ClassType::* getter)(), void (ClassType::* setter)(const DataType&));
            CTOR_DATA_OVERLOAD(const DataType&(ClassType::* getter)() const, void (ClassType::* setter)(const DataType&));
            CTOR_DATA_OVERLOAD(DataType(ClassType::* getter)(), void (ClassType::* setter)(DataType));
            CTOR_DATA_OVERLOAD(DataType(ClassType::* getter)() const, void (ClassType::* setter)(DataType));
            CTOR_DATA_OVERLOAD(DataType & (ClassType::* getter)(), void (ClassType::* setter)(DataType));
            CTOR_DATA_OVERLOAD(DataType & (ClassType::* getter)() const, void (ClassType::* setter)(DataType));
            CTOR_DATA_OVERLOAD(const DataType&(ClassType::* getter)(), void (ClassType::* setter)(DataType));
            CTOR_DATA_OVERLOAD(const DataType&(ClassType::* getter)() const, void (ClassType::* setter)(DataType));
#undef CTOR_DATA_OVERLOAD

        private:
            template <class ClassType,
                class DataType,
                class GetterFunction,
                class SetterFunction,
                typename MarshalerType = Marshaler<DataType> >
            ClassBuilderPtr CtorDataImpl(const char* name, GetterFunction getter, SetterFunction setter, const MarshalerType& marshaler = MarshalerType());

        private:
            ClassDescPtr m_binding;
            NetworkContext* m_context;
        };

        class DescBase
            : public IntrusiveRefCounted
        {
            friend class NetworkContext;
        public:
            AZ_CLASS_ALLOCATOR(DescBase, AZ::SystemAllocator, 0);
            DescBase(const char* name, ptrdiff_t offset);
            virtual ~DescBase() {}

            const char* GetName()   const { return m_name; }
            ptrdiff_t   GetOffset() const { return m_offset; }
        protected:
            const char* m_name;   ///< Field name, will be used as DataSet debug name
            ptrdiff_t   m_offset; ///< Offset from an instance pointer (a ReplicaChunk or the actual class instance)
        };

        class FieldDescBase
            : public DescBase
        {
            friend class NetworkContext;

        public:
            AZ_CLASS_ALLOCATOR(FieldDescBase, AZ::SystemAllocator, 0);
            FieldDescBase(const char* name, ptrdiff_t offset);
            virtual ~FieldDescBase() {}

            virtual void ConstructDataSet(void*) const = 0;
            virtual void DestructDataSet(void*) const = 0;
            virtual size_t GetDataSetSize() const = 0;

            size_t GetDataSetIndex() const { return m_dataSetIdx; }

        protected:
            size_t m_dataSetIdx;
        };

        /**
        * Represents a DataSet in a chunk or class
        * NOTE: m_offset in this class is the offset from ReplicaChunk* -> DataSet
        */
        template <typename DataSetType>
        class DataSetDesc
            : public FieldDescBase
        {
        public:
            AZ_CLASS_ALLOCATOR(DataSetDesc, AZ::SystemAllocator, 0);
            DataSetDesc(const char* name, ptrdiff_t offset);

            void ConstructDataSet(void*) const override {}
            void DestructDataSet(void*) const override {}
            size_t GetDataSetSize() const override { return sizeof(DataSetType); }
        };

        /**
        * Represents a field in a chunk, responsible for creating a DataSet<T, Marshaler, Throttler>
        * that represents the field
        * NOTE: m_offset in this class is the offset from NetBindable* -> NetBindable::Field
        */
        template <typename FieldType>
        class NetBindableFieldDesc
            : public FieldDescBase
        {
        public:
            using DataSetType = typename FieldType::DataSetType;

        public:
            AZ_CLASS_ALLOCATOR(NetBindableFieldDesc, AZ::SystemAllocator, 0);
            NetBindableFieldDesc(const char* name, ptrdiff_t offset);

            void ConstructDataSet(void* mem) const override { FieldType::ConstructDataSet(mem, m_name); }
            void DestructDataSet(void* mem) const override { FieldType::DestructDataSet(mem); }
            size_t GetDataSetSize() const override { return sizeof(DataSetType); }
        };

        class RpcDescBase
            : public DescBase
        {
            friend class NetworkContext;
        public:
            AZ_CLASS_ALLOCATOR(RpcDescBase, AZ::SystemAllocator, 0);
            RpcDescBase(const char* name, ptrdiff_t offset);
            virtual ~RpcDescBase() {}

            virtual void ConstructRpc(void*) const {}
            virtual void DestructRpc(void*) const {}
            virtual size_t GetRpcSize() const { return 0; }

            size_t GetRpcIndex() const { return m_rpcIdx; }

        protected:
            size_t m_rpcIdx;
        };

        template <typename RpcBindType>
        class NetBindableRpcDesc
            : public RpcDescBase
        {
            friend class NetworkContext;
        public:
            AZ_CLASS_ALLOCATOR(NetBindableRpcDesc, AZ::SystemAllocator, 0);
            NetBindableRpcDesc(const char* name, ptrdiff_t offset)
                : RpcDescBase(name, offset)
            {
                static_assert((AZStd::is_base_of<NetBindableRpcBase, RpcBindType>::value), "NetBindableRpcDesc is intended for use only with NetBindableRpcs");
            }

            void ConstructRpc(void* mem) const override { RpcBindType::ConstructRpc(mem, m_name); }
            void DestructRpc(void* mem) const override { RpcBindType::DestructRpc(mem); }
            size_t GetRpcSize() const override { return sizeof(typename RpcBindType::BindInterfaceType); }
        };

        class CtorDataBase
            : public IntrusiveRefCounted
        {
        public:
            AZ_CLASS_ALLOCATOR(CtorDataBase, AZ::SystemAllocator, 0);
            CtorDataBase(const char* name);
            virtual ~CtorDataBase() {}

            virtual void Marshal(NetBindable* netBindable, WriteBuffer& buffer) const = 0;
            virtual void Unmarshal(ReadBuffer& buffer, NetBindable* netBindable) const = 0;
            virtual void Copy(ReadBuffer& src, WriteBuffer& dest) const = 0;

        protected:
            const char* m_name;
        };

        template <class ClassType, class DataType, typename MarshalerType>
        class CtorDataDesc
            : public CtorDataBase
        {
            using GetterFunction = AZStd::function<DataType(ClassType*)>;
            using SetterFunction = AZStd::function<void (ClassType*, const DataType&)>;
        public:
            AZ_CLASS_ALLOCATOR(CtorDataDesc, AZ::SystemAllocator, 0);
            CtorDataDesc(const char* name, GetterFunction get, SetterFunction set)
                : CtorDataBase(name)
                , m_get(get)
                , m_set(set)
            {}

            CtorDataDesc(const char* name, DataType(ClassType::* getter)(), void (ClassType::* setter)(const DataType&))
                : CtorDataBase(name)
                , m_get(AZStd::bind(getter, AZStd::placeholders::_1))
                , m_set(AZStd::bind(setter, AZStd::placeholders::_1, AZStd::placeholders::_2))
            {}

            void Marshal(NetBindable* netBindable, WriteBuffer& buffer) const override;
            void Unmarshal(ReadBuffer& buffer, NetBindable* netBindable) const override;
            virtual void Copy(ReadBuffer& src, WriteBuffer& dest) const override;

            GetterFunction  m_get;
            SetterFunction  m_set;
            MarshalerType   m_marshaler;
        };

        struct ChunkDesc
        {
        public:
            using Fields = AZStd::vector<AZStd::intrusive_ptr<FieldDescBase> >;
            using Rpcs = AZStd::vector<AZStd::intrusive_ptr<RpcDescBase> >;
            using Ctors = AZStd::vector<AZStd::intrusive_ptr<CtorDataBase> >;

            const char*         m_name = nullptr;   ///< The name of the chunk
            ReplicaChunkClassId m_chunkId;          ///< The registered id of the ReplicaChunk this class will use
            Fields              m_fields;           ///< list of data fields in the ReplicaChunk
            Rpcs                m_rpcs;             ///< list of RPCs in the ReplicaChunk
            Ctors               m_ctors;            ///< list of ctor callbacks to gather/apply ctor data
            bool                m_external = false; ///< If true, this chunk is separate from the class bound to it
        };

        /**
         * Contains the chunk factory and field descriptions for a given class
         */
        class ClassDesc
            : public IntrusiveRefCounted
        {
        public:

            AZ_CLASS_ALLOCATOR(ClassDesc, AZ::SystemAllocator, 0);
            ClassDesc(const char* name = nullptr, const AZ::Uuid& typeId = AZ::Uuid());

        public:
            const char*         m_name;    ///< The name of the class that is bound
            AZ::Uuid            m_typeId;  ///< The type that this binding represents (null for chunks)
            ChunkDesc           m_chunkDesc;   ///< Descriptor for the chunk for this type

            /// Functor which will register the ReplicaChunkDescriptor with the global registry
            AZStd::function<bool()> RegisterChunkType;
            /// Functor to unregister the ReplicaChunkDescriptor (during reflection removal)
            AZStd::function<void()> UnregisterChunkType;
            /// Functor which will create a ReplicaChunk and bind it to the given instance
            AZStd::function<ReplicaChunkBase*()> CreateReplicaChunk;
            /// Functor which can destroy a ReplicaChunk and free its memory
            AZStd::function<void(ReplicaChunkBase*)> DestroyReplicaChunk;
            /// Functor which binds an instance of this class to its RPCs for local dispatch
            AZStd::function<void(NetBindable* bindable)> BindRpcs;
        };

        AZ_CLASS_ALLOCATOR(NetworkContext, AZ::SystemAllocator, 0);
        AZ_RTTI(NetworkContext, "{B1172D4A-EA1B-441D-AAE6-A9933DAECA8A}", AZ::ReflectContext);

        NetworkContext();
        virtual ~NetworkContext();

        /// Register a class with the NetworkContext for replication
        template <class ClassType>
        ClassBuilderPtr Class();

        /// Create a replica chunk for a given class
        ReplicaChunkBase* CreateReplicaChunk(const AZ::Uuid& typeId);

        /// Create a replica chunk for a given class, template version
        template <class ClassType>
        ReplicaChunkBase* CreateReplicaChunk();

        /// Destroy a replica chunk for a given class
        void DestroyReplicaChunk(ReplicaChunkBase * chunk);

        /// Bind an instance and a chunk to each other
        void Bind(NetBindable * instance, ReplicaChunkPtr chunk, NetworkContextBindMode mode);

        /// Returns whether or not a given type uses a reflected (automatic) ReplicaChunk
        bool UsesSelfAsChunk(const AZ::Uuid & typeId) const;

        /// Returns whether or not a given type uses a custom ReplicaChunk
        bool UsesExternalChunk(const AZ::Uuid & typeId) const;

        /// Return the size of the the chunk which will represent the given type
        size_t GetReflectedChunkSize(const AZ::Uuid & typeId) const;

        using FieldVisitor = AZStd::function<void(FieldDescBase*)>;
        void EnumerateFields(const ReplicaChunkClassId&chunkId, FieldVisitor visitor) const;
        void EnumerateFields(const AZ::Uuid & typeId, FieldVisitor visitor) const;

        using RpcVisitor = AZStd::function<void(RpcDescBase*)>;
        void EnumerateRpcs(const ReplicaChunkClassId&chunkId, RpcVisitor visitor) const;
        void EnumerateRpcs(const AZ::Uuid & typeId, RpcVisitor visitor) const;

        using CtorVisitor = AZStd::function<void(CtorDataBase*)>;
        void EnumerateCtorData(const ReplicaChunkClassId&chunkId, CtorVisitor visitor) const;
        void EnumerateCtorData(const AZ::Uuid & typeId, CtorVisitor visitor) const;

    private:
        template <class ClassType>
        void InitReflectedChunkBinding(ClassDescPtr binding);

        template <class ChunkType, typename DescriptorType = ExternalChunkDescriptor<ChunkType> >
        void InitExternalChunkBinding(ClassDescPtr binding);

    private:
        ClassBindings m_classBindings;
        ChunkBindings m_chunkBindings;
    };

    ///////////////////////////////////////////////////////////////////////////
    template <class ClassType>
    NetworkContext::ClassBuilderPtr NetworkContext::Class()
    {
        static_assert((AZStd::is_base_of<NetBindable, ClassType>::value), "Classes reflected through NetworkContext must be derived from NetBindable");
        const AZ::Uuid& typeId = AZ::AzTypeInfo<ClassType>::Uuid();
        ClassDescPtr binding = nullptr;
        if (IsRemovingReflection()) // Just remove the entire class definition
        {
            auto it = m_classBindings.find(typeId);
            if (it != m_classBindings.end())
            {
                binding = it->second;
                m_chunkBindings.erase(binding->m_chunkDesc.m_chunkId);
                m_classBindings.erase(it);
            }
        }
        else
        {
            auto ret = m_classBindings.insert_key(typeId);
            AZ_Assert(ret.second, "Cannot register more than one type with the same Uuid in the NetworkContext");
            binding = ret.first->second = aznew ClassDesc(AZ::AzTypeInfo<ClassType>::Name(), AZ::AzTypeInfo<ClassType>::Uuid());
        }

        return aznew ClassBuilder(this, binding);
    }

    template <class ClassType>
    void NetworkContext::InitReflectedChunkBinding(ClassDescPtr binding)
    {
        if (!binding->RegisterChunkType)
        {
            binding->m_chunkDesc.m_name = ReflectedReplicaChunk<ClassType>::GetChunkName();
            ReplicaChunkClassId chunkClassId = ReplicaChunkClassId(binding->m_chunkDesc.m_name);
            m_chunkBindings[chunkClassId] = binding;
            NetworkContext* netContext = this;

            binding->RegisterChunkType = [chunkClassId, netContext]()
                {
                    bool result = ReplicaChunkDescriptorTable::Get().RegisterChunkType<ReflectedReplicaChunk<ClassType>, AutoChunkDescriptor<ClassType> >();
                    ReplicaChunkDescriptor* desc = ReplicaChunkDescriptorTable::Get().FindReplicaChunkDescriptor(chunkClassId);
                    ReplicaChunkDescriptorTable::Get().BeginConstructReplicaChunk(desc);
                    // The offset recorded in NetBindableFields is the offset in the NetBindable
                    // We must compute the offset of the generated DataSets here and record the
                    // index from the descriptor
                    ptrdiff_t offset = sizeof(ReflectedReplicaChunk<ClassType>); // data sets are right after the ReflectedReplicaChunk<> in memory
                    netContext->EnumerateFields(chunkClassId,
                        [desc, &offset](FieldDescBase* field)
                        {
                            desc->RegisterDataSet(field->m_name, offset);
                            field->m_dataSetIdx = desc->GetDataSetIndex(offset);
                            offset += field->GetDataSetSize();
                        });
                    netContext->EnumerateRpcs(chunkClassId,
                        [desc, &offset](RpcDescBase* rpc)
                        {
                            desc->RegisterRPC(rpc->m_name, offset);
                            rpc->m_rpcIdx = desc->GetRpcIndex(offset);
                            offset += rpc->GetRpcSize();
                        });
                    AZ_Assert(offset == static_cast<ptrdiff_t>(ReflectedReplicaChunk<ClassType>::GetChunkSize()), "Overflow/underflow while registering DataSets for %s", ReflectedReplicaChunk<ClassType>::GetChunkName());
                    ReplicaChunkDescriptorTable::Get().EndConstructReplicaChunk();
                    return result;
                };

            binding->UnregisterChunkType = [chunkClassId]()
                {
                    ReplicaChunkDescriptorTable::Get().UnregisterReplicaChunkDescriptor(chunkClassId);
                };

            binding->CreateReplicaChunk = [netContext, chunkClassId]()
                {
                    ReflectedReplicaChunkBase* chunk = new(azmalloc(ReflectedReplicaChunk<ClassType>::GetChunkSize(), AZStd::alignment_of<ReflectedReplicaChunk<ClassType> >::value, AZ::SystemAllocator, ReflectedReplicaChunk<ClassType>::GetChunkName()))ReflectedReplicaChunk<ClassType>();
                    AZ::u8* dataStart = chunk->GetDataStart();
                    AZ::u8* dataEnd = reinterpret_cast<AZ::u8*>(chunk) + chunk->GetSize();
                    ptrdiff_t offset = 0;
                    netContext->EnumerateFields(chunkClassId,
                        [&offset, dataStart, dataEnd](FieldDescBase* field)
                        {
                            AZ_Assert((dataStart + offset) < dataEnd, "Overflow in NetworkContext::CreateReplicaChunk while creating %s", ReflectedReplicaChunk<ClassType>::GetChunkName());
                            void* dataSetMem = reinterpret_cast<void*>(dataStart + offset);
                            field->ConstructDataSet(dataSetMem);
                            offset += field->GetDataSetSize();
                        });
                    netContext->EnumerateRpcs(chunkClassId,
                        [&offset, dataStart, dataEnd](RpcDescBase* rpc)
                        {
                            AZ_Assert((dataStart + offset) < dataEnd, "Overflow in NetworkContext::CreateReplicaChunk while creating %s", ReflectedReplicaChunk<ClassType>::GetChunkName());
                            void* rpcMem = reinterpret_cast<void*>(dataStart + offset);
                            rpc->ConstructRpc(rpcMem);
                            offset += rpc->GetRpcSize();
                        });
                    AZ_Assert((dataStart + offset) == dataEnd, "Overflow/underflow in NetworkContext::CreateReplicaChunk while creating %s", ReflectedReplicaChunk<ClassType>::GetChunkName());
                    return chunk;
                };

            binding->DestroyReplicaChunk = [netContext, chunkClassId](ReplicaChunkBase* chunkBase)
                {
                    AZ_Assert(chunkBase->GetDescriptor()->GetChunkTypeId() == chunkClassId, "Mismatched chunk type id for %s (0x%p)", ReflectedReplicaChunk<ClassType>::GetChunkName(), chunkBase);
                    ReflectedReplicaChunkBase* chunk = static_cast<ReflectedReplicaChunkBase*>(chunkBase);
                    chunk->Unbind();

                    AZ::u8* dataStart = chunk->GetDataStart();
                    AZ::u8* dataEnd = reinterpret_cast<AZ::u8*>(chunk) + chunk->GetSize();
                    ptrdiff_t offset = 0;
                    netContext->EnumerateFields(chunkClassId,
                        [&offset, dataStart, dataEnd](FieldDescBase* field)
                        {
                            AZ_Assert((dataStart + offset) < dataEnd, "Overflow in dtor while destroying %s", ReflectedReplicaChunk<ClassType>::GetChunkName());
                            void* dataSetMem = reinterpret_cast<void*>(dataStart + offset);
                            field->DestructDataSet(dataSetMem);
                            offset += field->GetDataSetSize();
                        });
                    netContext->EnumerateRpcs(chunkClassId,
                        [&offset, dataStart, dataEnd](RpcDescBase* rpc)
                        {
                            AZ_Assert((dataStart + offset) < dataEnd, "Overflow in NetworkContext::CreateReplicaChunk while creating %s", ReflectedReplicaChunk<ClassType>::GetChunkName());
                            void* rpcMem = reinterpret_cast<void*>(dataStart + offset);
                            rpc->DestructRpc(rpcMem);
                            offset += rpc->GetRpcSize();
                        });
                    AZ_Assert((dataStart + offset) == dataEnd, "Overflow/underflow in dtor while destroying %s", ReflectedReplicaChunk<ClassType>::GetChunkName());
                    chunk->~ReflectedReplicaChunkBase();
                    azfree(chunk, AZ::SystemAllocator, ReflectedReplicaChunk<ClassType>::GetChunkSize(), AZStd::alignment_of<ReflectedReplicaChunk<ClassType> >::value);
                };

            binding->BindRpcs = [netContext, chunkClassId](NetBindable* bindable)
                {
                    ClassType* derivedInstance = static_cast<ClassType*>(bindable);
                    netContext->EnumerateRpcs(chunkClassId,
                        [derivedInstance](const RpcDescBase* rpc)
                        {
                            NetBindableRpcBase* bindableRpc = reinterpret_cast<NetBindableRpcBase*>(reinterpret_cast<AZ::u8*>(derivedInstance) + rpc->GetOffset());
                            bindableRpc->Bind(derivedInstance);
                        });
                };

            binding->m_chunkDesc.m_chunkId = chunkClassId;
        }
    }

    template <class ChunkType, typename DescriptorType>
    void NetworkContext::InitExternalChunkBinding(ClassDescPtr binding)
    {
        if (!binding->RegisterChunkType)
        {
            binding->m_chunkDesc.m_name = ChunkType::GetChunkName();
            ReplicaChunkClassId chunkClassId = ReplicaChunkClassId(binding->m_chunkDesc.m_name);
            m_chunkBindings[chunkClassId] = binding;
            const AZ::Uuid& typeId = binding->m_typeId;
            NetworkContext* netContext = this;
            binding->RegisterChunkType = [chunkClassId, typeId, netContext]()
                {
                    bool result = ReplicaChunkDescriptorTable::Get().RegisterChunkType<ChunkType, DescriptorType>();
                    NetworkContextChunkDescriptor* desc = static_cast<NetworkContextChunkDescriptor*>(ReplicaChunkDescriptorTable::Get().FindReplicaChunkDescriptor(chunkClassId));
                    desc->Bind(typeId);
                    netContext->EnumerateFields(chunkClassId,
                        [desc](FieldDescBase* field)
                        {
                            desc->RegisterDataSet(field->m_name, field->m_offset);
                            field->m_dataSetIdx = desc->GetDataSetIndex(field->m_offset);
                        });
                    netContext->EnumerateRpcs(chunkClassId,
                        [desc](RpcDescBase* rpc)
                        {
                            desc->RegisterRPC(rpc->m_name, rpc->m_offset);
                        });
                    return result;
                };

            binding->UnregisterChunkType = [chunkClassId]()
                {
                    return ReplicaChunkDescriptorTable::Get().UnregisterReplicaChunkDescriptor(chunkClassId);
                };

            binding->CreateReplicaChunk = []()
                {
                    return aznew ChunkType();
                };

            binding->DestroyReplicaChunk = [](ReplicaChunkBase* chunk)
                {
                    delete chunk;
                };

            binding->m_chunkDesc.m_chunkId = chunkClassId;
        }
    }

    template <class ClassType>
    ReplicaChunkBase* NetworkContext::CreateReplicaChunk()
    {
        return CreateReplicaChunk(AZ::AzTypeInfo<ClassType>::Uuid());
    }

    ///////////////////////////////////////////////////////////////////////////
    template <class DataSetType>
    NetworkContext::DataSetDesc<DataSetType>::DataSetDesc(const char* name, ptrdiff_t offset)
        : NetworkContext::FieldDescBase(name, offset)
    {
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename FieldType>
    NetworkContext::NetBindableFieldDesc<FieldType>::NetBindableFieldDesc(const char* name, ptrdiff_t offset)
        : NetworkContext::FieldDescBase(name, offset)
    {
    }

    ///////////////////////////////////////////////////////////////////////////
    template <class ClassType, class DataType, typename MarshalerType>
    void NetworkContext::CtorDataDesc<ClassType, DataType, MarshalerType>::Marshal(NetBindable* netBindable, WriteBuffer& buffer) const
    {
        ClassType* instance = static_cast<ClassType*>(netBindable);
        DataType data = m_get(instance);
        buffer.Write(data, m_marshaler);
    }

    template <class ClassType, class DataType, typename MarshalerType>
    void NetworkContext::CtorDataDesc<ClassType, DataType, MarshalerType>::Unmarshal(ReadBuffer& buffer, NetBindable* netBindable) const
    {
        ClassType* instance = static_cast<ClassType*>(netBindable);
        DataType data;
        buffer.Read(data, m_marshaler);
        if (instance)
        {
            m_set(instance, data);
        }
    }

    template <class ClassType, class DataType, typename MarshalerType>
    void NetworkContext::CtorDataDesc<ClassType, DataType, MarshalerType>::Copy(ReadBuffer& src, WriteBuffer& dest) const
    {
        DataType data;
        src.Read(data, m_marshaler);
        dest.Write(data, m_marshaler);
    }

    ///////////////////////////////////////////////////////////////////////////
    template <class ChunkType, typename DescriptorType>
    NetworkContext::ClassBuilderPtr NetworkContext::ClassBuilder::Chunk()
    {
        if (!m_context->IsRemovingReflection())
        {
            static_assert((AZStd::is_base_of<ReplicaChunkBase, ChunkType>::value), "ReplicaChunks being registered with the NetworkContext must derive from ReplicaChunk");
            static_assert((AZStd::is_base_of<NetworkContextChunkDescriptor, DescriptorType>::value), "Chunk bindings via NetworkContext must use a NetworkContextChunkDescriptor derived descriptor");
            AZ_Assert(!m_binding->m_typeId.IsNull(), "Cannot register a ReplicaChunk for a class which has not been declared to the NetworkContext");
            AZ_Assert(!m_binding->m_chunkDesc.m_chunkId, "Cannot register more than one ReplicaChunk binding for a class in the NetworkContext");

            m_context->InitExternalChunkBinding<ChunkType, DescriptorType>(m_binding);
            m_binding->m_chunkDesc.m_external = true;
        }
        return this;
    }

    template <class ClassType, typename FieldType>
    typename AZStd::enable_if<AZStd::is_base_of<NetBindableFieldBase, FieldType>::value, NetworkContext::ClassBuilderPtr>::type
    NetworkContext::ClassBuilder::Field(const char* name, FieldType ClassType::* address)
    {
        if (!m_context->IsRemovingReflection())
        {
            AZ_Assert(!m_binding->m_typeId.IsNull(), "Cannot register a field for a class which has not been declared to the NetworkContext");
            AZ_Assert(!m_binding->m_chunkDesc.m_external, "Cannot register a NetBindable::Field from within an external chunk");

            m_context->InitReflectedChunkBinding<ClassType>(m_binding);
            ptrdiff_t offset = reinterpret_cast<ptrdiff_t>(&(reinterpret_cast<ClassType const volatile*>(0)->*address));
            m_binding->m_chunkDesc.m_fields.push_back(aznew NetBindableFieldDesc<FieldType>(name, offset));
        }

        return this;
    }

    template <class ClassType, typename DataSetType>
    typename AZStd::enable_if<AZStd::is_base_of<DataSetBase, DataSetType>::value, NetworkContext::ClassBuilderPtr>::type
    NetworkContext::ClassBuilder::Field(const char* name, DataSetType ClassType::* address)
    {
        if (!m_context->IsRemovingReflection())
        {
            AZ_Assert(!m_binding->m_typeId.IsNull(), "Cannot register a field for a class which has not been declared to the NetworkContext");

            ptrdiff_t offset = reinterpret_cast<ptrdiff_t>(&(reinterpret_cast<ClassType const volatile*>(0)->*address));
            m_binding->m_chunkDesc.m_fields.push_back(aznew DataSetDesc<DataSetType>(name, offset));
        }

        return this;
    }

    template <class ClassType, class InterfaceType, typename ... Args, class Traits, typename RpcBindType>
    typename AZStd::enable_if<AZStd::is_base_of<RpcBase, RpcBindType>::value, NetworkContext::ClassBuilderPtr>::type
    NetworkContext::ClassBuilder::RPC(const char* name, RpcBindType ClassType::* rpc)
    {
        if (!m_context->IsRemovingReflection())
        {
            static_assert((AZStd::is_base_of<ReplicaChunkInterface, InterfaceType>::value), "Cannot bind an RPC call to an object which is not a ReplicaChunkInterface");
            AZ_Assert(!m_binding->m_typeId.IsNull(), "Cannot register an RPC for a class which has not been declared to the NetworkContext");

            ptrdiff_t offset = reinterpret_cast<ptrdiff_t>(&(reinterpret_cast<ClassType const volatile*>(0)->*rpc));
            m_binding->m_chunkDesc.m_rpcs.push_back(aznew RpcDescBase(name, offset));
        }

        return this;
    }

    template <class ClassType, class InterfaceType, typename ... Args, class Traits, typename RpcBindType>
    typename AZStd::enable_if<AZStd::is_base_of<NetBindableRpcBase, RpcBindType>::value, NetworkContext::ClassBuilderPtr>::type
    NetworkContext::ClassBuilder::RPC(const char* name, RpcBindType ClassType::* rpc)
    {
        if (!m_context->IsRemovingReflection())
        {
            static_assert((AZStd::is_base_of<ReplicaChunkInterface, InterfaceType>::value), "Cannot bind an RPC call to an object which is not a ReplicaChunkInterface");
            AZ_Assert(!m_binding->m_typeId.IsNull(), "Cannot register an RPC for a class which has not been declared to the NetworkContext");

            m_context->InitReflectedChunkBinding<ClassType>(m_binding);

            ptrdiff_t offset = reinterpret_cast<ptrdiff_t>(&(reinterpret_cast<ClassType const volatile*>(0)->*rpc));
            m_binding->m_chunkDesc.m_rpcs.push_back(aznew NetBindableRpcDesc<RpcBindType>(name, offset));
        }

        return this;
    }

    template <class ClassType, class DataType, typename GetterFunction, typename SetterFunction, typename MarshalerType>
    NetworkContext::ClassBuilderPtr NetworkContext::ClassBuilder::CtorDataImpl(const char* name, GetterFunction getter, SetterFunction setter, const MarshalerType&)
    {
        if (!m_context->IsRemovingReflection())
        {
            m_context->InitReflectedChunkBinding<ClassType>(m_binding);

            auto get = [getter](NetBindable* nb) -> DataType { return (*static_cast<ClassType*>(nb).*getter)(); };
            auto set = [setter](NetBindable* nb, const DataType& data) { (*static_cast<ClassType*>(nb).*setter)(data); };
            m_binding->m_chunkDesc.m_ctors.push_back(aznew CtorDataDesc<ClassType, DataType, MarshalerType>(name, get, set));
        }

        return this;
    }

    ///////////////////////////////////////////////////////////////////////////
    template <class ClassType>
    const char* ReflectedReplicaChunk<ClassType>::GetChunkName()
    {
        static char name[128] = { 0 };
        if (!name[0])
        {
            AZ::Internal::AzTypeInfoSafeCat(name, AZ_ARRAY_SIZE(name), "ReflectedReplicaChunk<");
            AZ::Internal::AzTypeInfoSafeCat(name, AZ_ARRAY_SIZE(name), AZ::AzTypeInfo<ClassType>::Name());
            AZ::Internal::AzTypeInfoSafeCat(name, AZ_ARRAY_SIZE(name), ">");
        }
        return name;
    }

    template <class ClassType>
    size_t ReflectedReplicaChunk<ClassType>::GetChunkSize()
    {
        static size_t chunkSize = 0;
        if (chunkSize == 0)
        {
            NetworkContext* netContext = nullptr;
            EBUS_EVENT_RESULT(netContext, NetSystemRequestBus, GetNetworkContext);
            AZ_Assert(netContext, "No NetworkContext found while trying to compute chunk size");
            if (!netContext)
            {
                return 0;
            }
            chunkSize = sizeof(ReflectedReplicaChunk<ClassType>) + netContext->GetReflectedChunkSize(AZ::AzTypeInfo<ClassType>::Uuid());
        }

        return chunkSize;
    }

    template <class ClassType>
    void ReflectedReplicaChunk<ClassType>::Bind(NetBindable* instance, NetworkContextBindMode mode)
    {
        SetHandler(instance);
        ClassType* derivedInstance = azrtti_cast<ClassType*>(instance);
        AZ_Assert(derivedInstance, "Unable to convert NetBindable to %s", AZ::AzTypeInfo<ClassType>::Name());
        ReplicaChunkDescriptor* desc = GetDescriptor();
        NetworkContext* netContext = nullptr;
        EBUS_EVENT_RESULT(netContext, NetSystemRequestBus, GetNetworkContext);
        netContext->EnumerateFields(desc->GetChunkTypeId(),
            [this, derivedInstance, desc, mode](NetworkContext::FieldDescBase* field)
            {
                NetBindableFieldBase* bindableField = reinterpret_cast<NetBindableFieldBase*>(reinterpret_cast<AZ::u8*>(derivedInstance) + field->GetOffset());
                DataSetBase* dataSet = desc->GetDataSet(this, field->GetDataSetIndex());
                bindableField->Bind(dataSet, mode);
            });
        netContext->EnumerateRpcs(desc->GetChunkTypeId(),
            [this, derivedInstance, desc](NetworkContext::RpcDescBase* rpc)
            {
                NetBindableRpcBase* bindableRpc = reinterpret_cast<NetBindableRpcBase*>(reinterpret_cast<AZ::u8*>(derivedInstance) + rpc->GetOffset());
                RpcBase* rpcBase = desc->GetRpc(this, rpc->GetRpcIndex());
                bindableRpc->Bind(rpcBase);
            });

        // Transfer any stored ctor data from the buffer -> NetBindable instance
        if (m_ctorBuffer.Size() > 0)
        {
            ReadBuffer ctorBuffer(m_ctorBuffer.GetEndianType(), m_ctorBuffer.Get(), m_ctorBuffer.Size());
            netContext->EnumerateCtorData(desc->GetChunkTypeId(),
                [instance, &ctorBuffer](NetworkContext::CtorDataBase* ctorData)
                {
                    ctorData->Unmarshal(ctorBuffer, instance);
                });
        }
    }

    template <class ClassType>
    void ReflectedReplicaChunk<ClassType>::Unbind()
    {
        ReplicaChunkInterface* handler = GetHandler();
        if (!handler || handler == this)
        {
            return;
        }

        NetBindable* netBindable = static_cast<NetBindable*>(handler);
        ClassType* derivedInstance = azrtti_cast<ClassType*>(netBindable);
        AZ_Assert(derivedInstance, "Unable to convert NetBindable to %s. Have you forgotten to derive your component from AzFramework::NetBindable?", AZ::AzTypeInfo<ClassType>::Name());
        if (derivedInstance)
        {
            ReplicaChunkDescriptor* desc = GetDescriptor();
            NetworkContext* netContext = nullptr;
            EBUS_EVENT_RESULT(netContext, NetSystemRequestBus, GetNetworkContext);
            netContext->EnumerateFields(desc->GetChunkTypeId(),
                [derivedInstance](NetworkContext::FieldDescBase* field)
                {
                    NetBindableFieldBase* bindableField = reinterpret_cast<NetBindableFieldBase*>(reinterpret_cast<AZ::u8*>(derivedInstance) + field->GetOffset());
                    bindableField->Bind(nullptr, NetworkContextBindMode::NonAuthoritative);
                });
            netContext->EnumerateRpcs(desc->GetChunkTypeId(),
                [derivedInstance](NetworkContext::RpcDescBase* rpc)
                {
                    NetBindableRpcBase* bindableRpc = reinterpret_cast<NetBindableRpcBase*>(reinterpret_cast<AZ::u8*>(derivedInstance) + rpc->GetOffset());
                    bindableRpc->Bind(derivedInstance);
                });
        }

        // We have disconnected from the handler and erased any connections from DataFields or Rpcs
        SetHandler(nullptr);
    }
} // namespace AZ
