/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AngularManipulatorCircleViewFeedback.h"

#include <AzCore/Console/IConsole.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/Plane.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/Manipulators/AngularManipulator.h>
#include <AzToolsFramework/Manipulators/ManipulatorDebug.h>
#include <AzToolsFramework/Manipulators/ManipulatorSnapping.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>
#include <AzToolsFramework/Maths/TransformUtils.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>

AZ_CVAR(
    AZ::Color,
    ed_angularManipulatorCircleFeedbackDisplayColor,
    AZ::Color::CreateFromRgba(255, 255, 0, 100),
    nullptr,
    AZ::ConsoleFunctorFlags::Null,
    "The color to use for the Angular Manipulator circle feedback display");

AZ_CVAR(
    AZ::Color,
    ed_angularManipulatorCircleFeedbackRadiusSegmentColor,
    AZ::Color::CreateFromRgba(255, 255, 255, 255),
    nullptr,
    AZ::ConsoleFunctorFlags::Null,
    "The color to use for the begin / end angle radius segments in the Angular Manipulator circle feedback display");

namespace AzToolsFramework
{
    void AngularManipulatorCircleViewFeedback::Display(
        const AngularManipulator* angularManipulator,
        AzFramework::DebugDisplayRequests& debugDisplay,
        const AzFramework::CameraState& cameraState)
    {
        if (!angularManipulator->PerformingAction())
        {
            return;
        }

        // note: we know at this point the view has to be a circle view due to the constraints we make about this type (see name)
        const auto* view = static_cast<const ManipulatorViewCircle*>(angularManipulator->GetView());

        const auto manipulatorState = angularManipulator->CalculateManipulatorState();
        const auto fixedAxis = m_mostRecentAction.m_fixed.m_axis;
        const auto localFromWorld = manipulatorState.m_worldFromLocal.GetInverse();
        const auto plane = AZ::Plane::CreateFromNormalAndPoint(fixedAxis, localFromWorld.GetTranslation());
        const auto initialLocalHitPosition = localFromWorld.TransformPoint(m_mostRecentAction.m_start.m_worldHitPosition);
        const auto initialPointOnPlane = plane.GetProjected(initialLocalHitPosition);

        if (ed_manipulatorDrawDebug)
        {
            DrawTransformAxes(
                debugDisplay,
                angularManipulator->GetSpace() *
                    AZ::Transform::CreateTranslation(manipulatorState.m_worldFromLocal.TransformPoint(initialPointOnPlane)));
        }

        const AZ::Transform orientation =
            AZ::Transform::CreateFromQuaternion((QuaternionFromTransformNoScaling(manipulatorState.m_worldFromLocal) *
                                                 AZ::Quaternion::CreateShortestArc(fixedAxis, angularManipulator->GetAxis()))
                                                    .GetNormalized());

        // transform circle based on delta between default z up axis and other axes
        const AZ::Transform worldFromLocalWithOrientation =
            AZ::Transform::CreateTranslation(manipulatorState.m_worldFromLocal.GetTranslation()) * orientation;

        debugDisplay.CullOn();
        debugDisplay.DepthTestOff();
        debugDisplay.PushMatrix(worldFromLocalWithOrientation);
        debugDisplay.SetColor(ed_angularManipulatorCircleFeedbackDisplayColor);

        constexpr auto totalAngle = AZ::DegToRad(360.0f);
        constexpr auto stepIncrement = totalAngle / 360.0f;
        const auto worldPosition = manipulatorState.m_worldFromLocal.TransformPoint(manipulatorState.m_localPosition);
        const float viewScale = CalculateScreenToWorldMultiplier(worldPosition, cameraState);
        const auto angle = m_mostRecentAction.m_current.m_deltaRadians;
        const auto initialPointToCenter = (initialPointOnPlane - manipulatorState.m_localPosition).GetNormalized();
        const auto angleSign = Sign(angle);
        const auto maxAngle =
            AZ::GetMin(AZ::GetAbs(angle), AZ::Constants::TwoPi);
        for (auto step = 0.0f; step < maxAngle; step += stepIncrement)
        {
            const auto first = AZ::Quaternion::CreateFromAxisAngle(fixedAxis, step * angleSign).TransformVector(initialPointToCenter);
            const auto second =
                AZ::Quaternion::CreateFromAxisAngle(fixedAxis, (step - stepIncrement) * angleSign).TransformVector(initialPointToCenter);
            debugDisplay.DrawTri(
                manipulatorState.m_localPosition,
                manipulatorState.m_localPosition + first * view->m_radius * viewScale,
                manipulatorState.m_localPosition + second * view->m_radius * viewScale);
        }

        debugDisplay.SetColor(ed_angularManipulatorCircleFeedbackRadiusSegmentColor);

        const auto drawRadiusSegment =
            [&debugDisplay, &fixedAxis, &initialPointToCenter, &manipulatorState, view, viewScale](float angle)
        {
            const auto direction = AZ::Quaternion::CreateFromAxisAngle(fixedAxis, angle).TransformVector(initialPointToCenter);
            debugDisplay.DrawLine(
                manipulatorState.m_localPosition, manipulatorState.m_localPosition + direction * view->m_radius * viewScale);
        };
        drawRadiusSegment(0.f);
        drawRadiusSegment(angle);

        debugDisplay.PopMatrix();
        debugDisplay.DepthTestOn();
        debugDisplay.CullOff();
    }
} // namespace AzToolsFramework
