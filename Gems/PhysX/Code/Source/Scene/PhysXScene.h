/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/Common/PhysicsEvents.h>
#include <AzFramework/Physics/Common/PhysicsSimulatedBody.h>
#include <AzFramework/Physics/Configuration/SceneConfiguration.h>

#include <Scene/PhysXSceneSimulationEventCallback.h>
#include <Scene/PhysXSceneSimulationFilterCallback.h>

namespace physx
{
    class PxScene;
}

namespace PhysX
{
    //! PhysX implementation of the AzPhysics::Scene.
    class PhysXScene
        : public AzPhysics::Scene
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL;
        AZ_RTTI(PhysXScene, "{B0FCFDE6-8B59-49D8-8819-E8C2F1EDC182}", AzPhysics::Scene);

        explicit PhysXScene(const AzPhysics::SceneConfiguration& config, const AzPhysics::SceneHandle& sceneHandle);
        ~PhysXScene();

        // AzPhysics::PhysicsScene ...
        void StartSimulation(float deltatime) override;
        void FinishSimulation() override;
        void SetEnabled(bool enable) override;
        bool IsEnabled() const override;
        const AzPhysics::SceneConfiguration& GetConfiguration() const override;
        void UpdateConfiguration(const AzPhysics::SceneConfiguration& config) override;
        AzPhysics::SimulatedBodyHandle AddSimulatedBody(const AzPhysics::SimulatedBodyConfiguration* simulatedBodyConfig) override;
        AzPhysics::SimulatedBodyHandleList AddSimulatedBodies(const AzPhysics::SimulatedBodyConfigurationList& simulatedBodyConfigs) override;
        AzPhysics::SimulatedBody* GetSimulatedBodyFromHandle(AzPhysics::SimulatedBodyHandle bodyHandle) override;
        AzPhysics::SimulatedBodyList GetSimulatedBodiesFromHandle(const AzPhysics::SimulatedBodyHandleList& bodyHandles) override;
        void RemoveSimulatedBody(AzPhysics::SimulatedBodyHandle& bodyHandle) override;
        void RemoveSimulatedBodies(AzPhysics::SimulatedBodyHandleList& bodyHandles) override;
        void EnableSimulationOfBody(AzPhysics::SimulatedBodyHandle bodyHandle) override;
        void DisableSimulationOfBody(AzPhysics::SimulatedBodyHandle bodyHandle) override;
        AzPhysics::SceneQueryHits QueryScene(const AzPhysics::SceneQueryRequest* request) override;
        AzPhysics::SceneQueryHitsList QuerySceneBatch(const AzPhysics::SceneQueryRequests& requests) override;
        [[nodiscard]] bool QuerySceneAsync(AzPhysics::SceneQuery::AsyncRequestId requestId,
            const AzPhysics::SceneQueryRequest* request, AzPhysics::SceneQuery::AsyncCallback callback) override;
        [[nodiscard]] bool QuerySceneAsyncBatch(AzPhysics::SceneQuery::AsyncRequestId requestId,
            const AzPhysics::SceneQueryRequests& requests, AzPhysics::SceneQuery::AsyncBatchCallback callback) override;
        void SuppressCollisionEvents(
            const AzPhysics::SimulatedBodyHandle& bodyHandleA,
            const AzPhysics::SimulatedBodyHandle& bodyHandleB) override;
        void UnsuppressCollisionEvents(
            const AzPhysics::SimulatedBodyHandle& bodyHandleA,
            const AzPhysics::SimulatedBodyHandle& bodyHandleB) override;
        void SetGravity(const AZ::Vector3& gravity) override;
        AZ::Vector3 GetGravity() const override;

        AzPhysics::SceneHandle GetSceneHandle() const { return m_sceneHandle; }
        const AZStd::vector<AZStd::pair<AZ::Crc32, AzPhysics::SimulatedBody*>>& GetSimulatedBodyList() const { return m_simulatedBodies; }

        void* GetNativePointer() const override;

        physx::PxControllerManager* GetOrCreateControllerManager();

    private:
        void EnableSimulationOfBodyInternal(AzPhysics::SimulatedBody& body);
        void DisableSimulationOfBodyInternal(AzPhysics::SimulatedBody& body);

        void FlushQueuedEvents();
        void ClearDeferedDeletions();
        void ProcessTriggerEvents();
        void ProcessCollisionEvents();

        void UpdateAzProfilerDataPoints();

        bool m_isEnabled = true;
        AzPhysics::SceneConfiguration m_config;
        AzPhysics::SceneHandle m_sceneHandle;
        float m_currentDeltaTime = 0.0f;

        AZStd::vector<AZStd::pair<AZ::Crc32, AzPhysics::SimulatedBody*>> m_simulatedBodies; //this will become a SimulatedBody with LYN-1334
        AZStd::vector<AzPhysics::SimulatedBody*> m_deferredDeletions;
        AZStd::queue<AzPhysics::SimulatedBodyIndex> m_freeSceneSlots;

        AzPhysics::SystemEvents::OnConfigurationChangedEvent::Handler m_physicsSystemConfigChanged;

        static thread_local AZStd::vector<physx::PxRaycastHit> s_rayCastBuffer; //!< thread local structure to hold hits for a single raycast or shapecast.
        static thread_local AZStd::vector<physx::PxSweepHit> s_sweepBuffer; //!< thread local structure to hold hits for a single shapecast.
        static thread_local AZStd::vector<physx::PxOverlapHit> s_overlapBuffer; //!< thread local structure to hold hits for a single overlap query.
        AZ::u64 m_raycastBufferSize = 32; //!< Maximum number of hits that will be returned from a raycast.
        AZ::u64 m_shapecastBufferSize = 32; //!< Maximum number of hits that can be returned from a shapecast.
        AZ::u64 m_overlapBufferSize = 32; //!< Maximum number of overlaps that can be returned from an overlap query.

        SceneSimulationFilterCallback m_collisionFilterCallback; //!< Handles the filtering of collision pairs reported from PhysX.
        SceneSimulationEventCallback m_simulationEventCallback; //!< Handles the collision and trigger events reported from PhysX.
        physx::PxScene* m_pxScene = nullptr; //!< The physx scene
        physx::PxControllerManager* m_controllerManager = nullptr; //!< The physx controller manager

        AZ::Vector3 m_gravity; // cache the gravity of the scene to avoid a lock in GetGravity().
    };
}
