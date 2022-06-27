/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/utility/as_const.h>
#include <AzFramework/Physics/Configuration/StaticRigidBodyConfiguration.h>
#include <AzFramework/Physics/ColliderComponentBus.h>
#include <AzFramework/Physics/SimulatedBodies/StaticRigidBody.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/SystemBus.h>
#include <Source/HeightfieldCollider.h>
#include <System/PhysXSystem.h>
#include <Source/RigidBodyStatic.h>
#include <Source/Shape.h>
#include <Source/Utils.h>
#include <PhysX/Material/PhysXMaterial.h>


namespace PhysX
{
    AZ_CVAR(float, physx_heightfieldColliderUpdateRegionSize, 128.0f, nullptr,
        AZ::ConsoleFunctorFlags::Null,
        "Size of a heightfield collider update region in meters, used for partitioning updates for faster cancellation.");

    // The HeightfieldUpdateJobContext is an extremely simplified way to manage the background update jobs.
    // On any heightfield change, the collider code will cancel any update job that's currently running, wait for it
    // to complete, and then start a new update job.
    // Also, on HeightfieldCollider destruction, any running jobs will get canceled and block on completion.
    // Eventually, this could get migrated to a more complex system that allows for overlapping jobs, or potentially using a queue
    // of regions to update in a currently-running job.
    void HeightfieldCollider::HeightfieldUpdateJobContext::Cancel()
    {
        m_isCanceled = true;
    }

    bool HeightfieldCollider::HeightfieldUpdateJobContext::IsCanceled() const
    {
        return m_isCanceled;
    }

    void HeightfieldCollider::HeightfieldUpdateJobContext::OnJobStart()
    {
        // When the update job starts, track that it has started and that we shouldn't cancel anything yet.
        AZStd::unique_lock<AZStd::mutex> lock(m_jobsRunningNotificationMutex);
        m_isCanceled = false;
        m_numRunningJobs++;
    }

    void HeightfieldCollider::HeightfieldUpdateJobContext::OnJobComplete()
    {
        // On completion, track that the job has finished, and notify any listneers that it's done.
        {
            AZStd::unique_lock<AZStd::mutex> lock(m_jobsRunningNotificationMutex);
            m_numRunningJobs--;
        }
        m_jobsRunning.notify_all();
    }

    void HeightfieldCollider::HeightfieldUpdateJobContext::BlockUntilComplete()
    {
        // Block until the update job completes (or don't block at all if the job never ran)
        AZStd::unique_lock<AZStd::mutex> lock(m_jobsRunningNotificationMutex);
        m_jobsRunning.wait(
            lock,
            [this]
            {
                return m_numRunningJobs <= 0;
            });
    }

    HeightfieldCollider::HeightfieldCollider(
        AZ::EntityId entityId,
        const AZStd::string& entityName,
        AzPhysics::SceneHandle sceneHandle,
        AZStd::shared_ptr<Physics::ColliderConfiguration> colliderConfig,
        AZStd::shared_ptr<Physics::HeightfieldShapeConfiguration> shapeConfig)
        : m_entityId(entityId)
        , m_entityName(entityName)
        , m_colliderConfig(colliderConfig)
        , m_shapeConfig(shapeConfig)
        , m_attachedSceneHandle(sceneHandle)
    {
        m_jobContext = AZStd::make_unique<HeightfieldUpdateJobContext>(AZ::JobContext::GetGlobalContext()->GetJobManager());
        m_dirtyRegion = AZ::Aabb::CreateNull();

        PhysX::ColliderShapeRequestBus::Handler::BusConnect(entityId);
        Physics::HeightfieldProviderNotificationBus::Handler::BusConnect(entityId);
        AzPhysics::SimulatedBodyComponentRequestsBus::Handler::BusConnect(entityId);

        // Make sure that we trigger a refresh on creation. Depending on initialization order, there might not be any other
        // refreshes that occur.
        RefreshHeightfield(Physics::HeightfieldProviderNotifications::HeightfieldChangeMask::Settings, AZ::Aabb::CreateNull());
    }

    HeightfieldCollider::~HeightfieldCollider()
    {
        AzPhysics::SimulatedBodyComponentRequestsBus::Handler::BusDisconnect();
        Physics::HeightfieldProviderNotificationBus::Handler::BusDisconnect();
        PhysX::ColliderShapeRequestBus::Handler::BusDisconnect();

        // Make sure any heightfield collider jobs that are running finish up before we destroy ourselves.
        m_jobContext->Cancel();
        m_jobContext->BlockUntilComplete();

        ClearHeightfield();

        m_jobContext.reset();
    }

