/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Scene/PhysXScene.h>


#include <Collision.h>
#include <RigidBody.h>
#include <RigidBodyStatic.h>
#include <Shape.h>
#include <Common/PhysXSceneQueryHelpers.h>
#include <PhysX/PhysXLocks.h>
#include <PhysX/Utils.h>
#include <PhysXCharacters/API/CharacterController.h>
#include <PhysXCharacters/API/CharacterUtils.h>
#include <System/PhysXSystem.h>
#include <PhysX/Joint/Configuration/PhysXJointConfiguration.h>
#include <PhysX/Debug/PhysXDebugConfiguration.h>
#include <PhysX/MathConversion.h>
#include <Joint/PhysXJoint.h>

#include <AzCore/Console/IConsole.h>
#include <AzCore/Debug/ProfilerBus.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/containers/variant.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Task/TaskGraph.h>
#include <AzFramework/Physics/Character.h>
#include <AzFramework/Physics/Collision/CollisionEvents.h>
#include <AzFramework/Physics/Configuration/RigidBodyConfiguration.h>
#include <AzFramework/Physics/Configuration/StaticRigidBodyConfiguration.h>
#include <AzFramework/Physics/Material/PhysicsMaterialManager.h>

namespace PhysX
{
    AZ_CVAR_EXTERNED(bool, physx_batchTransformSync);

    AZ_CVAR(bool, physx_parallelTransformSync, true, nullptr, AZ::ConsoleFunctorFlags::Null, "Multithreaded transform update for rigid bodies. "
        "Only relevant if batched transform update is enabled.");
    AZ_CVAR(size_t, physx_parallelTransformSyncBatchSize, 250, nullptr, AZ::ConsoleFunctorFlags::Null,
        "How many rigid bodies should be processed per task");

    AZ_CLASS_ALLOCATOR_IMPL(PhysXScene, AZ::SystemAllocator, 0);

    /*static*/ thread_local AZStd::vector<physx::PxRaycastHit> PhysXScene::s_rayCastBuffer;
    /*static*/ thread_local AZStd::vector<physx::PxSweepHit> PhysXScene::s_sweepBuffer;
    /*static*/ thread_local AZStd::vector<physx::PxOverlapHit> PhysXScene::s_overlapBuffer;

    namespace Internal
    {
        physx::PxScene* CreatePxScene(const AzPhysics::SceneConfiguration& config,
            SceneSimulationFilterCallback* filterCallback,
            SceneSimulationEventCallback* simEventCallback)
        {
            const physx::PxTolerancesScale tolerancesScale = physx::PxTolerancesScale();
            physx::PxSceneDesc sceneDesc(tolerancesScale);
            sceneDesc.gravity = PxMathConvert(config.m_gravity);
            if (config.m_enableCcd)
            {
                sceneDesc.flags |= physx::PxSceneFlag::eENABLE_CCD;
                sceneDesc.filterShader = Collision::DefaultFilterShaderCCD;
                sceneDesc.ccdMaxPasses = config.m_maxCcdPasses;
                if (config.m_enableCcdResweep)
                {
                    sceneDesc.flags.clear(physx::PxSceneFlag::eDISABLE_CCD_RESWEEP);
                }
                else
                {
                    sceneDesc.flags.set(physx::PxSceneFlag::eDISABLE_CCD_RESWEEP);
                }
            }
            else
            {
                sceneDesc.filterShader = Collision::DefaultFilterShader;
            }

            if (config.m_enableActiveActors)
            {
                sceneDesc.flags |= physx::PxSceneFlag::eENABLE_ACTIVE_ACTORS;
            }

            if (config.m_enablePcm)
            {
                sceneDesc.flags |= physx::PxSceneFlag::eENABLE_PCM;
            }
            else
            {
                sceneDesc.flags &= ~physx::PxSceneFlag::eENABLE_PCM;
            }

            if (config.m_kinematicFiltering)
            {
                sceneDesc.kineKineFilteringMode = physx::PxPairFilteringMode::eKEEP;
            }

            if (config.m_kinematicStaticFiltering)
            {
                sceneDesc.staticKineFilteringMode = physx::PxPairFilteringMode::eKEEP;
            }

            sceneDesc.bounceThresholdVelocity = config.m_bounceThresholdVelocity;

            sceneDesc.filterCallback = filterCallback;
            sceneDesc.simulationEventCallback = simEventCallback;
            #ifdef ENABLE_TGS_SOLVER
                // Use Temporal Gauss-Seidel solver by default
                sceneDesc.solverType = physx::PxSolverType::eTGS;
            #endif
            #ifdef PHYSX_ENABLE_MULTI_THREADING
                sceneDesc.flags |= physx::PxSceneFlag::eREQUIRE_RW_LOCK;
            #endif

            if (auto* physXSystem = GetPhysXSystem())
            {
                sceneDesc.cpuDispatcher = physXSystem->GetPxCpuDispathcher();
                if (physx::PxScene * pxScene = physXSystem->GetPxPhysics()->createScene(sceneDesc))
                {
                    if (physx::PxPvdSceneClient* pvdClient = pxScene->getScenePvdClient())
                    {
                        pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
                        pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
                        pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
                    }
                    return pxScene;
                }
            }
            return nullptr;
        }

        bool AddShape(AZStd::variant<AzPhysics::RigidBody*, AzPhysics::StaticRigidBody*> simulatedBody, const AzPhysics::ShapeVariantData& shapeData)
        {
            if (const auto* shapeColliderPair = AZStd::get_if<AzPhysics::ShapeColliderPair>(&shapeData))
            {
                bool shapeAdded = false;
                auto shapePtr = AZStd::make_shared<Shape>(*(shapeColliderPair->first), *(shapeColliderPair->second));
                AZStd::visit([shapePtr, &shapeAdded](auto&& body)
                    {
                        if (shapePtr->GetPxShape())
                        {
                            body->AddShape(shapePtr);
                            shapeAdded = true;
                        }
                    }, simulatedBody);
                return shapeAdded;
            }
            else if (const auto* shapeColliderPairList = AZStd::get_if<AZStd::vector<AzPhysics::ShapeColliderPair>>(&shapeData))
            {
                bool shapeAdded = false;
                for (const auto& shapeColliderConfigs : *shapeColliderPairList)
                {
                    auto shapePtr = AZStd::make_shared<Shape>(*(shapeColliderConfigs.first), *(shapeColliderConfigs.second));
                    AZStd::visit([shapePtr, &shapeAdded](auto&& body)
                        {
                            if (shapePtr->GetPxShape())
                            {
                                body->AddShape(shapePtr);
                                shapeAdded = true;
                            }
                        }, simulatedBody);
                }
                return shapeAdded;
            }
            else if (const auto* shape = AZStd::get_if<AZStd::shared_ptr<Physics::Shape>>(&shapeData))
            {
                auto shapePtr = *shape;
                AZStd::visit([shapePtr](auto&& body)
                    {
                        body->AddShape(shapePtr);
                    }, simulatedBody);
                return true;
            }
            else if (const auto* shapeList = AZStd::get_if<AZStd::vector<AZStd::shared_ptr<Physics::Shape>>>(&shapeData))
            {
                for (auto shapePtr : *shapeList)
                {
                    AZStd::visit([shapePtr](auto&& body)
                        {
                            body->AddShape(shapePtr);
                        }, simulatedBody);
                }
                return true;
            }
            return false;
        }

