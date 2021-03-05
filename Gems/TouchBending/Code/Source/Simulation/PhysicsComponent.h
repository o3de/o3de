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

#include <PxPhysicsAPI.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzFramework/Physics/World.h>
#include <AzFramework/Physics/World.h>
#include <AzFramework/Physics/TouchBendingBus.h>
#include <AzFramework/Physics/WorldEventhandler.h>

#include <CrySystemBus.h>
#include <ISystem.h>

#include <PhysX/TriggerEventCallback.h>
#include "AsyncSkeletonBuilderBus.h"

namespace Physics
{
    struct TouchBendingTriggerHandle;
}

namespace TouchBending
{
    namespace Simulation
    {
        class PhysicalizedSkeleton;
        /// This is the main system component of the TouchBending Gem in charge of managing physics simulation
        /// of touch bendable vegetation. It is basically a liason between Touch Bendable CVegetation render nodes
        /// and the PhysX Gem.
        class PhysicsComponent
            : public AZ::Component
            , public Physics::TouchBendingBus::Handler
            , public Physics::WorldNotificationBus::Handler
            , public PhysX::IPhysxTriggerEventCallback
            , public AsyncSkeletonBuilderBus::Handler
            , private CrySystemEventBus::Handler
            , private ISystemEventListener
            

