/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Joint/PhysXJoint.h>
#include <AzCore/Serialization/EditContext.h>
#include <Include/PhysX/NativeTypeIdentifiers.h>
#include <AzCore/Math/MathUtils.h>
#include <AzFramework/Physics/Common/PhysicsSimulatedBody.h>
#include <Source/Joint/PhysXJointUtils.h>
#include <PhysX/MathConversion.h>

namespace PhysX
{
    AzPhysics::SimulatedBodyHandle PhysXJoint::GetParentBodyHandle() const
    {
        return m_parentBodyHandle;
    }

    AzPhysics::SimulatedBodyHandle PhysXJoint::GetChildBodyHandle() const
    {
        return m_childBodyHandle;
    }

    PhysXJoint::PhysXJoint(
        AzPhysics::SceneHandle sceneHandle,
        AzPhysics::SimulatedBodyHandle parentBodyHandle,
        AzPhysics::SimulatedBodyHandle childBodyHandle)
        : m_sceneHandle(sceneHandle)
        , m_parentBodyHandle(parentBodyHandle)
        , m_childBodyHandle(childBodyHandle)
    {

    }

    bool PhysXJoint::SetPxActors()
    {
        physx::PxRigidActor* parentActor = Utils::GetPxRigidActor(m_sceneHandle, m_parentBodyHandle);
        physx::PxRigidActor* childActor = Utils::GetPxRigidActor(m_sceneHandle, m_childBodyHandle);
        if (!parentActor && !childActor)
        {
            AZ_Error("PhysX Joint", false, "Invalid PhysX actors in joint - at least one must be a PxRigidActor.");
            return false;
        }

        m_pxJoint->setActors(parentActor, childActor);
        return true;
    }

    void PhysXJoint::SetParentBody(AzPhysics::SimulatedBodyHandle parentBodyHandle)
    {
        auto* parentBody = Utils::GetSimulatedBodyFromHandle(m_sceneHandle, parentBodyHandle);
        auto* childBody = Utils::GetSimulatedBodyFromHandle(m_sceneHandle, m_childBodyHandle);

        if (Utils::IsAtLeastOneDynamic(parentBody, childBody))
        {
            m_parentBodyHandle = parentBodyHandle;
            SetPxActors();
        }
        else
        {
            AZ_Warning("PhysX Joint", false, "Call to SetParentBody would result in invalid joint - at least one "
                "body in a joint must be dynamic.");
        }
    }

    void PhysXJoint::SetChildBody(AzPhysics::SimulatedBodyHandle childBodyHandle)
    {
        auto* parentBody = Utils::GetSimulatedBodyFromHandle(m_sceneHandle, m_parentBodyHandle);
        auto* childBody = Utils::GetSimulatedBodyFromHandle(m_sceneHandle, childBodyHandle);

        if (Utils::IsAtLeastOneDynamic(parentBody, childBody))
        {
            m_childBodyHandle = childBodyHandle;
            SetPxActors();
        }
        else
        {
            AZ_Warning("PhysX Joint", false, "Call to SetChildBody would result in invalid joint - at least one "
                "body in a joint must be dynamic.");
        }
    }

    void* PhysXJoint::GetNativePointer() const
    {
        return m_pxJoint.get();
    }

    PhysXD6Joint::PhysXD6Joint(const D6JointLimitConfiguration& configuration,
        AzPhysics::SceneHandle sceneHandle,
        AzPhysics::SimulatedBodyHandle parentBodyHandle,
        AzPhysics::SimulatedBodyHandle childBodyHandle)
        : PhysXJoint(sceneHandle, parentBodyHandle, childBodyHandle)
    {
        m_pxJoint = Utils::PxJointFactories::CreatePxD6Joint(configuration, sceneHandle, parentBodyHandle, childBodyHandle);
    }

    PhysXFixedJoint::PhysXFixedJoint(const FixedJointConfiguration& configuration,
        AzPhysics::SceneHandle sceneHandle,
        AzPhysics::SimulatedBodyHandle parentBodyHandle,
        AzPhysics::SimulatedBodyHandle childBodyHandle)
        : PhysXJoint(sceneHandle, parentBodyHandle, childBodyHandle)
    {
        m_pxJoint = Utils::PxJointFactories::CreatePxFixedJoint(configuration, sceneHandle, parentBodyHandle, childBodyHandle);
    }

    PhysXBallJoint::PhysXBallJoint(const BallJointConfiguration& configuration,
        AzPhysics::SceneHandle sceneHandle,
        AzPhysics::SimulatedBodyHandle parentBodyHandle,
        AzPhysics::SimulatedBodyHandle childBodyHandle)
        : PhysXJoint(sceneHandle, parentBodyHandle, childBodyHandle)
    {
        m_pxJoint = Utils::PxJointFactories::CreatePxBallJoint(configuration, sceneHandle, parentBodyHandle, childBodyHandle);
    }

