/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/GemTestEnvironment.h>
#include <gmock/gmock.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Physics/Collision/CollisionGroups.h>
#include <AzFramework/Physics/Collision/CollisionLayers.h>
#include <AzFramework/Physics/Common/PhysicsSimulatedBody.h>
#include <AzFramework/Physics/Common/PhysicsTypes.h>
#include <World.h>

namespace ScriptCanvasPhysics
{
    namespace WorldFunctions
    {
        extern Result RayCastWorldSpaceWithGroup(const AZ::Vector3& start,
            const AZ::Vector3& direction,
            float distance,
            const AZStd::string& collisionGroup,
            AZ::EntityId ignore);

        extern Result RayCastLocalSpaceWithGroup(const AZ::EntityId& fromEntityId,
            const AZ::Vector3& direction,
            float distance,
            const AZStd::string& collisionGroup,
            AZ::EntityId ignore);

        extern AZStd::vector<AzPhysics::SceneQueryHit> RayCastMultipleLocalSpaceWithGroup(const AZ::EntityId& fromEntityId,
            const AZ::Vector3& direction,
            float distance,
            const AZStd::string& collisionGroup,
            AZ::EntityId ignore);

        extern Result ShapecastQuery(float distance,
            const AZ::Transform& pose,
            const AZ::Vector3& direction,
            AZStd::shared_ptr<Physics::ShapeConfiguration> shape,
            const AZStd::string& collisionGroup,
            AZ::EntityId ignore);
    }
}

namespace ScriptCanvasPhysicsTests
{
    using namespace ::testing;

