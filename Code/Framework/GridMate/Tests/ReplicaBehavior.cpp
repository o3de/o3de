/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "Tests.h"

#include <GridMate/Memory.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/smart_ptr/scoped_ptr.h>

#include <GridMate/Carrier/DefaultSimulator.h>
#include <GridMate/Containers/unordered_map.h>
#include <GridMate/Containers/unordered_set.h>
#include <GridMate/Replica/Interpolators.h>
#include <GridMate/Replica/Replica.h>
#include <GridMate/Replica/ReplicaFunctions.h>
#include <GridMate/Replica/ReplicaMgr.h>
#include <GridMate/Replica/ReplicaDrillerEvents.h>
#include <GridMate/Serialize/CompressionMarshal.h>

using namespace GridMate;

namespace GridMate
{
    class CustomInt
    {
    public:
        CustomInt() : m_value(0) {}
        explicit CustomInt(int value) : m_value(value) {}

        bool operator==(const CustomInt& other) const
        {
            return other.m_value == m_value;
        }

        int m_value;
    };

    template<>
    class Marshaler<CustomInt>
    {
    public:

        Marshaler()
            : m_marshalCalls(0)
            , m_unmarshalCalls(0)
        {
        }

        /// Defines the size that is written to the wire. This is only valid for fixed size marshalers, marshalers for dynamic objects don't define it.
        static const AZStd::size_t MarshalSize = 0;

        void Marshal(WriteBuffer& wb, const CustomInt& value) const
        {
            wb.Write(value.m_value);

            m_marshalCalls++;
        }
        void Unmarshal(CustomInt& value, ReadBuffer& rb) const
        {
            rb.Read(value.m_value);

            m_unmarshalCalls++;
        }

        mutable size_t m_marshalCalls;
        mutable size_t m_unmarshalCalls;
    };
}

namespace UnitTest {

namespace ReplicaBehavior {
#define GM_REPLICA_TEST_SESSION_CHANNEL 1

    class AbleToSetDirtyDataSet : public DataSet<int>
    {
    public:
        AbleToSetDirtyDataSet(const char* name, int value) : DataSet<int>(name, value) {}

        void ForceDirtyLikeScriptsDo()
        {
            DataSetBase::SetDirty();
        }
    };

    class RegularTestChunk
        : public ReplicaChunk
    {
    public:
        GM_CLASS_ALLOCATOR(RegularTestChunk);

        RegularTestChunk()
        {
        }

        typedef AZStd::intrusive_ptr<RegularTestChunk> Ptr;
        static const char* GetChunkName() { return "RegularTestChunk"; }

        bool IsReplicaMigratable() override { return false; }

        DataSet<AZ::u64> Data1 = { "Data1", 42 };
        DataSet<AZ::u64> Data2 = { "Data2", 0 };
    };

    class CustomMarshalerTestChunk
        : public ReplicaChunk
    {
    public:
        GM_CLASS_ALLOCATOR(CustomMarshalerTestChunk);

        CustomMarshalerTestChunk()
        {
        }

        typedef AZStd::intrusive_ptr<CustomMarshalerTestChunk> Ptr;
        static const char* GetChunkName() { return "CustomMarshalerTestChunk"; }

        bool IsReplicaMigratable() override { return false; }

        DataSet<CustomInt, Marshaler<CustomInt>> Data1 = { "Data1" };
    };

    class LargeChunkWithDefaults
        : public ReplicaChunk
    {
    public:
        GM_CLASS_ALLOCATOR(LargeChunkWithDefaults);

        LargeChunkWithDefaults()
        {
            Data1.MarkAsDefaultValue();
            Data2.MarkAsDefaultValue();
            Data3.MarkAsDefaultValue();
        }

        typedef AZStd::intrusive_ptr<LargeChunkWithDefaults> Ptr;
        static const char* GetChunkName() { return "LargeChunkWithDefaults"; }
        bool IsReplicaMigratable() override { return false; }

        DataSet<int> Data1 = { "Data1", 0 };
        DataSet<int> Data2 = { "Data2", 0 };
        DataSet<int> Data3 = { "Data3", 0 };
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

        DataSet<bool> Data1 = { "Data1", false };
        DataSet<bool> Data2 = { "Data2", false };
        DataSet<bool> Data3 = { "Data3", false };
        DataSet<bool> Data4 = { "Data4", false };
        DataSet<bool> Data5 = { "Data5", false };

        DataSet<bool> Data6 = { "Data6", false };
        DataSet<bool> Data7 = { "Data7", false };
        DataSet<bool> Data8 = { "Data8", false };
        DataSet<bool> Data9 = { "Data9", false };
        DataSet<bool> Data10 = { "Data10", false };
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

        DataSet<AZ::u8> Data1 = { "Data1", 0 };
        DataSet<AZ::u8> Data2 = { "Data2", 0 };
        DataSet<AZ::u8> Data3 = { "Data3", 0 };
        DataSet<AZ::u8> Data4 = { "Data4", 0 };
        DataSet<AZ::u8> Data5 = { "Data5", 0 };

        DataSet<AZ::u8> Data6 = { "Data6", 0 };
        DataSet<AZ::u8> Data7 = { "Data7", 0 };
        DataSet<AZ::u8> Data8 = { "Data8", 0 };
        DataSet<AZ::u8> Data9 = { "Data9", 0 };
        DataSet<AZ::u8> Data10 = { "Data10", 0 };
    };

