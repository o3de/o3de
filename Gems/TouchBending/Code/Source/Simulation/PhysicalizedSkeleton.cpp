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
#include <PhysX/MathConversion.h>
#include <PhysX/Utils.h>
#include <AzCore/Math/ToString.h>
#include <AzFramework/Physics/Collision/CollisionGroups.h>
#include <AzFramework/Physics/Collision/CollisionLayers.h>

#include "PhysicalizedSkeleton.h"

using namespace physx;

namespace TouchBending
{
    namespace Simulation
    {
        ///Vegetation Touch Bending is the first PhysX feature in Lumberyard
        ///that makes use of Dominance Groups. By default all actors get Dominance Group 0.
        ///These are values from 0 to 31.
        ///Originally TouchBending was using onContactModify Callback to make all other actors
        ///appear as having infinite mass when touching the segments of PhysicalizedSkeleton objects. With this adjustment
        ///Vegetation items get crushed/bent at the mercy of other actors without affectting the moment of inertia
        ///of those actors. This makes touch bending a pure cosmetic visual effect just like the original
        ///touch bending from CryPhysics.
        ///Using Dominance Groups has a performance advantage over using the OnContactModify callback.
        ///See: https://docs.nvidia.com/gameworks/content/gameworkslibrary/physx/guide/Manual/RigidBodyDynamics.html#dominance
        const PxDominanceGroup VegetationDominanceGroup = 1;

        static const char * const TRACE_WINDOW_NAME = "PhysicalizedSkeleton";

        //*********************************************************************
        // Helper classes & Functions START *********************************************

        /** @brief
         *
         *  @param newGeometryOut
         *  @param segment
         *  @param segmentShapeType
         *  @returns The volume of the geometry.
         */
        static float CreateBoneGeometry(PxGeometry** newGeometryOut,
            const float boneLength, const float boneThickness,
            const SegmentShapeType segmentShapeType,
            const float scale)
        {
            const float thickness = boneThickness * scale; //Thickness is a radius. no need to multiply by 0.5f.
            const float halfLength = (boneLength * 0.5f) * scale;

            float volume = 0.0f;
            switch (segmentShapeType)
            {
            case SegmentShapeType::BOX:
            {
                PxBoxGeometry* boxGeometry = new PxBoxGeometry(thickness, thickness, halfLength);
                volume = (thickness * thickness * halfLength) * 8.0f;
                *newGeometryOut = boxGeometry;
            }
                break;
            case SegmentShapeType::CAPSULE:
            {
                const float radius = thickness;
                // When translating segment length and thickness into a capsule, 
                // segment length = capsule length = radius + cylinder length + radius.  
                // If the radius (thickness) is larger than half the segment length, we will need to clamp
                // to a minimum cylinder length to ensure that the capsule remains valid, even though it will be larger
                // than the requested segment length.
                const float minimumHalfCylinderLength = 0.001f;
                const float halfCylinderLength = AZ::GetMax(minimumHalfCylinderLength, halfLength - radius);
                PxCapsuleGeometry* capsuleGeometry = new PxCapsuleGeometry(radius, halfCylinderLength);
                const float cylinderVolume = (AZ::Constants::Pi * radius * radius) * (halfCylinderLength * 2.0f);
                const float sphereVolume = (4.0f / 3.0f) * (AZ::Constants::Pi * radius * radius * radius);
                volume = cylinderVolume + sphereVolume;
                *newGeometryOut = capsuleGeometry;
            }
                break;
            case SegmentShapeType::SPHERE:
            {
                const float radius = halfLength;
                PxSphereGeometry* sphereGeometry = new PxSphereGeometry(radius);
                const float sphereVolume = (4.0f / 3.0f) * (AZ::Constants::Pi * radius * radius * radius);
                volume = sphereVolume;
                *newGeometryOut = sphereGeometry;
            }
            break;
            default:
                AZ_Assert(false, "segmentShapeType = %d is unsupported", static_cast<int>(segmentShapeType));
                break;
            }
            return volume;
        }

        

        // Helper Functions END ***********************************************
        //*********************************************************************



        static constexpr float MAX_JOINT_SPRING_VALUE = 1000.0f;
        static constexpr float MIN_JOINT_SPRING_VALUE = 0.0f;

        static constexpr float MAX_JOINT_DAMPING_VALUE = 1000.0f;
        static constexpr float MIN_JOINT_DAMPING_VALUE = 0.0f;

