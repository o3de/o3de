/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <System/PhysXJointInterface.h>
#include <AzFramework/Physics/Configuration/JointConfiguration.h>
#include <AzCore/std/optional.h>

#include <PhysX/Joint/Configuration/PhysXJointConfiguration.h>
#include <PhysX/Debug/PhysXDebugConfiguration.h>

namespace PhysX
{
    namespace
    {
        struct D6JointState
        {
            float m_swingAngleY;
            float m_swingAngleZ;
            float m_twistAngle;
        };

        D6JointState CalculateD6JointState(
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

        bool IsD6SwingValid(float swingAngleY, float swingAngleZ, float swingLimitY, float swingLimitZ)
        {
            const float epsilon = AZ::Constants::FloatEpsilon;
            const float yFactor = tanf(0.25f * swingAngleY) / AZStd::GetMax(epsilon, tanf(0.25f * swingLimitY));
            const float zFactor = tanf(0.25f * swingAngleZ) / AZStd::GetMax(epsilon, tanf(0.25f * swingLimitZ));

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

        template<typename Configuration>
        AZStd::unique_ptr<AzPhysics::JointConfiguration> ConfigurationFactory(
            const AZ::Quaternion& parentLocalRotation, const AZ::Quaternion& childLocalRotation)
        {
            auto jointConfig = AZStd::make_unique<Configuration>();
            jointConfig->m_childLocalRotation = childLocalRotation;
            jointConfig->m_parentLocalRotation = parentLocalRotation;

            return jointConfig;
        }

    } // namespace

    const AZStd::vector<AZ::TypeId> PhysXJointHelpersInterface::GetSupportedJointTypeIds() const
    {
        static AZStd::vector<AZ::TypeId> jointTypes = {
            D6JointLimitConfiguration::RTTI_Type(),
            FixedJointConfiguration::RTTI_Type(),
            BallJointConfiguration::RTTI_Type(),
            HingeJointConfiguration::RTTI_Type()
        };
        return jointTypes;
    }

    AZStd::optional<const AZ::TypeId> PhysXJointHelpersInterface::GetSupportedJointTypeId(AzPhysics::JointType typeEnum) const
    {
        switch (typeEnum)
        {
        case AzPhysics::JointType::D6Joint:
            return azrtti_typeid<D6JointLimitConfiguration>();
        case AzPhysics::JointType::FixedJoint:
            return azrtti_typeid<FixedJointConfiguration>();
        case AzPhysics::JointType::BallJoint:
            return azrtti_typeid<BallJointConfiguration>();
        case AzPhysics::JointType::HingeJoint:
            return azrtti_typeid<HingeJointConfiguration>();
        default:
            AZ_Warning("PhysX Joint Utils", false, "Unsupported joint type in GetSupportedJointTypeId");
        }
        return AZStd::nullopt;
    }

    AZStd::unique_ptr<AzPhysics::JointConfiguration> PhysXJointHelpersInterface::ComputeInitialJointLimitConfiguration(
        const AZ::TypeId& jointLimitTypeId,
        const AZ::Quaternion& parentWorldRotation,
        const AZ::Quaternion& childWorldRotation,
        const AZ::Vector3& axis,
        [[maybe_unused]] const AZStd::vector<AZ::Quaternion>& exampleLocalRotations)
    {
        const AZ::Vector3& normalizedAxis = axis.IsZero() ? AZ::Vector3::CreateAxisX() : axis.GetNormalized();
        const AZ::Quaternion childLocalRotation = AZ::Quaternion::CreateShortestArc(
            AZ::Vector3::CreateAxisX(), childWorldRotation.GetConjugate().TransformVector(normalizedAxis));
        const AZ::Quaternion parentLocalRotation = parentWorldRotation.GetConjugate() * childWorldRotation * childLocalRotation;

        if (jointLimitTypeId == azrtti_typeid<D6JointLimitConfiguration>())
        {
            return ConfigurationFactory<D6JointLimitConfiguration>(parentLocalRotation, childLocalRotation);
        }
        else if (jointLimitTypeId == azrtti_typeid<FixedJointConfiguration>())
        {
            return ConfigurationFactory<FixedJointConfiguration>(parentLocalRotation, childLocalRotation);
        }
        else if (jointLimitTypeId == azrtti_typeid<BallJointConfiguration>())
        {
            return ConfigurationFactory<BallJointConfiguration>(parentLocalRotation, childLocalRotation);
        }
        else if (jointLimitTypeId == azrtti_typeid<HingeJointConfiguration>())
        {
            return ConfigurationFactory<HingeJointConfiguration>(parentLocalRotation, childLocalRotation);
        }

        AZ_Warning("PhysX Joint Utils", false, "Unsupported joint type in ComputeInitialJointLimitConfiguration");
        return nullptr;
    }

    void PhysXJointHelpersInterface::GenerateJointLimitVisualizationData(
        const AzPhysics::JointConfiguration& configuration,
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

            const D6JointState jointState = CalculateD6JointState(
                parentRotation, d6JointConfiguration->m_parentLocalRotation, childRotation, d6JointConfiguration->m_childLocalRotation);
            const float swingAngleY = jointState.m_swingAngleY;
            const float swingAngleZ = jointState.m_swingAngleZ;
            const float twistAngle = jointState.m_twistAngle;
            const float swingLimitY = AZ::DegToRad(d6JointConfiguration->m_swingLimitY);
            const float swingLimitZ = AZ::DegToRad(d6JointConfiguration->m_swingLimitZ);
            const float twistLimitLower = AZ::DegToRad(d6JointConfiguration->m_twistLimitLower);
            const float twistLimitUpper = AZ::DegToRad(d6JointConfiguration->m_twistLimitUpper);

            AppendD6SwingConeToLineBuffer(
                d6JointConfiguration->m_parentLocalRotation, swingAngleY, swingAngleZ, swingLimitY, swingLimitZ, scale,
                angularSubdivisionsClamped, radialSubdivisionsClamped, lineBufferOut, lineValidityBufferOut);
            AppendD6TwistArcToLineBuffer(
                d6JointConfiguration->m_parentLocalRotation, twistAngle, twistLimitLower, twistLimitUpper, scale,
                angularSubdivisionsClamped, radialSubdivisionsClamped, lineBufferOut, lineValidityBufferOut);
            AppendD6CurrentTwistToLineBuffer(
                d6JointConfiguration->m_parentLocalRotation, twistAngle, twistLimitLower, twistLimitUpper, scale, lineBufferOut,
                lineValidityBufferOut);
        }
    }
}
