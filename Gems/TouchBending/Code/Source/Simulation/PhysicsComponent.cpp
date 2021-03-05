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
#include "TouchBending_precompiled.h"
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Jobs/JobFunction.h>

#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/World.h>
#include <AzFramework/Physics/Collision/CollisionGroups.h>
#include <AzFramework/Physics/Collision/CollisionLayers.h>

#include <PhysX/MathConversion.h>
#include <PhysX/Utils.h>
#include <PhysX/SystemComponentBus.h>
#include <PhysX/PhysXLocks.h>

#include "PhysicsComponent.h"

#include "PhysicalizedSkeleton.h"

using namespace physx;

namespace Physics
{
    struct TouchBendingTriggerHandle : public PhysX::BaseActorData
    {
        AZ_CLASS_ALLOCATOR(TouchBendingTriggerHandle, AZ::SystemAllocator, 0);
        TouchBendingTriggerHandle(PxActor* actor) : BaseActorData(PhysX::BaseActorType::TOUCHBENDING_TRIGGER, actor),
            m_callback(nullptr), m_callbackPrivateData(nullptr), m_staticTriggerActor(nullptr),
            m_enterTriggerCount(0), m_isSkeletonOwnedByEngine(false), m_skeleton(nullptr) {}

        ///We use this callback to ask the engine to build a Physics::SpineTree archetype for us.
        ///We also use it when the time comes to notify the engine that a PhsysicalizedSkeleton instance has been created and that the engine
        ///is supposed to create a CStatObjFoliage for it.
        ITouchBendingCallback* m_callback;

        ///This is private data coming from the Engine.
        ///It is basically a render node pointer.
        const void * m_callbackPrivateData;

        ///This is the PhysX actor that is used as collision trigger. When another actor touches
        ///this trigger a PhysicalizedSkeleton is built asynchronously on a Job.
        PxRigidStatic* m_staticTriggerActor;

        ///Keeps track of how many actors are inside m_staticTriggerActor.
        ///In case a PhysicalizedSkeleton is created when m_staticTriggerActor is touched, the engine will keep
        ///the PhysicalizedSkeleton alive in memory so long as this variable is greater than zero. Once it reaches zero the engine will decrement its lifetime
        ///until it reaches zero and the engine will call DephysicalizeTouchBendingSkeleton() in order to get the PhysicalizedSkeleton removed from memory.
        ///See TouchBendingCVegetationAgent::UpdateFoliage()
        AZ::u16 m_enterTriggerCount;

        ///If this is FALSE, m_skeleton should be deleted when this object is deleted.
        ///Otherwise, it is the responsibility of the engine to call  DephysicalizeTouchBendingSkeleton(...)
        ///to remove the PhysicalizedSkeleton from memory.
        bool m_isSkeletonOwnedByEngine; 

        ///World transform, with uniform scale, for this instance.
        AZ::Transform m_worldTransform;

        /// This is a pointer to the physicalized skeleton-like structure that is created when
        /// m_staticTriggerActor is touched.
        /// After being added to the Engine via ITouchBendingCallback::OnPhysicalizedTouchBendingSkeleton() it lives in simulation memory for as long as
        /// m_enterTriggerCount is greater than zero or its lifetime is greater than zero. The lifetime is managed by CStatObjFoliage (in the engine).
        /// REMARK: if \p m_isSkeletonOwnedByEngine is FALSE, it is the responsibility of the TouchBending Gem to remove this skeleton
        /// from memory.
        TouchBending::Simulation::PhysicalizedSkeleton* m_skeleton;

        ///Shared archetype instance of a SpineTree. The PhysicalizedSkeleton obejects are built from this archetype.
        ///This data is built from CStatObj which is also an archetype. Physics::SpineTree
        ///is the TouchBending Gem equivalent of SSpine found inside CStatObj. 
        AZStd::shared_ptr<Physics::SpineTree> m_spineTreeArchetype;
    }; //struct TouchBendingTriggerHandle
}; //namespace Physics

