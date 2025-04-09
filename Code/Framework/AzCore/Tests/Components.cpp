/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "FileIOBaseTestTypes.h"

#include <AzCore/Math/Crc.h>
#include <AzCore/Math/Sfmt.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Component/EntityUtils.h>

#include <AzCore/IO/Streamer/StreamerComponent.h>
#include <AzCore/Serialization/ObjectStream.h>

#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzCore/IO/SystemFile.h>

#include <AzCore/Memory/AllocationRecords.h>
#include <AzCore/Memory/IAllocator.h>
#include <AzCore/UnitTest/TestTypes.h>

#include <AzCore/std/parallel/containers/concurrent_unordered_set.h>
#include <AZTestShared/Utils/Utils.h>
#include <AzTest/Utils.h>

#if defined(HAVE_BENCHMARK)
#include <benchmark/benchmark.h>
#endif

using namespace AZ;
using namespace AZ::Debug;

namespace UnitTest
{
    class Components
        : public LeakDetectionFixture
    {
    public:
        Components()
            : LeakDetectionFixture()
        {
        }
    };

    TEST_F(Components, Test)
    {
        ComponentApplication app;

        //////////////////////////////////////////////////////////////////////////
        // Create application environment code driven
        ComponentApplication::Descriptor appDesc;
        appDesc.m_memoryBlocksByteSize = 10 * 1024 * 1024;
        appDesc.m_recordingMode = AllocationRecords::Mode::RECORD_FULL;
        AZ::ComponentApplication::StartupParameters startupParameters;
        startupParameters.m_loadSettingsRegistry = false;
        Entity* systemEntity = app.Create(appDesc, startupParameters);

        systemEntity->CreateComponent<StreamerComponent>();
        systemEntity->CreateComponent(AZ::Uuid("{CAE3A025-FAC9-4537-B39E-0A800A2326DF}")); // JobManager component
        systemEntity->CreateComponent(AZ::Uuid("{D5A73BCC-0098-4d1e-8FE4-C86101E374AC}")); // AssetDatabase component

        systemEntity->Init();
        systemEntity->Activate();

        app.Destroy();
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // Create application environment data driven
        systemEntity = app.Create(appDesc);
        systemEntity->Init();
        systemEntity->Activate();
        app.Destroy();

        //////////////////////////////////////////////////////////////////////////
    }

    //////////////////////////////////////////////////////////////////////////
    // Some component message bus, this is not really part of the component framework
    // but this is way components are suppose to communicate... using the EBus
    class SimpleComponentMessages
        : public AZ::EBusTraits
    {
    public:
        virtual ~SimpleComponentMessages() {}
        virtual void DoA(int a) = 0;
        virtual void DoB(int b) = 0;
    };
    typedef AZ::EBus<SimpleComponentMessages> SimpleComponentMessagesBus;
    //////////////////////////////////////////////////////////////////////////

    class SimpleComponent
        : public Component
        , public SimpleComponentMessagesBus::Handler
        , public TickBus::Handler
    {
    public:
        AZ_RTTI(SimpleComponent, "{6DFA17AF-014C-4624-B453-96E1F9807491}", Component)
        AZ_CLASS_ALLOCATOR(SimpleComponent, SystemAllocator);

        SimpleComponent()
            : m_a(0)
            , m_b(0)
            , m_isInit(false)
            , m_isActivated(false)
        {
        }

        //////////////////////////////////////////////////////////////////////////
        // Component base
        void Init() override { m_isInit = true; m_isTicked = false; }
        void Activate() override
        {
            SimpleComponentMessagesBus::Handler::BusConnect();
            // This is a very tricky (but valid example)... here we use the TickBus... thread safe
            // event queue, to queue the connection to be executed from the main thread, just before tick.
            // By using this even though TickBus is executed in single thread mode (main thread) for
            // performance reasons, you can technically issue command from multiple thread.
            // This requires advanced knowledge of the EBus and it's NOT recommended as a schema for
            // generic functionality. You should just call TickBus::Handler::BusConnect(GetEntityId()); in place
            // make sure you are doing this from the main thread.
            TickBus::QueueFunction(&TickBus::Handler::BusConnect, this);
            m_isActivated = true;
        }
        void Deactivate() override
        {
            SimpleComponentMessagesBus::Handler::BusDisconnect();
            TickBus::Handler::BusDisconnect();
            m_isActivated = false;
        }
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // SimpleComponentMessagesBus
        void DoA(int a) override { m_a = a; }
        void DoB(int b) override { m_b = b; }
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // TickBus
        void OnTick(float deltaTime, ScriptTimePoint time) override
        {
            m_isTicked = true;
            AZ_TEST_ASSERT(deltaTime >= 0);
            AZ_TEST_ASSERT(time.Get().time_since_epoch().count() != 0);
        }
        //////////////////////////////////////////////////////////////////////////

        int m_a;
        int m_b;
        bool m_isInit;
        bool m_isActivated;
        bool m_isTicked;
    };

    // Example how to implement custom desciptors
    class SimpleComponentDescriptor
        : public ComponentDescriptorHelper<SimpleComponent>
    {
    public:
        void Reflect(ReflectContext* /*reflection*/) const override
        {
        }
    };

    TEST_F(Components, SimpleTest)
    {
        SimpleComponentDescriptor descriptor;
        ComponentApplication componentApp;
        ComponentApplication::Descriptor desc;
        desc.m_useExistingAllocator = true;
        AZ::ComponentApplication::StartupParameters startupParameters;
        startupParameters.m_loadSettingsRegistry = false;
        Entity* systemEntity = componentApp.Create(desc, startupParameters);
        AZ_TEST_ASSERT(systemEntity);
        systemEntity->Init();

        Entity* entity = aznew Entity("My Entity");
        AZ_TEST_ASSERT(entity->GetState() == Entity::State::Constructed);

        // Make sure its possible to set the id of the entity before inited.
        AZ::EntityId newId = AZ::Entity::MakeId();
        entity->SetId(newId);
        AZ_TEST_ASSERT(entity->GetId() == newId);

        AZ_TEST_START_TRACE_SUPPRESSION;
        entity->SetId(SystemEntityId); // this is disallowed.
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);

        // we can always create components directly when we have the factory
        // but it is intended to be used in generic way...
        SimpleComponent* comp1 = aznew SimpleComponent;
        AZ_TEST_ASSERT(comp1 != nullptr);
        AZ_TEST_ASSERT(comp1->GetEntity() == nullptr);
        AZ_TEST_ASSERT(comp1->GetId() == InvalidComponentId);

        bool result = entity->AddComponent(comp1);
        AZ_TEST_ASSERT(result);

                                                              // try to find it
        SimpleComponent* comp2 = entity->FindComponent<SimpleComponent>();
        AZ_TEST_ASSERT(comp1 == comp2);

        // init entity
        entity->Init();
        AZ_TEST_ASSERT(entity->GetState() == Entity::State::Init);
        AZ_TEST_ASSERT(comp1->m_isInit);
        AZ_TEST_ASSERT(comp1->GetEntity() == entity);
        AZ_TEST_ASSERT(comp1->GetId() != InvalidComponentId); // id is set only for attached components

        // Make sure its NOT possible to set the id of the entity after INIT
        newId = AZ::Entity::MakeId();
        AZ::EntityId oldID = entity->GetId();
        AZ_TEST_START_TRACE_SUPPRESSION;
        entity->SetId(newId); // this should not work because its init.
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        AZ_TEST_ASSERT(entity->GetId() == oldID); // id should be unaffected.

                                                  // try to send a component message, since it's not active nobody should listen to it
        SimpleComponentMessagesBus::Broadcast(&SimpleComponentMessagesBus::Events::DoA, 1);
        AZ_TEST_ASSERT(comp1->m_a == 0); // it should still be 0

                                         // activate
        entity->Activate();
        AZ_TEST_ASSERT(entity->GetState() == Entity::State::Active);
        AZ_TEST_ASSERT(comp1->m_isActivated);

        // now the component should be active responsive to message
        SimpleComponentMessagesBus::Broadcast(&SimpleComponentMessagesBus::Events::DoA, 1);
        AZ_TEST_ASSERT(comp1->m_a == 1);

