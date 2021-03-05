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

#include <AzCore/Memory/SystemAllocator.h>
#include <AzToolsFramework/Manipulators/AngularManipulator.h>
#include <AzToolsFramework/API/EditorCameraBus.h>

namespace AzToolsFramework
{
    /// RotationManipulators is an aggregation of 3 angular manipulators who share the same origin
    /// in addition to a view aligned angular manipulator (facing the camera).
    class RotationManipulators
        : public Manipulators
    {
    public:
        AZ_RTTI(RotationManipulators, "{5D1F1D47-1D5B-4E42-B47E-23F108F8BF7D}")
        AZ_CLASS_ALLOCATOR(RotationManipulators, AZ::SystemAllocator, 0)

        explicit RotationManipulators(const AZ::Transform& worldFromLocal);
        ~RotationManipulators() = default;

        void InstallLeftMouseDownCallback(const AngularManipulator::MouseActionCallback& onMouseDownCallback);
        void InstallLeftMouseUpCallback(const AngularManipulator::MouseActionCallback& onMouseUpCallback);
        void InstallMouseMoveCallback(const AngularManipulator::MouseActionCallback& onMouseMoveCallback);

        void SetSpace(const AZ::Transform& worldFromLocal) override;
        void SetLocalTransform(const AZ::Transform& localTransform) override;
        void SetLocalPosition(const AZ::Vector3& localPosition) override;
        void SetLocalOrientation(const AZ::Quaternion& localOrientation) override;
        void RefreshView(const AZ::Vector3& worldViewPosition) override;

        void SetLocalAxes(
            const AZ::Vector3& axis1, const AZ::Vector3& axis2, const AZ::Vector3& axis3);
        void SetViewAxis(const AZ::Vector3& axis);

        void ConfigureView(
            float radius, const AZ::Color& axis1Color, const AZ::Color& axis2Color, const AZ::Color& axis3Color);

        bool PerformingActionViewAxis() const;

    private:
        AZ_DISABLE_COPY_MOVE(RotationManipulators)

        void ProcessManipulators(const AZStd::function<void(BaseManipulator*)>&) override;

        AZStd::array<AZStd::shared_ptr<AngularManipulator>, 3> m_localAngularManipulators;
        AZStd::shared_ptr<AngularManipulator> m_viewAngularManipulator;
    };
} // namespace AzToolsFramework