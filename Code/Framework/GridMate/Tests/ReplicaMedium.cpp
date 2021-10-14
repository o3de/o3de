/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "Tests.h"

#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/smart_ptr/scoped_ptr.h>

#include <GridMate/Carrier/DefaultSimulator.h>

#include <GridMate/Containers/unordered_map.h>
#include <GridMate/Containers/unordered_set.h>
#include <GridMate/Containers/vector.h>

#include <GridMate/Replica/Interpolators.h>
#include <GridMate/Replica/Replica.h>
#include <GridMate/Replica/ReplicaFunctions.h>
#include <GridMate/Replica/ReplicaMgr.h>
#include <GridMate/Replica/ReplicaDrillerEvents.h>
#include <GridMate/Replica/BasicHostChunkDescriptor.h>

#include <GridMate/Serialize/DataMarshal.h>
#include <GridMate/Serialize/CompressionMarshal.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

using namespace GridMate;

namespace UnitTest {

#define GM_REPLICA_TEST_SESSION_CHANNEL 1

// Example: DataSet<int> Data1 = { "Data1", 1 };
#define MAKE_DATASET(N) DataSet<int> Data ## N = { "Data" #N, N };

// Example: DataSet<bool> Data1 = { "Data1", false };
#define MAKE_BOOLDATASET(N) DataSet<bool> Data ## N = { "Data" #N, false }

// Example: DataSet<bool> Data1 = { "Data1", false };
#define MAKE_U8DATASET(N) DataSet<AZ::u8> Data ## N = { "Data" #N, 0 }

template<typename T1>
class IntrospectableRPC
    : public GridMate::Rpc<T1>
{
public:
    typedef Internal::VariadicStorage<T1> TypeTuple;

    template<class C, bool (C::* FuncPtr)(typename T1::Type, const RpcContext&), class Traits = RpcDefaultTraits>
    struct BindInterface
        : public Internal::RpcBindArgsWrapper<T1>::template BindInterface<Traits, TypeTuple, Internal::InterfaceResolver, C, FuncPtr>
    {
        typedef typename Internal::RpcBindArgsWrapper<T1>::template BindInterface<Traits, TypeTuple, Internal::InterfaceResolver, C, FuncPtr> ParentType;
        BindInterface(const char* debugName)
            : ParentType(debugName) { }
        typename ParentType::MarshalerSetType& GetMarshalers() { return ParentType::m_marshalers; }
    };
};

class RPCChunk
    : public ReplicaChunk
{
public:
    GM_CLASS_ALLOCATOR(RPCChunk);
    typedef AZStd::intrusive_ptr<RPCChunk> Ptr;
    static const char* GetChunkName() { return "RPCChunk"; }

    RPCChunk()
        : m_fromPrimaryBroadcast(0)
        , m_fromPrimaryNotBroadcast(0)
        , m_fromProxyBroadcast(0)
        , m_fromProxyNotBroadcast(0)
        , FromPrimaryBroadcast("FromPrimaryBroadcast")
        , FromPrimaryNotBroadcast("FromPrimaryNotBroadcast")
        , FromProxyBroadcast("FromProxyBroadcast")
        , FromProxyNotBroadcast("FromProxyNotBroadcast")
        , BroadcastInt("BroadcastInt")
    { }

    bool IsReplicaMigratable() override { return false; }

    bool FromPrimaryBroadcastFn(const RpcContext&)
    {
        AZ_TracePrintf("GridMate", "Executed FromPrimaryBroadcast %d %s\n", GetReplicaId(), GetReplica()->IsPrimary() ? "primary" : "proxy");
        m_fromPrimaryBroadcast++;
        return true;
    }

    bool FromPrimaryNotBroadcastFn(const RpcContext&)
    {
        AZ_TracePrintf("GridMate", "Executed FromPrimaryNotBroadcast %d %s\n", GetReplicaId(), GetReplica()->IsPrimary() ? "primary" : "proxy");
        m_fromPrimaryNotBroadcast++;
        return false;
    }

    bool FromProxyBroadcastFn(const RpcContext&)
    {
        AZ_TracePrintf("GridMate", "Executed FromProxyBroadcast %d %s\n", GetReplicaId(), GetReplica()->IsPrimary() ? "primary" : "proxy");
        m_fromProxyBroadcast++;
        return true;
    }

    bool FromProxyNotBroadcastFn(const RpcContext&)
    {
        AZ_TracePrintf("GridMate", "Executed FromProxyNotBroadcast %d %s\n", GetReplicaId(), GetReplica()->IsPrimary() ? "primary" : "proxy");
        m_fromProxyNotBroadcast++;
        return false;
    }

    bool BroadcastIntFn(int val, const RpcContext&)
    {
        m_sentData.push_back(val);
        return true;
    }

    int m_fromPrimaryBroadcast;
    int m_fromPrimaryNotBroadcast;
    int m_fromProxyBroadcast;
    int m_fromProxyNotBroadcast;
    AZStd::vector<int> m_sentData;

    Rpc<>::BindInterface<RPCChunk, & RPCChunk::FromPrimaryBroadcastFn> FromPrimaryBroadcast;
    Rpc<>::BindInterface<RPCChunk, & RPCChunk::FromPrimaryNotBroadcastFn> FromPrimaryNotBroadcast;
    Rpc<>::BindInterface<RPCChunk, & RPCChunk::FromProxyBroadcastFn> FromProxyBroadcast;
    Rpc<>::BindInterface<RPCChunk, & RPCChunk::FromProxyNotBroadcastFn> FromProxyNotBroadcast;
    Rpc<RpcArg<int> >::BindInterface<RPCChunk, & RPCChunk::BroadcastIntFn> BroadcastInt;
};

class FullRPCChunk
    : public ReplicaChunk
{
public:
    GM_CLASS_ALLOCATOR(FullRPCChunk);
    typedef AZStd::intrusive_ptr<FullRPCChunk> Ptr;
    static const char* GetChunkName() { return "FullRPCChunk"; }

    FullRPCChunk()
        : ZeroRPC("ZeroRPC")
        , OneRPC("OneRPC")
        , TwoRPC("TwoRPC")
        , ThreeRPC("ThreeRPC")
        , FourRPC("FourRPC")
        , FiveRPC("FiveRPC")
        , SixRPC("SixRPC")
        , SevenRPC("SevenRPC")
        , EightRPC("EightRPC")
        , NineRPC("NineRPC")
    { }

    bool IsReplicaMigratable() override { return false; }

    bool Zero(const RpcContext&)
    {
        auto& list = (IsPrimary() ? m_sentData : m_receivedData)[0];
        (void) list;
        return true;
    }

    bool One(AZ::u32 t1, const RpcContext&)
    {
        auto& list = (IsPrimary() ? m_sentData : m_receivedData)[1];
        list.push_back(t1);
        return true;
    }

    bool Two(AZ::u32 t1, AZ::u32 t2, const RpcContext&)
    {
        auto& list = (IsPrimary() ? m_sentData : m_receivedData)[2];
        list.push_back(t1);
        list.push_back(t2);
        return true;
    }

    bool Three(AZ::u32 t1, AZ::u32 t2, AZ::u32 t3, const RpcContext&)
    {
        auto& list = (IsPrimary() ? m_sentData : m_receivedData)[3];
        list.push_back(t1);
        list.push_back(t2);
        list.push_back(t3);
        return true;
    }

    bool Four(AZ::u32 t1, AZ::u32 t2, AZ::u32 t3, AZ::u32 t4, const RpcContext&)
    {
        auto& list = (IsPrimary() ? m_sentData : m_receivedData)[4];
        list.push_back(t1);
        list.push_back(t2);
        list.push_back(t3);
        list.push_back(t4);
        return true;
    }

    bool Five(AZ::u32 t1, AZ::u32 t2, AZ::u32 t3, AZ::u32 t4, AZ::u32 t5, const RpcContext&)
    {
        auto& list = (IsPrimary() ? m_sentData : m_receivedData)[5];
        list.push_back(t1);
        list.push_back(t2);
        list.push_back(t3);
        list.push_back(t4);
        list.push_back(t5);
        return true;
    }

    bool Six(AZ::u32 t1, AZ::u32 t2, AZ::u32 t3, AZ::u32 t4, AZ::u32 t5, AZ::u32 t6, const RpcContext&)
    {
        auto& list = (IsPrimary() ? m_sentData : m_receivedData)[6];
        list.push_back(t1);
        list.push_back(t2);
        list.push_back(t3);
        list.push_back(t4);
        list.push_back(t5);
        list.push_back(t6);
        return true;
    }

    bool Seven(AZ::u32 t1, AZ::u32 t2, AZ::u32 t3, AZ::u32 t4, AZ::u32 t5, AZ::u32 t6, AZ::u32 t7, const RpcContext&)
    {
        auto& list = (IsPrimary() ? m_sentData : m_receivedData)[7];
        list.push_back(t1);
        list.push_back(t2);
        list.push_back(t3);
        list.push_back(t4);
        list.push_back(t5);
        list.push_back(t6);
        list.push_back(t7);
        return true;
    }

    bool Eight(AZ::u32 t1, AZ::u32 t2, AZ::u32 t3, AZ::u32 t4, AZ::u32 t5, AZ::u32 t6, AZ::u32 t7, AZ::u32 t8, const RpcContext&)
    {
        auto& list = (IsPrimary() ? m_sentData : m_receivedData)[8];
        list.push_back(t1);
        list.push_back(t2);
        list.push_back(t3);
        list.push_back(t4);
        list.push_back(t5);
        list.push_back(t6);
        list.push_back(t7);
        list.push_back(t8);
        return true;
    }

    bool Nine(AZ::u32 t1, AZ::u32 t2, AZ::u32 t3, AZ::u32 t4, AZ::u32 t5, AZ::u32 t6, AZ::u32 t7, AZ::u32 t8, AZ::u32 t9, const RpcContext&)
    {
        auto& list = (IsPrimary() ? m_sentData : m_receivedData)[9];
        list.push_back(t1);
        list.push_back(t2);
        list.push_back(t3);
        list.push_back(t4);
        list.push_back(t5);
        list.push_back(t6);
        list.push_back(t7);
        list.push_back(t8);
        list.push_back(t9);
        return true;
    }

    unordered_map<AZ::u32, vector<AZ::u32> > m_sentData;
    unordered_map<AZ::u32, vector<AZ::u32> > m_receivedData;

    Rpc<>::BindInterface<FullRPCChunk, & FullRPCChunk::Zero> ZeroRPC;
    Rpc<RpcArg<AZ::u32> >::BindInterface<FullRPCChunk, & FullRPCChunk::One> OneRPC;
    Rpc<RpcArg<AZ::u32>, RpcArg<AZ::u32> >::BindInterface<FullRPCChunk, & FullRPCChunk::Two> TwoRPC;
    Rpc<RpcArg<AZ::u32>, RpcArg<AZ::u32>, RpcArg<AZ::u32> >::BindInterface<FullRPCChunk, & FullRPCChunk::Three> ThreeRPC;
    Rpc<RpcArg<AZ::u32>, RpcArg<AZ::u32>, RpcArg<AZ::u32>, RpcArg<AZ::u32> >::BindInterface<FullRPCChunk, & FullRPCChunk::Four> FourRPC;
    Rpc<RpcArg<AZ::u32>, RpcArg<AZ::u32>, RpcArg<AZ::u32>, RpcArg<AZ::u32>, RpcArg<AZ::u32> >::BindInterface<FullRPCChunk, & FullRPCChunk::Five> FiveRPC;
    Rpc<RpcArg<AZ::u32>, RpcArg<AZ::u32>, RpcArg<AZ::u32>, RpcArg<AZ::u32>, RpcArg<AZ::u32>, RpcArg<AZ::u32> >::BindInterface<FullRPCChunk, & FullRPCChunk::Six> SixRPC;
    Rpc<RpcArg<AZ::u32>, RpcArg<AZ::u32>, RpcArg<AZ::u32>, RpcArg<AZ::u32>, RpcArg<AZ::u32>, RpcArg<AZ::u32>, RpcArg<AZ::u32> >::BindInterface<FullRPCChunk, & FullRPCChunk::Seven> SevenRPC;
    Rpc<RpcArg<AZ::u32>, RpcArg<AZ::u32>, RpcArg<AZ::u32>, RpcArg<AZ::u32>, RpcArg<AZ::u32>, RpcArg<AZ::u32>, RpcArg<AZ::u32>, RpcArg<AZ::u32> >::BindInterface<FullRPCChunk, & FullRPCChunk::Eight> EightRPC;
    Rpc<RpcArg<AZ::u32>, RpcArg<AZ::u32>, RpcArg<AZ::u32>, RpcArg<AZ::u32>, RpcArg<AZ::u32>, RpcArg<AZ::u32>, RpcArg<AZ::u32>, RpcArg<AZ::u32>, RpcArg<AZ::u32> >::BindInterface<FullRPCChunk, & FullRPCChunk::Nine> NineRPC;
};

class DataSetChunk
    : public ReplicaChunk
{
public:
    GM_CLASS_ALLOCATOR(DataSetChunk);

    DataSetChunk()
        : Data1("Data1", 0)
    {
    }

    typedef AZStd::intrusive_ptr<DataSetChunk> Ptr;
    static const char* GetChunkName() { return "DataSetChunk"; }

    bool IsReplicaMigratable() override { return false; }

    void IntHandler(const int& val, const TimeContext&)
    {
        m_changedData.push_back(val);
    }

    AZStd::vector<int> m_changedData;

    DataSet<int>::BindInterface<DataSetChunk, & DataSetChunk::IntHandler> Data1;
};

class MixedTestChunk
    : public ReplicaChunk
{
public:
    GM_CLASS_ALLOCATOR(MixedTestChunk);

    MixedTestChunk()
    {
    }

    typedef AZStd::intrusive_ptr<MixedTestChunk> Ptr;
    static const char* GetChunkName() { return "MixedTestChunk"; }

    bool IsReplicaMigratable() override { return false; }

    DataSet<AZ::u64> Data1 = { "Data1", 42 };
    DataSet<AZ::u64> Data2 = { "Data2", 0 };
};

class LargeChunkWithDefaultsMedium
    : public ReplicaChunk
{
public:
    GM_CLASS_ALLOCATOR(LargeChunkWithDefaultsMedium);

    LargeChunkWithDefaultsMedium()
    {
    }

    typedef AZStd::intrusive_ptr<LargeChunkWithDefaultsMedium> Ptr;
    static const char* GetChunkName() { return "LargeChunkWithDefaultsMedium"; }
    bool IsReplicaMigratable() override { return false; }

    MAKE_DATASET(1)
        MAKE_DATASET(2)
        MAKE_DATASET(3)
        MAKE_DATASET(4)
        MAKE_DATASET(5)
        MAKE_DATASET(6)
        MAKE_DATASET(7)
        MAKE_DATASET(8)
        MAKE_DATASET(9)
        MAKE_DATASET(10)
};

class ChunkWithBools
    : public ReplicaChunk
{
public:
    GM_CLASS_ALLOCATOR(ChunkWithBools);

    ChunkWithBools()
    {
    }

    typedef AZStd::intrusive_ptr<ChunkWithBools> Ptr;
    static const char* GetChunkName() { return "ChunkWithBools"; }
    bool IsReplicaMigratable() override { return false; }

    MAKE_BOOLDATASET(1);
    MAKE_BOOLDATASET(2);
    MAKE_BOOLDATASET(3);
    MAKE_BOOLDATASET(4);
    MAKE_BOOLDATASET(5);
    MAKE_BOOLDATASET(6);
    MAKE_BOOLDATASET(7);
    MAKE_BOOLDATASET(8);
    MAKE_BOOLDATASET(9);
    MAKE_BOOLDATASET(10);
};

class ChunkWithShortInts
    : public ReplicaChunk
{
public:
    GM_CLASS_ALLOCATOR(ChunkWithShortInts);

    ChunkWithShortInts()
    {
    }

    typedef AZStd::intrusive_ptr<ChunkWithShortInts> Ptr;
    static const char* GetChunkName() { return "ChunkWithShortInts"; }
    bool IsReplicaMigratable() override { return false; }

    MAKE_U8DATASET(1);
    MAKE_U8DATASET(2);
    MAKE_U8DATASET(3);
    MAKE_U8DATASET(4);
    MAKE_U8DATASET(5);
    MAKE_U8DATASET(6);
    MAKE_U8DATASET(7);
    MAKE_U8DATASET(8);
    MAKE_U8DATASET(9);
    MAKE_U8DATASET(10);
};

class SourcePeerChunk
    : public ReplicaChunk
{
public:
    GM_CLASS_ALLOCATOR(SourcePeerChunk);

    SourcePeerChunk()
        : PeerRPC("PeerRPC")
        , ForwardPeerRPC("ForwardPeerRPC")
    { }

    typedef AZStd::intrusive_ptr<SourcePeerChunk> Ptr;
    static const char* GetChunkName() { return "SourcePeerChunk"; }

    bool IsReplicaMigratable() override { return false; }

    bool Peer(const RpcContext& context)
    {
        m_peers.push_back(context.m_sourcePeer);
        return true;
    }

    bool ForwardPeer(const RpcContext& context)
    {
        m_forwardPeers.push_back(context.m_sourcePeer);
        return true;
    }

    AZStd::vector<PeerId> m_peers;
    AZStd::vector<PeerId> m_forwardPeers;

    struct Traits
        : public RpcDefaultTraits
    {
        static const bool s_alwaysForwardSourcePeer = true;
    };

    Rpc<>::BindInterface<SourcePeerChunk, & SourcePeerChunk::Peer> PeerRPC;
    Rpc<>::BindInterface<SourcePeerChunk, & SourcePeerChunk::ForwardPeer, Traits> ForwardPeerRPC;
};

class CustomHandler
    : public ReplicaChunkInterface
{
public:
    GM_CLASS_ALLOCATOR(CustomHandler);

    void DataSetHandler(const int& val, const TimeContext&)
    {
        m_dataset.push_back(val);
    }

    bool RpcHandler(AZ::u32 t1, const RpcContext&)
    {
        m_rpc.push_back(t1);
        return true;
    }


    AZStd::vector<int> m_dataset;
    AZStd::vector<int> m_rpc;
};

class CustomHandlerChunk
    : public ReplicaChunkBase
{
public:
    GM_CLASS_ALLOCATOR(CustomHandlerChunk);

    CustomHandlerChunk()
        : Data("Data", 0)
        , RPC("RPC")
    {
    }

    typedef AZStd::intrusive_ptr<CustomHandlerChunk> Ptr;
    static const char* GetChunkName() { return "CustomHandlerChunk"; }

    bool IsReplicaMigratable() override { return false; }

    DataSet<int>::BindInterface<CustomHandler, & CustomHandler::DataSetHandler> Data;
    Rpc<RpcArg<AZ::u32> >::BindInterface<CustomHandler, & CustomHandler::RpcHandler> RPC;
};

class AllEventChunk
    : public ReplicaChunk
{
public:
    GM_CLASS_ALLOCATOR(AllEventChunk);

    AllEventChunk()
        : m_attaches(0)
        , m_detaches(0)
        , m_activates(0)
        , m_deactivates(0)
    {
    }

    typedef AZStd::intrusive_ptr<AllEventChunk> Ptr;
    static const char* GetChunkName() { return "AllEventChunk"; }

    bool IsReplicaMigratable() override { return false; }

    void OnAttachedToReplica(Replica*) override
    {
        m_attaches++;
    }

    void OnDetachedFromReplica(Replica*) override
    {
        m_detaches++;
    }

    void OnReplicaActivate(const ReplicaContext&) override
    {
        m_activates++;
    }

    void OnReplicaDeactivate(const ReplicaContext&) override
    {
        m_deactivates++;
    }

    int m_attaches;
    int m_detaches;
    int m_activates;
    int m_deactivates;
};

class DrillerTestChunk
    : public ReplicaChunk
{
public:
    GM_CLASS_ALLOCATOR(DrillerTestChunk);
    typedef AZStd::intrusive_ptr<DrillerTestChunk> Ptr;
    static const char* GetChunkName() { return "DrillerTestChunk"; }

    DrillerTestChunk() { }

    bool IsReplicaMigratable() override
    {
        return true;
    }
};

class NonConstMarshaler
{
public:
    typedef AZ::u32 DataType;
    typedef AZ::u32 SerializedType;

