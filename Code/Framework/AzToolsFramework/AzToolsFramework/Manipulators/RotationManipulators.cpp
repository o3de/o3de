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

        m_space = worldFromLocal;
    }

    void RotationManipulators::InstallLeftMouseDownCallback(
        const AngularManipulator::MouseActionCallback& onMouseDownCallback)
    {
        for (AZStd::shared_ptr<AngularManipulator>& manipulator : m_localAngularManipulators)
        {
            manipulator->InstallLeftMouseDownCallback(onMouseDownCallback);
        }

        m_viewAngularManipulator->InstallLeftMouseDownCallback(onMouseDownCallback);
    }

    void RotationManipulators::InstallMouseMoveCallback(
        const AngularManipulator::MouseActionCallback& onMouseMoveCallback)
    {
        for (AZStd::shared_ptr<AngularManipulator>& manipulator : m_localAngularManipulators)
        {
            manipulator->InstallMouseMoveCallback(onMouseMoveCallback);
        }

        m_viewAngularManipulator->InstallMouseMoveCallback(onMouseMoveCallback);
    }

    void RotationManipulators::InstallLeftMouseUpCallback(
        const AngularManipulator::MouseActionCallback& onMouseUpCallback)
    {
        for (AZStd::shared_ptr<AngularManipulator>& manipulator : m_localAngularManipulators)
        {
            manipulator->InstallLeftMouseUpCallback(onMouseUpCallback);
        }

        m_viewAngularManipulator->InstallLeftMouseUpCallback(onMouseUpCallback);
    }

    void RotationManipulators::SetLocalTransform(const AZ::Transform& localTransform)
    {
        for (AZStd::shared_ptr<AngularManipulator>& manipulator : m_localAngularManipulators)
        {
            manipulator->SetLocalTransform(localTransform);
        }

        m_viewAngularManipulator->SetLocalTransform(localTransform);

        m_localTransform = localTransform;
    }

    void RotationManipulators::SetLocalPosition(const AZ::Vector3& localPosition)
    {
        for (AZStd::shared_ptr<AngularManipulator>& manipulator : m_localAngularManipulators)
        {
            manipulator->SetLocalPosition(localPosition);
        }

        m_viewAngularManipulator->SetLocalPosition(localPosition);

        m_localTransform.SetTranslation(localPosition);
    }

    void RotationManipulators::SetLocalOrientation(const AZ::Quaternion& localOrientation)
    {
        for (AZStd::shared_ptr<AngularManipulator>& manipulator : m_localAngularManipulators)
        {
            manipulator->SetLocalOrientation(localOrientation);
        }

        m_viewAngularManipulator->SetLocalOrientation(localOrientation);

        m_localTransform = AZ::Transform::CreateFromQuaternionAndTranslation(
            localOrientation, m_localTransform.GetTranslation());
    }

    void RotationManipulators::RefreshView(const AZ::Vector3& worldViewPosition)
    {
        if (!PerformingActionViewAxis())
        {
            SetViewAxis(CalculateViewDirection(*this, worldViewPosition));
        }
    }

    void RotationManipulators::SetSpace(const AZ::Transform& worldFromLocal)
    {
        for (AZStd::shared_ptr<AngularManipulator>& manipulator : m_localAngularManipulators)
        {
            manipulator->SetSpace(worldFromLocal);
        }

        m_viewAngularManipulator->SetSpace(worldFromLocal);

        m_space = worldFromLocal;
    }

    void RotationManipulators::SetLocalAxes(
        const AZ::Vector3& axis1, const AZ::Vector3& axis2, const AZ::Vector3& axis3)
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

        if (auto circleView = azrtti_cast<ManipulatorViewCircle*>(
            m_viewAngularManipulator->GetView()))
        {
            circleView->m_axis = axis;
        }
    }

    void RotationManipulators::ConfigureView(
        const float radius, const AZ::Color& axis1Color,
        const AZ::Color& axis2Color, const AZ::Color& axis3Color)
    {
        const AZ::Color colors[] = {
            axis1Color, axis2Color, axis3Color
        };

        for (size_t manipulatorIndex = 0; manipulatorIndex < m_localAngularManipulators.size(); ++manipulatorIndex)
        {
            m_localAngularManipulators[manipulatorIndex]->SetView(
                CreateManipulatorViewCircle(
                    *m_localAngularManipulators[manipulatorIndex], colors[manipulatorIndex],
                    radius, 0.05f, DrawHalfDottedCircle));
        }

        m_viewAngularManipulator->SetView(
            CreateManipulatorViewCircle(
                *m_viewAngularManipulator,
                AZ::Color(1.0f, 1.0f, 1.0f, 1.0f),
                radius + (radius * 0.12f), 0.05f, DrawFullCircle));
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
} // namespace AzToolsFramework