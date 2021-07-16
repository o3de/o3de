/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_REPLICA_CHUNK_DESCRIPTOR_H
#define GM_REPLICA_CHUNK_DESCRIPTOR_H

#include <GridMate/Replica/ReplicaCommon.h>
#include <GridMate/Replica/ReplicaChunk.h>

#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/std/containers/intrusive_list.h>

namespace GridMate
{
    class ReplicaChunkBase;
    class RpcBase;
    class DataSetBase;

    /**
    * ReplicaChunkDescriptors provide the replica manager with
    * structural information about replica chunk types so they can
    * be created.
    *
    * Descriptors are created during replica chunk registration,
    * but their tables are not populated until first time an instance
    * of such chunk type is created.
    *
    * Replica chunk types are registered by calling
    * ReplicaChunkDescriptorTable::RegisterChunkType().
    */
    class ReplicaChunkDescriptor
    {
        friend class ReplicaChunkDescriptorTable;

    public:
        ReplicaChunkDescriptor(const char* pNameStr, size_t classSize);
        virtual ~ReplicaChunkDescriptor() { }

        //! Called by the system when creating replica chunks from network data.
        virtual ReplicaChunkBase* CreateFromStream(UnmarshalContext& mc) = 0;
        //! Called by the system to skip ctor data from the stream.
        virtual void DiscardCtorStream(UnmarshalContext& mc) = 0;
        //! Hook to implement chunk object deletion.
        virtual void DeleteReplicaChunk(ReplicaChunkBase* chunkInstance) = 0;
        //! Called by the system when chunk ctor data needs to be sent.
        virtual void MarshalCtorData(ReplicaChunkBase* chunkInstance, WriteBuffer& wb) = 0;

        virtual ZoneMask GetZoneMask() const { return ZoneMask_All; }

        bool IsInitialized() const { return m_isInitialized; }
        const char* GetChunkName() const { return m_chunkClassName; }
        ReplicaChunkClassId GetChunkTypeId() const { return m_chunkTypeId; }
        size_t GetChunkSize() const { return m_chunkClassSize; }

        void RegisterDataSet(const char* debugName, DataSetBase* ds);
        void RegisterDataSet(const char* debugName, ptrdiff_t offset);
        size_t GetDataSetCount() const { return m_vdt.size(); }
        DataSetBase* GetDataSet(const ReplicaChunkBase* base, size_t index) const;
        size_t GetDataSetIndex(const ReplicaChunkBase* base, const DataSetBase* dataset) const;
        size_t GetDataSetIndex(ptrdiff_t offset) const;
        const char* GetDataSetName(const ReplicaChunkBase* base, const DataSetBase* dataset) const;

        void RegisterRPC(const char* debugName, RpcBase* rpc);
        void RegisterRPC(const char* debugName, ptrdiff_t offset);
        size_t GetRpcCount() const { return m_vrt.size(); }
        RpcBase* GetRpc(const ReplicaChunkBase* base, size_t index) const;
        size_t GetRpcIndex(const ReplicaChunkBase* base, const RpcBase* rpc) const;
        size_t GetRpcIndex(ptrdiff_t offset) const;
        const char* GetRpcName(const ReplicaChunkBase* base, const RpcBase* rpc) const;

    protected:
        struct ReplicaChunkMemberDescriptor
        {
            ptrdiff_t m_offset;
            const char* m_debugName;
        };

        // Virtual DataSet Table
        typedef AZStd::fixed_vector<ReplicaChunkMemberDescriptor, GM_MAX_DATASETS_IN_CHUNK> VDT;

        // Virtual RPC Table
        typedef AZStd::fixed_vector<ReplicaChunkMemberDescriptor, GM_MAX_RPCS_DECL_PER_CHUNK> VRT;

        ReplicaChunkClassId m_chunkTypeId;
        const char* m_chunkClassName;
        size_t m_chunkClassSize;
        VDT m_vdt;
        VRT m_vrt;
        bool m_isInitialized;
    };

    /**
    * DefaultReplicaChunkDescriptor provides a common implementation for
    * chunk descriptors.
    * It can be used for chunk types that do not use ctor data and
    * have no special construction/destruction requirements.
    */
    template<typename ReplicaChunkType, ZoneMask mask = ZoneMask_All>
    class DefaultReplicaChunkDescriptor
        : public ReplicaChunkDescriptor
    {
    public:
        DefaultReplicaChunkDescriptor()
            : ReplicaChunkDescriptor(ReplicaChunkType::GetChunkName(), sizeof(ReplicaChunkType))
        { }

        ReplicaChunkBase* CreateFromStream(UnmarshalContext&) override
        {
            // Pre/Post construct allow DataSets and RPCs to bind to the chunk.
            ReplicaChunkBase* replicaChunk = aznew ReplicaChunkType;
            return replicaChunk;
        }

        void DiscardCtorStream(UnmarshalContext&) override { }
        void DeleteReplicaChunk(ReplicaChunkBase* chunkInstance) override { delete chunkInstance; }
        void MarshalCtorData(ReplicaChunkBase*, WriteBuffer&) override { }
        ZoneMask GetZoneMask() const override { return mask; }
    };