    static const AZStd::size_t MarshalSize = sizeof(SerializedType);

    NonConstMarshaler()
        : m_valueRead(0)
        , m_valueWritten(0) { }

    void Marshal(WriteBuffer& wb, const DataType& value)
    {
        wb.Write(value);
        m_valueWritten += value;
    }
    void Unmarshal(DataType& value, ReadBuffer& rb)
    {
        if (rb.Read(value))
        {
            m_valueRead += value;
        }
    }

    AZ::u32 m_valueRead;
    AZ::u32 m_valueWritten;
};

class NonConstMarshalerChunk
    : public ReplicaChunk
{
public:
    GM_CLASS_ALLOCATOR(NonConstMarshalerChunk);
    typedef AZStd::intrusive_ptr<NonConstMarshalerChunk> Ptr;
    static const char* GetChunkName() { return "NonConstMarshalerChunk"; }

    NonConstMarshalerChunk()
        : m_data("data", 0)
        , RPCTestRPC("RPC")
    { }

    bool IsReplicaMigratable() override { return false; }

    bool RPCTest(AZ::u32, const RpcContext&)
    {
        return true;
    }

    DataSet<AZ::u32, NonConstMarshaler> m_data;
    IntrospectableRPC<RpcArg<AZ::u32, NonConstMarshaler> >::BindInterface<NonConstMarshalerChunk, & NonConstMarshalerChunk::RPCTest> RPCTestRPC;
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
class MPSessionMedium
    : public CarrierEventBus::Handler
{
public:

    ~MPSessionMedium() override
    {
        CarrierEventBus::Handler::BusDisconnect();
    }

    ReplicaManager& GetReplicaMgr() { return m_rm; }
    void            SetTransport(Carrier* transport) { m_pTransport = transport; CarrierEventBus::Handler::BusConnect(transport->GetGridMate()); }
    Carrier*        GetTransport() { return m_pTransport; }
    void            SetClient(bool isClient) { m_client = isClient; }
    void            AcceptConn(bool accept) { m_acceptConn = accept; }

    void Update()
    {
        char buf[1500];
        for (ConnectionSet::iterator iConn = m_connections.begin(); iConn != m_connections.end(); ++iConn)
        {
            ConnectionID conn = *iConn;
            Carrier::ReceiveResult result = m_pTransport->Receive(buf, 1500, conn, GM_REPLICA_TEST_SESSION_CHANNEL);
            if (result.m_state == Carrier::ReceiveResult::RECEIVED)
            {
                if (strcmp(buf, "IM_A_CLIENT") == 0)
                {
                    m_rm.AddPeer(conn, Mode_Client);
                }
                else if (strcmp(buf, "IM_A_PEER") == 0)
                {
                    m_rm.AddPeer(conn, Mode_Peer);
                }
            }
        }
    }

    template<typename T>
    typename T::Ptr GetChunkFromReplica(ReplicaId id)
    {
        ReplicaPtr replica = GetReplicaMgr().FindReplica(id);
        if (!replica)
        {
            return nullptr;
        }
        return replica->FindReplicaChunk<T>();
    }

    //////////////////////////////////////////////////////////////////////////
    // CarrierEventBus
    void OnConnectionEstablished(Carrier* carrier, ConnectionID id) override
    {
        if (carrier != m_pTransport)
        {
            return; // not for us
        }
        m_connections.insert(id);
        if (m_client)
        {
            m_pTransport->Send("IM_A_CLIENT", 12, id, Carrier::SEND_RELIABLE, Carrier::PRIORITY_NORMAL, GM_REPLICA_TEST_SESSION_CHANNEL);
        }
        else
        {
            m_pTransport->Send("IM_A_PEER", 10, id, Carrier::SEND_RELIABLE, Carrier::PRIORITY_NORMAL, GM_REPLICA_TEST_SESSION_CHANNEL);
        }
    }

    void OnDisconnect(Carrier* carrier, ConnectionID id, CarrierDisconnectReason /*reason*/) override
    {
        if (carrier != m_pTransport)
        {
            return; // not for us
        }
        m_rm.RemovePeer(id);
        m_connections.erase(id);
    }

    void OnDriverError(Carrier* carrier, ConnectionID id, const DriverError& error) override
    {
        (void)error;
        if (carrier != m_pTransport)
        {
            return; // not for us
        }
        m_pTransport->Disconnect(id);
    }

    void OnSecurityError(Carrier* carrier, ConnectionID id, const SecurityError& error) override
    {
        (void)carrier;
        (void)id;
        (void)error;
        //Ignore security warnings in unit tests
    }
    //////////////////////////////////////////////////////////////////////////

    ReplicaManager              m_rm;
    Carrier*                    m_pTransport;
    typedef unordered_set<ConnectionID> ConnectionSet;
    ConnectionSet               m_connections;
    bool                        m_client;
    bool                        m_acceptConn;
};

const int k_delay = 100;

enum class TestStatus
{
    Running,
    Completed,
};

class SimpleTest
    : public UnitTest::GridMateMPTestFixture
    , public ::testing::Test
{
public:
    //GM_CLASS_ALLOCATOR(SimpleTest);

    SimpleTest()
        : m_sessionCount(0) { }

    virtual int GetNumSessions() { return 0; }
    virtual int GetHostSession() { return 0; }
    virtual void PreInit() { }
    virtual void PreConnect() { }
    virtual void PostInit() { }

    void SetUp() override
    {
        AZ_TracePrintf("GridMate", "\n");

        if (!ReplicaChunkDescriptorTable::Get().FindReplicaChunkDescriptor(ReplicaChunkClassId(RPCChunk::GetChunkName())))
        {
            ReplicaChunkDescriptorTable::Get().RegisterChunkType<RPCChunk>();
        }
        if (!ReplicaChunkDescriptorTable::Get().FindReplicaChunkDescriptor(ReplicaChunkClassId(FullRPCChunk::GetChunkName())))
        {
            ReplicaChunkDescriptorTable::Get().RegisterChunkType<FullRPCChunk>();
        }
        if (!ReplicaChunkDescriptorTable::Get().FindReplicaChunkDescriptor(ReplicaChunkClassId(DataSetChunk::GetChunkName())))
        {
            ReplicaChunkDescriptorTable::Get().RegisterChunkType<DataSetChunk>();
        }
        if (!ReplicaChunkDescriptorTable::Get().FindReplicaChunkDescriptor(ReplicaChunkClassId(AllEventChunk::GetChunkName())))
        {
            ReplicaChunkDescriptorTable::Get().RegisterChunkType<AllEventChunk>();
        }
        if (!ReplicaChunkDescriptorTable::Get().FindReplicaChunkDescriptor(ReplicaChunkClassId(DrillerTestChunk::GetChunkName())))
        {
            ReplicaChunkDescriptorTable::Get().RegisterChunkType<DrillerTestChunk>();
        }
        if (!ReplicaChunkDescriptorTable::Get().FindReplicaChunkDescriptor(ReplicaChunkClassId(NonConstMarshalerChunk::GetChunkName())))
        {
            ReplicaChunkDescriptorTable::Get().RegisterChunkType<NonConstMarshalerChunk>();
        }
        if (!ReplicaChunkDescriptorTable::Get().FindReplicaChunkDescriptor(ReplicaChunkClassId(CustomHandlerChunk::GetChunkName())))
        {
            ReplicaChunkDescriptorTable::Get().RegisterChunkType<CustomHandlerChunk>();
        }
        if (!ReplicaChunkDescriptorTable::Get().FindReplicaChunkDescriptor(ReplicaChunkClassId(SourcePeerChunk::GetChunkName())))
        {
            ReplicaChunkDescriptorTable::Get().RegisterChunkType<SourcePeerChunk>();
        }
        if (!ReplicaChunkDescriptorTable::Get().FindReplicaChunkDescriptor(ReplicaChunkClassId(MixedTestChunk::GetChunkName())))
        {
            ReplicaChunkDescriptorTable::Get().RegisterChunkType<MixedTestChunk>();
        }
        if (!ReplicaChunkDescriptorTable::Get().FindReplicaChunkDescriptor(ReplicaChunkClassId(LargeChunkWithDefaultsMedium::GetChunkName())))
        {
            ReplicaChunkDescriptorTable::Get().RegisterChunkType<LargeChunkWithDefaultsMedium>();
        }
        if (!ReplicaChunkDescriptorTable::Get().FindReplicaChunkDescriptor(ReplicaChunkClassId(ChunkWithBools::GetChunkName())))
        {
            ReplicaChunkDescriptorTable::Get().RegisterChunkType<ChunkWithBools>();
        }
        if (!ReplicaChunkDescriptorTable::Get().FindReplicaChunkDescriptor(ReplicaChunkClassId(ChunkWithShortInts::GetChunkName())))
        {
            ReplicaChunkDescriptorTable::Get().RegisterChunkType<ChunkWithShortInts>();
        }
    }

    void TearDown() override
    {
        for (int i = 0; i < m_sessionCount; ++i)
        {
            m_sessions[i].GetReplicaMgr().Shutdown();
            DefaultCarrier::Destroy(m_sessions[i].GetTransport());
        }
    }

    void RunTickLoop(AZStd::function<TestStatus(int)> tickBody)
    {
        // Setting up simulator with 50% outgoing packet loss
        m_defaultSimulator = AZStd::make_unique<DefaultSimulator>();
        m_defaultSimulator->SetOutgoingPacketLoss(0, 0);

        m_sessionCount = GetNumSessions();

        PreInit();

        // initialize transport
        int basePort = 4427;
        for (int i = 0; i < m_sessionCount; ++i)
        {
            TestCarrierDesc desc;
            desc.m_port = basePort + i;
            desc.m_enableDisconnectDetection = false;
            desc.m_simulator = m_defaultSimulator.get();

            // initialize replica managers
            m_sessions[i].SetTransport(DefaultCarrier::Create(desc, m_gridMate));
            m_sessions[i].AcceptConn(true);
            m_sessions[i].SetClient(false);
            m_sessions[i].GetReplicaMgr().Init(ReplicaMgrDesc(i + 1, m_sessions[i].GetTransport(), 0, i == GetHostSession() ? ReplicaMgrDesc::Role_SyncHost : 0));
            m_sessions[i].GetReplicaMgr().RegisterUserContext(12345, reinterpret_cast<void*>(static_cast<size_t>(i + 1)));
        }
        m_sessions[GetHostSession()].GetReplicaMgr().SetLocalLagAmt(1);

        PreConnect();

        for (int i = 1; i < m_sessionCount; ++i)
        {
            m_sessions[i].GetTransport()->Connect("127.0.0.1", basePort);
        }

        PostInit();

        // main test loop
        int count = 0;
        for (;; )
        {
            if (tickBody(count) == TestStatus::Completed)
            {
                break;
            }

            ++count;
            // tick everything
            for (int i = 0; i < m_sessionCount; ++i)
            {
                m_sessions[i].Update();
                m_sessions[i].GetReplicaMgr().Unmarshal();
            }
            for (int i = 0; i < m_sessionCount; ++i)
            {
                m_sessions[i].GetReplicaMgr().UpdateReplicas();
            }
            for (int i = 0; i < m_sessionCount; ++i)
            {
                m_sessions[i].GetReplicaMgr().UpdateFromReplicas();
                m_sessions[i].GetReplicaMgr().Marshal();
            }
            for (int i = 0; i < m_sessionCount; ++i)
            {
                m_sessions[i].GetTransport()->Update();
            }
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(k_delay));
        }
    }

    int m_sessionCount;
    AZStd::array<MPSessionMedium, 10> m_sessions;
    AZStd::unique_ptr<DefaultSimulator> m_defaultSimulator;
};

class ReplicaChunkRPCExec
    : public SimpleTest
{
public:
    ReplicaChunkRPCExec()
        : m_chunk(nullptr)
        , m_replicaId(0)
    { }

    enum
    {
        sHost,
        s2,
        s3,
        nSessions
    };

    int GetNumSessions() override { return nSessions; }

    void PreConnect() override
    {
        // put something on s1 to get it going
        auto replica = Replica::CreateReplica(nullptr);
        m_chunk = CreateAndAttachReplicaChunk<RPCChunk>(replica);
        m_replicaId = m_sessions[sHost].GetReplicaMgr().AddPrimary(replica);
    }

    RPCChunk::Ptr m_chunk;
    ReplicaId m_replicaId;
};

TEST_F(ReplicaChunkRPCExec, DISABLED_ReplicaChunkRPCExec)
{
    RunTickLoop([this](int tick) -> TestStatus
    {
        // perform some random actions on a timeline
        switch (tick)
        {
        case 10:
            m_chunk->FromPrimaryBroadcast();
            break;

        case 20:
            m_chunk->FromPrimaryNotBroadcast();
            break;

        case 30:
        {
            auto r = m_sessions[s2].GetReplicaMgr().FindReplica(m_replicaId);
            r->FindReplicaChunk<RPCChunk>()->FromProxyBroadcast();
            break;
        }

        case 40:
        {
            auto r = m_sessions[s2].GetReplicaMgr().FindReplica(m_replicaId);
            r->FindReplicaChunk<RPCChunk>()->FromProxyNotBroadcast();
            break;
        }

        case 50:
        {
            auto s1host = m_sessions[sHost].GetReplicaMgr().FindReplica(m_replicaId)->FindReplicaChunk<RPCChunk>();
            auto s2proxy = m_sessions[s2].GetReplicaMgr().FindReplica(m_replicaId)->FindReplicaChunk<RPCChunk>();
            auto s3proxy = m_sessions[s3].GetReplicaMgr().FindReplica(m_replicaId)->FindReplicaChunk<RPCChunk>();

            AZ_TEST_ASSERT(s1host->m_fromPrimaryBroadcast == 1);
            AZ_TEST_ASSERT(s2proxy->m_fromPrimaryBroadcast == 1);
            AZ_TEST_ASSERT(s3proxy->m_fromPrimaryBroadcast == 1);

            AZ_TEST_ASSERT(s1host->m_fromPrimaryNotBroadcast == 1);
            AZ_TEST_ASSERT(s2proxy->m_fromPrimaryNotBroadcast == 0);
            AZ_TEST_ASSERT(s3proxy->m_fromPrimaryNotBroadcast == 0);

            AZ_TEST_ASSERT(s1host->m_fromProxyBroadcast == 1);
            AZ_TEST_ASSERT(s2proxy->m_fromProxyBroadcast == 1);
            AZ_TEST_ASSERT(s3proxy->m_fromProxyBroadcast == 1);

            AZ_TEST_ASSERT(s1host->m_fromProxyNotBroadcast == 1);
            AZ_TEST_ASSERT(s2proxy->m_fromProxyNotBroadcast == 0);
            AZ_TEST_ASSERT(s3proxy->m_fromProxyNotBroadcast == 0);

            return TestStatus::Completed;
        }
        default:
            return TestStatus::Running;
        }
        return TestStatus::Running;
    });
}

//-----------------------------------------------------------------------------
class DestroyRPCChunk
    : public ReplicaChunk
{
public:
    GM_CLASS_ALLOCATOR(DestroyRPCChunk);

    DestroyRPCChunk()
        : DestroyFromPrimary("DestroyFromPrimary")
        , DestroyFromProxy("DestroyFromProxy")
        , BeforeDestroyFromProxy("BeforeDestroyFromProxy")
        , AfterDestroyFromProxy("AfterDestroyFromProxy")
        , BeforeDestroyFromPrimary("BeforeDestroyFromPrimary")
        , AfterDestroyFromPrimary("AfterDestroyFromPrimary")
    {
    }

    typedef AZStd::intrusive_ptr<DestroyRPCChunk> Ptr;
    static const char* GetChunkName() { return "DestroyRPCChunk"; }

    bool IsReplicaMigratable() override { return false; }

    bool DestroyFromPrimaryFn(const RpcContext&)
    {
        AZ_TracePrintf("GridMate", "Executed DestroyFromPrimary %d %s\n", GetReplicaId(), GetReplica()->IsPrimary() ? "primary" : "proxy");
        ++s_destroyFromPrimaryCalls;
        if (GetReplica()->IsPrimary())
        {
            GetReplica()->Destroy();
        }
        return true;
    }

    bool DestroyFromProxyFn(const RpcContext&)
    {
        AZ_TracePrintf("GridMate", "Executed DestroyFromProxy %d %s\n", GetReplicaId(), GetReplica()->IsPrimary() ? "primary" : "proxy");
        ++s_destroyFromProxyCalls;
        if (GetReplica()->IsPrimary())
        {
            GetReplica()->Destroy();
        }
        return true;
    }

    bool BeforeDestroyFromProxyFn(const RpcContext&)
    {
        AZ_TracePrintf("GridMate", "Executed BeforeDestroyFromProxy %d %s\n", GetReplicaId(), GetReplica()->IsPrimary() ? "primary" : "proxy");
        ++s_beforeDestroyFromProxyCalls;
        return true;
    }

    bool AfterDestroyFromProxyFn(const RpcContext&)
    {
        AZ_TracePrintf("GridMate", "Executed AfterDestroyFromProxy %d %s\n", GetReplicaId(), GetReplica()->IsPrimary() ? "primary" : "proxy");
        ++s_afterDestroyFromProxyCalls;
        return true;
    }

    bool BeforeDestroyFromPrimaryFn(const RpcContext&)
    {
        AZ_TracePrintf("GridMate", "Executed BeforeDestroyFromPrimary %d %s\n", GetReplicaId(), GetReplica()->IsPrimary() ? "primary" : "proxy");
        ++s_beforeDestroyFromPrimaryCalls;
        return true;
    }

    bool AfterDestroyFromPrimaryFn(const RpcContext&)
    {
        AZ_TracePrintf("GridMate", "Executed AfterDestroyFromPrimary %d %s\n", GetReplicaId(), GetReplica()->IsPrimary() ? "primary" : "proxy");
        ++s_afterDestroyFromPrimaryCalls;
        return true;
    }

    Rpc<>::BindInterface<DestroyRPCChunk, & DestroyRPCChunk::DestroyFromPrimaryFn> DestroyFromPrimary;
    Rpc<>::BindInterface<DestroyRPCChunk, & DestroyRPCChunk::DestroyFromProxyFn> DestroyFromProxy;
    Rpc<>::BindInterface<DestroyRPCChunk, & DestroyRPCChunk::BeforeDestroyFromProxyFn> BeforeDestroyFromProxy;
    Rpc<>::BindInterface<DestroyRPCChunk, & DestroyRPCChunk::AfterDestroyFromProxyFn> AfterDestroyFromProxy;
    Rpc<>::BindInterface<DestroyRPCChunk, & DestroyRPCChunk::BeforeDestroyFromPrimaryFn> BeforeDestroyFromPrimary;
    Rpc<>::BindInterface<DestroyRPCChunk, & DestroyRPCChunk::AfterDestroyFromPrimaryFn> AfterDestroyFromPrimary;

    static int s_destroyFromPrimaryCalls;
    static int s_beforeDestroyFromPrimaryCalls;
    static int s_afterDestroyFromPrimaryCalls;
    static int s_destroyFromProxyCalls;
    static int s_beforeDestroyFromProxyCalls;
    static int s_afterDestroyFromProxyCalls;
};

int DestroyRPCChunk::s_destroyFromProxyCalls = 0;
int DestroyRPCChunk::s_destroyFromPrimaryCalls = 0;
int DestroyRPCChunk::s_beforeDestroyFromProxyCalls = 0;
int DestroyRPCChunk::s_afterDestroyFromProxyCalls = 0;
int DestroyRPCChunk::s_beforeDestroyFromPrimaryCalls = 0;
int DestroyRPCChunk::s_afterDestroyFromPrimaryCalls = 0;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

class ReplicaDestroyedInRPC
    : public SimpleTest
{
public:
    enum
    {
        sHost,
        s2,
        s3,
        nSessions
    };

    int GetNumSessions() override { return nSessions; }

    void PreConnect() override
    {
        ReplicaChunkDescriptorTable::Get().RegisterChunkType<DestroyRPCChunk>();

        // creating 2 replicas on host
        for (int i = 0; i < 2; ++i)
        {
            auto replica = Replica::CreateReplica(nullptr);
            CreateAndAttachReplicaChunk<DestroyRPCChunk>(replica);
            m_repId[i] = m_sessions[sHost].GetReplicaMgr().AddPrimary(replica);
        }
    }

    ReplicaId m_repId[2];
};

TEST_F(ReplicaDestroyedInRPC, DISABLED_ReplicaDestroyedInRPC)
{
    RunTickLoop([this](int tick)->TestStatus
    {
        switch (tick)
        {
        case 10:
        {
            // calling destroy on primary
            auto primary = m_sessions[sHost].GetReplicaMgr().FindReplica(m_repId[0]);
            auto primaryChunk = primary->FindReplicaChunk<DestroyRPCChunk>();
            primaryChunk->BeforeDestroyFromPrimary();
            primaryChunk->DestroyFromPrimary();
            primaryChunk->AfterDestroyFromPrimary();

            // calling destroy on proxy
            auto proxy = m_sessions[s2].GetReplicaMgr().FindReplica(m_repId[1]);
            auto proxyChunk = proxy->FindReplicaChunk<DestroyRPCChunk>();
            proxyChunk->BeforeDestroyFromProxy();
            proxyChunk->DestroyFromProxy();
            proxyChunk->AfterDestroyFromProxy();

            break;
        }

        case 20:
        {
            // checking if before destroy RPC was called on every peer
            AZ_TEST_ASSERT(DestroyRPCChunk::s_beforeDestroyFromProxyCalls == nSessions);
            AZ_TEST_ASSERT(DestroyRPCChunk::s_beforeDestroyFromPrimaryCalls == nSessions);

            // checking if destroy itself was called on every peer
            AZ_TEST_ASSERT(DestroyRPCChunk::s_destroyFromProxyCalls == nSessions);
            AZ_TEST_ASSERT(DestroyRPCChunk::s_destroyFromPrimaryCalls == nSessions);

            // checking if after destroy RPC was never called
            AZ_TEST_ASSERT(DestroyRPCChunk::s_afterDestroyFromProxyCalls == 0);     // RPCs that arrive via the network after deactivation should be dropped.
            AZ_TEST_ASSERT(DestroyRPCChunk::s_afterDestroyFromPrimaryCalls == 1);     // RPCs explicitly called on an inactive replica should still be executed.

            return TestStatus::Completed;
        }
        default:
            return TestStatus::Running;
        }

        return TestStatus::Running;
    });
}

class ReplicaChunkAddWhileReplicated
    : public SimpleTest
{
public:
    ReplicaChunkAddWhileReplicated()
        : m_replica(nullptr)
        , m_chunk(nullptr)
        , m_replicaId(0)
    {
    }

    enum
    {
        sHost,
        s2,
        nSessions
    };

    int GetNumSessions() override { return nSessions; }

    void PreConnect() override
    {
        // put something on s1 to get it going
        m_replica = Replica::CreateReplica(nullptr);
        m_replicaId = m_sessions[sHost].GetReplicaMgr().AddPrimary(m_replica);
    }

    ReplicaPtr m_replica;
    RPCChunk::Ptr m_chunk;
    ReplicaId m_replicaId;
};

TEST_F(ReplicaChunkAddWhileReplicated, DISABLED_ReplicaChunkAddWhileReplicated)
{
    RunTickLoop([this](int tick)-> TestStatus
    {
        // perform some random actions on a timeline
        switch (tick)
        {
        case 10:
        {
            auto repHost = m_sessions[sHost].GetReplicaMgr().FindReplica(m_replicaId);
            AZ_TEST_ASSERT(!repHost->FindReplicaChunk<RPCChunk>());
            AZ_TEST_ASSERT(repHost->GetNumChunks() == 1);

            auto rep = m_sessions[s2].GetReplicaMgr().FindReplica(m_replicaId);
            AZ_TEST_ASSERT(!rep->FindReplicaChunk<RPCChunk>());
            AZ_TEST_ASSERT(rep->GetNumChunks() == 1);
            break;
        }

        case 20:
            m_chunk = CreateAndAttachReplicaChunk<RPCChunk>(m_replica);
            break;

        case 40:
        {
            auto repHost = m_sessions[sHost].GetReplicaMgr().FindReplica(m_replicaId);
            AZ_TEST_ASSERT(!repHost->FindReplicaChunk<RPCChunk>());
            AZ_TEST_ASSERT(repHost->GetNumChunks() == 1);

            auto rep = m_sessions[s2].GetReplicaMgr().FindReplica(m_replicaId);
            AZ_TEST_ASSERT(!rep->FindReplicaChunk<RPCChunk>());
            AZ_TEST_ASSERT(rep->GetNumChunks() == 1);
            return TestStatus::Completed;
        }
        default:
            return TestStatus::Running;
        }
        return TestStatus::Running;
    });
}


class ReplicaRPCValues
    : public SimpleTest
{
public:
    ReplicaRPCValues()
        : m_replica(nullptr)
        , m_chunk(nullptr)
        , m_replicaId(0)
    {
    }

    enum
    {
        sHost,
        s2,
        nSessions
    };

    int GetNumSessions() override { return nSessions; }

    void PreConnect() override
    {
        // put something on s1 to get it going
        m_replica = Replica::CreateReplica(nullptr);
        m_chunk = CreateAndAttachReplicaChunk<RPCChunk>(m_replica);
        m_replicaId = m_sessions[sHost].GetReplicaMgr().AddPrimary(m_replica);
    }

    ReplicaPtr m_replica;
    RPCChunk::Ptr m_chunk;
    ReplicaId m_replicaId;
};

TEST_F(ReplicaRPCValues, DISABLED_ReplicaRPCValues)
{
    RunTickLoop([this](int tick)-> TestStatus
    {
        if (tick < 100)
        {
            if (!(tick % 10))
            {
                m_chunk->BroadcastInt(tick);
            }
            else if (!(tick % 20))
            {
                auto rep = m_sessions[s2].GetReplicaMgr().FindReplica(m_replicaId);
                AZ_TEST_ASSERT(rep->FindReplicaChunk<RPCChunk>()->m_sentData.back() == (tick - 10));
            }
            return TestStatus::Running;
        }
        return TestStatus::Completed;
    });
}

class FullRPCValues
    : public SimpleTest
{
public:
    FullRPCValues()
        : m_replica(nullptr)
        , m_chunk(nullptr)
        , m_replicaId(0)
    {
    }

    enum
    {
        sHost,
        s2,
        nSessions
    };

    int GetNumSessions() override { return nSessions; }

    void PreConnect() override
    {
        // put something on s1 to get it going
        m_replica = Replica::CreateReplica(nullptr);
        m_chunk = CreateAndAttachReplicaChunk<FullRPCChunk>(m_replica);
        m_replicaId = m_sessions[sHost].GetReplicaMgr().AddPrimary(m_replica);
    }

    ReplicaPtr m_replica;
    FullRPCChunk::Ptr m_chunk;
    ReplicaId m_replicaId;
};

TEST_F(FullRPCValues, DISABLED_FullRPCValues)
{
    RunTickLoop([this](int tick)-> TestStatus
    {
        switch (tick)
        {
        case 10:
            m_chunk->ZeroRPC();
            break;
        case 20:
            m_chunk->OneRPC(11);
            break;
        case 30:
            m_chunk->TwoRPC(21, 22);
            break;
        case 40:
            m_chunk->ThreeRPC(31, 32, 33);
            break;
        case 50:
            m_chunk->FourRPC(41, 42, 43, 44);
            break;
        case 60:
            m_chunk->FiveRPC(51, 52, 53, 54, 55);
            break;
        case 70:
            m_chunk->SixRPC(61, 62, 63, 64, 65, 66);
            break;
        case 80:
            m_chunk->SevenRPC(71, 72, 73, 74, 75, 76, 77);
            break;
        case 90:
            m_chunk->EightRPC(81, 82, 83, 84, 85, 86, 87, 88);
            break;
        case 100:
            m_chunk->NineRPC(91, 92, 93, 94, 95, 96, 97, 98, 99);
            break;
        case 150:
        {
            auto client = m_sessions[s2].GetReplicaMgr().FindReplica(m_replicaId);
            AZ_TEST_ASSERT(client);
            auto clientChunk = client->FindReplicaChunk<FullRPCChunk>();
            AZ_TEST_ASSERT(clientChunk);

            AZ_TEST_ASSERT(m_chunk->m_sentData.size() == 10);
            AZ_TEST_ASSERT(clientChunk->m_receivedData.size() == 10);

            for (AZ::u32 i = 0; i <= 9; i++)
            {
                AZ_TEST_ASSERT(m_chunk->m_sentData[i].size() == static_cast<size_t>(i));
                for (AZ::u32 j = 0; j < i; j++)
                {
                    AZ_TEST_ASSERT(m_chunk->m_sentData[i][j] == (i * 10) + (j + 1));
                }
            }

            for (AZ::u32 i = 0; i <= 9; i++)
            {
                AZ_TEST_ASSERT(clientChunk->m_receivedData[i].size() == static_cast<size_t>(i));
                for (AZ::u32 j = 0; j < i; j++)
                {
                    AZ_TEST_ASSERT(clientChunk->m_receivedData[i][j] == (i * 10) + (j + 1));
                }
            }

            return TestStatus::Completed;
        }
        default:
            return TestStatus::Running;
        }
        return TestStatus::Running;
    });
}


class ReplicaRemoveProxy
    : public SimpleTest
{
public:
    ReplicaRemoveProxy()
        : m_replica(nullptr)
        , m_replicaId(0)
    {
    }

    enum
    {
        sHost,
        s2,
        nSessions
    };

    int GetNumSessions() override { return nSessions; }

    void PreConnect() override
    {
        // put something on s1 to get it going
        m_replica = Replica::CreateReplica(nullptr);
        m_replicaId = m_sessions[sHost].GetReplicaMgr().AddPrimary(m_replica);
    }


    ReplicaPtr m_replica;
    ReplicaId m_replicaId;
};

TEST_F(ReplicaRemoveProxy, DISABLED_ReplicaRemoveProxy)
{
    RunTickLoop([this](int tick)-> TestStatus
    {
        switch (tick)
        {
        case 10:
        {
            auto rep = m_sessions[s2].GetReplicaMgr().FindReplica(m_replicaId);
            AZ_TEST_ASSERT(rep);
            break;
        }
        case 20:
            m_replica->Destroy();
            break;
        case 30:
        {
            auto rep = m_sessions[s2].GetReplicaMgr().FindReplica(m_replicaId);
            AZ_TEST_ASSERT(!rep);
            return TestStatus::Completed;
        }
        default:
            return TestStatus::Running;
        }
        return TestStatus::Running;
    });
}


class ReplicaChunkEvents
    : public SimpleTest
{
public:
    ReplicaChunkEvents()
        : m_replicaId(InvalidReplicaId)
        , m_chunk(nullptr)
        , m_proxyChunk(nullptr)

    {
    }

    enum
    {
        sHost,
        s2,
        nSessions
    };

    int GetNumSessions() override { return nSessions; }

    void PreConnect() override
    {
        ReplicaPtr replica = Replica::CreateReplica(nullptr);
        m_chunk = CreateAndAttachReplicaChunk<AllEventChunk>(replica);
        m_replicaId = m_sessions[sHost].GetReplicaMgr().AddPrimary(replica);

        AZ_TEST_ASSERT(m_chunk->m_attaches == 1);
        AZ_TEST_ASSERT(m_chunk->m_activates == 1);
        AZ_TEST_ASSERT(m_chunk->m_detaches == 0);
        AZ_TEST_ASSERT(m_chunk->m_deactivates == 0);
    }


    ReplicaId m_replicaId;
    AllEventChunk::Ptr m_chunk;
    AllEventChunk::Ptr m_proxyChunk;
};

TEST_F(ReplicaChunkEvents, DISABLED_ReplicaChunkEvents)
{
    RunTickLoop([this](int tick)-> TestStatus
    {
        switch (tick)
        {
        case 20:
        {
            auto rep = m_sessions[s2].GetReplicaMgr().FindReplica(m_replicaId);
            AZ_TEST_ASSERT(rep);
            m_proxyChunk = rep->FindReplicaChunk<AllEventChunk>();
            AZ_TEST_ASSERT(m_proxyChunk);
            AZ_TEST_ASSERT(m_proxyChunk->m_attaches == 1);
            AZ_TEST_ASSERT(m_proxyChunk->m_activates == 1);
            AZ_TEST_ASSERT(m_proxyChunk->m_detaches == 0);
            AZ_TEST_ASSERT(m_proxyChunk->m_deactivates == 0);
            break;
        }
        case 40:
            m_sessions[sHost].GetReplicaMgr().FindReplica(m_replicaId)->Destroy();
            break;
        case 60:
            AZ_TEST_ASSERT(m_chunk->m_attaches == 1);
            AZ_TEST_ASSERT(m_chunk->m_activates == 1);
            AZ_TEST_ASSERT(m_chunk->m_detaches == 1);
            AZ_TEST_ASSERT(m_chunk->m_deactivates == 1);
            AZ_TEST_ASSERT(m_proxyChunk->m_attaches == 1);
            AZ_TEST_ASSERT(m_proxyChunk->m_activates == 1);
            AZ_TEST_ASSERT(m_proxyChunk->m_detaches == 1);
            AZ_TEST_ASSERT(m_proxyChunk->m_deactivates == 1);
            return TestStatus::Completed;
        default: break;
        }
        return TestStatus::Running;
    });
}


class ReplicaChunksBeyond32
    : public SimpleTest
{
public:
    ReplicaChunksBeyond32()
        : m_replicaId(InvalidReplicaId)
    {
    }

    enum
    {
        sHost,
        s2,
        nSessions
    };

    int GetNumSessions() override { return nSessions; }

    void PreConnect() override
    {
        ReplicaPtr replica = Replica::CreateReplica(nullptr);
        for (auto i = 0; i < GM_MAX_CHUNKS_PER_REPLICA; ++i)
        {
            auto chunk = CreateAndAttachReplicaChunk<AllEventChunk>(replica);
            AZ_TEST_ASSERT(chunk);
        }
        m_replicaId = m_sessions[sHost].GetReplicaMgr().AddPrimary(replica);

        auto numChunks = replica->GetNumChunks();
        AZ_TEST_ASSERT(numChunks == GM_MAX_CHUNKS_PER_REPLICA);
    }


    ReplicaId m_replicaId;
};

TEST_F(ReplicaChunksBeyond32, DISABLED_ReplicaChunksBeyond32)
{
    RunTickLoop([this](int tick)-> TestStatus
    {
        switch (tick)
        {
        case 20:
        {
            auto rep = m_sessions[s2].GetReplicaMgr().FindReplica(m_replicaId);
            AZ_TEST_ASSERT(rep);

            auto numChunks = rep->GetNumChunks();
            AZ_TEST_ASSERT(numChunks == GM_MAX_CHUNKS_PER_REPLICA);
            break;
        }
        case 40:
            m_sessions[sHost].GetReplicaMgr().FindReplica(m_replicaId)->Destroy();
            break;
        case 60:
            return TestStatus::Completed;
        default:
            break;
        }
        return TestStatus::Running;
    });
}


class ReplicaChunkEventsDeactivate
    : public SimpleTest
{
public:
    ReplicaChunkEventsDeactivate()
        : m_replica(nullptr)
        , m_replicaId(0)
        , m_chunk(nullptr)
        , m_proxyChunk(nullptr)
    {
    }

    enum
    {
        sHost,
        s2,
        nSessions
    };

    int GetNumSessions() override { return nSessions; }

    void PreConnect() override
    {
        m_replica = Replica::CreateReplica(nullptr);
        m_chunk = CreateAndAttachReplicaChunk<AllEventChunk>(m_replica);
        m_replicaId = m_sessions[sHost].GetReplicaMgr().AddPrimary(m_replica);

        AZ_TEST_ASSERT(m_chunk->m_attaches == 1);
        AZ_TEST_ASSERT(m_chunk->m_activates == 1);
        AZ_TEST_ASSERT(m_chunk->m_detaches == 0);
        AZ_TEST_ASSERT(m_chunk->m_deactivates == 0);
    }

    ReplicaPtr m_replica;
    ReplicaId m_replicaId;
    AllEventChunk::Ptr m_chunk;
    AllEventChunk::Ptr m_proxyChunk;
};

TEST_F(ReplicaChunkEventsDeactivate, DISABLED_ReplicaChunkEventsDeactivate)
{
    RunTickLoop([this](int tick)-> TestStatus
    {
        switch (tick)
        {
        case 20:
        {
            auto rep = m_sessions[s2].GetReplicaMgr().FindReplica(m_replicaId);
            AZ_TEST_ASSERT(rep);
            m_proxyChunk = rep->FindReplicaChunk<AllEventChunk>();
            AZ_TEST_ASSERT(m_proxyChunk);
            AZ_TEST_ASSERT(m_proxyChunk->m_attaches == 1);
            AZ_TEST_ASSERT(m_proxyChunk->m_activates == 1);
            AZ_TEST_ASSERT(m_proxyChunk->m_detaches == 0);
            AZ_TEST_ASSERT(m_proxyChunk->m_deactivates == 0);
            break;
        }
        case 40:
            m_replica->Destroy();
            AZ_TEST_ASSERT(m_chunk->m_attaches == 1);
            AZ_TEST_ASSERT(m_chunk->m_activates == 1);
            AZ_TEST_ASSERT(m_chunk->m_detaches == 0);
            AZ_TEST_ASSERT(m_chunk->m_deactivates == 1);
            break;
        case 50:
            m_replica = nullptr;
            AZ_TEST_ASSERT(m_chunk->m_attaches == 1);
            AZ_TEST_ASSERT(m_chunk->m_activates == 1);
            AZ_TEST_ASSERT(m_chunk->m_detaches == 1);
            AZ_TEST_ASSERT(m_chunk->m_deactivates == 1);
            break;
        case 60:
            AZ_TEST_ASSERT(m_proxyChunk->m_attaches == 1);
            AZ_TEST_ASSERT(m_proxyChunk->m_activates == 1);
            AZ_TEST_ASSERT(m_proxyChunk->m_detaches == 1);
            AZ_TEST_ASSERT(m_proxyChunk->m_deactivates == 1);
            return TestStatus::Completed;
        default: break;
        }
        return TestStatus::Running;
    });
}


class ReplicaDriller
    : public SimpleTest
{
public:
    ReplicaDriller()
        : m_replicaId(InvalidReplicaId)
    {
    }

    enum
    {
        sHost,
        s2,
        nSessions
    };

    class ReplicaDrillerHook
        : public Debug::ReplicaDrillerBus::Handler
    {
    public:
        ReplicaDrillerHook()
            : m_createdReplicas(0)
            , m_destroyedReplicas(0)
            , m_activatedReplicas(0)
            , m_deactivatedReplicas(0)
            , m_attachedChunks(0)
            , m_detachedChunks(0)
            , m_numReplicaBytesSent(0)
            , m_numReplicaBytesReceived(0)
            , m_numRequestChangeOwnership(0)
            , m_numChangedOwnership(0)
            , m_createdChunks(0)
            , m_destroyedChunks(0)
            , m_activatedChunks(0)
            , m_deactivatedChunks(0)
            , m_numChunkBytesSent(0)
            , m_numChunkBytesReceived(0)
            , m_numOutgoingDatasets(0)
            , m_numIncomingDatasets(0)
            , m_numRpcRequests(0)
            , m_numRpcInvokes(0)
            , m_outgoingRpcDataSize(0)
            , m_incomingRpcDataSize(0)
            , m_totalOutgoingBytes(0)
            , m_totalIncomingBytes(0)
            , m_curReplicaSend(nullptr)
            , m_curReplicaChunkSend(nullptr)
            , m_curReplicaChunkIndexSend(GM_MAX_CHUNKS_PER_REPLICA)
            , m_curReplicaReceive(nullptr)
            , m_curReplicaChunkReceive(nullptr)
            , m_curReplicaChunkIndexReceive(GM_MAX_CHUNKS_PER_REPLICA)
        {
        }

        AZStd::size_t m_createdReplicas;
        AZStd::size_t m_destroyedReplicas;
        AZStd::size_t m_activatedReplicas;
        AZStd::size_t m_deactivatedReplicas;
        AZStd::size_t m_attachedChunks;
        AZStd::size_t m_detachedChunks;
        AZStd::size_t m_numReplicaBytesSent;
        AZStd::size_t m_numReplicaBytesReceived;
        AZStd::size_t m_numRequestChangeOwnership;
        AZStd::size_t m_numChangedOwnership;

        AZStd::size_t m_createdChunks;
        AZStd::size_t m_destroyedChunks;
        AZStd::size_t m_activatedChunks;
        AZStd::size_t m_deactivatedChunks;
        AZStd::size_t m_numChunkBytesSent;
        AZStd::size_t m_numChunkBytesReceived;

        AZStd::size_t m_numOutgoingDatasets;
        AZStd::size_t m_numIncomingDatasets;

        AZStd::size_t m_numRpcRequests;
        AZStd::size_t m_numRpcInvokes;
        AZStd::size_t m_outgoingRpcDataSize;
        AZStd::size_t m_incomingRpcDataSize;

        AZStd::size_t m_totalOutgoingBytes;
        AZStd::size_t m_totalIncomingBytes;

        Replica* m_curReplicaSend;
        ReplicaChunkBase* m_curReplicaChunkSend;
        size_t m_curReplicaChunkIndexSend;
        Replica* m_curReplicaReceive;
        ReplicaChunkBase* m_curReplicaChunkReceive;
        AZ::u32 m_curReplicaChunkIndexReceive;

        void OnCreateReplica(Replica* replica) override
        {
            AZ_TEST_ASSERT(replica);
            ++m_createdReplicas;
        }

        void OnDestroyReplica(Replica* replica) override
        {
            AZ_TEST_ASSERT(replica);
            ++m_destroyedReplicas;
        }

        void OnActivateReplica(Replica* replica) override
        {
            AZ_TEST_ASSERT(replica);
            ++m_activatedReplicas;
        }

        void OnDeactivateReplica(Replica* replica) override
        {
            AZ_TEST_ASSERT(replica);
            ++m_deactivatedReplicas;
        }

        void OnAttachReplicaChunk(ReplicaChunkBase* chunk) override
        {
            AZ_TEST_ASSERT(chunk);
            ++m_attachedChunks;
        }

        void OnDetachReplicaChunk(ReplicaChunkBase* chunk) override
        {
            AZ_TEST_ASSERT(chunk);
            ++m_detachedChunks;
        }

        void OnSendReplicaBegin(Replica* replica) override
        {
            AZ_TEST_ASSERT(replica);
            AZ_TEST_ASSERT(m_curReplicaSend == nullptr);
            m_curReplicaSend = replica;
        }

        void OnSendReplicaEnd(Replica* replica, const void* data, size_t len) override
        {
            AZ_TEST_ASSERT(replica);
            AZ_TEST_ASSERT(replica == m_curReplicaSend);
            AZ_TEST_ASSERT(data);
            AZ_TEST_ASSERT(len > 0);
            m_numReplicaBytesSent += len;
            m_curReplicaSend = nullptr;
        }

        void OnReceiveReplicaBegin(Replica* replica, const void* data, size_t len) override
        {
            AZ_TEST_ASSERT(replica);
            AZ_TEST_ASSERT(m_curReplicaReceive == nullptr);
            AZ_TEST_ASSERT(data);
            AZ_TEST_ASSERT(len > 0);
            m_curReplicaReceive = replica;
            m_numReplicaBytesReceived += len;
        }

        void OnReceiveReplicaEnd(Replica* replica) override
        {
            AZ_TEST_ASSERT(replica);
            AZ_TEST_ASSERT(replica == m_curReplicaReceive);
            m_curReplicaReceive = nullptr;
        }

        void OnRequestReplicaChangeOwnership(Replica* replica, PeerId requestor) override
        {
            AZ_TEST_ASSERT(replica);
            AZ_TEST_ASSERT(requestor == (s2 + 1));
            ++m_numRequestChangeOwnership;
        }

        void OnReplicaChangeOwnership(Replica* replica, bool wasPrimary) override
        {
            AZ_TEST_ASSERT(replica);
            switch (m_numChangedOwnership)
            {
            case 0: // host loses ownership
                AZ_TEST_ASSERT(replica->IsProxy() && wasPrimary == true);
                break;
            case 1: // peer acquires ownership
                AZ_TEST_ASSERT(replica->IsPrimary() && wasPrimary == false);
                break;
            default:
                AZ_TEST_ASSERT(0);
            }

            ++m_numChangedOwnership;
        }

        void OnCreateReplicaChunk(ReplicaChunkBase* chunk) override
        {
            AZ_TEST_ASSERT(chunk);
            ++m_createdChunks;
        }

        void OnDestroyReplicaChunk(ReplicaChunkBase* chunk) override
        {
            AZ_TEST_ASSERT(chunk);
            ++m_destroyedChunks;
        }

        void OnActivateReplicaChunk(ReplicaChunkBase* chunk) override
        {
            AZ_TEST_ASSERT(chunk);
            ++m_activatedChunks;
        }

        void OnDeactivateReplicaChunk(ReplicaChunkBase* chunk) override
        {
            AZ_TEST_ASSERT(chunk);
            ++m_deactivatedChunks;
        }

        void OnSendReplicaChunkBegin(ReplicaChunkBase* chunk, AZ::u32 chunkIndex, PeerId from, PeerId to) override
        {
            (void)from;
            (void)to;

            AZ_TEST_ASSERT(chunk);
            AZ_TEST_ASSERT(m_curReplicaSend == chunk->GetReplica());
            AZ_TEST_ASSERT(m_curReplicaChunkSend == nullptr);
            AZ_TEST_ASSERT(m_curReplicaChunkIndexSend == GM_MAX_CHUNKS_PER_REPLICA);
            m_curReplicaChunkSend = chunk;
            m_curReplicaChunkIndexSend = chunkIndex;
        }

        void OnSendReplicaChunkEnd(ReplicaChunkBase* chunk, AZ::u32 chunkIndex, const void* data, size_t len) override
        {
            AZ_TEST_ASSERT(chunk);
            AZ_TEST_ASSERT(m_curReplicaSend == chunk->GetReplica());
            AZ_TEST_ASSERT(m_curReplicaChunkSend == chunk);
            AZ_TEST_ASSERT(m_curReplicaChunkIndexSend == chunkIndex);
            AZ_TEST_ASSERT(data);
            AZ_TEST_ASSERT(len > 0);
            m_numChunkBytesSent += len;
            m_curReplicaChunkSend = nullptr;
            m_curReplicaChunkIndexSend = GM_MAX_CHUNKS_PER_REPLICA;

        }

        void OnReceiveReplicaChunkBegin(ReplicaChunkBase* chunk, AZ::u32 chunkIndex, PeerId from, PeerId to, const void* data, size_t len) override
        {
            AZ_TEST_ASSERT(chunk);
            AZ_TEST_ASSERT(m_curReplicaReceive == chunk->GetReplica());
            AZ_TEST_ASSERT(m_curReplicaChunkReceive == nullptr);
            AZ_TEST_ASSERT(m_curReplicaChunkIndexReceive == GM_MAX_CHUNKS_PER_REPLICA);
            AZ_TEST_ASSERT(from);
            AZ_TEST_ASSERT(to);
            AZ_TEST_ASSERT(data);
            AZ_TEST_ASSERT(len > 0);
            m_curReplicaChunkReceive = chunk;
            m_curReplicaChunkIndexReceive = chunkIndex;
            m_numChunkBytesReceived += len;
        }

        void OnReceiveReplicaChunkEnd(ReplicaChunkBase* chunk, AZ::u32 chunkIndex) override
        {
            AZ_TEST_ASSERT(chunk);
            AZ_TEST_ASSERT(m_curReplicaReceive == chunk->GetReplica());
            AZ_TEST_ASSERT(m_curReplicaChunkReceive == chunk);
            AZ_TEST_ASSERT(m_curReplicaChunkIndexReceive == chunkIndex);
            m_curReplicaChunkReceive = nullptr;
            m_curReplicaChunkIndexReceive = GM_MAX_CHUNKS_PER_REPLICA;
        }

        void OnSendDataSet(ReplicaChunkBase* chunk, AZ::u32 chunkIndex, DataSetBase* dataSet, PeerId from, PeerId to, const void* data, size_t len) override
        {
            AZ_TEST_ASSERT(chunk);
            AZ_TEST_ASSERT(m_curReplicaChunkSend == chunk);
            AZ_TEST_ASSERT(m_curReplicaChunkIndexSend == chunkIndex);
            AZ_TEST_ASSERT(dataSet);
            AZ_TEST_ASSERT(from);
            AZ_TEST_ASSERT(to);
            AZ_TEST_ASSERT(data);
            AZ_TEST_ASSERT(len > 0);

            ++m_numOutgoingDatasets;
        }

        void OnReceiveDataSet(ReplicaChunkBase* chunk, AZ::u32 chunkIndex, DataSetBase* dataSet, PeerId from, PeerId to, const void* data, size_t len) override
        {
            AZ_TEST_ASSERT(chunk);
            AZ_TEST_ASSERT(m_curReplicaChunkReceive == chunk);
            AZ_TEST_ASSERT(m_curReplicaChunkIndexReceive == chunkIndex);
            AZ_TEST_ASSERT(dataSet);
            AZ_TEST_ASSERT(from);
            AZ_TEST_ASSERT(to);
            AZ_TEST_ASSERT(data);
            AZ_TEST_ASSERT(len > 0);

            ++m_numIncomingDatasets;
        }

        void OnRequestRpc(ReplicaChunkBase* chunk, Internal::RpcRequest* rpc) override
        {
            (void)chunk;
            (void)rpc;
            ++m_numRpcRequests;
        }

        void OnInvokeRpc(ReplicaChunkBase* chunk, Internal::RpcRequest* rpc) override
        {
            (void)chunk;
            (void)rpc;
            ++m_numRpcInvokes;
        }

        void OnSendRpc(ReplicaChunkBase* chunk, AZ::u32 chunkIndex, Internal::RpcRequest* rpc, PeerId from, PeerId to, const void* data, size_t len) override
        {
            AZ_TEST_ASSERT(chunk);
            AZ_TEST_ASSERT(m_curReplicaChunkSend == chunk);
            AZ_TEST_ASSERT(m_curReplicaChunkIndexSend == chunkIndex);
            AZ_TEST_ASSERT(rpc);
            AZ_TEST_ASSERT(from);
            AZ_TEST_ASSERT(to);
            AZ_TEST_ASSERT(data);
            AZ_TEST_ASSERT(len > 0);
            m_outgoingRpcDataSize += len;
        }

        void OnReceiveRpc(ReplicaChunkBase* chunk, AZ::u32 chunkIndex, Internal::RpcRequest* rpc, PeerId from, PeerId to, const void* data, size_t len) override
        {
            AZ_TEST_ASSERT(chunk);
            AZ_TEST_ASSERT(m_curReplicaChunkReceive == chunk);
            AZ_TEST_ASSERT(m_curReplicaChunkIndexReceive == chunkIndex);
            AZ_TEST_ASSERT(rpc);
            AZ_TEST_ASSERT(from);
            AZ_TEST_ASSERT(to);
            AZ_TEST_ASSERT(data);
            AZ_TEST_ASSERT(len > 0);
            m_incomingRpcDataSize += len;
        }

        void OnSend(PeerId to, const void* data, size_t len, bool isReliable) override
        {
            (void)to; // peerId might not be valid at this point, e.g. handshake (Cmd_Greetings) did not accomplish yet
            (void)isReliable;
            AZ_TEST_ASSERT(data);
            AZ_TEST_ASSERT(len > 0);
            m_totalOutgoingBytes += len;
        }

        void OnReceive(PeerId from, const void* data, size_t len) override
        {
            (void)from; // peerId might not be valid at this point, e.g. handshake (Cmd_Greetings) did not accomplish yet
            AZ_TEST_ASSERT(data);
            AZ_TEST_ASSERT(len > 0);
            m_totalIncomingBytes += len;
        }
    };

    int GetNumSessions() override { return nSessions; }

    void PreConnect() override
    {
        m_driller.BusConnect();

        ReplicaPtr replica = Replica::CreateReplica(nullptr);
        CreateAndAttachReplicaChunk<DrillerTestChunk>(replica);
        m_replicaId = m_sessions[sHost].GetReplicaMgr().AddPrimary(replica);
    }

    ~ReplicaDriller() override
    {
        m_driller.BusDisconnect();
    }

    ReplicaDrillerHook m_driller;
    ReplicaId m_replicaId;
};

TEST_F(ReplicaDriller, DISABLED_ReplicaDriller)
{
    RunTickLoop([this](int tick)-> TestStatus
    {
        switch (tick)
        {
        case 10:
        {
            auto rep = m_sessions[s2].GetReplicaMgr().FindReplica(m_replicaId);
            AZ_TEST_ASSERT(rep);
            AZ_TEST_ASSERT(rep->IsProxy());
            rep->RequestChangeOwnership();
            break;
        }
        case 30:
        {
            auto rep = m_sessions[s2].GetReplicaMgr().FindReplica(m_replicaId);
            AZ_TEST_ASSERT(rep);
            AZ_TEST_ASSERT(rep->IsPrimary());
            rep->Destroy();
            break;
        }
        case 40:
            // replicas
            AZ_TEST_ASSERT(m_driller.m_createdReplicas > 0);
            AZ_TEST_ASSERT(m_driller.m_destroyedReplicas > 0);
            AZ_TEST_ASSERT(m_driller.m_activatedReplicas > 0);
            AZ_TEST_ASSERT(m_driller.m_deactivatedReplicas > 0);
            AZ_TEST_ASSERT(m_driller.m_numReplicaBytesSent > 0);
            AZ_TEST_ASSERT(m_driller.m_numReplicaBytesReceived > 0);
            AZ_TEST_ASSERT(m_driller.m_numRequestChangeOwnership == 1);
            AZ_TEST_ASSERT(m_driller.m_numChangedOwnership == 2); // two because one call for host & one for peer

                                                                  // chunks
            AZ_TEST_ASSERT(m_driller.m_createdChunks >= m_driller.m_createdReplicas);
            AZ_TEST_ASSERT(m_driller.m_destroyedChunks >= m_driller.m_destroyedReplicas);
            AZ_TEST_ASSERT(m_driller.m_activatedChunks >= m_driller.m_activatedReplicas);
            AZ_TEST_ASSERT(m_driller.m_deactivatedChunks >= m_driller.m_deactivatedReplicas);
            AZ_TEST_ASSERT(m_driller.m_attachedChunks > 0);
            AZ_TEST_ASSERT(m_driller.m_detachedChunks > 0);
            AZ_TEST_ASSERT(m_driller.m_numChunkBytesReceived > 0);

            AZ_TEST_ASSERT(m_driller.m_numChunkBytesSent > 0);
            AZ_TEST_ASSERT(m_driller.m_numChunkBytesReceived > 0);

            // datasets
            AZ_TEST_ASSERT(m_driller.m_numOutgoingDatasets > 0);
            AZ_TEST_ASSERT(m_driller.m_numIncomingDatasets > 0);

            // rpcs
            AZ_TEST_ASSERT(m_driller.m_numRpcRequests > 0);
            AZ_TEST_ASSERT(m_driller.m_numRpcInvokes > 0);
            AZ_TEST_ASSERT(m_driller.m_outgoingRpcDataSize > 0);
            AZ_TEST_ASSERT(m_driller.m_incomingRpcDataSize > 0);

            // data
            AZ_TEST_ASSERT(m_driller.m_totalOutgoingBytes > 0);
            AZ_TEST_ASSERT(m_driller.m_totalIncomingBytes > 0);
            return TestStatus::Completed;
        default: break;
        }
        return TestStatus::Running;
    });
}


class DataSetChangedTest
    : public SimpleTest
{
public:
    DataSetChangedTest()
        : m_replica(nullptr)
        , m_replicaId(0)
        , m_chunk(nullptr)
    {
    }

    enum
    {
        sHost,
        s2,
        nSessions
    };

    int GetNumSessions() override { return nSessions; }

    void PreConnect() override
    {
        m_replica = Replica::CreateReplica(nullptr);
        m_chunk = CreateAndAttachReplicaChunk<DataSetChunk>(m_replica);
        m_replicaId = m_sessions[sHost].GetReplicaMgr().AddPrimary(m_replica);
    }


    ReplicaPtr m_replica;
    ReplicaId m_replicaId;
    DataSetChunk::Ptr m_chunk;
};

TEST_F(DataSetChangedTest, DISABLED_DataSetChangedTest)
{
    RunTickLoop([this](int tick)-> TestStatus
    {
        if (tick < 100)
        {
            if (!(tick % 10))
            {
                m_chunk->Data1.Set(tick);
            }
            return TestStatus::Running;
        }

        auto rep = m_sessions[s2].GetReplicaMgr().FindReplica(m_replicaId);
        auto chunk = rep->FindReplicaChunk<DataSetChunk>();
        AZ_TEST_ASSERT(m_chunk->m_changedData.size() == 0);
        AZ_TEST_ASSERT(chunk->m_changedData.size() == 10);
        int expected = 0;
        for (auto i : chunk->m_changedData)
        {
            AZ_TEST_ASSERT(i == expected);
            expected += 10;
        }

        return TestStatus::Completed;
    });
}


class CustomHandlerTest
    : public SimpleTest
{
public:
    CustomHandlerTest()
        : m_replica(nullptr)
        , m_replicaId(0)
        , m_chunk(nullptr)
    {
    }

    enum
    {
        sHost,
        s2,
        nSessions
    };

    int GetNumSessions() override { return nSessions; }

    void PreConnect() override
    {
        m_replica = Replica::CreateReplica(nullptr);
        m_chunk = CreateAndAttachReplicaChunk<CustomHandlerChunk>(m_replica);
        m_replicaId = m_sessions[sHost].GetReplicaMgr().AddPrimary(m_replica);
        m_primaryHandler.reset(aznew CustomHandler());
        m_proxyHandler.reset(aznew CustomHandler());
        m_chunk->SetHandler(m_primaryHandler.get());
    }

    ReplicaPtr m_replica;
    ReplicaId m_replicaId;
    CustomHandlerChunk::Ptr m_chunk;
    AZStd::scoped_ptr<CustomHandler> m_primaryHandler;
    AZStd::scoped_ptr<CustomHandler> m_proxyHandler;
};

TEST_F(CustomHandlerTest, DISABLED_CustomHandlerTest)
{
    RunTickLoop([this](int tick)-> TestStatus
    {
        {
            auto rep = m_sessions[s2].GetReplicaMgr().FindReplica(m_replicaId);
            if (rep)
            {
                auto chunk = rep->FindReplicaChunk<CustomHandlerChunk>();
                if (chunk && !chunk->GetHandler())
                {
                    chunk->SetHandler(m_proxyHandler.get());
                    m_proxyHandler->m_dataset.push_back(chunk->Data.Get());
                }
            }
        }

        if (tick < 100)
        {
            if (!(tick % 10))
            {
                m_chunk->Data.Set(tick);
            }
            return TestStatus::Running;
        }
        else if (tick < 200)
        {
            if (!(tick % 10))
            {
                m_chunk->RPC(tick);
            }
            return TestStatus::Running;
        }


        AZ_TEST_ASSERT(m_proxyHandler->m_dataset.size() == 10);
        int expected = 0;
        for (auto i : m_proxyHandler->m_dataset)
        {
            AZ_TEST_ASSERT(i == expected);
            expected += 10;
        }
        expected = 100;
        for (auto i : m_proxyHandler->m_rpc)
        {
            AZ_TEST_ASSERT(i == expected);
            expected += 10;
        }
        return TestStatus::Completed;
    });
}


class NonConstMarshalerTest
    : public SimpleTest
{
public:
    NonConstMarshalerTest()
        : m_replica(nullptr)
        , m_replicaId(0)
        , m_chunk(nullptr)
    {
    }

    enum
    {
        sHost,
        s2,
        nSessions
    };

    int GetNumSessions() override { return nSessions; }

    void PreConnect() override
    {
        m_replica = Replica::CreateReplica(nullptr);
        m_chunk = CreateAndAttachReplicaChunk<NonConstMarshalerChunk>(m_replica);
        m_replicaId = m_sessions[sHost].GetReplicaMgr().AddPrimary(m_replica);
    }

    ReplicaPtr m_replica;
    ReplicaId m_replicaId;
    NonConstMarshalerChunk::Ptr m_chunk;
};

TEST_F(NonConstMarshalerTest, DISABLED_NonConstMarshalerTest)
{
    RunTickLoop([this](int tick)-> TestStatus
    {
        switch (tick)
        {
        case 10:
            m_chunk->RPCTestRPC(1);
            break;

        case 20:
            m_chunk->RPCTestRPC(2);
            break;

        case 30:
            m_chunk->m_data.Set(10);
            break;

        case 40:
            m_chunk->m_data.Set(20);
            break;

        case 50:
        {
            auto rep = m_sessions[s2].GetReplicaMgr().FindReplica(m_replicaId);
            auto chunk = rep->FindReplicaChunk<NonConstMarshalerChunk>();

            AZ_TEST_ASSERT(m_chunk->RPCTestRPC.GetMarshalers().m_marshaler.m_valueWritten > 0);
            AZ_TEST_ASSERT(chunk->RPCTestRPC.GetMarshalers().m_marshaler.m_valueRead > 0);

            AZ_TEST_ASSERT(m_chunk->m_data.GetMarshaler().m_valueWritten > 0);
            AZ_TEST_ASSERT(chunk->m_data.GetMarshaler().m_valueRead > 0);

            return TestStatus::Completed;
        }
        default: ;
        }

        return TestStatus::Running;
    });
}


class SourcePeerTest
    : public SimpleTest
{
public:
    SourcePeerTest()
        : m_replica(nullptr)
        , m_replicaId(0)
        , m_chunk(nullptr)
        , m_chunk2(nullptr)
    {
    }

    enum
    {
        sHost,
        s2,
        nSessions
    };

    int GetNumSessions() override { return nSessions; }

    void PreConnect() override
    {
        m_replica = Replica::CreateReplica(nullptr);
        m_chunk = CreateAndAttachReplicaChunk<SourcePeerChunk>(m_replica);
        m_replicaId = m_sessions[sHost].GetReplicaMgr().AddPrimary(m_replica);
    }

    ReplicaPtr m_replica;
    ReplicaId m_replicaId;
    SourcePeerChunk::Ptr m_chunk;
    SourcePeerChunk::Ptr m_chunk2;
};

TEST_F(SourcePeerTest, DISABLED_SourcePeerTest)
{
    RunTickLoop([this](int tick)-> TestStatus
    {
        if (!m_chunk2)
        {
            auto rep = m_sessions[s2].GetReplicaMgr().FindReplica(m_replicaId);
            if (rep)
            {
                m_chunk2 = rep->FindReplicaChunk<SourcePeerChunk>();
            }
        }

        switch (tick)
        {
        case 10:
            m_chunk->PeerRPC();
            break;

        case 20:
            m_chunk2->PeerRPC();
            break;

        case 30:
            m_chunk->ForwardPeerRPC();
            break;

        case 40:
            m_chunk2->ForwardPeerRPC();
            break;

        case 50:
        {
            AZ_TEST_ASSERT(m_chunk->m_peers.size() == 2);
            AZ_TEST_ASSERT(m_chunk2->m_peers.size() == 2);

            AZ_TEST_ASSERT(m_chunk->m_peers[0] == m_sessions[sHost].GetReplicaMgr().GetLocalPeerId());
            AZ_TEST_ASSERT(m_chunk2->m_peers[0] == m_sessions[sHost].GetReplicaMgr().GetLocalPeerId());

            AZ_TEST_ASSERT(m_chunk->m_peers[1] == m_sessions[s2].GetReplicaMgr().GetLocalPeerId());
            AZ_TEST_ASSERT(m_chunk2->m_peers[1] == m_sessions[sHost].GetReplicaMgr().GetLocalPeerId());

            AZ_TEST_ASSERT(m_chunk->m_forwardPeers.size() == 2);
            AZ_TEST_ASSERT(m_chunk2->m_forwardPeers.size() == 2);

            AZ_TEST_ASSERT(m_chunk->m_forwardPeers[0] == m_sessions[sHost].GetReplicaMgr().GetLocalPeerId());
            AZ_TEST_ASSERT(m_chunk2->m_forwardPeers[0] == m_sessions[sHost].GetReplicaMgr().GetLocalPeerId());

            AZ_TEST_ASSERT(m_chunk->m_forwardPeers[1] == m_sessions[s2].GetReplicaMgr().GetLocalPeerId());
            AZ_TEST_ASSERT(m_chunk2->m_forwardPeers[1] == m_sessions[s2].GetReplicaMgr().GetLocalPeerId());

            return TestStatus::Completed;
        }
        default: break;
        }

        return TestStatus::Running;
    });
}


class SendWithPriority
    : public SimpleTest
{
public:
    enum
    {
        sHost,
        s2,
        nSessions
    };

    class PriorityChunk
        : public ReplicaChunk
    {
    public:
        GM_CLASS_ALLOCATOR(PriorityChunk);
        typedef AZStd::intrusive_ptr<PriorityChunk> Ptr;
        static const char* GetChunkName() { return "PriorityChunk"; }

        PriorityChunk()
            : m_value("Value")
        {
        }

        bool IsReplicaMigratable() override { return false; }

        DataSet<int> m_value;
    };

    class ReplicaDrillerHook
        : public Debug::ReplicaDrillerBus::Handler
    {
    public:
        ReplicaDrillerHook()
            : m_expectedSendValue(SendWithPriority::kNumReplicas)
            , m_expectedRecvValue(SendWithPriority::kNumReplicas)
        {
        }

        void OnReceiveReplicaEnd(Replica* replica) override
        {
            auto chunk = replica->FindReplicaChunk<PriorityChunk>();
            if (chunk && m_expectedRecvValue > 0)
            {
                AZ_TEST_ASSERT(chunk->m_value.Get() == m_expectedRecvValue); // checking reverse order
                --m_expectedRecvValue;
            }
        }

        void OnSendReplicaEnd(Replica* replica, const void* data, size_t len) override
        {
            (void)data;
            (void)len;

            auto chunk = replica->FindReplicaChunk<PriorityChunk>();
            if (chunk && m_expectedSendValue > 0)
            {
                AZ_TEST_ASSERT(chunk->m_value.Get() == m_expectedSendValue); // checking reverse order
                --m_expectedSendValue;
            }
        }

        int m_expectedSendValue;
        int m_expectedRecvValue;
    };

    int GetNumSessions() override { return nSessions; }

    void PreConnect() override
    {
        ReplicaChunkDescriptorTable::Get().RegisterChunkType<PriorityChunk>();

        m_driller.BusConnect();

        for (unsigned i = 0; i < kNumReplicas; ++i)
        {
            ReplicaPtr replica = Replica::CreateReplica(nullptr);
            m_chunks[i] = CreateAndAttachReplicaChunk<PriorityChunk>(replica);
            m_chunks[i]->m_value.Set(i + 1); // setting dataset values to 1..kNumReplicas
            m_chunks[i]->SetPriority(k_replicaPriorityNormal + static_cast<ReplicaPriority>(i)); // the later created - the higher priorities, so should be sent in reverse order
            m_sessions[sHost].GetReplicaMgr().AddPrimary(replica);
        }
    }


    static const size_t kNumReplicas = 5;

    ReplicaDrillerHook m_driller;
    PriorityChunk::Ptr m_chunks[kNumReplicas];
};

TEST_F(SendWithPriority, DISABLED_SendWithPriority)
{
    RunTickLoop([this](int tick)-> TestStatus
    {
        if (tick == 20)
        {
            AZ_TEST_ASSERT(m_driller.m_expectedSendValue == 0); // sent all the replicas in the right order
            AZ_TEST_ASSERT(m_driller.m_expectedRecvValue == 0); // received all the replicas in the right order
            return TestStatus::Completed;
        }

        return TestStatus::Running;
    });
}


class SuspendUpdatesTest
    : public SimpleTest
{
public:
    enum
    {
        sHost,
        s2,
        s3,
        nSessions
    };

    class SuspendUpdatesChunk
        : public ReplicaChunk
    {
    public:
        GM_CLASS_ALLOCATOR(SuspendUpdatesChunk);

        SuspendUpdatesChunk()
            : RPCTest("RPCTest")
            , Data1("Data1", 0)
            , m_numDataSetChanged(0)
            , m_numRpcCalled(0)
            , m_enabled(true)
        {
        }

        void OnReplicaActivate(const ReplicaContext& ctx) override
        {
            (void)ctx;
            SuspendUpdatesFromReplica();
        }

        typedef AZStd::intrusive_ptr<SuspendUpdatesChunk> Ptr;
        static const char* GetChunkName() { return "SuspendUpdatesChunk"; }

        bool IsReplicaMigratable() override { return false; }
        bool IsUpdateFromReplicaEnabled() override { return m_enabled; }

        void SuspendUpdatesFromReplica()
        {
            m_enabled = false;
        }

        void ResumeUpdatesFromReplica()
        {
            m_enabled = true;
        }

        void DatasetHandler(const int& val, const TimeContext& ctx)
        {
            (void)val;
            (void)ctx;
            ++m_numDataSetChanged;
        }

        bool RPCHandler(const RpcContext& ctx)
        {
            (void)ctx;
            ++m_numRpcCalled;
            return true;
        }

        Rpc<>::BindInterface<SuspendUpdatesChunk, &SuspendUpdatesChunk::RPCHandler> RPCTest;
        DataSet<int>::BindInterface<SuspendUpdatesChunk, &SuspendUpdatesChunk::DatasetHandler> Data1;

        unsigned int m_numDataSetChanged;
        unsigned int m_numRpcCalled;
        bool m_enabled;
    };

    int GetNumSessions() override { return nSessions; }

    void PreConnect() override
    {
        ReplicaChunkDescriptorTable::Get().RegisterChunkType<SuspendUpdatesChunk>();

        ReplicaPtr replica = Replica::CreateReplica(nullptr);
        m_chunk = CreateAndAttachReplicaChunk<SuspendUpdatesChunk>(replica);
        m_sessions[sHost].GetReplicaMgr().AddPrimary(replica);
    }

    SuspendUpdatesChunk::Ptr m_chunk = nullptr;
    unsigned int m_numRpcCalled = 0;
};

TEST_F(SuspendUpdatesTest, DISABLED_SuspendUpdatesTest)
{
    RunTickLoop([this](int tick)-> TestStatus
    {
        if (tick >= 10 && tick < 15)
        {
            m_chunk->Data1.Set(m_chunk->Data1.Get() + 1);
            m_chunk->RPCTest();
            ++m_numRpcCalled;
        }
        else if (tick >= 15 && tick < 20)
        {
            for (int i = sHost + 1; i < nSessions; ++i)
            {
                ReplicaPtr rep = m_sessions[i].GetReplicaMgr().FindReplica(m_chunk->GetReplicaId());
                AZ_Assert(rep, "No replica in the session %d\n", i);

                auto chunkPtr = rep->FindReplicaChunk<SuspendUpdatesChunk>();
                AZ_Assert(chunkPtr, "No SuspendUpdatesChunk is not found on replica\n");

                // rpcs and datasets updates should not be called
                AZ_TEST_ASSERT(chunkPtr->m_numDataSetChanged == 0);
                AZ_TEST_ASSERT(chunkPtr->m_numRpcCalled == 0);
            }
        }
        else if (tick == 20)
        {
            for (int i = sHost + 1; i < nSessions; ++i)
            {
                ReplicaPtr rep = m_sessions[i].GetReplicaMgr().FindReplica(m_chunk->GetReplicaId());
                AZ_Assert(rep, "No replica in the session %d\n", i);
                auto chunkPtr = rep->FindReplicaChunk<SuspendUpdatesChunk>();
                AZ_Assert(chunkPtr, "No SuspendUpdatesChunk is not found on replica\n");

                chunkPtr->ResumeUpdatesFromReplica();
            }
        }
        else if (tick == 25)
        {
            for (int i = sHost + 1; i < nSessions; ++i)
            {
                ReplicaPtr rep = m_sessions[i].GetReplicaMgr().FindReplica(m_chunk->GetReplicaId());
                AZ_Assert(rep, "No replica in the session %d\n", i);

                auto chunkPtr = rep->FindReplicaChunk<SuspendUpdatesChunk>();
                AZ_Assert(chunkPtr, "SuspendUpdatesChunk is not found on replica\n");

                // all rpcs and datasets callback should be called here
                AZ_TEST_ASSERT(chunkPtr->m_numDataSetChanged == 1);
                AZ_TEST_ASSERT(chunkPtr->m_numRpcCalled == m_numRpcCalled);
            }

            return TestStatus::Completed;
        }

        return TestStatus::Running;
    });
}


class BasicHostChunkDescriptorTest
    : public UnitTest::GridMateMPTestFixture
    , public ::testing::Test
{
public:
    enum
    {
        Host,
        Client,
        nNodes
    };

    class HostChunk : public ReplicaChunk
    {
    public:
        GM_CLASS_ALLOCATOR(HostChunk);

        static const char* GetChunkName() { return "BasicHostChunkDescriptorTest::HostChunk"; }

        bool IsReplicaMigratable() override { return false; }

        void OnReplicaActivate(const GridMate::ReplicaContext&) override
        {
            if (IsPrimary())
            {
                nPrimaryActivations++;
            }
            else
            {
                nProxyActivations++;
            }
        }

        static int nPrimaryActivations;
        static int nProxyActivations;
    };
};
int BasicHostChunkDescriptorTest::HostChunk::nPrimaryActivations = 0;
int BasicHostChunkDescriptorTest::HostChunk::nProxyActivations = 0;

TEST_F(BasicHostChunkDescriptorTest, DISABLED_BasicHostChunkDescriptorTest)
{
    AZ_TracePrintf("GridMate", "\n");

    // Register test chunks
    ReplicaChunkDescriptorTable::Get().RegisterChunkType<HostChunk, GridMate::BasicHostChunkDescriptor<HostChunk>>();

    MPSessionMedium nodes[nNodes];

    // initialize transport
    int basePort = 4427;
    for (int i = 0; i < nNodes; ++i)
    {
        TestCarrierDesc desc;
        desc.m_port = basePort + i;
        // initialize replica managers
        nodes[i].SetTransport(DefaultCarrier::Create(desc, m_gridMate));
        nodes[i].AcceptConn(true);
        nodes[i].SetClient(i != Host);
        nodes[i].GetReplicaMgr().Init(ReplicaMgrDesc(i + 1, nodes[i].GetTransport(), 0, i == 0 ? ReplicaMgrDesc::Role_SyncHost : 0));
    }

    // connect Client to Host
    nodes[Client].GetTransport()->Connect("127.0.0.1", basePort);

    ReplicaPtr hostReplica;
    ReplicaPtr clientReplica;

    // main test loop
    for (int tick = 0; tick < 1000; ++tick)
    {
        if (tick == 100)
        {
            for (int i = 0; i < nNodes; ++i)
            {
                AZ_TEST_ASSERT(nodes[i].GetReplicaMgr().IsReady());
            }
        }

        if (tick == 200)
        {
            hostReplica = Replica::CreateReplica("HostReplica");
            hostReplica->AttachReplicaChunk(CreateReplicaChunk<HostChunk>());
            nodes[Host].GetReplicaMgr().AddPrimary(hostReplica);
        }

        if (tick == 300)
        {
            AZ_TEST_ASSERT(HostChunk::nPrimaryActivations == 1);
            AZ_TEST_ASSERT(HostChunk::nProxyActivations == 1);
            AZ_TEST_ASSERT(nodes[Client].GetReplicaMgr().FindReplica(hostReplica->GetRepId())->FindReplicaChunk<HostChunk>());

            AZ_TEST_START_TRACE_SUPPRESSION;
            clientReplica = Replica::CreateReplica("ClientReplica");
            clientReplica->AttachReplicaChunk(CreateReplicaChunk<HostChunk>());
            nodes[Client].GetReplicaMgr().AddPrimary(clientReplica);
        }

        if (tick == 400)
        {
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
            AZ_TEST_ASSERT(HostChunk::nPrimaryActivations == 2);
            AZ_TEST_ASSERT(HostChunk::nProxyActivations == 1);
            AZ_TEST_ASSERT(!nodes[Host].GetReplicaMgr().FindReplica(clientReplica->GetRepId())->FindReplicaChunk<HostChunk>());
        }

        // tick everything
        for (int i = 0; i < nNodes; ++i)
        {
            nodes[i].Update();
            nodes[i].GetReplicaMgr().Unmarshal();
            nodes[i].GetReplicaMgr().UpdateReplicas();
            nodes[i].GetReplicaMgr().UpdateFromReplicas();
            nodes[i].GetReplicaMgr().Marshal();
            nodes[i].GetTransport()->Update();
        }

        AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(10));
    }