        template<class SimulatedBodyType, class ConfigurationType>
        AzPhysics::SimulatedBody* CreateSimulatedBody(const ConfigurationType* configuration, AZ::Crc32& crc)
        {
            SimulatedBodyType* newBody = aznew SimulatedBodyType(*configuration);
            if (!AZStd::holds_alternative<AZStd::monostate>(configuration->m_colliderAndShapeData))
            {
                [[maybe_unused]] const bool shapeAdded = AddShape(newBody, configuration->m_colliderAndShapeData);
                AZ_Warning("PhysXScene", shapeAdded, "No Collider or Shape information found when creating Rigid body [%s]", configuration->m_debugName.c_str());
            }
            crc = AZ::Crc32(newBody, sizeof(*newBody));
            return newBody;
        }

        AzPhysics::SimulatedBody* CreateRigidBody(const AzPhysics::RigidBodyConfiguration* configuration, AZ::Crc32& crc)
        {
            RigidBody* newBody = aznew RigidBody(*configuration);
            if (!AZStd::holds_alternative<AZStd::monostate>(configuration->m_colliderAndShapeData))
            {
                [[maybe_unused]] const bool shapeAdded = AddShape(newBody, configuration->m_colliderAndShapeData);
                AZ_Warning("PhysXScene", shapeAdded, "No Collider or Shape information found when creating Rigid body [%s]", configuration->m_debugName.c_str());
            }
            const AzPhysics::MassComputeFlags& flags = configuration->GetMassComputeFlags();
            newBody->UpdateMassProperties(flags, configuration->m_centerOfMassOffset,
                configuration->m_inertiaTensor, configuration->m_mass);

            crc = AZ::Crc32(newBody, sizeof(*newBody));
            return newBody;
        }

        AzPhysics::SimulatedBody* CreateCharacterBody(PhysXScene* scene,
            const Physics::CharacterConfiguration* characterConfig)
        {
            CharacterController* controller = Utils::Characters::CreateCharacterController(scene, *characterConfig);
            if (controller == nullptr)
            {
                AZ_Error("PhysXScene", false, "Failed to create character controller.");
                return nullptr;
            }
            controller->EnablePhysics(*characterConfig);
            controller->SetBasePosition(characterConfig->m_position);

            for (auto shape : characterConfig->m_colliders)
            {
                controller->AttachShape(shape);
            }

            return controller;
        }

        AzPhysics::SimulatedBody* CreateRagdollBody(PhysXScene* scene,
            const Physics::RagdollConfiguration* ragdollConfig)
        {
            return Utils::Characters::CreateRagdoll(*ragdollConfig,
                scene->GetSceneHandle());
        }

        template<class JointType, class ConfigurationType>
        AzPhysics::Joint* CreateJoint(const ConfigurationType* configuration,
            AzPhysics::SceneHandle sceneHandle,
            AzPhysics::SimulatedBodyHandle parentBodyHandle,
            AzPhysics::SimulatedBodyHandle childBodyHandle,
            AZ::Crc32& crc)
        {
            JointType* newBody = aznew JointType(*configuration, sceneHandle, parentBodyHandle, childBodyHandle);
            crc = AZ::Crc32(newBody, sizeof(*newBody));
            return newBody;
        }

        //helper to perform a ray cast
        AzPhysics::SceneQueryHits RayCast(const AzPhysics::RayCastRequest* raycastRequest,
            AZStd::vector<physx::PxRaycastHit>& raycastBuffer,
            physx::PxScene* physxScene,
            const physx::PxQueryFilterData queryData,
            const AZ::u64 sceneMaxResults)
        {
            // if this query need to report multiple hits, we need to prepare a buffer to hold up to the max allowed.
            // The filter should also use the eTOUCH flag to find all contacts with the ray.
            // Otherwise the default buffer (1 result) and eBLOCK flag is enough to find the first hit.
            physx::PxRaycastBuffer castResult;
            SceneQueryHelpers::PhysXQueryFilterCallback queryFilterCallback;
            if (raycastRequest->m_reportMultipleHits)
            {
                const AZ::u64 maxSize = AZStd::min(raycastRequest->m_maxResults, sceneMaxResults);
                if (raycastBuffer.size() < maxSize) //todo this needs to be limited by the config setting
                {
                    raycastBuffer.resize(maxSize);
                }
                castResult = physx::PxRaycastBuffer(raycastBuffer.begin(), aznumeric_cast<physx::PxU32>(maxSize));
                queryFilterCallback = SceneQueryHelpers::PhysXQueryFilterCallback(
                    raycastRequest->m_collisionGroup,
                    raycastRequest->m_filterCallback,
                    physx::PxQueryHitType::eTOUCH);
            }
            else
            {
                queryFilterCallback = SceneQueryHelpers::PhysXQueryFilterCallback(
                    raycastRequest->m_collisionGroup,
                    SceneQueryHelpers::GetSceneQueryBlockFilterCallback(raycastRequest->m_filterCallback),
                    physx::PxQueryHitType::eBLOCK);
            }

            const physx::PxVec3 orig = PxMathConvert(raycastRequest->m_start);
            const physx::PxVec3 dir = PxMathConvert(raycastRequest->m_direction.GetNormalized());
            const physx::PxHitFlags hitFlags = SceneQueryHelpers::GetPxHitFlags(raycastRequest->m_hitFlags);
            //Raycast
            bool status = false;
            {
                PHYSX_SCENE_READ_LOCK(physxScene);
                status = physxScene->raycast(orig, dir, raycastRequest->m_distance, castResult, hitFlags, queryData, &queryFilterCallback);
            }

            AzPhysics::SceneQueryHits hits;
            if (status)
            {
                if (castResult.hasBlock)
                {
                    hits.m_hits.emplace_back(SceneQueryHelpers::GetHitFromPxHit(castResult.block));
                }

                if (raycastRequest->m_reportMultipleHits)
                {
                    for (auto i = 0u; i < castResult.getNbTouches(); ++i)
                    {
                        const auto& pxHit = castResult.getTouch(i);
                        hits.m_hits.emplace_back(SceneQueryHelpers::GetHitFromPxHit(pxHit));
                    }
                }
            }
            return hits;
        }

