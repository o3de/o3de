/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CommonHierarchySetup.h>
#include <MockInterfaces.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Console/Console.h>
#include <AzCore/Name/Name.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UnitTest/UnitTest.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzTest/AzTest.h>
#include <Multiplayer/Components/NetBindComponent.h>
#include <Multiplayer/Components/NetworkRigidBodyComponent.h>
#include <Multiplayer/NetworkEntity/EntityReplication/EntityReplicator.h>
#include <Source/RigidBodyComponent.h>

namespace Multiplayer
{
    using namespace testing;
    using namespace ::UnitTest;

    class MockSceneInterface : public AZ::Interface<AzPhysics::SceneInterface>::Registrar
    {
    public:
        MockSceneInterface() = default;
        ~MockSceneInterface() override = default;

        MOCK_METHOD4(
            AddJoint,
            AzPhysics::JointHandle(
                AzPhysics::SceneHandle,
                const AzPhysics::JointConfiguration*,
                AzPhysics::SimulatedBodyHandle,
                AzPhysics::SimulatedBodyHandle));
        MOCK_METHOD2(
            AddSimulatedBodies,
            AzPhysics::SimulatedBodyHandleList(AzPhysics::SceneHandle, const AzPhysics::SimulatedBodyConfigurationList&));
        MOCK_METHOD2(
            AddSimulatedBody, AzPhysics::SimulatedBodyHandle(AzPhysics::SceneHandle, const AzPhysics::SimulatedBodyConfiguration*));
        MOCK_METHOD2(DisableSimulationOfBody, void(AzPhysics::SceneHandle, AzPhysics::SimulatedBodyHandle));
        MOCK_METHOD2(EnableSimulationOfBody, void(AzPhysics::SceneHandle, AzPhysics::SimulatedBodyHandle));
        MOCK_METHOD1(FinishSimulation, void(AzPhysics::SceneHandle));
        MOCK_CONST_METHOD1(GetGravity, AZ::Vector3(AzPhysics::SceneHandle));
        MOCK_METHOD2(GetJointFromHandle, AzPhysics::Joint*(AzPhysics::SceneHandle, AzPhysics::JointHandle));
        MOCK_METHOD1(GetSceneHandle, AzPhysics::SceneHandle(const AZStd::string&));
        MOCK_METHOD1(GetScene, AzPhysics::Scene*(AzPhysics::SceneHandle));
        MOCK_METHOD2(
            GetSimulatedBodiesFromHandle, AzPhysics::SimulatedBodyList(AzPhysics::SceneHandle, const AzPhysics::SimulatedBodyHandleList&));
        MOCK_METHOD2(GetSimulatedBodyFromHandle, AzPhysics::SimulatedBody*(AzPhysics::SceneHandle, AzPhysics::SimulatedBodyHandle));
        MOCK_CONST_METHOD1(IsEnabled, bool(AzPhysics::SceneHandle));
        MOCK_METHOD2(QueryScene, AzPhysics::SceneQueryHits(AzPhysics::SceneHandle, const AzPhysics::SceneQueryRequest*));
        MOCK_METHOD3(QueryScene, bool(AzPhysics::SceneHandle sceneHandle, const AzPhysics::SceneQueryRequest* request, AzPhysics::SceneQueryHits&));
        MOCK_METHOD4(
            QuerySceneAsync,
            bool(
                AzPhysics::SceneHandle,
                AzPhysics::SceneQuery::AsyncRequestId,
                const AzPhysics::SceneQueryRequest*,
                AzPhysics::SceneQuery::AsyncCallback));
        MOCK_METHOD4(
            QuerySceneAsyncBatch,
            bool(
                AzPhysics::SceneHandle,
                AzPhysics::SceneQuery::AsyncRequestId,
                const AzPhysics::SceneQueryRequests&,
                AzPhysics::SceneQuery::AsyncBatchCallback));
        MOCK_METHOD2(QuerySceneBatch, AzPhysics::SceneQueryHitsList(AzPhysics::SceneHandle, const AzPhysics::SceneQueryRequests&));
        MOCK_METHOD2(
            RegisterSceneActiveSimulatedBodiesHandler,
            void(
                AzPhysics::SceneHandle,
                AZ::Event<AZStd::tuple<AZ::Crc32, signed char>, const AZStd::vector<AZStd::tuple<AZ::Crc32, int>>&, float>::Handler&));
        MOCK_METHOD2(
            RegisterSceneCollisionEventHandler,
            void(
                AzPhysics::SceneHandle,
                AZ::Event<AZStd::tuple<AZ::Crc32, signed char>, const AZStd::vector<AzPhysics::CollisionEvent>&>::Handler&));
        MOCK_METHOD2(
            RegisterSceneConfigurationChangedEventHandler,
            void(AzPhysics::SceneHandle, AZ::Event<AZStd::tuple<AZ::Crc32, signed char>, const AzPhysics::SceneConfiguration&>::Handler&));
        MOCK_METHOD2(
            RegisterSceneGravityChangedEvent,
            void(AzPhysics::SceneHandle, AZ::Event<AZStd::tuple<AZ::Crc32, signed char>, const AZ::Vector3&>::Handler&));
        MOCK_METHOD2(
            RegisterSceneSimulationFinishHandler, void(AzPhysics::SceneHandle, AzPhysics::SceneEvents::OnSceneSimulationFinishHandler&));
        MOCK_METHOD2(
            RegisterSceneSimulationStartHandler, void(AzPhysics::SceneHandle, AzPhysics::SceneEvents::OnSceneSimulationStartHandler&));
        MOCK_METHOD2(
            RegisterSceneTriggersEventHandler,
            void(
                AzPhysics::SceneHandle,
                AZ::Event<AZStd::tuple<AZ::Crc32, signed char>, const AZStd::vector<AzPhysics::TriggerEvent>&>::Handler&));
        MOCK_METHOD2(
            RegisterSimulationBodyAddedHandler,
            void(AzPhysics::SceneHandle, AZ::Event<AZStd::tuple<AZ::Crc32, signed char>, AZStd::tuple<AZ::Crc32, int>>::Handler&));
        MOCK_METHOD2(
            RegisterSimulationBodyRemovedHandler,
            void(AzPhysics::SceneHandle, AZ::Event<AZStd::tuple<AZ::Crc32, signed char>, AZStd::tuple<AZ::Crc32, int>>::Handler&));
        MOCK_METHOD2(
            RegisterSimulationBodySimulationDisabledHandler,
            void(AzPhysics::SceneHandle, AZ::Event<AZStd::tuple<AZ::Crc32, signed char>, AZStd::tuple<AZ::Crc32, int>>::Handler&));
        MOCK_METHOD2(
            RegisterSimulationBodySimulationEnabledHandler,
            void(AzPhysics::SceneHandle, AZ::Event<AZStd::tuple<AZ::Crc32, signed char>, AZStd::tuple<AZ::Crc32, int>>::Handler&));
        MOCK_METHOD2(RemoveJoint, void(AzPhysics::SceneHandle, AzPhysics::JointHandle));
        MOCK_METHOD2(RemoveSimulatedBodies, void(AzPhysics::SceneHandle, AzPhysics::SimulatedBodyHandleList&));
        MOCK_METHOD2(RemoveSimulatedBody, void(AzPhysics::SceneHandle, AzPhysics::SimulatedBodyHandle&));
        MOCK_METHOD2(SetEnabled, void(AzPhysics::SceneHandle, bool));
        MOCK_METHOD2(SetGravity, void(AzPhysics::SceneHandle, const AZ::Vector3&));
        MOCK_METHOD2(StartSimulation, void(AzPhysics::SceneHandle, float));
        MOCK_METHOD3(
            SuppressCollisionEvents,
            void(AzPhysics::SceneHandle, const AzPhysics::SimulatedBodyHandle&, const AzPhysics::SimulatedBodyHandle&));
        MOCK_METHOD3(
            UnsuppressCollisionEvents,
            void(AzPhysics::SceneHandle, const AzPhysics::SimulatedBodyHandle&, const AzPhysics::SimulatedBodyHandle&));
    };