        static constexpr float MAX_LINEAR_AND_ANGULAR_DAMPING_PER_SEGMENT = 2.0f;

        PhysicalizedSkeleton::~PhysicalizedSkeleton()
        {
            if (!m_aggregate)
            {
                return;
            }

            PHYSX_SCENE_WRITE_LOCK(m_aggregate->getScene());

            for (PxD6Joint* joint : m_physicsJoints)
            {
                if (joint)
                {
                    joint->release();
                }
            }

            for (SpineData& spine : m_physicsSpines)
            {
                for (BoneData& bone : spine.m_bones)
                {
                    if (bone.m_rigidDynamicActor)
                    {
                        bone.m_rigidDynamicActor->release();
                    }
                }
            }

            m_aggregate->release();
        }

        bool PhysicalizedSkeleton::BuildFromArchetype(PxPhysics& physics, PxMaterial& pxMaterial, const AZ::Transform& worldTransform, const Physics::SpineTree*  archetype, SegmentShapeType segmentShapeType)
        {
            AZ_Assert(m_aggregate == nullptr, "BuildFromArchetype should not be called more than once per PhysicalizedSkeleton");

            AZStd::vector<AZStd::vector<AZ::Quaternion>> boneOrientationsInSkeleton;

            m_spineTreeArchetype = nullptr;
            const PxU32 numberOfBones = (PxU32)archetype->CalculateTotalNumberOfBones();

            const bool selfCollisions = false;
            m_aggregate = physics.createAggregate(numberOfBones, selfCollisions);
            m_physicsJoints.set_capacity(numberOfBones); //Each segment has a joint at its base: local (0,0,0).

            const size_t numberOfSpines = archetype->m_spines.size();
            m_physicsSpines.set_capacity(numberOfSpines);
            boneOrientationsInSkeleton.set_capacity(numberOfSpines);

            const AZ::Vector3 xyzUniformScale = worldTransform.GetScale();
            const float scale = xyzUniformScale.GetX();
            m_scale = scale;
            const PxVec3 pxSkeletonWorldPosition = PxMathConvert(worldTransform.GetTranslation());
            for (size_t spineIndex = 0; spineIndex < numberOfSpines; ++spineIndex)
            {
                const Physics::Spine& archetypeSpine = archetype->m_spines[spineIndex];
                const size_t numberOfPointsInSpine = archetypeSpine.m_points.size();
                const size_t numberOfBonesInSpine = numberOfPointsInSpine - 1;

                const int parentSpineIndex = archetypeSpine.m_parentSpineIndex;
                const int parentPointIndex = archetypeSpine.m_parentPointIndex;

                boneOrientationsInSkeleton.emplace_back(AZStd::vector<AZ::Quaternion>());
                AZStd::vector<AZ::Quaternion>& boneOrientationsInSpine = boneOrientationsInSkeleton.back();
                boneOrientationsInSpine.set_capacity(numberOfBonesInSpine);

                m_physicsSpines.emplace_back(SpineData());
                SpineData& spineData = m_physicsSpines.back();
                spineData.m_bones.set_capacity(numberOfBonesInSpine);

                //Let's find the bottom point of the first bone of this spine.
                int bottomPointSpineIndex = spineIndex;
                int bottomPointIndex = 0;
                if (spineIndex > 0)
                {
                    AZ_Assert((parentSpineIndex >= 0) && (parentPointIndex >= 0), "Invalid parent spine data");
                    bottomPointSpineIndex = parentSpineIndex;
                    bottomPointIndex = parentPointIndex;
                }

                const Physics::SpinePoint* boneBottomPoint = &(archetype->m_spines[bottomPointSpineIndex].m_points[bottomPointIndex]);
                AZ::Vector3 boneBottomPointTransformedPosition = worldTransform.TransformPoint(boneBottomPoint->m_position);

                size_t boneIndex = 0;
                for (size_t pointIndex = 1; pointIndex < numberOfPointsInSpine; ++pointIndex)
                {
                    boneOrientationsInSpine.emplace_back(AZ::Quaternion());
                    AZ::Quaternion& boneOrientation = boneOrientationsInSpine.back();

                    spineData.m_bones.emplace_back(BoneData());
                    BoneData& boneData = spineData.m_bones.back();

                    const Physics::SpinePoint* boneTopPoint = &(archetypeSpine.m_points[pointIndex]);
                    AZ::Vector3 boneTopPointTransformedPosition = worldTransform.TransformPoint(boneTopPoint->m_position);

                    const AZ::Vector3 boneVector = boneTopPointTransformedPosition - boneBottomPointTransformedPosition;
                    const float boneVectorLength = boneVector.GetLength();
                    if (AZ::IsClose(boneVectorLength, 0.0f, AZ::Constants::FloatEpsilon))
                    {
                        AZ_Error(TRACE_WINDOW_NAME, false, "Failed to create geometry for Spine Index=%zu, at Segment Index=%u: 0-length bone.", spineIndex, pointIndex);
                        return false;
                    }

                    const AZ::Vector3 boneVectorNormalized = boneVector * (1.0f / boneVectorLength);

                    PxGeometry* boneGeometry = nullptr;
                    const float boneVolume = CreateBoneGeometry(&boneGeometry, boneVectorLength, boneBottomPoint->m_thickness, segmentShapeType, 1.0f);
                    if (!boneGeometry || boneVolume <= 0.0f)
                    {
                        AZ_Error(TRACE_WINDOW_NAME, false, "Failed to create geometry for Spine Index=%zu, at Segment Index=%u", spineIndex, pointIndex);
                        return false;
                    }
                    
                    const AZ::Vector3 boneWorldPosition = boneBottomPointTransformedPosition + (boneTopPointTransformedPosition - boneBottomPointTransformedPosition) * 0.5f;
                    const PxVec3 bonePosition = PxMathConvert(boneWorldPosition);

                    //Calculate the Orientation of the bone with respect to world Z-Axis
                    const AZ::Vector3 refBasisZ(0, 0, 1);
                    boneOrientation = AZ::Quaternion::CreateShortestArc(refBasisZ, boneVectorNormalized);
                    PxTransform boneTransform(bonePosition, PxMathConvert(boneOrientation));
                    const float boneDensity = (boneBottomPoint->m_mass * scale) / boneVolume;

                    PxRigidDynamic* pxRigidDynamicBone = physics.createRigidDynamic(boneTransform);
#ifdef TOUCHBENDING_VISUALIZE_ENABLED
                    const PxShapeFlags shapeFlags = PxShapeFlag::eVISUALIZATION | PxShapeFlag::eSIMULATION_SHAPE;
#else
                    const PxShapeFlags shapeFlags = PxShapeFlag::eSIMULATION_SHAPE;
#endif
                    PxShape* pxShape = PxRigidActorExt::createExclusiveShape(*pxRigidDynamicBone, *boneGeometry, pxMaterial, shapeFlags);
                    if (!pxShape)
                    {
                        AZ_Error(TRACE_WINDOW_NAME, false, "Failed to create shape for spine Index=%zu, at bone Index=%u", spineIndex, boneIndex);
                        delete boneGeometry;
                        return false;
                    }
                    if (segmentShapeType == SegmentShapeType::CAPSULE)
                    {
                        //PhysX capsules have their main axis oriented parallel to the X-axis.
                        const PxTransform relativePoseForCapsule(PxQuat(PxHalfPi, PxVec3(0, 1, 0)));
                        pxShape->setLocalPose(relativePoseForCapsule);
                    }
                    PxRigidBodyExt::updateMassAndInertia(*pxRigidDynamicBone, boneDensity);

                    const float normalizedSegmentDamping = AZ::GetClamp(boneBottomPoint->m_damping, 0.0f, 1.0f);
                    const float segmentDamping = normalizedSegmentDamping * MAX_JOINT_DAMPING_VALUE;
                    const float linear_and_angular_damping = (segmentDamping / MAX_JOINT_DAMPING_VALUE) * MAX_LINEAR_AND_ANGULAR_DAMPING_PER_SEGMENT;
                    pxRigidDynamicBone->setLinearDamping(linear_and_angular_damping);
                    pxRigidDynamicBone->setAngularDamping(linear_and_angular_damping);
                    pxRigidDynamicBone->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, true); //No gravity, otherwise the PhysicalizedSkeleton looks like a sad willow.

                    boneData.m_length = boneVectorLength;
                    boneData.m_rigidDynamicActor = pxRigidDynamicBone;
                    m_aggregate->addActor(*pxRigidDynamicBone);

                    //Time to create the PhysX joint.
                    BoneData* previousBone = nullptr;
                    const AZ::Quaternion* previousBoneOrientation = nullptr;
                    PxRigidActor* previousPxRigidDynamicBone = nullptr;
                    PxTransform previousBoneLinkFrame;

                    if (boneIndex == 0)
                    {
                        //If we are at the first segment of the current spine, then there are two options
                        //of joint attachment:
                        if (spineIndex == 0)
                        {
                            //1- If there's no parent spine, the linkFramePrevSegment is at the World Position of the PhysicalizedSkeleton.
                            previousBoneLinkFrame.p = pxSkeletonWorldPosition;
                            previousBoneLinkFrame.q = PxQuat(PxIdentity);
                        }
                        else
                        {
                            //2- If there's a parent spine, then linkFramePrevSegment is with respect to the segment within the
                            //   parent spine.
                            previousBone = GetBoneData(bottomPointSpineIndex, bottomPointIndex - 1);
                            previousBoneOrientation = &boneOrientationsInSkeleton[bottomPointSpineIndex][bottomPointIndex - 1];
                            previousPxRigidDynamicBone = previousBone->m_rigidDynamicActor;
                            previousBoneLinkFrame.p = PxVec3(0, 0, 1) * (previousBone->m_length * 0.5f);
                            previousBoneLinkFrame.q = PxQuat(PxIdentity);
                        }
                    }
                    else
                    {
                        previousBone = GetBoneData(spineIndex, boneIndex - 1);
                        previousBoneOrientation = &boneOrientationsInSkeleton[spineIndex][boneIndex - 1];
                        previousPxRigidDynamicBone = previousBone->m_rigidDynamicActor;
                        previousBoneLinkFrame.p = PxVec3(0, 0, 1) * (previousBone->m_length * 0.5f);
                        previousBoneLinkFrame.q = PxQuat(PxIdentity);
                    }

                    PxQuat currentBoneRelativePxQuaternion;
                    if (previousBone)
                    {
                        //Calculate relative Orientation between current segment and previous segment.
                        const AZ::Quaternion prevBoneQuaternionInverse = previousBoneOrientation->GetInverseFull();
                        AZ::Quaternion currentBoneRelativeAZQuaternion = prevBoneQuaternionInverse * boneOrientation;
                        currentBoneRelativePxQuaternion = PxMathConvert(currentBoneRelativeAZQuaternion);
                    }
                    else
                    {
                        //Calculate relative Orientation between current segment and previous segment.
                       currentBoneRelativePxQuaternion = PxMathConvert(boneOrientation);
                    }

                    //Let's get the Relative World Up.
                    PxTransform currentBoneLinkFrame;
                    const float  currentArchetypeSegmentHalfLength = boneVectorLength * 0.5f;

                    currentBoneLinkFrame.p = PxVec3(0, 0, -1) * currentArchetypeSegmentHalfLength;
                    currentBoneLinkFrame.q = PxQuat(PxIdentity);

                    PxD6Joint* pxJoint = PxD6JointCreate(physics, previousPxRigidDynamicBone, previousBoneLinkFrame, pxRigidDynamicBone, currentBoneLinkFrame);

                    //The joints allow rotation in Y and Z only.
                    pxJoint->setMotion(PxD6Axis::eSWING1, PxD6Motion::eLIMITED);
                    pxJoint->setMotion(PxD6Axis::eSWING2, PxD6Motion::eLIMITED);
                    pxJoint->setMotion(PxD6Axis::eTWIST, PxD6Motion::eLIMITED);
                    PxJointLimitCone swingLimit(PxPi * 0.6f, PxPi * 0.6f);
                    pxJoint->setSwingLimit(swingLimit);
                    
                    const float normalizedSegmentStiffness = AZ::GetClamp(boneBottomPoint->m_stiffness, 0.0f, 1.0f);
                    const float segmentStiffness = normalizedSegmentStiffness * MAX_JOINT_SPRING_VALUE;
                    PxD6JointDrive drive(segmentStiffness, segmentDamping, PX_MAX_F32, true);
                    pxJoint->setDrive(PxD6Drive::eSLERP, drive);
                    pxJoint->setDrivePosition(PxTransform(PxZero, currentBoneRelativePxQuaternion));
                    pxJoint->setDriveVelocity(PxVec3(PxZero), PxVec3(PxZero));

                    //Make sure we have limits, so when the joint breaks the whole structure goes back to zero.
                    pxJoint->setProjectionLinearTolerance(0.1f);
                    pxJoint->setProjectionAngularTolerance(PxPi);
                    pxJoint->setConstraintFlag(PxConstraintFlag::ePROJECTION, true);

                    pxRigidDynamicBone->setDominanceGroup(VegetationDominanceGroup);
                    PhysX::Utils::Collision::SetCollisionLayerAndGroup(pxShape, AzPhysics::CollisionLayer::TouchBend, AzPhysics::CollisionGroup::All_NoTouchBend);

                    // add to joints array for cleanup
                    m_physicsJoints.push_back(pxJoint);

                    delete boneGeometry;
                    boneIndex++;
                    boneBottomPoint = boneTopPoint;
                    boneBottomPointTransformedPosition = boneTopPointTransformedPosition;
                }
            }
            m_spineTreeArchetype = archetype;
            return true;
        }