    class ForcingDirtyTestChunk
        : public ReplicaChunk
    {
    public:
        GM_CLASS_ALLOCATOR(ForcingDirtyTestChunk);

        void ForceDirtyLikeScriptsDo()
        {
            Data1.ForceDirtyLikeScriptsDo();
        }

        typedef AZStd::intrusive_ptr<ForcingDirtyTestChunk> Ptr;
        static const char* GetChunkName() { return "ForcingDirtyTestChunk"; }

        bool IsReplicaMigratable() override { return false; }

        AbleToSetDirtyDataSet Data1 = { "Data1", 42 };
    };

    typedef DataSet<int> EntityLikeScriptDataSetType;

    class EntityLikeScriptDataSet
        : public EntityLikeScriptDataSetType
    {
        static const char* GetDataSetName();

    public:
        GM_CLASS_ALLOCATOR(EntityLikeScriptDataSet);

        EntityLikeScriptDataSet();

        void SetIsEnabled(bool isEnabled)
        {
            m_isEnabled = isEnabled;
        }

        bool IsEnabled() const
        {
            return m_isEnabled;
        }

        PrepareDataResult PrepareData(GridMate::EndianType endianType, AZ::u32 marshalFlags) override
        {
            if (!IsEnabled())
            {
                return PrepareDataResult(false, false, false, false);
            }

            return EntityLikeScriptDataSetType::PrepareData(endianType, marshalFlags);
        }

        void SetDirty() override
        {
            if (!IsEnabled())
            {
                return;
            }

            EntityLikeScriptDataSetType::SetDirty();
        }

    private:

        bool m_isEnabled;
    };

    class EntityLikeScriptReplicaChunk
        : public GridMate::ReplicaChunk
    {
    public:
        static const int k_maxScriptableDataSets = GM_MAX_DATASETS_IN_CHUNK;
    private:

        friend class EntityLikeScriptDataSet;
        friend class EntityReplica;

        typedef AZStd::unordered_map<AZStd::string, int> DataSetIndexMapping;

    public:
        GM_CLASS_ALLOCATOR(EntityLikeScriptReplicaChunk);

        EntityLikeScriptReplicaChunk();
        ~EntityLikeScriptReplicaChunk() override = default;

        //////////////////////////////////////////////////////////////////////
        //! GridMate::ReplicaChunk overrides.
        static const char* GetChunkName() { return "EntityLikeScriptReplicaChunk"; }
        void UpdateChunk(const GridMate::ReplicaContext& rc) override { (void)rc; }
        void OnReplicaActivate(const GridMate::ReplicaContext& rc) override { AZ_UNUSED(rc); }
        void OnReplicaDeactivate(const GridMate::ReplicaContext& rc) override { (void)rc; }
        void UpdateFromChunk(const GridMate::ReplicaContext& rc) override { (void)rc; }
        bool IsReplicaMigratable() override { return false; }
        //////////////////////////////////////////////////////////////////////

        int GetMaxServerProperties() const { return k_maxScriptableDataSets; }

        AZ::u32 CalculateDirtyDataSetMask(MarshalContext& marshalContext) override;

        EntityLikeScriptDataSet m_scriptDataSets[k_maxScriptableDataSets];
        AZ::u32 m_enabledDataSetMask;
    };

    EntityLikeScriptDataSet::EntityLikeScriptDataSet()
        : EntityLikeScriptDataSetType(GetDataSetName())
        , m_isEnabled(false)
    {
    }

    const char* EntityLikeScriptDataSet::GetDataSetName()
    {
        static int          s_chunkIndex = 0;
        static const char*  s_nameArray[] = {
            "DataSet1","DataSet2","DataSet3","DataSet4","DataSet5",
            "DataSet6","DataSet7","DataSet8","DataSet9","DataSet10",
            "DataSet11","DataSet12","DataSet13","DataSet14","DataSet15",
            "DataSet16","DataSet17","DataSet18","DataSet19","DataSet20",
            "DataSet21","DataSet22","DataSet23","DataSet24","DataSet25",
            "DataSet26","DataSet27","DataSet28","DataSet29","DataSet30",
            "DataSet31","DataSet32"
        };

        static_assert(EntityLikeScriptReplicaChunk::k_maxScriptableDataSets <= AZ_ARRAY_SIZE(s_nameArray), "Insufficient number of names supplied to EntityLikeScriptDataSet::GetDataSetName()");

        if (s_chunkIndex > EntityLikeScriptReplicaChunk::k_maxScriptableDataSets)
        {
            s_chunkIndex = s_chunkIndex%EntityLikeScriptReplicaChunk::k_maxScriptableDataSets;
        }

        return s_nameArray[s_chunkIndex++];
    }

