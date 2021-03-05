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
#include <AzFramework/Physics/TouchBendingBus.h>
#include <PhysX/PhysXLocks.h>

namespace Physics
{
    struct TouchBendingSkeletonHandle
    {
    };
    struct TouchBendingTriggerHandle;
}

namespace TouchBending
{
    namespace Simulation
    {
        ///Shape of the bones.
        ///In Theory: SPHERE faster than CAPSULE,
        ///           CAPSULE faster than BOX.
        ///In Reality (From personal observations):
        ///           CAPSULE 0.1% faster than BOX,
        ///           BOX 1% faster than SPHERE.
        enum SegmentShapeType
        {
            BOX,
            CAPSULE,
            SPHERE,
        };

        struct BoneData
        {
            physx::PxRigidDynamic* m_rigidDynamicActor;
            float m_length;
        };

        struct SpineData
        {
            AZStd::vector<BoneData> m_bones;
        };

        /// This class represents a unique instance of a skeleton made of segments (PxRigidDynamic)
        /// attached to each other via D6Joints. When a collider touches the Trigger box owned by
        /// each TouchBendingInstanceHandle, one of these instances will be created temporarily  as a means
        /// to simulate bone movement that eventually will be fed to the Renderer for Skinning.
        class PhysicalizedSkeleton : public Physics::TouchBendingSkeletonHandle
        {
        public:
            AZ_CLASS_ALLOCATOR(PhysicalizedSkeleton, AZ::SystemAllocator, 0);
            PhysicalizedSkeleton() : m_aggregate(nullptr), m_spineTreeArchetype(nullptr), m_triggerHandle(nullptr), m_isPresentInTheScene(false) {}
            ~PhysicalizedSkeleton();

            
            /** @brief Creates all of the required PhysX actors and joints that resemble the structure of a skeleton.
             *
             *  A PhysicalizedSkeleton is related to an object of type TouchBendingTriggerHandle in the sense that when
             *  a  TouchBendingTriggerHandle is touched by a collider, a PhysicalizedSkeleton is created.
             *  If this method returns successfully, the PhysicalizedSkeleton can be added to or removed from the scene at will.
             *
             *  @param physics The PhysX Physics object.
             *  @param pxMaterial 
             *  @param worldTransform Position, scale and orientation of the base of the PhysicalizedSkeleton (The static anchor point in the terrain).
             *  @param archetypeTree Pointer to the archetype that defines the structure of the PhysicalizedSkeleton.
             *  @param segmentShapeType Geometrical Shape of each segment (aka bone) of the PhysicalizedSkeleton. BOX and CAPSULE look like Bones,
             *         but SPHERE is also possible.
             *  @returns True if the PhysicalizedSkeleton was created successfully, otherwise false. In general failure is only related
             *           with lack of system memory.
             */
            bool BuildFromArchetype(physx::PxPhysics& physics, physx::PxMaterial& pxMaterial, const AZ::Transform& worldTransform,
                const Physics::SpineTree* archetypeTree, SegmentShapeType segmentShapeType = SegmentShapeType::CAPSULE);

            void AddToScene(physx::PxScene& scene)
            {
                if (m_isPresentInTheScene)
                {
                    return;
                }
                AZ_Assert(m_aggregate, "This method was called with no aggregated data.  Most likely, BuildFromArchetype() was never called.");
                PHYSX_SCENE_WRITE_LOCK(scene);
                scene.addAggregate(*m_aggregate);
                m_isPresentInTheScene = true;
            }

            void RemoveFromScene(physx::PxScene& scene)
            {
                if (!m_isPresentInTheScene)
                {
                    return;
                }
                AZ_Assert(m_aggregate, "This method was called with no aggregated data.  Most likely, BuildFromArchetype() was never called.");
                PHYSX_SCENE_WRITE_LOCK(scene);
                scene.removeAggregate(*m_aggregate);
                m_isPresentInTheScene = false;
            }


            void ReadJointPositionsOld(Physics::JointPositions* jointPositions);
            void ReadJointPositions(Physics::JointPositions* jointPositions);

            AZ::u32 GetBoneCount() const
            {
                const size_t numPxJoints = m_physicsJoints.size();
                return (AZ::u32)(numPxJoints);
            }

            bool GetIsPresentInTheScene() const
            {
                return m_isPresentInTheScene;
            }

            //For debug. Prints in the console the GlobalPose of each segment.
            void DumpSegmentPoses();

            Physics::TouchBendingTriggerHandle* GetTriggerHandle() { return m_triggerHandle; }
            void SetTriggerHandle(Physics::TouchBendingTriggerHandle* triggerHandle) { m_triggerHandle = triggerHandle; }

            physx::PxScene* GetScene() { return m_aggregate ? m_aggregate->getScene() : nullptr; }

        private:
            physx::PxAggregate* m_aggregate;
            const Physics::SpineTree* m_spineTreeArchetype;


            /// When the trigger handle is destroyed this pointer will become
            /// nullptr. We use this handle to get how many objects are touching
            /// the trigger volume. The PhysicsComponent also uses it to reset its touch count when
            /// this object is destroyed.
            Physics::TouchBendingTriggerHandle* m_triggerHandle;

            float m_scale; //Each instance can be scaled differently.
            bool m_isPresentInTheScene;


            AZStd::vector<SpineData> m_physicsSpines;
            AZStd::vector<physx::PxD6Joint*> m_physicsJoints;

            BoneData* GetBoneData(const int spineIndex, const int boneIndex)
            {
                SpineData& spine = m_physicsSpines[spineIndex];
                return &spine.m_bones[boneIndex];
            }
        };
    } // namespace Physics
} // namespace TouchBending
