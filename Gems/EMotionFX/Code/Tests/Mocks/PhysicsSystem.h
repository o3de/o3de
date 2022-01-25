/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/PhysicsSystem.h>
#include <gmock/gmock.h>

namespace Physics
{
    class MockPhysicsSystem
        : public Physics::SystemRequestBus::Handler
        , AZ::Interface<Physics::System>::Registrar
    {
    public:
        // This uses the same uuid as the production PhysX::SystemComponent.
        // The Ragdoll UI uses this UUID to see if physx is available.
        AZ_RTTI(MockPhysicsSystem, "{85F90819-4D9A-4A77-AB89-68035201F34B}");

        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<MockPhysicsSystem>()
                    ->Version(0)
                    ;
            }
        }

        MockPhysicsSystem()
        {
            BusConnect();
        }
        ~MockPhysicsSystem()
        {
            BusDisconnect();
        }
        MOCK_METHOD2(CreateShape, AZStd::shared_ptr<Physics::Shape>(const Physics::ColliderConfiguration& colliderConfiguration, const Physics::ShapeConfiguration& configuration));
        MOCK_METHOD1(ReleaseNativeMeshObject, void(void* nativeMeshObject));
        MOCK_METHOD1(ReleaseNativeHeightfieldObject, void(void* nativeMeshObject));
        MOCK_METHOD1(CreateMaterial, AZStd::shared_ptr<Physics::Material>(const Physics::MaterialConfiguration& materialConfiguration));
        MOCK_METHOD3(CookConvexMeshToFile, bool(const AZStd::string& filePath, const AZ::Vector3* vertices, AZ::u32 vertexCount));
        MOCK_METHOD3(CookConvexMeshToMemory, bool(const AZ::Vector3* vertices, AZ::u32 vertexCount, AZStd::vector<AZ::u8>& result));
        MOCK_METHOD5(CookTriangleMeshToFile, bool(const AZStd::string& filePath, const AZ::Vector3* vertices, AZ::u32 vertexCount, const AZ::u32* indices, AZ::u32 indexCount));
        MOCK_METHOD5(CookTriangleMeshToMemory, bool(const AZ::Vector3* vertices, AZ::u32 vertexCount, const AZ::u32* indices, AZ::u32 indexCount, AZStd::vector<AZ::u8>& result));
    };

    //Mocked of the AzPhysics System Interface. To keep things simple just mocked functions that have a return value.
    class MockPhysicsInterface
        : AZ::Interface<AzPhysics::SystemInterface>::Registrar
    {
    public:
        void Initialize([[maybe_unused]]const AzPhysics::SystemConfiguration* config) override {}
        void Reinitialize() override {}
        void Shutdown() override {}
        void Simulate([[maybe_unused]] float deltaTime) override {}
        void UpdateConfiguration([[maybe_unused]] const AzPhysics::SystemConfiguration* newConfig, [[maybe_unused]] bool forceReinitialization = false) override {}
        void UpdateDefaultSceneConfiguration([[maybe_unused]] const AzPhysics::SceneConfiguration& sceneConfiguration) override {}
        void RemoveScene([[maybe_unused]] AzPhysics::SceneHandle handle) override {}
        void RemoveScenes([[maybe_unused]] const AzPhysics::SceneHandleList& handles) override {}
        void RemoveAllScenes() override {}

        MOCK_METHOD1(AddScene, AzPhysics::SceneHandle(const AzPhysics::SceneConfiguration& config));
        MOCK_METHOD1(AddScenes, AzPhysics::SceneHandleList(const AzPhysics::SceneConfigurationList& configs));
        MOCK_METHOD1(GetSceneHandle, AzPhysics::SceneHandle(const AZStd::string& sceneName));
        MOCK_METHOD1(GetScene, AzPhysics::Scene* (AzPhysics::SceneHandle handle));
        MOCK_METHOD1(GetScenes, AzPhysics::SceneList (const AzPhysics::SceneHandleList& handles));
        MOCK_METHOD0(GetAllScenes, AzPhysics::SceneList& ());
        MOCK_METHOD1(FindAttachedBodyHandleFromEntityId, AZStd::pair<AzPhysics::SceneHandle, AzPhysics::SimulatedBodyHandle>(AZ::EntityId entityId));
        MOCK_CONST_METHOD0(GetConfiguration, const AzPhysics::SystemConfiguration* ());
        MOCK_CONST_METHOD0(GetDefaultSceneConfiguration, const AzPhysics::SceneConfiguration& ());
    };

    class MockJointHelpersInterface : AZ::Interface<AzPhysics::JointHelpersInterface>::Registrar
    {
    public:
        MOCK_CONST_METHOD0(GetSupportedJointTypeIds, const AZStd::vector<AZ::TypeId>());
        MOCK_CONST_METHOD1(GetSupportedJointTypeId, AZStd::optional<const AZ::TypeId>(AzPhysics::JointType typeEnum));

        MOCK_METHOD5(
            ComputeInitialJointLimitConfiguration,
            AZStd::unique_ptr<AzPhysics::JointConfiguration>(
                const AZ::TypeId& jointLimitTypeId,
                const AZ::Quaternion& parentWorldRotation,
                const AZ::Quaternion& childWorldRotation,
                const AZ::Vector3& axis,
                const AZStd::vector<AZ::Quaternion>& exampleLocalRotations));

        MOCK_METHOD10(
            GenerateJointLimitVisualizationData,
            void(
                const AzPhysics::JointConfiguration& configuration,
                const AZ::Quaternion& parentRotation,
                const AZ::Quaternion& childRotation,
                float scale,
                AZ::u32 angularSubdivisions,
                AZ::u32 radialSubdivisions,
                AZStd::vector<AZ::Vector3>& vertexBufferOut,
                AZStd::vector<AZ::u32>& indexBufferOut,
                AZStd::vector<AZ::Vector3>& lineBufferOut,
                AZStd::vector<bool>& lineValidityBufferOut));
    };

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
        MOCK_CONST_METHOD1(IsEnabled, bool(AzPhysics::SceneHandle sceneHandle));
        MOCK_METHOD2(AddSimulatedBody, AzPhysics::SimulatedBodyHandle(AzPhysics::SceneHandle sceneHandle, const AzPhysics::SimulatedBodyConfiguration* simulatedBodyConfig));
        MOCK_METHOD2(AddSimulatedBodies, AzPhysics::SimulatedBodyHandleList(AzPhysics::SceneHandle sceneHandle, const AzPhysics::SimulatedBodyConfigurationList& simulatedBodyConfigs));
        MOCK_METHOD2(GetSimulatedBodyFromHandle, AzPhysics::SimulatedBody* (AzPhysics::SceneHandle sceneHandle, AzPhysics::SimulatedBodyHandle bodyHandle));
        MOCK_METHOD2(GetSimulatedBodiesFromHandle, AzPhysics::SimulatedBodyList(AzPhysics::SceneHandle sceneHandle, const AzPhysics::SimulatedBodyHandleList& bodyHandles));
        MOCK_METHOD4(AddJoint, AzPhysics::JointHandle(AzPhysics::SceneHandle sceneHandle, const AzPhysics::JointConfiguration* jointConfig,
            AzPhysics::SimulatedBodyHandle parentBody, AzPhysics::SimulatedBodyHandle childBody));
        MOCK_METHOD2(GetJointFromHandle, AzPhysics::Joint* (AzPhysics::SceneHandle sceneHandle, AzPhysics::JointHandle bodyHandle));
        MOCK_CONST_METHOD1(GetGravity, AZ::Vector3(AzPhysics::SceneHandle sceneHandle));
        MOCK_METHOD2(RegisterSceneSimulationFinishHandler, void(AzPhysics::SceneHandle sceneHandle, AzPhysics::SceneEvents::OnSceneSimulationFinishHandler& handler));
        MOCK_CONST_METHOD2(GetLegacyBody, AzPhysics::SimulatedBody* (AzPhysics::SceneHandle sceneHandle, AzPhysics::SimulatedBodyHandle handle));
        MOCK_METHOD2(QueryScene, AzPhysics::SceneQueryHits(AzPhysics::SceneHandle sceneHandle, const AzPhysics::SceneQueryRequest* request));
        MOCK_METHOD2(QuerySceneBatch, AzPhysics::SceneQueryHitsList(AzPhysics::SceneHandle sceneHandle, const AzPhysics::SceneQueryRequests& requests));
        MOCK_METHOD4(QuerySceneAsync, bool(AzPhysics::SceneHandle sceneHandle, AzPhysics::SceneQuery::AsyncRequestId requestId,
            const AzPhysics::SceneQueryRequest* request, AzPhysics::SceneQuery::AsyncCallback callback));
        MOCK_METHOD4(QuerySceneAsyncBatch, bool(AzPhysics::SceneHandle sceneHandle, AzPhysics::SceneQuery::AsyncRequestId requestId,
            const AzPhysics::SceneQueryRequests& requests, AzPhysics::SceneQuery::AsyncBatchCallback callback));
    };
} // namespace Physics