    hostReplica = nullptr;
    clientReplica = nullptr;

    for (int i = 0; i < nNodes; ++i)
    {
        nodes[i].GetReplicaMgr().Shutdown();
        DefaultCarrier::Destroy(nodes[i].GetTransport());
    }
}

/*
 * Create and immedietly destroy primary replica
 * Test that it does not result in any network sync
*/
class CreateDestroyPrimary
    : public SimpleTest
    , public Debug::ReplicaDrillerBus::Handler
{
public:
    enum
    {
        sHost,
        s2,
        nSessions
    };

    int GetNumSessions() override { return nSessions; }


    // ReplicaDrillerBus
    void OnReceive(PeerId from, const void* data, size_t len) override
    {
        (void)from;
        (void)data;
        (void)len;

        AZ_TEST_ASSERT(false); // should not receive any replica data
    }

    void ConnectDriller()
    {
        Debug::ReplicaDrillerBus::Handler::BusConnect();
    }

    void DisconnectDriller()
    {
        Debug::ReplicaDrillerBus::Handler::BusDisconnect();
    }
};

TEST_F(CreateDestroyPrimary, DISABLED_CreateDestroyPrimary)
{
    RunTickLoop([this](int tick)-> TestStatus
    {
        switch (tick)
        {

        case 10:
        {
            ConnectDriller();
            auto replica = Replica::CreateReplica(nullptr);
            CreateAndAttachReplicaChunk<DataSetChunk>(replica);
            m_sessions[sHost].GetReplicaMgr().AddPrimary(replica);

            // Destroying replica right away
            replica->Destroy();
            break;
        }

        case 20:
            DisconnectDriller();
            return TestStatus::Completed;
        default: break;
        }

        return TestStatus::Running;
    });
}