    /////////////////////////////
    // EntityLikeScriptReplicaChunk
    /////////////////////////////
    EntityLikeScriptReplicaChunk::EntityLikeScriptReplicaChunk()
        : m_enabledDataSetMask(0)
    {
    }

    AZ::u32 EntityLikeScriptReplicaChunk::CalculateDirtyDataSetMask(MarshalContext& marshalContext)
    {
        if ((marshalContext.m_marshalFlags & ReplicaMarshalFlags::ForceDirty))
        {
            return m_enabledDataSetMask;
        }

        return (m_enabledDataSetMask & GridMate::ReplicaChunk::CalculateDirtyDataSetMask(marshalContext));
    }

    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------
    class MPSession
        : public CarrierEventBus::Handler
    {
    public:
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

    const int k_delay = 50;

    enum class TestStatus
    {
        Running,
        Completed,
    };

    class SimpleBehaviorTest
        : public UnitTest::GridMateMPTestFixture
    {
    public:
        //GM_CLASS_ALLOCATOR(SimpleBehaviorTest);

        SimpleBehaviorTest()
            : m_sessionCount(0) { }

        virtual int GetNumSessions() { return 0; }
        virtual int GetHostSession() { return 0; }
        virtual void PreInit() { }
        virtual void PreConnect() { }
        virtual void PostInit() { }
        virtual TestStatus Tick(int ticks) = 0;

        void run()
        {
            AZ_TracePrintf("GridMate", "\n");

            if (!ReplicaChunkDescriptorTable::Get().FindReplicaChunkDescriptor(ReplicaChunkClassId(ForcingDirtyTestChunk::GetChunkName())))
            {
                ReplicaChunkDescriptorTable::Get().RegisterChunkType<ForcingDirtyTestChunk>();
            }
            if (!ReplicaChunkDescriptorTable::Get().FindReplicaChunkDescriptor(ReplicaChunkClassId(EntityLikeScriptReplicaChunk::GetChunkName())))
            {
                ReplicaChunkDescriptorTable::Get().RegisterChunkType<EntityLikeScriptReplicaChunk>();
            }
            if (!ReplicaChunkDescriptorTable::Get().FindReplicaChunkDescriptor(ReplicaChunkClassId(RegularTestChunk::GetChunkName())))
            {
                ReplicaChunkDescriptorTable::Get().RegisterChunkType<RegularTestChunk>();
            }
            if (!ReplicaChunkDescriptorTable::Get().FindReplicaChunkDescriptor(ReplicaChunkClassId(LargeChunkWithDefaults::GetChunkName())))
            {
                ReplicaChunkDescriptorTable::Get().RegisterChunkType<LargeChunkWithDefaults>();
            }
            if (!ReplicaChunkDescriptorTable::Get().FindReplicaChunkDescriptor(ReplicaChunkClassId(ChunkWithBools::GetChunkName())))
            {
                ReplicaChunkDescriptorTable::Get().RegisterChunkType<ChunkWithBools>();
            }
            if (!ReplicaChunkDescriptorTable::Get().FindReplicaChunkDescriptor(ReplicaChunkClassId(ChunkWithShortInts::GetChunkName())))
            {
                ReplicaChunkDescriptorTable::Get().RegisterChunkType<ChunkWithShortInts>();
            }
            if (!ReplicaChunkDescriptorTable::Get().FindReplicaChunkDescriptor(ReplicaChunkClassId(CustomMarshalerTestChunk::GetChunkName())))
            {
                ReplicaChunkDescriptorTable::Get().RegisterChunkType<CustomMarshalerTestChunk>();
            }

            // Setting up simulator with 50% outgoing packet loss
            DefaultSimulator defaultSimulator;
            defaultSimulator.SetOutgoingPacketLoss(0, 0);

            m_sessionCount = GetNumSessions();

            PreInit();

            // initialize transport
            int basePort = 4427;
            for (int i = 0; i < m_sessionCount; ++i)
            {
                CarrierDesc desc;
                desc.m_port = basePort + i;
                desc.m_enableDisconnectDetection = false;
                desc.m_simulator = &defaultSimulator;

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
                if (Tick(count) == TestStatus::Completed)
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

            for (int i = 0; i < m_sessionCount; ++i)
            {
                m_sessions[i].GetReplicaMgr().Shutdown();
                DefaultCarrier::Destroy(m_sessions[i].GetTransport());
            }
        }

        int m_sessionCount;
        AZStd::array<MPSession, 10> m_sessions;
    };

    /*
    * A hook to intercept the payload size of a replica and it's contents.
    */
    class ReplicaDrillerHook
        : public Debug::ReplicaDrillerBus::Handler
    {
    public:
        ReplicaDrillerHook()
        {
        }

        void OnSendReplicaEnd(Replica* /*replica*/, const void* /*data*/, size_t len) override
        {
            m_replicaLengths.push_back(len);
        }

        void ResetCounts([[maybe_unused]] bool trace = false)
        {
#if defined(AZ_ENABLE_TRACING)
            if (trace && m_replicaLengths.size() > 0)
            {
                AZ_TracePrintf("GridMate", "Driller saw replicas with the following byte sizes:\n");
                for (auto length : m_replicaLengths)
                {
                    AZ_TracePrintf("GridMate", "\t\t\t %d \n", length);
                }
            }
#endif

            m_replicaLengths.clear();
        }

        AZStd::vector<AZ::u64> m_replicaLengths;
    };

    template <typename ReplicaChunkType>
    class FilteredHook : public ReplicaDrillerHook
    {
    public:
        void OnSendReplicaEnd(Replica* replica, const void* /*data*/, size_t len) override
        {
            if (ContainsChunkTypeWeWant(replica))
            {
                m_replicaLengths.push_back(len);
            }
        }

    private:
        bool ContainsChunkTypeWeWant(Replica* replica) const
        {
            auto numChunks = replica->GetNumChunks();
            for (size_t i = 0; i < numChunks; i++)
            {
                auto chunk = replica->GetChunkByIndex(i);
                if (chunk->GetDescriptor()->GetChunkName() == ReplicaChunkType::GetChunkName())
                {
                    return true;
                }
            }

            return false;
        }
    };

    /*
    * The most basic functionality test for sending datasets that have a default value and have not yet been modified
    * from their constructor values.
    *
    * This is a simple sanity check to ensure the logic sends the update when it's necessary.
    */
    class Replica_DontSendDataSets_WithNoDiffFromCtorData
        : public SimpleBehaviorTest
    {
    public:
        Replica_DontSendDataSets_WithNoDiffFromCtorData()
            : m_replicaIdDefault(InvalidReplicaId), m_replicaIdModified(InvalidReplicaId)
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
            m_driller.BusConnect();
            {
                ReplicaPtr replica = Replica::CreateReplica(nullptr);

                auto chunk = CreateAndAttachReplicaChunk<LargeChunkWithDefaults>(replica);
                AZ_TEST_ASSERT(chunk);
                AZ_TEST_ASSERT(chunk->Data1.IsDefaultValue());
                AZ_TEST_ASSERT(chunk->Data2.IsDefaultValue());

                m_replicaIdDefault = m_sessions[sHost].GetReplicaMgr().AddPrimary(replica);
            }
        }

        const int ExpectedReplicaSizeWithDefaults = 37;
        const int ExpectedReplicaSizeWithNonDefaults = 46;

        TestStatus Tick(int tick) override
        {
            switch (tick)
            {
            case 20:
            {
                {
                    ReplicaPtr rep = m_sessions[s2].GetReplicaMgr().FindReplica(m_replicaIdDefault);
                    AZ_TEST_ASSERT(rep);

                    auto chunk = rep->FindReplicaChunk<LargeChunkWithDefaults>();
                    AZ_TEST_ASSERT(chunk);

                    auto replicaSize = m_driller.m_replicaLengths[0];
                    AZ_TEST_ASSERT(replicaSize == ExpectedReplicaSizeWithDefaults);
                    m_driller.ResetCounts();
                }
                // create another replica with non-default values
                {
                    ReplicaPtr replica = Replica::CreateReplica(nullptr);

                    auto chunk = CreateAndAttachReplicaChunk<LargeChunkWithDefaults>(replica);
                    AZ_TEST_ASSERT(chunk);

                    AZ_TEST_ASSERT(chunk->Data1.IsDefaultValue());
                    AZ_TEST_ASSERT(chunk->Data2.IsDefaultValue());
                    chunk->Data1.Set(4242);
                    chunk->Data2.Set(4242);
                    AZ_TEST_ASSERT(!chunk->Data1.IsDefaultValue());
                    AZ_TEST_ASSERT(!chunk->Data2.IsDefaultValue());

                    m_replicaIdModified = m_sessions[sHost].GetReplicaMgr().AddPrimary(replica);
                }
                break;
            }
            case 40:
            {
                {
                    ReplicaPtr rep = m_sessions[s2].GetReplicaMgr().FindReplica(m_replicaIdModified);
                    AZ_TEST_ASSERT(rep);

                    auto chunk = rep->FindReplicaChunk<LargeChunkWithDefaults>();
                    AZ_TEST_ASSERT(chunk);

                    auto replicaSize = m_driller.m_replicaLengths[0];
                    AZ_TEST_ASSERT(replicaSize == ExpectedReplicaSizeWithNonDefaults);
                    m_driller.ResetCounts();

                    // check that non-default values are set for the dataset
                    {
                        AZ_TEST_ASSERT(!chunk->Data1.IsDefaultValue());
                        auto value = chunk->Data1.Get();
                        AZ_TEST_ASSERT(value == 4242);
                    }
                    {
                        AZ_TEST_ASSERT(!chunk->Data2.IsDefaultValue());
                        auto value = chunk->Data2.Get();
                        AZ_TEST_ASSERT(value == 4242);
                    }
                }
                m_driller.ResetCounts(true);
                break;
            }
            case 45:
            {
                {
                    m_sessions[sHost].GetReplicaMgr().FindReplica(m_replicaIdDefault)->Destroy();
                    m_sessions[sHost].GetReplicaMgr().FindReplica(m_replicaIdModified)->Destroy();
                }
                break;
            }
            case 50:
                return TestStatus::Completed;
            default:
                break;
            }
            return TestStatus::Running;
        }

        ReplicaId m_replicaIdDefault;
        ReplicaId m_replicaIdModified;
        FilteredHook<LargeChunkWithDefaults> m_driller;
    };

    TEST(Replica_DontSendDataSets_WithNoDiffFromCtorData, DISABLED_Replica_DontSendDataSets_WithNoDiffFromCtorData)
    {
        Replica_DontSendDataSets_WithNoDiffFromCtorData tester;
        tester.run();
    }

    /*
    * This test checks the actual size of the replica as marshalled in the binary payload.
    * The assessment of the payload size is done using driller EBus.
    */
    class ReplicaDefaultDataSetDriller
        : public SimpleBehaviorTest
    {
    public:
        ReplicaDefaultDataSetDriller()
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

        static const int NonDefaultValue = 4242;

        void PreConnect() override
        {
            m_driller.BusConnect();

            ReplicaPtr replica = Replica::CreateReplica(nullptr);
            LargeChunkWithDefaults* chunk = CreateAndAttachReplicaChunk<LargeChunkWithDefaults>(replica);
            AZ_TEST_ASSERT(chunk);

            m_replicaId = m_sessions[sHost].GetReplicaMgr().AddPrimary(replica);
        }

        ~ReplicaDefaultDataSetDriller() override
        {
            m_driller.BusDisconnect();
        }

        TestStatus Tick(int tick) override
        {
            switch (tick)
            {
            case 10:
            {
                auto rep = m_sessions[s2].GetReplicaMgr().FindReplica(m_replicaId);
                AZ_TEST_ASSERT(rep);

                m_driller.ResetCounts();

                break;
            }
            case 15:
            {
                ReplicaPtr replica = m_sessions[sHost].GetReplicaMgr().FindReplica(m_replicaId);
                auto chunk = replica->FindReplicaChunk<LargeChunkWithDefaults>();
                int nonDefaultValue = NonDefaultValue;
                auto touch = [nonDefaultValue](DataSet<int>& dataSet) { dataSet.Set(nonDefaultValue); };
                touch(chunk->Data1);
                touch(chunk->Data2);
                touch(chunk->Data3);

                m_driller.ResetCounts();

                break;
            }
            case 20:
            {
                auto repLengths = m_driller.m_replicaLengths;
                m_driller.ResetCounts();

                // check exact expected sizes
                const auto countUnreliable = 4;
                const auto countReliable = 1;
                const auto expectedReplicaSize = 22;

                AZ_TEST_ASSERT(repLengths.size() == countUnreliable + countReliable);
                for (auto length : repLengths)
                {
                    AZ_TEST_ASSERT(length == expectedReplicaSize);
                }

                m_sessions[sHost].GetReplicaMgr().FindReplica(m_replicaId)->Destroy();
                break;
            }
            case 25:
            {
                return TestStatus::Completed;
            }
            default:
                break;
            }
            return TestStatus::Running;
        }

        ReplicaDrillerHook m_driller;
        ReplicaId m_replicaId;
    };
    
    const int ReplicaDefaultDataSetDriller::NonDefaultValue;

    TEST(ReplicaDefaultDataSetDriller, DISABLED_ReplicaDefaultDataSetDriller)
    {
        ReplicaDefaultDataSetDriller tester;
        tester.run();
    }

    /*
    * This test checks the actual size of the replica as marshalled in the binary payload.
    * The assessment of the payload size is done using driller EBus.
    */
    class Replica_ComparePackingBoolsVsU8
        : public SimpleBehaviorTest
    {
    public:
        Replica_ComparePackingBoolsVsU8()
            : m_replicaBoolsId(InvalidReplicaId)
            , m_replicaU8Id(InvalidReplicaId)
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
            m_driller.BusConnect();

            ReplicaPtr replica1 = Replica::CreateReplica(nullptr);
            ChunkWithBools* chunk1 = CreateAndAttachReplicaChunk<ChunkWithBools>(replica1);
            AZ_TEST_ASSERT(chunk1);

            m_replicaBoolsId = m_sessions[sHost].GetReplicaMgr().AddPrimary(replica1);

            ReplicaPtr replica2 = Replica::CreateReplica(nullptr);
            ChunkWithShortInts* chunk2 = CreateAndAttachReplicaChunk<ChunkWithShortInts>(replica2);
            AZ_TEST_ASSERT(chunk2);

            m_replicaU8Id = m_sessions[sHost].GetReplicaMgr().AddPrimary(replica2);
        }

        ~Replica_ComparePackingBoolsVsU8() override
        {
            m_driller.BusDisconnect();
        }

        TestStatus Tick(int tick) override
        {
            switch (tick)
            {
            case 10:
            {
                auto rep1 = m_sessions[s2].GetReplicaMgr().FindReplica(m_replicaBoolsId);
                AZ_TEST_ASSERT(rep1);
                auto rep2 = m_sessions[s2].GetReplicaMgr().FindReplica(m_replicaU8Id);
                AZ_TEST_ASSERT(rep2);
                break;
            }
            case 15:
            {
                // we have to poke the values so that they become non-default
                {
                    ReplicaPtr replica = m_sessions[sHost].GetReplicaMgr().FindReplica(m_replicaBoolsId);
                    auto chunk = replica->FindReplicaChunk<ChunkWithBools>();

                    auto touch = [](DataSet<bool>& dataSet) { dataSet.Set(true); };
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
                }
                {
                    ReplicaPtr replica = m_sessions[sHost].GetReplicaMgr().FindReplica(m_replicaU8Id);
                    auto chunk = replica->FindReplicaChunk<ChunkWithShortInts>();

                    auto touch = [](DataSet<AZ::u8>& dataSet) { dataSet.Set(42); };
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
                }
                m_driller.ResetCounts();

                break;
            }
            case 30:
            {
                auto repLengths = m_driller.m_replicaLengths;
                m_driller.ResetCounts();

                // check exact expected sizes
                const auto expectedReplicaSizeWithBools = 12;
                const auto expectedReplicaSizeWithShortInts = 20;

                AZ_TEST_ASSERT(repLengths.size() >= 2);
                AZ_TEST_ASSERT(AZStd::find(repLengths.begin(), repLengths.end(), expectedReplicaSizeWithBools));
                AZ_TEST_ASSERT(AZStd::find(repLengths.begin(), repLengths.end(), expectedReplicaSizeWithShortInts));

                m_sessions[sHost].GetReplicaMgr().FindReplica(m_replicaBoolsId)->Destroy();
                m_sessions[sHost].GetReplicaMgr().FindReplica(m_replicaU8Id)->Destroy();
                break;
            }
            case 35:
            {
                //auto boolDatasetSize = m_driller.m_boolChunkLengths[1];
                //auto u8DatasetSize = m_driller.m_u8ChunkLengths[1];
                //AZ_TEST_ASSERT(boolDatasetSize < u8DatasetSize); // Observed example: 5bytes < 13bytes

                return TestStatus::Completed;
            }
            default:
                break;
            }
            return TestStatus::Running;
        }

        ReplicaDrillerHook m_driller;
        ReplicaId m_replicaBoolsId;
        ReplicaId m_replicaU8Id;
    };

