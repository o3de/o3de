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
#include <AzToolsFramework/Viewport/ViewportMessages.h>

namespace AzToolsFramework
{
    class ManipulatorView;
    struct GridSnapParameters;

    //! PlanarManipulator serves as a visual tool for users to modify values
    //! in two dimension in a plane defined two non-collinear axes in 3D space.
    class PlanarManipulator
        : public BaseManipulator
        , public ManipulatorSpaceWithLocalTransform
    {
        //! Private constructor.
        explicit PlanarManipulator(const AZ::Transform& worldFromLocal);

    public:
        AZ_RTTI(PlanarManipulator, "{2B1C2140-F3B1-4DB2-B066-156B67B57B97}", BaseManipulator)
        AZ_CLASS_ALLOCATOR(PlanarManipulator, AZ::SystemAllocator)

        PlanarManipulator() = delete;
        PlanarManipulator(const PlanarManipulator&) = delete;
        PlanarManipulator& operator=(const PlanarManipulator&) = delete;

        ~PlanarManipulator() = default;

        //! A Manipulator must only be created and managed through a shared_ptr.
        static AZStd::shared_ptr<PlanarManipulator> MakeShared(const AZ::Transform& worldFromLocal);

        //! Unchanging data set once for the planar manipulator.
        struct Fixed
        {
            AZ::Vector3 m_axis1 =
                AZ::Vector3::CreateAxisX(); //!< m_axis1 and m_axis2 have to be orthogonal, they together define a plane in 3d space.
            AZ::Vector3 m_axis2 = AZ::Vector3::CreateAxisY();
            AZ::Vector3 m_normal = AZ::Vector3::CreateAxisZ(); //!< m_normal is calculated automatically when setting the axes.
        };

        //! The state of the manipulator at the start of an interaction.
        struct Start
        {
            AZ::Vector3 m_localPosition; //!< The current position of the manipulator in local space.
            AZ::Vector3 m_localHitPosition; //!< The intersection point in local space between the ray and the manipulator when the mouse
                                            //!< down event happens.
        };

        //! The state of the manipulator during an interaction.
        struct Current
        {
            AZ::Vector3 m_localOffset; //!< The current position of the manipulator in local space.
        };

        //! Mouse action data used by MouseActionCallback (wraps Start and Current manipulator state).
        struct Action
        {
            Fixed m_fixed;
            Start m_start;
            Current m_current;
            ViewportInteraction::KeyboardModifiers m_modifiers;
            AZ::Vector3 LocalPosition() const
            {
                return m_start.m_localPosition + m_current.m_localOffset;
            }
            AZ::Vector3 LocalPositionOffset() const
            {
                return m_current.m_localOffset;
            }
        };

        //! This is the function signature of callbacks that will be invoked whenever a manipulator
        //! is being clicked on or dragged.
        using MouseActionCallback = AZStd::function<void(const Action&)>;

        void InstallLeftMouseDownCallback(const MouseActionCallback& onMouseDownCallback);
        void InstallMouseMoveCallback(const MouseActionCallback& onMouseMoveCallback);
        void InstallLeftMouseUpCallback(const MouseActionCallback& onMouseUpCallback);

        void Draw(
            const ManipulatorManagerState& managerState,
            AzFramework::DebugDisplayRequests& debugDisplay,
            const AzFramework::CameraState& cameraState,
            const ViewportInteraction::MouseInteraction& mouseInteraction) override;

        //! Ensure @param axis1 and @param axis2 are not collinear.
        void SetAxes(const AZ::Vector3& axis1, const AZ::Vector3& axis2);

        const AZ::Vector3& GetAxis1() const
        {
            return m_fixed.m_axis1;
        }
        const AZ::Vector3& GetAxis2() const
        {
            return m_fixed.m_axis2;
        }

        template<typename Views>
        void SetViews(Views&& views)
        {
            m_manipulatorViews = AZStd::forward<Views>(views);
        }

    private:
        void OnLeftMouseDownImpl(const ViewportInteraction::MouseInteraction& interaction, float rayIntersectionDistance) override;
        void OnLeftMouseUpImpl(const ViewportInteraction::MouseInteraction& interaction) override;
        void OnMouseMoveImpl(const ViewportInteraction::MouseInteraction& interaction) override;

        void InvalidateImpl() override;
        void SetBoundsDirtyImpl() override;

        //! Initial data recorded when a press first happens with a planar manipulator.
        struct StartInternal
        {
            AZ::Vector3 m_localPosition; //!< The starting position of the manipulator in local space.
            AZ::Vector3 m_localHitPosition; //!< The intersection point in world space between the ray and the manipulator when the mouse
                                            //!< down event happens.
        };

        Fixed m_fixed;
        StartInternal m_startInternal;

        MouseActionCallback m_onLeftMouseDownCallback = nullptr;
        MouseActionCallback m_onLeftMouseUpCallback = nullptr;
        MouseActionCallback m_onMouseMoveCallback = nullptr;

        ManipulatorViews m_manipulatorViews; //!< Look of manipulator.

        static StartInternal CalculateManipulationDataStart(
            const Fixed& fixed,
            const AZ::Transform& worldFromLocal,
            const AZ::Vector3& nonUniformScale,
            const AZ::Transform& localTransform,
            const ViewportInteraction::MouseInteraction& interaction,
            float intersectionDistance);

        static Action CalculateManipulationDataAction(
            const Fixed& fixed,
            const StartInternal& startInternal,
            const AZ::Transform& worldFromLocal,
            const AZ::Vector3& nonUniformScale,
            const AZ::Transform& localTransform,
            const GridSnapParameters& gridSnapParams,
            const ViewportInteraction::MouseInteraction& interaction);
    };
} // namespace AzToolsFramework