        static inline void CopyPxVec3ToArray(const PxVec3& v, float* dst)
        {
            dst[0] = v.x;
            dst[1] = v.y;
            dst[2] = v.z;
        }

        static inline void CalculateTopJointLocation(const float boneLength, const PxVec3& boneLocation, const PxVec3& boneBasisZ, const float scale, PxVec3& jointLocationOut)
        {
            jointLocationOut = boneLocation + boneBasisZ * (boneLength * scale * 0.5f);
        }
        static inline void CalculateBottomJointLocation(const float boneLength, const PxVec3& boneLocation, const PxVec3& boneBasisZ, const float scale, PxVec3& jointLocationOut)
        {
            jointLocationOut = boneLocation - boneBasisZ * (boneLength * scale * 0.5f);
        }

        void PhysicalizedSkeleton::ReadJointPositions(Physics::JointPositions* jointPositions)
        {
            AZ_Assert(m_spineTreeArchetype, "A Valid PhysicalizedSkeleton always has a pointer to the archetype it was built from.");
            AZ::u32 jointIndex = 0;
            AZ::u32 numberOfSpines = (AZ::u32)m_spineTreeArchetype->m_spines.size();
            for (AZ::u32 spineIndex = 0; spineIndex < numberOfSpines; ++spineIndex)
            {
                const Physics::Spine& archetypeSpine = m_spineTreeArchetype->m_spines[spineIndex];
                const SpineData& spineData = m_physicsSpines[spineIndex];

                const AZ::u32 numberOfBones = (AZ::u32)spineData.m_bones.size();

                PxTransform bonePose = spineData.m_bones[0].m_rigidDynamicActor->getGlobalPose();
                PxVec3 bonePoseBasisZ =  bonePose.q.getBasisVector2();

                PxVec3 bottomPoint;
                CalculateBottomJointLocation(spineData.m_bones[0].m_length, bonePose.p, bonePoseBasisZ, 1.0f, bottomPoint);
                jointPositions[jointIndex].m_hasNewData = 1.0f;
                for (size_t boneIndex = 0; boneIndex < numberOfBones; ++boneIndex)
                {
                    const BoneData& boneData = spineData.m_bones[boneIndex];
                    bonePose = boneData.m_rigidDynamicActor->getGlobalPose();
                    bonePoseBasisZ = bonePose.q.getBasisVector2();

                    PxVec3 topPoint;
                    CalculateTopJointLocation(boneData.m_length, bonePose.p, bonePoseBasisZ, 1.0f, topPoint);
                    Physics::JointPositions* dstPositions = &jointPositions[jointIndex];
                    CopyPxVec3ToArray(bottomPoint, dstPositions->m_BottomJointLocation);
                    CopyPxVec3ToArray(topPoint, dstPositions->m_TopJointLocation);

                    bottomPoint = topPoint;
                    ++jointIndex;
                }
            }
        }

        static AZStd::string PxVec3ToStr(const PxVec3& v)
        {
            AZStd::string retStr = AZStd::string::format("%f, %f, %f", v.x, v.y, v.z);
            return retStr;
        }

        void PhysicalizedSkeleton::DumpSegmentPoses()
        {
            AZ_Printf("AzTouchBending", "\n***** Segment Poses ****");
            int spineIndex = 0;
            for (SpineData& spine : m_physicsSpines)
            {
                int boneIndex = 0;
                for (BoneData& boneData : spine.m_bones)
                {
                    const PxTransform pose = boneData.m_rigidDynamicActor->getGlobalPose();
                    const PxVec3 position = pose.p;
                    AZ_Printf(TRACE_WINDOW_NAME, "spine[%d] bone[%d] position=(%s)", spineIndex, boneIndex, PxVec3ToStr(position).c_str());
                    ++boneIndex;
                }
                ++spineIndex;
            }
            AZ_Printf("AzTouchBending", "***********************\n");
        }

    } // Physics
} // TouchBending
