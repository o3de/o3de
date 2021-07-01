/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PhysX_precompiled.h>
#include <Joint.h>
#include <AzCore/Serialization/EditContext.h>
#include <Include/PhysX/NativeTypeIdentifiers.h>
#include <AzCore/Math/MathUtils.h>
#include <PhysX/PhysXLocks.h>
#include <AzFramework/Physics/Common/PhysicsSimulatedBody.h>

namespace PhysX
{
    namespace JointConstants
    {
        // Setting swing limits to very small values can cause extreme stability problems, so clamp above a small
        // threshold.
        static const float MinSwingLimitDegrees = 1.0f;
    } // namespace JointConstants

    AzPhysics::SimulatedBody* Joint::GetParentBody() const
    {
        return m_parentBody;
    }

    AzPhysics::SimulatedBody* Joint::GetChildBody() const
    {
        return m_childBody;
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

    physx::PxRigidActor* GetPxRigidActor(AzPhysics::SimulatedBody* worldBody)
    {
        if (worldBody && static_cast<physx::PxBase*>(worldBody->GetNativePointer())->is<physx::PxRigidActor>())
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

    Joint::Joint(physx::PxJoint* pxJoint, AzPhysics::SimulatedBody* parentBody,
        AzPhysics::SimulatedBody* childBody)
        : m_parentBody(parentBody)
        , m_childBody(childBody)
    {
        m_pxJoint = PxJointUniquePtr(pxJoint, releasePxJoint);
    }

    bool Joint::SetPxActors()
    {
        physx::PxRigidActor* parentActor = GetPxRigidActor(m_parentBody);
        physx::PxRigidActor* childActor = GetPxRigidActor(m_childBody);
        if (!parentActor && !childActor)
        {
            AZ_Error("PhysX Joint", false, "Invalid PhysX actors in joint - at least one must be a PxRigidActor.");
            return false;
        }

        m_pxJoint->setActors(parentActor, childActor);
        return true;
    }

    void Joint::SetParentBody(AzPhysics::SimulatedBody* parentBody)
    {
        if (IsAtLeastOneDynamic(parentBody, m_childBody))
        {
            m_parentBody = parentBody;
            SetPxActors();
        }
        else
        {
            AZ_Warning("PhysX Joint", false, "Call to SetParentBody would result in invalid joint - at least one "
                "body in a joint must be dynamic.");
        }
    }

    void Joint::SetChildBody(AzPhysics::SimulatedBody* childBody)
    {
        if (IsAtLeastOneDynamic(m_parentBody, childBody))
        {
            m_childBody = childBody;
            SetPxActors();
        }
        else
        {
            AZ_Warning("PhysX Joint", false, "Call to SetChildBody would result in invalid joint - at least one "
                "body in a joint must be dynamic.");
        }
    }

    const AZStd::string& Joint::GetName() const
    {
        return m_name;
    }

    void Joint::SetName(const AZStd::string& name)
    {
        m_name = name;
    }

    void* Joint::GetNativePointer()
    {
        return m_pxJoint.get();
    }

    const AZ::Crc32 D6Joint::GetNativeType() const
    {
        return NativeTypeIdentifiers::D6Joint;
    }

    void D6Joint::GenerateJointLimitVisualizationData(
        float scale,
        AZ::u32 angularSubdivisions,
        AZ::u32 radialSubdivisions,
        [[maybe_unused]] AZStd::vector<AZ::Vector3>& vertexBufferOut,
        [[maybe_unused]] AZStd::vector<AZ::u32>& indexBufferOut,
        AZStd::vector<AZ::Vector3>& lineBufferOut,
        AZStd::vector<bool>& lineValidityBufferOut)
    {
        const AZ::u32 angularSubdivisionsClamped = AZ::GetClamp(angularSubdivisions, 4u, 32u);
        const AZ::u32 radialSubdivisionsClamped = AZ::GetClamp(radialSubdivisions, 1u, 4u);

        const physx::PxD6Joint* joint = static_cast<physx::PxD6Joint*>(m_pxJoint.get());
        const AZ::Quaternion parentLocalRotation = PxMathConvert(joint->getLocalPose(physx::PxJointActorIndex::eACTOR0).q);
        const AZ::Quaternion parentWorldRotation = m_parentBody ? m_parentBody->GetOrientation() : AZ::Quaternion::CreateIdentity();
        const AZ::Quaternion childLocalRotation = PxMathConvert(joint->getLocalPose(physx::PxJointActorIndex::eACTOR1).q);
        const AZ::Quaternion childWorldRotation = m_childBody ? m_childBody->GetOrientation() : AZ::Quaternion::CreateIdentity();

        const float swingAngleY = joint->getSwingYAngle();
        const float swingAngleZ = joint->getSwingZAngle();
        const float swingLimitY = joint->getSwingLimit().yAngle;
        const float swingLimitZ = joint->getSwingLimit().zAngle;
        const float twistAngle = joint->getTwist();
        const float twistLimitLower = joint->getTwistLimit().lower;
        const float twistLimitUpper = joint->getTwistLimit().upper;

        JointUtils::AppendD6SwingConeToLineBuffer(parentLocalRotation, swingAngleY, swingAngleZ, swingLimitY, swingLimitZ,
            scale, angularSubdivisionsClamped, radialSubdivisionsClamped, lineBufferOut, lineValidityBufferOut);
        JointUtils::AppendD6TwistArcToLineBuffer(parentLocalRotation, twistAngle, twistLimitLower, twistLimitUpper,
            scale, angularSubdivisionsClamped, radialSubdivisionsClamped, lineBufferOut, lineValidityBufferOut);
        JointUtils::AppendD6CurrentTwistToLineBuffer(parentLocalRotation, twistAngle, twistLimitLower, twistLimitUpper,
            scale, lineBufferOut, lineValidityBufferOut);

        // draw the X-axis of the child joint frame
        // make the axis slightly longer than the radius of the twist arc so that it is easy to see
        float axisLength = 1.25f * scale;
        AZ::Vector3 childAxis = (parentWorldRotation.GetConjugate() * childWorldRotation * childLocalRotation).TransformVector(
            AZ::Vector3::CreateAxisX(axisLength));
        lineBufferOut.push_back(AZ::Vector3::CreateZero());
        lineBufferOut.push_back(childAxis);
    }

    const AZ::Crc32 FixedJoint::GetNativeType() const
    {
        return NativeTypeIdentifiers::FixedJoint;
    }

    const AZ::Crc32 HingeJoint::GetNativeType() const
    {
        return NativeTypeIdentifiers::HingeJoint;
    }

    const AZ::Crc32 BallJoint::GetNativeType() const
    {
        return NativeTypeIdentifiers::BallJoint;
    }

    void GenericJointConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<GenericJointConfiguration>()
                ->Version(2, &VersionConverter)
                ->Field("Follower Local Transform", &GenericJointConfiguration::m_localTransformFromFollower)
                ->Field("Maximum Force", &GenericJointConfiguration::m_forceMax)
                ->Field("Maximum Torque", &GenericJointConfiguration::m_torqueMax)
                ->Field("Lead Entity", &GenericJointConfiguration::m_leadEntity)
                ->Field("Follower Entity", &GenericJointConfiguration::m_followerEntity)
                ->Field("Flags", &GenericJointConfiguration::m_flags)
                ;
        }
    }