    class MockPhysicsDefaultWorldRequestsHandler : public Physics::DefaultWorldBus::Handler
    {
    public:
        MockPhysicsDefaultWorldRequestsHandler()
        {
            Physics::DefaultWorldBus::Handler::BusConnect();
        }
        ~MockPhysicsDefaultWorldRequestsHandler()
        {
            Physics::DefaultWorldBus::Handler::BusDisconnect();
        }
        AzPhysics::SceneHandle GetDefaultSceneHandle() const
        {
            return { AZ::Crc32(), AzPhysics::SceneIndex(0) };
        }
    };

    class NetworkRigidBodyTests : public HierarchyTests
    {
    public:
        void SetUp() override
        {
            HierarchyTests::SetUp();

            m_rigidBodyDescriptor.reset(PhysX::RigidBodyComponent::CreateDescriptor());
            m_rigidBodyDescriptor->Reflect(m_serializeContext.get());

            m_netRigidBodyDescriptor.reset(NetworkRigidBodyComponent::CreateDescriptor());
            m_netRigidBodyDescriptor->Reflect(m_serializeContext.get());

            m_rigidBody = AZStd::make_unique<PhysX::RigidBody>();
            m_mockSceneInterface = AZStd::make_unique<Multiplayer::MockSceneInterface>();
            ON_CALL(*m_mockSceneInterface, GetSimulatedBodyFromHandle(_, _))
                .WillByDefault(Invoke(
                    [this](AzPhysics::SceneHandle, AzPhysics::SimulatedBodyHandle)
                    {
                        return m_rigidBody.get();
                    }
            ));

            m_mockDefaultWorld = AZStd::make_unique<Multiplayer::MockPhysicsDefaultWorldRequestsHandler>();

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

            m_mockDefaultWorld.reset();
            m_mockSceneInterface.reset();
            m_rigidBody.reset();

            m_netRigidBodyDescriptor.reset();
            m_rigidBodyDescriptor.reset();

            HierarchyTests::TearDown();
        }