    TEST(Replica_ComparePackingBoolsVsU8, DISABLED_Replica_ComparePackingBoolsVsU8)
    {
        Replica_ComparePackingBoolsVsU8 tester;
        tester.run();
    }

    class CheckDataSetStreamIsntWrittenMoreThanNecessary
        : public SimpleBehaviorTest
    {
    public:
        CheckDataSetStreamIsntWrittenMoreThanNecessary()
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

        static const int NonDefaultValue = 4242;

        void PreConnect() override
        {
            m_driller.BusConnect();

            ReplicaPtr replica = Replica::CreateReplica(nullptr);
            auto chunk = CreateAndAttachReplicaChunk<CustomMarshalerTestChunk>(replica);
            AZ_TEST_ASSERT(chunk);

            m_replicaId = m_sessions[sHost].GetReplicaMgr().AddPrimary(replica);
        }

        ~CheckDataSetStreamIsntWrittenMoreThanNecessary() override
        {
            m_driller.BusDisconnect();
        }

        CustomMarshalerTestChunk::Ptr GetHostChunk()
        {
            ReplicaPtr replica = m_sessions[sHost].GetReplicaMgr().FindReplica(m_replicaId);
            auto chunk = replica->FindReplicaChunk<CustomMarshalerTestChunk>();

            return chunk;
        }

        TestStatus Tick(int tick) override
        {
            switch (tick)
            {
            case 10:
            {
                auto rep = m_sessions[s2].GetReplicaMgr().FindReplica(m_replicaId);
                AZ_TEST_ASSERT(rep);
                break;
            }
            case 15:
            {
                auto chunk = GetHostChunk();
                //chunk->Data1.Set(CustomInt(41));

                const auto& m = chunk->Data1.GetMarshaler();
                // Only the initial setup call should have occurred
                AZ_TEST_ASSERT(m.m_marshalCalls == 1);
                m.m_marshalCalls = 0;
                m_driller.ResetCounts();

                break;
            }
            case 42:
            {
                auto chunk = GetHostChunk();
                const auto& m = chunk->Data1.GetMarshaler();
                // No reason for any new calls to occur
                AZ_TEST_ASSERT(m.m_marshalCalls == 0);

                m_sessions[sHost].GetReplicaMgr().FindReplica(m_replicaId)->Destroy();
                break;
            }
            case 45:
            {
                return TestStatus::Completed;
            }
            default:
                break;
            }
            return TestStatus::Running;
        }

        ReplicaDrillerHook m_driller;
        ReplicaId m_replicaId;
    };

