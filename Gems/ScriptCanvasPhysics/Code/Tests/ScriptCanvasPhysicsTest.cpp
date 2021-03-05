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

#include "ScriptCanvasPhysics_precompiled.h"

#include <AzTest/GemTestEnvironment.h>
#include <gmock/gmock.h>
#include <AzFramework/Physics/Material.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzFramework/Physics/World.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Physics/Collision/CollisionGroups.h>
#include <AzFramework/Physics/Collision/CollisionLayers.h>
#include <WorldNodes.h>

namespace ScriptCanvasPhysics
{
    namespace WorldNodes
    {
        using Result = AZStd::tuple<
            bool /*true if an object was hit*/,
            AZ::Vector3 /*world space position*/,
            AZ::Vector3 /*surface normal*/,
            float /*distance to the hit*/,
            AZ::EntityId /*entity hit, if any*/,
            AZ::Crc32 /*tag of material surface hit, if any*/
        >;

        using OverlapResult = AZStd::tuple<
            bool, /*true if the overlap returned hits*/
            AZStd::vector<AZ::EntityId> /*list of entityIds*/
        >;

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

        extern AZStd::vector<Physics::RayCastHit> RayCastMultipleLocalSpaceWithGroup(const AZ::EntityId& fromEntityId,
            const AZ::Vector3& direction,
            float distance,
            const AZStd::string& collisionGroup,
            AZ::EntityId ignore);

        extern Result ShapecastQuery(float distance,
            const AZ::Transform& pose,
            const AZ::Vector3& direction,
            Physics::ShapeConfiguration& shape,
            const AZStd::string& collisionGroup,
            AZ::EntityId ignore);
    }
}

namespace ScriptCanvasPhysicsTests
{
    using namespace ::testing;

    class MockWorld
        : public Physics::WorldRequestBus::Handler
    {
    public:
        MockWorld()
        {
            Physics::WorldRequestBus::Handler::BusConnect(Physics::DefaultPhysicsWorldId);
        }
        ~MockWorld() override
        {
            Physics::WorldRequestBus::Handler::BusDisconnect();
        }

        MOCK_METHOD1(Update, void(float deltaTime));
        MOCK_METHOD1(StartSimulation, void(float deltaTime));
        MOCK_METHOD0(FinishSimulation, void());
        MOCK_METHOD1(RayCast, Physics::RayCastHit(const Physics::RayCastRequest& request));
        MOCK_METHOD1(RayCastMultiple, AZStd::vector<Physics::RayCastHit>(const Physics::RayCastRequest& request));
        MOCK_METHOD1(ShapeCast, Physics::RayCastHit(const Physics::ShapeCastRequest& request));
        MOCK_METHOD1(ShapeCastMultiple, AZStd::vector<Physics::RayCastHit>(const Physics::ShapeCastRequest& request));
        MOCK_METHOD1(Overlap, AZStd::vector<Physics::OverlapHit>(const Physics::OverlapRequest& request));
        MOCK_METHOD2(OverlapUnbounded, void(const Physics::OverlapRequest& request, const Physics::HitCallback<Physics::OverlapHit>& hitCallback));
        MOCK_METHOD2(RegisterSuppressedCollision, void(const Physics::WorldBody& body0, const Physics::WorldBody& body1));
        MOCK_METHOD2(UnregisterSuppressedCollision, void(const Physics::WorldBody& body0, const Physics::WorldBody& body1));
        MOCK_METHOD1(AddBody, void(Physics::WorldBody& body));
        MOCK_METHOD1(RemoveBody, void(Physics::WorldBody& body));
        MOCK_METHOD1(SetSimFunc, void(std::function<void(void*)> func));
        MOCK_METHOD1(SetEventHandler, void(Physics::WorldEventHandler* eventHandler));
        MOCK_CONST_METHOD0(GetGravity, AZ::Vector3());
        MOCK_METHOD1(SetGravity, void(const AZ::Vector3& gravity));
        MOCK_METHOD1(SetMaxDeltaTime, void(float maxDeltaTime));
        MOCK_METHOD1(SetFixedDeltaTime, void(float fixedDeltaTime));
        MOCK_METHOD1(DeferDelete, void(AZStd::unique_ptr<Physics::WorldBody> worldBody));
        MOCK_METHOD1(SetTriggerEventCallback, void(Physics::ITriggerEventCallback* triggerCallback));
        MOCK_CONST_METHOD0(GetWorldId, AZ::Crc32());
    };

