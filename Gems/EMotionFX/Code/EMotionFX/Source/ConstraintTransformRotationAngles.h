/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include the required files
#include "EMotionFXConfig.h"
#include "ConstraintTransform.h"
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Color.h>
#include <Source/Integration/System/SystemCommon.h>

namespace AZ
{
    class ReflectContext;
    class Transform;
}

namespace EMotionFX
{
    class ActorInstance;

    /**
     * The rotation angle constraint.
     * This constraint works on a transform and limits the rotation to be within a given range defined by minimum and maximum swing and twist angles in degrees.
     */
    class EMFX_API ConstraintTransformRotationAngles
        : public ConstraintTransform
    {
    public:
        AZ_RTTI(ConstraintTransformRotationAngles, "{A57FB6A9-A95F-4ED8-900D-4676243AF8FC}", ConstraintTransform)
        AZ_CLASS_ALLOCATOR_DECL

        enum
        {
            TYPE_ID = 0x00000001
        };

        enum EAxis : int
        {
            AXIS_X = 0,
            AXIS_Y = 1,
            AXIS_Z = 2
        };

        ConstraintTransformRotationAngles();
        ~ConstraintTransformRotationAngles() override = default;

        uint32 GetType() const override;
        const char* GetTypeString() const override;
        void Execute() override;

        void DebugDraw(ActorInstance* actorInstance, const AZ::Transform& offset, const AZ::Color& color, float radius) const;

        void SetMinRotationAngles(const AZ::Vector2& minSwingDegrees);
        void SetMaxRotationAngles(const AZ::Vector2& maxSwingDegrees);
        void SetMinTwistAngle(float minAngleDegrees);
        void SetMaxTwistAngle(float maxAngleDegrees);
        void SetTwistAxis(EAxis axisIndex);

        AZ::Vector2 GetMinRotationAnglesDegrees() const;        // All returned angles are in degrees.
        AZ::Vector2 GetMaxRotationAnglesDegrees() const;
        AZ::Vector2 GetMinRotationAnglesRadians() const;        // All returned angles are in radians.
        AZ::Vector2 GetMaxRotationAnglesRadians() const;
        float GetMinTwistAngle() const;
        float GetMaxTwistAngle() const;
        EAxis GetTwistAxis() const;

        static void Reflect(AZ::ReflectContext* context);

    protected:
        AZ::Vector2     m_minRotationAngles;         ///< The minimum rotation angles, actually the precalculated sin(halfAngleRadians).
        AZ::Vector2     m_maxRotationAngles;         ///< The maximum rotation angles, actually the precalculated sin(halfAngleRadians).
        float           m_minTwist;                  ///< The minimum twist angle, actually the precalculated sin(halfAngleRadians).
        float           m_maxTwist;                  ///< The maximum twist angle, actually the precalculated sin(halfAngleRadians).
        EAxis           m_twistAxis;                 ///< The twist axis index, which has to be either 0, 1 or 2 (default=AXIS_X, which equals 0).

        void DrawSphericalLine(ActorInstance* actorInstance, const AZ::Vector2& start, const AZ::Vector2& end, uint32 numSteps, const AZ::Color& color, float radius, const AZ::Transform& offset) const;
        AZ::Vector3 GetSphericalPos(float x, float y) const;
    };
} // namespace EMotionFX


namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(EMotionFX::ConstraintTransformRotationAngles::EAxis, "{E6426BCD-9ADF-4211-87F8-F647901F4D0E}");
} // namespace AZ