    TEST(CheckDataSetStreamIsntWrittenMoreThanNecessary, DISABLED_CheckDataSetStreamIsntWrittenMoreThanNecessary)
    {
        CheckDataSetStreamIsntWrittenMoreThanNecessary tester;
        tester.run();
    }

    class CheckDataSetStreamIsntWrittenMoreThanNecessaryOnceDirty
        : public SimpleBehaviorTest
    {
    public:
        CheckDataSetStreamIsntWrittenMoreThanNecessaryOnceDirty()
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

        static const int NonDefaultValue = 4242;

        void PreConnect() override
        {
            m_driller.BusConnect();

            ReplicaPtr replica = Replica::CreateReplica(nullptr);
            auto chunk = CreateAndAttachReplicaChunk<CustomMarshalerTestChunk>(replica);
            AZ_TEST_ASSERT(chunk);

            m_replicaId = m_sessions[sHost].GetReplicaMgr().AddPrimary(replica);
        }

        ~CheckDataSetStreamIsntWrittenMoreThanNecessaryOnceDirty() override
        {
            m_driller.BusDisconnect();
        }

        CustomMarshalerTestChunk::Ptr GetHostChunk()
        {
            ReplicaPtr replica = m_sessions[sHost].GetReplicaMgr().FindReplica(m_replicaId);
            auto chunk = replica->FindReplicaChunk<CustomMarshalerTestChunk>();

            return chunk;
        }

        TestStatus Tick(int tick) override
        {
            switch (tick)
            {
            case 10:
            {
                auto rep = m_sessions[s2].GetReplicaMgr().FindReplica(m_replicaId);
                AZ_TEST_ASSERT(rep);
                break;
            }
            case 15:
            {
                auto chunk = GetHostChunk();
                chunk->Data1.Set(CustomInt(41));

                const auto& m = chunk->Data1.GetMarshaler();
                // Only the initial setup call
                AZ_TEST_ASSERT(m.m_marshalCalls == 1);
                m.m_marshalCalls = 0;
                m_driller.ResetCounts();

                break;
            }
            case 42:
            {
                auto chunk = GetHostChunk();
                const auto& m = chunk->Data1.GetMarshaler();
                AZ_TEST_ASSERT(m.m_marshalCalls == 6 /* 5 unreliables + 1 reliable */);

                m_sessions[sHost].GetReplicaMgr().FindReplica(m_replicaId)->Destroy();
                break;
            }
            case 45:
            {
                return TestStatus::Completed;
            }
            default:
                break;
            }
            return TestStatus::Running;
        }

        ReplicaDrillerHook m_driller;
        ReplicaId m_replicaId;
    };

