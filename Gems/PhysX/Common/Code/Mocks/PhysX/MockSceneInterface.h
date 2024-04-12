/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Interface/Interface.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <gmock/gmock.h>

namespace UnitTest
{
    class MockSceneInterface : public AZ::Interface<AzPhysics::SceneInterface>::Registrar
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
        MOCK_METHOD1(GetScene, AzPhysics::Scene*(AzPhysics::SceneHandle));
        MOCK_METHOD2(GetSimulatedBodiesFromHandle, AzPhysics::SimulatedBodyList(AzPhysics::SceneHandle, const AzPhysics::
            SimulatedBodyHandleList&));
        MOCK_METHOD2(GetSimulatedBodyFromHandle, AzPhysics::SimulatedBody* (AzPhysics::SceneHandle, AzPhysics::SimulatedBodyHandle));
        MOCK_CONST_METHOD1(IsEnabled, bool(AzPhysics::SceneHandle));
        MOCK_METHOD2(QueryScene, AzPhysics::SceneQueryHits(AzPhysics::SceneHandle, const AzPhysics::SceneQueryRequest*));
        MOCK_METHOD3(QueryScene, bool(AzPhysics::SceneHandle sceneHandle, const AzPhysics::SceneQueryRequest* request, AzPhysics::SceneQueryHits&));
        MOCK_METHOD4(QuerySceneAsync, bool(AzPhysics::SceneHandle, AzPhysics::SceneQuery::AsyncRequestId, const AzPhysics::
            SceneQueryRequest*, AzPhysics::SceneQuery::AsyncCallback));
        MOCK_METHOD4(QuerySceneAsyncBatch, bool(AzPhysics::SceneHandle, AzPhysics::SceneQuery::AsyncRequestId, const AzPhysics::
            SceneQueryRequests&, AzPhysics::SceneQuery::AsyncBatchCallback));
        MOCK_METHOD2(QuerySceneBatch, AzPhysics::SceneQueryHitsList(AzPhysics::SceneHandle, const AzPhysics::SceneQueryRequests&));
        MOCK_METHOD2(RegisterSceneActiveSimulatedBodiesHandler, void(AzPhysics::SceneHandle, AZ::Event<AZStd::tuple<AZ::Crc32, signed char>, const AZStd::vector<
            AZStd::tuple<AZ::Crc32, int>>&, float>::Handler&));
        MOCK_METHOD2(RegisterSceneCollisionEventHandler, void(AzPhysics::SceneHandle, AZ::Event<AZStd::tuple<AZ::Crc32, signed char>, const AZStd::vector<
            AzPhysics::CollisionEvent>&>::Handler&));
        MOCK_METHOD2(RegisterSceneConfigurationChangedEventHandler, void(AzPhysics::SceneHandle, AZ::Event<AZStd::tuple<AZ::Crc32, signed char>, const
            AzPhysics::SceneConfiguration&>::Handler&));
        MOCK_METHOD2(RegisterSceneGravityChangedEvent, void(AzPhysics::SceneHandle, AZ::Event<AZStd::tuple<AZ::Crc32, signed char>, const AZ::Vector3&>::
            Handler&));
        MOCK_METHOD2(RegisterSceneSimulationFinishHandler, void(AzPhysics::SceneHandle, AzPhysics::SceneEvents::
            OnSceneSimulationFinishHandler&));
        MOCK_METHOD2(RegisterSceneSimulationStartHandler, void(AzPhysics::SceneHandle, AzPhysics::SceneEvents::
            OnSceneSimulationStartHandler&));
        MOCK_METHOD2(RegisterSceneTriggersEventHandler, void(AzPhysics::SceneHandle, AZ::Event<AZStd::tuple<AZ::Crc32, signed char>, const AZStd::vector<
            AzPhysics::TriggerEvent>&>::Handler&));
        MOCK_METHOD2(RegisterSimulationBodyAddedHandler, void(AzPhysics::SceneHandle, AZ::Event<AZStd::tuple<AZ::Crc32, signed char>, AZStd::tuple<AZ::Crc32, int>>::
            Handler&));
        MOCK_METHOD2(RegisterSimulationBodyRemovedHandler, void(AzPhysics::SceneHandle, AZ::Event<AZStd::tuple<AZ::Crc32, signed char>, AZStd::tuple<AZ::Crc32, int>>
            ::Handler&));
        MOCK_METHOD2(RegisterSimulationBodySimulationDisabledHandler, void(AzPhysics::SceneHandle, AZ::Event<AZStd::tuple<AZ::Crc32, signed char>, AZStd::tuple<
            AZ::Crc32, int>>::Handler&));
        MOCK_METHOD2(RegisterSimulationBodySimulationEnabledHandler, void(AzPhysics::SceneHandle, AZ::Event<AZStd::tuple<AZ::Crc32, signed char>, AZStd::tuple<
            AZ::Crc32, int>>::Handler&));
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
} // namespace UnitTest
