
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Time/ITime.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <AzTest/AzTest.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <RecastNavigation/RecastNavigationMeshBus.h>

namespace RecastNavigationTests
{
    class MockShapeComponent
        : public AZ::Component
        , public LmbrCentral::ShapeComponentRequestsBus::Handler
    {
    public:
        AZ_COMPONENT(MockShapeComponent,
            "{A9406916-365D-4C72-9F4C-2A3E5220CE2B}");

        static void Reflect(AZ::ReflectContext*) {}

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
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

        AZ::Aabb GetEncompassingAabb() const override
        {
            return AZ::Aabb::CreateCenterHalfExtents(AZ::Vector3::CreateZero(), AZ::Vector3::CreateOne() * 10);
        }

        MOCK_CONST_METHOD0(GetShapeType, AZ::Crc32());
        MOCK_CONST_METHOD2(GetTransformAndLocalBounds, void(AZ::Transform&, AZ::Aabb&));
        MOCK_CONST_METHOD1(IsPointInside, bool(const AZ::Vector3&));
        MOCK_CONST_METHOD1(DistanceSquaredFromPoint, float(const AZ::Vector3&));
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
        explicit Wait(AZ::EntityId id)
        {
            RecastNavigation::RecastNavigationMeshNotificationBus::Handler::BusConnect(id);
        }

        ~Wait() override
        {
            RecastNavigation::RecastNavigationMeshNotificationBus::Handler::BusDisconnect();
        }

        void OnNavigationMeshUpdated(AZ::EntityId) override
        {
            m_updatedCalls++;
        }

        int m_updatedCalls = 0;

        void OnNavigationMeshBeganRecalculating(AZ::EntityId) override
        {
            m_recalculatingCalls++;
        }

        int m_recalculatingCalls = 0;

        void Reset()
        {
            m_updatedCalls = 0;
            m_recalculatingCalls = 0;
        }

        void BlockUntilNavigationMeshRecalculating(AZ::TimeMs timeout = AZ::TimeMs{ 2000 }) const
        {
            const AZ::TimeMs timeStep{ 5 };
            AZ::TimeMs current{ 0 };
            while (current < timeout && m_recalculatingCalls == 0)
            {
                // Nav mesh notifications occurs on main threads via ticks.
                AZ::TickBus::Broadcast(&AZ::TickBus::Events::OnTick, 0.1f, AZ::ScriptTimePoint{});

                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(static_cast<int>(timeStep)));
                current += timeStep;
            }
        }

        void BlockUntilCalled(AZ::TimeMs timeout = AZ::TimeMs{ 2000 }) const
        {
            const AZ::TimeMs timeStep{ 5 };
            AZ::TimeMs current{ 0 };
            while (current < timeout && m_updatedCalls == 0)
            {
                // Nav mesh notifications occurs on main threads via ticks.
                AZ::TickBus::Broadcast(&AZ::TickBus::Events::OnTick, 0.1f, AZ::ScriptTimePoint{});

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

        void BindTransformChangedEventHandler(AZ::TransformChangedEvent::Handler&) override {}
        void BindParentChangedEventHandler(AZ::ParentChangedEvent::Handler&) override {}
        void BindChildChangedEventHandler(AZ::ChildChangedEvent::Handler&) override {}
        void NotifyChildChangedEvent(AZ::ChildChangeType, AZ::EntityId) override {}

        MOCK_METHOD0(GetLocalTM, const AZ::Transform& ());
        MOCK_METHOD0(GetWorldTM, const AZ::Transform& ());
        MOCK_METHOD0(IsStaticTransform, bool());
    };
}