    TEST(CheckDataSetStreamIsntWrittenMoreThanNecessaryOnceDirty, DISABLED_CheckDataSetStreamIsntWrittenMoreThanNecessaryOnceDirty)
    {
        CheckDataSetStreamIsntWrittenMoreThanNecessaryOnceDirty tester;
        tester.run();
    }

    class CheckReplicaIsntSentWithNoChanges
        : public SimpleBehaviorTest
    {
    public:
        CheckReplicaIsntSentWithNoChanges()
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
            m_driller.BusConnect();

            ReplicaPtr replica = Replica::CreateReplica(nullptr);
            ForcingDirtyTestChunk* chunk = CreateAndAttachReplicaChunk<ForcingDirtyTestChunk>(replica);
            AZ_TEST_ASSERT(chunk);

            m_replicaId = m_sessions[sHost].GetReplicaMgr().AddPrimary(replica);
        }

        ~CheckReplicaIsntSentWithNoChanges() override
        {
            m_driller.BusDisconnect();
        }

        const int NewValue = 999;
        const int MomentaryValue = 1;
        const int ExpectedNumberReplicasSent = 6;

        ReplicaPtr GetHostReplica()
        {
            return m_sessions[sHost].GetReplicaMgr().FindReplica(m_replicaId);
        }

