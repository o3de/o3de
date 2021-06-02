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

#include <PhysX_precompiled.h>

#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/PhysicsSystem.h>
#include <AzFramework/Physics/Common/PhysicsSimulatedBody.h>
#include <AzCore/EBus/EBus.h>

#include <Joint/Configuration/PhysXJointConfiguration.h>
#include <PhysX/PhysXLocks.h>
#include <Source/Joint/PhysXApiJointUtils.h>
#include <Include/PhysX/NativeTypeIdentifiers.h>

namespace PhysX {
    namespace Utils 
    {
        struct PxJointActorData 
        {
            static PxJointActorData InvalidPxJointActorData;

            physx::PxRigidActor* parentActor = nullptr;
            physx::PxTransform parentLocalTransform;
            
            physx::PxRigidActor* childActor = nullptr;
            physx::PxTransform childLocalTransform;
        };
        PxJointActorData PxJointActorData::InvalidPxJointActorData;

        PxJointActorData CalculateActorData(
            const AzPhysics::ApiJointConfiguration& configuration,
            AzPhysics::SceneHandle sceneHandle,
            AzPhysics::SimulatedBodyHandle parentBodyHandle,
            AzPhysics::SimulatedBodyHandle childBodyHandle)
        {
            auto* parentBody = GetSimulatedBodyFromHandle(sceneHandle, parentBodyHandle);
            auto* childBody = GetSimulatedBodyFromHandle(sceneHandle, childBodyHandle);

            if (!IsAtLeastOneDynamic(parentBody, childBody))
            {
                AZ_Warning("PhysX Joint", false, "CreateJoint failed - at least one body must be dynamic.");
                return PxJointActorData::InvalidPxJointActorData;
            }

            physx::PxRigidActor* parentActor = GetPxRigidActor(sceneHandle, parentBodyHandle);
            physx::PxRigidActor* childActor = GetPxRigidActor(sceneHandle, childBodyHandle);

            if (!parentActor && !childActor)
            {
                AZ_Warning("PhysX Joint", false, "CreateJoint failed - at least one body must be a PxRigidActor.");
                return PxJointActorData::InvalidPxJointActorData;
            }

            const physx::PxTransform parentWorldTransform = parentActor ? parentActor->getGlobalPose() : physx::PxTransform(physx::PxIdentity);
            const physx::PxTransform childWorldTransform = childActor ? childActor->getGlobalPose() : physx::PxTransform(physx::PxIdentity);
            const physx::PxVec3 childOffset = childWorldTransform.p - parentWorldTransform.p;
            physx::PxTransform parentLocalTransform(PxMathConvert(configuration.m_parentLocalRotation).getNormalized());
            const physx::PxTransform childLocalTransform(PxMathConvert(configuration.m_childLocalRotation).getNormalized());
            parentLocalTransform.p = parentWorldTransform.q.rotateInv(childOffset);

            return PxJointActorData{
                parentActor,
                parentLocalTransform,
                childActor,
                childLocalTransform
            };
        }

        bool IsAtLeastOneDynamic(AzPhysics::SimulatedBody* body0,
            AzPhysics::SimulatedBody* body1)
        {
            for (const AzPhysics::SimulatedBody* body : { body0, body1 })
            {
                if (body)
                {
                    if (body->GetNativeType() == NativeTypeIdentifiers::RigidBody ||
                        body->GetNativeType() == NativeTypeIdentifiers::ArticulationLink)
                    {
                        return true;
                    }
                }
            }
            return false;
        }
        
        physx::PxRigidActor* GetPxRigidActor(AzPhysics::SceneHandle sceneHandle, AzPhysics::SimulatedBodyHandle worldBodyHandle)
        {
            auto* worldBody = GetSimulatedBodyFromHandle(sceneHandle, worldBodyHandle);
            if (worldBody != nullptr 
                && static_cast<physx::PxBase*>(worldBody->GetNativePointer())->is<physx::PxRigidActor>())
            {
                return static_cast<physx::PxRigidActor*>(worldBody->GetNativePointer());
            }

            return nullptr;
        }

        void ReleasePxJoint(physx::PxJoint* joint)
        {
            PHYSX_SCENE_WRITE_LOCK(joint->getScene());
            joint->userData = nullptr;
            joint->release();
        }

        AzPhysics::SimulatedBody* GetSimulatedBodyFromHandle(AzPhysics::SceneHandle sceneHandle,
            AzPhysics::SimulatedBodyHandle bodyHandle) 
        {
            if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
            {
                return sceneInterface->GetSimulatedBodyFromHandle(sceneHandle, bodyHandle);
            }
            return nullptr;
        }