    bool GenericJointConfiguration::VersionConverter(
        AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& classElement)
    {
        if (classElement.GetVersion() <= 1)
        {
            // Convert bool breakable to GenericJointConfiguration::GenericJointFlag
            const int breakableElementIndex = classElement.FindElement(AZ_CRC("Breakable", 0xb274ecd4));
            if (breakableElementIndex >= 0)
            {
                bool breakable = false;
                AZ::SerializeContext::DataElementNode& breakableNode = classElement.GetSubElement(breakableElementIndex);
                breakableNode.GetData(breakable);
                if (!breakableNode.GetData<bool>(breakable))
                {
                    return false;
                }
                classElement.RemoveElement(breakableElementIndex);
                GenericJointConfiguration::GenericJointFlag flags = breakable ? GenericJointConfiguration::GenericJointFlag::Breakable : GenericJointConfiguration::GenericJointFlag::None;
                classElement.AddElementWithData(context, "Flags", flags);
            }
        }

        return true;
    }

    GenericJointConfiguration::GenericJointConfiguration(float forceMax,
        float torqueMax,
        AZ::Transform localTransformFromFollower,
        AZ::EntityId leadEntity,
        AZ::EntityId followerEntity,
        GenericJointFlag flags)
        : m_forceMax(forceMax)
        , m_torqueMax(torqueMax)
        , m_localTransformFromFollower(localTransformFromFollower)
        , m_leadEntity(leadEntity)
        , m_followerEntity(followerEntity)
        , m_flags(flags)
    {
    }

