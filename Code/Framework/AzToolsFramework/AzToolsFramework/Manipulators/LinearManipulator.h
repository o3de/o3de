/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "BaseManipulator.h"

#include <AzCore/Memory/SystemAllocator.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>
#include <AzToolsFramework/Viewport/ViewportTypes.h>

namespace AzToolsFramework
{
    struct GridSnapParameters;

    //! LinearManipulator serves as a visual tool for users to modify values
    //! in one dimension on an axis defined in 3D space.
    class LinearManipulator
        : public BaseManipulator
        , public ManipulatorSpaceWithLocalTransform
    {
        //! Private constructor.
        explicit LinearManipulator(const AZ::Transform& worldFromLocal);

    public:
        AZ_RTTI(LinearManipulator, "{4AA805DA-7D3C-4AFA-8110-EECF32B8F530}", BaseManipulator)
        AZ_CLASS_ALLOCATOR(LinearManipulator, AZ::SystemAllocator)

        LinearManipulator() = delete;
        LinearManipulator(const LinearManipulator&) = delete;
        LinearManipulator& operator=(const LinearManipulator&) = delete;

        ~LinearManipulator() = default;

        //! A Manipulator must only be created and managed through a shared_ptr.
        //! @note worldFromLocal should not contain scale.
        static AZStd::shared_ptr<LinearManipulator> MakeShared(const AZ::Transform& worldFromLocal);

        //! Unchanging data set once for the linear manipulator.
        struct Fixed
        {
            AZ::Vector3 m_axis = AZ::Vector3::CreateAxisX(); //!< The axis the manipulator will move along.
        };

        //! Data passed between the initial press and first movement of the linear manipulator.
        struct StartTransition
        {
            //! The normal in local space of the manipulator when the mouse down event happens.
            AZ::Vector3 m_localNormal;
            //! Used to scale movement based on camera distance if we want screen space instead
            //! of world space displacement.
            float m_screenToWorldScale;
        };

        //! The state of the manipulator at the start of an interaction.
        struct Start
        {
            AZ::Vector3 m_localPosition; //!< The current position of the manipulator in local space.
            AZ::Vector3 m_localScale; //!< The current scale of the manipulator in local space.
            AZ::Vector3 m_localHitPosition; //!< The intersection point in local space between the ray and the manipulator when the mouse
                                            //!< down event happens.
            AZ::Vector3 m_localAxis; //!< The axis in the local space of the manipulator itself.
            float m_sign; //!< Used to determine which side of the axis we clicked on in case it's flipped to face the camera.
            AzFramework::ScreenPoint m_screenPosition; //!< The initial position in screen space of the manipulator.
        };

        //! The state of the manipulator during an interaction.
        struct Current
        {
            AZ::Vector3 m_localPositionOffset; //!< The current offset of the manipulator from its starting position in local space.
            AZ::Vector3 m_localScaleOffset; //!< The current offset of the manipulator from its starting scale in local space.
            AzFramework::ScreenPoint m_screenPosition; //!< The current position in screen space of the manipulator.
        };

        //! Mouse action data used by MouseActionCallback (wraps Fixed, Start and Current manipulator state).
        struct Action
        {
            Fixed m_fixed;
            Start m_start;
            Current m_current;
            ViewportInteraction::KeyboardModifiers m_modifiers;
            int m_viewportId; //!< The id of the viewport this manipulator is being used in.
            AZ::Vector3 LocalScale() const
            {
                return m_start.m_localScale + m_current.m_localScaleOffset;
            }
            AZ::Vector3 LocalScaleOffset() const
            {
                return m_current.m_localScaleOffset;
            }
            AZ::Vector3 LocalPosition() const
            {
                return m_start.m_localPosition + m_current.m_localPositionOffset;
            }
            AZ::Vector3 LocalPositionOffset() const
            {
                return m_current.m_localPositionOffset;
            }
            AZ::Vector2 ScreenOffset() const
            {
                return AzFramework::Vector2FromScreenVector(m_current.m_screenPosition - m_start.m_screenPosition);
            }
        };

        //! This is the function signature of callbacks that will be invoked whenever a manipulator
        //! is clicked on or dragged.
        using MouseActionCallback = AZStd::function<void(const Action&)>;
        //! Tuple of StartTransition (initial mouse down to mouse move) and Start state.
        using Starter = AZStd::tuple<StartTransition, Start>;

        void InstallLeftMouseDownCallback(const MouseActionCallback& onMouseDownCallback);
        void InstallMouseMoveCallback(const MouseActionCallback& onMouseMoveCallback);
        void InstallLeftMouseUpCallback(const MouseActionCallback& onMouseUpCallback);

        void Draw(
            const ManipulatorManagerState& managerState,
            AzFramework::DebugDisplayRequests& debugDisplay,
            const AzFramework::CameraState& cameraState,
            const ViewportInteraction::MouseInteraction& mouseInteraction) override;

        void SetAxis(const AZ::Vector3& axis);
        const AZ::Vector3& GetAxis() const
        {
            return m_fixed.m_axis;
        }

        template<typename Views>
        void SetViews(Views&& views)
        {
            m_manipulatorViews = AZStd::forward<Views>(views);
        }

        void UseVisualOrientationOverride(const bool useVisualOverride)
        {
            m_useVisualsOverride = useVisualOverride;
        }

        void SetVisualOrientationOverride(const AZ::Quaternion& visualOrientation)
        {
            m_visualOrientationOverride = visualOrientation;
        }

    private:
        void OnLeftMouseDownImpl(const ViewportInteraction::MouseInteraction& interaction, float rayIntersectionDistance) override;
        void OnLeftMouseUpImpl(const ViewportInteraction::MouseInteraction& interaction) override;
        void OnMouseMoveImpl(const ViewportInteraction::MouseInteraction& interaction) override;

        void InvalidateImpl() override;
        void SetBoundsDirtyImpl() override;

        bool m_useVisualsOverride = false; // Set this to true to use the Visual Quaternion Override (decoupled from logical axis).
        AZ::Quaternion m_visualOrientationOverride = AZ::Quaternion::CreateIdentity(); // Quaternion to use only for visuals.

        Fixed m_fixed;
        Starter m_starter;

        MouseActionCallback m_onLeftMouseDownCallback = nullptr;
        MouseActionCallback m_onLeftMouseUpCallback = nullptr;
        MouseActionCallback m_onMouseMoveCallback = nullptr;

        ManipulatorViews m_manipulatorViews; //!< Look of manipulator.
    };

    LinearManipulator::Starter CalculateLinearManipulationDataStart(
        const LinearManipulator::Fixed& fixed,
        const AZ::Transform& worldFromLocal,
        const AZ::Vector3& nonUniformScale,
        const AZ::Transform& localTransform,
        const ViewportInteraction::MouseInteraction& interaction,
        float intersectionDistance,
        const AzFramework::CameraState& cameraState);

    LinearManipulator::Action CalculateLinearManipulationDataAction(
        const LinearManipulator::Fixed& fixed,
        const LinearManipulator::Starter& starter,
        const AZ::Transform& worldFromLocal,
        const AZ::Vector3& nonUniformScale,
        const AZ::Transform& localTransform,
        const GridSnapParameters& gridSnapParams,
        const ViewportInteraction::MouseInteraction& interaction);
} // namespace AzToolsFramework
