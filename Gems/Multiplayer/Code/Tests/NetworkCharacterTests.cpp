/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CommonHierarchySetup.h>
#include <MockInterfaces.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Console/Console.h>
#include <AzCore/Name/Name.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UnitTest/UnitTest.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Physics/Material/PhysicsMaterialSystemComponent.h>
#include <AzFramework/Visibility/EntityVisibilityBoundsUnionSystem.h>
#include <AzTest/AzTest.h>
#include <Multiplayer/Components/NetBindComponent.h>
#include <Multiplayer/Components/NetworkCharacterComponent.h>
#include <Multiplayer/NetworkEntity/EntityReplication/EntityReplicator.h>
#include <Source/SystemComponent.h>
#include <Source/System/PhysXCookingParams.h>
#include <Source/System/PhysXSystem.h>
#include <PhysXCharacters/Components/CharacterControllerComponent.h>

namespace Multiplayer
{
    using namespace testing;
    using namespace ::UnitTest;

    /*
     * (Networked) Parent -> (Networked) Child
     */
    class NetworkCharacterTests : public HierarchyTests
    {
    public:
        void SetUp() override
        {
            HierarchyTests::SetUp();

            // create the asset database
            {
                AZ::Data::AssetManager::Descriptor desc;
                AZ::Data::AssetManager::Create(desc);
            }

            m_systemEntity = new AZ::Entity();
            m_physXSystem = AZStd::make_unique<PhysX::PhysXSystem>(AZStd::make_unique<PhysX::PhysXSettingsRegistryManager>(), PhysX::PxCooking::GetRealTimeCookingParams());

            m_physMaterialSystemDescriptor.reset(Physics::MaterialSystemComponent::CreateDescriptor());
            m_physMaterialSystemDescriptor->Reflect(m_serializeContext.get());

            m_physXSystemDescriptor.reset(PhysX::SystemComponent::CreateDescriptor());
            m_physXSystemDescriptor->Reflect(m_serializeContext.get());

            m_systemEntity->CreateComponent<Physics::MaterialSystemComponent>();
            m_systemEntity->CreateComponent<PhysX::SystemComponent>();
            m_systemEntity->Init();
            m_systemEntity->Activate();

            m_visisbilitySystem = AZStd::make_unique<AzFramework::EntityVisibilityBoundsUnionSystem>();
            m_visisbilitySystem->Connect();

            AZ::EntityBus::Broadcast(&AZ::EntityBus::Events::OnEntityActivated, m_systemEntity->GetId());
            AzFramework::GameEntityContextEventBus::Broadcast(&AzFramework::GameEntityContextEventBus::Events::OnPreGameEntitiesStarted);

            m_charControllerDescriptor.reset(PhysX::CharacterControllerComponent::CreateDescriptor());
            m_charControllerDescriptor->Reflect(m_serializeContext.get());

            m_netCharDescriptor.reset(NetworkCharacterComponent::CreateDescriptor());
            m_netCharDescriptor->Reflect(m_serializeContext.get());

            m_root = AZStd::make_unique<EntityInfo>(1, "root", NetEntityId{ 1 }, EntityInfo::Role::Root);
            m_child = AZStd::make_unique<EntityInfo>(2, "child", NetEntityId{ 2 }, EntityInfo::Role::Child);

            CreateNetworkParentChild(*m_root, *m_child);

            AZ::Transform rootTransform = AZ::Transform::CreateIdentity();
            rootTransform.SetTranslation(AZ::Vector3::CreateOne());
            m_root->m_entity->FindComponent<AzFramework::TransformComponent>()->SetWorldTM(rootTransform);
            m_child->m_entity->FindComponent<AzFramework::TransformComponent>()->SetWorldTM(rootTransform);

            m_child->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_root->m_entity->GetId());
            m_child->m_entity->FindComponent<AzFramework::TransformComponent>()->SetLocalTM(AZ::Transform::CreateIdentity());

            AZ::EntityBus::Broadcast(&AZ::EntityBus::Events::OnEntityActivated, m_root->m_entity->GetId());
        }

        void TearDown() override
        {
            m_child.reset();
            m_root.reset();

            m_visisbilitySystem->Disconnect();
            m_visisbilitySystem.reset();
            m_systemEntity->Deactivate();
            delete m_systemEntity;
            m_physXSystem.reset();

            m_netCharDescriptor.reset();
            m_charControllerDescriptor.reset();
            m_physXSystemDescriptor.reset();
            m_physMaterialSystemDescriptor.reset();            

            AZ::Data::AssetManager::Destroy();

            HierarchyTests::TearDown();
        }

        void PopulateNetworkEntity(const EntityInfo& entityInfo)
        {
            entityInfo.m_entity->CreateComponent<AzFramework::TransformComponent>();
            entityInfo.m_entity->CreateComponent<NetBindComponent>();
            entityInfo.m_entity->CreateComponent<NetworkTransformComponent>();
            entityInfo.m_entity->CreateComponent<PhysX::CharacterControllerComponent>(
                AZStd::make_unique<Physics::CharacterConfiguration>(),
                AZStd::make_shared<Physics::BoxShapeConfiguration>());
            entityInfo.m_entity->CreateComponent<NetworkCharacterComponent>();
        }

        void CreateNetworkParentChild(EntityInfo& root, EntityInfo& child)
        {
            PopulateNetworkEntity(root);
            SetupEntity(root.m_entity, root.m_netId, NetEntityRole::Authority);

            PopulateNetworkEntity(child);
            SetupEntity(child.m_entity, child.m_netId, NetEntityRole::Authority);

            // Create an entity replicator for the child entity
            const NetworkEntityHandle childHandle(child.m_entity.get(), m_networkEntityTracker.get());
            child.m_replicator = AZStd::make_unique<EntityReplicator>(*m_entityReplicationManager, m_mockConnection.get(), NetEntityRole::Client, childHandle);
            child.m_replicator->Initialize(childHandle);

            // Create an entity replicator for the root entity
            const NetworkEntityHandle rootHandle(root.m_entity.get(), m_networkEntityTracker.get());
            root.m_replicator = AZStd::make_unique<EntityReplicator>(*m_entityReplicationManager, m_mockConnection.get(), NetEntityRole::Client, rootHandle);
            root.m_replicator->Initialize(rootHandle);

            root.m_entity->Activate();
            child.m_entity->Activate();
        }

        AZStd::unique_ptr<AZ::ComponentDescriptor> m_physMaterialSystemDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_physXSystemDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_charControllerDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_netCharDescriptor;

        AZStd::unique_ptr<PhysX::PhysXSystem> m_physXSystem;
        AZStd::unique_ptr<AzFramework::EntityVisibilityBoundsUnionSystem> m_visisbilitySystem;

        AZ::Entity* m_systemEntity;
        AZStd::unique_ptr<EntityInfo> m_root;
        AZStd::unique_ptr<EntityInfo> m_child;
    };

    TEST_F(NetworkCharacterTests, TestMoveWithVelocity)
    {
        NetworkCharacterComponentController* controller = dynamic_cast<NetworkCharacterComponentController*>(
            m_root->m_entity->FindComponent<NetworkCharacterComponent>()->GetController());

        // NO_COUNT suppression here as we expect a MATH_ASSERT which is disabled in Profile
        AZ_TEST_START_TRACE_SUPPRESSION;
        controller->TryMoveWithVelocity(AZ::Vector3(100.f,100.f,100.f), 1.f);
        AZ_TEST_STOP_TRACE_SUPPRESSION_NO_COUNT;
        m_root->m_entity->FindComponent<NetBindComponent>()->NotifySyncRewindState();
    }
}