        //helper to preform a shape cast
        AzPhysics::SceneQueryHits ShapeCast(const AzPhysics::ShapeCastRequest* shapecastRequest,
            AZStd::vector<physx::PxSweepHit>& shapecastBuffer,
            physx::PxScene* physxScene,
            const physx::PxQueryFilterData queryData,
            const AZ::u64 sceneMaxResults)
        {
            // if this query need to report multiple hits, we need to prepare a buffer to hold up to the max allowed.
            // The filter should also use the eTOUCH flag to find all contacts with the shape.
            // Otherwise the default buffer (1 result) and eBLOCK flag is enough to find the first hit.
            physx::PxSweepBuffer castResult;
            SceneQueryHelpers::PhysXQueryFilterCallback queryFilterCallback;
            if (shapecastRequest->m_reportMultipleHits)
            {
                const AZ::u64 maxSize = AZStd::min(shapecastRequest->m_maxResults, sceneMaxResults);
                if (shapecastBuffer.size() < maxSize) //todo this needs to be limited by the config setting
                {
                    shapecastBuffer.resize(maxSize);
                }
                castResult = physx::PxSweepBuffer(shapecastBuffer.begin(), aznumeric_cast<physx::PxU32>(maxSize));
                queryFilterCallback = SceneQueryHelpers::PhysXQueryFilterCallback(
                    shapecastRequest->m_collisionGroup,
                    shapecastRequest->m_filterCallback,
                    physx::PxQueryHitType::eTOUCH);
            }
            else
            {
                queryFilterCallback = SceneQueryHelpers::PhysXQueryFilterCallback(
                    shapecastRequest->m_collisionGroup,
                    SceneQueryHelpers::GetSceneQueryBlockFilterCallback(shapecastRequest->m_filterCallback),
                    physx::PxQueryHitType::eBLOCK);
            }

            physx::PxGeometryHolder pxGeometry;
            Utils::CreatePxGeometryFromConfig(*(shapecastRequest->m_shapeConfiguration), pxGeometry);

            AzPhysics::SceneQueryHits results;
            if (pxGeometry.any().getType() == physx::PxGeometryType::eSPHERE ||
                pxGeometry.any().getType() == physx::PxGeometryType::eBOX ||
                pxGeometry.any().getType() == physx::PxGeometryType::eCAPSULE ||
                pxGeometry.any().getType() == physx::PxGeometryType::eCONVEXMESH)
            {
                const physx::PxTransform pose = PxMathConvert(shapecastRequest->m_start);
                const physx::PxVec3 dir = PxMathConvert(shapecastRequest->m_direction.GetNormalized());
                AZ_Warning("PhysXScene", (static_cast<AZ::u16>(shapecastRequest->m_hitFlags & AzPhysics::SceneQuery::HitFlags::MTD) != 0),
                    "Not having MTD set for shape scene queries may result in incorrect reporting of colliders that are in contact or intersect the initial pose of the sweep.");
                const physx::PxHitFlags hitFlags = SceneQueryHelpers::GetPxHitFlags(shapecastRequest->m_hitFlags);

                bool status = false;
                {
                    PHYSX_SCENE_READ_LOCK(*physxScene);
                    status = physxScene->sweep(pxGeometry.any(), pose, dir, shapecastRequest->m_distance,
                        castResult, hitFlags, queryData, &queryFilterCallback);
                }

                if (status)
                {
                    if (castResult.hasBlock)
                    {
                        results.m_hits.emplace_back(SceneQueryHelpers::GetHitFromPxHit(castResult.block));
                    }

                    if (shapecastRequest->m_reportMultipleHits)
                    {
                        for (auto i = 0u; i < castResult.getNbTouches(); ++i)
                        {
                            const auto& pxHit = castResult.getTouch(i);
                            results.m_hits.emplace_back(SceneQueryHelpers::GetHitFromPxHit(pxHit));
                        }
                    }
                }
            }
            else
            {
                AZ_Warning("World", false, "Invalid geometry type passed to shape cast. Only sphere, box, capsule or convex mesh is supported");
            }

            return results;
        }

        bool OverlapGeneric(physx::PxScene* physxScene, const AzPhysics::OverlapRequest* overlapRequest,
            physx::PxOverlapCallback& overlapCallback, const physx::PxQueryFilterData& filterData)
        {
            // Prepare overlap data
            const physx::PxTransform pose = PxMathConvert(overlapRequest->m_pose);
            physx::PxGeometryHolder pxGeometry;
            Utils::CreatePxGeometryFromConfig(*(overlapRequest->m_shapeConfiguration), pxGeometry);

            SceneQueryHelpers::PhysXQueryFilterCallback filterCallback(
                overlapRequest->m_collisionGroup,
                SceneQueryHelpers::GetFilterCallbackFromOverlap(overlapRequest->m_filterCallback),
                physx::PxQueryHitType::eTOUCH);

            bool status = false;
            {
                PHYSX_SCENE_READ_LOCK(*physxScene);
                status = physxScene->overlap(pxGeometry.any(), pose, overlapCallback, filterData, &filterCallback);
            }
            return status;
        }

        AzPhysics::SceneQueryHits OverlapQuery(const AzPhysics::OverlapRequest* overlapRequest,
            AZStd::vector<physx::PxOverlapHit>& overlapBuffer,
            physx::PxScene* physxScene,
            const physx::PxQueryFilterData queryData,
            const AZ::u64 sceneMaxResults)
        {
            const AZ::u64 maxSize = AZStd::min(overlapRequest->m_maxResults, sceneMaxResults);
            if (overlapBuffer.size() < maxSize)
            {
                overlapBuffer.resize(maxSize);
            }

            if (overlapRequest->m_unboundedOverlapHitCallback)
            {
                SceneQueryHelpers::UnboundedOverlapCallback callback(overlapRequest->m_unboundedOverlapHitCallback, overlapBuffer);
                const bool status = OverlapGeneric(physxScene, overlapRequest, callback, queryData);
                if (status)
                {
                    return callback.m_results;
                }
                return {};
            }

            physx::PxOverlapBuffer queryHits(overlapBuffer.begin(), aznumeric_cast<physx::PxU32>(maxSize));
            bool status = OverlapGeneric(physxScene, overlapRequest, queryHits, queryData);

            AzPhysics::SceneQueryHits results;
            if (status)
            {
                // Process results
                AZ::u32 hitNum = queryHits.getNbAnyHits();
                results.m_hits.reserve(hitNum);
                for (AZ::u32 i = 0; i < hitNum; ++i)
                {
                    const AzPhysics::SceneQueryHit hit = SceneQueryHelpers::GetHitFromPxOverlapHit(queryHits.getAnyHit(i));
                    if (hit.IsValid())
                    {
                        results.m_hits.emplace_back(hit);
                    }
                }
                results.m_hits.shrink_to_fit();
            }
            return results;
        }
    }

    PhysXScene::PhysXScene(const AzPhysics::SceneConfiguration& config, const AzPhysics::SceneHandle& sceneHandle)
        : Scene(config)
        , m_config(config)
        , m_sceneHandle(sceneHandle)
        , m_physicsSystemConfigChanged([this](const AzPhysics::SystemConfiguration* config)
            {
                m_raycastBufferSize = config->m_raycastBufferSize;
                m_shapecastBufferSize = config->m_shapecastBufferSize;
                m_overlapBufferSize = config->m_overlapBufferSize;
            })
    {
        //setup the scene query buffer sizes
        if (auto* physXSystem = GetPhysXSystem())
        {
            if (const AzPhysics::SystemConfiguration* sysConfig = physXSystem->GetConfiguration())
            {
                m_raycastBufferSize = sysConfig->m_raycastBufferSize;
                m_shapecastBufferSize = sysConfig->m_shapecastBufferSize;
                m_overlapBufferSize = sysConfig->m_overlapBufferSize;
            }
            //register for future changes to the buffer sizes.
            physXSystem->RegisterSystemConfigurationChangedEvent(m_physicsSystemConfigChanged);
        }

        PhysXScene::s_rayCastBuffer = {};
        PhysXScene::s_sweepBuffer = {};
        PhysXScene::s_overlapBuffer = {};

        m_pxScene = Internal::CreatePxScene(m_config, &m_collisionFilterCallback, &m_simulationEventCallback);
        AZ_Assert(m_pxScene != nullptr, "PhysX::Scene creation failed.");

        m_pxScene->userData = this;

        m_gravity = m_config.m_gravity;
    }

    PhysXScene::~PhysXScene()
    {
        m_physicsSystemConfigChanged.Disconnect();

        s_overlapBuffer = {};
        s_rayCastBuffer = {};
        s_sweepBuffer = {};

        for (auto& simulatedBody : m_simulatedBodies)
        {
            if (simulatedBody.second != nullptr)
            {
                if (simulatedBody.second->m_simulating)
                {
                    // Disable simulation on body (not signaling OnSimulationBodySimulationDisabled event)
                    DisableSimulationOfBodyInternal(*simulatedBody.second);
                }
                m_simulatedBodyRemovedEvent.Signal(m_sceneHandle, simulatedBody.second->m_bodyHandle);
                delete simulatedBody.second;
            }
        }
        m_simulatedBodies.clear();

        ClearDeferedDeletions();

        if (m_controllerManager)
        {
            m_controllerManager->release();
            m_controllerManager = nullptr;
        }

        if (m_pxScene)
        {
            m_pxScene->release();
            m_pxScene = nullptr;
        }
    }