        {
        public:
            AZ_COMPONENT(PhysicsComponent, "{E3BE4294-1FC3-4B05-BD8F-7B6D96FE0CE1}", AZ::Component);

            PhysicsComponent();
            ~PhysicsComponent() override = default;

            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

            //AZ::Component
            void Activate() override;
            void Deactivate() override;

            //::Physics::TouchBendingBus::Handler START
            bool IsTouchBendingEnabled() const override;
            Physics::TouchBendingTriggerHandle* CreateTouchBendingTrigger(const AZ::Transform& worldTransform,
                const AZ::Aabb&worldAabb, Physics::ITouchBendingCallback* callback, const void * callbackPrivateData) override;
            void SetTouchBendingSkeletonVisibility(Physics::TouchBendingSkeletonHandle* skeletonHandle,
                bool isVisible, AZ::u32& skeletonBoneCountOut, AZ::u32& triggerTouchCountOut) override;
            void DeleteTouchBendingTrigger(Physics::TouchBendingTriggerHandle* handle) override;
            void DephysicalizeTouchBendingSkeleton(Physics::TouchBendingSkeletonHandle* skeletonHandle) override;
            void ReadJointPositionsOfSkeleton(Physics::TouchBendingSkeletonHandle* skeletonHandle, Physics::JointPositions* jointPositions) override;
            //::Physics::TouchBendingBus::Handler END

            //AsyncSkeletonBuilderBus::Handler START
            void AsyncBuildSkeleton(Physics::TouchBendingTriggerHandle* proximityTrigger,
                                AZ::Transform worldTransform,
                                AZStd::shared_ptr<Physics::SpineTree> spineTreeArchetype) override;
            //AsyncSkeletonBuilderBus::Handler END

            //::Physics::SystemNotificationBus::Handler START        
            void OnPrePhysicsSubtick(float fixedDeltaTime) override;
            void OnPostPhysicsSubtick(float fixedDeltaTime) override;
            //::Physics::SystemNotificationBus::Handler END

            //CrySystemEventBus::Handler START
            //So we can register for Level Load/Unload events.
            void OnCrySystemInitialized(ISystem&, const SSystemInitParams&) override;
            void OnCrySystemShutdown(ISystem&) override;
            //CrySystemEventBus::Handler END

            //ISystemEventListener START
            //Here we get level Load/Unload events
            void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;
            //ISystemEventListener END

            //PhysX::IPhysxTriggerEventCallback START
            bool OnTriggerCallback(physx::PxTriggerPair* triggerPair) override;
            //PhysX::IPhysxTriggerEventCallback END

        private:
            /// Completely removes a TouchBending Instance.
            void DeleteTouchBendingTriggerLocked(Physics::TouchBendingTriggerHandle* triggerHandle);

            /** @brief Tries to erase a SpineTree Archetype from the cache.
             *
             *  @param archetypeId Identifier of the archetype.
             *  @param wasPresentInTheCache [out] Set to TRUE upon return if the archetype ID was registered in the cache.
             *         Set to FALSE upon return if the archetype ID was not present in the cache at all.
             *  @returns TRUE if the archetype identifier was present in the cache
             *         and it was erased from the cache because its reference count was set to zero.
             *         Otherwise returns FALSE.
             */
            bool TryEraseSpineTreeArchetypeFromCache(Physics::SpineTreeIDType archetypeId, bool& wasPresentInTheCache);

            /// Clean up just the level-based physics data, but not any system-level data.
            void UnloadPhysicsLevelData();
            
            /// Reference to the physics world (aka PxScene) where all touch bending actors
            /// are added for simulation.
            AZStd::shared_ptr<Physics::World> m_world;

            /// Reference to the PhysX Foundation, needed for establishing global SDK pointers when using separated memory spaces
            physx::PxFoundation* m_foundation = nullptr;

            /// Reference to the PhysX Physics SDK, needed for establishing global SDK pointers when using separated memory spaces
            physx::PxPhysics* m_physics = nullptr;

            ///To create Shapes, even is they are used for trigger objects, you need
            ///a material. In this case, this material is used for proximity triggers,
            ///hence the prefix m_dummy...
            physx::PxMaterial* m_dummyMaterialForTriggers;

            /// Default PhysX material used by all Touch Bendable Rigid Actors.
            physx::PxMaterial* m_commonMaterialForSkeletonBones;

            /// Tree objects owned by TouchBendingTriggerHandle are built with AZ Jobs
            /// to alleviate pressure on main thread. We avoid creating too many jobs
            /// with this flag. If this is true, we simply add build requests implictily
            /// in the queue of  AsyncSkeletonBuilderBus. IF this is false we create an AZ Job
            /// and dispatch the Ebus events stored in AsyncSkeletonBuilderBus.
            bool m_isRunningJob;

            /// Adding or removing TouchBendingTriggerHandle objects is an uncommon operation,
            /// none the less it may happen that the engine wants to delete a CVegetation render node
            /// while at the same time we are trying to build a Tree for it on an AZ Job. With this mutex
            /// we prevent catastrophe.
            AZStd::mutex m_triggerActorsLock;

            /// In this set we keep track of all of the  TouchBendingTriggerHandle object that
            /// the engine ordered this Gem to instantiate. In other words, for each Touch Bendable
            /// CVegetation render node, one corresponding TouchBendingTriggerHandle object is created
            /// here in this Gem.
            AZStd::unordered_set<Physics::TouchBendingTriggerHandle*> m_triggerActors;

            /// We use this mutex to prevent race condition access to m_triggerActorsWithTreesReadyToBeAddedToScene,
            /// between AZ::Job and the main thread.
            AZStd::mutex m_triggerActorsWithSkeletonlock;

            /// As PhysicalizedSkeleton objects are completely built by the AZ::Job, they are added to this Vector for further
            /// dequeueing by the main thread on OnPrePhysicsSubtick().
            AZStd::vector<Physics::TouchBendingTriggerHandle*> m_triggerActorsWithSkeletonReadyToBeAddedToScene;

            /// This is a cache in the sense that the cache itself doesn't own the spineTrees,
            /// But so long as a trigger object owns a reference to the spine tree archetype,
            /// the archetype will remain in memory.
            AZStd::unordered_map<Physics::SpineTreeIDType, AZStd::weak_ptr<Physics::SpineTree>> m_spineTreeArchetypeCache;
        };
    } // namespace Physics
} // namespace TouchBending