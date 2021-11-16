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
            MODE_ONE = 1,
            MODE_TWO = 2,
            MODE_THREE = 3,
            MODE_FOUR = 4
        };

        void Update(const ActorInstance* actorInstance,
            const FeatureTrajectory* trajectoryFeature,
            const TrajectoryHistory& trajectoryHistory,
            EMode mode,
            const AZ::Vector3& targetPos,
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

        AZStd::vector<ControlPoint> m_pastControlPoints;
        AZStd::vector<ControlPoint> m_futureControlPoints;
    };
} // namespace EMotionFX::MotionMatching