    //Mocked of the AzPhysics Scene Interface. To keep things simple just mocked functions that have a return value OR required for a test.
    class MockPhysicsSceneInterface
        : AZ::Interface<AzPhysics::SceneInterface>::Registrar
    {
    public:

        void StartSimulation(
            [[maybe_unused]] AzPhysics::SceneHandle sceneHandle,
            [[maybe_unused]] float deltatime) override {}
        void FinishSimulation([[maybe_unused]] AzPhysics::SceneHandle sceneHandle) override {}
        void SetEnabled(
            [[maybe_unused]] AzPhysics::SceneHandle sceneHandle,
            [[maybe_unused]] bool enable) override {}
        void RemoveSimulatedBody(
            [[maybe_unused]] AzPhysics::SceneHandle sceneHandle,
            [[maybe_unused]] AzPhysics::SimulatedBodyHandle& bodyHandle) override {}
        void RemoveSimulatedBodies(
            [[maybe_unused]] AzPhysics::SceneHandle sceneHandle,
            [[maybe_unused]] AzPhysics::SimulatedBodyHandleList& bodyHandles) override {}
        void EnableSimulationOfBody(
            [[maybe_unused]] AzPhysics::SceneHandle sceneHandle,
            [[maybe_unused]] AzPhysics::SimulatedBodyHandle bodyHandle) override {}
        void DisableSimulationOfBody(
            [[maybe_unused]] AzPhysics::SceneHandle sceneHandle,
            [[maybe_unused]] AzPhysics::SimulatedBodyHandle bodyHandle) override {}
        void RemoveJoint(
            [[maybe_unused]]AzPhysics::SceneHandle sceneHandle,
            [[maybe_unused]] AzPhysics::JointHandle jointHandle) override {}
        void SuppressCollisionEvents(
            [[maybe_unused]] AzPhysics::SceneHandle sceneHandle,
            [[maybe_unused]] const AzPhysics::SimulatedBodyHandle& bodyHandleA,
            [[maybe_unused]] const AzPhysics::SimulatedBodyHandle& bodyHandleB) override {}
        void UnsuppressCollisionEvents(
            [[maybe_unused]] AzPhysics::SceneHandle sceneHandle,
            [[maybe_unused]] const AzPhysics::SimulatedBodyHandle& bodyHandleA,
            [[maybe_unused]] const AzPhysics::SimulatedBodyHandle& bodyHandleB) override {}
        void SetGravity(
            [[maybe_unused]] AzPhysics::SceneHandle sceneHandle,
            [[maybe_unused]] const AZ::Vector3& gravity) override {}
        void RegisterSceneConfigurationChangedEventHandler(
            [[maybe_unused]] AzPhysics::SceneHandle sceneHandle,
            [[maybe_unused]] AzPhysics::SceneEvents::OnSceneConfigurationChanged::Handler& handler) override {}
        void RegisterSimulationBodyAddedHandler(
            [[maybe_unused]] AzPhysics::SceneHandle sceneHandle,
            [[maybe_unused]] AzPhysics::SceneEvents::OnSimulationBodyAdded::Handler& handler) override {}
        void RegisterSimulationBodyRemovedHandler(
            [[maybe_unused]] AzPhysics::SceneHandle sceneHandle,
            [[maybe_unused]] AzPhysics::SceneEvents::OnSimulationBodyRemoved::Handler& handler) override {}
        void RegisterSimulationBodySimulationEnabledHandler(
            [[maybe_unused]] AzPhysics::SceneHandle sceneHandle,
            [[maybe_unused]] AzPhysics::SceneEvents::OnSimulationBodySimulationEnabled::Handler& handler) override {}
        void RegisterSimulationBodySimulationDisabledHandler(
            [[maybe_unused]] AzPhysics::SceneHandle sceneHandle,
            [[maybe_unused]] AzPhysics::SceneEvents::OnSimulationBodySimulationDisabled::Handler& handler) override {}
        void RegisterSceneSimulationStartHandler(
            [[maybe_unused]] AzPhysics::SceneHandle sceneHandle,
            [[maybe_unused]] AzPhysics::SceneEvents::OnSceneSimulationStartHandler& handler) override {}
        void RegisterSceneActiveSimulatedBodiesHandler(
            [[maybe_unused]] AzPhysics::SceneHandle sceneHandle,
            [[maybe_unused]] AzPhysics::SceneEvents::OnSceneActiveSimulatedBodiesEvent::Handler& handler) override {}
        void RegisterSceneCollisionEventHandler(
            [[maybe_unused]] AzPhysics::SceneHandle sceneHandle,
            [[maybe_unused]] AzPhysics::SceneEvents::OnSceneCollisionsEvent::Handler& handler) override {}
        void RegisterSceneTriggersEventHandler(
            [[maybe_unused]] AzPhysics::SceneHandle sceneHandle,
            [[maybe_unused]] AzPhysics::SceneEvents::OnSceneTriggersEvent::Handler& handler) override {}
        void RegisterSceneGravityChangedEvent(
            [[maybe_unused]] AzPhysics::SceneHandle sceneHandle,
            [[maybe_unused]] AzPhysics::SceneEvents::OnSceneGravityChangedEvent::Handler& handler) override {}

        MOCK_METHOD1(GetSceneHandle, AzPhysics::SceneHandle(const AZStd::string& sceneName));
        MOCK_METHOD1(GetScene, AzPhysics::Scene*(AzPhysics::SceneHandle));
        MOCK_CONST_METHOD1(IsEnabled, bool(AzPhysics::SceneHandle sceneHandle));
        MOCK_METHOD2(AddSimulatedBody, AzPhysics::SimulatedBodyHandle(AzPhysics::SceneHandle sceneHandle, const AzPhysics::SimulatedBodyConfiguration* simulatedBodyConfig));
        MOCK_METHOD2(AddSimulatedBodies, AzPhysics::SimulatedBodyHandleList(AzPhysics::SceneHandle sceneHandle, const AzPhysics::SimulatedBodyConfigurationList& simulatedBodyConfigs));
        MOCK_METHOD2(GetSimulatedBodyFromHandle, AzPhysics::SimulatedBody* (AzPhysics::SceneHandle sceneHandle, AzPhysics::SimulatedBodyHandle bodyHandle));
        MOCK_METHOD2(GetSimulatedBodiesFromHandle, AzPhysics::SimulatedBodyList(AzPhysics::SceneHandle sceneHandle, const AzPhysics::SimulatedBodyHandleList& bodyHandles));
        MOCK_METHOD4(AddJoint, AzPhysics::JointHandle(AzPhysics::SceneHandle sceneHandle, const AzPhysics::JointConfiguration* jointConfig,
            AzPhysics::SimulatedBodyHandle parentBody, AzPhysics::SimulatedBodyHandle childBody));
        MOCK_METHOD2(
            GetJointFromHandle, AzPhysics::Joint*(AzPhysics::SceneHandle sceneHandle, AzPhysics::JointHandle jointHandle));
        MOCK_CONST_METHOD1(GetGravity, AZ::Vector3(AzPhysics::SceneHandle sceneHandle));
        MOCK_METHOD2(RegisterSceneSimulationFinishHandler, void(AzPhysics::SceneHandle sceneHandle, AzPhysics::SceneEvents::OnSceneSimulationFinishHandler& handler));
        MOCK_CONST_METHOD2(GetLegacyBody, AzPhysics::SimulatedBody* (AzPhysics::SceneHandle sceneHandle, AzPhysics::SimulatedBodyHandle handle));
        MOCK_METHOD2(QueryScene, AzPhysics::SceneQueryHits(AzPhysics::SceneHandle sceneHandle, const AzPhysics::SceneQueryRequest* request));
        MOCK_METHOD3(QueryScene, bool(AzPhysics::SceneHandle sceneHandle, const AzPhysics::SceneQueryRequest* request, AzPhysics::SceneQueryHits&));
        MOCK_METHOD2(QuerySceneBatch, AzPhysics::SceneQueryHitsList(AzPhysics::SceneHandle sceneHandle, const AzPhysics::SceneQueryRequests& requests));
        MOCK_METHOD4(QuerySceneAsync, bool(AzPhysics::SceneHandle sceneHandle, AzPhysics::SceneQuery::AsyncRequestId requestId,
            const AzPhysics::SceneQueryRequest* request, AzPhysics::SceneQuery::AsyncCallback callback));
        MOCK_METHOD4(QuerySceneAsyncBatch, bool(AzPhysics::SceneHandle sceneHandle, AzPhysics::SceneQuery::AsyncRequestId requestId,
            const AzPhysics::SceneQueryRequests& requests, AzPhysics::SceneQuery::AsyncBatchCallback callback));
    };