        // Make sure its NOT possible to set the id of the entity after Activate
        newId = AZ::Entity::MakeId();
        AZ_TEST_START_TRACE_SUPPRESSION;
        entity->SetId(newId); // this should not work because its init.
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);

        // test the tick events
        componentApp.Tick(); // first tick will set-up timers and have 0 delta time
        AZ_TEST_ASSERT(comp1->m_isTicked);
        componentApp.Tick(); // this will dispatch actual valid delta time

                             // make sure we can't remove components while active
        AZ_TEST_START_TRACE_SUPPRESSION;
        AZ_TEST_ASSERT(entity->RemoveComponent(comp1) == false);
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);

            // make sure we can't add components while active
            {
                SimpleComponent anotherComp;
                AZ_TEST_START_TRACE_SUPPRESSION;
                AZ_TEST_ASSERT(entity->AddComponent(&anotherComp) == false);
                AZ_TEST_STOP_TRACE_SUPPRESSION(1);
            }

            AZ_TEST_START_TRACE_SUPPRESSION;
            AZ_TEST_ASSERT(entity->CreateComponent<SimpleComponent>() == nullptr);
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);

            AZ_TEST_START_TRACE_SUPPRESSION;
            AZ_TEST_ASSERT(entity->CreateComponent(azrtti_typeid<SimpleComponent>()) == nullptr);
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);

        // deactivate
        entity->Deactivate();
        AZ_TEST_ASSERT(entity->GetState() == Entity::State::Init);
        AZ_TEST_ASSERT(comp1->m_isActivated == false);

        // try to send a component message, since it's not active nobody should listen to it
        SimpleComponentMessagesBus::Broadcast(&SimpleComponentMessagesBus::Events::DoA, 2);
        AZ_TEST_ASSERT(comp1->m_a == 1);

        // make sure we can remove components
        AZ_TEST_ASSERT(entity->RemoveComponent(comp1));
        AZ_TEST_ASSERT(comp1->GetEntity() == nullptr);
        AZ_TEST_ASSERT(comp1->GetId() == InvalidComponentId);

        delete comp1;
        delete entity;
        descriptor.BusDisconnect(); // disconnect from the descriptor bus (so the app doesn't try to clean us up)
    }

    //////////////////////////////////////////////////////////////////////////
    // Component A
    class ComponentA
        : public Component
    {
    public:
        AZ_CLASS_ALLOCATOR(ComponentA, SystemAllocator);
        AZ_RTTI(ComponentA, "{4E93E03A-0B71-4630-ACCA-C6BB78E6DEB9}", Component)

        void Activate() override {}
        void Deactivate() override {}
    };

    /// Custom descriptor... example
    class ComponentADescriptor
        : public ComponentDescriptorHelper<ComponentA>
    {
    public:
        AZ_CLASS_ALLOCATOR(ComponentADescriptor, SystemAllocator);

        ComponentADescriptor()
            : m_isDependent(false)
        {
        }

        void GetProvidedServices(DependencyArrayType& provided, const Component* instance) const override
        {
            (void)instance;
            provided.push_back(AZ_CRC_CE("ServiceA"));
        }
        void GetDependentServices(DependencyArrayType& dependent, const Component* instance) const override
        {
            (void)instance;
            if (m_isDependent)
            {
                dependent.push_back(AZ_CRC_CE("ServiceD"));
            }
        }
        void Reflect(ReflectContext* /*reflection*/) const override {}

        bool    m_isDependent;
    };
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Component B
    class ComponentB
        : public Component
    {
    public:
        AZ_COMPONENT(ComponentB, "{30B266B3-AFD6-4173-8BEB-39134A3167E3}")

        void Activate() override {}
        void Deactivate() override {}

        static void GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided) { provided.push_back(AZ_CRC_CE("ServiceB")); }
        static void GetDependentServices(ComponentDescriptor::DependencyArrayType& dependent) { dependent.push_back(AZ_CRC_CE("ServiceE")); }
        static void GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible) { incompatible.push_back(AZ_CRC_CE("ServiceF")); }
        static void Reflect(ReflectContext* /*reflection*/)  {}
    };

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    // Component C
    class ComponentC
        : public Component
    {
    public:
        AZ_COMPONENT(ComponentC, "{A24C5D97-641F-4A92-90BB-647213A9D054}");

        void Activate() override {}
        void Deactivate() override {}

        static void GetRequiredServices(ComponentDescriptor::DependencyArrayType& required) { required.push_back(AZ_CRC_CE("ServiceB")); }
        static void Reflect(ReflectContext* /*reflection*/) {}
    };

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    // Component D
    class ComponentD
        : public Component
    {
    public:
        AZ_COMPONENT(ComponentD, "{90888AD7-9D15-4356-8B95-C233A2E3083C}");

        void Activate() override {}
        void Deactivate() override {}

        static void GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided) { provided.push_back(AZ_CRC_CE("ServiceD")); }
        static void Reflect(ReflectContext* /*reflection*/) {}
    };
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Component E
    class ComponentE
        : public Component
    {
    public:
        AZ_COMPONENT(ComponentE, "{8D28A94A-9F70-4ADA-999E-D8A56A3048FB}", Component);

        void Activate() override {}
        void Deactivate() override {}

        static void GetDependentServices(ComponentDescriptor::DependencyArrayType& dependent) { dependent.push_back(AZ_CRC_CE("ServiceD")); dependent.push_back(AZ_CRC_CE("ServiceA")); }
        static void GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided) { provided.push_back(AZ_CRC_CE("ServiceE")); }
        static void Reflect(ReflectContext* /*reflection*/) {}
    };
    //////////////////////////////////////////////////////////////////////////


    //////////////////////////////////////////////////////////////////////////
    // Component E2 - provides ServiceE but has no dependencies
    class ComponentE2
        : public Component
    {
    public:
        AZ_COMPONENT(ComponentE2, "{33FE383C-92E0-48A4-A89A-91283DFC714A}", Component);

        void Activate() override {}
        void Deactivate() override {}

        static void GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided) { provided.push_back(AZ_CRC_CE("ServiceE")); }
        static void Reflect(ReflectContext* /*reflection*/) {}
    };
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Component F
    class ComponentF
        : public Component
    {
    public:
        AZ_COMPONENT(ComponentF, "{9A04F820-DFB6-42CF-9D1B-F970CEF1A02A}");

        void Activate() override {}
        void Deactivate() override {}

        static void GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible) { incompatible.push_back(AZ_CRC_CE("ServiceA")); }
        static void GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided) { provided.push_back(AZ_CRC_CE("ServiceF")); }
        static void Reflect(ReflectContext* /*reflection*/) {}
    };
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Component G - has cyclic dependency with H
    class ComponentG
        : public Component
    {
    public:
        AZ_COMPONENT(ComponentG, "{1CF8894A-CFE4-42FE-8127-63416DF734E1}");

        void Activate() override {}
        void Deactivate() override {}

        static void GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided) { provided.push_back(AZ_CRC_CE("ServiceG")); }
        static void GetRequiredServices(ComponentDescriptor::DependencyArrayType& required) { required.push_back(AZ_CRC_CE("ServiceH")); }
        static void Reflect(ReflectContext* /*reflection*/) {}
    };
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Component H - has cyclic dependency with G
    class ComponentH
        : public Component
    {
    public:
        AZ_COMPONENT(ComponentH, "{2FCF9245-B579-45D1-950B-A6779CA16F66}");

        void Activate() override {}
        void Deactivate() override {}

        static void GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided) { provided.push_back(AZ_CRC_CE("ServiceH")); }
        static void GetRequiredServices(ComponentDescriptor::DependencyArrayType& required) { required.push_back(AZ_CRC_CE("ServiceG")); }
        static void Reflect(ReflectContext* /*reflection*/) {}
    };
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Component I - incompatible with other components providing the same service
    class ComponentI
        : public Component
    {
    public:
        AZ_COMPONENT(ComponentI, "{5B509DB8-5D8A-4141-8701-4244E2F99025}");

        void Activate() override {}
        void Deactivate() override {}

        static void GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided) { provided.push_back(AZ_CRC_CE("ServiceI")); }
        static void GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& provided) { provided.push_back(AZ_CRC_CE("ServiceI")); }
        static void Reflect(ReflectContext* /*reflection*/) {}
    };
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Component J - "accidentally" provides same service twice
    class ComponentJ
        : public Component
    {
    public:
        AZ_COMPONENT(ComponentJ, "{67D56E5D-AB39-4BC3-AB1B-5B1F622E2A7F}");

        void Activate() override {}
        void Deactivate() override {}

        static void GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided) { provided.push_back(AZ_CRC_CE("ServiceJ")); provided.push_back(AZ_CRC_CE("ServiceJ")); }
        static void Reflect(ReflectContext* /*reflection*/) {}
    };
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Component K - depends on component that declared its provided service twice
    class ComponentK
        : public Component
    {
    public:
        AZ_COMPONENT(ComponentK, "{9FEB506A-03BD-485B-A5D5-133B34E290F5}");

        void Activate() override {}
        void Deactivate() override {}

        static void GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided) { provided.push_back(AZ_CRC_CE("ServiceK")); }
        static void GetDependentServices(ComponentDescriptor::DependencyArrayType& dependent) { dependent.push_back(AZ_CRC_CE("ServiceJ")); }
        static void Reflect(ReflectContext* /*reflection*/) {}
    };
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Component L - "accidentally" depends on same service twice
    class ComponentL
        : public Component
    {
    public:
        AZ_COMPONENT(ComponentL, "{17A80803-C0F1-4595-A29D-AAD81D69B82E}");

        void Activate() override {}
        void Deactivate() override {}

        static void GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided) { provided.push_back(AZ_CRC_CE("ServiceL")); }
        static void GetDependentServices(ComponentDescriptor::DependencyArrayType& dependent) { dependent.push_back(AZ_CRC_CE("ServiceA")); dependent.push_back(AZ_CRC_CE("ServiceA")); }
        static void Reflect(ReflectContext* /*reflection*/) {}
    };
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Component M - "accidentally" depends on and requires the same service
    class ComponentM
        : public Component
    {
    public:
        AZ_COMPONENT(ComponentM, "{74A118BC-2049-4C90-82B1-094934BD86F7}");

        void Activate() override {}
        void Deactivate() override {}

        static void GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided) { provided.push_back(AZ_CRC_CE("ServiceM")); }
        static void GetDependentServices(ComponentDescriptor::DependencyArrayType& dependent) { dependent.push_back(AZ_CRC_CE("ServiceA")); }
        static void GetRequiredServices(ComponentDescriptor::DependencyArrayType& dependent) { dependent.push_back(AZ_CRC_CE("ServiceA")); }
        static void Reflect(ReflectContext* /*reflection*/) {}
    };
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Component N - "accidentally" lists an incompatibility twice
    class ComponentN
        : public Component
    {
    public:
        AZ_COMPONENT(ComponentN, "{B1026620-ED77-4897-B3EF-D03D4DDAF84B}");

        void Activate() override {}
        void Deactivate() override {}

        static void GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided) { provided.push_back(AZ_CRC_CE("ServiceN")); }
        static void GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& provided) { provided.push_back(AZ_CRC_CE("ServiceA")); provided.push_back(AZ_CRC_CE("ServiceA")); }
        static void Reflect(ReflectContext* /*reflection*/) {}
    };
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Component O - "accidentally" lists its own service twice in incompatibility list
    class ComponentO
        : public Component
    {
    public:
        AZ_COMPONENT(ComponentO, "{14916FA3-8A74-4974-AED9-43CB222C6883}");

        void Activate() override {}
        void Deactivate() override {}

        static void GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided) { provided.push_back(AZ_CRC_CE("ServiceO")); }
        static void GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& provided) { provided.push_back(AZ_CRC_CE("ServiceO")); provided.push_back(AZ_CRC_CE("ServiceO")); }
        static void Reflect(ReflectContext* /*reflection*/) {}
    };
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Component P - no services at all
    class ComponentP
        : public Component
    {
    public:
        AZ_COMPONENT(ComponentP, "{0D71F310-FEBC-418D-9C4B-847C89DF6606}");

        void Activate() override {}
        void Deactivate() override {}

        static void Reflect(ReflectContext* /*reflection*/) {}
    };
    //////////////////////////////////////////////////////////////////////////

    class ComponentDependency
        : public Components
    {
    protected:
        void SetUp() override
        {
            LeakDetectionFixture::SetUp();

            // component descriptors are cleaned up when application shuts down
            m_descriptorComponentA = aznew ComponentADescriptor;
            aznew ComponentB::DescriptorType;
            aznew ComponentC::DescriptorType;
            aznew ComponentD::DescriptorType;
            aznew ComponentE::DescriptorType;
            aznew ComponentE2::DescriptorType;
            aznew ComponentF::DescriptorType;
            aznew ComponentG::DescriptorType;
            aznew ComponentH::DescriptorType;
            aznew ComponentI::DescriptorType;
            aznew ComponentJ::DescriptorType;
            aznew ComponentK::DescriptorType;
            aznew ComponentL::DescriptorType;
            aznew ComponentM::DescriptorType;
            aznew ComponentN::DescriptorType;
            aznew ComponentO::DescriptorType;
            aznew ComponentP::DescriptorType;

            m_componentApp = aznew ComponentApplication();

            ComponentApplication::Descriptor desc;
            desc.m_useExistingAllocator = true;

            Entity* systemEntity = m_componentApp->Create(desc, {});
            systemEntity->Init();

            m_entity = aznew Entity();
        }

        void TearDown() override
        {
            delete m_entity;
            delete m_componentApp;

            LeakDetectionFixture::TearDown();
        }

        void CreateComponents_ABCDE()
        {
            m_entity->CreateComponent<ComponentA>();
            m_entity->CreateComponent<ComponentB>();
            m_entity->CreateComponent<ComponentC>();
            m_entity->CreateComponent<ComponentD>();
            m_entity->CreateComponent<ComponentE>();
        }

        ComponentADescriptor* m_descriptorComponentA;
        ComponentApplication* m_componentApp;

        Entity *m_entity; // an entity to mess with in each test
    };

    TEST_F(ComponentDependency, FixtureSanityCheck)
    {
        // Tests that Setup/TearDown work as expected
    }

    TEST_F(ComponentDependency, IsComponentReadyToAdd_ExaminesRequiredServices)
    {
        ComponentC* componentC = aznew ComponentC;

        ComponentDescriptor::DependencyArrayType requiredServices;
        EXPECT_FALSE(m_entity->IsComponentReadyToAdd(componentC, &requiredServices)); // we require B component to be added
        ASSERT_EQ(1, requiredServices.size());

        Crc32 requiredService = requiredServices[0];
        EXPECT_EQ(Crc32("ServiceB"), requiredService);

        m_entity->CreateComponent<ComponentB>();
        EXPECT_TRUE(m_entity->IsComponentReadyToAdd(componentC)); // we require B component to be added

        delete componentC;
    }

    TEST_F(ComponentDependency, IsComponentReadyToAdd_ExaminesIncompatibleServices)
    {
        ComponentA* componentA = m_entity->CreateComponent<ComponentA>();
        ComponentB* componentB = m_entity->CreateComponent<ComponentB>(); // B incompatible with F

        ComponentF* componentF = aznew ComponentF(); // F incompatible with A

        Entity::ComponentArrayType incompatible;
        EXPECT_FALSE(m_entity->IsComponentReadyToAdd(componentF, nullptr, &incompatible));
        EXPECT_EQ(2, incompatible.size());

        bool incompatibleWithComponentA = AZStd::find(incompatible.begin(), incompatible.end(), componentA) != incompatible.end();
        bool incompatibleWithComponentB = AZStd::find(incompatible.begin(), incompatible.end(), componentB) != incompatible.end();
        EXPECT_TRUE(incompatibleWithComponentA);
        EXPECT_TRUE(incompatibleWithComponentB);

        delete componentF;
    }

    TEST_F(ComponentDependency, Init_DoesNotChangeComponentOrder)
    {
        Entity::ComponentArrayType originalOrder = m_entity->GetComponents();

        m_entity->Init(); // Init should not change the component order

        EXPECT_EQ(originalOrder, m_entity->GetComponents());
    }

    TEST_F(ComponentDependency, Activate_SortsComponentsCorrectly)
    {
        CreateComponents_ABCDE();
        m_entity->Init();
        m_entity->Activate(); // here components will be sorted based on order

        const Entity::ComponentArrayType& components = m_entity->GetComponents();
        EXPECT_TRUE(components[0]->RTTI_IsTypeOf(AzTypeInfo<ComponentA>::Uuid()));
        EXPECT_TRUE(components[1]->RTTI_IsTypeOf(AzTypeInfo<ComponentD>::Uuid()));
        EXPECT_TRUE(components[2]->RTTI_IsTypeOf(AzTypeInfo<ComponentE>::Uuid()));
        EXPECT_TRUE(components[3]->RTTI_IsTypeOf(AzTypeInfo<ComponentB>::Uuid()));
        EXPECT_TRUE(components[4]->RTTI_IsTypeOf(AzTypeInfo<ComponentC>::Uuid()));
    }

    TEST_F(ComponentDependency, Deactivate_DoesNotChangeComponentOrder)
    {
        CreateComponents_ABCDE();
        m_entity->Init();
        m_entity->Activate();

        Entity::ComponentArrayType orderAfterActivate = m_entity->GetComponents();

        m_entity->Deactivate();

        EXPECT_EQ(orderAfterActivate, m_entity->GetComponents());
    }

    TEST_F(ComponentDependency, CachedDependency_PreventsComponentSort)
    {
        CreateComponents_ABCDE();
        m_entity->Init();
        m_entity->Activate();
        m_entity->Deactivate();

        Entity::ComponentArrayType originalSortedOrder = m_entity->GetComponents();

        m_descriptorComponentA->m_isDependent = true; // now A should depend on D (but only after we notify the entity of the change)

        m_entity->Activate();

        // order should be unchanged (because we cache the dependency)
        EXPECT_EQ(originalSortedOrder, m_entity->GetComponents());
    }

    TEST_F(ComponentDependency, InvalidatingDependency_CausesComponentSort)
    {
        CreateComponents_ABCDE();
        m_entity->Init();
        m_entity->Activate();
        m_entity->Deactivate();

        m_descriptorComponentA->m_isDependent = true; // now A should depend on D
        m_entity->InvalidateDependencies();

        m_entity->Activate();

        // check the new order
        const Entity::ComponentArrayType& components = m_entity->GetComponents();
        EXPECT_TRUE(components[0]->RTTI_IsTypeOf(AzTypeInfo<ComponentD>::Uuid()));
        EXPECT_TRUE(components[1]->RTTI_IsTypeOf(AzTypeInfo<ComponentA>::Uuid()));
        EXPECT_TRUE(components[2]->RTTI_IsTypeOf(AzTypeInfo<ComponentE>::Uuid()));
        EXPECT_TRUE(components[3]->RTTI_IsTypeOf(AzTypeInfo<ComponentB>::Uuid()));
        EXPECT_TRUE(components[4]->RTTI_IsTypeOf(AzTypeInfo<ComponentC>::Uuid()));
    }

    TEST_F(ComponentDependency, IsComponentReadyToRemove_ExaminesRequiredServices)
    {
        ComponentB* componentB = m_entity->CreateComponent<ComponentB>();
        ComponentC* componentC = m_entity->CreateComponent<ComponentC>();

        Entity::ComponentArrayType requiredComponents;
        EXPECT_FALSE(m_entity->IsComponentReadyToRemove(componentB, &requiredComponents)); // component C requires us
        ASSERT_EQ(1, requiredComponents.size());

        Component* requiredComponent = requiredComponents[0];
        EXPECT_EQ(componentC, requiredComponent);

        m_entity->RemoveComponent(componentC);
        delete componentC;

        EXPECT_TRUE(m_entity->IsComponentReadyToRemove(componentB)); // we should be ready for remove
    }

    // there was once a bug where, if multiple different component types provided the same service,
    // those components didn't necessarily sort before components that depended on that service
    TEST_F(ComponentDependency, DependingOnSameServiceFromTwoDifferentComponents_PutsServiceProvidersFirst)
    {
        m_entity->CreateComponent<ComponentD>(); // no dependencies
        Component* e2 = m_entity->CreateComponent<ComponentE2>(); // no dependencies
        Component* e = m_entity->CreateComponent<ComponentE>(); // depends on ServiceD
        Component* b = m_entity->CreateComponent<ComponentB>(); // depends on ServiceE (provided by E and E2)

        EXPECT_EQ(Entity::DependencySortResult::Success, m_entity->EvaluateDependencies());

        const AZ::Entity::ComponentArrayType& components = m_entity->GetComponents();
        auto locationB = AZStd::find(components.begin(), components.end(), b);
        auto locationE = AZStd::find(components.begin(), components.end(), e);
        auto locationE2 = AZStd::find(components.begin(), components.end(), e2);

        EXPECT_LT(locationE, locationB);
        EXPECT_LT(locationE2, locationB);
    }

    TEST_F(ComponentDependency, ComponentsThatProvideNoServices_SortedLast)
    {
        // components providing no services
        Component* c = m_entity->CreateComponent<ComponentC>(); // requires ServiceB
        Component* p = m_entity->CreateComponent<ComponentP>();

        // components providing a service
        Component* b = m_entity->CreateComponent<ComponentB>();
        Component* d = m_entity->CreateComponent<ComponentD>();
        Component* i = m_entity->CreateComponent<ComponentI>();
        Component* k = m_entity->CreateComponent<ComponentK>();

        // the only dependency between these components is that C requires B

        EXPECT_EQ(Entity::DependencySortResult::DSR_OK, m_entity->EvaluateDependencies());

        const AZ::Entity::ComponentArrayType& components = m_entity->GetComponents();
        const ptrdiff_t numComponents = m_entity->GetComponents().size();

        ptrdiff_t maxIndexOfComponentProvidingServices = PTRDIFF_MIN;
        for (Component* component : { b, d, i, k })
        {
            ptrdiff_t index = AZStd::distance(components.begin(), AZStd::find(components.begin(), components.end(), component));
            EXPECT_TRUE(index >= 0 && index < numComponents);
            maxIndexOfComponentProvidingServices = AZStd::max(maxIndexOfComponentProvidingServices, index);
        }

        ptrdiff_t minIndexOfComponentProvidingNoServices = PTRDIFF_MAX;
        for (Component* component : { c, p })
        {
            ptrdiff_t index = AZStd::distance(components.begin(), AZStd::find(components.begin(), components.end(), component));
            EXPECT_TRUE(index >= 0 && index < numComponents);
            minIndexOfComponentProvidingNoServices = AZStd::min(minIndexOfComponentProvidingNoServices, index);
        }

        EXPECT_LT(maxIndexOfComponentProvidingServices, minIndexOfComponentProvidingNoServices);
    }

    // there was once a bug where we didn't check requirements if there was only 1 component
    TEST_F(ComponentDependency, OneComponentRequiringService_FailsDueToMissingRequirements)
    {
        m_entity->CreateComponent<ComponentG>(); // requires ServiceH

        EXPECT_EQ(Entity::DependencySortResult::MissingRequiredService, m_entity->EvaluateDependencies());
    }

    // there was once a bug where we didn't check requirements of components that provided no services
    TEST_F(ComponentDependency, RequiringServiceWithoutProvidingService_FailsDueToMissingRequirements)
    {
        m_entity->CreateComponent<ComponentC>(); // requires ServiceB
        m_entity->CreateComponent<ComponentC>(); // requires ServiceB

        EXPECT_EQ(Entity::DependencySortResult::MissingRequiredService, m_entity->EvaluateDependencies());

        // there was also once a bug where failed sorts would result in components vanishing
        EXPECT_EQ(2, m_entity->GetComponents().size());
    }

    TEST_F(ComponentDependency, ComponentIncompatibleWithServiceItProvides_IsOkByItself)
    {
        m_entity->CreateComponent<ComponentI>(); // incompatible with ServiceI

        EXPECT_EQ(Entity::DependencySortResult::Success, m_entity->EvaluateDependencies());
    }

    TEST_F(ComponentDependency, TwoInstancesOfComponentIncompatibleWithServiceItProvides_AreIncompatible)
    {
        m_entity->CreateComponent<ComponentI>(); // incompatible with ServiceI
        m_entity->CreateComponent<ComponentI>(); // incompatible with ServiceI

        EXPECT_EQ(Entity::DependencySortResult::HasIncompatibleServices, m_entity->EvaluateDependencies());
    }

    // there was once a bug where failures due to cyclic dependencies would result in components vanishing
    TEST_F(ComponentDependency, FailureDueToCyclicDependencies_LeavesComponentsInPlace)
    {
        m_entity->CreateComponent<ComponentG>(); // requires ServiceH
        m_entity->CreateComponent<ComponentH>(); // requires ServiceG

        EXPECT_EQ(Entity::DependencySortResult::HasCyclicDependency, m_entity->EvaluateDependencies());

        // there was also once a bug where failed sorts would result in components vanishing
        EXPECT_EQ(2, m_entity->GetComponents().size());
    }

    TEST_F(ComponentDependency, ComponentWithoutDescriptor_FailsDueToUnregisteredDescriptor)
    {
        CreateComponents_ABCDE();

        // delete ComponentB's descriptor
        ComponentDescriptorBus::Event(azrtti_typeid<ComponentB>(), &ComponentDescriptorBus::Events::ReleaseDescriptor);

        EXPECT_EQ(Entity::DependencySortResult::DescriptorNotRegistered, m_entity->EvaluateDependencies());
    }

    TEST_F(ComponentDependency, StableSort_GetsSameResultsEveryTime)
    {
        // put a bunch of components on the entity
        CreateComponents_ABCDE();
        CreateComponents_ABCDE();
        CreateComponents_ABCDE();

        // throw in components whose dependencies could make the sort order ambiguous
        m_entity->CreateComponent<ComponentI>(); // I is incompatible with itself, but depends on nothing
        m_entity->CreateComponent<ComponentP>(); // P has no service declarations whatsoever
        m_entity->CreateComponent<ComponentP>();
        m_entity->CreateComponent<ComponentP>();
        m_entity->CreateComponent<ComponentK>(); // K depends on J (but J not present)
        m_entity->CreateComponent<ComponentK>();
        m_entity->CreateComponent<ComponentK>();

        // set Component IDs (using seeded random) so we get same results each time this test runs
        u32 randSeed[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
        Sfmt randGen(randSeed, AZ_ARRAY_SIZE(randSeed));
        AZStd::unordered_map<Component*, ComponentId> componentIds;

        for (Component* component : m_entity->GetComponents())
        {
            ComponentId id = randGen.Rand64();
            componentIds[component] = id;
            component->SetId(id);
        }

        // perform initial sort
        EXPECT_EQ(Entity::DependencySortResult::Success, m_entity->EvaluateDependencies());

        const AZ::Entity::ComponentArrayType originalSortedOrder = m_entity->GetComponents();

        // try shuffling the components a bunch of times
        // we should always get the same sorted results
        for (int iteration = 0; iteration < 50; ++iteration)
        {
            AZ::Entity::ComponentArrayType componentsToShuffle = m_entity->GetComponents();

            // remove all components from entity
            for (Component* component : componentsToShuffle)
            {
                m_entity->RemoveComponent(component);
            }

            // shuffle components
            for (int i = 0; i < 200; ++i)
            {
                size_t swapA = randGen.Rand64() % componentsToShuffle.size();
                size_t swapB = randGen.Rand64() % componentsToShuffle.size();
                AZStd::swap(componentsToShuffle[swapA], componentsToShuffle[swapB]);
            }

            // put components back on entity
            for (Component* component : componentsToShuffle)
            {
                m_entity->AddComponent(component);

                // removing components resets their ID
                // set it back to previous value so sort results are the same
                component->SetId(componentIds[component]);
            }

            EXPECT_EQ(Entity::DependencySortResult::Success, m_entity->EvaluateDependencies());
            const AZ::Entity::ComponentArrayType& sorted = m_entity->GetComponents();
            EXPECT_EQ(originalSortedOrder, sorted);

            if (HasFailure())
            {
                break;
            }
        };
    }

    // Check that invalid user input, in the form of services accidentally listed multiple times,
    // is handled appropriately and doesn't result in infinite loops.

    TEST_F(ComponentDependency, ComponentAccidentallyProvidingSameServiceTwice_IsOk)
    {
        m_entity->CreateComponent<ComponentJ>(); // provides ServiceJ twice

        EXPECT_EQ(Entity::DependencySortResult::Success, m_entity->EvaluateDependencies());
    }

    TEST_F(ComponentDependency, DependingOnComponentWhichAccidentallyProvidesSameServiceTwice_IsOk)
    {
        m_entity->CreateComponent<ComponentJ>(); // provides ServiceJ twice
        m_entity->CreateComponent<ComponentK>(); // depends on ServiceJ

        EXPECT_EQ(Entity::DependencySortResult::Success, m_entity->EvaluateDependencies());
        EXPECT_EQ(azrtti_typeid<ComponentJ>(), azrtti_typeid(m_entity->GetComponents()[0]));
        EXPECT_EQ(azrtti_typeid<ComponentK>(), azrtti_typeid(m_entity->GetComponents()[1]));
    }

    TEST_F(ComponentDependency, ComponentAccidentallyDependingOnSameServiceTwice_IsOk)
    {
        m_entity->CreateComponent<ComponentL>(); // depends on ServiceA twice
        m_entity->CreateComponent<ComponentA>();

        EXPECT_EQ(Entity::DependencySortResult::Success, m_entity->EvaluateDependencies());
        EXPECT_EQ(azrtti_typeid<ComponentA>(), azrtti_typeid(m_entity->GetComponents()[0]));
        EXPECT_EQ(azrtti_typeid<ComponentL>(), azrtti_typeid(m_entity->GetComponents()[1]));
    }

    TEST_F(ComponentDependency, ComponentAccidentallyDependingAndRequiringSameService_IsOk)
    {
        m_entity->CreateComponent<ComponentM>(); // depends on ServiceA and requires Service A
        m_entity->CreateComponent<ComponentA>();

        EXPECT_EQ(Entity::DependencySortResult::Success, m_entity->EvaluateDependencies());
        EXPECT_EQ(azrtti_typeid<ComponentA>(), azrtti_typeid(m_entity->GetComponents()[0]));
        EXPECT_EQ(azrtti_typeid<ComponentM>(), azrtti_typeid(m_entity->GetComponents()[1]));
    }

    TEST_F(ComponentDependency, ComponentAccidentallyListsIncompatibleServiceTwice_IsOkByItself)
    {
        m_entity->CreateComponent<ComponentN>(); // incompatible with ServiceA twice

        EXPECT_EQ(Entity::DependencySortResult::Success, m_entity->EvaluateDependencies());
    }

    TEST_F(ComponentDependency, ComponentAccidentallyListsIncompatibleServiceTwice_IncompatibilityStillDetected)
    {
        m_entity->CreateComponent<ComponentN>(); // incompatible with ServiceA twice
        m_entity->CreateComponent<ComponentA>();

        EXPECT_EQ(Entity::DependencySortResult::HasIncompatibleServices, m_entity->EvaluateDependencies());
    }

    TEST_F(ComponentDependency, ComponentAccidentallyListingIncompatibilityWithSelfTwice_IsOkByItself)
    {
        m_entity->CreateComponent<ComponentO>(); // incompatible with ServiceO twice

        EXPECT_EQ(Entity::DependencySortResult::Success, m_entity->EvaluateDependencies());
    }

    TEST_F(ComponentDependency, TwoInstancesOfComponentAccidentallyListingIncompatibilityWithSelfTwice_AreIncompatible)
    {
        m_entity->CreateComponent<ComponentO>(); // incompatible with ServiceO twice
        m_entity->CreateComponent<ComponentO>(); // incompatible with ServiceO twice

        EXPECT_EQ(Entity::DependencySortResult::HasIncompatibleServices, m_entity->EvaluateDependencies());
    }

    /**
     * UserSettingsComponent test
     */
    class UserSettingsTestApp
        : public ComponentApplication
        , public UserSettingsFileLocatorBus::Handler
    {
        AZ::Test::ScopedAutoTempDirectory m_tempDir;
    public:
        AZ_CLASS_ALLOCATOR(UserSettingsTestApp, SystemAllocator)
        AZStd::string ResolveFilePath(u32 providerId) override
        {
            auto filePath = AZ::IO::Path(m_tempDir.GetDirectory());
            if (providerId == UserSettings::CT_GLOBAL)
            {
                filePath /= "GlobalUserSettings.xml";
            }
            else if (providerId == UserSettings::CT_LOCAL)
            {
                filePath /= "LocalUserSettings.xml";
            }
            return filePath.Native();
        }

        void SetSettingsRegistrySpecializations(SettingsRegistryInterface::Specializations& specializations) override
        {
            ComponentApplication::SetSettingsRegistrySpecializations(specializations);
            specializations.Append("test");
            specializations.Append("usersettingstest");
        }
    };

    class MyUserSettings
        : public UserSettings
    {
    public:
        AZ_CLASS_ALLOCATOR(MyUserSettings, SystemAllocator);
        AZ_RTTI(MyUserSettings, "{ACC60C7B-60D8-4491-AD5D-42BA6656CC1F}", UserSettings);

        static void Reflect(AZ::SerializeContext* sc)
        {
            sc->Class<MyUserSettings>()
                ->Field("intOption1", &MyUserSettings::m_intOption1);
        }

        int m_intOption1;
    };

    using UserSettingsTestFixture = UnitTest::LeakDetectionFixture;
    TEST_F(UserSettingsTestFixture, Test)
    {
        UserSettingsTestApp app;

        //////////////////////////////////////////////////////////////////////////
        // Create application environment code driven
        ComponentApplication::Descriptor appDesc;
        appDesc.m_memoryBlocksByteSize = 10 * 1024 * 1024;
        AZ::ComponentApplication::StartupParameters startupParameters;
        startupParameters.m_loadSettingsRegistry = false;
        Entity* systemEntity = app.Create(appDesc, startupParameters);
        app.UserSettingsFileLocatorBus::Handler::BusConnect();

        MyUserSettings::Reflect(app.GetSerializeContext());

        UserSettingsComponent* globalUserSettingsComponent = systemEntity->CreateComponent<UserSettingsComponent>();
        AZ_TEST_ASSERT(globalUserSettingsComponent);
        globalUserSettingsComponent->SetProviderId(UserSettings::CT_GLOBAL);

        UserSettingsComponent* localUserSettingsComponent = systemEntity->CreateComponent<UserSettingsComponent>();
        AZ_TEST_ASSERT(localUserSettingsComponent);
        localUserSettingsComponent->SetProviderId(UserSettings::CT_LOCAL);

        systemEntity->Init();
        systemEntity->Activate();

        AZStd::intrusive_ptr<MyUserSettings> myGlobalUserSettings = UserSettings::CreateFind<MyUserSettings>(AZ_CRC_CE("MyUserSettings"), UserSettings::CT_GLOBAL);
        AZ_TEST_ASSERT(myGlobalUserSettings);
        myGlobalUserSettings->m_intOption1 = 10;
        AZStd::intrusive_ptr<MyUserSettings> storedGlobalSettings = UserSettings::CreateFind<MyUserSettings>(AZ_CRC_CE("MyUserSettings"), UserSettings::CT_GLOBAL);
        AZ_TEST_ASSERT(myGlobalUserSettings == storedGlobalSettings);
        AZ_TEST_ASSERT(storedGlobalSettings->m_intOption1 == 10);

        AZStd::intrusive_ptr<MyUserSettings> myLocalUserSettings = UserSettings::CreateFind<MyUserSettings>(AZ_CRC_CE("MyUserSettings"), UserSettings::CT_LOCAL);
        AZ_TEST_ASSERT(myLocalUserSettings);
        myLocalUserSettings->m_intOption1 = 20;
        AZStd::intrusive_ptr<MyUserSettings> storedLocalSettings = UserSettings::CreateFind<MyUserSettings>(AZ_CRC_CE("MyUserSettings"), UserSettings::CT_LOCAL);
        AZ_TEST_ASSERT(myLocalUserSettings == storedLocalSettings);
        AZ_TEST_ASSERT(storedLocalSettings->m_intOption1 == 20);

        // Deactivating will not trigger saving of user options, saving must be performed manually.
        UserSettingsComponentRequestBus::Broadcast(&UserSettingsComponentRequests::Save);
        systemEntity->Deactivate();

        // Deactivate() should have cleared all the registered user options
        storedGlobalSettings = UserSettings::Find<MyUserSettings>(AZ_CRC_CE("MyUserSettings"), UserSettings::CT_GLOBAL);
        AZ_TEST_ASSERT(!storedGlobalSettings);
        storedLocalSettings = UserSettings::Find<MyUserSettings>(AZ_CRC_CE("MyUserSettings"), UserSettings::CT_LOCAL);
        AZ_TEST_ASSERT(!storedLocalSettings);

        systemEntity->Activate();

        // Verify that upon re-activation, we successfully loaded all settings saved during deactivation
        storedGlobalSettings = UserSettings::Find<MyUserSettings>(AZ_CRC_CE("MyUserSettings"), UserSettings::CT_GLOBAL);
        AZ_TEST_ASSERT(storedGlobalSettings);
        myGlobalUserSettings = UserSettings::CreateFind<MyUserSettings>(AZ_CRC_CE("MyUserSettings"), UserSettings::CT_GLOBAL);
        AZ_TEST_ASSERT(myGlobalUserSettings == storedGlobalSettings);
        AZ_TEST_ASSERT(storedGlobalSettings->m_intOption1 == 10);

        storedLocalSettings = UserSettings::Find<MyUserSettings>(AZ_CRC_CE("MyUserSettings"), UserSettings::CT_LOCAL);
        AZ_TEST_ASSERT(storedLocalSettings);
        myLocalUserSettings = UserSettings::CreateFind<MyUserSettings>(AZ_CRC_CE("MyUserSettings"), UserSettings::CT_LOCAL);
        AZ_TEST_ASSERT(myLocalUserSettings == storedLocalSettings);
        AZ_TEST_ASSERT(storedLocalSettings->m_intOption1 == 20);


        myGlobalUserSettings = nullptr;
        storedGlobalSettings = nullptr;
        UserSettings::Release(myLocalUserSettings);
        UserSettings::Release(storedLocalSettings);

        app.Destroy();
        //////////////////////////////////////////////////////////////////////////
    }

    class SimpleEntityRefTestComponent
        : public Component
    {
    public:
        AZ_COMPONENT(SimpleEntityRefTestComponent, "{ED4D3C2A-454D-47B0-B04E-9A26DC55D03B}");

        SimpleEntityRefTestComponent(EntityId useId = EntityId())
            : m_entityId(useId) {}

        void Activate() override {}
        void Deactivate() override {}

        static void Reflect(ReflectContext* reflection)
        {
            SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<SimpleEntityRefTestComponent>()
                    ->Field("entityId", &SimpleEntityRefTestComponent::m_entityId);
            }
        }

        EntityId m_entityId;
    };

    class ComplexEntityRefTestComponent
        : public Component
    {
    public:
        AZ_COMPONENT(ComplexEntityRefTestComponent, "{BCCCD213-4A77-474C-B432-48DE6DB2FE4D}");

        ComplexEntityRefTestComponent()
            : m_entityIdHashMap(3) // create some buckets to make sure we distribute elements even when we have less than load factor
            , m_entityIdHashSet(3) // create some buckets to make sure we distribute elements even when we have less than load factor
        {
        }

        void Activate() override {}
        void Deactivate() override {}

        static void Reflect(ReflectContext* reflection)
        {
            SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<ComplexEntityRefTestComponent>()
                    ->Field("entityIds", &ComplexEntityRefTestComponent::m_entityIds)
                    ->Field("entityIdHashMap", &ComplexEntityRefTestComponent::m_entityIdHashMap)
                    ->Field("entityIdHashSet", &ComplexEntityRefTestComponent::m_entityIdHashSet)
                    ->Field("entityId", &ComplexEntityRefTestComponent::m_entityIdIntMap);
            }
        }

        AZStd::vector<EntityId> m_entityIds;
        AZStd::unordered_map<EntityId, int> m_entityIdHashMap;
        AZStd::unordered_set<EntityId> m_entityIdHashSet;
        AZStd::map<EntityId, int> m_entityIdIntMap;
    };

    struct EntityIdRemapContainer
    {
        AZ_TYPE_INFO(EntityIdRemapContainer, "{63854212-37E9-480B-8E46-529682AB9EF7}");
        AZ_CLASS_ALLOCATOR(EntityIdRemapContainer, AZ::SystemAllocator);

        static void Reflect(SerializeContext& serializeContext)
        {
            serializeContext.Class<EntityIdRemapContainer>()
                ->Field("Entity", &EntityIdRemapContainer::m_entity)
                ->Field("Id", &EntityIdRemapContainer::m_id)
                ->Field("otherId", &EntityIdRemapContainer::m_otherId)
                ;
        }
        AZ::Entity* m_entity;
        AZ::EntityId m_id;
        AZ::EntityId m_otherId;
    };

    TEST_F(Components, EntityUtilsTest)
    {
        EntityId id1 = Entity::MakeId();

        {
            EntityId id2 = Entity::MakeId();
            EntityId id3 = Entity::MakeId();
            EntityId id4 = Entity::MakeId();
            EntityId id5 = Entity::MakeId();
            SimpleEntityRefTestComponent testComponent1(id1);
            SimpleEntityRefTestComponent testComponent2(id2);
            SimpleEntityRefTestComponent testComponent3(id3);
            Entity testEntity(id1);
            testEntity.AddComponent(&testComponent1);
            testEntity.AddComponent(&testComponent2);
            testEntity.AddComponent(&testComponent3);

            SerializeContext context;
            const ComponentDescriptor* entityRefTestDescriptor = SimpleEntityRefTestComponent::CreateDescriptor();
            entityRefTestDescriptor->Reflect(&context);
            Entity::Reflect(&context);

                unsigned int nReplaced = EntityUtils::ReplaceEntityRefs(
                &testEntity
                , [=](EntityId key, bool /*isEntityId*/) -> EntityId
            {
                if (key == id1)
                {
                    return id4;
                }
                if (key == id2)
                {
                    return id5;
                }
                return key;
            }
                , &context
                );
            AZ_TEST_ASSERT(nReplaced == 2);
            AZ_TEST_ASSERT(testEntity.GetId() == id1);
            AZ_TEST_ASSERT(testComponent1.m_entityId == id4);
            AZ_TEST_ASSERT(testComponent2.m_entityId == id5);
            AZ_TEST_ASSERT(testComponent3.m_entityId == id3);

            testEntity.RemoveComponent(&testComponent1);
            testEntity.RemoveComponent(&testComponent2);
            testEntity.RemoveComponent(&testComponent3);
            delete entityRefTestDescriptor;
        }

        // Test entity IDs replacement in special containers (that require update as a result of EntityId replacement)
        {
            // special crafted id, so we can change the hashing structure as
            // we replace the entities ID
            EntityId id2(1);
            EntityId id3(13);
            EntityId replace2(14);
            EntityId replace3(3);

            SerializeContext context;
            const ComponentDescriptor* entityRefTestDescriptor = ComplexEntityRefTestComponent::CreateDescriptor();
            entityRefTestDescriptor->Reflect(&context);
            Entity::Reflect(&context);

            ComplexEntityRefTestComponent testComponent1;
            Entity testEntity(id1);
            testEntity.AddComponent(&testComponent1);
            // vector (baseline, it should not change, same with all other AZStd containers not tested below)
            testComponent1.m_entityIds.push_back(id2);
            testComponent1.m_entityIds.push_back(id3);
            testComponent1.m_entityIds.push_back(EntityId(32));
            // hash map
            testComponent1.m_entityIdHashMap.insert(AZStd::make_pair(id2, 1));
            testComponent1.m_entityIdHashMap.insert(AZStd::make_pair(id3, 2));
            testComponent1.m_entityIdHashMap.insert(AZStd::make_pair(EntityId(32), 3));
            testComponent1.m_entityIdHashMap.insert(AZStd::make_pair(EntityId(5), 4));
            testComponent1.m_entityIdHashMap.insert(AZStd::make_pair(EntityId(16), 5));
            // hash set
            testComponent1.m_entityIdHashSet.insert(id2);
            testComponent1.m_entityIdHashSet.insert(id3);
            testComponent1.m_entityIdHashSet.insert(EntityId(32));
            testComponent1.m_entityIdHashSet.insert(EntityId(5));
            testComponent1.m_entityIdHashSet.insert(EntityId(16));
            // map
            testComponent1.m_entityIdIntMap.insert(AZStd::make_pair(id2, 1));
            testComponent1.m_entityIdIntMap.insert(AZStd::make_pair(id3, 2));
            testComponent1.m_entityIdIntMap.insert(AZStd::make_pair(EntityId(32), 3));
            testComponent1.m_entityIdIntMap.insert(AZStd::make_pair(EntityId(5), 4));
            testComponent1.m_entityIdIntMap.insert(AZStd::make_pair(EntityId(16), 5));
            // set is currently not supported in the serializer, when implemented if it uses the same Associative container storage (which it should) it should just work

                unsigned int nReplaced = EntityUtils::ReplaceEntityRefs(
                &testEntity
                , [=](EntityId key, bool /*isEntityId*/) -> EntityId
            {
                if (key == id2)
                {
                    return replace2;
                }
                if (key == id3)
                {
                    return replace3;
                }
                return key;
            }
                , &context
                );
            AZ_TEST_ASSERT(nReplaced == 8);
            AZ_TEST_ASSERT(testEntity.GetId() == id1);

            AZ_TEST_ASSERT(AZStd::find(testComponent1.m_entityIds.begin(), testComponent1.m_entityIds.end(), id2) == testComponent1.m_entityIds.end());
            AZ_TEST_ASSERT(AZStd::find(testComponent1.m_entityIds.begin(), testComponent1.m_entityIds.end(), replace2) != testComponent1.m_entityIds.end());
            AZ_TEST_ASSERT(AZStd::find(testComponent1.m_entityIds.begin(), testComponent1.m_entityIds.end(), replace3) != testComponent1.m_entityIds.end());
            AZ_TEST_ASSERT(AZStd::find(testComponent1.m_entityIds.begin(), testComponent1.m_entityIds.end(), EntityId(32)) != testComponent1.m_entityIds.end());

            AZ_TEST_ASSERT(testComponent1.m_entityIdHashMap.find(id2) == testComponent1.m_entityIdHashMap.end());
            AZ_TEST_ASSERT(testComponent1.m_entityIdHashMap.find(replace2) != testComponent1.m_entityIdHashMap.end());
            AZ_TEST_ASSERT(testComponent1.m_entityIdHashMap.find(replace3) != testComponent1.m_entityIdHashMap.end());
            AZ_TEST_ASSERT(testComponent1.m_entityIdHashMap.find(EntityId(32)) != testComponent1.m_entityIdHashMap.end());

            AZ_TEST_ASSERT(testComponent1.m_entityIdHashSet.find(id2) == testComponent1.m_entityIdHashSet.end());
            AZ_TEST_ASSERT(testComponent1.m_entityIdHashSet.find(replace2) != testComponent1.m_entityIdHashSet.end());
            AZ_TEST_ASSERT(testComponent1.m_entityIdHashSet.find(replace3) != testComponent1.m_entityIdHashSet.end());
            AZ_TEST_ASSERT(testComponent1.m_entityIdHashSet.find(EntityId(32)) != testComponent1.m_entityIdHashSet.end());

            AZ_TEST_ASSERT(testComponent1.m_entityIdIntMap.find(id2) == testComponent1.m_entityIdIntMap.end());
            AZ_TEST_ASSERT(testComponent1.m_entityIdIntMap.find(replace2) != testComponent1.m_entityIdIntMap.end());
            AZ_TEST_ASSERT(testComponent1.m_entityIdIntMap.find(replace3) != testComponent1.m_entityIdIntMap.end());
            AZ_TEST_ASSERT(testComponent1.m_entityIdIntMap.find(EntityId(32)) != testComponent1.m_entityIdIntMap.end());

            testEntity.RemoveComponent(&testComponent1);
            delete entityRefTestDescriptor;
        }
    }

    // Temporary disabled. This will be re-enabled in the short term upon completion of SPEC-7384 and
    // fixed in the long term upon completion of SPEC-4849
    TEST_F(Components, DISABLED_EntityIdGeneration)
    {
        // Generate 1 million ids across 100 threads, and ensure that none collide
        AZStd::concurrent_unordered_set<AZ::EntityId> entityIds;
        auto GenerateIdThread = [&entityIds]()
        {
            for (size_t i = 0; i < AZ_TRAIT_UNIT_TEST_ENTITY_ID_GEN_TEST_COUNT; ++i)
            {
                EXPECT_TRUE(entityIds.insert(Entity::MakeId()));
            }
        };

        //////////////////////////////////////////////////////////////////////////
        // test generating EntityIDs from multiple threads
        {
            AZStd::vector<AZStd::thread> threads;
            for (size_t i = 0; i < 100; ++i)
            {
                threads.emplace_back(GenerateIdThread);
            }
            for (AZStd::thread& thread : threads)
            {
                thread.join();
            }
        }
    }

    //=========================================================================
    // Component Configuration

    class ConfigurableComponentConfig : public ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(ConfigurableComponentConfig , SystemAllocator)
        AZ_RTTI(ConfigurableComponentConfig, "{109C5A93-5571-4D45-BD2F-3938BF63AD83}", ComponentConfig);

        int m_intVal = 0;
    };

    class ConfigurableComponent : public Component
    {
    public:
        AZ_COMPONENT(ConfigurableComponent, "{E3103830-70F3-47AE-8F22-EF09BF3D57E9}");
        static void Reflect(ReflectContext*) {}

        int m_intVal = 0;

    protected:
        void Activate() override {}
        void Deactivate() override {}

        bool ReadInConfig(const ComponentConfig* baseConfig) override
        {
            if (auto config = azrtti_cast<const ConfigurableComponentConfig*>(baseConfig))
            {
                m_intVal = config->m_intVal;
                return true;
            }
            return false;
        }

        bool WriteOutConfig(ComponentConfig* outBaseConfig) const override
        {
            if (auto config = azrtti_cast<ConfigurableComponentConfig*>(outBaseConfig))
            {
                config->m_intVal = m_intVal;
                return true;
            }
            return false;
        }
    };

    class UnconfigurableComponent : public Component
    {
    public:
        AZ_COMPONENT(UnconfigurableComponent, "{772E3AA6-67AC-4655-B6C4-70BC45BAFD35}");
        static void Reflect(ReflectContext*) {}

        void Activate() override {}
        void Deactivate() override {}
    };


    // fixture for testing ComponentConfig stuff
    class ComponentConfiguration
       : public Components
    {
    public:
        void SetUp() override
        {
            Components::SetUp();

            m_descriptors.emplace_back(ConfigurableComponent::CreateDescriptor());
            m_descriptors.emplace_back(UnconfigurableComponent::CreateDescriptor());
        }

        void TearDown() override
        {
            m_descriptors.clear();
            m_descriptors.set_capacity(0);

            Components::TearDown();
        }

        AZStd::vector<AZStd::unique_ptr<ComponentDescriptor>> m_descriptors;
    };

    TEST_F(ComponentConfiguration, SetConfiguration_Succeeds)
    {
        ConfigurableComponentConfig config;
        config.m_intVal = 5;

        ConfigurableComponent component;

        EXPECT_TRUE(component.SetConfiguration(config));
        EXPECT_EQ(component.m_intVal, 5);
    }

    TEST_F(ComponentConfiguration, SetConfigurationOnActiveEntity_DoesNothing)
    {
        ConfigurableComponentConfig config;
        config.m_intVal = 5;

        Entity entity;
        auto component = entity.CreateComponent<ConfigurableComponent>();
        entity.Init();
        entity.Activate();
        EXPECT_EQ(Entity::State::Active, entity.GetState());

        EXPECT_FALSE(component->SetConfiguration(config));
        EXPECT_NE(component->m_intVal, 5);
    }

    TEST_F(ComponentConfiguration, SetWrongKindOfConfiguration_DoesNothing)
    {
        ComponentConfig config; // base config type

        ConfigurableComponent component;
        component.m_intVal = 19;

        EXPECT_FALSE(component.SetConfiguration(config));
        EXPECT_EQ(component.m_intVal, 19);
    }

    TEST_F(ComponentConfiguration, GetConfiguration_Succeeds)
    {
        ConfigurableComponent component;
        component.m_intVal = 9;

        ConfigurableComponentConfig config;

        component.GetConfiguration(config);
        EXPECT_EQ(component.m_intVal, 9);
    }

    TEST_F(ComponentConfiguration, SetConfigurationOnUnconfigurableComponent_Fails)
    {
        UnconfigurableComponent component;
        ConfigurableComponentConfig config;

        EXPECT_FALSE(component.SetConfiguration(config));
    }

    TEST_F(ComponentConfiguration, GetConfigurationOnUnconfigurableComponent_Fails)
    {
        UnconfigurableComponent component;
        ConfigurableComponentConfig config;

        EXPECT_FALSE(component.GetConfiguration(config));
    }

    //=========================================================================

    TEST_F(Components, GenerateNewIdsAndFixRefsExistingMapTest)
    {
        SerializeContext context;
        Entity::Reflect(&context);
        EntityIdRemapContainer::Reflect(context);

        const AZ::EntityId testId(21);
        const AZ::EntityId nonMappedId(5465);
        EntityIdRemapContainer testContainer1;
        testContainer1.m_entity = aznew Entity(testId);
        testContainer1.m_id = testId;
        testContainer1.m_otherId = nonMappedId;

        EntityIdRemapContainer clonedContainer;
        context.CloneObjectInplace(clonedContainer, &testContainer1);

        // Check cloned entity has same ids
        EXPECT_NE(nullptr, clonedContainer.m_entity);
        EXPECT_EQ(testContainer1.m_entity->GetId(), clonedContainer.m_entity->GetId());
        EXPECT_EQ(testContainer1.m_id, clonedContainer.m_id);
        EXPECT_EQ(testContainer1.m_otherId, clonedContainer.m_otherId);

        // Generated new Ids in the testContainer store the results in the newIdMap
        // The m_entity Entity id values should be remapped to a new value
        AZStd::unordered_map<AZ::EntityId, AZ::EntityId> newIdMap;
        EntityUtils::GenerateNewIdsAndFixRefs(&testContainer1, newIdMap, &context);

        EXPECT_EQ(testContainer1.m_entity->GetId(), testContainer1.m_id);
        EXPECT_NE(clonedContainer.m_entity->GetId(), testContainer1.m_entity->GetId());
        EXPECT_NE(clonedContainer.m_id, testContainer1.m_id);
        EXPECT_EQ(clonedContainer.m_otherId, testContainer1.m_otherId);

        // Use the existing newIdMap to generate entityIds for the clonedContainer
        // The testContainer1 and ClonedContainer should now have the same ids again
        EntityUtils::GenerateNewIdsAndFixRefs(&clonedContainer, newIdMap, &context);

        EXPECT_EQ(clonedContainer.m_entity->GetId(), clonedContainer.m_id);
        EXPECT_EQ(testContainer1.m_entity->GetId(), clonedContainer.m_entity->GetId());
        EXPECT_EQ(testContainer1.m_id, clonedContainer.m_id);
        EXPECT_EQ(testContainer1.m_otherId, clonedContainer.m_otherId);

        // Use a new map to generate entityIds for the clonedContainer
        // The testContainer1 and ClonedContainer should have different ids again
        AZStd::map<AZ::EntityId, AZ::EntityId> clonedIdMap; // Using regular map to test that different map types works with GenerateNewIdsAndFixRefs
        EntityUtils::GenerateNewIdsAndFixRefs(&clonedContainer, clonedIdMap, &context);

        EXPECT_EQ(clonedContainer.m_entity->GetId(), clonedContainer.m_id);
        EXPECT_NE(testContainer1.m_entity->GetId(), clonedContainer.m_entity->GetId());
        EXPECT_NE(testContainer1.m_id, clonedContainer.m_id);
        EXPECT_EQ(testContainer1.m_otherId, clonedContainer.m_otherId);
        delete testContainer1.m_entity;
        delete clonedContainer.m_entity;
    }

    //=========================================================================
    // Component Configuration versioning

    // Version 1 of a configuration for a HydraComponent
    class HydraConfigV1
        : public ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(HydraConfigV1, SystemAllocator)
        AZ_RTTI(HydraConfigV1, "{02198FDB-5CDB-4983-BC0B-CF1AA20FF2AF}", ComponentConfig);

        int m_numHeads = 1;
    };

    // To add fields, inherit from previous version.
    class HydraConfigV2
        : public HydraConfigV1
    {
    public:
        AZ_CLASS_ALLOCATOR(HydraConfigV2, SystemAllocator)
        AZ_RTTI(HydraConfigV2, "{BC68C167-6B01-489C-8415-626455670C34}", HydraConfigV1);

        int m_numArms = 2; // now the hydra has multiple arms, as well as multiple heads
    };

    // To make a breaking change, start from scratch by inheriting from base ComponentConfig.
    class HydraConfigV3
        : public ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(HydraConfigV3, SystemAllocator)
        AZ_RTTI(HydraConfigV3, "{71C41829-AA51-4179-B8B4-3C278CBB26AA}", ComponentConfig);

        int m_numHeads = 1;
        int m_numArmsPerHead = 2; // now we require each head to have the same number of arms
    };

    // A component with many heads, and many arms
    class HydraComponent
        : public Component
    {
    public:
        AZ_RTTI(HydraComponent, "", Component);
        AZ_CLASS_ALLOCATOR(HydraComponent, AZ::SystemAllocator);

        // serialized data
        HydraConfigV3 m_config;

        // runtime data
        int m_numArms;

        HydraComponent() = default;

        void Activate() override
        {
            m_numArms = m_config.m_numHeads * m_config.m_numArmsPerHead;
        }

        void Deactivate() override {}

        bool ReadInConfig(const ComponentConfig* baseConfig) override
        {
            if (auto v1 = azrtti_cast<const HydraConfigV1*>(baseConfig))
            {
                m_config.m_numHeads = v1->m_numHeads;

                // v2 is based on v1
                if (auto v2 = azrtti_cast<const HydraConfigV2*>(v1))
                {
                    // v2 let user specify the total number of arms, but now we force each head to have same number of arms
                    if (v2->m_numHeads <= 0)
                    {
                        m_config.m_numArmsPerHead = 0;
                    }
                    else
                    {
                        m_config.m_numArmsPerHead = v2->m_numArms / v2->m_numHeads;
                    }
                }
                else
                {
                    // v1 assumed 2 arms per head
                    m_config.m_numArmsPerHead = 2;
                }

                return true;
            }

            if (auto v3 = azrtti_cast<const HydraConfigV3*>(baseConfig))
            {
                m_config = *v3;
                return true;
            }

            return false;
        }

        bool WriteOutConfig(ComponentConfig* outBaseConfig) const override
        {
            if (auto v1 = azrtti_cast<HydraConfigV1*>(outBaseConfig))
            {
                v1->m_numHeads = m_config.m_numHeads;

                // v2 is based on v1
                if (auto v2 = azrtti_cast<HydraConfigV2*>(v1))
                {
                    v2->m_numArms = m_config.m_numHeads * m_config.m_numArmsPerHead;
                }

                return true;
            }

            if (auto v3 = azrtti_cast<HydraConfigV3*>(outBaseConfig))
            {
                *v3 = m_config;
                return true;
            }

            return false;
        }
    };

    TEST_F(Components, SetConfigurationV1_Succeeds)
    {
        HydraConfigV1 config;
        config.m_numHeads = 3;

        HydraComponent component;

        EXPECT_TRUE(component.SetConfiguration(config));
        EXPECT_EQ(component.m_config.m_numHeads, 3);
    }

    TEST_F(Components, GetConfigurationV1_Succeeds)
    {
        HydraConfigV1 config;

        HydraComponent component;
        component.m_config.m_numHeads = 8;

        EXPECT_TRUE(component.GetConfiguration(config));
        EXPECT_EQ(config.m_numHeads, component.m_config.m_numHeads);
    }

    TEST_F(Components, SetConfigurationV2_Succeeds)
    {
        HydraConfigV2 config;
        config.m_numHeads = 4;
        config.m_numArms = 12;

        HydraComponent component;

        EXPECT_TRUE(component.SetConfiguration(config));
        EXPECT_EQ(component.m_config.m_numHeads, config.m_numHeads);
        EXPECT_EQ(component.m_config.m_numArmsPerHead, 3);
    }

    TEST_F(Components, GetConfigurationV2_Succeeds)
    {
        HydraConfigV2 config;

        HydraComponent component;
        component.m_config.m_numHeads = 12;
        component.m_config.m_numArmsPerHead = 1;

        EXPECT_TRUE(component.GetConfiguration(config));
        EXPECT_EQ(config.m_numHeads, component.m_config.m_numHeads);
        EXPECT_EQ(config.m_numArms, 12);
    }

    TEST_F(Components, SetConfigurationV3_Succeeds)
    {
        HydraConfigV3 config;
        config.m_numHeads = 2;
        config.m_numArmsPerHead = 4;

        HydraComponent component;

        EXPECT_TRUE(component.SetConfiguration(config));
        EXPECT_EQ(component.m_config.m_numHeads, config.m_numHeads);
        EXPECT_EQ(component.m_config.m_numArmsPerHead, config.m_numArmsPerHead);
    }

    TEST_F(Components, GetConfigurationV3_Succeeds)
    {
        HydraConfigV3 config;

        HydraComponent component;
        component.m_config.m_numHeads = 94;
        component.m_config.m_numArmsPerHead = 18;

        EXPECT_TRUE(component.GetConfiguration(config));
        EXPECT_EQ(config.m_numHeads, component.m_config.m_numHeads);
        EXPECT_EQ(config.m_numArmsPerHead, component.m_config.m_numArmsPerHead);
    }

    TEST_F(Components, RemoveDuplicateServicesOfAndAfterIterator_EmptyList_ReturnsFalse)
    {
        AZ::ComponentDescriptor::DependencyArrayType dependencyList;
        const ComponentDescriptor::DependencyArrayType::iterator dependencyIter = dependencyList.begin();
        const bool servicesRemoved = EntityUtils::RemoveDuplicateServicesOfAndAfterIterator(dependencyIter, dependencyList, nullptr);

        EXPECT_FALSE(servicesRemoved);
    }

    TEST_F(Components, RemoveDuplicateServicesOfAndAfterIterator_OnlyOneService_ReturnsFalse)
    {
        AZ::ComponentDescriptor::DependencyArrayType dependencyList;
        dependencyList.push_back(AZ_CRC_CE("SomeService"));

        const ComponentDescriptor::DependencyArrayType::iterator dependencyIter = dependencyList.begin();
        const bool servicesRemoved = EntityUtils::RemoveDuplicateServicesOfAndAfterIterator(dependencyIter, dependencyList, nullptr);

        EXPECT_FALSE(servicesRemoved);
    }

    TEST_F(Components, RemoveDuplicateServicesOfAndAfterIterator_NoDuplicates_ReturnsFalse)
    {
        AZ::ComponentDescriptor::DependencyArrayType dependencyList;
        dependencyList.push_back(AZ_CRC_CE("SomeService"));
        dependencyList.push_back(AZ_CRC_CE("AnotherService"));
        dependencyList.push_back(AZ_CRC_CE("YetAnotherService"));

        for (ComponentDescriptor::DependencyArrayType::iterator dependencyIter = dependencyList.begin();
            dependencyIter != dependencyList.end();
            ++dependencyIter)
        {
            const bool servicesRemoved = EntityUtils::RemoveDuplicateServicesOfAndAfterIterator(dependencyIter, dependencyList, nullptr);
            EXPECT_FALSE(servicesRemoved);
        }
        // Make sure no services were removed.
        EXPECT_EQ(dependencyList.size(), 3);
        EXPECT_EQ(dependencyList[0], AZ_CRC_CE("SomeService"));
        EXPECT_EQ(dependencyList[1], AZ_CRC_CE("AnotherService"));
        EXPECT_EQ(dependencyList[2], AZ_CRC_CE("YetAnotherService"));
    }

    TEST_F(Components, RemoveDuplicateServicesOfAndAfterIterator_DuplicateAfterIterator_ReturnsTrueClearsDuplicates)
    {
        AZ::ComponentDescriptor::DependencyArrayType dependencyList;
        dependencyList.push_back(AZ_CRC_CE("SomeService"));
        dependencyList.push_back(AZ_CRC_CE("AnotherService"));
        dependencyList.push_back(AZ_CRC_CE("YetAnotherService"));
        dependencyList.push_back(AZ_CRC_CE("SomeService"));

        ComponentDescriptor::DependencyArrayType::iterator dependencyIter = dependencyList.begin();
        EXPECT_TRUE(EntityUtils::RemoveDuplicateServicesOfAndAfterIterator(dependencyIter, dependencyList, nullptr));
        ++dependencyIter;
        EXPECT_FALSE(EntityUtils::RemoveDuplicateServicesOfAndAfterIterator(dependencyIter, dependencyList, nullptr));
        ++dependencyIter;
        EXPECT_FALSE(EntityUtils::RemoveDuplicateServicesOfAndAfterIterator(dependencyIter, dependencyList, nullptr));
        ++dependencyIter;
        EXPECT_EQ(dependencyIter, dependencyList.end());
        // Make sure the service was removed.
        EXPECT_EQ(dependencyList.size(), 3);
        EXPECT_EQ(dependencyList[0], AZ_CRC_CE("SomeService"));
        EXPECT_EQ(dependencyList[1], AZ_CRC_CE("AnotherService"));
        EXPECT_EQ(dependencyList[2], AZ_CRC_CE("YetAnotherService"));
    }

    TEST_F(Components, RemoveDuplicateServicesOfAndAfterIterator_2DuplicatesAfterIterator_ReturnsTrueClearsDuplicates)
    {
        AZ::ComponentDescriptor::DependencyArrayType dependencyList;
        dependencyList.push_back(AZ_CRC_CE("SomeService"));
        dependencyList.push_back(AZ_CRC_CE("AnotherService"));
        dependencyList.push_back(AZ_CRC_CE("SomeService"));
        dependencyList.push_back(AZ_CRC_CE("YetAnotherService"));
        dependencyList.push_back(AZ_CRC_CE("SomeService"));

        ComponentDescriptor::DependencyArrayType::iterator dependencyIter = dependencyList.begin();
        EXPECT_TRUE(EntityUtils::RemoveDuplicateServicesOfAndAfterIterator(dependencyIter, dependencyList, nullptr));
        ++dependencyIter;
        EXPECT_FALSE(EntityUtils::RemoveDuplicateServicesOfAndAfterIterator(dependencyIter, dependencyList, nullptr));
        ++dependencyIter;
        EXPECT_FALSE(EntityUtils::RemoveDuplicateServicesOfAndAfterIterator(dependencyIter, dependencyList, nullptr));
        ++dependencyIter;
        EXPECT_EQ(dependencyIter, dependencyList.end());
        // Make sure the service was removed.
        EXPECT_EQ(dependencyList.size(), 3);
        EXPECT_EQ(dependencyList[0], AZ_CRC_CE("SomeService"));
        EXPECT_EQ(dependencyList[1], AZ_CRC_CE("AnotherService"));
        EXPECT_EQ(dependencyList[2], AZ_CRC_CE("YetAnotherService"));
    }

    // The duplicate check logic only checks after the current iterator for performance reasons.
    // This function is primarily used in loops that are already iterating over the service dependencies.
    TEST_F(Components, RemoveDuplicateServicesOfAndAfterIterator_DuplicateBeforeIterator_ReturnsFalseDuplicateRemains)
    {
        AZ::ComponentDescriptor::DependencyArrayType dependencyList;
        dependencyList.push_back(AZ_CRC_CE("SomeService"));
        dependencyList.push_back(AZ_CRC_CE("AnotherService"));
        dependencyList.push_back(AZ_CRC_CE("YetAnotherService"));
        dependencyList.push_back(AZ_CRC_CE("SomeService"));

        ComponentDescriptor::DependencyArrayType::iterator dependencyIter = dependencyList.begin();
        // Skip the first element to leave a duplicate before the iterator.
        ++dependencyIter;
        EXPECT_FALSE(EntityUtils::RemoveDuplicateServicesOfAndAfterIterator(dependencyIter, dependencyList, nullptr));
        ++dependencyIter;
        EXPECT_FALSE(EntityUtils::RemoveDuplicateServicesOfAndAfterIterator(dependencyIter, dependencyList, nullptr));
        ++dependencyIter;
        EXPECT_FALSE(EntityUtils::RemoveDuplicateServicesOfAndAfterIterator(dependencyIter, dependencyList, nullptr));
        ++dependencyIter;
        EXPECT_EQ(dependencyIter, dependencyList.end());
        // Make sure the service was not removed.
        EXPECT_EQ(dependencyList.size(), 4);
        EXPECT_EQ(dependencyList[0], AZ_CRC_CE("SomeService"));
        EXPECT_EQ(dependencyList[1], AZ_CRC_CE("AnotherService"));
        EXPECT_EQ(dependencyList[2], AZ_CRC_CE("YetAnotherService"));
        EXPECT_EQ(dependencyList[3], AZ_CRC_CE("SomeService"));
    }

    TEST_F(Components, RemoveDuplicateServicesOfAndAfterIterator_DuplicateBeforeAndAfterIterator_ReturnsTrueListUpdated)
    {
        AZ::ComponentDescriptor::DependencyArrayType dependencyList;
        dependencyList.push_back(AZ_CRC_CE("SomeService"));
        dependencyList.push_back(AZ_CRC_CE("AnotherService"));
        dependencyList.push_back(AZ_CRC_CE("SomeService"));
        dependencyList.push_back(AZ_CRC_CE("YetAnotherService"));
        dependencyList.push_back(AZ_CRC_CE("SomeService"));

        ComponentDescriptor::DependencyArrayType::iterator dependencyIter = dependencyList.begin();
        // Skip the first element to leave a duplicate before the iterator.
        ++dependencyIter;
        EXPECT_FALSE(EntityUtils::RemoveDuplicateServicesOfAndAfterIterator(dependencyIter, dependencyList, nullptr));
        ++dependencyIter;
        EXPECT_TRUE(EntityUtils::RemoveDuplicateServicesOfAndAfterIterator(dependencyIter, dependencyList, nullptr));
        ++dependencyIter;
        EXPECT_FALSE(EntityUtils::RemoveDuplicateServicesOfAndAfterIterator(dependencyIter, dependencyList, nullptr));
        ++dependencyIter;
        EXPECT_EQ(dependencyIter, dependencyList.end());
        // Make sure one service was removed, and another not removed.
        EXPECT_EQ(dependencyList.size(), 4);
        EXPECT_EQ(dependencyList[0], AZ_CRC_CE("SomeService"));
        EXPECT_EQ(dependencyList[1], AZ_CRC_CE("AnotherService"));
        EXPECT_EQ(dependencyList[2], AZ_CRC_CE("SomeService"));
        EXPECT_EQ(dependencyList[3], AZ_CRC_CE("YetAnotherService"));
    }

    class ComponentDeclImpl
        : public AZ::Component
    {
    public:
        AZ_COMPONENT_DECL(ComponentDeclImpl);
        void Activate() override {}
        void Deactivate() override {}

        static void Reflect(AZ::ReflectContext*) {}
    };

    AZ_COMPONENT_IMPL(ComponentDeclImpl, "ComponentDeclImpl", "{8E5C2D28-8A6D-402E-8018-5AEC828CC3B1}");

    template<class T, class U>
    class TemplateComponent
        : public ComponentDeclImpl
    {
    public:
        AZ_COMPONENT_DECL((TemplateComponent, AZ_CLASS, AZ_CLASS));
    };

    AZ_COMPONENT_IMPL_INLINE((TemplateComponent, AZ_CLASS, AZ_CLASS), "TemplateComponent", "{E8B62C59-CAAC-466C-A583-4FCAABC399E6}", ComponentDeclImpl);

    TEST_F(Components, ComponentDecl_ComponentImpl_Macros_ProvidesCompleteComponentDescriptor_Succeeds)
    {
        {
            auto componentDeclImplDescriptor = AZStd::unique_ptr<AZ::ComponentDescriptor>(ComponentDeclImpl::CreateDescriptor());
            ASSERT_NE(nullptr, componentDeclImplDescriptor);
            auto componentDeclImplComponent = AZStd::unique_ptr<AZ::Component>(componentDeclImplDescriptor->CreateComponent());
            EXPECT_NE(nullptr, componentDeclImplComponent);
        }

        {
            using SpecializedComponent = TemplateComponent<int, int>;
            auto specializedDescriptor = AZStd::unique_ptr<AZ::ComponentDescriptor>(SpecializedComponent::CreateDescriptor());
            ASSERT_NE(nullptr, specializedDescriptor);
            auto specializedDescriptorComponent = AZStd::unique_ptr<AZ::Component>(specializedDescriptor->CreateComponent());
            EXPECT_NE(nullptr, specializedDescriptorComponent);
        }
    }
} // namespace UnitTest