    void PhysXScene::StartSimulation(float deltatime)
    {
        AZ_PROFILE_SCOPE(Physics, "PhysXScene::StartSimulation");

        if (!IsEnabled())
        {
            return;
        }

        {
            AZ_PROFILE_SCOPE(Physics, "OnSceneSimulationStartEvent::Signaled");
            m_sceneSimulationStartEvent.Signal(m_sceneHandle, deltatime);
        }

        m_currentDeltaTime = deltatime;

        PHYSX_SCENE_WRITE_LOCK(m_pxScene);
        m_pxScene->simulate(deltatime);
    }

    void PhysXScene::FinishSimulation()
    {
        AZ_PROFILE_SCOPE(Physics, "PhysXScene::FinishSimulation");

        if (!IsEnabled())
        {
            return;
        }

        {
            AZ_PROFILE_SCOPE(Physics, "PhysXScene::CheckResults");

            // Wait for the simulation to complete.
            // In the multithreaded environment we need to make sure we don't lock the scene for write here.
            // This is because contact modification callbacks can be issued from the job threads and cause deadlock
            // due to the callback code locking the scene.
            // https://devtalk.nvidia.com/default/topic/1024408/pxcontactmodifycallback-and-pxscene-locking/
            m_pxScene->checkResults(true);
        }

        bool activeActorsEnabled = false;
        {
            AZ_PROFILE_SCOPE(Physics, "PhysXScene::FetchResults");
            PHYSX_SCENE_WRITE_LOCK(m_pxScene);

            activeActorsEnabled = m_pxScene->getFlags() & physx::PxSceneFlag::eENABLE_ACTIVE_ACTORS;

            // Swap the buffers, invoke callbacks, build the list of active actors.
            m_pxScene->fetchResults(true);
        }

        if (activeActorsEnabled)
        {
            AZ_PROFILE_SCOPE(Physics, "PhysXScene::ActiveActors");

            AzPhysics::SimulatedBodyHandleList activeBodyHandles;

            {
                PHYSX_SCENE_READ_LOCK(m_pxScene);
                physx::PxU32 numActiveActors = 0;
                physx::PxActor** activeActors = m_pxScene->getActiveActors(numActiveActors);
                activeBodyHandles.reserve(numActiveActors);
                for (physx::PxU32 i = 0; i < numActiveActors; ++i)
                {
                    if (ActorData* actorData = Utils::GetUserData(activeActors[i]))
                    {
                        activeBodyHandles.emplace_back(actorData->GetBodyHandle());
                    }
                }
            }

            // Keep the event signal outside of the scene lock since there may be handlers that want to lock the scene for write
            m_sceneActiveSimulatedBodies.Signal(m_sceneHandle, activeBodyHandles, m_currentDeltaTime);

            if (physx_batchTransformSync)
            {
                m_queuedActiveBodyIndices.IncreaseCapacity(activeBodyHandles.size());

                for (const AzPhysics::SimulatedBodyHandle& bodyHandle : activeBodyHandles)
                {
                    AzPhysics::SimulatedBodyIndex bodyIndex = AZStd::get<1>(bodyHandle);
                    m_queuedActiveBodyIndices.Insert(bodyIndex);
                }

                m_accumulatedDeltaTime += m_currentDeltaTime;
            }
            else
            {
                SyncActiveBodyTransform(activeBodyHandles);
            }
        }

        FlushQueuedEvents();
        ClearDeferedDeletions();

        {
            AZ_PROFILE_SCOPE(Physics, "OnSceneSimulationFinishedEvent::Signaled");
            m_sceneSimulationFinishEvent.Signal(m_sceneHandle, m_currentDeltaTime);
        }

        UpdateAzProfilerDataPoints();
    }

    void PhysXScene::FlushQueuedEvents()
    {
        //send queued trigger events
        ProcessTriggerEvents();

        //send queued collision events
        ProcessCollisionEvents();
    }

    void PhysXScene::SetEnabled(bool enable)
    {
        m_isEnabled = enable;
    }

    bool PhysXScene::IsEnabled() const
    {
        return m_isEnabled;
    }

    const AzPhysics::SceneConfiguration& PhysXScene::GetConfiguration() const
    {
        return m_config;
    }

    void PhysXScene::UpdateConfiguration(const AzPhysics::SceneConfiguration& config)
    {
        if (m_config != config)
        {
            m_config = config;
            m_configChangeEvent.Signal(m_sceneHandle, m_config);

            // set gravity verifies this is a new value.
            SetGravity(m_config.m_gravity);
        }
    }

    AzPhysics::SimulatedBodyHandle PhysXScene::AddSimulatedBody(const AzPhysics::SimulatedBodyConfiguration* simulatedBodyConfig)
    {
        AzPhysics::SimulatedBody* newBody = nullptr;
        AZ::Crc32 newBodyCrc;
        if (azrtti_istypeof<AzPhysics::RigidBodyConfiguration>(simulatedBodyConfig))
        {
            newBody = Internal::CreateRigidBody(
                azdynamic_cast<const AzPhysics::RigidBodyConfiguration*>(simulatedBodyConfig), newBodyCrc);
        }
        else if (azrtti_istypeof<AzPhysics::StaticRigidBodyConfiguration>(simulatedBodyConfig))
        {
            newBody = Internal::CreateSimulatedBody<StaticRigidBody, AzPhysics::StaticRigidBodyConfiguration>(
                azdynamic_cast<const AzPhysics::StaticRigidBodyConfiguration*>(simulatedBodyConfig), newBodyCrc);
        }
        else if (azrtti_istypeof<Physics::CharacterConfiguration>(simulatedBodyConfig))
        {
            newBody = Internal::CreateCharacterBody(this, azdynamic_cast<const Physics::CharacterConfiguration*>(simulatedBodyConfig));
        }
        else if (azrtti_istypeof<Physics::RagdollConfiguration>(simulatedBodyConfig))
        {
            newBody = Internal::CreateRagdollBody(this, azdynamic_cast<const Physics::RagdollConfiguration*>(simulatedBodyConfig));
        }
        else
        {
            AZ_Warning("PhysXScene", false, "Unknown SimulatedBodyConfiguration.");
            return AzPhysics::InvalidSimulatedBodyHandle;
        }

        if (newBody != nullptr)
        {
            AzPhysics::SimulatedBodyIndex index;

            if (m_freeSceneSlots.empty())
            {
                m_simulatedBodies.emplace_back(newBodyCrc, newBody);
                index = static_cast<AzPhysics::SimulatedBodyIndex>(m_simulatedBodies.size() - 1);
            }
            else
            {
                //fill any free slots first before increasing the size of the simulatedBodies vector.
                index = m_freeSceneSlots.front();
                m_freeSceneSlots.pop();
                AZ_Assert(index < m_simulatedBodies.size(), "PhysXScene::AddSimulatedBody: Free simulated body index is out of bounds");
                AZ_Assert(m_simulatedBodies[index].second == nullptr, "PhysXScene::AddSimulatedBody: Free simulated body index is not free");

                m_simulatedBodies[index] = AZStd::make_pair(newBodyCrc, newBody);
            }

            const AzPhysics::SimulatedBodyHandle newBodyHandle(newBodyCrc, index);
            newBody->m_sceneOwner = m_sceneHandle;
            newBody->m_bodyHandle = newBodyHandle;
            m_simulatedBodyAddedEvent.Signal(m_sceneHandle, newBodyHandle);

            // Enable simulation by default (not signaling OnSimulationBodySimulationEnabled event)
            if (simulatedBodyConfig->m_startSimulationEnabled)
            {
                EnableSimulationOfBodyInternal(*newBody);
            }

            return newBodyHandle;
        }

        return AzPhysics::InvalidSimulatedBodyHandle;
    }