        TestStatus Tick(int tick) override
        {
            switch (tick)
            {
            case 9:
            {
                auto rep = GetHostReplica();
                AZ_TEST_ASSERT(rep);
                m_driller.ResetCounts();

                auto chunk = rep->FindReplicaChunk<ForcingDirtyTestChunk>();
                chunk->Data1.Set(NewValue);

                break;
            }
            case 15:
            {
                auto rep = GetHostReplica();
                AZ_TEST_ASSERT(rep);

                auto counts = m_driller.m_replicaLengths.size();
                AZ_TEST_ASSERT(counts == ExpectedNumberReplicasSent);
                m_driller.ResetCounts();

                auto chunk = rep->FindReplicaChunk<ForcingDirtyTestChunk>();
                chunk->Data1.Set(MomentaryValue);

                break;
            }
            case 16:
            {
                auto rep = GetHostReplica();
                AZ_TEST_ASSERT(rep);
                auto chunk = rep->FindReplicaChunk<ForcingDirtyTestChunk>();
                chunk->Data1.Set(NewValue);

                auto counts = m_driller.m_replicaLengths.size();
                AZ_TEST_ASSERT(counts == 1);
                m_driller.ResetCounts();

                break;
            }
            case 100:
            {
                auto counts = m_driller.m_replicaLengths.size();
                AZ_TEST_ASSERT(counts == ExpectedNumberReplicasSent);
                m_driller.ResetCounts();

                m_sessions[sHost].GetReplicaMgr().FindReplica(m_replicaId)->Destroy();
                return TestStatus::Completed;
            }
            default:
                break;
            }
            return TestStatus::Running;
        }