#if defined(HAVE_BENCHMARK)
namespace Benchmark
{
    static void BM_ComponentDependencySort(::benchmark::State& state)
    {
        // descriptors are cleaned up when ComponentApplication shuts down
        aznew UnitTest::ComponentADescriptor;
        aznew UnitTest::ComponentB::DescriptorType;
        aznew UnitTest::ComponentC::DescriptorType;
        aznew UnitTest::ComponentD::DescriptorType;
        aznew UnitTest::ComponentE::DescriptorType;
        aznew UnitTest::ComponentE2::DescriptorType;

        ComponentApplication componentApp;

        ComponentApplication::Descriptor desc;
        desc.m_useExistingAllocator = true;
        AZ::ComponentApplication::StartupParameters startupParameters;
        startupParameters.m_loadSettingsRegistry = false;
        Entity* systemEntity = componentApp.Create(desc, startupParameters);
        systemEntity->Init();

        while(state.KeepRunning())
        {
            // create components to sort
            state.PauseTiming();
            AZ::Entity::ComponentArrayType components;
            AZ_Assert((state.range(0) % 6) == 0, "Multiple of 6 required");
            while ((int)components.size() < state.range(0))
            {
                components.push_back(aznew UnitTest::ComponentA());
                components.push_back(aznew UnitTest::ComponentB());
                components.push_back(aznew UnitTest::ComponentC());
                components.push_back(aznew UnitTest::ComponentD());
                components.push_back(aznew UnitTest::ComponentE());
                components.push_back(aznew UnitTest::ComponentE2());
            }
            state.ResumeTiming();

            // do sort
            Entity::DependencySortOutcome outcome = Entity::DependencySort(components);

            // cleanup
            state.PauseTiming();
            AZ_Assert(outcome.IsSuccess(), "Sort failed");
            for (Component* component : components)
            {
                delete component;
            }
            state.ResumeTiming();
        }
    }

    BENCHMARK(BM_ComponentDependencySort)->Arg(6)->Arg(60);

} // Benchmark
#endif // HAVE_BENCHMARK
