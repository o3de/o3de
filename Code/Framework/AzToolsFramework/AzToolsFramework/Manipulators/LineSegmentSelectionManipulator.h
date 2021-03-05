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
#include <AzToolsFramework/Manipulators/BaseManipulator.h>
#include <AzToolsFramework/Viewport/ViewportTypes.h>

namespace AzToolsFramework
{
    class ManipulatorView;

    /// A manipulator to expose where on a line a user is moving their mouse.
    class LineSegmentSelectionManipulator
        : public BaseManipulator
    {
        /// Private constructor.
        LineSegmentSelectionManipulator();

    public:
        AZ_RTTI(LineSegmentSelectionManipulator, "{8BA5A9E4-72B4-4B48-BD54-D9DB58EDDA72}", BaseManipulator);
        AZ_CLASS_ALLOCATOR(LineSegmentSelectionManipulator, AZ::SystemAllocator, 0);

        LineSegmentSelectionManipulator(const LineSegmentSelectionManipulator&) = delete;
        LineSegmentSelectionManipulator& operator=(const LineSegmentSelectionManipulator&) = delete;

        ~LineSegmentSelectionManipulator();

        /// A Manipulator must only be created and managed through a shared_ptr.
        static AZStd::shared_ptr<LineSegmentSelectionManipulator> MakeShared();

        /// Mouse action data used by MouseActionCallback.
        struct Action
        {
            AZ::Vector3 m_localLineHitPosition;
        };

        using MouseActionCallback = AZStd::function<void(const Action&)>;

        void InstallLeftMouseDownCallback(const MouseActionCallback& onMouseDownCallback);
        void InstallLeftMouseUpCallback(const MouseActionCallback& onMouseUpCallback);

        void Draw(
            const ManipulatorManagerState& managerState,
            AzFramework::DebugDisplayRequests& debugDisplay,
            const AzFramework::CameraState& cameraState,
            const ViewportInteraction::MouseInteraction& mouseInteraction) override;

        void SetSpace(const AZ::Transform& worldFromLocal) { m_worldFromLocal = worldFromLocal; }
        void SetStart(const AZ::Vector3& startLocal) { m_localStart = startLocal; }
        void SetEnd(const AZ::Vector3& endLocal) { m_localEnd = endLocal; }
        const AZ::Vector3& GetStart() const { return m_localStart; }
        const AZ::Vector3& GetEnd() const { return m_localEnd; }

        void SetView(AZStd::unique_ptr<ManipulatorView>&& view);

    private:
        void OnLeftMouseDownImpl(
            const ViewportInteraction::MouseInteraction& interaction, float rayIntersectionDistance) override;
        void OnLeftMouseUpImpl(
            const ViewportInteraction::MouseInteraction& interaction) override;

        void InvalidateImpl() override;
        void SetBoundsDirtyImpl() override;

        AZ::Transform m_worldFromLocal = AZ::Transform::CreateIdentity(); ///< Space the manipulator is in (identity is world space).
        AZ::Vector3 m_localStart = AZ::Vector3::CreateZero();
        AZ::Vector3 m_localEnd = AZ::Vector3::CreateZero();

        MouseActionCallback m_onLeftMouseDownCallback = nullptr;
        MouseActionCallback m_onLeftMouseUpCallback = nullptr;

        ViewportInteraction::KeyboardModifiers m_keyboardModifiers; ///< What modifier keys are pressed when interacting with this manipulator.

        AZStd::unique_ptr<ManipulatorView> m_manipulatorView = nullptr; ///< Look of manipulator.
    };

    LineSegmentSelectionManipulator::Action CalculateManipulationDataAction(
        const AZ::Transform& worldFromLocal, const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDirection,
        float rayLength, const AZ::Vector3& localStart, const AZ::Vector3& localEnd);
} // namespace AzToolsFramework