    class MockSimulatedBody
        : public AzPhysics::SimulatedBody
    {
    public:
        AZ_CLASS_ALLOCATOR(MockSimulatedBody, AZ::SystemAllocator)
        MOCK_CONST_METHOD0(GetEntityId, AZ::EntityId());
        MOCK_CONST_METHOD0(GetTransform, AZ::Transform());
        MOCK_METHOD1(SetTransform, void(const AZ::Transform& transform));
        MOCK_CONST_METHOD0(GetPosition, AZ::Vector3());
        MOCK_CONST_METHOD0(GetOrientation, AZ::Quaternion());
        MOCK_CONST_METHOD0(GetAabb, AZ::Aabb());
        MOCK_METHOD1(RayCast, AzPhysics::SceneQueryHit(const AzPhysics::RayCastRequest& request));
        MOCK_CONST_METHOD0(GetNativeType, AZ::Crc32());
        MOCK_CONST_METHOD0(GetNativePointer, void*());
    };

    class MockShape
        : public Physics::Shape
    {
    public:
        AZ_CLASS_ALLOCATOR(MockShape, AZ::SystemAllocator)
        MOCK_METHOD1(SetMaterial, void(const AZStd::shared_ptr<Physics::Material>& material));
        MOCK_CONST_METHOD0(GetMaterial, AZStd::shared_ptr<Physics::Material>());
        MOCK_CONST_METHOD0(GetMaterialId, Physics::MaterialId());
        MOCK_METHOD1(SetCollisionLayer, void(const AzPhysics::CollisionLayer& layer));
        MOCK_CONST_METHOD0(GetCollisionLayer, AzPhysics::CollisionLayer());
        MOCK_METHOD1(SetCollisionGroup, void(const AzPhysics::CollisionGroup& group));
        MOCK_CONST_METHOD0(GetCollisionGroup, AzPhysics::CollisionGroup());
        MOCK_METHOD1(SetName, void(const char* name));
        MOCK_METHOD2(SetLocalPose, void(const AZ::Vector3& offset, const AZ::Quaternion& rotation));
        MOCK_CONST_METHOD0(GetLocalPose, AZStd::pair<AZ::Vector3, AZ::Quaternion>());
        MOCK_METHOD0(GetNativePointer, void*());
        MOCK_CONST_METHOD0(GetNativePointer, const void*());
        MOCK_CONST_METHOD0(GetTag, AZ::Crc32());
        MOCK_METHOD1(AttachedToActor, void(void* actor));
        MOCK_METHOD0(DetachedFromActor, void());
        MOCK_METHOD2(RayCast, AzPhysics::SceneQueryHit(const AzPhysics::RayCastRequest& worldSpaceRequest, const AZ::Transform& worldTransform));
        MOCK_METHOD1(RayCastLocal, AzPhysics::SceneQueryHit(const AzPhysics::RayCastRequest& localSpaceRequest));
        MOCK_CONST_METHOD3(GetGeometry, void(AZStd::vector<AZ::Vector3>&, AZStd::vector<AZ::u32>&, const AZ::Aabb*));
        MOCK_CONST_METHOD1(GetAabb, AZ::Aabb(const AZ::Transform& worldTransform));
        MOCK_CONST_METHOD0(GetAabbLocal, AZ::Aabb());
        MOCK_CONST_METHOD0(GetRestOffset, float());
        MOCK_METHOD1(SetRestOffset, void(float));
        MOCK_CONST_METHOD0(GetContactOffset, float());
        MOCK_METHOD1(SetContactOffset, void(float));
    };

