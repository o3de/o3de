/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Color.h>

#include <AzFramework/Entity/EntityDebugDisplayBus.h>

#include <EMotionFX/Source/Pose.h>

#include <TrajectoryHistory.h>

namespace EMotionFX::MotionMatching
{
    class FeatureTrajectory;

    //! Builds the input trajectory query data for the motion matching algorithm.
    //! Reads the number of past and future samples and the time ranges from the trajectory feature,
    //! constructs the future trajectory based on the target and the past trajectory based on the trajectory history.
    class EMFX_API TrajectoryQuery
    {
    public:
        struct ControlPoint
        {
            AZ::Vector3 m_position;
            AZ::Vector3 m_facingDirection;
        };

        enum EMode : AZ::u8
        {
            MODE_TARGETDRIVEN = 0,
            MODE_AUTOMATIC = 1
        };

        void Update(const ActorInstance& actorInstance,
            const FeatureTrajectory* trajectoryFeature,
            const TrajectoryHistory& trajectoryHistory,
            EMode mode,
            const AZ::Vector3& targetPos,
            const AZ::Vector3& targetFacingDir,
            bool useTargetFacingDir,
            float timeDelta,
            float pathRadius,
            float pathSpeed);

        void DebugDraw(AzFramework::DebugDisplayRequests& debugDisplay, const AZ::Color& color) const;

        const AZStd::vector<ControlPoint>& GetPastControlPoints() const { return m_pastControlPoints; }
        const AZStd::vector<ControlPoint>& GetFutureControlPoints() const { return m_futureControlPoints; }

    private:
        static void DebugDrawControlPoints(AzFramework::DebugDisplayRequests& debugDisplay,
            const AZStd::vector<ControlPoint>& controlPoints,
            const AZ::Color& color);

        void PredictFutureTrajectory(const ActorInstance& actorInstance,
            const FeatureTrajectory* trajectoryFeature,
            const AZ::Vector3& targetPos,
            const AZ::Vector3& targetFacingDir,
            bool useTargetFacingDir);

        AZStd::vector<ControlPoint> m_pastControlPoints;
        AZStd::vector<ControlPoint> m_futureControlPoints;

        float m_positionBias = 2.0f; //< Indicates how fast the curve will bend towards the target.
        float m_rotationBias = 3.0f; //< Indicates how fast the facing direction matches the target facing direction.
        float m_deadZone = 0.2f; //< Similarly to a joystick deadzone, this represents the area around the character that does not respond to movement.

        float m_automaticModePhase = 0.0f; //< Current phase for the automatic demo mode. Not needed by the target-driven mode.
    };
} // namespace EMotionFX::MotionMatching
