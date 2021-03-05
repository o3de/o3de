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

#include <AzFramework/Network/NetworkContext.h>
#include <AzFramework/Application/Application.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Memory/MemoryComponent.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>

#include <GridMate/Replica/ReplicaChunk.h>
#include <GridMate/Replica/DataSet.h>
#include <GridMate/Replica/RemoteProcedureCall.h>
#include <GridMate/Serialize/DataMarshal.h>
#include <GridMate/Serialize/UtilityMarshal.h>
#include <GridMate/Serialize/ContainerMarshal.h>
#include <GridMate/Replica/ReplicaMgr.h>
#include <AzFramework/Network/InterestManagerComponent.h>

namespace UnitTest
{
    using namespace AZ;
    using namespace AzFramework;

    class TestComponentExternalChunk
        : public AZ::Component
        , public NetBindable
    {
    public:
        AZ_COMPONENT(TestComponentExternalChunk, "{73BB3B15-7C4D-4BD5-9568-F3B2DCBC7725}", AZ::Component);

        static void Reflect(ReflectContext* context);

        void Init() override
        {
            NetBindable::NetInit();
        }

        void Activate() override {}
        void Deactivate() override {}

        bool SetPos(float x, float y, const RpcContext&)
        {
            m_x = x;
            m_y = y;
            return true;
        }

        void OnFloatChanged(const float&, const TimeContext&)
        {
            m_floatChanged = true;
        }

        bool m_floatChanged = false;
    private:
        float m_x = 0, m_y = 0;
    };

    class TestComponentReplicaChunk
        : public ReplicaChunkBase
        , public ReplicaChunkInterface
    {
    public:
        GM_CLASS_ALLOCATOR(TestComponentReplicaChunk);
        static const char* GetChunkName() { return "TestComponentReplicaChunk"; }
        bool IsReplicaMigratable() override { return true; }

    public:
        TestComponentReplicaChunk()
            : m_int("m_int", 42)
            , m_float("m_float", 96.4f)
            , SetInt("SetInt")
            , SetPos("SetPos")
        {
        }

        bool SetIntImpl(int newValue, const RpcContext&)
        {
            m_int.Set(newValue);
            return true;
        }

        DataSet<int> m_int;
        DataSet<float>::BindInterface<TestComponentExternalChunk, &TestComponentExternalChunk::OnFloatChanged> m_float;
        GridMate::Rpc<GridMate::RpcArg<int, Marshaler<int> > >::BindInterface<TestComponentReplicaChunk, &TestComponentReplicaChunk::SetIntImpl> SetInt;
        GridMate::Rpc<GridMate::RpcArg<float>, GridMate::RpcArg<float> >::BindInterface<TestComponentExternalChunk, &TestComponentExternalChunk::SetPos> SetPos;
    };

    void TestComponentExternalChunk::Reflect(ReflectContext* context)
    {
        NetworkContext* netContext = azrtti_cast<NetworkContext*>(context);
        if (netContext)
        {
            netContext->Class<TestComponentExternalChunk>()
                ->Chunk<TestComponentReplicaChunk>()
                ->Field("m_int", &TestComponentReplicaChunk::m_int)
                ->Field("m_float", &TestComponentReplicaChunk::m_float)
                ->RPC("SetInt", &TestComponentReplicaChunk::SetInt)
                ->RPC("SetPos", &TestComponentReplicaChunk::SetPos);
        }
    }

    class TestComponentAutoChunk
        : public AZ::Component
        , public NetBindable
    {
    public:
        enum TestEnum
        {
            TEST_Value0 = 0,
            TEST_Value1 = 1,
            TEST_Value255 = 255
        };

        AZ_COMPONENT(TestComponentAutoChunk, "{003FD1BC-8456-43D5-9879-1B3804327A4F}", AZ::Component);

        static void Reflect(ReflectContext* context)
        {
            NetworkContext* netContext = azrtti_cast<NetworkContext*>(context);
            if (netContext)
            {
                netContext->Class<TestComponentAutoChunk>()
                    ->Field("m_int", &TestComponentAutoChunk::m_int)
                    ->Field("m_float", &TestComponentAutoChunk::m_float)
                    ->Field("m_enum", &TestComponentAutoChunk::m_enum)
                    ->RPC("SetInt", &TestComponentAutoChunk::SetInt)
                    ->CtorData("CtorInt", &TestComponentAutoChunk::GetCtorInt, &TestComponentAutoChunk::SetCtorInt)
                    ->CtorData("CtorVec", &TestComponentAutoChunk::GetCtorVec, &TestComponentAutoChunk::SetCtorVec);
            }
            SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<TestComponentAutoChunk, AZ::Component>()
                    ->Version(1)
                    ->Field("m_int", &TestComponentAutoChunk::m_int)
                    ->Field("m_float", &TestComponentAutoChunk::m_float)
                    ->Field("m_enum", &TestComponentAutoChunk::m_enum)
                    ->Field("ctorInt", &TestComponentAutoChunk::m_ctorInt)
                    ->Field("ctorVec", &TestComponentAutoChunk::m_ctorVec);
            }
        }