    class ScriptCanvasPhysicsTestEnvironment
        : public AZ::Test::GemTestEnvironment
    {
        void AddGemsAndComponents() override
        {
            AddComponentDescriptors({ AzFramework::TransformComponent::CreateDescriptor() });
        }
    };

    class ScriptCanvasPhysicsTest
        : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            ::testing::Test::SetUp();

            m_hit.m_position = AZ::Vector3(1.f, 2.f, 3.f);
            m_hit.m_distance = 2.5f;
            m_hit.m_normal = AZ::Vector3(-1.f, 3.5f, 0.5f);
            m_hit.m_shape = &m_shape;
            m_hit.m_physicsMaterialId = Physics::MaterialId::CreateName("Default");
            m_hit.m_resultFlags = AzPhysics::SceneQuery::ResultFlags::Position |
                AzPhysics::SceneQuery::ResultFlags::Distance |
                AzPhysics::SceneQuery::ResultFlags::Normal |
                AzPhysics::SceneQuery::ResultFlags::Shape |
                AzPhysics::SceneQuery::ResultFlags::Material;
            m_hitResult.m_hits.push_back(m_hit);
        }

        NiceMock<MockSimulatedBody> m_worldBody;
        NiceMock<MockShape> m_shape;
        NiceMock<MockPhysicsSceneInterface> m_sceneInterfaceMock;
        AzPhysics::SceneQueryHit m_hit;
        AzPhysics::SceneQueryHits m_hitResult;

        bool ResultIsEqualToHit(const ScriptCanvasPhysics::WorldFunctions::Result& result, const AzPhysics::SceneQueryHit& hit)
        {
            return
                AZStd::get<0>(result) == hit.IsValid() &&
                AZStd::get<1>(result) == hit.m_position &&
                AZStd::get<2>(result) == hit.m_normal &&
                AZStd::get<3>(result) == hit.m_distance &&
                AZStd::get<4>(result) == hit.m_entityId &&
                AZStd::get<5>(result) == AZ::Crc32(hit.m_physicsMaterialId.ToString<AZStd::string>())
                ;
        }
    };


    TEST_F(ScriptCanvasPhysicsTest, WorldNodes_RayCastWorldSpaceWithGroup_FT)
    {
        ON_CALL(m_sceneInterfaceMock, QueryScene(_, _))
            .WillByDefault(Return(m_hitResult));

        // given raycast data
        const AZ::Vector3 start = AZ::Vector3::CreateZero();
        const AZ::Vector3 direction = AZ::Vector3(0.f,1.f,0.f);
        const float distance = 1.f;
        const AZStd::string collisionGroup = "default";
        const AZ::EntityId ignoreEntityId;

        // when a raycast is performed
        auto result = ScriptCanvasPhysics::WorldFunctions::RayCastWorldSpaceWithGroup(
            start,
            direction,
            distance,
            collisionGroup,
            ignoreEntityId
        );

        // expect a valid hit is returned
        EXPECT_TRUE(ResultIsEqualToHit(result, m_hit));
    }

    TEST_F(ScriptCanvasPhysicsTest, WorldNodes_RayCastLocalSpaceWithGroup_FT)
    {
        ON_CALL(m_sceneInterfaceMock, QueryScene(_, _))
            .WillByDefault(Return(m_hitResult));

        // given raycast data
        const AZ::Vector3 direction = AZ::Vector3(0.f,1.f,0.f);
        const float distance = 1.f;
        const AZStd::string collisionGroup = "default";
        const AZ::EntityId ignoreEntityId;

        auto fromEntity = AZStd::make_unique<AZ::Entity>("Entity");
        fromEntity->CreateComponent<AzFramework::TransformComponent>()->SetWorldTM(AZ::Transform::Identity());
        fromEntity->Init();
        fromEntity->Activate();

        // when a raycast is performed
        auto result = ScriptCanvasPhysics::WorldFunctions::RayCastLocalSpaceWithGroup(
            fromEntity->GetId(),
            direction,
            distance,
            collisionGroup,
            fromEntity->GetId()
        );

        // expect a valid hit is returned
        EXPECT_TRUE(ResultIsEqualToHit(result, m_hit));
    }

    TEST_F(ScriptCanvasPhysicsTest, WorldNodes_RayCastMultipleLocalSpaceWithGroup_FT)
    {
        AZStd::vector<AzPhysics::SceneQueryHit> hits;
        hits.push_back(m_hit);

        ON_CALL(m_sceneInterfaceMock, QueryScene(_, _))
            .WillByDefault(Return(m_hitResult));

        // given raycast data
        const AZ::Vector3 direction = AZ::Vector3(0.f,1.f,0.f);
        const float distance = 1.f;
        const AZStd::string collisionGroup = "default";
        const AZ::EntityId ignoreEntityId;

        auto fromEntity = AZStd::make_unique<AZ::Entity>("Entity");
        fromEntity->CreateComponent<AzFramework::TransformComponent>()->SetWorldTM(AZ::Transform::Identity());
        fromEntity->Init();
        fromEntity->Activate();

        // when a raycast is performed
        auto results = ScriptCanvasPhysics::WorldFunctions::RayCastMultipleLocalSpaceWithGroup(
            fromEntity->GetId(),
            direction,
            distance,
            collisionGroup,
            fromEntity->GetId()
        );

        // expect valid hits are returned
        EXPECT_FALSE(results.empty());

        for (auto result : results)
        {
            EXPECT_EQ(result.m_distance, m_hit.m_distance);
            EXPECT_EQ(result.m_physicsMaterialId, m_hit.m_physicsMaterialId);
            EXPECT_EQ(result.m_normal, m_hit.m_normal);
            EXPECT_EQ(result.m_position, m_hit.m_position);
            EXPECT_EQ(result.m_shape, m_hit.m_shape);
        }
    }

    TEST_F(ScriptCanvasPhysicsTest, WorldNodes_ShapecastQuery_FT)
    {
        ON_CALL(m_sceneInterfaceMock, QueryScene(_, _))
            .WillByDefault(Return(m_hitResult));

        // given shapecast data
        const AZ::Vector3 direction = AZ::Vector3(0.f,1.f,0.f);
        const float distance = 1.f;
        const AZStd::string collisionGroup = "default";
        const AZ::EntityId ignoreEntityId;
        const AZ::Transform pose = AZ::Transform::CreateIdentity();

        // when a shapecast is performed
        auto result = ScriptCanvasPhysics::WorldFunctions::ShapecastQuery(
            distance,
            pose,
            direction,
            AZStd::make_shared<Physics::BoxShapeConfiguration>(),
            collisionGroup,
            ignoreEntityId
        );

        // expect a valid hit is returned
        EXPECT_TRUE(ResultIsEqualToHit(result, m_hit));
    }

}

AZ_UNIT_TEST_HOOK(new ScriptCanvasPhysicsTests::ScriptCanvasPhysicsTestEnvironment);
