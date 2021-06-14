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

#include <PhysX/Joint/Configuration/PhysXJointConfiguration.h>
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

        void InitializeGenericProperties(const ApiJointGenericProperties& properties, physx::PxJoint* nativeJoint) 
        {
            if (!nativeJoint)
            {
                return;
            }
            PHYSX_SCENE_WRITE_LOCK(nativeJoint->getScene());
            nativeJoint->setConstraintFlag(
                physx::PxConstraintFlag::eCOLLISION_ENABLED,
                properties.IsFlagSet(ApiJointGenericProperties::GenericApiJointFlag::SelfCollide));

            if (properties.IsFlagSet(ApiJointGenericProperties::GenericApiJointFlag::Breakable))
            {
                nativeJoint->setBreakForce(properties.m_forceMax, properties.m_torqueMax);
            }
        }

        void InitializeSphericalLimitProperties(const ApiJointLimitProperties& properties, physx::PxSphericalJoint* nativeJoint)
        {
            if (!nativeJoint)
            {
                return;
            }

            if (!properties.m_isLimited)
            {
                nativeJoint->setSphericalJointFlag(physx::PxSphericalJointFlag::eLIMIT_ENABLED, false);
                return;
            }

            // Hard limit uses a tolerance value (distance to limit at which limit becomes active).
            // Soft limit allows angle to exceed limit but springs back with configurable spring stiffness and damping.
            physx::PxJointLimitCone swingLimit(
                AZ::DegToRad(properties.m_limitFirst), 
                AZ::DegToRad(properties.m_limitSecond), 
                properties.m_tolerance);
            
            if (properties.m_isSoftLimit)
            {
                swingLimit.stiffness = properties.m_stiffness;
                swingLimit.damping = properties.m_damping;
            }

            nativeJoint->setLimitCone(swingLimit);
            nativeJoint->setSphericalJointFlag(physx::PxSphericalJointFlag::eLIMIT_ENABLED, true);
        }

        void InitializeRevoluteLimitProperties(const ApiJointLimitProperties& properties, physx::PxRevoluteJoint* nativeJoint)
        {
            if (!nativeJoint)
            {
                return;
            }

            if (!properties.m_isLimited)
            {
                nativeJoint->setRevoluteJointFlag(physx::PxRevoluteJointFlag::eLIMIT_ENABLED, false);
                return;
            }

            physx::PxJointAngularLimitPair limitPair(
                AZ::DegToRad(properties.m_limitSecond), 
                AZ::DegToRad(properties.m_limitFirst), 
                properties.m_tolerance);

            if (properties.m_isSoftLimit)
            {
                limitPair.stiffness = properties.m_stiffness;
                limitPair.damping = properties.m_damping;
            }

            nativeJoint->setLimit(limitPair);
            nativeJoint->setRevoluteJointFlag(physx::PxRevoluteJointFlag::eLIMIT_ENABLED, true);
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
                    configuration.m_genericProperties, 
                    static_cast<physx::PxJoint*>(joint));

                return Utils::PxJointUniquePtr(joint, ReleasePxJoint);
            }

            PxJointUniquePtr CreatePxBallJoint(
                const PhysX::BallApiJointConfiguration& configuration,
                AzPhysics::SceneHandle sceneHandle,
                AzPhysics::SimulatedBodyHandle parentBodyHandle,
                AzPhysics::SimulatedBodyHandle childBodyHandle) 
            {
                PxJointActorData actorData = CalculateActorData(configuration, sceneHandle, parentBodyHandle, childBodyHandle);

                if (!actorData.parentActor || !actorData.childActor)
                {
                    return nullptr;
                }

                physx::PxSphericalJoint* joint;

                {
                    PHYSX_SCENE_READ_LOCK(actorData.childActor->getScene());
                    joint = physx::PxSphericalJointCreate(PxGetPhysics(), 
                        actorData.parentActor, actorData.parentLocalTransform,
                        actorData.childActor, actorData.childLocalTransform);
                }

                InitializeSphericalLimitProperties(configuration.m_limitProperties, joint);
                InitializeGenericProperties(
                    configuration.m_genericProperties, 
                    static_cast<physx::PxJoint*>(joint));

                return Utils::PxJointUniquePtr(joint, ReleasePxJoint);
            }

            PxJointUniquePtr CreatePxHingeJoint(
                const PhysX::HingeApiJointConfiguration& configuration,
                AzPhysics::SceneHandle sceneHandle,
                AzPhysics::SimulatedBodyHandle parentBodyHandle,
                AzPhysics::SimulatedBodyHandle childBodyHandle) 
            {
                PxJointActorData actorData = CalculateActorData(configuration, sceneHandle, parentBodyHandle, childBodyHandle);

                if (!actorData.parentActor || !actorData.childActor)
                {
                    return nullptr;
                }

                physx::PxRevoluteJoint* joint;

                {
                    PHYSX_SCENE_READ_LOCK(actorData.childActor->getScene());
                    joint = physx::PxRevoluteJointCreate(PxGetPhysics(), 
                        actorData.parentActor, actorData.parentLocalTransform,
                        actorData.childActor, actorData.childLocalTransform);
                }

                InitializeRevoluteLimitProperties(configuration.m_limitProperties, joint);
                InitializeGenericProperties(
                    configuration.m_genericProperties, 
                    static_cast<physx::PxJoint*>(joint));

                return Utils::PxJointUniquePtr(joint, ReleasePxJoint);
            }
        } // namespace PxJointFactories

        namespace Joints
        {
            bool IsD6SwingValid(float swingAngleY, float swingAngleZ, float swingLimitY, float swingLimitZ)
            {
                const float epsilon = AZ::Constants::FloatEpsilon;
                const float yFactor = AZStd::tan(0.25f * swingAngleY) / AZStd::GetMax(epsilon, AZStd::tan(0.25f * swingLimitY));
                const float zFactor = AZStd::tan(0.25f * swingAngleZ) / AZStd::GetMax(epsilon, AZStd::tan(0.25f * swingLimitZ));

                return (yFactor * yFactor + zFactor * zFactor <= 1.0f + epsilon);
            }

            void AppendD6SwingConeToLineBuffer(
                const AZ::Quaternion& parentLocalRotation,
                float swingAngleY,
                float swingAngleZ,
                float swingLimitY,
                float swingLimitZ,
                float scale,
                AZ::u32 angularSubdivisions,
                AZ::u32 radialSubdivisions,
                AZStd::vector<AZ::Vector3>& lineBufferOut,
                AZStd::vector<bool>& lineValidityBufferOut)
            {
                const AZ::u32 numLinesSwingCone = angularSubdivisions * (1u + radialSubdivisions);
                lineBufferOut.reserve(lineBufferOut.size() + 2u * numLinesSwingCone);
                lineValidityBufferOut.reserve(lineValidityBufferOut.size() + numLinesSwingCone);

                // the orientation quat for a radial line in the cone can be represented in terms of sin and cos half angles
                // these expressions can be efficiently calculated using tan quarter angles as follows:
                // writing t = tan(x / 4)
                // sin(x / 2) = 2 * t / (1 + t * t)
                // cos(x / 2) = (1 - t * t) / (1 + t * t)
                const float tanQuarterSwingZ = AZStd::tan(0.25f * swingLimitZ);
                const float tanQuarterSwingY = AZStd::tan(0.25f * swingLimitY);

                AZ::Vector3 previousRadialVector = AZ::Vector3::CreateZero();
                for (AZ::u32 angularIndex = 0; angularIndex <= angularSubdivisions; angularIndex++)
                {
                    const float angle = AZ::Constants::TwoPi / angularSubdivisions * angularIndex;
                    // the axis about which to rotate the x-axis to get the radial vector for this segment of the cone
                    const AZ::Vector3 rotationAxis(0, -tanQuarterSwingY * sinf(angle), tanQuarterSwingZ * cosf(angle));
                    const float normalizationFactor = rotationAxis.GetLengthSq();
                    const AZ::Quaternion radialVectorRotation = 1.0f / (1.0f + normalizationFactor) *
                        AZ::Quaternion::CreateFromVector3AndValue(2.0f * rotationAxis, 1.0f - normalizationFactor);
                    const AZ::Vector3 radialVector =
                        (parentLocalRotation * radialVectorRotation).TransformVector(AZ::Vector3::CreateAxisX(scale));

                    if (angularIndex > 0)
                    {
                        for (AZ::u32 radialIndex = 1; radialIndex <= radialSubdivisions; radialIndex++)
                        {
                            float radiusFraction = 1.0f / radialSubdivisions * radialIndex;
                            lineBufferOut.push_back(radiusFraction * radialVector);
                            lineBufferOut.push_back(radiusFraction * previousRadialVector);
                        }
                    }

                    if (angularIndex < angularSubdivisions)
                    {
                        lineBufferOut.push_back(AZ::Vector3::CreateZero());
                        lineBufferOut.push_back(radialVector);
                    }

                    previousRadialVector = radialVector;
                }

                const bool swingValid = IsD6SwingValid(swingAngleY, swingAngleZ, swingLimitY, swingLimitZ);
                lineValidityBufferOut.insert(lineValidityBufferOut.end(), numLinesSwingCone, swingValid);
            }

            void AppendD6TwistArcToLineBuffer(
                const AZ::Quaternion& parentLocalRotation,
                float twistAngle,
                float twistLimitLower,
                float twistLimitUpper,
                float scale,
                AZ::u32 angularSubdivisions,
                AZ::u32 radialSubdivisions,
                AZStd::vector<AZ::Vector3>& lineBufferOut,
                AZStd::vector<bool>& lineValidityBufferOut)
            {
                const AZ::u32 numLinesTwistArc = angularSubdivisions * (1u + radialSubdivisions) + 1u;
                lineBufferOut.reserve(lineBufferOut.size() + 2u * numLinesTwistArc);

                AZ::Vector3 previousRadialVector = AZ::Vector3::CreateZero();
                const float twistRange = twistLimitUpper - twistLimitLower;

                for (AZ::u32 angularIndex = 0; angularIndex <= angularSubdivisions; angularIndex++)
                {
                    const float angle = twistLimitLower + twistRange / angularSubdivisions * angularIndex;
                    const AZ::Vector3 radialVector =
                        parentLocalRotation.TransformVector(scale * AZ::Vector3(0.0f, cosf(angle), sinf(angle)));

                    if (angularIndex > 0)
                    {
                        for (AZ::u32 radialIndex = 1; radialIndex <= radialSubdivisions; radialIndex++)
                        {
                            const float radiusFraction = 1.0f / radialSubdivisions * radialIndex;
                            lineBufferOut.push_back(radiusFraction * radialVector);
                            lineBufferOut.push_back(radiusFraction * previousRadialVector);
                        }
                    }

                    lineBufferOut.push_back(AZ::Vector3::CreateZero());
                    lineBufferOut.push_back(radialVector);

                    previousRadialVector = radialVector;
                }

                const bool twistValid = (twistAngle >= twistLimitLower && twistAngle <= twistLimitUpper);
                lineValidityBufferOut.insert(lineValidityBufferOut.end(), numLinesTwistArc, twistValid);
            }

            void AppendD6CurrentTwistToLineBuffer(
                const AZ::Quaternion& parentLocalRotation,
                float twistAngle,
                [[maybe_unused]] float twistLimitLower,
                [[maybe_unused]] float twistLimitUpper,
                float scale,
                AZStd::vector<AZ::Vector3>& lineBufferOut,
                AZStd::vector<bool>& lineValidityBufferOut)
            {
                const AZ::Vector3 twistVector =
                    parentLocalRotation.TransformVector(1.25f * scale * AZ::Vector3(0.0f, cosf(twistAngle), sinf(twistAngle)));
                lineBufferOut.push_back(AZ::Vector3::CreateZero());
                lineBufferOut.push_back(twistVector);
                lineValidityBufferOut.push_back(true);
            }
        } // namespace Joints
    } // namespace Utils
} // namespace PhysX
