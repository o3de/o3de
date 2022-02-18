/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Jobs/JobManagerComponent.h>
#include <AzCore/Memory/MemoryComponent.h>
#include <AzCore/Memory/PoolAllocator.h>

#include <AzTest/AzTest.h>

#include <Translation/TranslationBus.h>

//using ::testing::AtLeast;
//using ::testing::FloatNear;
//using ::testing::FloatEq;
//using ::testing::IsFalse;
//using ::testing::Ne;
//using ::testing::NiceMock;
//using ::testing::Return;
//using ::testing::SetArgReferee;

class GraphCanvasTest : public ::testing::Test
{
protected:

        void SetUp() override
        {
            AZ::AllocatorInstance<AZ::PoolAllocator>::Create();
            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Create();

            AZ::ComponentApplication::Descriptor appDesc;
            appDesc.m_memoryBlocksByteSize = 20 * 1024 * 1024;
            appDesc.m_recordingMode = AZ::Debug::AllocationRecords::RECORD_NO_RECORDS;
            appDesc.m_stackRecordLevels = 20;

            m_app.Create(appDesc);

            // Create the global job manager.
            m_jobManagerEntity = CreateEntity();
            CreateComponent<AZ::JobManagerComponent>(m_jobManagerEntity.get());
            ActivateEntity(m_jobManagerEntity.get());
        }

        void TearDown() override
        {
            // Destroy the global job manager.
            m_jobManagerEntity->Deactivate();
            m_jobManagerEntity.reset();

            m_app.Destroy();

            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Destroy();
            AZ::AllocatorInstance<AZ::PoolAllocator>::Destroy();
        }

        AZStd::unique_ptr<AZ::Entity> CreateEntity()
        {
            return AZStd::make_unique<AZ::Entity>();
        }


        void ActivateEntity(AZ::Entity* entity)
        {
            entity->Init();
            EXPECT_EQ(AZ::Entity::State::Init, entity->GetState());

            entity->Activate();
            EXPECT_EQ(AZ::Entity::State::Active, entity->GetState());
        }

        template<typename Component, typename Configuration>
        AZ::Component* CreateComponent(AZ::Entity* entity, const Configuration& config)
        {
            m_app.RegisterComponentDescriptor(Component::CreateDescriptor());
            return entity->CreateComponent<Component>(config);
        }

        template<typename Component>
        AZ::Component* CreateComponent(AZ::Entity* entity)
        {
            m_app.RegisterComponentDescriptor(Component::CreateDescriptor());
            return entity->CreateComponent<Component>();
        }

        AZ::ComponentApplication m_app;
        AZStd::unique_ptr<AZ::Entity> m_jobManagerEntity = nullptr;

};

TEST_F(GraphCanvasTest, Translation)
{
    GraphCanvas::TranslationKey key1("Constructed");
    EXPECT_STRCASEEQ(key1.ToString().c_str(), "Constructed");

    GraphCanvas::TranslationKey key2;

    // Test key assignment
    key2 = "START";
    EXPECT_EQ(key2, "START");

    // Test key concatenation with const char*
    key2 << "TEST";
    EXPECT_EQ(key2, "START.TEST");

    // Test key concatenation with AZStd::string
    AZStd::string test1 = "STRING";
    key2 << test1;
    EXPECT_EQ(key2, "START.TEST.STRING");

    // Test clear
    key2.clear();
    EXPECT_EQ(key2, "");

    // Test Key assignment into AZStd::string
    key2 << "NEW";
    AZStd::string test2 = key2;
    EXPECT_STRCASEEQ(test2.c_str(), key2.ToString().c_str());

    GraphCanvas::TranslationKey key3 = key2;

    // Compare Key to Key
    EXPECT_EQ(key2, key3);

    // Compare Key to const char*
    EXPECT_EQ(key3, "NEW");
}