    class MockWorldBody
        : public Physics::WorldBody
    {
    public:
        MOCK_CONST_METHOD0(GetEntityId, AZ::EntityId());
        MOCK_CONST_METHOD0(GetWorld, Physics::World*());
        MOCK_CONST_METHOD0(GetTransform, AZ::Transform());
        MOCK_METHOD1(SetTransform, void(const AZ::Transform& transform));
        MOCK_CONST_METHOD0(GetPosition, AZ::Vector3());
        MOCK_CONST_METHOD0(GetOrientation, AZ::Quaternion());
        MOCK_CONST_METHOD0(GetAabb, AZ::Aabb());
        MOCK_METHOD1(RayCast, Physics::RayCastHit(const Physics::RayCastRequest& request));
        MOCK_CONST_METHOD0(GetNativeType, AZ::Crc32());
        MOCK_CONST_METHOD0(GetNativePointer, void*());
        MOCK_METHOD1(AddToWorld, void(Physics::World&));
        MOCK_METHOD1(RemoveFromWorld, void(Physics::World&));
    };

    class MockShape        
        : public Physics::Shape
    {
    public:
        MOCK_METHOD1(SetMaterial, void(const AZStd::shared_ptr<Physics::Material>& material));
        MOCK_CONST_METHOD0(GetMaterial, AZStd::shared_ptr<Physics::Material>());
        MOCK_METHOD1(SetCollisionLayer, void(const AzPhysics::CollisionLayer& layer));
        MOCK_CONST_METHOD0(GetCollisionLayer, AzPhysics::CollisionLayer());
        MOCK_METHOD1(SetCollisionGroup, void(const AzPhysics::CollisionGroup& group));
        MOCK_CONST_METHOD0(GetCollisionGroup, AzPhysics::CollisionGroup());
        MOCK_METHOD1(SetName, void(const char* name));
        MOCK_METHOD2(SetLocalPose, void(const AZ::Vector3& offset, const AZ::Quaternion& rotation));
        MOCK_CONST_METHOD0(GetLocalPose, AZStd::pair<AZ::Vector3, AZ::Quaternion>());
        MOCK_METHOD0(GetNativePointer, void*());
        MOCK_CONST_METHOD0(GetTag, AZ::Crc32());
        MOCK_METHOD1(AttachedToActor, void(void* actor));
        MOCK_METHOD0(DetachedFromActor, void());
        MOCK_METHOD2(RayCast, Physics::RayCastHit(const Physics::RayCastRequest& worldSpaceRequest, const AZ::Transform& worldTransform));
        MOCK_METHOD1(RayCastLocal, Physics::RayCastHit(const Physics::RayCastRequest& localSpaceRequest));
        MOCK_METHOD3(GetGeometry, void(AZStd::vector<AZ::Vector3>&, AZStd::vector<AZ::u32>&, AZ::Aabb*));
        MOCK_CONST_METHOD1(GetAabb, AZ::Aabb(const AZ::Transform& worldTransform));
        MOCK_CONST_METHOD0(GetAabbLocal, AZ::Aabb());
        MOCK_CONST_METHOD0(GetRestOffset, float());
        MOCK_METHOD1(SetRestOffset, void(float));
        MOCK_CONST_METHOD0(GetContactOffset, float());
        MOCK_METHOD1(SetContactOffset, void(float));
    };

    class MockPhysicsMaterial        
        : public Physics::Material
    {
    public:
        MOCK_CONST_METHOD0(GetSurfaceType, AZ::Crc32());
        MOCK_METHOD1(SetSurfaceType, void(AZ::Crc32));
        MOCK_CONST_METHOD0(GetSurfaceTypeName, const AZStd::string&());
        MOCK_CONST_METHOD0(GetDynamicFriction, float());
        MOCK_METHOD1(SetDynamicFriction, void(float));
        MOCK_CONST_METHOD0(GetStaticFriction, float());
        MOCK_METHOD1(SetStaticFriction, void(float));
        MOCK_CONST_METHOD0(GetRestitution, float());
        MOCK_METHOD1(SetRestitution, void(float));
        MOCK_CONST_METHOD0(GetFrictionCombineMode, CombineMode());
        MOCK_METHOD1(SetFrictionCombineMode, void(CombineMode));
        MOCK_CONST_METHOD0(GetRestitutionCombineMode, CombineMode());
        MOCK_METHOD1(SetRestitutionCombineMode, void(CombineMode));
        MOCK_CONST_METHOD0(GetCryEngineSurfaceId, AZ::u32());
        MOCK_METHOD0(GetNativePointer, void*());
        MOCK_CONST_METHOD0(GetDensity, float());
        MOCK_METHOD1(SetDensity, void(float));
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

            ON_CALL(m_material, GetSurfaceType())
                .WillByDefault(Return(AZ::Crc32("CustomSurface")));

            m_hit.m_body = &m_worldBody;
            m_hit.m_position = AZ::Vector3(1.f, 2.f, 3.f);
            m_hit.m_distance = 2.5f;
            m_hit.m_normal = AZ::Vector3(-1.f, 3.5f, 0.5f);
            m_hit.m_shape = &m_shape;
            m_hit.m_material = &m_material;
        }

        NiceMock<MockWorldBody> m_worldBody;
        NiceMock<MockShape> m_shape;
        NiceMock<MockPhysicsMaterial> m_material;
        NiceMock<MockWorld> m_worldMock;
        Physics::RayCastHit m_hit;

