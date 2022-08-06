/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AngularManipulatorCircleViewFeedback.h"

#include <AzCore/Console/IConsole.h>
#include <AzCore/Math/Plane.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/Manipulators/AngularManipulator.h>
#include <AzToolsFramework/Manipulators/ManipulatorDebug.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>

AZ_CVAR(
    AZ::Color,
    ed_angularManipulatorCircleFeedbackDisplayColor,
    AZ::Color::CreateFromRgba(255, 255, 0, 100),
    nullptr,
    AZ::ConsoleFunctorFlags::Null,
    "The color to use for the Angular Manipulator circle feedback display");

namespace AzToolsFramework
{
    void AngularManipulatorCircleViewFeedback::Display(
        const AngularManipulator* angularManipulator,
        AzFramework::DebugDisplayRequests& debugDisplay,
        const AzFramework::CameraState& cameraState)
    {
        const auto manipulatorState = angularManipulator->CalculateManipulatorState();

        const auto worldPosition = manipulatorState.m_worldFromLocal.TransformPoint(manipulatorState.m_localPosition);
        const auto localFromWorld = manipulatorState.m_worldFromLocal.GetInverse();
        const auto initialLocalHitPosition = localFromWorld.TransformPoint(m_mostRecentAction.m_start.m_worldHitPosition);
        const auto currentLocalHitPosition = localFromWorld.TransformPoint(m_mostRecentAction.m_current.m_worldHitPosition);

        const auto fixedAxis = m_mostRecentAction.m_fixed.m_axis;
        const auto plane = AZ::Plane::CreateFromNormalAndPoint(fixedAxis, localFromWorld.GetTranslation());
        const auto initialPointOnPlane = plane.GetProjected(initialLocalHitPosition);
        const auto currentPointOnPlane = plane.GetProjected(currentLocalHitPosition);

        const float viewScale = CalculateScreenToWorldMultiplier(worldPosition, cameraState);

        const AZ::Transform orientation =
            AZ::Transform::CreateFromQuaternion((QuaternionFromTransformNoScaling(manipulatorState.m_worldFromLocal) *
                                                 AZ::Quaternion::CreateShortestArc(fixedAxis, angularManipulator->GetAxis()))
                                                    .GetNormalized());

        // transform circle based on delta between default z up axis and other axes
        const AZ::Transform worldFromLocalWithOrientation =
            AZ::Transform::CreateTranslation(manipulatorState.m_worldFromLocal.GetTranslation()) * orientation;

        if (angularManipulator->PerformingAction())
        {
            if (ed_manipulatorDrawDebug)
            {
                DrawTransformAxes(
                    debugDisplay,
                    angularManipulator->GetSpace() *
                        AZ::Transform::CreateTranslation(manipulatorState.m_worldFromLocal.TransformPoint(initialPointOnPlane)));
            }

            const auto initialPointToCenter = (initialPointOnPlane - manipulatorState.m_localPosition).GetNormalized();
            const auto currentPointToCenter = (currentPointOnPlane - manipulatorState.m_localPosition).GetNormalized();

            debugDisplay.CullOn();
            debugDisplay.DepthTestOff();
            debugDisplay.PushMatrix(worldFromLocalWithOrientation);
            debugDisplay.SetColor(ed_angularManipulatorCircleFeedbackDisplayColor);

            const auto totalAngle = AZ::DegToRad(360.0f);
            const auto stepIncrement = totalAngle / 360.0f;

            const auto right = AZ::Quaternion::CreateFromAxisAngle(fixedAxis, AZ::DegToRad(90.0f)).TransformVector(initialPointToCenter);
            const auto angle = m_mostRecentAction.m_current.m_delta.GetAngle();
            const auto sign = Sign(currentPointToCenter.Dot(right));
            const auto rotationDirection = angle < AZ::Constants::Pi ? sign : -sign;

            const auto angleStr = AZStd::string::format("Angle %f", angle);
            debugDisplay.Draw2dTextLabel(100, 100, 1.0f, angleStr.c_str());

            for (auto step = 0.0f; step < angle; step += stepIncrement)
            {
                const auto first =
                    AZ::Quaternion::CreateFromAxisAngle(fixedAxis, step * rotationDirection).TransformVector(initialPointToCenter);
                const auto second = AZ::Quaternion::CreateFromAxisAngle(fixedAxis, (step - stepIncrement) * rotationDirection)
                                        .TransformVector(initialPointToCenter);

                debugDisplay.DrawTri(
                    manipulatorState.m_localPosition, manipulatorState.m_localPosition + first * 2.0f * viewScale,
                    manipulatorState.m_localPosition + second * 2.0f * viewScale);
            }

            debugDisplay.PopMatrix();
            debugDisplay.DepthTestOn();
            debugDisplay.CullOff();
        }
    }
} // namespace AzToolsFramework