        void Init() override
        {
            NetBindable::NetInit();
        }

        void Activate() override {}
        void Deactivate() override {}

        void SetNetworkBinding(ReplicaChunkPtr chunk) override {}
        void UnbindFromNetwork() override {}

        bool SetIntImpl(int val, const RpcContext&)
        {
            m_int = val;
            return true;
        }

        void OnFloatChanged(const float&, const TimeContext&)
        {
        }

        int GetCtorInt() const { return m_ctorInt; }
        void SetCtorInt(const int& ctorInt) { m_ctorInt = ctorInt; }

        AZStd::vector<int>& GetCtorVec() { return m_ctorVec; }
        void SetCtorVec(const AZStd::vector<int>& vec) { m_ctorVec = vec; }

        int m_ctorInt;
        AZStd::vector<int> m_ctorVec;
        Field<int> m_int;
        BoundField<float, TestComponentAutoChunk, &TestComponentAutoChunk::OnFloatChanged> m_float;
        Field<TestEnum, GridMate::ConversionMarshaler<AZ::u8, TestEnum> > m_enum;
        Rpc<int>::Binder<TestComponentAutoChunk, &TestComponentAutoChunk::SetIntImpl> SetInt;
    };

    class NetContextReflectionTest
        : public AllocatorsTestFixture
    {
    public:
        void SetUp() override
        {
            AllocatorsTestFixture::SetUp();

            AZ::AllocatorInstance<GridMate::GridMateAllocatorMP>::Create();
        }

        void TearDown() override
        {
            AZ::AllocatorInstance<GridMate::GridMateAllocatorMP>::Destroy();

            AllocatorsTestFixture::TearDown();
        }

        void run()
        {
            AzFramework::Application app;
            AzFramework::Application::Descriptor appDesc;
            appDesc.m_recordingMode = Debug::AllocationRecords::RECORD_NO_RECORDS;
            appDesc.m_allocationRecords = false;
            appDesc.m_enableDrilling = false;

            app.Start(appDesc);

            // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
            // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash 
            // in the unit tests.
            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);

            AzFramework::NetworkContext* netContext = nullptr;
            EBUS_EVENT_RESULT(netContext, NetSystemRequestBus, GetNetworkContext);
            AZ_TEST_ASSERT(netContext);

            AZ::ComponentDescriptor* descTestComponentExternalChunk = TestComponentExternalChunk::CreateDescriptor();
            app.RegisterComponentDescriptor(descTestComponentExternalChunk);

            AZ::ComponentDescriptor* descTestComponentAutoChunk = TestComponentAutoChunk::CreateDescriptor();
            app.RegisterComponentDescriptor(descTestComponentAutoChunk);

            AZ::Entity* testEntity = aznew AZ::Entity("TestEntity");
            testEntity->Init();
            testEntity->CreateComponent<TestComponentAutoChunk>();
            testEntity->CreateComponent<TestComponentExternalChunk>();
            testEntity->Activate();

            // test field binding/auto reflection/creation
            {
                TestComponentAutoChunk* testComponent = testEntity->FindComponent<TestComponentAutoChunk>();
                AZ_TEST_ASSERT(testComponent);

                testComponent->SetInt(2048); // should happen locally
                AZ_TEST_ASSERT(testComponent->m_int == 2048);

                ReplicaChunkPtr chunk = testComponent->GetNetworkBinding();
                AZ_TEST_ASSERT(chunk);

                GridMate::ReplicaChunkDescriptor* desc = chunk->GetDescriptor();
                AZ_TEST_ASSERT(desc);

                testComponent->m_ctorInt = 8192;
                for (int n = 0; n < 16; ++n)
                {
                    testComponent->m_ctorVec.push_back(n);
                }

                GridMate::WriteBufferDynamic wb(GridMate::EndianType::IgnoreEndian);
                desc->MarshalCtorData(chunk.get(), wb);

                {
                    // Create a chunk from the recorded ctor data, ensure that it stores
                    // the ctor data in preparation for copying it to the instance
                    GridMate::TimeContext tc;
                    GridMate::ReplicaContext rc(nullptr, tc);
                    GridMate::ReadBuffer rb(wb.GetEndianType(), wb.Get(), wb.Size());
                    GridMate::UnmarshalContext ctx(rc);
                    ctx.m_hasCtorData = true;
                    ctx.m_iBuf = &rb;
                    ReplicaChunkPtr chunk2 = desc->CreateFromStream(ctx);
                    AZ_TEST_ASSERT(chunk2); // ensure a new chunk was created
                    ReflectedReplicaChunkBase* refChunk = static_cast<ReflectedReplicaChunkBase*>(chunk2.get());
                    AZ_TEST_ASSERT(refChunk->m_ctorBuffer.Size() == sizeof(int) + sizeof(AZ::u16) + (sizeof(int) * testComponent->m_ctorVec.size()));
                }

                {
                    // discard a ctor data stream and ensure that the stream is emptied
                    GridMate::TimeContext tc;
                    GridMate::ReplicaContext rc(nullptr, tc);
                    GridMate::ReadBuffer rb(wb.GetEndianType(), wb.Get(), wb.Size());
                    GridMate::UnmarshalContext ctx(rc);
                    ctx.m_hasCtorData = true;
                    ctx.m_iBuf = &rb;
                    desc->DiscardCtorStream(ctx);
                    AZ_TEST_ASSERT(rb.IsEmptyIgnoreTrailingBits()); // should have discarded the whole stream
                }

                {
                    // Make another chunk and bind it to a new component and make sure the ctor data matches
                    AZ::Entity* testEntity2 = aznew AZ::Entity("TestEntity2");
                    testEntity2->Init();
                    testEntity2->CreateComponent<TestComponentAutoChunk>();
                    testEntity2->Activate();

                    GridMate::TimeContext tc;
                    GridMate::ReplicaContext rc(nullptr, tc);
                    GridMate::ReadBuffer rb(wb.GetEndianType(), wb.Get(), wb.Size());
                    GridMate::UnmarshalContext ctx(rc);
                    ctx.m_hasCtorData = true;
                    ctx.m_iBuf = &rb;
                    ReplicaChunkPtr chunk2 = desc->CreateFromStream(ctx);

                    TestComponentAutoChunk* testComponent2 = testEntity2->FindComponent<TestComponentAutoChunk>();
                    netContext->Bind(testComponent2, chunk2, NetworkContextBindMode::NonAuthoritative);
                    // Ensure values match after ctor data is applied
                    AZ_TEST_ASSERT(testComponent2->m_ctorInt == testComponent->m_ctorInt);
                    AZ_TEST_ASSERT(testComponent2->m_ctorVec == testComponent->m_ctorVec);
                }

                testComponent->SetInt(4096);
                AZ_TEST_ASSERT(testComponent->m_int == 4096);

                testComponent->m_int = 42; // now it should change
                AZ_TEST_ASSERT(testComponent->m_int == 42);
                testComponent->m_enum = TestComponentAutoChunk::TEST_Value1;

                chunk.reset(); // should cause netContext->DestroyReplicaChunk()
            }

            // test chunk binding/creation
            {
                TestComponentExternalChunk* testComponent = testEntity->FindComponent<TestComponentExternalChunk>();
                AZ_TEST_ASSERT(testComponent);

                ReplicaChunkPtr chunk = testComponent->GetNetworkBinding();
                AZ_TEST_ASSERT(chunk);

                TestComponentReplicaChunk* testChunk = static_cast<TestComponentReplicaChunk*>(chunk.get());

                // for now, this will throw a warning, but will at least attempt the dispatch
                testChunk->SetPos(42.0f, 96.0f);

                AZ_TEST_ASSERT(testComponent->m_floatChanged == false);
                testChunk->m_float.Set(1024.0f);
                // I would like to test that the notify fired, but without a Replica, cant :(

                testComponent->UnbindFromNetwork();
                chunk.reset(); ///// CRASHES FROM HERE
            }

            // test serialization of NetBindable::Fields
            {
                TestComponentAutoChunk* testComponent = testEntity->FindComponent<TestComponentAutoChunk>();
                AZStd::vector<AZ::u8> buffer;
                AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8> > saveStream(&buffer);
                bool saved = AZ::Utils::SaveObjectToStream(saveStream, AZ::DataStream::ST_XML, testComponent);
                AZ_TEST_ASSERT(saved);
                AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8> > loadStream(&buffer);
                TestComponentAutoChunk* testCopy = AZ::Utils::LoadObjectFromStream<TestComponentAutoChunk>(loadStream);
                AZ_TEST_ASSERT(testCopy);
                delete testCopy;
            }

            testEntity->Deactivate();
            delete testEntity;

            descTestComponentExternalChunk->ReleaseDescriptor();
            descTestComponentAutoChunk->ReleaseDescriptor();

            app.Stop();
        }
    };

    TEST_F(NetContextReflectionTest, Test)
    {
        run();
    }

    template <typename ComponentType>
    class NetContextFixture
        : public ::testing::Test
    {
    public:
        NetContextFixture() = default;
        ~NetContextFixture() = default;

        void SetUp() override
        {
            AZ::AllocatorInstance<SystemAllocator>::Create();

            m_app = AZStd::make_unique<AzFramework::Application>();
            m_app->Start(AzFramework::Application::Descriptor());

            // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
            // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash 
            // in the unit tests.
            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);

            AzFramework::NetworkContext* netContext = nullptr;
            EBUS_EVENT_RESULT(netContext, NetSystemRequestBus, GetNetworkContext);
            AZ_TEST_ASSERT(netContext);

            m_descTestComponentAutoChunk = ComponentType::CreateDescriptor();
            m_app->RegisterComponentDescriptor(m_descTestComponentAutoChunk);

            m_entity = AZStd::make_unique<AZ::Entity>("TestEntity");
            m_entity->Init();
            m_entity->CreateComponent<ComponentType>();
            m_entity->Activate();
        }

        void TearDown() override
        {
            m_descTestComponentAutoChunk->ReleaseDescriptor();

            m_entity->Deactivate();
            m_entity.reset();

            m_app->Stop();
            m_app.reset();

            AZ::AllocatorInstance<SystemAllocator>::Destroy();
        }

        void RunTest()
        {
            const ComponentType* testComponent = m_entity->FindComponent<ComponentType>();
            AZStd::vector<AZ::u8> buffer;
            AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8> > saveStream(&buffer);
            const bool saved = AZ::Utils::SaveObjectToStream(saveStream, AZ::DataStream::ST_XML, testComponent);
            AZ_TEST_ASSERT(saved);
            AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8> > loadStream(&buffer);
            const AZStd::unique_ptr<ComponentType> testCopy(AZ::Utils::LoadObjectFromStream<ComponentType>(loadStream));
            AZ_TEST_ASSERT(testCopy);
        }

        AZStd::unique_ptr<AzFramework::Application> m_app;
        AZStd::unique_ptr<AZ::Entity> m_entity;
        AZ::ComponentDescriptor* m_descTestComponentAutoChunk = nullptr;
    };

    class TestComponent_EmptyNetContext
        : public AZ::Component
        , public NetBindable
    {
    public:
        AZ_COMPONENT(TestComponent_EmptyNetContext, "{B1E2E2DD-DA70-4D59-A185-AF9A5CCF1574}", AZ::Component, NetBindable);

        static void Reflect(ReflectContext* context)
        {
            if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<TestComponent_EmptyNetContext, AZ::Component>()
                    ->Version(1);
            }
            if (NetworkContext* netContext = azrtti_cast<NetworkContext*>(context))
            {
                netContext->Class<TestComponent_EmptyNetContext>();
            }
        }

        void Activate() override {}
        void Deactivate() override {}
    };

    using NetContextEmpty = NetContextFixture<TestComponent_EmptyNetContext>;
    TEST_F(NetContextEmpty, SerializationTests)
    {
        RunTest();
    }

    template<typename FieldType>
    class TestComponent_OneField
        : public AZ::Component
        , public NetBindable
    {
    public:
        AZ_COMPONENT(TestComponent_OneField, "{A7BCDBEF-3D4F-4D04-A6FA-DF48D4B66ABE}", AZ::Component, NetBindable);

        using ThisComponentType = TestComponent_OneField<FieldType>;

        static void Reflect(ReflectContext* context)
        {
            if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<TestComponent_OneField, AZ::Component>()
                    ->Field("Field", &TestComponent_OneField::m_field)
                    ->Version(1);
            }
            if (NetworkContext* netContext = azrtti_cast<NetworkContext*>(context))
            {
                netContext->Class<TestComponent_OneField>()
                    ->Field("Field", &TestComponent_OneField::m_field);
            }
        }

        void Activate() override {}
        void Deactivate() override {}

        Field<FieldType> m_field;
    };

    TYPED_TEST_CASE_P(NetContextFixture);

    TYPED_TEST_P(NetContextFixture, SerializationTests)
    {
        this->RunTest();
    }

    REGISTER_TYPED_TEST_CASE_P(NetContextFixture, SerializationTests);

    /*
     * Testing the basic common types.
     */
    using CommonTypes = ::testing::Types<
        TestComponent_OneField<bool>,
        TestComponent_OneField<float>,
        TestComponent_OneField<AZ::u32>,
        TestComponent_OneField<AZ::EntityId>,
        TestComponent_OneField<AZ::Vector2>,
        TestComponent_OneField<AZ::Vector3>,
        TestComponent_OneField<AZ::Quaternion>
    >;

    INSTANTIATE_TYPED_TEST_CASE_P(NetContextCommonSerialization, NetContextFixture, CommonTypes);

    /*
     * And some less common types.
     */
    using LessCommonTypes = ::testing::Types<
        TestComponent_OneField<AZStd::string>,
        TestComponent_OneField<AZ::Transform>,
        TestComponent_OneField<AZ::Color>,
        TestComponent_OneField<AZStd::vector<int>>,
        TestComponent_OneField<AZ::Uuid>
    >;

    INSTANTIATE_TYPED_TEST_CASE_P(NetContextLessCommonSerialization, NetContextFixture, LessCommonTypes);

    /*
     * Next up are marshal and unmarshal tests.
     */

    template <typename ComponentType>
    class NetContextMarshalFixture
        : public UnitTest::AllocatorsTestFixture
    {
    public:
        NetContextMarshalFixture() = default;
        ~NetContextMarshalFixture() = default;

        void SetUp() override
        {
            UnitTest::AllocatorsTestFixture::SetUp();

            AZ::AllocatorInstance<GridMate::GridMateAllocator>::Create();
            AZ::AllocatorInstance<GridMate::GridMateAllocatorMP>::Create();

            m_app = AZStd::make_unique<AzFramework::Application>();
            m_app->Start(AzFramework::Application::Descriptor());

            // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
            // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash 
            // in the unit tests.
            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);

            AzFramework::NetworkContext* netContext = nullptr;
            EBUS_EVENT_RESULT(netContext, NetSystemRequestBus, GetNetworkContext);
            AZ_TEST_ASSERT(netContext);

            m_descTestComponentAutoChunk = ComponentType::CreateDescriptor();
            m_app->RegisterComponentDescriptor(m_descTestComponentAutoChunk);

            m_entityFrom = AZStd::make_unique<AZ::Entity>("TestEntityFrom");
            m_entityFrom->Init();
            m_componentFrom = m_entityFrom->CreateComponent<ComponentType>();
            m_entityFrom->Activate();

            m_entityTo = AZStd::make_unique<AZ::Entity>("TestEntityTo");
            m_entityTo->Init();
            m_componentTo = m_entityTo->CreateComponent<ComponentType>();
            m_entityTo->Activate();
        }

        void MarshalUnMarshal()
        {
            AzFramework::NetworkContext* netContext = nullptr;
            NetSystemRequestBus::BroadcastResult(netContext, &NetSystemRequestBus::Events::GetNetworkContext);
            AZ_TEST_ASSERT(netContext);

            ComponentType* testComponent = m_entityFrom->FindComponent<ComponentType>();
            AZ_TEST_ASSERT(testComponent);
            ReplicaChunkPtr chunk = testComponent->GetNetworkBinding();
            AZ_TEST_ASSERT(chunk);

            m_outReplica = AZStd::make_unique<GridMate::Replica>("ReplicaTo");
            {
                m_outManager = AZStd::make_unique<GridMate::ReplicaManager>();
                m_outPeer = AZStd::make_unique<GridMate::ReplicaPeer>(m_outManager.get());

                GridMate::WriteBufferDynamic wb(GridMate::EndianType::IgnoreEndian);
                {
                    GridMate::TimeContext tc;
                    const GridMate::ReplicaContext rc(nullptr, tc);
                    GridMate::MarshalContext mc(GridMate::ReplicaMarshalFlags::FullSync, &wb, nullptr, rc);
                    mc.m_peer = m_outPeer.get();
                    mc.m_rm = m_outManager.get();
                    chunk->Debug_PrepareData(wb.GetEndianType(), GridMate::ReplicaMarshalFlags::FullSync);
                    chunk->Debug_Marshal(mc, 0);
                }

                // and now unmarshal into the other entity
                {
                    GridMate::TimeContext tc;
                    const GridMate::ReplicaContext rc(nullptr, tc);
                    GridMate::ReadBuffer rb(wb.GetEndianType(), wb.Get(), wb.Size());
                    GridMate::UnmarshalContext ctx(rc);
                    ctx.m_hasCtorData = false;
                    ctx.m_iBuf = &rb;
                    ctx.m_peer = m_outPeer.get();
                    ctx.m_rm = m_outManager.get();
                    m_outReplicaChunk = chunk->GetDescriptor()->CreateFromStream(ctx);

                    m_outReplicaChunk->Debug_AttachedToReplica(m_outReplica.get());
                    ctx.m_peer->Debug_Add(m_outReplica.get());
                    m_outReplicaChunk->Debug_Unmarshal(ctx, 0);
                    /*
                     * Note the order: unmarshal first to populate the chunk with data, then apply it to a component.
                     * The expectation is that the valid will apply to NetBindable::Field without being overwritten.
                     */
                    m_componentTo->SetNetworkBinding(m_outReplicaChunk);

                    // the main test body can now test for the equality
                }
            }
        }

        void TearDown() override
        {
            m_outReplicaChunk.reset();
            m_outManager.reset();
            m_outPeer.reset();
            m_outReplica.release(); // Replica is held by as an intrusive pointer in @m_outPeer and is destroyed there.

            if (m_entityFrom)
            {
                m_entityFrom->Deactivate();
                m_entityFrom.reset();
            }

            if (m_entityTo)
            {
                m_entityTo->Deactivate();
                m_entityTo.reset();
            }

            m_descTestComponentAutoChunk->ReleaseDescriptor();

            m_app->Stop();
            m_app.reset();

            AZ::AllocatorInstance<GridMate::GridMateAllocatorMP>::Destroy();
            AZ::AllocatorInstance<GridMate::GridMateAllocator>::Destroy();

            UnitTest::AllocatorsTestFixture::TearDown();
        }

        AZStd::unique_ptr<AzFramework::Application> m_app;
        AZStd::unique_ptr<AZ::Entity> m_entityFrom;
        AZStd::unique_ptr<AZ::Entity> m_entityTo;

        ComponentType* m_componentFrom = nullptr;
        ComponentType* m_componentTo = nullptr;

        AZ::ComponentDescriptor* m_descTestComponentAutoChunk = nullptr;

        GridMate::ReplicaChunkPtr m_outReplicaChunk;
        AZStd::unique_ptr<GridMate::Replica> m_outReplica;
        AZStd::unique_ptr<GridMate::ReplicaManager> m_outManager;
        AZStd::unique_ptr<GridMate::ReplicaPeer> m_outPeer;
    };

    using NetContextVector3 = NetContextMarshalFixture<TestComponent_OneField<AZ::Vector3>>;
    TEST_F(NetContextVector3, SerializationTests)
    {
        const Vector3 value = AZ::Vector3::CreateAxisZ( 1.f );

        m_componentFrom->m_field = value;
        MarshalUnMarshal();

        AZ_TEST_ASSERT(m_componentTo->m_field.Get() == value);
    }

    /*
     * Now the same test but with NetBindable::BoundField<>
     */

    template<typename FieldType>
    class TestComponent_OneBoundField
        : public AZ::Component
        , public NetBindable
    {
    public:
        AZ_COMPONENT(TestComponent_OneBoundField, "{2B283821-41DF-46BB-BE8E-66EF7301B62A}", AZ::Component, NetBindable);

        using ThisComponentType = TestComponent_OneBoundField<FieldType>;

        static void Reflect(ReflectContext* context)
        {
            if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ThisComponentType, AZ::Component>()
                    ->Field("Field", &ThisComponentType::m_boundField)
                    ->Version(1);
            }
            if (NetworkContext* netContext = azrtti_cast<NetworkContext*>(context))
            {
                netContext->Class<ThisComponentType>()
                    ->Field("Field", &ThisComponentType::m_boundField);
            }
        }

        void Activate() override {}
        void Deactivate() override {}

        void OnBoundFieldChanged( const FieldType&, const GridMate::TimeContext& ) {}

        BoundField<FieldType, ThisComponentType, &ThisComponentType::OnBoundFieldChanged> m_boundField;
    };

    using NetContextBoundVector2 = NetContextMarshalFixture<TestComponent_OneBoundField<AZ::Vector2>>;
    TEST_F(NetContextBoundVector2, SerializationTests)
    {
        const Vector2 value = AZ::Vector2::CreateAxisX( 4.f );

        m_componentFrom->m_boundField = value;
        MarshalUnMarshal();

        AZ_TEST_ASSERT(m_componentTo->m_boundField.Get() == value);
    }

    TEST_F(NetContextBoundVector2, Delete_Authoritative_Entity)
    {
        using ThisComponentType = TestComponent_OneBoundField<AZ::Vector2>;

        AzFramework::NetworkContext* netContext = nullptr;
        NetSystemRequestBus::BroadcastResult(netContext, &NetSystemRequestBus::Events::GetNetworkContext);
        AZ_TEST_ASSERT(netContext);
        ThisComponentType* testComponent = m_entityFrom->FindComponent<ThisComponentType>();
        AZ_TEST_ASSERT(testComponent);
        ReplicaChunkPtr chunk = testComponent->GetNetworkBinding();

        // Testing early deletion of an entity on the server.
        m_entityFrom->Deactivate();
        m_entityFrom.reset();

        // This test passes if it doesn't crash on cleanup.
        chunk.reset();
    }

    template<typename FieldType>
    class TestComponent_OneBoundField_ServerCallback
        : public AZ::Component
        , public NetBindable
    {
    public:
        AZ_COMPONENT(TestComponent_OneBoundField_ServerCallback, "{74F5B232-0544-45CA-B207-9846052ED1AD}", AZ::Component, NetBindable);

        using ThisComponentType = TestComponent_OneBoundField_ServerCallback<FieldType>;

        static void Reflect(ReflectContext* context)
        {
            if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ThisComponentType, AZ::Component>()
                    ->Field("Field", &ThisComponentType::m_boundField)
                    ->Version(1);
            }
            if (NetworkContext* netContext = azrtti_cast<NetworkContext*>(context))
            {
                netContext->Class<ThisComponentType>()
                    ->Field("Field", &ThisComponentType::m_boundField);
            }
        }

        void Activate() override {}
        void Deactivate() override {}

        void OnBoundFieldChanged( const FieldType&, const GridMate::TimeContext& )
        {
            ++m_callbacksInvokeCount;
        }

        AZ::u8 m_callbacksInvokeCount = 0;

        BoundField<FieldType, ThisComponentType, &ThisComponentType::OnBoundFieldChanged> m_boundField;
    };

    using NetContextBoundVector2WithCallbackCount = NetContextMarshalFixture<TestComponent_OneBoundField_ServerCallback<AZ::Vector2>>;
    TEST_F(NetContextBoundVector2WithCallbackCount, BoundField_Invoke_OnServer_Test)
    {
        MarshalUnMarshal();

        m_componentFrom->m_callbacksInvokeCount = 0; // resetting the count

        const Vector2 value = AZ::Vector2::CreateAxisX( 4.f );
        m_componentFrom->m_boundField = value;

        AZ_TEST_ASSERT(m_componentFrom->m_callbacksInvokeCount ==  1);
    }
}