/*
* This test checks that when the carrier ACKs a message it feeds back to the ReplicaTarget.
* The ReplicaTarget will prevent sending more updates.
*/
class ReplicaACKfeedbackTestFixture
    : public SimpleTest
{
public:
    ReplicaACKfeedbackTestFixture()
        : m_replicaId(InvalidReplicaId)
    {
    }

    enum
    {
        sHost,
        sClient,
        nSessions
    };

    int GetNumSessions() override { return nSessions; }

    static const int NonDefaultValue = 4242;
    static const int k_headerBytes = 12;
    static const int k_updateBytes = k_headerBytes + 10 * sizeof(int);

    void PreConnect() override
    {
        m_driller.BusConnect();

        ReplicaPtr replica = Replica::CreateReplica("ReplicaACKfeedbackTest");
        LargeChunkWithDefaultsMedium* chunk = CreateAndAttachReplicaChunk<LargeChunkWithDefaultsMedium>(replica);
        AZ_TEST_ASSERT(chunk);

        m_replicaId = m_sessions[sHost].GetReplicaMgr().AddPrimary(replica);
    }

    ~ReplicaACKfeedbackTestFixture() override
    {
        m_driller.BusDisconnect();
    }

    size_t                              m_replicaBytesSentPrev = 0;
    ReplicaId                           m_replicaId;
    ReplicaDriller::ReplicaDrillerHook  m_driller;
};

