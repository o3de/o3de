/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzFramework/Application/Application.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Spawnable/SpawnableAssetHandler.h>
#include <AzFramework/Spawnable/SpawnableEntitiesManager.h>
#include <AzFramework/Spawnable/Script/SpawnableScriptAssetRef.h>
#include <AzFramework/Spawnable/Script/SpawnableScriptBus.h>
#include <AzFramework/Spawnable/Script/SpawnableScriptMediator.h>
#include <AzTest/AzTest.h>

namespace UnitTest
{
    class TestApplication : public AzFramework::Application
    {
    public:
        // ComponentApplication
        void SetSettingsRegistrySpecializations(AZ::SettingsRegistryInterface::Specializations& specializations) override
        {
            Application::SetSettingsRegistrySpecializations(specializations);
            specializations.Append("test");
            specializations.Append("spawnable");
        }
    };
    
    class SpawnableScriptMediatorTests
        : public AllocatorsFixture
        , public AzFramework::Scripts::SpawnableScriptNotificationsBus::MultiHandler
    {
    public:
        using TicketToEntitiesPair = AZStd::pair<AzFramework::EntitySpawnTicket, AZStd::vector<AZ::EntityId>>;
        constexpr static AZ::u64 EntityIdStartId = 40;

        AZStd::vector<TicketToEntitiesPair> m_spawnedEntities;

        void OnSpawn(AzFramework::EntitySpawnTicket spawnTicket, AZStd::vector<AZ::EntityId> entityList) override
        {
            m_spawnedEntities.push_back({ spawnTicket, AZStd::move(entityList) });
        }

        virtual void OnDespawn(AzFramework::EntitySpawnTicket spawnTicket) override
        {
            AZStd::erase_if(
                m_spawnedEntities,
                [&spawnTicket](const TicketToEntitiesPair& pair) -> bool
                {
                    return pair.first == spawnTicket;
                });
        }

        void SetUp() override
        {
            AllocatorsFixture::SetUp();

            m_application = new TestApplication();
            AZ::ComponentApplication::Descriptor descriptor;
            m_application->Start(descriptor);

            // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
            // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash
            // in the unit tests.
            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);

            auto managerInterface = AzFramework::SpawnableEntitiesInterface::Get();
            m_manager = azrtti_cast<AzFramework::SpawnableEntitiesManager*>(managerInterface);
        }

        void TearDown() override
        {
            AzFramework::Scripts::SpawnableScriptNotificationsBus::MultiHandler::BusDisconnect();
            // One more tick on the spawnable entities manager in order to delete the ticket fully.
            while (m_manager->ProcessQueue(
                       AzFramework::SpawnableEntitiesManager::CommandQueuePriority::High |
                       AzFramework::SpawnableEntitiesManager::CommandQueuePriority::Regular) !=
                   AzFramework::SpawnableEntitiesManager::CommandQueueStatus::NoCommandsLeft)
                ;
            
            m_spawnedEntities = {};

            delete m_application;
            m_application = nullptr;

            AllocatorsFixture::TearDown();
        }

        void WaitForResponse(AzFramework::Scripts::SpawnableScriptMediator& mediator)
        {
            for (size_t i=0; i<1000; ++i) // Don't do this indefinitely to avoid deadlocking on a failing test.
            {
                if (m_manager->ProcessQueue(
                        AzFramework::SpawnableEntitiesManager::CommandQueuePriority::High |
                        AzFramework::SpawnableEntitiesManager::CommandQueuePriority::Regular) ==
                    AzFramework::SpawnableEntitiesManager::CommandQueueStatus::NoCommandsLeft)
                {
                    break;
                }
            }
            mediator.OnTick(0, AZ::ScriptTimePoint());
        }
        