    void HeightfieldCollider::BlockOnPendingJobs()
    {
        m_jobContext->BlockUntilComplete();
    }

    // ColliderShapeRequestBus
    AZ::Aabb HeightfieldCollider::GetColliderShapeAabb()
    {
        // Get the Collider AABB directly from the heightfield provider.
        AZ::Aabb colliderAabb = AZ::Aabb::CreateNull();
        Physics::HeightfieldProviderRequestsBus::EventResult(
            colliderAabb, m_entityId, &Physics::HeightfieldProviderRequestsBus::Events::GetHeightfieldAabb);

        return colliderAabb;
    }

    // Physics::HeightfieldProviderNotificationBus
    void HeightfieldCollider::OnHeightfieldDataChanged(
        const AZ::Aabb& dirtyRegion, const Physics::HeightfieldProviderNotifications::HeightfieldChangeMask changeMask)
    {
        RefreshHeightfield(changeMask, dirtyRegion);
    }

    void HeightfieldCollider::ClearHeightfield()
    {
        // There are two references to the heightfield data, we need to clear both to make the heightfield clear out and deallocate:
        // - The simulated body has a pointer to the shape, which has a GeometryHolder, which has the Heightfield inside it
        // - The shape config is also holding onto a pointer to the Heightfield

        // We remove the simulated body first, since we don't want the heightfield to exist any more.
        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();
            sceneInterface && m_staticRigidBodyHandle != AzPhysics::InvalidSimulatedBodyHandle)
        {
            sceneInterface->RemoveSimulatedBody(m_attachedSceneHandle, m_staticRigidBodyHandle);
        }