    AzPhysics::SimulatedBodyHandleList PhysXScene::AddSimulatedBodies(const AzPhysics::SimulatedBodyConfigurationList& simulatedBodyConfigs)
    {
        AzPhysics::SimulatedBodyHandleList newBodyHandles;
        newBodyHandles.reserve(simulatedBodyConfigs.size());
        for (auto* config : simulatedBodyConfigs)
        {
            newBodyHandles.emplace_back(AddSimulatedBody(config));
        }
        return newBodyHandles;
    }

    AzPhysics::SimulatedBody* PhysXScene::GetSimulatedBodyFromHandle(AzPhysics::SimulatedBodyHandle bodyHandle)
    {
        if (bodyHandle == AzPhysics::InvalidSimulatedBodyHandle)
        {
            return nullptr;
        }

        AzPhysics::SimulatedBodyIndex index = AZStd::get<AzPhysics::HandleTypeIndex::Index>(bodyHandle);
        if (index < m_simulatedBodies.size()
            && m_simulatedBodies[index].first == AZStd::get<AzPhysics::HandleTypeIndex::Crc>(bodyHandle))
        {
            return m_simulatedBodies[index].second;
        }
        return nullptr;
    }

    AzPhysics::SimulatedBodyList PhysXScene::GetSimulatedBodiesFromHandle(const AzPhysics::SimulatedBodyHandleList& bodyHandles)
    {
        AzPhysics::SimulatedBodyList results;
        for (auto& handle : bodyHandles)
        {
            results.emplace_back(GetSimulatedBodyFromHandle(handle));
        }
        return results;
    }

    void PhysXScene::RemoveSimulatedBody(AzPhysics::SimulatedBodyHandle& bodyHandle)
    {
        if (bodyHandle == AzPhysics::InvalidSimulatedBodyHandle)
        {
            return;
        }

        AzPhysics::SimulatedBodyIndex index = AZStd::get<AzPhysics::HandleTypeIndex::Index>(bodyHandle);
        if (index < m_simulatedBodies.size()
            && m_simulatedBodies[index].first == AZStd::get<AzPhysics::HandleTypeIndex::Crc>(bodyHandle))
        {
            if (m_simulatedBodies[index].second->m_simulating)
            {
                // Disable simulation on body (not signaling OnSimulationBodySimulationDisabled event)
                DisableSimulationOfBodyInternal(*m_simulatedBodies[index].second);
            }

            m_simulatedBodyRemovedEvent.Signal(m_sceneHandle, bodyHandle);

            m_deferredDeletions.push_back(m_simulatedBodies[index].second);
            m_simulatedBodies[index] = AZStd::make_pair(AZ::Crc32(), nullptr);
            m_freeSceneSlots.push(index);

            bodyHandle = AzPhysics::InvalidSimulatedBodyHandle;
        }
    }

    void PhysXScene::RemoveSimulatedBodies(AzPhysics::SimulatedBodyHandleList& bodyHandles)
    {
        for (auto& handle: bodyHandles)
        {
            RemoveSimulatedBody(handle);
        }
    }

    void PhysXScene::EnableSimulationOfBody(AzPhysics::SimulatedBodyHandle bodyHandle)
    {
        if (bodyHandle == AzPhysics::InvalidSimulatedBodyHandle)
        {
            return;
        }

        if (AzPhysics::SimulatedBody* body = GetSimulatedBodyFromHandle(bodyHandle))
        {
            if (body->m_simulating)
            {
                return;
            }

            m_simulatedBodySimulationEnabledEvent.Signal(m_sceneHandle, bodyHandle);

            EnableSimulationOfBodyInternal(*body);
        }
        else
        {
            AZ_Warning("PhysXScene", false, "Unable to enable Simulated body, failed to find body.")
        }
    }

    void PhysXScene::DisableSimulationOfBody(AzPhysics::SimulatedBodyHandle bodyHandle)
    {
        if (bodyHandle == AzPhysics::InvalidSimulatedBodyHandle)
        {
            return;
        }

        if (AzPhysics::SimulatedBody* body = GetSimulatedBodyFromHandle(bodyHandle))
        {
            if (!body->m_simulating)
            {
                return;
            }

            m_simulatedBodySimulationDisabledEvent.Signal(m_sceneHandle, bodyHandle);

            DisableSimulationOfBodyInternal(*body);
        }
        else
        {
            AZ_Warning("PhysXScene", false, "Unable to disable Simulated body, failed to find body.")
        }
    }

    AzPhysics::JointHandle PhysXScene::AddJoint(const AzPhysics::JointConfiguration* jointConfig,
        AzPhysics::SimulatedBodyHandle parentBody, AzPhysics::SimulatedBodyHandle childBody)
    {
        AzPhysics::Joint* newJoint = nullptr;
        AZ::Crc32 newJointCrc;
        if (azrtti_istypeof<PhysX::D6JointLimitConfiguration>(jointConfig))
        {
            newJoint = Internal::CreateJoint<PhysXD6Joint, D6JointLimitConfiguration>(
                azdynamic_cast<const D6JointLimitConfiguration*>(jointConfig),
                m_sceneHandle, parentBody, childBody, newJointCrc);
        }
        else if (azrtti_istypeof<PhysX::FixedJointConfiguration*>(jointConfig))
        {
            newJoint = Internal::CreateJoint<PhysXFixedJoint, FixedJointConfiguration>(
                azdynamic_cast<const FixedJointConfiguration*>(jointConfig),
                m_sceneHandle, parentBody, childBody, newJointCrc);
        }
        else if (azrtti_istypeof<PhysX::BallJointConfiguration*>(jointConfig))
        {
            newJoint = Internal::CreateJoint<PhysXBallJoint, BallJointConfiguration>(
                azdynamic_cast<const BallJointConfiguration*>(jointConfig),
                m_sceneHandle, parentBody, childBody, newJointCrc);
        }
        else if (azrtti_istypeof<PhysX::HingeJointConfiguration*>(jointConfig))
        {
            newJoint = Internal::CreateJoint<PhysXHingeJoint, HingeJointConfiguration>(
                azdynamic_cast<const HingeJointConfiguration*>(jointConfig),
                m_sceneHandle, parentBody, childBody, newJointCrc);
        }
        else if (azrtti_istypeof<PhysX::PrismaticJointConfiguration*>(jointConfig))
        {
            newJoint = Internal::CreateJoint<PhysXPrismaticJoint, PrismaticJointConfiguration>(
                azdynamic_cast<const PrismaticJointConfiguration*>(jointConfig),
                m_sceneHandle, parentBody, childBody, newJointCrc);
        }
        else
        {
            AZ_Warning("PhysXScene", false, "Unknown JointConfiguration.");
            return AzPhysics::InvalidJointHandle;
        }

        if (newJoint != nullptr)
        {
            AzPhysics::JointIndex index = static_cast<AzPhysics::JointIndex>(m_joints.size());
            m_joints.emplace_back(newJointCrc, newJoint);

            const AzPhysics::JointHandle newJointHandle(newJointCrc, index);
            newJoint->m_sceneOwner = m_sceneHandle;
            newJoint->m_jointHandle = newJointHandle;

            return newJointHandle;
        }

        return AzPhysics::InvalidJointHandle;
    }