    bool GenericJointConfiguration::GetFlag(GenericJointFlag flag)
    {
        return static_cast<bool>(m_flags & flag);
    }

    void GenericJointLimitsConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<GenericJointLimitsConfiguration>()
                ->Version(1)
                ->Field("First Limit", &GenericJointLimitsConfiguration::m_limitFirst)
                ->Field("Second Limit", &GenericJointLimitsConfiguration::m_limitSecond)
                ->Field("Tolerance", &GenericJointLimitsConfiguration::m_tolerance)
                ->Field("Is Limited", &GenericJointLimitsConfiguration::m_isLimited)
                ->Field("Is Soft Limit", &GenericJointLimitsConfiguration::m_isSoftLimit)
                ->Field("Damping", &GenericJointLimitsConfiguration::m_damping)
                ->Field("Spring", &GenericJointLimitsConfiguration::m_stiffness)
                ;
        }
    }

    GenericJointLimitsConfiguration::GenericJointLimitsConfiguration(float damping
        , bool isLimited
        , bool isSoftLimit
        , float limitFirst
        , float limitSecond
        , float stiffness
        , float tolerance)
        : m_damping(damping)
        , m_isLimited(isLimited)
        , m_isSoftLimit(isSoftLimit)
        , m_limitFirst(limitFirst)
        , m_limitSecond(limitSecond)
        , m_stiffness(stiffness)
        , m_tolerance(tolerance)
    {
    }

    AZStd::vector<AZ::TypeId> JointUtils::GetSupportedJointTypes()
    {
        return AZStd::vector<AZ::TypeId>
        {
            D6JointLimitConfiguration::RTTI_Type()
        };
    }

    AZStd::shared_ptr<Physics::JointLimitConfiguration> JointUtils::CreateJointLimitConfiguration([[maybe_unused]] AZ::TypeId jointType)
    {
        return AZStd::make_shared<D6JointLimitConfiguration>();
    }

    AZStd::shared_ptr<Physics::Joint> JointUtils::CreateJoint(const AZStd::shared_ptr<Physics::JointLimitConfiguration>& configuration,
        AzPhysics::SimulatedBody* parentBody, AzPhysics::SimulatedBody* childBody)
    {
        if (!configuration)
        {
            AZ_Warning("PhysX Joint", false, "CreateJoint failed - configuration was nullptr.");
            return nullptr;
        }

        if (auto d6Config = AZStd::rtti_pointer_cast<D6JointLimitConfiguration>(configuration))
        {
            if (!IsAtLeastOneDynamic(parentBody, childBody))
            {
                AZ_Warning("PhysX Joint", false, "CreateJoint failed - at least one body must be dynamic.");
                return nullptr;
            }

            physx::PxRigidActor* parentActor = GetPxRigidActor(parentBody);
            physx::PxRigidActor* childActor = GetPxRigidActor(childBody);

            if (!parentActor && !childActor)
            {
                AZ_Warning("PhysX Joint", false, "CreateJoint failed - at least one body must be a PxRigidActor.");
                return nullptr;
            }

            const physx::PxTransform parentWorldTransform = parentActor ? parentActor->getGlobalPose() : physx::PxTransform(physx::PxIdentity);
            const physx::PxTransform childWorldTransform = childActor ? childActor->getGlobalPose() : physx::PxTransform(physx::PxIdentity);
            const physx::PxVec3 childOffset = childWorldTransform.p - parentWorldTransform.p;
            physx::PxTransform parentLocalTransform(PxMathConvert(d6Config->m_parentLocalRotation).getNormalized());
            const physx::PxTransform childLocalTransform(PxMathConvert(d6Config->m_childLocalRotation).getNormalized());
            parentLocalTransform.p = parentWorldTransform.q.rotateInv(childOffset);

            physx::PxD6Joint* joint = PxD6JointCreate(PxGetPhysics(), parentActor, parentLocalTransform,
                childActor, childLocalTransform);

            joint->setMotion(physx::PxD6Axis::eTWIST, physx::PxD6Motion::eLIMITED);
            joint->setMotion(physx::PxD6Axis::eSWING1, physx::PxD6Motion::eLIMITED);
            joint->setMotion(physx::PxD6Axis::eSWING2, physx::PxD6Motion::eLIMITED);

            AZ_Warning("PhysX Joint",
                d6Config->m_swingLimitY >= JointConstants::MinSwingLimitDegrees && d6Config->m_swingLimitZ >= JointConstants::MinSwingLimitDegrees,
                "Very small swing limit requested for joint between \"%s\" and \"%s\", increasing to %f degrees to improve stability",
                parentActor ? parentActor->getName() : "world", childActor ? childActor->getName() : "world",
                JointConstants::MinSwingLimitDegrees);
            float swingLimitY = AZ::DegToRad(AZ::GetMax(JointConstants::MinSwingLimitDegrees, d6Config->m_swingLimitY));
            float swingLimitZ = AZ::DegToRad(AZ::GetMax(JointConstants::MinSwingLimitDegrees, d6Config->m_swingLimitZ));
            physx::PxJointLimitCone limitCone(swingLimitY, swingLimitZ);
            joint->setSwingLimit(limitCone);

            float twistLower = AZ::DegToRad(AZStd::GetMin(d6Config->m_twistLimitLower, d6Config->m_twistLimitUpper));
            float twistUpper = AZ::DegToRad(AZStd::GetMax(d6Config->m_twistLimitLower, d6Config->m_twistLimitUpper));
            physx::PxJointAngularLimitPair twistLimitPair(twistLower, twistUpper);
            joint->setTwistLimit(twistLimitPair);

            return AZStd::make_shared<D6Joint>(joint, parentBody, childBody);
        }
        else
        {
            AZ_Warning("PhysX Joint", false, "Unrecognized joint limit configuration.");
            return nullptr;
        }
    }

    D6JointState JointUtils::CalculateD6JointState(
        const AZ::Quaternion& parentWorldRotation,
        const AZ::Quaternion& parentLocalRotation,
        const AZ::Quaternion& childWorldRotation,
        const AZ::Quaternion& childLocalRotation)
    {
        D6JointState result;

        const AZ::Quaternion parentRotation = parentWorldRotation * parentLocalRotation;
        const AZ::Quaternion childRotation = childWorldRotation * childLocalRotation;
        const AZ::Quaternion relativeRotation = parentRotation.GetConjugate() * childRotation;
        AZ::Quaternion twistQuat = AZ::IsClose(relativeRotation.GetX(), 0.0f, AZ::Constants::FloatEpsilon)
            ? AZ::Quaternion::CreateIdentity()
            : AZ::Quaternion(relativeRotation.GetX(), 0.0f, 0.0f, relativeRotation.GetW()).GetNormalized();
        AZ::Quaternion swingQuat = relativeRotation * twistQuat.GetConjugate();

        // make sure the twist angle has the correct sign for the rotation
        twistQuat *= AZ::GetSign(twistQuat.GetX());
        // make sure we get the shortest arcs for the swing degrees of freedom
        swingQuat *= AZ::GetSign(swingQuat.GetW());
        // the PhysX swing limits work in terms of tan quarter angles
        result.m_swingAngleY = 4.0f * atan2f(swingQuat.GetY(), 1.0f + swingQuat.GetW());
        result.m_swingAngleZ = 4.0f * atan2f(swingQuat.GetZ(), 1.0f + swingQuat.GetW());
        const float twistAngle = twistQuat.GetAngle();
        // GetAngle returns an angle in the range 0..2 pi, but the twist limits work in the range -pi..pi
        const float wrappedTwistAngle = twistAngle > AZ::Constants::Pi ? twistAngle - AZ::Constants::TwoPi : twistAngle;
        result.m_twistAngle = wrappedTwistAngle;

        return result;
    }

    bool JointUtils::IsD6SwingValid(
        float swingAngleY,
        float swingAngleZ,
        float swingLimitY,
        float swingLimitZ)
    {
        const float epsilon = AZ::Constants::FloatEpsilon;
        const float yFactor = tanf(0.25f * swingAngleY) / AZStd::GetMax(epsilon, tanf(0.25f * swingLimitY));
        const float zFactor = tanf(0.25f * swingAngleZ) / AZStd::GetMax(epsilon, tanf(0.25f * swingLimitZ));

        return (yFactor * yFactor + zFactor * zFactor <= 1.0f + epsilon);
    }

    void JointUtils::AppendD6SwingConeToLineBuffer(
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
        const float tanQuarterSwingZ = tanf(0.25f * swingLimitZ);
        const float tanQuarterSwingY = tanf(0.25f * swingLimitY);

        AZ::Vector3 previousRadialVector = AZ::Vector3::CreateZero();
        for (AZ::u32 angularIndex = 0; angularIndex <= angularSubdivisions; angularIndex++)
        {
            const float angle = AZ::Constants::TwoPi / angularSubdivisions * angularIndex;
            // the axis about which to rotate the x-axis to get the radial vector for this segment of the cone
            const AZ::Vector3 rotationAxis(0, -tanQuarterSwingY * sinf(angle), tanQuarterSwingZ * cosf(angle));
            const float normalizationFactor = rotationAxis.GetLengthSq();
            const AZ::Quaternion radialVectorRotation = 1.0f / (1.0f + normalizationFactor) *
                AZ::Quaternion::CreateFromVector3AndValue(2.0f * rotationAxis, 1.0f - normalizationFactor);
            const AZ::Vector3 radialVector = (parentLocalRotation * radialVectorRotation).TransformVector(AZ::Vector3::CreateAxisX(scale));

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

    void JointUtils::AppendD6TwistArcToLineBuffer(
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
            const AZ::Vector3 radialVector = parentLocalRotation.TransformVector(scale * AZ::Vector3(0.0f, cosf(angle), sinf(angle)));

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

    void JointUtils::AppendD6CurrentTwistToLineBuffer(
        const AZ::Quaternion& parentLocalRotation,
        float twistAngle,
        [[maybe_unused]] float twistLimitLower,
        [[maybe_unused]] float twistLimitUpper,
        float scale,
        AZStd::vector<AZ::Vector3>& lineBufferOut,
        AZStd::vector<bool>& lineValidityBufferOut
    )
    {
        const AZ::Vector3 twistVector = parentLocalRotation.TransformVector(1.25f * scale * AZ::Vector3(0.0f, cosf(twistAngle), sinf(twistAngle)));
        lineBufferOut.push_back(AZ::Vector3::CreateZero());
        lineBufferOut.push_back(twistVector);
        lineValidityBufferOut.push_back(true);
    }

    void JointUtils::GenerateJointLimitVisualizationData(
        const Physics::JointLimitConfiguration& configuration,
        const AZ::Quaternion& parentRotation,
        const AZ::Quaternion& childRotation,
        float scale,
        AZ::u32 angularSubdivisions,
        AZ::u32 radialSubdivisions,
        [[maybe_unused]] AZStd::vector<AZ::Vector3>& vertexBufferOut,
        [[maybe_unused]] AZStd::vector<AZ::u32>& indexBufferOut,
        AZStd::vector<AZ::Vector3>& lineBufferOut,
        AZStd::vector<bool>& lineValidityBufferOut)
    {
        if (const auto d6JointConfiguration = azrtti_cast<const D6JointLimitConfiguration*>(&configuration))
        {
            const AZ::u32 angularSubdivisionsClamped = AZ::GetClamp(angularSubdivisions, 4u, 32u);
            const AZ::u32 radialSubdivisionsClamped = AZ::GetClamp(radialSubdivisions, 1u, 4u);

            const D6JointState jointState = CalculateD6JointState(parentRotation, d6JointConfiguration->m_parentLocalRotation,
                childRotation, d6JointConfiguration->m_childLocalRotation);
            const float swingAngleY = jointState.m_swingAngleY;
            const float swingAngleZ = jointState.m_swingAngleZ;
            const float twistAngle = jointState.m_twistAngle;
            const float swingLimitY = AZ::DegToRad(d6JointConfiguration->m_swingLimitY);
            const float swingLimitZ = AZ::DegToRad(d6JointConfiguration->m_swingLimitZ);
            const float twistLimitLower = AZ::DegToRad(d6JointConfiguration->m_twistLimitLower);
            const float twistLimitUpper = AZ::DegToRad(d6JointConfiguration->m_twistLimitUpper);

            AppendD6SwingConeToLineBuffer(d6JointConfiguration->m_parentLocalRotation, swingAngleY, swingAngleZ, swingLimitY,
                swingLimitZ, scale, angularSubdivisionsClamped, radialSubdivisionsClamped, lineBufferOut, lineValidityBufferOut);
            AppendD6TwistArcToLineBuffer(d6JointConfiguration->m_parentLocalRotation, twistAngle, twistLimitLower, twistLimitUpper,
                scale, angularSubdivisionsClamped, radialSubdivisionsClamped, lineBufferOut, lineValidityBufferOut);
            AppendD6CurrentTwistToLineBuffer(d6JointConfiguration->m_parentLocalRotation, twistAngle, twistLimitLower,
                twistLimitUpper, scale, lineBufferOut, lineValidityBufferOut);
        }
    }

    AZStd::unique_ptr<Physics::JointLimitConfiguration> JointUtils::ComputeInitialJointLimitConfiguration(
        const AZ::TypeId& jointLimitTypeId,
        const AZ::Quaternion& parentWorldRotation,
        const AZ::Quaternion& childWorldRotation,
        const AZ::Vector3& axis,
        const AZStd::vector<AZ::Quaternion>& exampleLocalRotations)
    {
        AZ_UNUSED(exampleLocalRotations);

        if (jointLimitTypeId == D6JointLimitConfiguration::RTTI_Type())
        {
            const AZ::Vector3& normalizedAxis = axis.IsZero()
                ? AZ::Vector3::CreateAxisX()
                : axis.GetNormalized();

            D6JointLimitConfiguration d6JointLimitConfig;
            const AZ::Quaternion childLocalRotation = AZ::Quaternion::CreateShortestArc(AZ::Vector3::CreateAxisX(),
                childWorldRotation.GetConjugate().TransformVector(normalizedAxis));
            d6JointLimitConfig.m_childLocalRotation = childLocalRotation;
            d6JointLimitConfig.m_parentLocalRotation = parentWorldRotation.GetConjugate() * childWorldRotation * childLocalRotation;

            return AZStd::make_unique<D6JointLimitConfiguration>(d6JointLimitConfig);
        }

        AZ_Warning("PhysX Joint Utils", false, "Unsupported joint type in ComputeInitialJointLimitConfiguration");
        return nullptr;
    }

    const char* D6JointLimitConfiguration::GetTypeName()
    {
        return "D6 Joint";
    }

    void D6JointLimitConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<D6JointLimitConfiguration, Physics::JointLimitConfiguration>()
                ->Version(1)
                ->Field("SwingLimitY", &D6JointLimitConfiguration::m_swingLimitY)
                ->Field("SwingLimitZ", &D6JointLimitConfiguration::m_swingLimitZ)
                ->Field("TwistLowerLimit", &D6JointLimitConfiguration::m_twistLimitLower)
                ->Field("TwistUpperLimit", &D6JointLimitConfiguration::m_twistLimitUpper)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<D6JointLimitConfiguration>(
                    "PhysX D6 Joint Configuration", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &D6JointLimitConfiguration::m_swingLimitY, "Swing limit Y",
                        "Maximum angle from the Y axis of the joint frame")
                    ->Attribute(AZ::Edit::Attributes::Suffix, " degrees")
                    ->Attribute(AZ::Edit::Attributes::Min, JointConstants::MinSwingLimitDegrees)
                    ->Attribute(AZ::Edit::Attributes::Max, 180.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &D6JointLimitConfiguration::m_swingLimitZ, "Swing limit Z",
                        "Maximum angle from the Z axis of the joint frame")
                    ->Attribute(AZ::Edit::Attributes::Suffix, " degrees")
                    ->Attribute(AZ::Edit::Attributes::Min, JointConstants::MinSwingLimitDegrees)
                    ->Attribute(AZ::Edit::Attributes::Max, 180.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &D6JointLimitConfiguration::m_twistLimitLower, "Twist lower limit",
                        "Lower limit for rotation about the X axis of the joint frame")
                    ->Attribute(AZ::Edit::Attributes::Suffix, " degrees")
                    ->Attribute(AZ::Edit::Attributes::Min, -180.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 180.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &D6JointLimitConfiguration::m_twistLimitUpper, "Twist upper limit",
                        "Upper limit for rotation about the X axis of the joint frame")
                    ->Attribute(AZ::Edit::Attributes::Suffix, " degrees")
                    ->Attribute(AZ::Edit::Attributes::Min, -180.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 180.0f)
                ;
            }
        }
    }
} // namespace PhysX