        AzFramework::Scripts::SpawnableScriptAssetRef CreateSpawnable(size_t numElements)
        {
            auto spawnable = aznew AzFramework::Spawnable(
                AZ::Data::AssetId::CreateString("{EB2E8A2B-F253-4A90-BBF4-55F2EED786B8}:0"), AZ::Data::AssetData::AssetStatus::Ready);
            AzFramework::Spawnable::EntityList& entities = spawnable->GetEntities();
            entities.reserve(numElements);
            for (size_t i = 0; i < numElements; ++i)
            {
                auto entry = AZStd::make_unique<AZ::Entity>();
                entry->SetId(AZ::EntityId(EntityIdStartId + i));
                AzFramework::TransformComponent* transformComponent = aznew AzFramework::TransformComponent();
                entry->AddComponent(transformComponent);
                entities.push_back(AZStd::move(entry));
            }
            auto spawnableAsset = AZ::Data::Asset<AzFramework::Spawnable>(spawnable, AZ::Data::AssetLoadBehavior::Default);
            AzFramework::Scripts::SpawnableScriptAssetRef spawnableScriptAssetRef;
            spawnableScriptAssetRef.SetAsset(spawnableAsset);
            return spawnableScriptAssetRef;
        }

    protected:
        AzFramework::SpawnableEntitiesManager* m_manager { nullptr };
        TestApplication* m_application { nullptr };
    };

    TEST_F(SpawnableScriptMediatorTests, CreateSpawnTicket_Works)
    {
        using namespace AzFramework::Scripts;
        auto spawnable = CreateSpawnable(1);
        SpawnableScriptMediator mediator;
        auto ticket = mediator.CreateSpawnTicket(spawnable);
        EXPECT_TRUE(ticket.IsValid());
    }

    TEST_F(SpawnableScriptMediatorTests, SpawnAndDespawn_Works)
    {
        using namespace AzFramework::Scripts;

        auto spawnable = CreateSpawnable(1);
        SpawnableScriptMediator mediator;
        const auto ticket = mediator.CreateSpawnTicket(spawnable);

        SpawnableScriptNotificationsBus::MultiHandler::BusConnect(ticket.GetId());

        mediator.Spawn(ticket);
        WaitForResponse(mediator);
        EXPECT_EQ(m_spawnedEntities.size(), 1);
        EXPECT_EQ(m_spawnedEntities[0].first, ticket);
        EXPECT_EQ(m_spawnedEntities[0].second.size(), 1);

        mediator.Despawn(ticket);
        WaitForResponse(mediator);

        EXPECT_TRUE(m_spawnedEntities.empty());
    }

    TEST_F(SpawnableScriptMediatorTests, SpawnAndMultipleAtOnce_Works)
    {
        using namespace AzFramework::Scripts;
        
        auto spawnable1 = CreateSpawnable(1);
        auto spawnable2 = CreateSpawnable(2);

        SpawnableScriptMediator mediator;

        auto ticket1 = mediator.CreateSpawnTicket(spawnable1);
        auto ticket2 = mediator.CreateSpawnTicket(spawnable2);

        SpawnableScriptNotificationsBus::MultiHandler::BusConnect(ticket1.GetId());
        SpawnableScriptNotificationsBus::MultiHandler::BusConnect(ticket2.GetId());

        mediator.Spawn(ticket1);
        mediator.Spawn(ticket2);
        WaitForResponse(mediator);

        auto it1 = AZStd::find_if(
            m_spawnedEntities.begin(), m_spawnedEntities.end(),
            [&ticket1](const TicketToEntitiesPair& pair) -> bool
            {
                return pair.first == ticket1;
            });
        EXPECT_FALSE(it1 == m_spawnedEntities.end());
        EXPECT_EQ(it1->second.size(), 1);

        auto it2 = AZStd::find_if(
            m_spawnedEntities.begin(), m_spawnedEntities.end(),
            [&ticket2](const TicketToEntitiesPair& pair) -> bool
            {
                return pair.first == ticket2;
            });
        EXPECT_FALSE(it2 == m_spawnedEntities.end());
        EXPECT_EQ(it2->second.size(), 2);

        mediator.Despawn(ticket1);
        mediator.Despawn(ticket2);
        WaitForResponse(mediator);

        EXPECT_TRUE(m_spawnedEntities.empty());
    }

    TEST_F(SpawnableScriptMediatorTests, SpawnAndParent_Works)
    {
        using namespace AzFramework::Scripts;

        auto spawnable = CreateSpawnable(1);
        SpawnableScriptMediator mediator;
        auto ticket = mediator.CreateSpawnTicket(spawnable);

        SpawnableScriptNotificationsBus::MultiHandler::BusConnect(ticket.GetId());

        AZ::EntityId parentId = AZ::Entity::MakeId();
        AZ::Entity parentEntity(parentId);
        AzFramework::TransformComponent parentTransformComponent;
        parentEntity.AddComponent(&parentTransformComponent);
        parentEntity.Init();
        parentEntity.Activate();

        mediator.SpawnAndParent(ticket, parentEntity.GetId());
        WaitForResponse(mediator);

        AZStd::vector<AZ::EntityId> descendantIds;
        AZ::TransformBus::EventResult(descendantIds, parentId, &AZ::TransformBus::Events::GetAllDescendants);
        EXPECT_EQ(descendantIds.size(), 1);
        EXPECT_EQ(m_spawnedEntities.size(), 1);
        EXPECT_EQ(m_spawnedEntities[0].second.size(), 1);
        EXPECT_EQ(descendantIds[0], m_spawnedEntities[0].second[0]);

        mediator.Despawn(ticket);
        WaitForResponse(mediator);

        AZ::TransformBus::EventResult(descendantIds, parentId, &AZ::TransformBus::Events::GetAllDescendants);
        EXPECT_TRUE(descendantIds.empty());

        parentEntity.Deactivate();
    }

    TEST_F(SpawnableScriptMediatorTests, SpawnAndParentAndTransform_Works)
    {
        using namespace AzFramework::Scripts;

        auto spawnable = CreateSpawnable(1);
        SpawnableScriptMediator mediator;
        auto ticket = mediator.CreateSpawnTicket(spawnable);

        SpawnableScriptNotificationsBus::MultiHandler::BusConnect(ticket.GetId());

        AZ::EntityId parentId = AZ::Entity::MakeId();
        AZ::Entity parentEntity(parentId);
        AzFramework::TransformComponent parentTransformComponent;
        parentEntity.AddComponent(&parentTransformComponent);
        parentEntity.Init();
        parentEntity.Activate();

        AZ::Vector3 translation(5, 0, 0);
        AZ::Vector3 rotation(90, 0, 0);
        float scale = 2.0f;

        mediator.SpawnAndParentAndTransform(ticket, parentEntity.GetId(), translation, rotation, scale);
        WaitForResponse(mediator);

        AZStd::vector<AZ::EntityId> descendantIds;
        AZ::TransformBus::EventResult(descendantIds, parentId, &AZ::TransformBus::Events::GetAllDescendants);

        EXPECT_EQ(descendantIds.size(), 1);
        auto it = AZStd::find_if(
            m_spawnedEntities.begin(), m_spawnedEntities.end(),
            [&ticket](const TicketToEntitiesPair& pair) -> bool
            {
                return pair.first.GetId() == ticket.GetId();
            });
        EXPECT_FALSE(it == m_spawnedEntities.end());
        EXPECT_FALSE(it->second.empty());

        AZ::EntityId entityId = it->second[0];
        AZ::Vector3 spawnedTranslation = {};
        AZ::Quaternion spawnedRotation = {};
        float spawnedScale;

        AZ::TransformBus::EventResult(spawnedTranslation, entityId, &AZ::TransformBus::Events::GetLocalTranslation);
        AZ::TransformBus::EventResult(spawnedRotation, entityId, &AZ::TransformBus::Events::GetLocalRotationQuaternion);
        AZ::TransformBus::EventResult(spawnedScale, entityId, &AZ::TransformBus::Events::GetLocalUniformScale);
        EXPECT_EQ(translation, spawnedTranslation);
        EXPECT_EQ(AZ::Quaternion::CreateFromEulerAnglesDegrees(rotation), spawnedRotation);
        EXPECT_EQ(scale, spawnedScale);

        mediator.Despawn(ticket);
        WaitForResponse(mediator);

        AZ::TransformBus::EventResult(descendantIds, parentId, &AZ::TransformBus::Events::GetAllDescendants);
        EXPECT_TRUE(descendantIds.empty());

        parentEntity.Deactivate();
    }
} // namespace UnitTest