        // Now we can safely clear out the cached heightfield pointer.
        m_shapeConfig->SetCachedNativeHeightfield(nullptr);
    }

    void HeightfieldCollider::InitStaticRigidBody(const AZ::Transform& baseTransform)
    {
        // Set the rigid body's position and orientation to match the entity's position and orientation.
        AzPhysics::StaticRigidBodyConfiguration configuration;
        configuration.m_orientation = baseTransform.GetRotation();
        configuration.m_position = baseTransform.GetTranslation();
        configuration.m_entityId = m_entityId;
        configuration.m_debugName = m_entityName;

        AzPhysics::ShapeColliderPairList colliderShapePairs{ AzPhysics::ShapeColliderPair(m_colliderConfig, m_shapeConfig) };
        configuration.m_colliderAndShapeData = colliderShapePairs;

        // Get the transform from the HeightfieldProvider.  Because rotation and scale can indirectly affect how the heightfield itself
        // is computed and the size of the heightfield, and the heightfield might snap or clamp to grids, it's possible that the
        // HeightfieldProvider will provide a different transform back to us than the one that's directly on that entity.
        AZ::Transform transform = AZ::Transform::CreateIdentity();
        Physics::HeightfieldProviderRequestsBus::EventResult(
            transform, m_entityId, &Physics::HeightfieldProviderRequestsBus::Events::GetHeightfieldTransform);

        // Because the heightfield's transform may not match the entity's transform, use the heightfield transform
        // to generate an offset rotation/position from the entity's transform for the collider configuration.
        m_colliderConfig->m_rotation = transform.GetRotation() * baseTransform.GetRotation().GetInverseFull();
        m_colliderConfig->m_position =
            m_colliderConfig->m_rotation.TransformVector(transform.GetTranslation() - baseTransform.GetTranslation());

        // Update material selection from the mapping
        Utils::SetMaterialsFromHeightfieldProvider(m_entityId, m_colliderConfig->m_materialSlots);

        // Create a new simulated body in the world from the given collision / shape configuration.
        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            m_staticRigidBodyHandle = sceneInterface->AddSimulatedBody(m_attachedSceneHandle, &configuration);
        }
    }

    void HeightfieldCollider::RefreshHeightfield(
        const Physics::HeightfieldProviderNotifications::HeightfieldChangeMask changeMask,
        const AZ::Aabb& dirtyRegion)
    {
        using Physics::HeightfieldProviderNotifications;

        // If the change is only about heightfield materials mapping, we can simply update material selection in the heightfield shape
        if (changeMask == Physics::HeightfieldProviderNotifications::HeightfieldChangeMask::SurfaceMapping)
        {
            Physics::MaterialSlots updatedMaterialSlots;
            Utils::SetMaterialsFromHeightfieldProvider(m_entityId, updatedMaterialSlots);

            // Make sure the number of slots is the same.
            // Otherwise the heightfield needs to be rebuilt to support updated indices.
            if (updatedMaterialSlots.GetSlotsCount() ==
                m_colliderConfig->m_materialSlots.GetSlotsCount())
            {
                UpdateHeightfieldMaterialSlots(updatedMaterialSlots);
                return;
            }
        }

        AZ::Aabb heightfieldAabb = GetColliderShapeAabb();
        AZ::Aabb requestRegion = dirtyRegion;

        if (!requestRegion.IsValid())
        {
            requestRegion = heightfieldAabb;
        }

        // Early out if the updated region is outside of the heightfield Aabb
        if (heightfieldAabb.IsValid() && heightfieldAabb.Disjoint(requestRegion))
        {
            return;
        }

        // Clamp requested region to the heightfield AABB so that it only references the area we need to update.
        requestRegion.Clamp(heightfieldAabb);

        // There are two refresh possibilities - resizing the area or updating the data.
        // Resize: we need to cancel any running job, wait for it to finish, resize the area, and kick it off again.
        //   PhysX heightfields need to have a static number of points, so a resize requires a complete rebuild of the heightfield.
        // Update: technically, we could get more clever with updates, and potentially keep the same job running with a running list
        //   of update regions. But for now, we're keeping it simple. Our update job will update in multiples of heightfield rows so
        //   that we can incrementally shrink the update region as we finish updating pieces of it. On a new update, we can then cancel
        //   the job, grow our update region as needed, and start it back up again.

        // If we don't have a shape configuration yet, or if the configuration itself changed, we need to recreate the entire heightfield.
        bool shouldRecreateHeightfield = (m_shapeConfig == nullptr) ||
            ((changeMask & Physics::HeightfieldProviderNotifications::HeightfieldChangeMask::Settings) ==
             Physics::HeightfieldProviderNotifications::HeightfieldChangeMask::Settings);

        // Check if base configuration parameters have changed. If any of the sizes have changed, we'll recreate the entire heightfield.
        if (!shouldRecreateHeightfield)
        {
            Physics::HeightfieldShapeConfiguration baseConfiguration = Utils::CreateBaseHeightfieldShapeConfiguration(m_entityId);
            shouldRecreateHeightfield = shouldRecreateHeightfield || (baseConfiguration.GetNumRowVertices() != m_shapeConfig->GetNumRowVertices());
            shouldRecreateHeightfield = shouldRecreateHeightfield || (baseConfiguration.GetNumColumnVertices() != m_shapeConfig->GetNumColumnVertices());
            shouldRecreateHeightfield = shouldRecreateHeightfield || (baseConfiguration.GetMinHeightBounds() != m_shapeConfig->GetMinHeightBounds());
            shouldRecreateHeightfield = shouldRecreateHeightfield || (baseConfiguration.GetMaxHeightBounds() != m_shapeConfig->GetMaxHeightBounds());
        }

        // If the update job is running, stop it and wait for it to complete.
        m_jobContext->Cancel();
        m_jobContext->BlockUntilComplete();

        // If our heightfield has changed size, recreate the configuration and initialize it.
        if (shouldRecreateHeightfield)
        {
            // Destroy the existing heightfield. This will completely remove it from the world.
            ClearHeightfield();

            *m_shapeConfig = Utils::CreateBaseHeightfieldShapeConfiguration(m_entityId);
            size_t numSamples = m_shapeConfig->GetNumRowVertices() * m_shapeConfig->GetNumColumnVertices();

            // A heightfield needs to be at least a 1 x 1 square.
            if ((m_shapeConfig->GetNumRowSquares() > 0) && (m_shapeConfig->GetNumColumnSquares() > 0))
            {
                AZStd::vector<Physics::HeightMaterialPoint> samples(numSamples);
                m_shapeConfig->SetSamples(AZStd::move(samples));
            }
        }

        // If our new size is "none", we're done.
        if ((m_shapeConfig->GetNumRowSquares() == 0) || (m_shapeConfig->GetNumColumnSquares() == 0))
        {
            return;
        }

        if (shouldRecreateHeightfield)
        {
            // Create a new rigid body for the heightfield on the main thread. This will ensure that other physics calls can safely
            // request the rigid body even while we're asynchronously updating the heightfield itself on a separate thread.
            AZ::Transform baseTransform = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(baseTransform, m_entityId, &AZ::TransformInterface::GetWorldTM);
            InitStaticRigidBody(baseTransform);
        }

        m_dirtyRegion.AddAabb(requestRegion);

        // Get the number of meters to subdivide our update region into. We process the region as subdivided regions
        // so that cancellation requests can be detected and processed more quickly. If we just processed a single full dirty region,
        // regardless of size, there would be a lot more work that needs to complete before we could cancel a job.
        const float regionDivider = physx_heightfieldColliderUpdateRegionSize;

        auto jobLambda = [this, regionDivider]() -> void
        {
            AZ::Aabb dirtyRegion = m_dirtyRegion;

            auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get();
            auto* scene = physicsSystem->GetScene(m_attachedSceneHandle);

            auto shape = GetHeightfieldShape();

            // For each sub-region in our dirty region, get the updated height and material data for the heightfield.
            for (float y = dirtyRegion.GetMin().GetY(); y < dirtyRegion.GetMax().GetY(); y += regionDivider)
            {
                // On each sub-region, if a cancellation has been requested, early-out.
                if (m_jobContext->IsCanceled())
                {
                    break;
                }

                float x = dirtyRegion.GetMin().GetX();

                // Create the sub-region to process.
                const float xMax = dirtyRegion.GetMax().GetX();
                const float yMax = AZStd::min(y + regionDivider, dirtyRegion.GetMax().GetY());

                AZ::Aabb subRegion;
                subRegion.Set(
                    AZ::Vector3(x, y, dirtyRegion.GetMin().GetZ()),
                    AZ::Vector3(xMax, yMax, dirtyRegion.GetMax().GetZ()));

                size_t startRow = 0;
                size_t startColumn = 0;
                size_t numRows = 0;
                size_t numColumns = 0;

                Physics::HeightfieldProviderRequestsBus::Event(
                    m_entityId, &Physics::HeightfieldProviderRequestsBus::Events::GetHeightfieldIndicesFromRegion,
                    subRegion, startColumn, startRow, numColumns, numRows);

                if ((numRows > 0) && (numColumns > 0))
                {
                    // Update the shape configuration with the new height and material data for the heightfield.
                    // This makes the assumption that the shape configuration already has been created with the correct number
                    // of samples.
                    Physics::HeightfieldProviderRequestsBus::Event(
                        m_entityId, &Physics::HeightfieldProviderRequestsBus::Events::UpdateHeightsAndMaterials,
                        [this](size_t col, size_t row, const Physics::HeightMaterialPoint& point)
                        {
                            m_shapeConfig->ModifySample(col, row, point);
                        },
                        startColumn, startRow, numColumns, numRows);

                    Utils::RefreshHeightfieldShape(scene, &(*shape), *m_shapeConfig, startColumn, startRow, numColumns, numRows);
                }

                m_dirtyRegion.SetMin(AZ::Vector3(dirtyRegion.GetMin().GetX(), yMax, dirtyRegion.GetMin().GetZ()));
            }


            // If the job hasn't been canceled, use the updated shape configuration to create a new heightfield in the world
            // and notify any listeners that the collider has changed.
            if (!m_jobContext->IsCanceled())
            {
                m_dirtyRegion = AZ::Aabb::CreateNull();
                Physics::ColliderComponentEventBus::Event(m_entityId, &Physics::ColliderComponentEvents::OnColliderChanged);
            }

            // Notify the job context that the job is completed, so that anything blocking on job completion knows it can proceed.
            m_jobContext->OnJobComplete();
        };

        // Kick off the job to update the heightfield configuration and create the heightfield.
        constexpr bool autoDelete = true;
        auto* runningJob = AZ::CreateJobFunction(jobLambda, autoDelete, m_jobContext.get());
        m_jobContext->OnJobStart();
        runningJob->Start();
    }

    void HeightfieldCollider::UpdateHeightfieldMaterialSlots(const Physics::MaterialSlots& updatedMaterialSlots)
    {
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();
        AzPhysics::SimulatedBody* simulatedBody =
            sceneInterface->GetSimulatedBodyFromHandle(m_attachedSceneHandle, m_staticRigidBodyHandle);
        if (!simulatedBody)
        {
            return;
        }

        AzPhysics::StaticRigidBody* rigidBody = azdynamic_cast<AzPhysics::StaticRigidBody*>(simulatedBody);

        if (rigidBody->GetShapeCount() != 1)
        {
            AZ_Error(
                "UpdateHeightfieldMaterialSelection", rigidBody->GetShapeCount() == 1,
                "Heightfield collider should have only 1 shape. Count: %d", rigidBody->GetShapeCount());
            return;
        }

        AZStd::shared_ptr<Physics::Shape> shape = rigidBody->GetShape(0);
        PhysX::Shape* physxShape = azdynamic_cast<PhysX::Shape*>(shape.get());

        AZStd::vector<AZStd::shared_ptr<Material>> materials =
            Material::FindOrCreateMaterials(updatedMaterialSlots);

        physxShape->SetPhysXMaterials(materials);

        m_colliderConfig->m_materialSlots = updatedMaterialSlots;
    }

    // SimulatedBodyComponentRequestsBus
    void HeightfieldCollider::EnablePhysics()
    {
        if (IsPhysicsEnabled())
        {
            return;
        }
        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            sceneInterface->EnableSimulationOfBody(m_attachedSceneHandle, m_staticRigidBodyHandle);
        }
    }

    // SimulatedBodyComponentRequestsBus
    void HeightfieldCollider::DisablePhysics()
    {
        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            sceneInterface->DisableSimulationOfBody(m_attachedSceneHandle, m_staticRigidBodyHandle);
        }
    }

    // SimulatedBodyComponentRequestsBus
    bool HeightfieldCollider::IsPhysicsEnabled() const
    {
        if (m_staticRigidBodyHandle != AzPhysics::InvalidSimulatedBodyHandle)
        {
            if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();
                sceneInterface != nullptr && sceneInterface->IsEnabled(m_attachedSceneHandle)) // check if the scene is enabled
            {
                if (AzPhysics::SimulatedBody* body =
                        sceneInterface->GetSimulatedBodyFromHandle(m_attachedSceneHandle, m_staticRigidBodyHandle))
                {
                    return body->m_simulating;
                }
            }
        }
        return false;
    }

    // SimulatedBodyComponentRequestsBus
    AzPhysics::SimulatedBodyHandle HeightfieldCollider::GetSimulatedBodyHandle() const
    {
        // The simulated body is created on the main thread, so it should be safe to return it even if we have active jobs
        // running that are updating the simulated body.
        return m_staticRigidBodyHandle;
    }

    // SimulatedBodyComponentRequestsBus
    AzPhysics::SimulatedBody* HeightfieldCollider::GetSimulatedBody()
    {
        return const_cast<AzPhysics::SimulatedBody*>(AZStd::as_const(*this).GetSimulatedBody());
    }

    const AzPhysics::SimulatedBody* HeightfieldCollider::GetSimulatedBody() const
    {
        // The simulated body is created on the main thread, so it should be safe to return it even if we have active jobs
        // running that are updating the simulated body.
        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            return sceneInterface->GetSimulatedBodyFromHandle(m_attachedSceneHandle, m_staticRigidBodyHandle);
        }
        return nullptr;
    }

    // SimulatedBodyComponentRequestsBus
    AzPhysics::SceneQueryHit HeightfieldCollider::RayCast(const AzPhysics::RayCastRequest& request)
    {
        if (auto* body = azdynamic_cast<PhysX::StaticRigidBody*>(GetSimulatedBody()))
        {
            return body->RayCast(request);
        }
        return AzPhysics::SceneQueryHit();
    }

    // SimulatedBodyComponentRequestsBus
    AZ::Aabb HeightfieldCollider::GetAabb() const
    {
        // On the SimulatedBodyComponentRequestsBus, get the AABB from the simulated body instead of the collider.
        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            if (AzPhysics::SimulatedBody* body = sceneInterface->GetSimulatedBodyFromHandle(m_attachedSceneHandle, m_staticRigidBodyHandle))
            {
                return body->GetAabb();
            }
        }
        return AZ::Aabb::CreateNull();
    }

    AZStd::shared_ptr<Physics::Shape> HeightfieldCollider::GetHeightfieldShape()
    {
        if (auto* body = azdynamic_cast<PhysX::StaticRigidBody*>(GetSimulatedBody()))
        {
            // Heightfields should only have one shape
            AZ_Assert(body->GetShapeCount() == 1, "Heightfield rigid body has the wrong number of shapes:  %zu", body->GetShapeCount());
            return body->GetShape(0);
        }

        return {};
    }

} // namespace PhysX
