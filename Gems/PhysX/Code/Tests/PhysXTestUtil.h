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

#pragma once

#include "AzCore/Component/Component.h"
#include <AzFramework/Physics/CollisionNotificationBus.h>
#include <AzFramework/Physics/TriggerBus.h>
#include <AzFramework/Terrain/TerrainDataRequestBus.h>

namespace PhysX
{
    //! CollisionCallbacksListener listens to collision events for a particular entity id.
    class CollisionCallbacksListener
        : public Physics::CollisionNotificationBus::Handler
    {
    public:
        CollisionCallbacksListener(AZ::EntityId entityId);
        ~CollisionCallbacksListener();

        void OnCollisionBegin(const Physics::CollisionEvent& collision) override;
        void OnCollisionPersist(const Physics::CollisionEvent& collision) override;
        void OnCollisionEnd(const Physics::CollisionEvent& collision) override;

        AZStd::vector<Physics::CollisionEvent> m_beginCollisions;
        AZStd::vector<Physics::CollisionEvent> m_persistCollisions;
        AZStd::vector<Physics::CollisionEvent> m_endCollisions;

        AZStd::function<void(const Physics::CollisionEvent& collisionEvent)> m_onCollisionBegin;
        AZStd::function<void(const Physics::CollisionEvent& collisionEvent)> m_onCollisionPersist;
        AZStd::function<void(const Physics::CollisionEvent& collisionEvent)> m_onCollisionEnd;
    };

    //! TestTriggerAreaNotificationListener listens to trigger events for a particular entity id.
    class TestTriggerAreaNotificationListener
        : protected Physics::TriggerNotificationBus::Handler
    {
    public:
        TestTriggerAreaNotificationListener(AZ::EntityId triggerAreaEntityId);
        ~TestTriggerAreaNotificationListener();

        void OnTriggerEnter(const Physics::TriggerEvent& event) override;
        void OnTriggerExit(const Physics::TriggerEvent& event) override;

        const AZStd::vector<Physics::TriggerEvent>& GetEnteredEvents() const { return m_enteredEvents; }
        const AZStd::vector<Physics::TriggerEvent>& GetExitedEvents() const { return m_exitedEvents; }

        AZStd::function<void(const Physics::TriggerEvent& event)> m_onTriggerEnter;
        AZStd::function<void(const Physics::TriggerEvent& event)> m_onTriggerExit;

    private:
        AZStd::vector<Physics::TriggerEvent> m_enteredEvents;
        AZStd::vector<Physics::TriggerEvent> m_exitedEvents;
    };


    //! Dummy component emulating presence of terrain by connecting to TerrainDataRequestBus
    //! PhysX Terrain Component skips activation if there's no terrain present,
    //! so in order to test it we also add the DummyTestTerrainComponent.
    class DummyTestTerrainComponent
        : public AZ::Component
        , private AzFramework::Terrain::TerrainDataRequestBus::Handler
    {
    public:
        AZ_COMPONENT(DummyTestTerrainComponent, "{EE4ECA23-9C27-4D5D-9C6F-271A19C0333E}");
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("TerrainService"));
        }

    private:
        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override
        {
            AzFramework::Terrain::TerrainDataRequestBus::Handler::BusConnect();
        }
        void Deactivate() override
        {
            AzFramework::Terrain::TerrainDataRequestBus::Handler::BusDisconnect();
        }
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // TerrainDataRequestBus interface dummy implementation
        AZ::Vector2 GetTerrainGridResolution() const override { return {}; }
        AZ::Aabb GetTerrainAabb() const override { return {}; }
        float GetHeight(AZ::Vector3, Sampler, bool*) const override { return {}; }
        float GetHeightFromFloats(float, float, Sampler, bool*) const override { return {}; }
        AzFramework::SurfaceData::SurfaceTagWeight GetMaxSurfaceWeight(AZ::Vector3, Sampler, bool*) const override { return {}; }
        AzFramework::SurfaceData::SurfaceTagWeight GetMaxSurfaceWeightFromFloats(float, float, Sampler, bool*) const override { return {}; }
        const char* GetMaxSurfaceName(AZ::Vector3, Sampler, bool*) const override { return {}; }
        bool GetIsHoleFromFloats(float, float, Sampler) const override { return {}; }
        AZ::Vector3 GetNormal(AZ::Vector3, Sampler, bool*) const override { return {}; }
        AZ::Vector3 GetNormalFromFloats(float, float, Sampler, bool*) const override { return {}; }
        ////////////////////////////////////////////////////////////////////////
    };

} // namespace PhysX