        FilteredHook<ForcingDirtyTestChunk> m_driller;
        ReplicaId m_replicaId;
    };

    TEST(CheckReplicaIsntSentWithNoChanges, DISABLED_CheckReplicaIsntSentWithNoChanges)
    {
        CheckReplicaIsntSentWithNoChanges tester;
        tester.run();
    }

    class CheckEntityScriptReplicaIsntSentWithNoChanges
        : public SimpleBehaviorTest
    {
    public:
        CheckEntityScriptReplicaIsntSentWithNoChanges()
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
            m_driller.BusConnect();

            ReplicaPtr replica = Replica::CreateReplica(nullptr);
            auto chunk = CreateAndAttachReplicaChunk<EntityLikeScriptReplicaChunk>(replica);
            AZ_TEST_ASSERT(chunk);

            m_replicaId = m_sessions[sHost].GetReplicaMgr().AddPrimary(replica);
        }

        ~CheckEntityScriptReplicaIsntSentWithNoChanges() override
        {
            m_driller.BusDisconnect();
        }

        const int NewValue = 999;
        const int MomentaryValue = 1;
        const int ExpectedNumberReplicasSent = 6;

        ReplicaPtr GetHostReplica()
        {
            return m_sessions[sHost].GetReplicaMgr().FindReplica(m_replicaId);
        }

        TestStatus Tick(int tick) override
        {
            switch (tick)
            {
            case 10:
            {
                auto rep = GetHostReplica();
                AZ_TEST_ASSERT(rep);
                m_driller.ResetCounts();

                auto chunk = rep->FindReplicaChunk<EntityLikeScriptReplicaChunk>();

                // mimicing behavior of entity script chunk
                chunk->m_scriptDataSets[0].SetIsEnabled(true);
                chunk->m_scriptDataSets[0].Set(NewValue);

                break;
            }
            case 60:
            {
                auto counts = m_driller.m_replicaLengths.size();
                AZ_TEST_ASSERT(counts == ExpectedNumberReplicasSent);
                m_driller.ResetCounts();

                m_sessions[sHost].GetReplicaMgr().FindReplica(m_replicaId)->Destroy();
                return TestStatus::Completed;
            }
            default:
                break;
            }
            return TestStatus::Running;
        }

        ReplicaDrillerHook m_driller;
        ReplicaId m_replicaId;
    };

    TEST(CheckEntityScriptReplicaIsntSentWithNoChanges, DISABLED_CheckEntityScriptReplicaIsntSentWithNoChanges)
    {
        CheckEntityScriptReplicaIsntSentWithNoChanges tester;
        tester.run();
    }

} // namespace ReplicaBehavior
} // namespace UnitTest