    AzPhysics::Joint* PhysXScene::GetJointFromHandle(AzPhysics::JointHandle jointHandle)
    {
        if (jointHandle == AzPhysics::InvalidJointHandle)
        {
            return nullptr;
        }

        AzPhysics::JointIndex index = AZStd::get<AzPhysics::HandleTypeIndex::Index>(jointHandle);
        if (index < m_joints.size()
            && m_joints[index].first == AZStd::get<AzPhysics::HandleTypeIndex::Crc>(jointHandle))
        {
            return m_joints[index].second;
        }
        return nullptr;
    }

    void PhysXScene::RemoveJoint(AzPhysics::JointHandle jointHandle)
    {
        if (jointHandle == AzPhysics::InvalidJointHandle)
        {
            return;
        }

        AzPhysics::JointIndex index = AZStd::get<AzPhysics::HandleTypeIndex::Index>(jointHandle);
        if (index < m_joints.size()
            && m_joints[index].first == AZStd::get<AzPhysics::HandleTypeIndex::Crc>(jointHandle))
        {
            m_deferredDeletionsJoints.push_back(m_joints[index].second);
            m_joints[index] = AZStd::make_pair(AZ::Crc32(), nullptr);
            m_freeJointSlots.push(index);
            jointHandle = AzPhysics::InvalidJointHandle;
        }
    }

    AzPhysics::SceneQueryHits PhysXScene::QueryScene(const AzPhysics::SceneQueryRequest* request)
    {
        if (request == nullptr)
        {
            return {}; //return 0 hits
        }

        // Query flags.
        const physx::PxQueryFlags queryFlags = SceneQueryHelpers::GetPxQueryFlags(request->m_queryType);
        const physx::PxQueryFilterData queryData(queryFlags);

        if (azrtti_istypeof<AzPhysics::RayCastRequest>(request))
        {
            return Internal::RayCast(azdynamic_cast<const AzPhysics::RayCastRequest*>(request),
                s_rayCastBuffer, m_pxScene, queryData, m_raycastBufferSize);
        }
        else if (azrtti_istypeof<AzPhysics::ShapeCastRequest>(request))
        {
            return Internal::ShapeCast(azdynamic_cast<const AzPhysics::ShapeCastRequest*>(request),
                s_sweepBuffer, m_pxScene, queryData, m_shapecastBufferSize);
        }
        else if (azrtti_istypeof<AzPhysics::OverlapRequest>(request))
        {
            return Internal::OverlapQuery(azdynamic_cast<const AzPhysics::OverlapRequest*>(request),
                s_overlapBuffer, m_pxScene, queryData, m_overlapBufferSize);
        }
        else
        {
            AZ_Warning("Physx", false, "Unknown Scene Query request type.");
        }
        return AzPhysics::SceneQueryHits();
    }

    AzPhysics::SceneQueryHitsList PhysXScene::QuerySceneBatch(const AzPhysics::SceneQueryRequests& requests)
    {
        AzPhysics::SceneQueryHitsList results;
        results.reserve(requests.size());
        for (auto& request : requests)
        {
            results.emplace_back(QueryScene(request.get()));
        }
        return results;
    }

    [[nodiscard]] bool PhysXScene::QuerySceneAsync([[maybe_unused]] AzPhysics::SceneQuery::AsyncRequestId requestId,
        [[maybe_unused]] const AzPhysics::SceneQueryRequest* request, [[maybe_unused]] AzPhysics::SceneQuery::AsyncCallback callback)
    {
        AZ_Warning("Physx", false, "Currently unimplemented."); // LYN-2306
        return false;
    }

    [[nodiscard]] bool PhysXScene::QuerySceneAsyncBatch([[maybe_unused]] AzPhysics::SceneQuery::AsyncRequestId requestId,
        [[maybe_unused]] const AzPhysics::SceneQueryRequests& requests, [[maybe_unused]] AzPhysics::SceneQuery::AsyncBatchCallback callback)
    {
        AZ_Warning("Physx", false, "Currently unimplemented."); // LYN-2306
        return false;
    }

    void PhysXScene::SuppressCollisionEvents(
        const AzPhysics::SimulatedBodyHandle& bodyHandleA,
        const AzPhysics::SimulatedBodyHandle& bodyHandleB)
    {
        AzPhysics::SimulatedBody* bodyA = GetSimulatedBodyFromHandle(bodyHandleA);
        AzPhysics::SimulatedBody* bodyB = GetSimulatedBodyFromHandle(bodyHandleB);
        if (bodyA != nullptr && bodyB != nullptr)
        {
            m_collisionFilterCallback.RegisterSuppressedCollision(bodyA, bodyB);
        }
    }

    void PhysXScene::UnsuppressCollisionEvents(
        const AzPhysics::SimulatedBodyHandle& bodyHandleA,
        const AzPhysics::SimulatedBodyHandle& bodyHandleB)
    {
        AzPhysics::SimulatedBody* bodyA = GetSimulatedBodyFromHandle(bodyHandleA);
        AzPhysics::SimulatedBody* bodyB = GetSimulatedBodyFromHandle(bodyHandleB);
        if (bodyA != nullptr && bodyB != nullptr)
        {
            m_collisionFilterCallback.UnregisterSuppressedCollision(bodyA, bodyB);
        }
    }

    void PhysXScene::SetGravity(const AZ::Vector3& gravity)
    {
        if (m_pxScene && !m_gravity.IsClose(gravity))
        {
            m_gravity = gravity;
            {
                PHYSX_SCENE_WRITE_LOCK(m_pxScene);
                m_pxScene->setGravity(PxMathConvert(m_gravity));
            }
            m_sceneGravityChangedEvent.Signal(m_sceneHandle, m_gravity);
        }
    }

    AZ::Vector3 PhysXScene::GetGravity() const
    {
        return m_gravity;
    }

    void PhysXScene::EnableSimulationOfBodyInternal(AzPhysics::SimulatedBody& body)
    {
        //character controller is a special actor and only needs the m_simulating flag set,
        if (!azrtti_istypeof<PhysX::CharacterController>(body) &&
            !azrtti_istypeof<PhysX::Ragdoll>(body))
        {
            auto pxActor = static_cast<physx::PxActor*>(body.GetNativePointer());
            AZ_Assert(pxActor, "Simulated Body doesn't have a valid physx actor");

            {
                PHYSX_SCENE_WRITE_LOCK(m_pxScene);
                m_pxScene->addActor(*pxActor);
            }

            if (azrtti_istypeof<PhysX::RigidBody>(body))
            {
                auto rigidBody = azdynamic_cast<PhysX::RigidBody*>(&body);
                if (rigidBody->ShouldStartAsleep())
                {
                    rigidBody->ForceAsleep();
                }
            }
        }

        body.m_simulating = true;
    }

    void PhysXScene::DisableSimulationOfBodyInternal(AzPhysics::SimulatedBody& body)
    {
        //character controller is a special actor and only needs the m_simulating flag set,
        if (!azrtti_istypeof<PhysX::CharacterController>(body) &&
            !azrtti_istypeof<PhysX::Ragdoll>(body))
        {
            auto pxActor = static_cast<physx::PxActor*>(body.GetNativePointer());
            AZ_Assert(pxActor, "Simulated Body doesn't have a valid physx actor");

            {
                PHYSX_SCENE_WRITE_LOCK(m_pxScene);
                m_pxScene->removeActor(*pxActor);
            }
        }
        body.m_simulating = false;
    }

