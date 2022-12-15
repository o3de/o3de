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
#include <AzCore/Jobs/MultipleDependentJob.h>
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
    AZ_CVAR(size_t, physx_heightfieldColliderUpdateRegionSize, 512 * 512, nullptr,
        AZ::ConsoleFunctorFlags::Null,
        "Max size of a heightfield collider update region in heightfield points, used for partitioning updates for faster cancellation. "
        "Each update will be the largest number of heightfield rows that stays below this total point count threshold.");

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

    void HeightfieldCollider::HeightfieldUpdateJobContext::OnRefreshStart()
    {
        // When the update job starts, track that it has started and that we shouldn't cancel anything yet.
        AZStd::unique_lock<AZStd::mutex> lock(m_jobsRunningNotificationMutex);
        m_isCanceled = false;
        m_refreshInProgress = true;
    }

    void HeightfieldCollider::HeightfieldUpdateJobContext::OnRefreshComplete()
    {
        // On completion, track that the job has finished, and notify any listneers that it's done.
        {
            AZStd::unique_lock<AZStd::mutex> lock(m_jobsRunningNotificationMutex);
            m_refreshInProgress = false;
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
                return !m_refreshInProgress;
            });
    }


    HeightfieldCollider::DirtyHeightfieldRegion::DirtyHeightfieldRegion()
    {
        SetNull();
    }

    void HeightfieldCollider::DirtyHeightfieldRegion::SetNull()
    {
        m_minRowVertex = AZStd::numeric_limits<size_t>::max();
        m_minColumnVertex = AZStd::numeric_limits<size_t>::max();
        m_maxRowVertex = AZStd::numeric_limits<size_t>::lowest();
        m_maxColumnVertex = AZStd::numeric_limits<size_t>::lowest();
    }

    void HeightfieldCollider::DirtyHeightfieldRegion::AddAabb(const AZ::Aabb& dirtyRegion, AZ::EntityId entityId)
    {
        size_t startRowVertex = 0;
        size_t startColumnVertex = 0;
        size_t numRowVertices = 0;
        size_t numColumnVertices = 0;

        Physics::HeightfieldProviderRequestsBus::Event(
            entityId,
            &Physics::HeightfieldProviderRequestsBus::Events::GetHeightfieldIndicesFromRegion,
            dirtyRegion,
            startColumnVertex,
            startRowVertex,
            numColumnVertices,
            numRowVertices);

        m_minRowVertex = AZStd::min(m_minRowVertex, startRowVertex);
        m_minColumnVertex = AZStd::min(m_minColumnVertex, startColumnVertex);
        // Note that if the heightfield size has decreased, these numbers can end up larger than the total current heightfield size.
        m_maxRowVertex = AZStd::max(m_maxRowVertex, startRowVertex + numRowVertices);
        m_maxColumnVertex = AZStd::max(m_maxColumnVertex, startColumnVertex + numColumnVertices);
    }



    HeightfieldCollider::HeightfieldCollider(
        AZ::EntityId entityId,
        const AZStd::string& entityName,
        AzPhysics::SceneHandle sceneHandle,
        AZStd::shared_ptr<Physics::ColliderConfiguration> colliderConfig,
        AZStd::shared_ptr<Physics::HeightfieldShapeConfiguration> shapeConfig,
        DataSource dataSourceType)
        : m_entityId(entityId)
        , m_entityName(entityName)
        , m_colliderConfig(colliderConfig)
        , m_shapeConfig(shapeConfig)
        , m_attachedSceneHandle(sceneHandle)
        , m_dataSourceType(dataSourceType)
    {
        m_jobContext = AZStd::make_unique<HeightfieldUpdateJobContext>(AZ::JobContext::GetGlobalContext()->GetJobManager());

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

    void HeightfieldCollider::InitStaticRigidBody()
    {
        AZ::Transform baseTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(baseTransform, m_entityId, &AZ::TransformInterface::GetWorldTM);
        InitStaticRigidBody(baseTransform);
    }
    
    void HeightfieldCollider::UpdateShapeConfigRows(
        AZ::Job* updateCompleteJob, size_t startColumn, size_t startRow, size_t numColumns, size_t numRows)
    {
        // This method is called by an update job to update a portion of the heightfield shape configuration to contain the latest
        // heightfield data.

        // This callback is used by UpdateHeightsAndMaterialsAsync to update each point in the heightfield.
        auto modifySample = [this](size_t col, size_t row, const Physics::HeightMaterialPoint& point)
        {
            m_shapeConfig->ModifySample(col, row, point);
        };

        // This callback is triggered when UpdateHeightsAndMaterialsAsync is complete.
        // This triggers our empty UpdateComplete job which is used a placeholder to trigger the other jobs that depend on it completing.
        auto updateComplete = [updateCompleteJob]()
        {
            updateCompleteJob->Start();
        };

        // If we're trying to cancel the update, or there's nothing to update, just trigger the update completion job and return.
        if (m_jobContext->IsCanceled() || (numRows == 0) || (numColumns == 0))
        {
            updateComplete();
            return;
        }

        // Update the shape configuration with the new height and material data for the heightfield.
        // This assumes that the shape configuration already has been created with the correct number of samples.
        if (Physics::HeightfieldProviderRequestsBus::HasHandlers(m_entityId))
        {
            Physics::HeightfieldProviderRequestsBus::Event(
                m_entityId, &Physics::HeightfieldProviderRequestsBus::Events::UpdateHeightsAndMaterialsAsync,
                modifySample, updateComplete, startColumn, startRow, numColumns, numRows);
        }
        else
        {
            // If nothing is connected to the bus, call updateComplete() directly so that the job processing chain completes.
            updateComplete();
        }
    }

    void HeightfieldCollider::UpdatePhysXHeightfieldRows(
        AzPhysics::Scene* scene, AZStd::shared_ptr<Physics::Shape> shape,
        size_t startColumn, size_t startRow, size_t numColumns, size_t numRows)
    {
        // This method is called by an update job to update a portion of the PhysX heightfield to contain the latest heightfield data.

        if (!m_jobContext->IsCanceled() && (numRows > 0) && (numColumns > 0))
        {
            // Refresh a subset of the PhysX heightfield.
            // This assumes that the shape configuration for this region has already been updated.
            // NOTE: For a given heightfield, only one of these calls should be executed at a time, since the underlying PhysX
            // heightfield has no thread safety protections and modifies min/max height data global to the heightfield on every refresh.
            Utils::RefreshHeightfieldShape(scene, &(*shape), *m_shapeConfig, startColumn, startRow, numColumns, numRows);

            // Reduce our dirty region by the number of rows that we're processing in this piece of the update job chain.
            // We've updated both the shape configuration and the PhysX heightfield at this point, so those rows have completed
            // their update. Even if we cancel the job at this point, we'll only need to reprocess these rows if data in those rows
            // have changed.
            // This dirty region logic assumes that we're updating all dirty columns for a row on every call. If this assumption
            // ever changes, we'll need more complicated dirty region logic to track which columns in each row are dirty.
            m_dirtyRegion.m_minRowVertex = startRow + numRows;
        }
    }

    void HeightfieldCollider::RefreshComplete()
    {
        // This method is called by an update job to signal that the chain of update jobs have completed.

        // If the job hasn't been canceled, notify any listeners that the collider has changed.
        if (!m_jobContext->IsCanceled())
        {
            m_dirtyRegion.SetNull();
            Physics::ColliderComponentEventBus::Event(m_entityId, &Physics::ColliderComponentEvents::OnColliderChanged);
        }

        // Notify the job context that the job is completed, so that anything blocking on job completion knows it can proceed.
        m_jobContext->OnRefreshComplete();
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

        // Early out if Heightfield Collider works only with cached heightfield data
        if (m_dataSourceType == DataSource::UseCachedHeightfield)
        {
            if (m_staticRigidBodyHandle == AzPhysics::InvalidSimulatedBodyHandle
                && m_shapeConfig->GetCachedNativeHeightfield() != nullptr)
            {
                InitStaticRigidBody();
            }

            return;
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
        // Resize: we need to cancel any running jobs, wait for them to finish, resize the area, and kick them off again.
        //   PhysX heightfields need to have a static number of points, so a resize requires a complete rebuild of the heightfield.
        // Update: technically, we could get more clever with updates, and potentially keep the same job chain running with a running list
        //   of update regions. But for now, we're keeping it simple. Our update job will update in multiples of heightfield rows so
        //   that we can incrementally shrink the update region as we finish updating pieces of it and cancel at a more granular level.
        //   On a new update, we can then cancel the job, grow our update region as needed, and start the job chain back up again.

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
            InitStaticRigidBody();
        }

        // Add the new request region to our dirty heightfield region
        m_dirtyRegion.AddAabb(requestRegion, m_entityId);

        AZ_Assert(m_dirtyRegion.m_maxRowVertex >= m_dirtyRegion.m_minRowVertex,
            "Invalid dirty region (min=%zu max=%zu)", m_dirtyRegion.m_minRowVertex, m_dirtyRegion.m_maxRowVertex);

        AZ_Assert(m_dirtyRegion.m_maxColumnVertex >= m_dirtyRegion.m_minColumnVertex,
            "Invalid dirty region (min=%zu max=%zu)", m_dirtyRegion.m_maxColumnVertex, m_dirtyRegion.m_minColumnVertex);

        // If our heightfield size has just shrunk and we had a pre-existing dirty region, the max vertex values could be higher than
        // our current size, so clamp them to the current size.
        m_dirtyRegion.m_maxRowVertex = AZStd::min(m_dirtyRegion.m_maxRowVertex, m_shapeConfig->GetNumRowVertices());
        m_dirtyRegion.m_maxColumnVertex = AZStd::min(m_dirtyRegion.m_maxColumnVertex, m_shapeConfig->GetNumColumnVertices());

        size_t startColumn = m_dirtyRegion.m_minColumnVertex;
        size_t numColumns = m_dirtyRegion.m_maxColumnVertex - m_dirtyRegion.m_minColumnVertex;
        size_t numRows = m_dirtyRegion.m_maxRowVertex - m_dirtyRegion.m_minRowVertex;

        // If our dirty region is too small to affect any vertices, early-out.
        if ((numRows == 0) || (numColumns == 0))
        {
            return;
        }

        auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get();
        auto* scene = physicsSystem->GetScene(m_attachedSceneHandle);

        auto shape = GetHeightfieldShape();

        // Get the number of rows to update in each job. We subdivide the region into multiple jobs when processing
        // so that cancellation requests can be detected and processed more quickly. If we just processed a single full dirty region,
        // regardless of size, there would be a lot more work that needs to complete before we could cancel a job.
        const size_t rowsPerUpdate = AZStd::max(physx_heightfieldColliderUpdateRegionSize / numColumns, static_cast<size_t>(1));

        AZStd::vector<AZ::Job*> updateShapeConfigJobs;
        AZStd::vector<AZ::MultipleDependentJob*> updateShapeConfigCompleteJobs;
        AZStd::vector<AZ::Job*> updatePhysXHeightfieldJobs;

        constexpr bool autoDelete = true;

        // The work for refreshing a heightfield is broken up into a series of jobs designed to maximize parallelization, avoid jobs
        // blocking on other jobs, and to respond to cancellation requests reasonably quickly.
        // 
        // For each block of rows being processed we do the following:
        // UpdateShapeConfigJob -> (UpdateHeightsAndMaterialsAsync) -> UpdateShapeConfigCompleteJob -> UpdatePhysXHeightfieldJob
        // i.e. we update the shape configuration, then we update the PhysX Heightfield
        // The final UpdatePhysXHeightfieldJob triggers the RefreshCompleteJob to signify that all the work is completed.
        // 
        // For simplicity in managing the job chain, the entire chain of jobs is still triggered on cancellation, but all
        // of the updating logic is skipped.
        //
        // For better parallelization, we set up the job dependencies to overlap the jobs like this:
        // Usc = UpdateShapeConfigJob
        // Csc = UpdateShapeConfigCompleteJob
        // Uph = UpdatePhysXHeightfieldJob
        // RC  = RefreshCompleteJob
        //
        // Usc1 -> Csc1 -> Usc2 -> Csc2 -> Usc3 -> Csc3
        //             \-> Uph1 ---->  \-> Uph2 ---->  \-> Uph3 -> RC
        // Basically, the UpdateShapeConfig runs in parallel with the UpdatePhysXHeightfield, but each type of update runs sequentially
        // to avoid threading update problems, and the UpdatePhysXHeightfield step can't run until the UpdateShapeConfig step it depends
        // on is complete.

        for (size_t row = 0; row < numRows; row += rowsPerUpdate)
        {
            size_t startRow = m_dirtyRegion.m_minRowVertex + row;
            size_t subregionRows = AZStd::min(m_dirtyRegion.m_maxRowVertex - startRow, rowsPerUpdate);

            // Create the jobs for this set of rows

            auto* updateShapeConfigCompleteJob = aznew AZ::MultipleDependentJob(autoDelete, m_jobContext.get());

            auto* updateShapeConfigJob = AZ::CreateJobFunction(
                AZStd::bind(&HeightfieldCollider::UpdateShapeConfigRows,
                    this, updateShapeConfigCompleteJob, startColumn, startRow, numColumns, subregionRows),
                    autoDelete, m_jobContext.get());

            auto* updatePhysXHeightfieldJob = AZ::CreateJobFunction(
                AZStd::bind(&HeightfieldCollider::UpdatePhysXHeightfieldRows,
                    this, scene, shape, startColumn, startRow, numColumns, subregionRows),
                    autoDelete, m_jobContext.get());

            // Set up the dependencies:
            // UpdateShapeConfigJob 1 -> UpdateShapeConfigCompleteJob 1 -> UpdatePhysXHeightfieldJob 1
            updateShapeConfigJob->SetDependent(updateShapeConfigCompleteJob);
            updateShapeConfigCompleteJob->AddDependent(updatePhysXHeightfieldJob);

            // Set up additional dependencies for all jobs past the first one:
            // UpdateShapeConfigCompleteJob 1 -> UpdateShapeConfigJob 2
            // UpdatePhysXHeightfieldJob 1 -> UpdatePhysXHeightfieldJob 2
            if (!updateShapeConfigCompleteJobs.empty())
            {
                updateShapeConfigCompleteJobs.back()->AddDependent(updateShapeConfigJob);
                updatePhysXHeightfieldJobs.back()->SetDependent(updatePhysXHeightfieldJob);
            }

            // Temporarily store all the jobs we're creating so that we can continue to set up dependencies and start the jobs at the end.
            updateShapeConfigJobs.emplace_back(updateShapeConfigJob);
            updateShapeConfigCompleteJobs.emplace_back(updateShapeConfigCompleteJob);
            updatePhysXHeightfieldJobs.emplace_back(updatePhysXHeightfieldJob);
        }

        if (!updateShapeConfigJobs.empty())
        {
            // Set up the final completion job and dependency:
            // UpdatePhysXHeightfieldJob -> RefreshCompleteJob
            auto* refreshCompleteJob =
                AZ::CreateJobFunction(AZStd::bind(&HeightfieldCollider::RefreshComplete, this), autoDelete, m_jobContext.get());
            updatePhysXHeightfieldJobs.back()->SetDependent(refreshCompleteJob);

            // Track that we're starting our refresh job chain.
            m_jobContext->OnRefreshStart();

            // Start all the jobs except the UpdateShapeConfigCompletion jobs.
            // None of the jobs will actually start until all their dependencies are met, this just "primes" them so that they'll start
            // as soon as they can.
            // The completion jobs are started from the completion callback that's provided to UpdateHeightsAndMaterialsAsync. This 
            // effectively lets us create an implicit dependency on all the jobs created by that API, because until we start the
            // completion jobs, nothing downstream from them can start either.

            for (size_t jobIndex = 0; jobIndex < updateShapeConfigJobs.size(); jobIndex++)
            {
                updateShapeConfigJobs[jobIndex]->Start();
                updatePhysXHeightfieldJobs[jobIndex]->Start();
            }

            refreshCompleteJob->Start();
        }
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
