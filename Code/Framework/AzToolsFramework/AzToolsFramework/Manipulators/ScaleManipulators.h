/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzToolsFramework/Manipulators/LinearManipulator.h>

namespace AzToolsFramework
{
    //! ScaleManipulators is an aggregation of 3 linear manipulators for each basis axis who share
    //! the same transform, and a single linear manipulator at the center of the transform whose
    //! axis is world up (z).
    class ScaleManipulators : public Manipulators
    {
    public:
        AZ_RTTI(ScaleManipulators, "{C6350CE0-7B7A-46F8-B65F-D4A54DD9A7D9}")
        AZ_CLASS_ALLOCATOR(ScaleManipulators, AZ::SystemAllocator)

        explicit ScaleManipulators(const AZ::Transform& worldFromLocal);

        void InstallAxisLeftMouseDownCallback(const LinearManipulator::MouseActionCallback& onMouseDownCallback);
        void InstallAxisMouseMoveCallback(const LinearManipulator::MouseActionCallback& onMouseMoveCallback);
        void InstallAxisLeftMouseUpCallback(const LinearManipulator::MouseActionCallback& onMouseUpCallback);

        void InstallUniformLeftMouseDownCallback(const LinearManipulator::MouseActionCallback& onMouseDownCallback);
        void InstallUniformMouseMoveCallback(const LinearManipulator::MouseActionCallback& onMouseMoveCallback);
        void InstallUniformLeftMouseUpCallback(const LinearManipulator::MouseActionCallback& onMouseUpCallback);

        void SetSpaceImpl(const AZ::Transform& worldFromLocal) override;
        void SetLocalTransformImpl(const AZ::Transform& localTransform) override;
        void SetLocalPositionImpl(const AZ::Vector3& localPosition) override;
        void SetLocalOrientationImpl(const AZ::Quaternion& localOrientation) override;

        void SetAxes(const AZ::Vector3& axis1, const AZ::Vector3& axis2, const AZ::Vector3& axis3);

        void ConfigureView(float axisLength, const AZ::Color& axis1Color, const AZ::Color& axis2Color, const AZ::Color& axis3Color);

        //! Sets the bound width to use for the line/axis of a linear manipulator.
        void SetLineBoundWidth(float lineBoundWidth);

        void ProcessManipulators(const ManipulatorVisitCallback&) override;

    private:
        AZ_DISABLE_COPY_MOVE(ScaleManipulators)

        AZStd::array<AZStd::shared_ptr<LinearManipulator>, 3> m_axisScaleManipulators;
        AZStd::shared_ptr<LinearManipulator> m_uniformScaleManipulator;
        float m_lineBoundWidth = 0.01f; //!< The default line bound width for the linear manipulator axis.
    };
} // namespace AzToolsFramework