    physx::PxControllerManager* PhysXScene::GetOrCreateControllerManager()
    {
        if (m_controllerManager)
        {
            return m_controllerManager;
        }

        if (m_pxScene)
        {
            m_controllerManager = PxCreateControllerManager(*m_pxScene);
        }

        if (m_controllerManager)
        {
            m_controllerManager->setOverlapRecoveryModule(true);
        }
        else
        {
            AZ_Error("PhysX Character Controller System", false, "Unable to create a Controller Manager.");
        }

        return m_controllerManager;
    }

    void* PhysXScene::GetNativePointer() const
    {
        return m_pxScene;
    }

    void PhysXScene::ClearDeferedDeletions()
    {
        // swap the deletions in case the simulated body
        // manages more bodies and removes them on destruction (ie. Ragdoll).
        AZStd::vector<AzPhysics::SimulatedBody*> deletions;
        deletions.swap(m_deferredDeletions);
        for (auto* simulatedBody : deletions)
        {
            delete simulatedBody;
        }

        AZStd::vector<AzPhysics::Joint*> jointDeletions;
        jointDeletions.swap(m_deferredDeletionsJoints);
        for (auto* joint : jointDeletions)
        {
            delete joint;
        }
    }

    void PhysXScene::ProcessTriggerEvents()
    {
        AZ_PROFILE_SCOPE(Physics, "PhysXScene::ProcessTriggerEvents");

        AzPhysics::TriggerEventList& triggers = m_simulationEventCallback.GetQueuedTriggerEvents();
        if (triggers.empty())
        {
            return; // nothing to signal
        }
        m_sceneTriggerEvent.Signal(m_sceneHandle, triggers);

        for (auto& triggerEvent : triggers)
        {
            if (triggerEvent.m_triggerBody != nullptr)
            {
                triggerEvent.m_triggerBody->ProcessTriggerEvent(triggerEvent);
            }
            if (triggerEvent.m_otherBody != nullptr)
            {
                triggerEvent.m_otherBody->ProcessTriggerEvent(triggerEvent);
            }
        }

        //cleanup events for next simulate
        m_simulationEventCallback.FlushQueuedTriggerEvents();
    }

    void PhysXScene::ProcessCollisionEvents()
    {
        AZ_PROFILE_SCOPE(Physics, "PhysXScene::ProcessCollisionEvents");

        AzPhysics::CollisionEventList& collisions = m_simulationEventCallback.GetQueuedCollisionEvents();
        if (collisions.empty())
        {
            return; //nothing to signal
        }
        //send all event to any scene listeners
        m_sceneCollisionEvent.Signal(m_sceneHandle, collisions);

        //send events to each body listener
        for (auto& collision : collisions)
        {
            //trigger on body 1
            if (collision.m_body1 != nullptr)
            {
                collision.m_body1->ProcessCollisionEvent(collision);
            }
            //trigger for body 2
            if (collision.m_body2 != nullptr)
            {
                //swap the data as the event expects the trigger body to be body1.
                //this is ok to do as this event is no longer used after calling TriggerCollisionEvent
                AZStd::swap(collision.m_bodyHandle1, collision.m_bodyHandle2);
                AZStd::swap(collision.m_body1, collision.m_body2);
                AZStd::swap(collision.m_shape1, collision.m_shape2);
                collision.m_body1->ProcessCollisionEvent(collision);
            }
        }

        //cleanup events for next simulate
        m_simulationEventCallback.FlushQueuedCollisionEvents();
    }

    void PhysXScene::UpdateAzProfilerDataPoints()
    {
        using physx::PxGeometryType;

        bool isProfilingActive = false;
        if (auto profilerSystem = AZ::Debug::ProfilerSystemInterface::Get(); profilerSystem)
        {
            isProfilingActive = profilerSystem->IsActive();
        }

        if (!isProfilingActive)
        {
            return;
        }

        AZ_PROFILE_SCOPE(Physics, "PhysX::Statistics");

        physx::PxSimulationStatistics stats;

        {
            PHYSX_SCENE_READ_LOCK(m_pxScene);
            m_pxScene->getSimulationStatistics(stats);
        }

        [[maybe_unused]] const char* RootCategory = "PhysX/%s/%s";

        [[maybe_unused]] const char* ShapesSubCategory = "Shapes";
        AZ_PROFILE_DATAPOINT(Physics, stats.nbShapes[PxGeometryType::eSPHERE], RootCategory, ShapesSubCategory, "Sphere");
        AZ_PROFILE_DATAPOINT(Physics, stats.nbShapes[PxGeometryType::ePLANE], RootCategory, ShapesSubCategory, "Plane");
        AZ_PROFILE_DATAPOINT(Physics, stats.nbShapes[PxGeometryType::eCAPSULE], RootCategory, ShapesSubCategory, "Capsule");
        AZ_PROFILE_DATAPOINT(Physics, stats.nbShapes[PxGeometryType::eBOX], RootCategory, ShapesSubCategory, "Box");
        AZ_PROFILE_DATAPOINT(Physics, stats.nbShapes[PxGeometryType::eCONVEXMESH], RootCategory, ShapesSubCategory, "ConvexMesh");
        AZ_PROFILE_DATAPOINT(Physics, stats.nbShapes[PxGeometryType::eTRIANGLEMESH], RootCategory, ShapesSubCategory, "TriangleMesh");
        AZ_PROFILE_DATAPOINT(Physics, stats.nbShapes[PxGeometryType::eHEIGHTFIELD], RootCategory, ShapesSubCategory, "Heightfield");

        [[maybe_unused]] const char* ObjectsSubCategory = "Objects";
        AZ_PROFILE_DATAPOINT(Physics, stats.nbActiveConstraints, RootCategory, ObjectsSubCategory, "ActiveConstraints");
        AZ_PROFILE_DATAPOINT(Physics, stats.nbActiveDynamicBodies, RootCategory, ObjectsSubCategory, "ActiveDynamicBodies");
        AZ_PROFILE_DATAPOINT(Physics, stats.nbActiveKinematicBodies, RootCategory, ObjectsSubCategory, "ActiveKinematicBodies");
        AZ_PROFILE_DATAPOINT(Physics, stats.nbStaticBodies, RootCategory, ObjectsSubCategory, "StaticBodies");
        AZ_PROFILE_DATAPOINT(Physics, stats.nbDynamicBodies, RootCategory, ObjectsSubCategory, "DynamicBodies");
        AZ_PROFILE_DATAPOINT(Physics, stats.nbKinematicBodies, RootCategory, ObjectsSubCategory, "KinematicBodies");
        AZ_PROFILE_DATAPOINT(Physics, stats.nbAggregates, RootCategory, ObjectsSubCategory, "Aggregates");
        AZ_PROFILE_DATAPOINT(Physics, stats.nbArticulations, RootCategory, ObjectsSubCategory, "Articulations");

        [[maybe_unused]] const char* SolverSubCategory = "Solver";
        AZ_PROFILE_DATAPOINT(Physics, stats.nbAxisSolverConstraints, RootCategory, SolverSubCategory, "AxisSolverConstraints");
        AZ_PROFILE_DATAPOINT(Physics, stats.compressedContactSize, RootCategory, SolverSubCategory, "CompressedContactSize");
        AZ_PROFILE_DATAPOINT(Physics, stats.requiredContactConstraintMemory, RootCategory, SolverSubCategory, "RequiredContactConstraintMemory");
        AZ_PROFILE_DATAPOINT(Physics, stats.peakConstraintMemory, RootCategory, SolverSubCategory, "PeakConstraintMemory");

        [[maybe_unused]] const char* BroadphaseSubCategory = "Broadphase";
        AZ_PROFILE_DATAPOINT(Physics, stats.getNbBroadPhaseAdds(), RootCategory, BroadphaseSubCategory, "BroadPhaseAdds");
        AZ_PROFILE_DATAPOINT(Physics, stats.getNbBroadPhaseRemoves(), RootCategory, BroadphaseSubCategory, "BroadPhaseRemoves");

        // Compute pair stats for all geometry types
#if AZ_PROFILE_DATAPOINT
        AZ::u32 ccdPairs = 0;
        AZ::u32 modifiedPairs = 0;
        AZ::u32 triggerPairs = 0;
        for (AZ::u32 i = 0; i < PxGeometryType::eGEOMETRY_COUNT; i++)
        {
            // stat[i][j] = stat[j][i], hence, discarding the symmetric entries
            for (AZ::u32 j = i; j < PxGeometryType::eGEOMETRY_COUNT; j++)
            {
                const PxGeometryType::Enum firstGeom = static_cast<PxGeometryType::Enum>(i);
                const PxGeometryType::Enum secondGeom = static_cast<PxGeometryType::Enum>(j);
                ccdPairs += stats.getRbPairStats(physx::PxSimulationStatistics::eCCD_PAIRS, firstGeom, secondGeom);
                modifiedPairs += stats.getRbPairStats(physx::PxSimulationStatistics::eMODIFIED_CONTACT_PAIRS, firstGeom, secondGeom);
                triggerPairs += stats.getRbPairStats(physx::PxSimulationStatistics::eTRIGGER_PAIRS, firstGeom, secondGeom);
            }
        }
#endif

        [[maybe_unused]] const char* CollisionsSubCategory = "Collisions";
        AZ_PROFILE_DATAPOINT(Physics, ccdPairs, RootCategory, CollisionsSubCategory, "CCDPairs");
        AZ_PROFILE_DATAPOINT(Physics, modifiedPairs, RootCategory, CollisionsSubCategory, "ModifiedPairs");
        AZ_PROFILE_DATAPOINT(Physics, triggerPairs, RootCategory, CollisionsSubCategory, "TriggerPairs");
        AZ_PROFILE_DATAPOINT(Physics, stats.nbDiscreteContactPairsTotal, RootCategory, CollisionsSubCategory, "DiscreteContactPairsTotal");
        AZ_PROFILE_DATAPOINT(Physics, stats.nbDiscreteContactPairsWithCacheHits, RootCategory, CollisionsSubCategory, "DiscreteContactPairsWithCacheHits");
        AZ_PROFILE_DATAPOINT(Physics, stats.nbDiscreteContactPairsWithContacts, RootCategory, CollisionsSubCategory, "DiscreteContactPairsWithContacts");
        AZ_PROFILE_DATAPOINT(Physics, stats.nbNewPairs, RootCategory, CollisionsSubCategory, "NewPairs");
        AZ_PROFILE_DATAPOINT(Physics, stats.nbLostPairs, RootCategory, CollisionsSubCategory, "LostPairs");
        AZ_PROFILE_DATAPOINT(Physics, stats.nbNewTouches, RootCategory, CollisionsSubCategory, "NewTouches");
        AZ_PROFILE_DATAPOINT(Physics, stats.nbLostTouches, RootCategory, CollisionsSubCategory, "LostTouches");
        AZ_PROFILE_DATAPOINT(Physics, stats.nbPartitions, RootCategory, CollisionsSubCategory, "Partitions");
    }