    /**
    * ReplicaChunkInitStack is used during chunk creation to
    * provide creation context and to build the VDT/VRT on first use.
    */
    struct ReplicaChunkInitContext
    {
        ReplicaChunkDescriptor* m_descriptor;
        ReplicaChunkBase* m_chunk; // current replica chunk instance being constructed
    };
    typedef AZStd::fixed_vector<ReplicaChunkInitContext, 8> ReplicaChunkInitContextStack;

    /**
    * Stores descriptors for registered replica chunk types.
    * This table is stored as an AZ::EnvironmentVariable for cross-dll compatibility,
    * so it is subject to all the rules of AZ::Environment.
    */
    class ReplicaChunkDescriptorTable
    {
    public:
        ~ReplicaChunkDescriptorTable();

        static ReplicaChunkDescriptorTable& Get();

        //! Register a replica chunk type. Replica chunk types must be registered before they can be instantiated.
        //! Returns true if successfully registered, false otherwise.
        template<typename ReplicaChunkType>
        bool RegisterChunkType()
        {
            return RegisterChunkType<ReplicaChunkType, DefaultReplicaChunkDescriptor<ReplicaChunkType> >();
        }

        //! Register a replica chunk type. Replica chunk types must be registered before they can be instantiated.
        //! Returns true if successfully registered, false otherwise.
        template<typename ReplicaChunkType, typename ReplicaChunkDescriptorType>
        bool RegisterChunkType()
        {
            ReplicaChunkClassId chunkTypeId(ReplicaChunkType::GetChunkName());
            ReplicaChunkDescriptor* descriptor = FindReplicaChunkDescriptor(chunkTypeId);
            if (descriptor)
            {
                // TODO: verify descriptor compatibility
                AZ_TracePrintf("GridMate", "Replica type %s(0x%x) already registered. New registration ignored.\n", ReplicaChunkType::GetChunkName(), static_cast<unsigned int>(chunkTypeId));
            }
            else
            {
                // Descriptor memory is owned by the table.
                // All entries will be freed automatically when the table is destroyed.
                descriptor = azcreate(ReplicaChunkDescriptorType, (), AZ::OSAllocator);
                AddReplicaChunkDescriptor(chunkTypeId, descriptor);
            }

            return true;
        }

        //! Returns the descriptor for a particular ReplicaChunk type
        ReplicaChunkDescriptor* FindReplicaChunkDescriptor(ReplicaChunkClassId chunkTypeId);

        //! Unregister a chunk descriptor. Returns false is descriptor was not found.
        bool UnregisterReplicaChunkDescriptor(ReplicaChunkClassId chunkTypeId);

        //! Called right before instantiating a replica chunk.
        void BeginConstructReplicaChunk(ReplicaChunkDescriptor* descriptor);

        //! Called right after instantiation of a replica chunk.
        void EndConstructReplicaChunk();

        //! Returns the current replica chunk init context.
        ReplicaChunkInitContext* GetCurrentReplicaChunkInitContext();

    protected:

        /**
        * The replica chunk descriptor table registers descriptors by adding them to two intrusive lists of DescriptorInfos,
        * one owned by the module doing the registration used to track descriptor ownership, and a global one used for queries.
        * The module list tracks the descriptors created by the module so they can be automatically unregistered when
        * the module is unloaded. The global one is used to share the descriptors across modules.
        * A static copy of ReplicaChunkDescriptorTable is created for each module and holds a reference to the global table to
        * guarantee its existence, but individual entries are guaranteed to be unregistered before their contents become invalid.
        * Descriptors and DescriptorInfos are allocated using AZ_OS_MALLOC to avoid dependencies on any allocators. This makes
        * things easier for users by removing the requirement of always having to explicitly unregister descriptors.
        */
        struct DescriptorInfo
            : public AZStd::intrusive_list_node<DescriptorInfo>
        {
            AZ_CLASS_ALLOCATOR(DescriptorInfo, AZ::OSAllocator, 0);
            ReplicaChunkClassId m_chunkTypeId;
            ReplicaChunkDescriptor* m_descriptor; // pointer to the replica descriptor
        };
        typedef AZStd::intrusive_list<DescriptorInfo, AZStd::list_base_hook<DescriptorInfo> > DescriptorContainerType;

        //! Adds the descriptor to the tables. Does not check for duplicates!
        void AddReplicaChunkDescriptor(ReplicaChunkClassId chunkTypeId, ReplicaChunkDescriptor* descriptor);

        //! Unregisters the descriptor from the global table. Returns false if the descriptor is not found.
        bool UnregisterReplicaChunkDescriptorFromGlobalTable(ReplicaChunkClassId chunkTypeId);

        DescriptorContainerType m_moduleDescriptorTable;                                        // Tracks descriptors created by the module
        AZ::EnvironmentVariable<DescriptorContainerType> m_globalDescriptorTable;               // Holds the global list of descriptors

        AZ::EnvironmentVariable<ReplicaChunkInitContextStack> m_globalChunkInitContextStack;    // Tracks the current chunk type that is being constructed

        static ReplicaChunkDescriptorTable s_theTable;  // Per-module static copy of the table
    };
} // namespace GridMate

#endif // GM_REPLICA_CHUNK_DESCRIPTOR_H

#pragma once
