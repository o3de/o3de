/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "BaseManipulator.h"

#include <AzCore/Math/Spline.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/weak_ptr.h>
#include <AzToolsFramework/Viewport/ViewportTypes.h>

namespace AzToolsFramework
{
    class ManipulatorView;

    //! A manipulator to represent selection of a spline. Underlying spline data is
    //! used to test mouse picking ray against to preview closest point on spline.
    class SplineSelectionManipulator
        : public BaseManipulator
        , public ManipulatorSpace
    {
        //! Private constructor.
        SplineSelectionManipulator();

    public:
        AZ_RTTI(SplineSelectionManipulator, "{3E6B2206-E910-48C9-BDB6-F45B539C00F4}", BaseManipulator);
        AZ_CLASS_ALLOCATOR(SplineSelectionManipulator, AZ::SystemAllocator);

        SplineSelectionManipulator(const SplineSelectionManipulator&) = delete;
        SplineSelectionManipulator& operator=(const SplineSelectionManipulator&) = delete;

        ~SplineSelectionManipulator();

        //! A Manipulator must only be created and managed through a shared_ptr.
        static AZStd::shared_ptr<SplineSelectionManipulator> MakeShared();

        //! Mouse action data used by MouseActionCallback.
        struct Action
        {
            AZ::Vector3 m_localSplineHitPosition;
            AZ::SplineAddress m_splineAddress;
        };

        using MouseActionCallback = AZStd::function<void(const Action&)>;

        void InstallLeftMouseDownCallback(const MouseActionCallback& onMouseDownCallback);
        void InstallLeftMouseUpCallback(const MouseActionCallback& onMouseUpCallback);

        void Draw(
            const ManipulatorManagerState& managerState,
            AzFramework::DebugDisplayRequests& debugDisplay,
            const AzFramework::CameraState& cameraState,
            const ViewportInteraction::MouseInteraction& mouseInteraction) override;

        void SetSpline(AZStd::shared_ptr<const AZ::Spline> spline)
        {
            m_spline = AZStd::move(spline);
        }
        AZStd::weak_ptr<const AZ::Spline> GetSpline() const
        {
            return m_spline;
        }

        void SetView(AZStd::unique_ptr<ManipulatorView>&& view);

    private:
        void OnLeftMouseDownImpl(const ViewportInteraction::MouseInteraction& interaction, float rayIntersectionDistance) override;
        void OnLeftMouseUpImpl(const ViewportInteraction::MouseInteraction& interaction) override;

        void InvalidateImpl() override;
        void SetBoundsDirtyImpl() override;

        AZStd::weak_ptr<const AZ::Spline> m_spline;
        AZStd::unique_ptr<ManipulatorView> m_manipulatorView = nullptr; //!< Look of manipulator and bounds for interaction.
        MouseActionCallback m_onLeftMouseDownCallback = nullptr;
        MouseActionCallback m_onLeftMouseUpCallback = nullptr;
        ViewportInteraction::KeyboardModifiers
            m_keyboardModifiers; //!< What modifier keys are pressed when interacting with this manipulator.
    };

    SplineSelectionManipulator::Action CalculateManipulationDataAction(
        const AZ::Transform& worldFromLocal,
        const AZ::Vector3& rayOrigin,
        const AZ::Vector3& rayDirection,
        const AZStd::weak_ptr<const AZ::Spline>& spline);

} // namespace AzToolsFramework
