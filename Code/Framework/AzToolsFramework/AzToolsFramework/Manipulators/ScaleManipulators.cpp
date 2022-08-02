/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ScaleManipulators.h"

#include <AzToolsFramework/Maths/TransformUtils.h>
#include <AzToolsFramework/Viewport/ViewportSettings.h>

namespace AzToolsFramework
{
    ScaleManipulators::ScaleManipulators(const AZ::Transform& worldFromLocal)
    {
        for (size_t manipulatorIndex = 0; manipulatorIndex < m_axisScaleManipulators.size(); ++manipulatorIndex)
        {
            m_axisScaleManipulators[manipulatorIndex] = LinearManipulator::MakeShared(worldFromLocal);
        }

        m_uniformScaleManipulator = LinearManipulator::MakeShared(worldFromLocal);

        m_manipulatorSpaceWithLocalTransform.SetSpace(worldFromLocal);
    }

    void ScaleManipulators::InstallAxisLeftMouseDownCallback(const LinearManipulator::MouseActionCallback& onMouseDownCallback)
    {
        for (AZStd::shared_ptr<LinearManipulator>& manipulator : m_axisScaleManipulators)
        {
            manipulator->InstallLeftMouseDownCallback(onMouseDownCallback);
        }
    }

    void ScaleManipulators::InstallAxisMouseMoveCallback(const LinearManipulator::MouseActionCallback& onMouseMoveCallback)
    {
        for (AZStd::shared_ptr<LinearManipulator>& manipulator : m_axisScaleManipulators)
        {
            manipulator->InstallMouseMoveCallback(onMouseMoveCallback);
        }
    }

    void ScaleManipulators::InstallAxisLeftMouseUpCallback(const LinearManipulator::MouseActionCallback& onMouseUpCallback)
    {
        for (AZStd::shared_ptr<LinearManipulator>& manipulator : m_axisScaleManipulators)
        {
            manipulator->InstallLeftMouseUpCallback(onMouseUpCallback);
        }
    }

    void ScaleManipulators::InstallUniformLeftMouseDownCallback(const LinearManipulator::MouseActionCallback& onMouseDownCallback)
    {
        m_uniformScaleManipulator->InstallLeftMouseDownCallback(onMouseDownCallback);
    }

    void ScaleManipulators::InstallUniformMouseMoveCallback(const LinearManipulator::MouseActionCallback& onMouseMoveCallback)
    {
        m_uniformScaleManipulator->InstallMouseMoveCallback(onMouseMoveCallback);
    }

    void ScaleManipulators::InstallUniformLeftMouseUpCallback(const LinearManipulator::MouseActionCallback& onMouseUpCallback)
    {
        m_uniformScaleManipulator->InstallLeftMouseUpCallback(onMouseUpCallback);
    }

    void ScaleManipulators::SetLocalTransformImpl(const AZ::Transform& localTransform)
    {
        for (AZStd::shared_ptr<LinearManipulator>& manipulator : m_axisScaleManipulators)
        {
            manipulator->SetLocalTransform(localTransform);
        }

        m_uniformScaleManipulator->SetVisualOrientationOverride(QuaternionFromTransformNoScaling(localTransform));
        m_uniformScaleManipulator->SetLocalPosition(localTransform.GetTranslation());
        m_uniformScaleManipulator->SetLocalOrientation(AZ::Quaternion::CreateIdentity());
    }

    void ScaleManipulators::SetLocalPositionImpl(const AZ::Vector3& localPosition)
    {
        for (AZStd::shared_ptr<LinearManipulator>& manipulator : m_axisScaleManipulators)
        {
            manipulator->SetLocalPosition(localPosition);
        }

        m_uniformScaleManipulator->SetLocalPosition(localPosition);
    }

    void ScaleManipulators::SetLocalOrientationImpl(const AZ::Quaternion& localOrientation)
    {
        for (AZStd::shared_ptr<LinearManipulator>& manipulator : m_axisScaleManipulators)
        {
            manipulator->SetLocalOrientation(localOrientation);
        }
    }

    void ScaleManipulators::SetSpaceImpl(const AZ::Transform& worldFromLocal)
    {
        for (AZStd::shared_ptr<LinearManipulator>& manipulator : m_axisScaleManipulators)
        {
            manipulator->SetSpace(worldFromLocal);
        }

        m_uniformScaleManipulator->SetSpace(worldFromLocal);
    }

    void ScaleManipulators::SetAxes(const AZ::Vector3& axis1, const AZ::Vector3& axis2, const AZ::Vector3& axis3)
    {
        AZ::Vector3 axes[] = { axis1, axis2, axis3 };

        for (size_t manipulatorIndex = 0; manipulatorIndex < m_axisScaleManipulators.size(); ++manipulatorIndex)
        {
            m_axisScaleManipulators[manipulatorIndex]->SetAxis(axes[manipulatorIndex]);
        }

        // uniform scale manipulator uses Z axis for scaling (always in world space)
        m_uniformScaleManipulator->SetAxis(AZ::Vector3::CreateAxisZ());
        m_uniformScaleManipulator->UseVisualOrientationOverride(true);
    }

    void ScaleManipulators::ConfigureView(
        const float axisLength, const AZ::Color& axis1Color, const AZ::Color& axis2Color, const AZ::Color& axis3Color)
    {
        const float boxHalfExtent = ScaleManipulatorBoxHalfExtent();
        const AZ::Color colors[] = { axis1Color, axis2Color, axis3Color };

        for (size_t manipulatorIndex = 0; manipulatorIndex < m_axisScaleManipulators.size(); ++manipulatorIndex)
        {
            const auto lineLength = axisLength - (2.0f * boxHalfExtent);

            ManipulatorViews views;
            views.emplace_back(CreateManipulatorViewLine(
                *m_axisScaleManipulators[manipulatorIndex], colors[manipulatorIndex], axisLength, m_lineBoundWidth));
            views.emplace_back(CreateManipulatorViewBox(
                AZ::Transform::CreateIdentity(), colors[manipulatorIndex],
                m_axisScaleManipulators[manipulatorIndex]->GetAxis() * (lineLength + boxHalfExtent), AZ::Vector3(boxHalfExtent)));
            m_axisScaleManipulators[manipulatorIndex]->SetViews(AZStd::move(views));
        }

        ManipulatorViews views;
        views.emplace_back(CreateManipulatorViewBox(
            AZ::Transform::CreateIdentity(), AZ::Color::CreateOne(), AZ::Vector3::CreateZero(), AZ::Vector3(boxHalfExtent)));
        m_uniformScaleManipulator->SetViews(AZStd::move(views));
    }

    void ScaleManipulators::ProcessManipulators(const AZStd::function<void(BaseManipulator*)>& manipulatorFn)
    {
        for (AZStd::shared_ptr<LinearManipulator>& manipulator : m_axisScaleManipulators)
        {
            manipulatorFn(manipulator.get());
        }

        manipulatorFn(m_uniformScaleManipulator.get());
    }

    void ScaleManipulators::SetLineBoundWidth(const float lineBoundWidth)
    {
        m_lineBoundWidth = lineBoundWidth;
    }
} // namespace AzToolsFramework