    PhysXHingeJoint::PhysXHingeJoint(const HingeJointConfiguration& configuration,
        AzPhysics::SceneHandle sceneHandle,
        AzPhysics::SimulatedBodyHandle parentBodyHandle,
        AzPhysics::SimulatedBodyHandle childBodyHandle)
        : PhysXJoint(sceneHandle, parentBodyHandle, childBodyHandle)
    {
        m_pxJoint = Utils::PxJointFactories::CreatePxHingeJoint(configuration, sceneHandle, parentBodyHandle, childBodyHandle);
    }

    PhysXPrismaticJoint::PhysXPrismaticJoint(const PrismaticJointConfiguration& configuration,
        AzPhysics::SceneHandle sceneHandle,
        AzPhysics::SimulatedBodyHandle parentBodyHandle,
        AzPhysics::SimulatedBodyHandle childBodyHandle)
        : PhysXJoint(sceneHandle, parentBodyHandle, childBodyHandle)
    {
        m_pxJoint = Utils::PxJointFactories::CreatePxPrismaticJoint(configuration, sceneHandle, parentBodyHandle, childBodyHandle);
    }

    AZ::Crc32 PhysXD6Joint::GetNativeType() const
    {
        return NativeTypeIdentifiers::D6Joint;
    }

    AZ::Crc32 PhysXFixedJoint::GetNativeType() const
    {
        return NativeTypeIdentifiers::FixedJoint;
    }

    AZ::Crc32 PhysXBallJoint::GetNativeType() const
    {
        return NativeTypeIdentifiers::BallJoint;
    }

    AZ::Crc32 PhysXHingeJoint::GetNativeType() const
    {
        return NativeTypeIdentifiers::HingeJoint;
    }

    AZ::Crc32 PhysXPrismaticJoint::GetNativeType() const
    {
        return NativeTypeIdentifiers::PrismaticJoint;
    }

    void PhysXD6Joint::GenerateJointLimitVisualizationData(
        float scale,
        AZ::u32 angularSubdivisions,
        AZ::u32 radialSubdivisions,
        [[maybe_unused]] AZStd::vector<AZ::Vector3>& vertexBufferOut,
        [[maybe_unused]] AZStd::vector<AZ::u32>& indexBufferOut,
        AZStd::vector<AZ::Vector3>& lineBufferOut,
        AZStd::vector<bool>& lineValidityBufferOut)
    {
        auto* parentBody = Utils::GetSimulatedBodyFromHandle(m_sceneHandle, m_parentBodyHandle);
        auto* childBody = Utils::GetSimulatedBodyFromHandle(m_sceneHandle, m_childBodyHandle);

        const AZ::u32 angularSubdivisionsClamped = AZ::GetClamp(angularSubdivisions, 4u, 32u);
        const AZ::u32 radialSubdivisionsClamped = AZ::GetClamp(radialSubdivisions, 1u, 4u);

        const physx::PxD6Joint* joint = static_cast<physx::PxD6Joint*>(m_pxJoint.get());
        const AZ::Quaternion parentLocalRotation = PxMathConvert(joint->getLocalPose(physx::PxJointActorIndex::eACTOR0).q);
        const AZ::Quaternion parentWorldRotation = parentBody ? parentBody->GetOrientation() : AZ::Quaternion::CreateIdentity();
        const AZ::Quaternion childLocalRotation = PxMathConvert(joint->getLocalPose(physx::PxJointActorIndex::eACTOR1).q);
        const AZ::Quaternion childWorldRotation = childBody ? childBody->GetOrientation() : AZ::Quaternion::CreateIdentity();

        const float swingAngleY = joint->getSwingYAngle();
        const float swingAngleZ = joint->getSwingZAngle();
        const float swingLimitY = joint->getSwingLimit().yAngle;
        const float swingLimitZ = joint->getSwingLimit().zAngle;
        const float twistAngle = joint->getTwist();
        const float twistLimitLower = joint->getTwistLimit().lower;
        const float twistLimitUpper = joint->getTwistLimit().upper;

        Utils::Joints::AppendD6SwingConeToLineBuffer(
            parentLocalRotation, swingAngleY, swingAngleZ, swingLimitY, swingLimitZ,
            scale, angularSubdivisionsClamped, radialSubdivisionsClamped, lineBufferOut, lineValidityBufferOut);
        Utils::Joints::AppendD6TwistArcToLineBuffer(
            parentLocalRotation, twistAngle, twistLimitLower, twistLimitUpper,
            scale, angularSubdivisionsClamped, radialSubdivisionsClamped, lineBufferOut, lineValidityBufferOut);
        Utils::Joints::AppendD6CurrentTwistToLineBuffer(
            parentLocalRotation, twistAngle, twistLimitLower, twistLimitUpper,
            scale, lineBufferOut, lineValidityBufferOut);

        // draw the X-axis of the child joint frame
        // make the axis slightly longer than the radius of the twist arc so that it is easy to see
        float axisLength = 1.25f * scale;
        AZ::Vector3 childAxis = (parentWorldRotation.GetConjugate() * childWorldRotation * childLocalRotation).TransformVector(
            AZ::Vector3::CreateAxisX(axisLength));
        lineBufferOut.push_back(AZ::Vector3::CreateZero());
        lineBufferOut.push_back(childAxis);
    }
} // namespace PhysX