    void PhysXScene::SyncActiveBodyTransform(const AzPhysics::SimulatedBodyHandleList& activeBodyHandles)
    {
        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            for (const AzPhysics::SimulatedBodyHandle& bodyHandle : activeBodyHandles)
            {
                if (AzPhysics::SimulatedBody* simBody = sceneInterface->GetSimulatedBodyFromHandle(m_sceneHandle, bodyHandle))
                {
                    simBody->SyncTransform(m_currentDeltaTime);
                }
            }
        }
    }

    void PhysXScene::FlushTransformSync()
    {
        AZ_PROFILE_SCOPE(Physics, "PhysX::FlushTransformSync");

        auto transformSync = [this](AzPhysics::SimulatedBodyIndex bodyIndex)
        {
            if (bodyIndex < m_simulatedBodies.size() && m_simulatedBodies[bodyIndex].second)
            {
                m_simulatedBodies[bodyIndex].second->SyncTransform(m_accumulatedDeltaTime);
            }
        };

        if (physx_parallelTransformSync)
        {
            m_queuedActiveBodyIndices.ApplyParallel(transformSync, m_pxScene);
        }
        else
        {
            m_queuedActiveBodyIndices.Apply(transformSync);
        }

        m_queuedActiveBodyIndices.Clear();
        m_accumulatedDeltaTime = 0.0f;
    }

    void PhysXScene::QueuedActiveBodyIndices::Insert(AzPhysics::SimulatedBodyIndex bodyIndex)
    {
        if (m_uniqueIndices.insert(bodyIndex).second)
        {
            m_packedIndices.emplace_back(bodyIndex);
        }
    }

    void PhysXScene::QueuedActiveBodyIndices::IncreaseCapacity(size_t extraSize)
    {
        m_packedIndices.reserve(m_packedIndices.size() + extraSize);
    }

    void PhysXScene::QueuedActiveBodyIndices::Clear()
    {
        m_uniqueIndices.clear();
        m_packedIndices.clear();
    }

    void PhysXScene::QueuedActiveBodyIndices::Apply(const AZStd::function<void(AzPhysics::SimulatedBodyIndex)>& applyFunction)
    {
        AZStd::for_each(m_packedIndices.begin(), m_packedIndices.end(), applyFunction);
    }

    void PhysXScene::QueuedActiveBodyIndices::ApplyParallel(const AZStd::function<void(AzPhysics::SimulatedBodyIndex)>& applyFunction, physx::PxScene* pxScene)
    {
        AZ::TaskGraph taskGraph("Parallel Sync");
        AZ::TaskGraphEvent finishEvent("Parallel sync event");

        {
            AZ_PROFILE_SCOPE(Physics, "Sync Setup");

            size_t batchSize = physx_parallelTransformSyncBatchSize;
            size_t fullSize = m_packedIndices.size();
            for (size_t i = 0; i < fullSize; i += batchSize)
            {
                AZ::TaskDescriptor taskDescriptor{"SyncTask", "Physics"};
                taskGraph.AddTask(
                    taskDescriptor,
                    [start = i, end = AZStd::min(i + batchSize, fullSize), &applyFunction, pxScene, this]()
                    {
                        AZ_PROFILE_SCOPE(Physics, "Sync Task");

                        // Note: It is important to keep the scene locked for read for the entire task execution.
                        // Otherwise the functions reading data from the rigid body will have to lock it locally.
                        // This causes a huge amount of context switches making the execution of each task ~20x slower. 
                        PHYSX_SCENE_READ_LOCK(pxScene);

                        for (size_t batchIndex = start; batchIndex < end; ++batchIndex)
                        {
                            applyFunction(m_packedIndices[batchIndex]);
                        }
                    });
            }

            taskGraph.Submit(&finishEvent);
        }

        finishEvent.Wait();
    }

} // namespace PhysX

