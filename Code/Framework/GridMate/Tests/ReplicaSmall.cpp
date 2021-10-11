/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "Tests.h"

#include <AzCore/std/parallel/thread.h>

#include <GridMate/Carrier/DefaultSimulator.h>

#include <GridMate/Containers/unordered_set.h>

#include <GridMate/Replica/Interpolators.h>
#include <GridMate/Replica/Replica.h>
#include <GridMate/Replica/ReplicaFunctions.h>
#include <GridMate/Replica/ReplicaMgr.h>
#include <GridMate/Replica/ReplicaStatus.h>

#include <GridMate/Serialize/DataMarshal.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

using namespace GridMate;

namespace UnitTest {

class TestChunk
    : public ReplicaChunk
{
public:
    GM_CLASS_ALLOCATOR(TestChunk);
    typedef AZStd::intrusive_ptr<TestChunk> Ptr;
    static const char* GetChunkName() { return "TestChunk"; }

    bool IsReplicaMigratable() override { return false; }
};

class BaseChunk
    : public ReplicaChunk
{
public:
    GM_CLASS_ALLOCATOR(BaseChunk);
    typedef AZStd::intrusive_ptr<BaseChunk> Ptr;
    static const char* GetChunkName() { return "BaseChunk"; }

    bool IsReplicaMigratable() override { return false; }
};

class ChildChunk
    : public BaseChunk
{
public:
    GM_CLASS_ALLOCATOR(ChildChunk);
    typedef AZStd::intrusive_ptr<ChildChunk> Ptr;
    static const char* GetChunkName() { return "ChildChunk"; }

    bool IsReplicaMigratable() override { return false; }
};

class ChildChildChunk
    : public ChildChunk
{
public:
    GM_CLASS_ALLOCATOR(ChildChildChunk);
    typedef AZStd::intrusive_ptr<ChildChildChunk> Ptr;
    static const char* GetChunkName() { return "ChildChildChunk"; }

    bool IsReplicaMigratable() override { return false; }
};

class ChildChunk2
    : public BaseChunk
{
public:
    GM_CLASS_ALLOCATOR(ChildChunk2);
    typedef AZStd::intrusive_ptr<ChildChunk2> Ptr;
    static const char* GetChunkName() { return "ChildChunk2"; }

    bool IsReplicaMigratable() override { return false; }
};

class EventChunk
    : public ReplicaChunk
{
public:
    GM_CLASS_ALLOCATOR(EventChunk);
    EventChunk()
        : m_attaches(0)
        , m_detaches(0)
    {
    }
    typedef AZStd::intrusive_ptr<EventChunk> Ptr;
    static const char* GetChunkName() { return "EventChunk"; }

    bool IsReplicaMigratable() override { return false; }

    void OnAttachedToReplica(Replica*) override
    {
        m_attaches++;
    }

    void OnDetachedFromReplica(Replica*) override
    {
        m_detaches++;
    }

    int m_attaches;
    int m_detaches;
};

class ChunkAdd
    : public UnitTest::GridMateMPTestFixture
{
public:
    GM_CLASS_ALLOCATOR(ChunkAdd);

    void run()
    {
        AZ_TracePrintf("GridMate", "\n");

        ReplicaChunkDescriptorTable::Get().RegisterChunkType<BaseChunk>();
        ReplicaChunkDescriptorTable::Get().RegisterChunkType<TestChunk>();

        ReplicaPtr replica = Replica::CreateReplica(nullptr);
        AZ_TEST_ASSERT(replica->GetNumChunks() == 1);

        CreateAndAttachReplicaChunk<BaseChunk>(replica);
        AZ_TEST_ASSERT(replica->GetNumChunks() == 2);

        CreateAndAttachReplicaChunk<TestChunk>(replica);
        AZ_TEST_ASSERT(replica->GetNumChunks() == 3);
    }
};

class ChunkCast
    : public UnitTest::GridMateMPTestFixture
{
public:
    GM_CLASS_ALLOCATOR(ChunkCast);

    void run()
    {
        AZ_TracePrintf("GridMate", "\n");

        ReplicaChunkDescriptorTable::Get().RegisterChunkType<BaseChunk>();
        ReplicaChunkDescriptorTable::Get().RegisterChunkType<ChildChunk>();
        ReplicaChunkDescriptorTable::Get().RegisterChunkType<ChildChildChunk>();
        ReplicaChunkDescriptorTable::Get().RegisterChunkType<ChildChunk2>();
        ReplicaChunkDescriptorTable::Get().RegisterChunkType<TestChunk>();

        ReplicaPtr r1 = Replica::CreateReplica(nullptr);
        CreateAndAttachReplicaChunk<BaseChunk>(r1);

        ReplicaPtr r2 = Replica::CreateReplica(nullptr);
        CreateAndAttachReplicaChunk<ChildChunk>(r2);

        ReplicaPtr r3 = Replica::CreateReplica(nullptr);
        CreateAndAttachReplicaChunk<ChildChildChunk>(r3);

        ReplicaPtr r4 = Replica::CreateReplica(nullptr);
        CreateAndAttachReplicaChunk<ChildChunk2>(r4);

        AZ_TEST_ASSERT(r1->FindReplicaChunk<BaseChunk>());
        AZ_TEST_ASSERT(!r1->FindReplicaChunk<ChildChunk>());
        AZ_TEST_ASSERT(!r1->FindReplicaChunk<ChildChildChunk>());
        AZ_TEST_ASSERT(!r1->FindReplicaChunk<ChildChunk2>());
        AZ_TEST_ASSERT(!r1->FindReplicaChunk<TestChunk>());

        AZ_TEST_ASSERT(!r2->FindReplicaChunk<BaseChunk>());
        AZ_TEST_ASSERT(r2->FindReplicaChunk<ChildChunk>());
        AZ_TEST_ASSERT(!r2->FindReplicaChunk<ChildChildChunk>());
        AZ_TEST_ASSERT(!r2->FindReplicaChunk<ChildChunk2>());
        AZ_TEST_ASSERT(!r2->FindReplicaChunk<TestChunk>());

        AZ_TEST_ASSERT(!r3->FindReplicaChunk<BaseChunk>());
        AZ_TEST_ASSERT(!r3->FindReplicaChunk<ChildChunk>());
        AZ_TEST_ASSERT(r3->FindReplicaChunk<ChildChildChunk>());
        AZ_TEST_ASSERT(!r3->FindReplicaChunk<ChildChunk2>());
        AZ_TEST_ASSERT(!r3->FindReplicaChunk<TestChunk>());

        AZ_TEST_ASSERT(!r4->FindReplicaChunk<BaseChunk>());
        AZ_TEST_ASSERT(!r4->FindReplicaChunk<ChildChunk>());
        AZ_TEST_ASSERT(!r4->FindReplicaChunk<ChildChildChunk>());
        AZ_TEST_ASSERT(r4->FindReplicaChunk<ChildChunk2>());
        AZ_TEST_ASSERT(!r4->FindReplicaChunk<TestChunk>());
    }
};

class ChunkEvents
    : public UnitTest::GridMateMPTestFixture
{
public:
    GM_CLASS_ALLOCATOR(ChunkEvents);

    void run()
    {
        AZ_TracePrintf("GridMate", "\n");

        ReplicaChunkDescriptorTable::Get().RegisterChunkType<EventChunk>();

        ReplicaPtr r1 = Replica::CreateReplica(nullptr);
        EventChunk::Ptr c1 = CreateAndAttachReplicaChunk<EventChunk>(r1);
        AZ_TEST_ASSERT(c1->m_attaches == 1);
        AZ_TEST_ASSERT(c1->m_detaches == 0);

        ReplicaPtr r2 = Replica::CreateReplica(nullptr);
        EventChunk::Ptr c2 = CreateReplicaChunk<EventChunk>();
        AZ_TEST_ASSERT(c2->m_attaches == 0);
        AZ_TEST_ASSERT(c2->m_detaches == 0);

        r2->AttachReplicaChunk(c2);
        AZ_TEST_ASSERT(c2->m_attaches == 1);
        AZ_TEST_ASSERT(c2->m_detaches == 0);

        r2->DetachReplicaChunk(c2);
        AZ_TEST_ASSERT(c2->m_attaches == 1);
        AZ_TEST_ASSERT(c2->m_detaches == 1);

        ReplicaPtr r3 = Replica::CreateReplica(nullptr);
        EventChunk::Ptr c3 = CreateAndAttachReplicaChunk<EventChunk>(r3);
        AZ_TEST_ASSERT(c3->m_attaches == 1);
        AZ_TEST_ASSERT(c3->m_detaches == 0);
        r3 = nullptr;
        AZ_TEST_ASSERT(c3->m_attaches == 1);
        AZ_TEST_ASSERT(c3->m_detaches == 1);
    }
};

/**
* OfflineModeTest verifies that replica chunks are usable without
* an active session, and basically behave as primarys.
*/
class OfflineModeTest
    : public UnitTest::GridMateMPTestFixture
{
public:
    class OfflineChunk
        : public ReplicaChunk
    {
    public:
        GM_CLASS_ALLOCATOR(OfflineChunk);

        static const char* GetChunkName() { return "OfflineChunk"; }

        OfflineChunk()
            : m_data1("Data1")
            , m_data2("Data2")
            , CallRpc("Rpc")
            , m_nCallsDataSetChangeCB(0)
            , m_nCallsRpcHandlerCB(0)
        {
            s_nInstances++;
        }

        ~OfflineChunk() override
        {
            s_nInstances--;
        }

        void DataSetChangeCB(const int& v, const TimeContext& tc)
        {
            (void)v;
            (void)tc;
            m_nCallsDataSetChangeCB++;
        }

        bool RpcHandlerCB(const RpcContext& rc)
        {
            (void)rc;
            m_nCallsRpcHandlerCB++;
            return true;
        }

        bool IsReplicaMigratable() override { return true; }

        DataSet<int> m_data1;
        DataSet<int>::BindInterface<OfflineChunk, & OfflineChunk::DataSetChangeCB> m_data2;
        Rpc<>::BindInterface<OfflineChunk, & OfflineChunk::RpcHandlerCB> CallRpc;

        int m_nCallsDataSetChangeCB;
        int m_nCallsRpcHandlerCB;

        static int s_nInstances;
    };

    void run()
    {
        ReplicaChunkDescriptorTable::Get().RegisterChunkType<OfflineChunk>();

        OfflineChunk* offlineChunk = CreateReplicaChunk<OfflineChunk>();
        AZ_TEST_ASSERT(OfflineChunk::s_nInstances == 1);
        ReplicaChunkPtr chunkPtr = offlineChunk;
        chunkPtr->Init(ReplicaChunkClassId(OfflineChunk::GetChunkName()));
        AZ_TEST_ASSERT(chunkPtr->IsPrimary());
        AZ_TEST_ASSERT(!chunkPtr->IsProxy());
        offlineChunk->m_data1.Set(5);
        AZ_TEST_ASSERT(offlineChunk->m_data1.Get() == 5);
        offlineChunk->m_data1.Modify([](int& v)
            {
                v = 10;
                return true;
            });
        AZ_TEST_ASSERT(offlineChunk->m_data1.Get() == 10);
        offlineChunk->m_data2.Set(5);
        AZ_TEST_ASSERT(offlineChunk->m_data2.Get() == 5);
        offlineChunk->m_data2.Modify([](int& v)
            {
                v = 10;
                return true;
            });
        AZ_TEST_ASSERT(offlineChunk->m_data2.Get() == 10);
        AZ_TEST_ASSERT(offlineChunk->m_nCallsDataSetChangeCB == 0); // DataSet change CB doesn't get called on primary.

        offlineChunk->CallRpc();
        AZ_TEST_ASSERT(offlineChunk->m_nCallsRpcHandlerCB == 1);

        const char* replicaName = "OfflineReplica";
        ReplicaPtr offlineReplica = Replica::CreateReplica(replicaName);
        AZ_TEST_ASSERT(strcmp(offlineReplica->GetDebugName(), replicaName) == 0);

        offlineReplica->AttachReplicaChunk(chunkPtr);
        AZ_TEST_ASSERT(chunkPtr->IsPrimary());
        AZ_TEST_ASSERT(!chunkPtr->IsProxy());

        offlineReplica->DetachReplicaChunk(chunkPtr);
        AZ_TEST_ASSERT(chunkPtr->IsPrimary());
        AZ_TEST_ASSERT(!chunkPtr->IsProxy());

        AZ_TEST_ASSERT(OfflineChunk::s_nInstances == 1);
        chunkPtr = nullptr;
        AZ_TEST_ASSERT(OfflineChunk::s_nInstances == 0);
    }
};
int OfflineModeTest::OfflineChunk::s_nInstances = 0;

class DataSet_PrepareTest
    : public UnitTest::GridMateMPTestFixture
{
public:
    GM_CLASS_ALLOCATOR(DataSet_PrepareTest);

    class TestDataSet : public DataSet<int>
    {
    public:
        TestDataSet() : DataSet<int>( "Test", 0 ) {}

        // A wrapper to call a protected method
        PrepareDataResult PrepareData(EndianType endianType, AZ::u32 marshalFlags) override
        {
            return DataSet<int>::PrepareData(endianType, marshalFlags);
        }
    };

    class SimpleDataSetChunk
        : public ReplicaChunk
    {
    public:
        GM_CLASS_ALLOCATOR(SimpleDataSetChunk);

        SimpleDataSetChunk()
        {
            Data1.MarkAsDefaultValue();
        }

        bool IsReplicaMigratable() override
        {
            return false;
        }

        static const char* GetChunkName()
        {
            static const char* name = "SimpleDataSetChunk";
            return name;
        }

        TestDataSet Data1;
    };

    void run()
    {
        AZ_TracePrintf("GridMate", "\n");

        ReplicaChunkDescriptorTable::Get().RegisterChunkType<SimpleDataSetChunk>();
        AZStd::unique_ptr<SimpleDataSetChunk> chunk(CreateReplicaChunk<SimpleDataSetChunk>());

        // If data set was not changed it should remain as non-dirty even after several PrepareData calls
        for (auto i = 0; i < 10; ++i)
        {
            [[maybe_unused]] auto pdr = chunk->Data1.PrepareData(EndianType::BigEndian, 0);

            AZ_TEST_ASSERT(chunk->Data1.IsDefaultValue() == true);
        }
    }
};

class DataSet_ACKTest
    : public UnitTest::GridMateMPTestFixture
{
public:
    GM_CLASS_ALLOCATOR(DataSet_ACKTest);

    class TestDataSet : public DataSet<int>
    {
    public:
        TestDataSet() : DataSet<int>("Test", 0) {}

        // A wrapper to call a protected method
        PrepareDataResult PrepareData(EndianType endianType, AZ::u32 marshalFlags) override
        {
            return DataSet<int>::PrepareData(endianType, marshalFlags);
        }
    };

    class SimpleDataSetChunk
        : public ReplicaChunk
    {
        friend class DataSet_ACKTest;
    public:
        GM_CLASS_ALLOCATOR(SimpleDataSetChunk);

        SimpleDataSetChunk()
        {
        }

        bool IsReplicaMigratable() override
        {
            return false;
        }

        static const char* GetChunkName()
        {
            static const char* name = "SimpleDataSetChunk2";
            return name;
        }

        TestDataSet Data1;
    };

    void run()
    {
        if (!ReplicaTarget::IsAckEnabled())
        {
            return;
        }
//        const int chunkHeader = 3*8;
//        int replicaHeader = 7*8;
//        //Add ReplicaStatusChunk header
//#ifdef GM_REPLICA_HAS_DEBUG_NAME
//        replicaHeader += 16 + 176;
//#else
//        replicaHeader += 16;
//#endif
//        const int marshalDataSize = 48; //Data plus length
        //Only for Driller
        ReplicaManager rm;
        ReplicaPeer peer(&rm);

        AZ_TracePrintf("GridMate", "\n");
        Replica* replica = Replica::CreateReplica("TestPrimaryReplica");

        ReplicaChunkDescriptorTable::Get().RegisterChunkType<SimpleDataSetChunk>();
        AZStd::unique_ptr<SimpleDataSetChunk> chunk(CreateReplicaChunk<SimpleDataSetChunk>());
        replica->AttachReplicaChunk(chunk.get());

        //1. Pre-change
        auto pdr = replica->DebugPrepareData(EndianType::BigEndian, 0);

        WriteBufferDynamic writeBuffer(EndianType::BigEndian, 0);
        CallbackBuffer callbackBuffer;
        MarshalContext mcNoPeer(ReplicaMarshalFlags::IncludeDatasets, &writeBuffer, &callbackBuffer, ReplicaContext(&rm, TimeContext(), &peer));
        replica->DebugMarshal(mcNoPeer);
        //AZ_Printf("GridMateTests", "buffer size %d\n", writeBuffer.GetExactSize().GetTotalSizeInBits());
        AZ_TEST_ASSERT(writeBuffer.GetExactSize().GetTotalSizeInBits() == 272); //272

        //2. change the data. confirm it marshals correctly
        chunk->Data1.Set(1);
        pdr = replica->DebugPrepareData(EndianType::BigEndian, 0);
        AZ_TEST_ASSERT(chunk->Data1.IsDefaultValue() == false);
        AZ_TEST_ASSERT(pdr.m_isDownstreamUnreliableDirty == true);

        //Marshal
        writeBuffer.Clear();
        callbackBuffer.clear();
        replica->DebugMarshal(mcNoPeer);
        // AZ_Printf("GridMateTests", "buffer size %d\n", writeBuffer.GetExactSize().GetTotalSizeInBits());
        AZ_TEST_ASSERT(writeBuffer.GetExactSize().GetTotalSizeInBits() == 128);   //Headers and data, 128

        //3. confirm next PrepareData sends nothing new
        pdr = replica->DebugPrepareData(EndianType::BigEndian, 0);
        AZ_TEST_ASSERT(pdr.m_isDownstreamUnreliableDirty == false && pdr.m_isDownstreamReliableDirty == false
                            && pdr.m_isUpstreamUnreliableDirty == false && pdr.m_isUpstreamReliableDirty == false);

        //Add an old stamp to the marshal context then confirm marshal re-adds data
        writeBuffer.Clear();
        callbackBuffer.clear();
        MarshalContext mcPreAck(ReplicaMarshalFlags::IncludeDatasets, &writeBuffer, &callbackBuffer, ReplicaContext(&rm, TimeContext(), &peer), 1); //Un-Ack'd
        replica->DebugMarshal(mcPreAck);
        //AZ_Printf("GridMateTests", "With old stamp buffer size %d\n", writeBuffer.GetExactSize().GetTotalSizeInBits());
        AZ_TEST_ASSERT(writeBuffer.GetExactSize().GetTotalSizeInBits() == 304);   //Headers and data, 304

        //4
        for (int i = 0; i < 10; ++i)
        {
            pdr = replica->DebugPrepareData(EndianType::BigEndian, 0);
            AZ_TEST_ASSERT(pdr.m_isDownstreamUnreliableDirty == false && pdr.m_isDownstreamReliableDirty == false
                && pdr.m_isUpstreamUnreliableDirty == false && pdr.m_isUpstreamReliableDirty == false);
            writeBuffer.Clear();
            callbackBuffer.clear();
            MarshalContext mcPostAck(ReplicaMarshalFlags::IncludeDatasets, &writeBuffer, &callbackBuffer, ReplicaContext(&rm, TimeContext(), &peer), 2); //ACK'd
            replica->DebugMarshal(mcPostAck);
            //AZ_Printf("GridMateTests", "with up-to-date stamp buffer size %d\n", writeBuffer.GetExactSize().GetTotalSizeInBits());
            AZ_TEST_ASSERT(writeBuffer.GetExactSize().GetTotalSizeInBits() == 104);           //ACK'd nothing to send; just basic headers 104
        }

        //Remove ref-count
        replica->DebugPreDestruct();
        chunk.release();
        //delete replica;
    }
};

class RpcNullHandlerCrash_Test
    : public UnitTest::GridMateMPTestFixture
{
public:
    GM_CLASS_ALLOCATOR(RpcNullHandlerCrash_Test);

    class Handler
        : public GridMate::ReplicaChunkInterface
    {
    public:
        bool OnRpc(const RpcContext& rc)
        {
            AZ_UNUSED(rc);
            return false;
        }
    };

    class RpcWithoutArgumentsChunk
        : public ReplicaChunk
    {
        friend class DataSet_ACKTest;
    public:
        GM_CLASS_ALLOCATOR(RpcWithoutArgumentsChunk);

        RpcWithoutArgumentsChunk()
            : Rpc1("rpc")
        {
        }

        bool IsReplicaMigratable() override
        {
            return false;
        }

        static const char* GetChunkName()
        {
            return "RpcWithoutArgumentsChunk";
        }

        GridMate::Rpc<>::BindInterface<Handler, &Handler::OnRpc> Rpc1;
    };

    void run()
    {
        AZ_TracePrintf("GridMate", "\n");
        ReplicaPtr replica = Replica::CreateReplica("TestReplica");

        ReplicaChunkDescriptorTable::Get().RegisterChunkType<RpcWithoutArgumentsChunk>();
        AZStd::unique_ptr<RpcWithoutArgumentsChunk> chunk(CreateReplicaChunk<RpcWithoutArgumentsChunk>());
        replica->AttachReplicaChunk(chunk.get());

        chunk->SetHandler(nullptr);

        // This call would fail before LY-68517 "RPC without arguments crashes when a handler is null"
        const bool result = chunk->Rpc1.InvokeImpl(nullptr);
        AZ_TEST_ASSERT(!result);

        chunk.release();
    }
};

class ReplicaChunkDescriptor_Rpc_Crash_Test
    : public UnitTest::GridMateMPTestFixture
{
public:
    GM_CLASS_ALLOCATOR(ReplicaChunkDescriptor_Rpc_Crash_Test);

    class Handler
        : public GridMate::ReplicaChunkInterface
    {
    public:
        bool OnRpc(const RpcContext& rc)
        {
            AZ_UNUSED(rc);
            return false;
        }
    };

    class JustRpcChunk
        : public ReplicaChunk
    {
        friend class DataSet_ACKTest;
    public:
        GM_CLASS_ALLOCATOR(JustRpcChunk);

        JustRpcChunk()
            : Rpc1("rpc")
        {
        }

        bool IsReplicaMigratable() override
        {
            return false;
        }

        static const char* GetChunkName()
        {
            return "JustRpcChunk";
        }

        GridMate::Rpc<>::BindInterface<Handler, &Handler::OnRpc> Rpc1;
    };

    void run()
    {
        ReplicaPtr replica = Replica::CreateReplica("TestReplica");

        ReplicaChunkDescriptorTable::Get().RegisterChunkType<JustRpcChunk>();
        AZStd::unique_ptr<JustRpcChunk> chunk(CreateReplicaChunk<JustRpcChunk>());
        replica->AttachReplicaChunk(chunk.get());

        chunk->SetHandler(nullptr);

        /*
         * Testing that calling GetRpc with bad index won't crash GridMate.
         */
        RpcBase* rpcBase = chunk->GetDescriptor()->GetRpc( chunk.get(), 100 );

        AZ_TEST_ASSERT(!rpcBase);

        chunk.release(); // chunks are owned by replica
    }
};

class DataSet_AuthoritativeCallback_Test
    : public UnitTest::GridMateMPTestFixture
{
public:
    GM_CLASS_ALLOCATOR(DataSet_AuthoritativeCallback_Test);

    class Handler
        : public GridMate::ReplicaChunkInterface
    {
    public:
        void OnDataOnServer(const int& /*value*/, const TimeContext& /*tc*/)
        {
            ++m_invokes;
        }

        int m_invokes = 0;
    };

    class DataWithCustomTraitsChunk
        : public ReplicaChunk
    {
        friend class DataSet_ACKTest;
    public:
        GM_CLASS_ALLOCATOR(DataWithCustomTraitsChunk);

        DataWithCustomTraitsChunk()
            : m_dataSet1("dataset 1")
        {
        }

        bool IsReplicaMigratable() override
        {
            return false;
        }

        static const char* GetChunkName()
        {
            return "DataWithCustomTraitsChunk";
        }

        // DataSetInvokeEverywhereTraits leads to the callback being invoked on the server, i.e. authoritative handler
        GridMate::DataSet<int>::BindInterface<Handler, &Handler::OnDataOnServer, DataSetInvokeEverywhereTraits> m_dataSet1;
    };

    void run()
    {
        ReplicaPtr replica = Replica::CreateReplica("TestReplica");

        ReplicaChunkDescriptorTable::Get().RegisterChunkType<DataWithCustomTraitsChunk>();
        AZStd::unique_ptr<DataWithCustomTraitsChunk> chunk(CreateReplicaChunk<DataWithCustomTraitsChunk>());
        replica->AttachReplicaChunk(chunk.get());

        Handler handler;

        chunk->SetHandler(&handler);

        // This call should invoke OnDataOnServer because we specified DataSetInvokeEverywhereTraits
        chunk->m_dataSet1.Set(1);
        AZ_TEST_ASSERT(handler.m_invokes == 1);

        chunk.release();
    }
};

class DataSet_AuthoritativeCallback_Without_Handler_Test
    : public UnitTest::GridMateMPTestFixture
{
public:
    GM_CLASS_ALLOCATOR(DataSet_AuthoritativeCallback_Without_Handler_Test);
    
    class Handler
        : public GridMate::ReplicaChunkInterface
    {
    public:
        void OnDataOnServer(const int& /*value*/, const TimeContext& /*tc*/)
        {
        }
    };

    class DataWithCustomTraitsAndCallingSetChunk
        : public ReplicaChunk
    {
        friend class DataSet_ACKTest;
    public:
        GM_CLASS_ALLOCATOR(DataWithCustomTraitsAndCallingSetChunk);

        DataWithCustomTraitsAndCallingSetChunk()
            : m_dataSet1("dataset 1")
        {
            m_dataSet1.Set(1); // also testing very early Set() invocation
        }

        bool IsReplicaMigratable() override
        {
            return false;
        }

        static const char* GetChunkName()
        {
            return "DataWithCustomTraitsAndCallingSetChunk";
        }

        // DataSetInvokeEverywhereTraits leads to the callback being invoked on the server, i.e. authoritative handler
        GridMate::DataSet<int>::BindInterface<Handler, &Handler::OnDataOnServer, DataSetInvokeEverywhereTraits> m_dataSet1;
    };

    void run()
    {
        ReplicaPtr replica = Replica::CreateReplica("TestReplica");

        ReplicaChunkDescriptorTable::Get().RegisterChunkType<DataWithCustomTraitsAndCallingSetChunk>();
        AZStd::unique_ptr<DataWithCustomTraitsAndCallingSetChunk> chunk(CreateReplicaChunk<DataWithCustomTraitsAndCallingSetChunk>());
        replica->AttachReplicaChunk(chunk.get());

        // This should not crash on nullptr access (a handler isn't set)
        chunk->m_dataSet1.Set(1);

        chunk.release();
    }
};

}; // namespace UnitTest

GM_TEST_SUITE(ReplicaSmallSuite)
GM_TEST(ChunkAdd);
GM_TEST(ChunkCast);
GM_TEST(ChunkEvents);
GM_TEST(OfflineModeTest);
GM_TEST(DataSet_PrepareTest);
GM_TEST(DataSet_ACKTest);
GM_TEST(RpcNullHandlerCrash_Test);
GM_TEST(ReplicaChunkDescriptor_Rpc_Crash_Test);
GM_TEST(DataSet_AuthoritativeCallback_Test);
GM_TEST(DataSet_AuthoritativeCallback_Without_Handler_Test);
GM_TEST_SUITE_END()
