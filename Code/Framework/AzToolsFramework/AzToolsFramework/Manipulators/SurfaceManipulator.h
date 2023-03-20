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
#include <AzFramework/Render/GeometryIntersectionStructures.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>

namespace AzToolsFramework
{
    class ManipulatorView;

    //! Surface manipulator will ensure the point(s) it controls snap precisely to the xy grid
    //! while also staying aligned exactly to the height of the terrain.
    class SurfaceManipulator
        : public BaseManipulator
        , public ManipulatorSpaceWithLocalPosition
    {
        //! Private constructor.
        explicit SurfaceManipulator(const AZ::Transform& worldFromLocal);

    public:
        AZ_RTTI(SurfaceManipulator, "{75B8EF42-A5F0-48EB-893E-84BED1BC8BAF}", BaseManipulator)
        AZ_CLASS_ALLOCATOR(SurfaceManipulator, AZ::SystemAllocator)

        SurfaceManipulator() = delete;
        SurfaceManipulator(const SurfaceManipulator&) = delete;
        SurfaceManipulator& operator=(const SurfaceManipulator&) = delete;

        ~SurfaceManipulator() = default;

        //! A Manipulator must only be created and managed through a shared_ptr.
        static AZStd::shared_ptr<SurfaceManipulator> MakeShared(const AZ::Transform& worldFromLocal);

        //! Callback function to determine which EntityIds to ignore when performing the ray intersection.
        using EntityIdsToIgnoreFn = AZStd::function<UniqueEntityIds(const ViewportInteraction::MouseInteraction&)>;

        //! The state of the manipulator at the start of an interaction.
        struct Start
        {
            AZ::Vector3 m_localPosition; //!< The current position of the manipulator in local space.
            AZ::Vector3 m_snapOffset; //!< The snap offset amount to ensure manipulator is aligned to the grid.
        };

        //! The state of the manipulator during an interaction.
        struct Current
        {
            AZ::Vector3 m_localOffset; //!< The current offset of the manipulator from its starting position in local space.
        };

        //! Mouse action data used by MouseActionCallback (wraps Start and Current manipulator state).
        struct Action
        {
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

        using MouseActionCallback = AZStd::function<void(const Action&)>;

        void InstallLeftMouseDownCallback(const MouseActionCallback& onMouseDownCallback);
        void InstallLeftMouseUpCallback(const MouseActionCallback& onMouseUpCallback);
        void InstallMouseMoveCallback(const MouseActionCallback& onMouseMoveCallback);

        void InstallEntityIdsToIgnoreFn(EntityIdsToIgnoreFn entityIdsToIgnoreFn);

        void Draw(
            const ManipulatorManagerState& managerState,
            AzFramework::DebugDisplayRequests& debugDisplay,
            const AzFramework::CameraState& cameraState,
            const ViewportInteraction::MouseInteraction& interaction) override;

        void SetView(AZStd::unique_ptr<ManipulatorView>&& view);

    private:
        void OnLeftMouseDownImpl(const ViewportInteraction::MouseInteraction& interaction, float rayIntersectionDistance) override;
        void OnLeftMouseUpImpl(const ViewportInteraction::MouseInteraction& interaction) override;
        void OnMouseMoveImpl(const ViewportInteraction::MouseInteraction& interaction) override;

        void InvalidateImpl() override;
        void SetBoundsDirtyImpl() override;

        //! Initial data recorded when a press first happens with a surface manipulator.
        struct StartInternal
        {
            AZ::Vector3 m_localPosition; //!< The current position of the manipulator in local space.
            AZ::Vector3 m_localHitPosition; //!< The hit position with the terrain in local space.
            AZ::Vector3 m_snapOffset; //!< The snap offset amount to ensure manipulator is aligned to the grid.
        };

        StartInternal m_startInternal; //!< Internal initial state recorded/created in OnMouseDown.

        AZStd::unique_ptr<ManipulatorView> m_manipulatorView = nullptr; //!< Look of manipulator.

        MouseActionCallback m_onLeftMouseDownCallback = nullptr;
        MouseActionCallback m_onLeftMouseUpCallback = nullptr;
        MouseActionCallback m_onMouseMoveCallback = nullptr;

        //! Customization point to determine which (if any) EntityIds to ignore while performing the ray intersection.
        EntityIdsToIgnoreFn m_entityIdsToIgnoreFn = nullptr;

        //! Cached ray request initialized at mouse down and updated during mouse move.
        AzFramework::RenderGeometry::RayRequest m_rayRequest;

        static StartInternal CalculateManipulationDataStart(
            const AZ::Transform& worldFromLocal,
            const AZ::Vector3& worldSurfacePosition,
            const AZ::Vector3& localPosition,
            bool snapping,
            float gridSize,
            int viewportId);

        static Action CalculateManipulationDataAction(
            const StartInternal& startInternal,
            const AZ::Transform& worldFromLocal,
            const AZ::Vector3& worldSurfacePosition,
            bool snapping,
            float gridSize,
            ViewportInteraction::KeyboardModifiers keyboardModifiers,
            int viewportId);
    };
} // namespace AzToolsFramework