        bool ResultIsEqualToHit(const ScriptCanvasPhysics::WorldNodes::Result& result, const Physics::RayCastHit& hit)
        {
            return
                AZStd::get<0>(result) == (hit.m_body != nullptr) &&
                AZStd::get<1>(result) == hit.m_position &&
                AZStd::get<2>(result) == hit.m_normal &&
                AZStd::get<3>(result) == hit.m_distance &&
                AZStd::get<4>(result) == (hit.m_body ? hit.m_body->GetEntityId() : AZ::EntityId()) &&
                AZStd::get<5>(result) == (hit.m_material ? hit.m_material->GetSurfaceType() : AZ::Crc32())
                ;
        }
    };


    TEST_F(ScriptCanvasPhysicsTest, WorldNodes_RayCastWorldSpaceWithGroup_FT)
    {
        ON_CALL(m_worldMock, RayCast(_))
            .WillByDefault(Return(m_hit));

        // given raycast data
        const AZ::Vector3 start = AZ::Vector3::CreateZero();
        const AZ::Vector3 direction = AZ::Vector3(0.f,1.f,0.f);
        const float distance = 1.f;
        const AZStd::string collisionGroup = "default";
        const AZ::EntityId ignoreEntityId;

        // when a raycast is performed
        auto result = ScriptCanvasPhysics::WorldNodes::RayCastWorldSpaceWithGroup(
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
        ON_CALL(m_worldMock, RayCast(_))
            .WillByDefault(Return(m_hit));

        // given raycast data
        const AZ::Vector3 start = AZ::Vector3::CreateZero();
        const AZ::Vector3 direction = AZ::Vector3(0.f,1.f,0.f);
        const float distance = 1.f;
        const AZStd::string collisionGroup = "default";
        const AZ::EntityId ignoreEntityId;

        auto fromEntity = AZStd::make_unique<AZ::Entity>("Entity");
        fromEntity->CreateComponent<AzFramework::TransformComponent>()->SetWorldTM(AZ::Transform::Identity());
        fromEntity->Init();
        fromEntity->Activate();

        // when a raycast is performed
        auto result = ScriptCanvasPhysics::WorldNodes::RayCastLocalSpaceWithGroup(
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
        AZStd::vector<Physics::RayCastHit> hits;
        hits.push_back(m_hit);

        ON_CALL(m_worldMock, RayCastMultiple(_))
            .WillByDefault(Return(hits));

        // given raycast data
        const AZ::Vector3 start = AZ::Vector3::CreateZero();
        const AZ::Vector3 direction = AZ::Vector3(0.f,1.f,0.f);
        const float distance = 1.f;
        const AZStd::string collisionGroup = "default";
        const AZ::EntityId ignoreEntityId;

        auto fromEntity = AZStd::make_unique<AZ::Entity>("Entity");
        fromEntity->CreateComponent<AzFramework::TransformComponent>()->SetWorldTM(AZ::Transform::Identity());
        fromEntity->Init();
        fromEntity->Activate();

        // when a raycast is performed
        auto results = ScriptCanvasPhysics::WorldNodes::RayCastMultipleLocalSpaceWithGroup(
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
            EXPECT_EQ(result.m_body, m_hit.m_body);
            EXPECT_EQ(result.m_distance, m_hit.m_distance);
            EXPECT_EQ(result.m_material, m_hit.m_material);
            EXPECT_EQ(result.m_normal, m_hit.m_normal);
            EXPECT_EQ(result.m_position, m_hit.m_position);
            EXPECT_EQ(result.m_shape, m_hit.m_shape);
        }
    }

    TEST_F(ScriptCanvasPhysicsTest, WorldNodes_ShapecastQuery_FT)
    {
        ON_CALL(m_worldMock, ShapeCast(_))
            .WillByDefault(Return(m_hit));

        // given shapecast data
        const AZ::Vector3 start = AZ::Vector3::CreateZero();
        const AZ::Vector3 direction = AZ::Vector3(0.f,1.f,0.f);
        const float distance = 1.f;
        const AZStd::string collisionGroup = "default";
        const AZ::EntityId ignoreEntityId;
        const AZ::Transform pose = AZ::Transform::CreateIdentity();
        Physics::BoxShapeConfiguration boxShapeConfiguration = Physics::BoxShapeConfiguration();

        // when a shapecast is performed
        auto result = ScriptCanvasPhysics::WorldNodes::ShapecastQuery(
            distance,
            pose,
            direction,
            boxShapeConfiguration,
            collisionGroup,
            ignoreEntityId
        );

        // expect a valid hit is returned
        EXPECT_TRUE(ResultIsEqualToHit(result, m_hit));
    }

}

AZ_UNIT_TEST_HOOK(new ScriptCanvasPhysicsTests::ScriptCanvasPhysicsTestEnvironment);
