/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <EMotionFX/Source/ConstraintTransformRotationAngles.h>
#include <EMotionFX/Source/DebugDraw.h>
#include <EMotionFX/Source/EMotionFXManager.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(ConstraintTransformRotationAngles, Integration::EMotionFXAllocator)

    // Constructor, which inits on default values
    ConstraintTransformRotationAngles::ConstraintTransformRotationAngles()
    {
        const float angleX = 0.382683f; // 45 degrees --> sin(45degInRadians * 0.5f)
        const float angleY = 0.382683f; // 45 degrees
        const float twistAngle = 0.0f; // 0 degrees

        m_minRotationAngles.Set(-angleX, -angleY);
        m_maxRotationAngles.Set(angleX, angleY);
        m_minTwist = twistAngle;
        m_maxTwist = twistAngle;
        m_twistAxis = AXIS_Y;
    }

    uint32 ConstraintTransformRotationAngles::GetType() const
    {
        return TYPE_ID;
    }

    const char* ConstraintTransformRotationAngles::GetTypeString() const
    {
        return "ConstraintTransformRotationAngles";
    }

    void ConstraintTransformRotationAngles::SetMinRotationAngles(const AZ::Vector2& minSwingDegrees)
    {
        const float angleX = MCore::Math::Sin(MCore::Math::DegreesToRadians(minSwingDegrees.GetX()) * 0.5f);
        const float angleY = MCore::Math::Sin(MCore::Math::DegreesToRadians(minSwingDegrees.GetY()) * 0.5f);
        m_minRotationAngles.Set(angleX, angleY);
    }

    void ConstraintTransformRotationAngles::SetMaxRotationAngles(const AZ::Vector2& maxSwingDegrees)
    {
        const float angleX = MCore::Math::Sin(MCore::Math::DegreesToRadians(maxSwingDegrees.GetX()) * 0.5f);
        const float angleY = MCore::Math::Sin(MCore::Math::DegreesToRadians(maxSwingDegrees.GetY()) * 0.5f);
        m_maxRotationAngles.Set(angleX, angleY);
    }

    void ConstraintTransformRotationAngles::SetMinTwistAngle(float minTwistDegrees)
    {
        m_minTwist = MCore::Math::Sin(MCore::Math::DegreesToRadians(minTwistDegrees) * 0.5f);
    }

    void ConstraintTransformRotationAngles::SetMaxTwistAngle(float maxTwistDegrees)
    {
        m_maxTwist = MCore::Math::Sin(MCore::Math::DegreesToRadians(maxTwistDegrees) * 0.5f);
    }

    void ConstraintTransformRotationAngles::SetTwistAxis(ConstraintTransformRotationAngles::EAxis axis)
    {
        m_twistAxis = axis;
    }

    AZ::Vector2 ConstraintTransformRotationAngles::GetMinRotationAnglesDegrees() const
    {
        return AZ::Vector2(MCore::Math::RadiansToDegrees(MCore::Math::ASin(m_minRotationAngles.GetX()) * 2.0f),
            MCore::Math::RadiansToDegrees(MCore::Math::ASin(m_minRotationAngles.GetY()) * 2.0f));
    }

    AZ::Vector2 ConstraintTransformRotationAngles::GetMaxRotationAnglesDegrees() const
    {
        return AZ::Vector2(MCore::Math::RadiansToDegrees(MCore::Math::ASin(m_maxRotationAngles.GetX()) * 2.0f),
            MCore::Math::RadiansToDegrees(MCore::Math::ASin(m_maxRotationAngles.GetY()) * 2.0f));
    }

    AZ::Vector2 ConstraintTransformRotationAngles::GetMinRotationAnglesRadians() const
    {
        return AZ::Vector2(MCore::Math::ASin(m_minRotationAngles.GetX()) * 2.0f,
            MCore::Math::ASin(m_minRotationAngles.GetY()) * 2.0f);
    }

    AZ::Vector2 ConstraintTransformRotationAngles::GetMaxRotationAnglesRadians() const
    {
        return AZ::Vector2(MCore::Math::ASin(m_maxRotationAngles.GetX()) * 2.0f,
            MCore::Math::ASin(m_maxRotationAngles.GetY()) * 2.0f);
    }

    float ConstraintTransformRotationAngles::GetMinTwistAngle() const
    {
        return MCore::Math::RadiansToDegrees(MCore::Math::ASin(m_minTwist) * 2.0f);
    }

    float ConstraintTransformRotationAngles::GetMaxTwistAngle() const
    {
        return MCore::Math::RadiansToDegrees(MCore::Math::ASin(m_maxTwist) * 2.0f);
    }

    ConstraintTransformRotationAngles::EAxis ConstraintTransformRotationAngles::GetTwistAxis() const
    {
        return m_twistAxis;
    }

    // The main execution function, which performs the actual constraint.
    void ConstraintTransformRotationAngles::Execute()
    {
        AZ::Quaternion q = m_transform.m_rotation;

        // Always keep w positive.
        if (q.GetW() < 0.0f)
        {
            q = -q;
        }

        // Get the axes indices for swing
        uint32 swingX;
        uint32 swingY;
        switch (m_twistAxis)
        {
            // Twist is the X-axis.
            case AXIS_X:
                swingX = 2;
                swingY = 1;
                break;

            // Twist is the Y-axis.
            case AXIS_Y:
                swingX = 2;
                swingY = 0;
                break;

            // Twist is the Z-axis.
            case AXIS_Z:
                swingX = 1;
                swingY = 0;
                break;

            default:
                MCORE_ASSERT(false);
                swingX = 2;
                swingY = 0;
        }

        // Calculate the twist quaternion, based on over which axis we assume there is twist.
        AZ::Quaternion twist;
        const float twistAngle = q.GetElement(m_twistAxis);
        const float s = twistAngle * twistAngle + q.GetW() * q.GetW();
        if (!MCore::Math::IsFloatZero(s))
        {
            const float r = MCore::Math::InvSqrt(s);
            twist.SetElement(swingX, 0.0f);
            twist.SetElement(swingY, 0.0f);
            twist.SetElement(m_twistAxis, MCore::Clamp(twistAngle * r, m_minTwist, m_maxTwist));
            twist.SetW(MCore::Math::Sqrt(MCore::Max<float>(0.0f, 1.0f - twist.GetElement(m_twistAxis) * twist.GetElement(m_twistAxis))));
        }
        else
        {
            twist = AZ::Quaternion::CreateIdentity();
        }

        // Remove the twist from the input rotation so that we are left with a swing and then limit the swing.
        AZ::Quaternion swing = q * twist.GetConjugate();
        swing.SetElement(swingX, MCore::Clamp(static_cast<float>(swing.GetElement(swingX)), m_minRotationAngles.GetX(), m_maxRotationAngles.GetX()));
        swing.SetElement(swingY, MCore::Clamp(static_cast<float>(swing.GetElement(swingY)), m_minRotationAngles.GetY(), m_maxRotationAngles.GetY()));
        swing.SetElement(m_twistAxis, 0.0f);
        swing.SetW(MCore::Math::Sqrt(MCore::Max<float>(0.0f, 1.0f - swing.GetElement(swingX) * swing.GetElement(swingX) - swing.GetElement(swingY) * swing.GetElement(swingY))));

        // Combine the limited swing and twist again into a final rotation.
        m_transform.m_rotation = swing * twist;
    }

    AZ::Vector3 ConstraintTransformRotationAngles::GetSphericalPos(float x, float y) const
    {
        float sx = MCore::Math::Sin(x);
        float sy = MCore::Math::Sin(y);
        AZ::Vector3 pos(
            sx,
            sy,
            MCore::Math::SafeSqrt(1.0f - sx * sx - sy * sy));
        pos.Normalize();
        return pos;
    }

    void ConstraintTransformRotationAngles::DrawSphericalLine(ActorInstance* actorInstance, const AZ::Vector2& start, const AZ::Vector2& end, uint32 numSteps, const AZ::Color& color, float radius, const AZ::Transform& offset) const
    {
        const AZ::Vector2 totalVector = end - start;
        const AZ::Vector2 stepVector = totalVector / static_cast<float>(numSteps);

        EMotionFX::DebugDraw& debugDraw = GetDebugDraw();
        DebugDraw::ActorInstanceData* drawData = debugDraw.GetActorInstanceData(actorInstance);
        drawData->Lock();

        AZ::Vector2 current = start;
        AZ::Transform offsetRotation(offset);
        offsetRotation.SetTranslation(AZ::Vector3::CreateZero());

        AZ::Vector3 lastPos = offsetRotation.TransformPoint(GetSphericalPos(start.GetX(), -start.GetY())) * radius;
        for (uint32 i = 0; i < numSteps; ++i)
        {
            current += stepVector;

            AZ::Vector3 pos = GetSphericalPos(current.GetX(), -current.GetY());
            pos *= radius;
            pos = offset.TransformPoint(pos);

            drawData->DrawLine(lastPos, pos, color);
            lastPos = pos;
        }

        drawData->Unlock();
    }

    void ConstraintTransformRotationAngles::DebugDraw(ActorInstance* actorInstance, const AZ::Transform& offset, const AZ::Color& color, float radius) const
    {
        const uint32 numSegments = 64;
        const AZ::Vector2 minValues = GetMinRotationAnglesRadians();
        const AZ::Vector2 maxValues = GetMaxRotationAnglesRadians();
        const float minX = minValues.GetX();
        const float maxX = maxValues.GetX();
        const float minY = minValues.GetY();
        const float maxY = maxValues.GetY();
        DrawSphericalLine(actorInstance, AZ::Vector2(minX, minY), AZ::Vector2(maxX, minY), numSegments, color, radius, offset);
        DrawSphericalLine(actorInstance, AZ::Vector2(minX, maxY), AZ::Vector2(maxX, maxY), numSegments, color, radius, offset);
        DrawSphericalLine(actorInstance, AZ::Vector2(minX, minY), AZ::Vector2(minX, maxY), numSegments, color, radius, offset);
        DrawSphericalLine(actorInstance, AZ::Vector2(maxX, minY), AZ::Vector2(maxX, maxY), numSegments, color, radius, offset);
    }

    void ConstraintTransformRotationAngles::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<ConstraintTransformRotationAngles>()
            ->Version(1);

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Enum<EAxis>("", "")
            ->Value("X Axis", AXIS_X)
            ->Value("Y Axis", AXIS_Y)
            ->Value("Z Axis", AXIS_Z);
    }
} // namespace EMotionFX