namespace TouchBending
{
    namespace Simulation
    {
        PhysicsComponent::PhysicsComponent()
        {

        }

        void PhysicsComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("TouchBendingPhysicsService", 0x7cb7696e));
        }

        void PhysicsComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("TouchBendingPhysicsService", 0x7cb7696e));
        }

        void PhysicsComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("PhysXService", 0x75beae2d));
        }

        void PhysicsComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            (void)dependent;
        }

        void PhysicsComponent::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<PhysicsComponent, AZ::Component>()->Version(1);
            }
        }

        void PhysicsComponent::Activate()
        {
            m_isRunningJob = false;

            PxPhysics& physics = PxGetPhysics();

            m_dummyMaterialForTriggers = physics.createMaterial(0, 0, 0);
            AZ_Assert(m_dummyMaterialForTriggers, "Failed to create default dummy material for triggers");

            m_commonMaterialForSkeletonBones = physics.createMaterial(0, 0, 0);
            AZ_Assert(m_commonMaterialForSkeletonBones, "Failed to create default common material for skeleton bones");

            CrySystemEventBus::Handler::BusConnect();
            AsyncSkeletonBuilderBus::Handler::BusConnect();
            Physics::TouchBendingBus::Handler::BusConnect();
            Physics::WorldNotificationBus::Handler::BusConnect(Physics::DefaultPhysicsWorldId);
        }

        void PhysicsComponent::Deactivate()
        {
            Physics::WorldNotificationBus::Handler::BusDisconnect();
            Physics::TouchBendingBus::Handler::BusDisconnect();
            AsyncSkeletonBuilderBus::Handler::BusDisconnect();
            CrySystemEventBus::Handler::BusDisconnect();

            UnloadPhysicsLevelData();

            if (m_dummyMaterialForTriggers)
            {
                m_dummyMaterialForTriggers->release();
            }
            if (m_commonMaterialForSkeletonBones)
            {
                m_commonMaterialForSkeletonBones->release();
            }

            if (m_physics)
            {
                m_physics->release();
                m_physics = nullptr;
            }
            if (m_foundation)
            {
                m_foundation->release();
                m_foundation = nullptr;
            }
        }

        void PhysicsComponent::UnloadPhysicsLevelData()
        {
            {
                AZStd::lock_guard<AZStd::mutex> actorsLock(m_triggerActorsLock);

                {
                    AZStd::lock_guard<AZStd::mutex> actorsWithTreeLock(m_triggerActorsWithSkeletonlock);
                    m_triggerActorsWithSkeletonReadyToBeAddedToScene.clear();
                } // Unlock m_triggerActorsWithTreelock

                for (Physics::TouchBendingTriggerHandle* triggerHandle : m_triggerActors)
                {
                    triggerHandle->m_spineTreeArchetype = nullptr;
                    if (PxScene* pxScene = triggerHandle->m_staticTriggerActor->getScene())
                    {
                        PHYSX_SCENE_WRITE_LOCK(pxScene);
                        pxScene->removeActor(*(triggerHandle->m_staticTriggerActor));
                    }

                    triggerHandle->m_staticTriggerActor->release();

                    if (triggerHandle->m_skeleton && !triggerHandle->m_isSkeletonOwnedByEngine)
                    {
                        delete triggerHandle->m_skeleton;
                    }
                    delete triggerHandle;
                }
                m_triggerActors.clear();

            } // Unlock m_triggerActorsLock

            if (m_world)
            {
                m_world->SetTriggerEventCallback(nullptr);
                m_world = nullptr;
            }
        }


        //*********************************************************************
        // Physics::TouchBendingBus::Handler START ****************************
        bool PhysicsComponent::IsTouchBendingEnabled() const
        {
            return true;
        }

        Physics::TouchBendingTriggerHandle* PhysicsComponent::CreateTouchBendingTrigger(const AZ::Transform& worldTransform,
            const AZ::Aabb&worldAabb, Physics::ITouchBendingCallback* callback, const void * callbackPrivateData)
        {
            if (!m_world)
            {
                Physics::DefaultWorldBus::BroadcastResult(m_world, &Physics::DefaultWorldRequests::GetDefaultWorld);
                if (!m_world)
                {
                    AZ_Error(Physics::AZ_TOUCH_BENDING_WINDOW, false, "Default physics world is missing, touch bending will be disabled");
                    return nullptr;
                }
                m_world->SetTriggerEventCallback(this);
                PxScene* pxSceneForDominanceGroup = (PxScene*)m_world->GetNativePointer();
                PHYSX_SCENE_WRITE_LOCK(pxSceneForDominanceGroup);
                pxSceneForDominanceGroup->setDominanceGroupPair(0, 1, PxDominanceGroupPair(0, 1));
            }

            //Create the static rigid body actor.
            PxVec3 halfExtents(worldAabb.GetXExtent() * 0.5f, worldAabb.GetYExtent() * 0.5f, worldAabb.GetZExtent() * 0.5f);
            AZ::Vector3 position = worldTransform.GetTranslation();
            //Find the center of the BOX, based on its world Up Basis rotation vector.
            AZ::Transform worldTransfromWithoutScale = worldTransform;
            worldTransfromWithoutScale.ExtractScale();
            const AZ::Vector3 worldUp = worldTransfromWithoutScale.GetBasisZ();
            const AZ::Vector3 worldUpOffset = worldUp * halfExtents.z;
            position += worldUpOffset;
            AZ::Quaternion orientation = worldTransform.GetRotation();
            PxTransform pxTransform(PxMathConvert(position),
                PxMathConvert(orientation).getNormalized());

            PxPhysics& physics = PxGetPhysics();
            PxRigidStatic* rigidStatic = physics.createRigidStatic(pxTransform);

            //REMARK: PhysX suggests to reuse Shapes for saving memory. At this moment we are not doing so.
#ifdef TOUCHBENDING_VISUALIZE_ENABLED
            const PxShapeFlags shapeFlags = PxShapeFlag::eVISUALIZATION | PxShapeFlag::eTRIGGER_SHAPE;
#else
            const PxShapeFlags shapeFlags = PxShapeFlag::eTRIGGER_SHAPE;
#endif
            PxShape* shape = physics.createShape(PxBoxGeometry(halfExtents), *m_dummyMaterialForTriggers, true, shapeFlags);
            PhysX::Utils::Collision::SetCollisionLayerAndGroup(shape, AzPhysics::CollisionLayer::TouchBend, AzPhysics::CollisionGroup::All);
            rigidStatic->attachShape(*shape);
            //We should decrement by one shape ownership
            shape->release();

            Physics::TouchBendingTriggerHandle* retHandle = aznew Physics::TouchBendingTriggerHandle(rigidStatic);
            retHandle->m_callback = callback;
            retHandle->m_callbackPrivateData = callbackPrivateData;
            retHandle->m_staticTriggerActor = rigidStatic;
            retHandle->m_worldTransform = worldTransform;

            //Add the body to the scene.
            PxScene* pxScene = (PxScene*)m_world->GetNativePointer();
            {
                PHYSX_SCENE_WRITE_LOCK(pxScene);
                pxScene->addActor(*rigidStatic);
            }

            AZStd::lock_guard<AZStd::mutex> lock(m_triggerActorsLock);
            m_triggerActors.insert(retHandle);

            //Create the shape as trigger
            return retHandle;
        }

        void PhysicsComponent::SetTouchBendingSkeletonVisibility(Physics::TouchBendingSkeletonHandle* skeletonHandle,
            bool isVisible, AZ::u32& skeletonBoneCountOut, AZ::u32& triggerTouchCountOut)
        {
            PhysicalizedSkeleton* skeleton = static_cast<PhysicalizedSkeleton*>(skeletonHandle);
            skeletonBoneCountOut = skeleton->GetBoneCount();
            PxScene* pxScene = (PxScene*)m_world->GetNativePointer();
            if (isVisible)
            {
                skeleton->AddToScene(*pxScene);
            }
            else
            {
                skeleton->RemoveFromScene(*pxScene);
            }
            const Physics::TouchBendingTriggerHandle* triggerHandle = skeleton->GetTriggerHandle();
            triggerTouchCountOut = triggerHandle ? triggerHandle->m_enterTriggerCount : 0;
        }

        void PhysicsComponent:: DeleteTouchBendingTrigger(Physics::TouchBendingTriggerHandle* handle)
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_triggerActorsLock);
            DeleteTouchBendingTriggerLocked(handle);
        }

        void PhysicsComponent::DephysicalizeTouchBendingSkeleton(Physics::TouchBendingSkeletonHandle* skeletonHandle)
        {
            AZ_Assert(skeletonHandle != nullptr, "There's no reason for skeletonHandle to be nullptr");
            PhysicalizedSkeleton* skeleton = static_cast<PhysicalizedSkeleton*>(skeletonHandle);

            if (PxScene* pxScene = skeleton->GetScene())
            {
                PHYSX_SCENE_WRITE_LOCK(pxScene);
                skeleton->RemoveFromScene(*pxScene);
            }

            Physics::TouchBendingTriggerHandle* triggerHandle = skeleton->GetTriggerHandle();
            if (triggerHandle)
            {
                triggerHandle->m_enterTriggerCount = 0;
                triggerHandle->m_skeleton = nullptr;
            }

            delete skeleton;

        }
            
        void PhysicsComponent::ReadJointPositionsOfSkeleton(Physics::TouchBendingSkeletonHandle* skeletonHandle,
            Physics::JointPositions* jointPositions)
        {
            PhysicalizedSkeleton* skeleton = static_cast<PhysicalizedSkeleton*>(skeletonHandle);

            PHYSX_SCENE_READ_LOCK(static_cast<physx::PxScene*>(m_world->GetNativePointer()));
            skeleton->ReadJointPositions(jointPositions);
        }

        // Physics::TouchBendingBus::Handler END ******************************
        //*********************************************************************




        //*********************************************************************
        // PhysX::IPhysxTriggerEventCallback START ***************************
        
        //This callback occurs on Main Thread. But it occurs somewhere in between physx::simulate() and
        //physx::fetchResult(), so all we will do is to queue up the events using EBUS
        //and later We will job dispatch the instantiation of the Trees.
        bool PhysicsComponent::OnTriggerCallback(physx::PxTriggerPair* triggerPair)
        {
            PhysX::BaseActorData* userData = (PhysX::BaseActorData*)triggerPair->triggerActor->userData;
            if (!userData)
            {
                return false;
            }
            const PhysX::BaseActorType triggerDataType = userData->GetType();
            if (triggerDataType != PhysX::BaseActorType::TOUCHBENDING_TRIGGER)
            {
                return false;
            }
            Physics::TouchBendingTriggerHandle* triggerHandle = static_cast<Physics::TouchBendingTriggerHandle*>(userData);
            if (triggerPair->status == physx::PxPairFlag::eNOTIFY_TOUCH_FOUND)
            {
                if (!triggerHandle->m_enterTriggerCount && !triggerHandle->m_skeleton)
                {
                    //Check if we are within camera visible radius. if true, we get a SpineTree Archetype Id we can use to build
                    //the Tree.
                    Physics::SpineTreeIDType spineTreeArchetypeId = triggerHandle->m_callback->CheckDistanceToCamera(triggerHandle->m_callbackPrivateData);
                    if (spineTreeArchetypeId)
                    {
                        AZStd::shared_ptr<Physics::SpineTree> spineTreeArchetype;
                        //If a Tree was built in the past on behalf of this triggerHandle, let's check if this is a new one.
                        if (triggerHandle->m_spineTreeArchetype)
                        {
                            Physics::SpineTreeIDType previousArchetypeId = triggerHandle->m_spineTreeArchetype->m_spineTreeId;
                            if (previousArchetypeId != spineTreeArchetypeId)
                            {
                                //We are using an obsolete spine tree archetype. Let's release it.
                                triggerHandle->m_spineTreeArchetype = nullptr;
                                bool wasPresent = false;
                                TryEraseSpineTreeArchetypeFromCache(previousArchetypeId, wasPresent);
                            }
                            else
                            {
                                spineTreeArchetype = triggerHandle->m_spineTreeArchetype;
                            }
                        }

                        if (!spineTreeArchetype)
                        {
                            spineTreeArchetype = m_spineTreeArchetypeCache[spineTreeArchetypeId].lock();
                            if (!spineTreeArchetype)
                            {
                                //Time to create a spineTree archetype and add it to the cache.
                                spineTreeArchetype = AZStd::make_shared<Physics::SpineTree>();
                                if (!triggerHandle->m_callback->BuildSpineTree(triggerHandle->m_callbackPrivateData, spineTreeArchetypeId, *spineTreeArchetype))
                                {
                                    AZ_Error(::Physics::AZ_TOUCH_BENDING_WINDOW, false, "Failed to build spine tree archetype");
                                    return true;
                                }
                                m_spineTreeArchetypeCache[spineTreeArchetypeId] = spineTreeArchetype;
                            }
                        }

                        //Let's enqueue a request to build the Tree from the SpineTree Archetype.
                        AsyncSkeletonBuilderBus::QueueBroadcast(&AsyncSkeletonBuilderRequest::AsyncBuildSkeleton, triggerHandle, triggerHandle->m_worldTransform, spineTreeArchetype);
                    }
                }
                triggerHandle->m_enterTriggerCount++;
            }
            else if (triggerPair->status == physx::PxPairFlag::eNOTIFY_TOUCH_LOST)
            {
                if (triggerHandle->m_enterTriggerCount > 0)
                {
                    triggerHandle->m_enterTriggerCount--;
                }
            }
            else
            {
                AZ_Warning(::Physics::AZ_TOUCH_BENDING_WINDOW, false, "Touch Bending Proximity Trigger with status different from TOUCH_FOUND and TOUCH_LOST.");
            }
            //The event has been consumed by Touch Bending.
            return true;
        }

        // PhysX::IPhysxTriggerEventCallback END *****************************
        //*********************************************************************




        //*********************************************************************
        // AsyncSkeletonBuilderBus::Handler START *************************

        void PhysicsComponent::AsyncBuildSkeleton(Physics::TouchBendingTriggerHandle* triggerHandle,
                                              AZ::Transform worldTransform,
                                              AZStd::shared_ptr<Physics::SpineTree> spineTreeArchetype)
        {
            //Let's do a preliminary check to see whether it is worth it or not
            //to build the PhysicalizedSkeleton. 99% of the time, the outcome of this early test
            //will be accurate. The other 1% of the time building the Tree would be simply a waste
            //of cpu cycles.
            PhysicalizedSkeleton* skeleton = aznew PhysicalizedSkeleton;
            if (!skeleton->BuildFromArchetype(PxGetPhysics(), *m_commonMaterialForSkeletonBones, worldTransform, spineTreeArchetype.get(), SegmentShapeType::CAPSULE))
            {
                delete skeleton;
                AZ_Error(::Physics::AZ_TOUCH_BENDING_WINDOW, false, "Failed to create skeleton from Archetype");
                return;
            }
            
            {
                AZStd::lock_guard<AZStd::mutex> actorlock(m_triggerActorsLock);

                //Is the proximity Trigger still alive?
                auto search = m_triggerActors.find(triggerHandle);
                if (search == m_triggerActors.end())
                {
                    delete skeleton;
                    return;
                }

                //Time to reacquire the lock, we are competing with the main thread because the engine
                //decrements the time of life of the PhysicalizedSkeleton, and when it reaches zero it  
                //calls DephysicalizeTouchBendingSkeleton() which ends up removing.
                {
                    AZStd::lock_guard<AZStd::mutex> lock(m_triggerActorsWithSkeletonlock);
                    if (!triggerHandle->m_skeleton)
                    {
                        //The skeleton is not owned by the engine yet. Once OnPrePhysicsSubtick() is called
                        //it will be owned by the engine.
                        triggerHandle->m_isSkeletonOwnedByEngine = false;
                        triggerHandle->m_skeleton = skeleton;
                        triggerHandle->m_spineTreeArchetype = spineTreeArchetype;
                        m_triggerActorsWithSkeletonReadyToBeAddedToScene.push_back(triggerHandle);
                    }
                    else
                    {
                        delete skeleton;
                        AZ_Warning(::Physics::AZ_TOUCH_BENDING_WINDOW, false, "A skeleton was already attached to triggerHandle=%p", triggerHandle);
                    }

                } //unlock m_triggerActorsWithTreelock

            } //unlock m_triggerActorsLock

        } //End of PhysicsComponent::AsyncBuildSkeleton

        // AsyncSkeletonBuilderBus::Handler END ***************************
        //*********************************************************************



        //*********************************************************************
        // Physics::SystemNotificationBus::Handler START **********************       
        void PhysicsComponent::OnPrePhysicsSubtick([[maybe_unused]] float fixedDeltaTime)
        {
            if (!m_world)
            {
                return;
            }

            AZStd::vector<Physics::TouchBendingTriggerHandle*> triggerActorsWithSkeletons;
            {
                //Are there any PhysicalizedSkeletons we should add to the Scene?
                AZStd::lock_guard<AZStd::mutex> lock(m_triggerActorsWithSkeletonlock);
                triggerActorsWithSkeletons.set_capacity(m_triggerActorsWithSkeletonReadyToBeAddedToScene.size());
                for (Physics::TouchBendingTriggerHandle* triggerHandle : m_triggerActorsWithSkeletonReadyToBeAddedToScene)
                {
                    //Has this instance been removed already?
                    auto search = m_triggerActors.find(triggerHandle);
                    if (search == m_triggerActors.end())
                    {
                        continue;
                    }
                    triggerActorsWithSkeletons.push_back(triggerHandle);
                }
                m_triggerActorsWithSkeletonReadyToBeAddedToScene.clear();
            } //Unlock m_triggerActorsWithTreelock

            for (Physics::TouchBendingTriggerHandle* triggerActor : triggerActorsWithSkeletons)
            {
                const bool success = triggerActor->m_callback->OnPhysicalizedTouchBendingSkeleton(triggerActor->m_callbackPrivateData, triggerActor->m_skeleton);
                AZ_Warning(Physics::AZ_TOUCH_BENDING_WINDOW, success, "Engine was not ready to physicalize actor %p with privateData %p", triggerActor, triggerActor->m_callbackPrivateData);
                triggerActor->m_isSkeletonOwnedByEngine = success;
                triggerActor->m_skeleton->SetTriggerHandle(triggerActor);
            }
        }

        void PhysicsComponent::OnPostPhysicsSubtick([[maybe_unused]] float fixedDeltaTime)
        {
            if (!m_world)
            {
                return;
            }

            //If there are Trigger events queued, let's kickoff a Job to handle the creation
            //of the SpineTree, etc
            const size_t eventCount = AsyncSkeletonBuilderBus::QueuedEventCount();
            if (eventCount && !m_isRunningJob)
            {
                auto lambda = [this]()
                {
                    AsyncSkeletonBuilderBus::ExecuteQueuedEvents();
                    m_isRunningJob = false;
                };
                m_isRunningJob = true;
                AZ::Job* job = AZ::CreateJobFunction(lambda, true, nullptr);
                job->Start();
            }
        }
        // Physics::SystemNotificationBus::Handler END ************************ 
        //*********************************************************************


        //*********************************************************************
        // CrySystemEventBus::Handler START ***********************************
        void PhysicsComponent::OnCrySystemInitialized(ISystem& system, const SSystemInitParams&)
        {
            system.GetISystemEventDispatcher()->RegisterListener(this);
        }

        void PhysicsComponent::OnCrySystemShutdown(ISystem& system)
        {
            system.GetISystemEventDispatcher()->RemoveListener(this);
        }
        // CrySystemEventBus::Handler END *************************************
        //*********************************************************************



        //*********************************************************************
        // ISystemEventListener START *****************************************
        void PhysicsComponent::OnSystemEvent(ESystemEvent event, [[maybe_unused]] UINT_PTR wparam, [[maybe_unused]] UINT_PTR lparam)
        {
            switch (event)
            {
            case ESystemEvent::ESYSTEM_EVENT_LEVEL_UNLOAD:
                //Release the reference to the default physics world because
                //CryAction/ActionGame.cpp CActionGame::Init(const SGameStartParams* pGameStartParams)
                //will attempt to create a new one and will fail
                //because we may would be holding a reference to the previous world.
                //UnloadPhysicsLevelData() is not called in this case to avoid issues
                //with AsyncBuildTree() which runs as AZ::Job.
                if (m_world)
                {
                    m_world->SetTriggerEventCallback(nullptr);
                    m_world = nullptr;
                }
                break;
            case ESystemEvent::ESYSTEM_EVENT_LEVEL_POST_UNLOAD:
                //PhysX Creates a new World per level, so we need to get rid of anything that's still registered, loaded,
                //queued up, etc, and get rid of the reference to m_world so it can be deleted.
                UnloadPhysicsLevelData();
                break;
            default:
                break;
            }
        }
        // ISystemEventListener END *******************************************
        //*********************************************************************

        bool PhysicsComponent::TryEraseSpineTreeArchetypeFromCache(Physics::SpineTreeIDType archetypeId, bool& wasPresentInTheCache)
        {
            auto iterator = m_spineTreeArchetypeCache.find(archetypeId);
            if (iterator == m_spineTreeArchetypeCache.end())
            {
                wasPresentInTheCache = false;
                return false;
            }

            wasPresentInTheCache = true;
            AZStd::shared_ptr<Physics::SpineTree> spineTreeArchetype = iterator->second.lock();
            if (!spineTreeArchetype)
            {
                //No Body Is Using the Archetype anymore. Let's erase the Pointer for good.
                m_spineTreeArchetypeCache.erase(iterator);
                return true;
            }
            //It seems somebody else is still referencing the archetype.
            return false;
        }

        void PhysicsComponent::DeleteTouchBendingTriggerLocked(Physics::TouchBendingTriggerHandle* triggerHandle)
        {
            auto triggerActorSearchIterator = m_triggerActors.find(triggerHandle);
            if (triggerActorSearchIterator == m_triggerActors.end())
            {
                //Could happen if we call UnloadPhysicsLevelData() before the engine calls to DeleteProximityTrigger()
                return;
            }

            if (triggerHandle->m_spineTreeArchetype)
            {
                Physics::SpineTreeIDType spineTreeId = triggerHandle->m_spineTreeArchetype->m_spineTreeId;
                triggerHandle->m_spineTreeArchetype = nullptr; //Decrease ref count.
                bool wasPresentInTheCache = false;
                if (!TryEraseSpineTreeArchetypeFromCache(spineTreeId, wasPresentInTheCache))
                {
                    AZ_Assert(wasPresentInTheCache, "A Valid Spine Tree Archetype was Not Found In The Cache");
                }
            }
            
            if (triggerHandle->m_skeleton)
            {
                if (!triggerHandle->m_isSkeletonOwnedByEngine)
                {
                    //If the the skeleton never got to be owned by the engine, then this gem is responsible for deleting it.
                    delete triggerHandle->m_skeleton;
                    triggerHandle->m_skeleton = nullptr;
                }
                else
                {
                    //The engine owns the skeleton. Only the engine can delete it by calling DephysicalizeTouchBendingSkeleton().
                    triggerHandle->m_skeleton->SetTriggerHandle(nullptr);
                }
            }

            if (PxScene* pxScene = triggerHandle->m_staticTriggerActor->getScene())
            {
                PHYSX_SCENE_WRITE_LOCK(pxScene);
                pxScene->removeActor(*(triggerHandle->m_staticTriggerActor));
            }

            m_triggerActors.erase(triggerActorSearchIterator);
            delete triggerHandle;
        }

    } // namespace Simulation
} // namespace TouchBending