        void InitializeGenericProperties(const GenericApiJointConfiguration& configuration, physx::PxJoint* nativeJoint) 
        {
            if (!nativeJoint)
            {
                return;
            }
            PHYSX_SCENE_WRITE_LOCK(nativeJoint->getScene());
            nativeJoint->setConstraintFlag(
                physx::PxConstraintFlag::eCOLLISION_ENABLED,
                configuration.IsFlagSet(GenericApiJointConfiguration::GenericApiJointFlag::SelfCollide));

            if (configuration.IsFlagSet(GenericApiJointConfiguration::GenericApiJointFlag::Breakable))
            {
                nativeJoint->setBreakForce(configuration.m_forceMax, configuration.m_torqueMax);
            }
        }

        namespace PxJointFactories
        {   
            PxJointUniquePtr CreatePxD6Joint(
                const PhysX::D6ApiJointLimitConfiguration& configuration,
                AzPhysics::SceneHandle sceneHandle,
                AzPhysics::SimulatedBodyHandle parentBodyHandle,
                AzPhysics::SimulatedBodyHandle childBodyHandle) 
            {
                PxJointActorData actorData = CalculateActorData(configuration, sceneHandle, parentBodyHandle, childBodyHandle);

                if (!actorData.parentActor || !actorData.childActor)
                {
                    return nullptr;
                }

                physx::PxD6Joint* joint = PxD6JointCreate(PxGetPhysics(), 
                    actorData.parentActor, actorData.parentLocalTransform,
                    actorData.childActor, actorData.childLocalTransform);

                joint->setMotion(physx::PxD6Axis::eTWIST, physx::PxD6Motion::eLIMITED);
                joint->setMotion(physx::PxD6Axis::eSWING1, physx::PxD6Motion::eLIMITED);
                joint->setMotion(physx::PxD6Axis::eSWING2, physx::PxD6Motion::eLIMITED);

                AZ_Warning("PhysX Joint",
                    configuration.m_swingLimitY >= JointConstants::MinSwingLimitDegrees && configuration.m_swingLimitZ >= JointConstants::MinSwingLimitDegrees,
                    "Very small swing limit requested for joint between \"%s\" and \"%s\", increasing to %f degrees to improve stability",
                    actorData.parentActor ? actorData.parentActor->getName() : "world", 
                    actorData.childActor ? actorData.childActor->getName() : "world",
                    JointConstants::MinSwingLimitDegrees);
                
                const float swingLimitY = AZ::DegToRad(AZ::GetMax(JointConstants::MinSwingLimitDegrees, configuration.m_swingLimitY));
                const float swingLimitZ = AZ::DegToRad(AZ::GetMax(JointConstants::MinSwingLimitDegrees, configuration.m_swingLimitZ));
                physx::PxJointLimitCone limitCone(swingLimitY, swingLimitZ);
                joint->setSwingLimit(limitCone);

                const float twistLower = AZ::DegToRad(AZStd::GetMin(configuration.m_twistLimitLower, configuration.m_twistLimitUpper));
                const float twistUpper = AZ::DegToRad(AZStd::GetMax(configuration.m_twistLimitLower, configuration.m_twistLimitUpper));
                physx::PxJointAngularLimitPair twistLimitPair(twistLower, twistUpper);
                joint->setTwistLimit(twistLimitPair);

                return Utils::PxJointUniquePtr(joint, ReleasePxJoint);
            }

            PxJointUniquePtr CreatePxFixedJoint(
                const PhysX::FixedApiJointConfiguration& configuration,
                AzPhysics::SceneHandle sceneHandle,
                AzPhysics::SimulatedBodyHandle parentBodyHandle,
                AzPhysics::SimulatedBodyHandle childBodyHandle) 
            {
                PxJointActorData actorData = CalculateActorData(configuration, sceneHandle, parentBodyHandle, childBodyHandle);

                if (!actorData.parentActor || !actorData.childActor)
                {
                    return nullptr;
                }

                physx::PxFixedJoint* joint;

                {
                    PHYSX_SCENE_READ_LOCK(actorData.childActor->getScene());
                    joint = physx::PxFixedJointCreate(PxGetPhysics(), 
                        actorData.parentActor, actorData.parentLocalTransform,
                        actorData.childActor, actorData.childLocalTransform);
                }

                InitializeGenericProperties(
                    static_cast<const GenericApiJointConfiguration&>(configuration), 
                    static_cast<physx::PxJoint*>(joint));

                return Utils::PxJointUniquePtr(joint, ReleasePxJoint);
            }
        } // namespace PxJointFactories
    } // namespace Utils
} // namespace PhysX