        void PopulateNetworkEntity(const EntityInfo& entityInfo)
        {
            entityInfo.m_entity->CreateComponent<AzFramework::TransformComponent>();
            entityInfo.m_entity->CreateComponent<NetBindComponent>();
            entityInfo.m_entity->CreateComponent<NetworkTransformComponent>();
            entityInfo.m_entity->CreateComponent<PhysX::RigidBodyComponent>();
            entityInfo.m_entity->CreateComponent<NetworkRigidBodyComponent>();
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

        AZStd::unique_ptr<Multiplayer::MockSceneInterface> m_mockSceneInterface;
        AZStd::unique_ptr<Multiplayer::MockPhysicsDefaultWorldRequestsHandler> m_mockDefaultWorld;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_rigidBodyDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_netRigidBodyDescriptor;

        AZStd::unique_ptr<PhysX::RigidBody> m_rigidBody;
        AZStd::unique_ptr<EntityInfo> m_root;
        AZStd::unique_ptr<EntityInfo> m_child;
    };

    TEST_F(NetworkRigidBodyTests, TestHandleSendApplyImpulse)
    {
        NetworkRigidBodyComponentController* controller =
            dynamic_cast < NetworkRigidBodyComponentController*>(m_root->m_entity->FindComponent<NetworkRigidBodyComponent>()->GetController());

        controller->HandleSendApplyImpulse(nullptr, AZ::Vector3::CreateOne(), AZ::Vector3::CreateOne());
    }

    TEST_F(NetworkRigidBodyTests, TestSyncRewind)
    {
        m_root->m_entity->FindComponent<NetBindComponent>()->NotifySyncRewindState();
    }
}
