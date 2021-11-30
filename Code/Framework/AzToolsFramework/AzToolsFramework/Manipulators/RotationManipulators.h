/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzToolsFramework/API/EditorCameraBus.h>
#include <AzToolsFramework/Manipulators/AngularManipulator.h>

namespace AzToolsFramework
{
    //! RotationManipulators is an aggregation of 3 angular manipulators who share the same origin
    //! in addition to a view aligned angular manipulator (facing the camera).
    class RotationManipulators : public Manipulators
    {
    public:
        AZ_RTTI(RotationManipulators, "{5D1F1D47-1D5B-4E42-B47E-23F108F8BF7D}")
        AZ_CLASS_ALLOCATOR(RotationManipulators, AZ::SystemAllocator, 0)

        explicit RotationManipulators(const AZ::Transform& worldFromLocal);
        ~RotationManipulators() = default;

        void InstallLeftMouseDownCallback(const AngularManipulator::MouseActionCallback& onMouseDownCallback);
        void InstallLeftMouseUpCallback(const AngularManipulator::MouseActionCallback& onMouseUpCallback);
        void InstallMouseMoveCallback(const AngularManipulator::MouseActionCallback& onMouseMoveCallback);

        void SetSpaceImpl(const AZ::Transform& worldFromLocal) override;
        void SetLocalTransformImpl(const AZ::Transform& localTransform) override;
        void SetLocalPositionImpl(const AZ::Vector3& localPosition) override;
        void SetLocalOrientationImpl(const AZ::Quaternion& localOrientation) override;
        void RefreshView(const AZ::Vector3& worldViewPosition) override;

        void SetLocalAxes(const AZ::Vector3& axis1, const AZ::Vector3& axis2, const AZ::Vector3& axis3);
        void SetViewAxis(const AZ::Vector3& axis);

        void ConfigureView(float radius, const AZ::Color& axis1Color, const AZ::Color& axis2Color, const AZ::Color& axis3Color);

        bool PerformingActionViewAxis() const;

        //! Sets the bound width to use for the circle (torus) of an angular manipulator.
        void SetCircleBoundWidth(float circleBoundWidth);

    private:
        AZ_DISABLE_COPY_MOVE(RotationManipulators)

        void ProcessManipulators(const AZStd::function<void(BaseManipulator*)>&) override;

        AZStd::array<AZStd::shared_ptr<AngularManipulator>, 3> m_localAngularManipulators;
        AZStd::shared_ptr<AngularManipulator> m_viewAngularManipulator;
        float m_circleBoundWidth = 0.1f;  //!< The default circle bound width for the angular manipulator torus.
    };
} // namespace AzToolsFramework
