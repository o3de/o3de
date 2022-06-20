/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RotationManipulators.h"

#include <AzToolsFramework/Manipulators/ManipulatorView.h>

namespace AzToolsFramework
{
    RotationManipulators::RotationManipulators(const AZ::Transform& worldFromLocal)
    {
        for (size_t manipulatorIndex = 0; manipulatorIndex < m_localAngularManipulators.size(); ++manipulatorIndex)
        {
            m_localAngularManipulators[manipulatorIndex] = AngularManipulator::MakeShared(worldFromLocal);
        }

        m_viewAngularManipulator = AngularManipulator::MakeShared(worldFromLocal);

        m_manipulatorSpaceWithLocalTransform.SetSpace(worldFromLocal);
    }

    void RotationManipulators::InstallLeftMouseDownCallback(const AngularManipulator::MouseActionCallback& onMouseDownCallback)
    {
        for (AZStd::shared_ptr<AngularManipulator>& manipulator : m_localAngularManipulators)
        {
            manipulator->InstallLeftMouseDownCallback(onMouseDownCallback);
        }

        m_viewAngularManipulator->InstallLeftMouseDownCallback(onMouseDownCallback);
    }

    void RotationManipulators::InstallMouseMoveCallback(const AngularManipulator::MouseActionCallback& onMouseMoveCallback)
    {
        for (AZStd::shared_ptr<AngularManipulator>& manipulator : m_localAngularManipulators)
        {
            manipulator->InstallMouseMoveCallback(onMouseMoveCallback);
        }

        m_viewAngularManipulator->InstallMouseMoveCallback(onMouseMoveCallback);
    }

    void RotationManipulators::InstallLeftMouseUpCallback(const AngularManipulator::MouseActionCallback& onMouseUpCallback)
    {
        for (AZStd::shared_ptr<AngularManipulator>& manipulator : m_localAngularManipulators)
        {
            manipulator->InstallLeftMouseUpCallback(onMouseUpCallback);
        }

        m_viewAngularManipulator->InstallLeftMouseUpCallback(onMouseUpCallback);
    }

    void RotationManipulators::SetLocalTransformImpl(const AZ::Transform& localTransform)
    {
        for (AZStd::shared_ptr<AngularManipulator>& manipulator : m_localAngularManipulators)
        {
            manipulator->SetLocalTransform(localTransform);
        }

        m_viewAngularManipulator->SetLocalTransform(localTransform);
    }

    void RotationManipulators::SetLocalPositionImpl(const AZ::Vector3& localPosition)
    {
        for (AZStd::shared_ptr<AngularManipulator>& manipulator : m_localAngularManipulators)
        {
            manipulator->SetLocalPosition(localPosition);
        }

        m_viewAngularManipulator->SetLocalPosition(localPosition);
    }

    void RotationManipulators::SetLocalOrientationImpl(const AZ::Quaternion& localOrientation)
    {
        for (AZStd::shared_ptr<AngularManipulator>& manipulator : m_localAngularManipulators)
        {
            manipulator->SetLocalOrientation(localOrientation);
        }

        m_viewAngularManipulator->SetLocalOrientation(localOrientation);
    }

    void RotationManipulators::RefreshView(const AZ::Vector3& worldViewPosition)
    {
        if (!PerformingActionViewAxis())
        {
            SetViewAxis(CalculateViewDirection(*this, worldViewPosition));
        }
    }

    void RotationManipulators::SetSpaceImpl(const AZ::Transform& worldFromLocal)
    {
        for (AZStd::shared_ptr<AngularManipulator>& manipulator : m_localAngularManipulators)
        {
            manipulator->SetSpace(worldFromLocal);
        }

        m_viewAngularManipulator->SetSpace(worldFromLocal);
    }

    void RotationManipulators::SetLocalAxes(const AZ::Vector3& axis1, const AZ::Vector3& axis2, const AZ::Vector3& axis3)
    {
        const AZ::Vector3 axes[] = { axis1, axis2, axis3 };

        for (size_t manipulatorIndex = 0; manipulatorIndex < m_localAngularManipulators.size(); ++manipulatorIndex)
        {
            m_localAngularManipulators[manipulatorIndex]->SetAxis(axes[manipulatorIndex]);
        }
    }

    void RotationManipulators::SetViewAxis(const AZ::Vector3& axis)
    {
        m_viewAngularManipulator->SetAxis(axis);

        if (auto circleView = azrtti_cast<ManipulatorViewCircle*>(m_viewAngularManipulator->GetView()))
        {
            circleView->m_axis = axis;
        }
    }

    void RotationManipulators::ConfigureView(
        const float radius, const AZ::Color& axis1Color, const AZ::Color& axis2Color, const AZ::Color& axis3Color)
    {
        const AZ::Color colors[] = { axis1Color, axis2Color, axis3Color };

        for (size_t manipulatorIndex = 0; manipulatorIndex < m_localAngularManipulators.size(); ++manipulatorIndex)
        {
            m_localAngularManipulators[manipulatorIndex]->SetView(CreateManipulatorViewCircle(
                *m_localAngularManipulators[manipulatorIndex], colors[manipulatorIndex], radius, m_circleBoundWidth, DrawHalfDottedCircle));
        }

        const float viewAlignedScale = 1.12f;
        m_viewAngularManipulator->SetView(CreateManipulatorViewCircle(
            *m_viewAngularManipulator, AZ::Color(1.0f, 1.0f, 1.0f, 1.0f), radius * viewAlignedScale, m_circleBoundWidth,
            DrawFullCircle));
    }

    bool RotationManipulators::PerformingActionViewAxis() const
    {
        return m_viewAngularManipulator->PerformingAction();
    }

    void RotationManipulators::ProcessManipulators(const AZStd::function<void(BaseManipulator*)>& manipulatorFn)
    {
        for (AZStd::shared_ptr<AngularManipulator>& manipulator : m_localAngularManipulators)
        {
            manipulatorFn(manipulator.get());
        }

        manipulatorFn(m_viewAngularManipulator.get());
    }

    void RotationManipulators::SetCircleBoundWidth(const float circleBoundWidth)
    {
        m_circleBoundWidth = circleBoundWidth;
    }
} // namespace AzToolsFramework
