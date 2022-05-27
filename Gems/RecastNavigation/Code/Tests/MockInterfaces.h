/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/Entity.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/Shape.h>
#include <AzTest/AzTest.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <RecastNavigation/RecastNavigationMeshBus.h>
#include <AzCore/Time/ITime.h>
#include <AzCore/Component/TransformBus.h>

namespace RecastNavigationTests
{
    using namespace AZ;
    using namespace AZStd;
    using namespace testing;

    class MockShapeComponent
        : public AZ::Component
        , public LmbrCentral::ShapeComponentRequestsBus::Handler
    {
    public:
        AZ_COMPONENT(MockShapeComponent,
            "{A9406916-365D-4C72-9F4C-2A3E5220CE2B}");

        static void Reflect(ReflectContext*) {}

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("BoxShapeService"));
            provided.push_back(AZ_CRC_CE("AxisAlignedBoxShapeService"));
        }

        void Activate() override
        {
            LmbrCentral::ShapeComponentRequestsBus::Handler::BusConnect(GetEntityId());
        }

        void Deactivate() override
        {
            LmbrCentral::ShapeComponentRequestsBus::Handler::BusDisconnect();
        }

        AZ::Aabb GetEncompassingAabb() override
        {
            return AZ::Aabb::CreateCenterHalfExtents(AZ::Vector3::CreateZero(), AZ::Vector3::CreateOne() * 10);
        }

        MOCK_METHOD0(GetShapeType, AZ::Crc32());
        MOCK_METHOD2(GetTransformAndLocalBounds, void(AZ::Transform&, AZ::Aabb&));
        MOCK_METHOD1(IsPointInside, bool(const AZ::Vector3&));
        MOCK_METHOD1(DistanceSquaredFromPoint, float(const AZ::Vector3&));
    };

    class MockPhysicsShape : public Physics::Shape
    {
    public:
        MOCK_METHOD1(AttachedToActor, void (void*));
        MOCK_METHOD0(DetachedFromActor, void ());
        MOCK_CONST_METHOD1(GetAabb, AZ::Aabb (const AZ::Transform&));
        MOCK_CONST_METHOD0(GetAabbLocal, AZ::Aabb ());
        MOCK_CONST_METHOD0(GetCollisionGroup, AzPhysics::CollisionGroup ());
        MOCK_CONST_METHOD0(GetCollisionLayer, AzPhysics::CollisionLayer ());
        MOCK_CONST_METHOD0(GetContactOffset, float ());
        MOCK_METHOD3(GetGeometry, void (AZStd::vector<AZ::Vector3>&, AZStd::vector<AZ::u32>&, AZ::Aabb*));
        MOCK_CONST_METHOD0(GetLocalPose, AZStd::pair<AZ::Vector3, AZ::Quaternion> ());
        MOCK_CONST_METHOD0(GetMaterial, AZStd::shared_ptr<Physics::Material> ());
        MOCK_METHOD0(GetNativePointer, void* ());
        MOCK_CONST_METHOD0(GetRestOffset, float ());
        MOCK_CONST_METHOD0(GetTag, AZ::Crc32 ());
        MOCK_METHOD2(RayCast, AzPhysics::SceneQueryHit (const AzPhysics::RayCastRequest&, const AZ::Transform&));
        MOCK_METHOD1(RayCastLocal, AzPhysics::SceneQueryHit (const AzPhysics::RayCastRequest&));
        MOCK_METHOD1(SetCollisionGroup, void (const AzPhysics::CollisionGroup&));
        MOCK_METHOD1(SetCollisionLayer, void (const AzPhysics::CollisionLayer&));
        MOCK_METHOD1(SetContactOffset, void (float));
        MOCK_METHOD2(SetLocalPose, void (const AZ::Vector3&, const AZ::Quaternion&));
        MOCK_METHOD1(SetMaterial, void (const AZStd::shared_ptr<Physics::Material>&));
        MOCK_METHOD1(SetName, void (const char*));
        MOCK_METHOD1(SetRestOffset, void (float));
    };

    class MockSimulatedBody : public AzPhysics::SimulatedBody
    {
    public:
        MOCK_CONST_METHOD0(GetAabb, AZ::Aabb ());
        MOCK_CONST_METHOD0(GetEntityId, AZ::EntityId ());
        MOCK_CONST_METHOD0(GetNativePointer, void* ());
        MOCK_CONST_METHOD0(GetNativeType, AZ::Crc32 ());
        MOCK_CONST_METHOD0(GetOrientation, AZ::Quaternion ());
        MOCK_CONST_METHOD0(GetPosition, AZ::Vector3 ());
        MOCK_METHOD0(GetScene, AzPhysics::Scene* ());
        MOCK_CONST_METHOD0(GetTransform, AZ::Transform ());
        MOCK_METHOD1(RayCast, AzPhysics::SceneQueryHit (const AzPhysics::RayCastRequest&));
        MOCK_METHOD1(SetTransform, void (const AZ::Transform&));
    };

    class MockSceneInterface : public Interface<AzPhysics::SceneInterface>::Registrar
    {
    public:
        MockSceneInterface() = default;
        ~MockSceneInterface() override = default;

        MOCK_METHOD4(AddJoint, AzPhysics::JointHandle(AzPhysics::SceneHandle, const AzPhysics::JointConfiguration*, AzPhysics::
            SimulatedBodyHandle, AzPhysics::SimulatedBodyHandle));
        MOCK_METHOD2(AddSimulatedBodies, AzPhysics::SimulatedBodyHandleList(AzPhysics::SceneHandle, const AzPhysics::
            SimulatedBodyConfigurationList&));
        MOCK_METHOD2(AddSimulatedBody, AzPhysics::SimulatedBodyHandle(AzPhysics::SceneHandle, const AzPhysics::SimulatedBodyConfiguration*)
        );
        MOCK_METHOD2(DisableSimulationOfBody, void(AzPhysics::SceneHandle, AzPhysics::SimulatedBodyHandle));
        MOCK_METHOD2(EnableSimulationOfBody, void(AzPhysics::SceneHandle, AzPhysics::SimulatedBodyHandle));
        MOCK_METHOD1(FinishSimulation, void(AzPhysics::SceneHandle));
        MOCK_CONST_METHOD1(GetGravity, AZ::Vector3(AzPhysics::SceneHandle));
        MOCK_METHOD2(GetJointFromHandle, AzPhysics::Joint* (AzPhysics::SceneHandle, AzPhysics::JointHandle));
        MOCK_METHOD1(GetSceneHandle, AzPhysics::SceneHandle(const AZStd::string&));
        MOCK_METHOD2(GetSimulatedBodiesFromHandle, AzPhysics::SimulatedBodyList(AzPhysics::SceneHandle, const AzPhysics::
            SimulatedBodyHandleList&));
        MOCK_METHOD2(GetSimulatedBodyFromHandle, AzPhysics::SimulatedBody* (AzPhysics::SceneHandle, AzPhysics::SimulatedBodyHandle));
        MOCK_CONST_METHOD1(IsEnabled, bool(AzPhysics::SceneHandle));
        MOCK_METHOD2(QueryScene, AzPhysics::SceneQueryHits(AzPhysics::SceneHandle, const AzPhysics::SceneQueryRequest*));
        MOCK_METHOD4(QuerySceneAsync, bool(AzPhysics::SceneHandle, AzPhysics::SceneQuery::AsyncRequestId, const AzPhysics::
            SceneQueryRequest*, AzPhysics::SceneQuery::AsyncCallback));
        MOCK_METHOD4(QuerySceneAsyncBatch, bool(AzPhysics::SceneHandle, AzPhysics::SceneQuery::AsyncRequestId, const AzPhysics::
            SceneQueryRequests&, AzPhysics::SceneQuery::AsyncBatchCallback));
        MOCK_METHOD2(QuerySceneBatch, AzPhysics::SceneQueryHitsList(AzPhysics::SceneHandle, const AzPhysics::SceneQueryRequests&));
        MOCK_METHOD2(RegisterSceneActiveSimulatedBodiesHandler, void(AzPhysics::SceneHandle, Event<tuple<Crc32, signed char>, const vector<
            tuple<Crc32, int>>&>::Handler&));
        MOCK_METHOD2(RegisterSceneCollisionEventHandler, void(AzPhysics::SceneHandle, Event<tuple<Crc32, signed char>, const vector<
            AzPhysics::CollisionEvent>&>::Handler&));
        MOCK_METHOD2(RegisterSceneConfigurationChangedEventHandler, void(AzPhysics::SceneHandle, Event<tuple<Crc32, signed char>, const
            AzPhysics::SceneConfiguration&>::Handler&));
        MOCK_METHOD2(RegisterSceneGravityChangedEvent, void(AzPhysics::SceneHandle, Event<tuple<Crc32, signed char>, const Vector3&>::
            Handler&));
        MOCK_METHOD2(RegisterSceneSimulationFinishHandler, void(AzPhysics::SceneHandle, AzPhysics::SceneEvents::
            OnSceneSimulationFinishHandler&));
        MOCK_METHOD2(RegisterSceneSimulationStartHandler, void(AzPhysics::SceneHandle, AzPhysics::SceneEvents::
            OnSceneSimulationStartHandler&));
        MOCK_METHOD2(RegisterSceneTriggersEventHandler, void(AzPhysics::SceneHandle, Event<tuple<Crc32, signed char>, const vector<
            AzPhysics::TriggerEvent>&>::Handler&));
        MOCK_METHOD2(RegisterSimulationBodyAddedHandler, void(AzPhysics::SceneHandle, Event<tuple<Crc32, signed char>, tuple<Crc32, int>>::
            Handler&));
        MOCK_METHOD2(RegisterSimulationBodyRemovedHandler, void(AzPhysics::SceneHandle, Event<tuple<Crc32, signed char>, tuple<Crc32, int>>
            ::Handler&));
        MOCK_METHOD2(RegisterSimulationBodySimulationDisabledHandler, void(AzPhysics::SceneHandle, Event<tuple<Crc32, signed char>, tuple<
            Crc32, int>>::Handler&));
        MOCK_METHOD2(RegisterSimulationBodySimulationEnabledHandler, void(AzPhysics::SceneHandle, Event<tuple<Crc32, signed char>, tuple<
            Crc32, int>>::Handler&));
        MOCK_METHOD2(RemoveJoint, void(AzPhysics::SceneHandle, AzPhysics::JointHandle));
        MOCK_METHOD2(RemoveSimulatedBodies, void(AzPhysics::SceneHandle, AzPhysics::SimulatedBodyHandleList&));
        MOCK_METHOD2(RemoveSimulatedBody, void(AzPhysics::SceneHandle, AzPhysics::SimulatedBodyHandle&));
        MOCK_METHOD2(SetEnabled, void(AzPhysics::SceneHandle, bool));
        MOCK_METHOD2(SetGravity, void(AzPhysics::SceneHandle, const AZ::Vector3&));
        MOCK_METHOD2(StartSimulation, void(AzPhysics::SceneHandle, float));
        MOCK_METHOD3(SuppressCollisionEvents, void(AzPhysics::SceneHandle, const AzPhysics::SimulatedBodyHandle&, const AzPhysics::
            SimulatedBodyHandle&));
        MOCK_METHOD3(UnsuppressCollisionEvents, void(AzPhysics::SceneHandle, const AzPhysics::SimulatedBodyHandle&, const AzPhysics::
            SimulatedBodyHandle&));
    };

    class MockDebug : public AzFramework::DebugDisplayRequestBus::Handler
    {
    public:
        MockDebug()
        {
            AzFramework::DebugDisplayRequestBus::Handler::BusConnect(AzFramework::g_defaultSceneEntityDebugDisplayId);
        }

        ~MockDebug() override
        {
            AzFramework::DebugDisplayRequestBus::Handler::BusDisconnect();
        }
    };

    struct Wait : public RecastNavigation::RecastNavigationMeshNotificationBus::Handler
    {
        Wait(AZ::EntityId id)
        {
            RecastNavigation::RecastNavigationMeshNotificationBus::Handler::BusConnect(id);
        }

        void OnNavigationMeshUpdated(AZ::EntityId) override
        {
            m_calls++;
        }

        int m_calls = 0;

        void Reset()
        {
            m_calls = 0;
        }

        void BlockUntilCalled(AZ::TimeMs timeout = AZ::TimeMs{ 100 }) const
        {
            const AZ::TimeMs timeStep{ 5 };
            AZ::TimeMs current{ 0 };
            while (current < timeout && m_calls == 0)
            {
                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(static_cast<int>(timeStep)));
                current += timeStep;                
            }
        }
    };

    struct MockTransforms : AZ::TransformBus::MultiHandler
    {
        explicit MockTransforms(const AZStd::vector<AZ::EntityId>& entities)
        {
            for (AZ::EntityId id : entities)
            {
                AZ::TransformBus::MultiHandler::BusConnect(id);
            }
        }

        ~MockTransforms() override
        {
            AZ::TransformBus::MultiHandler::BusDisconnect();
        }

        void BindTransformChangedEventHandler(TransformChangedEvent::Handler&) override {}
        void BindParentChangedEventHandler(ParentChangedEvent::Handler&) override {}
        void BindChildChangedEventHandler(ChildChangedEvent::Handler&) override {}
        void NotifyChildChangedEvent(ChildChangeType, EntityId) override {}

        MOCK_METHOD0(GetLocalTM, const Transform& ());
        MOCK_METHOD0(GetWorldTM, const Transform& ());
        MOCK_METHOD0(IsStaticTransform, bool());
    };
}
