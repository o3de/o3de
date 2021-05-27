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

        void releasePxJoint(physx::PxJoint* joint)
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

        namespace PxJointFactories
        {   
            PxJointUniquePtr CreatePxD6Joint(
                const PhysX::D6ApiJointLimitConfiguration& configuration,
                AzPhysics::SceneHandle sceneHandle,
                AzPhysics::SimulatedBodyHandle parentBodyHandle,
                AzPhysics::SimulatedBodyHandle childBodyHandle) 
            {
                auto* parentBody = GetSimulatedBodyFromHandle(sceneHandle, parentBodyHandle);
                auto* childBody = GetSimulatedBodyFromHandle(sceneHandle, childBodyHandle);

                if (!IsAtLeastOneDynamic(parentBody, childBody))
                {
                    AZ_Warning("PhysX Joint", false, "CreateJoint failed - at least one body must be dynamic.");
                    return nullptr;
                }

                physx::PxRigidActor* parentActor = GetPxRigidActor(sceneHandle, parentBodyHandle);
                physx::PxRigidActor* childActor = GetPxRigidActor(sceneHandle, childBodyHandle);

                if (!parentActor && !childActor)
                {
                    AZ_Warning("PhysX Joint", false, "CreateJoint failed - at least one body must be a PxRigidActor.");
                    return nullptr;
                }

                const physx::PxTransform parentWorldTransform = parentActor ? parentActor->getGlobalPose() : physx::PxTransform(physx::PxIdentity);
                const physx::PxTransform childWorldTransform = childActor ? childActor->getGlobalPose() : physx::PxTransform(physx::PxIdentity);
                const physx::PxVec3 childOffset = childWorldTransform.p - parentWorldTransform.p;
                physx::PxTransform parentLocalTransform(PxMathConvert(configuration.m_parentLocalRotation).getNormalized());
                const physx::PxTransform childLocalTransform(PxMathConvert(configuration.m_childLocalRotation).getNormalized());
                parentLocalTransform.p = parentWorldTransform.q.rotateInv(childOffset);

                physx::PxD6Joint* joint = PxD6JointCreate(PxGetPhysics(), parentActor, parentLocalTransform,
                    childActor, childLocalTransform);

                joint->setMotion(physx::PxD6Axis::eTWIST, physx::PxD6Motion::eLIMITED);
                joint->setMotion(physx::PxD6Axis::eSWING1, physx::PxD6Motion::eLIMITED);
                joint->setMotion(physx::PxD6Axis::eSWING2, physx::PxD6Motion::eLIMITED);

                AZ_Warning("PhysX Joint",
                    configuration.m_swingLimitY >= JointConstants::MinSwingLimitDegrees && configuration.m_swingLimitZ >= JointConstants::MinSwingLimitDegrees,
                    "Very small swing limit requested for joint between \"%s\" and \"%s\", increasing to %f degrees to improve stability",
                    parentActor ? parentActor->getName() : "world", childActor ? childActor->getName() : "world",
                    JointConstants::MinSwingLimitDegrees);
                float swingLimitY = AZ::DegToRad(AZ::GetMax(JointConstants::MinSwingLimitDegrees, configuration.m_swingLimitY));
                float swingLimitZ = AZ::DegToRad(AZ::GetMax(JointConstants::MinSwingLimitDegrees, configuration.m_swingLimitZ));
                physx::PxJointLimitCone limitCone(swingLimitY, swingLimitZ);
                joint->setSwingLimit(limitCone);

                float twistLower = AZ::DegToRad(AZStd::GetMin(configuration.m_twistLimitLower, configuration.m_twistLimitUpper));
                float twistUpper = AZ::DegToRad(AZStd::GetMax(configuration.m_twistLimitLower, configuration.m_twistLimitUpper));
                physx::PxJointAngularLimitPair twistLimitPair(twistLower, twistUpper);
                joint->setTwistLimit(twistLimitPair);

                return Utils::PxJointUniquePtr(joint, releasePxJoint);
            }
        }
    }
}
        