TEST_F(ReplicaACKfeedbackTestFixture, ReplicaACKfeedbackTest)
{
    RunTickLoop([this](int tick)-> TestStatus
    {
        if(! ReplicaTarget::IsAckEnabled())
        {
            return TestStatus::Completed;
        }

        //AZ_Printf("GridMateTests", "%d %d\n", tick, m_driller.m_numReplicaBytesSent);
        // Tests the Revision stamp with Carrier ACK feedback
        // result is true on the immediate tick after changing, but false on the next and stays false until next change
        auto CheckHostReplicaChanged = [this](bool result)
        {
            ReplicaPtr replica = m_sessions[sHost].GetReplicaMgr().FindReplica(m_replicaId);
            if (replica)
            {
                for (auto& dst : replica->DebugGetTargets() )
                {
                    const bool targetHasUnAckdData = ReplicaTarget::IsAckEnabled() && dst.HasOldRevision(replica->GetRevision());
                    //AZ_Printf("GridMateTests", "target's stamp %d replica last_change %d : assert(%d)\n",
                    //    dst.GetRevision(), replica->GetRevision(), targetHasUnAckdData == result);
                    AZ_TEST_ASSERT(targetHasUnAckdData == result);
                }
            }
        };
        auto updateDataSets = [](AZStd::intrusive_ptr<LargeChunkWithDefaultsMedium>& chunk, int val)
        {
            AZ_TEST_ASSERT(chunk);
            auto touch = [val](DataSet<int>& dataSet) { dataSet.Set(val); };
            touch(chunk->Data1);
            touch(chunk->Data2);
            touch(chunk->Data3);
            touch(chunk->Data4);
            touch(chunk->Data5);
            touch(chunk->Data6);
            touch(chunk->Data7);
            touch(chunk->Data8);
            touch(chunk->Data9);
            touch(chunk->Data10);
        };

        switch (tick)
        {
        case 6:
        {
            CheckHostReplicaChanged(false);     //Default value sent reliably. Called back immediately. Nothing to ACK.
        }break;
        case 10:
        {
            auto rep = m_sessions[sClient].GetReplicaMgr().FindReplica(m_replicaId);
            AZ_TEST_ASSERT(rep);                //Client has recvd
        }break;
        case 15:
        {
            ReplicaPtr replica = m_sessions[sHost].GetReplicaMgr().FindReplica(m_replicaId);
            auto chunk = replica->FindReplicaChunk<LargeChunkWithDefaultsMedium>();

            updateDataSets(chunk, NonDefaultValue);

            m_replicaBytesSentPrev = m_driller.m_numReplicaBytesSent;
            CheckHostReplicaChanged(false);  //Changed now, but wont know until next prepareData()
        }break;
        case 16:
        {
            AZ_TEST_ASSERT(m_driller.m_numReplicaBytesSent - m_replicaBytesSentPrev == k_updateBytes);
            CheckHostReplicaChanged(true);  //Detected change. ACK feedback on next tick returns to false.
        }break;
        case 20:
        {
            ReplicaPtr replica = m_sessions[sHost].GetReplicaMgr().FindReplica(m_replicaId);
            auto chunk = replica->FindReplicaChunk<LargeChunkWithDefaultsMedium>();

            updateDataSets(chunk, NonDefaultValue + 1);

            m_replicaBytesSentPrev = m_driller.m_numReplicaBytesSent;
            CheckHostReplicaChanged(false);  //Changed now, but wont know until next prepareData()
        }break;
        case 21:
        {
            AZ_TEST_ASSERT(m_driller.m_numReplicaBytesSent - m_replicaBytesSentPrev == k_updateBytes);
            CheckHostReplicaChanged(true);  //Detected change. ACK feedback on next tick returns to false.
        }break;
        case 25:
        {
            CheckHostReplicaChanged(false);
            m_sessions[sHost].GetReplicaMgr().FindReplica(m_replicaId)->Destroy();
        } break;
        case 30:
        {
            return TestStatus::Completed;
        }
        default:
            CheckHostReplicaChanged(false);      //All other ticks leave Replica unchanged!
            break;
        }

        return TestStatus::Running;
    });
}

} // namespace UnitTest
