/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Jobs/Job.h>
#include <AzCore/std/parallel/condition_variable.h>

#include <AzFramework/Physics/Components/SimulatedBodyComponentBus.h>
#include <AzFramework/Physics/HeightfieldProviderBus.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/Shape.h>

#include <PhysX/ColliderShapeBus.h>

namespace PhysX
{
    //! PhysX Heightfield Collider base class.
    //! This contains all the logic shared between the Editor Heightfield Collider Component and the Heightfield Collider Component
    //! to create, update, and destroy the heightfield collider at runtime.
    class HeightfieldCollider
        : protected AzPhysics::SimulatedBodyComponentRequestsBus::Handler
        , protected Physics::HeightfieldProviderNotificationBus::Handler
        , protected PhysX::ColliderShapeRequestBus::Handler
    {
    public:
        HeightfieldCollider() = default;
        ~HeightfieldCollider();

        //! Create a HeightfieldCollider that operates on the given set of data.
        //! @param entityId The entity Id for the entity that contains this heightfield collider
        //! @param entityName The enntity name for the entity that contains this heightfield collider (for debug purposes)
        //! @param sceneHandler The physics scene to create the collider in (Editor or runtime)
        //! @param colliderConfig The collider configuration to use. Some of its data will get modified based on the heightfield data.
        //! @param shapeConfig The shape configuration to use. All of its data will get modified based on the heightfield data.
        HeightfieldCollider(
            AZ::EntityId entityId,
            const AZStd::string& entityName,
            AzPhysics::SceneHandle sceneHandle, 
            AZStd::shared_ptr<Physics::ColliderConfiguration> colliderConfig,
            AZStd::shared_ptr<Physics::HeightfieldShapeConfiguration> shapeConfig);

        //! Get the currently-spawned heightfield shape.
        //! @return Pointer to the heightfield shape.
        AZStd::shared_ptr<Physics::Shape> GetHeightfieldShape();

        //! Notify the heightfield that it may need to refresh some or all of its data.
        //! @param changeMask The types of data changes causing the notification.
        //! @param dirtyRegion The area affected by the notification, or a Null Aabb if everything is affected.
        void RefreshHeightfield(
            Physics::HeightfieldProviderNotifications::HeightfieldChangeMask changeMask,
            const AZ::Aabb& dirtyRegion = AZ::Aabb::CreateNull());

        //! Get a pointer to the currently-spawned simulated body.
        //! @return Pointer to the simulated body.
        const AzPhysics::SimulatedBody* GetSimulatedBody() const;

        void BlockOnPendingJobs();

        // AzPhysics::SimulatedBodyComponentRequestsBus::Handler overrides ...
        void EnablePhysics() override;
        void DisablePhysics() override;
        bool IsPhysicsEnabled() const override;
        AZ::Aabb GetAabb() const override;
        AzPhysics::SimulatedBody* GetSimulatedBody() override;
        AzPhysics::SimulatedBodyHandle GetSimulatedBodyHandle() const override;
        AzPhysics::SceneQueryHit RayCast(const AzPhysics::RayCastRequest& request) override;

    protected:
        // Physics::HeightfieldProviderNotificationBus::Handler overrides ...
        void OnHeightfieldDataChanged(
            const AZ::Aabb& dirtyRegion, Physics::HeightfieldProviderNotifications::HeightfieldChangeMask changeMask) override;

        // ColliderShapeRequestBus
        AZ::Aabb GetColliderShapeAabb() override;
        bool IsTrigger() override
        {
            // PhysX Heightfields don't support triggers.
            return false;
        }

        void ClearHeightfield();
        void InitStaticRigidBody(const AZ::Transform& baseTransform);

        void UpdateHeightfieldMaterialSlots(const Physics::MaterialSlots& updatedMaterialSlots);

    private:
        //! Helper class to manage the spawned physics update jobs.
        class HeightfieldUpdateJobContext : public AZ::JobContext
        {
        public:
            HeightfieldUpdateJobContext(AZ::JobManager& jobManager)
                : JobContext(jobManager)
            {
            }

            //! Cancel any running jobs
            void Cancel();

            //! Check to see if the jobs should be canceled.
            bool IsCanceled() const;

            //! Track all jobs that are being started.
            void OnJobStart();

            //! Track all jobs that have been completed.
            void OnJobComplete();

            //! Block until all jobs have been completed.
            void BlockUntilComplete();

        private:
            //! Track the number of currently-running jobs. (This will currently either be 0 or 1, but may get more complicated someday)
            int m_numRunningJobs = 0;
            //! Mutex to protect the job-running state
            AZStd::mutex m_jobsRunningNotificationMutex;
            //! Notification mechanism for knowing when the jobs have stopped running.
            //! This uses a condition_variable instead of a semaphore so that there doesn't need to be an equal number of job starts
            //! vs "block on complete" calls.
            AZStd::condition_variable m_jobsRunning;

            //! Track whether or not the currently-running jobs should be canceled.
            AZStd::atomic_bool m_isCanceled = false;
        };

        //! Stores collision layers, whether the collider is a trigger, etc.
        AZStd::shared_ptr<Physics::ColliderConfiguration> m_colliderConfig;

        //! Stores all of the cached information for the heightfield shape.
        AZStd::shared_ptr<Physics::HeightfieldShapeConfiguration> m_shapeConfig;

        //! Handle to the body in the provided physics scene.
        AzPhysics::SimulatedBodyHandle m_staticRigidBodyHandle = AzPhysics::InvalidSimulatedBodyHandle; 

        //! Handle to the provided physics scene.
        AzPhysics::SceneHandle m_attachedSceneHandle = AzPhysics::InvalidSceneHandle;

        //! Job context for managing the collider update jobs that get spawned.
        AZStd::unique_ptr<HeightfieldUpdateJobContext> m_jobContext;

        //! Cached entity ID for the entity this collider is attached to.
        AZ::EntityId m_entityId;

        //! Cached entity name for the entity this collider is attached to.
        AZStd::string m_entityName;

        //! Track the current dirty region for async heightfield refreshes.
        AZ::Aabb m_dirtyRegion;
    };

} // namespace PhysX
