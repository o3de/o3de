/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "BaseManipulator.h"

#include <AzCore/Math/Quaternion.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>

namespace AzToolsFramework
{
    class ManipulatorViewProjectedCircle;

    class PaintBrushManipulator : public BaseManipulator, public ManipulatorSpace
    {
        //! Private constructor.
        PaintBrushManipulator(const AZ::Transform& worldFromLocal, const AZ::EntityComponentIdPair& entityComponentIdPair);

    public:
        AZ_RTTI(PaintBrushManipulator, "{0621CB58-21FD-474A-A296-5B1192E714E7}", BaseManipulator);
        AZ_CLASS_ALLOCATOR(PaintBrushManipulator, AZ::SystemAllocator, 0);

        PaintBrushManipulator() = delete;
        PaintBrushManipulator(const PaintBrushManipulator&) = delete;
        PaintBrushManipulator& operator=(const PaintBrushManipulator&) = delete;

        ~PaintBrushManipulator() = default;

        //! A Manipulator must only be created and managed through a shared_ptr.
        static AZStd::shared_ptr<PaintBrushManipulator> MakeShared(
            const AZ::Transform& worldFromLocal, const AZ::EntityComponentIdPair& entityComponentIdPair);

        //! The state of the manipulator at the start of an interaction.
        struct Start
        {
        };

        //! The state of the manipulator during an interaction.
        struct Current
        {
        };

        //! Mouse action data used by MouseActionCallback (wraps Start and Current manipulator state).
        struct Action
        {
            Start m_start;
            Current m_current;
            ViewportInteraction::KeyboardModifiers m_modifiers;
        };

        void Draw(
            const ManipulatorManagerState& managerState, AzFramework::DebugDisplayRequests& debugDisplay,
            const AzFramework::CameraState& cameraState, const ViewportInteraction::MouseInteraction& mouseInteraction) override;

        void SetView(AZStd::shared_ptr<ManipulatorViewProjectedCircle> view);
        void SetRadius(const float radius);

    private:
        struct StartInternal
        {
            AZ::Transform m_worldFromLocal;
            AZ::Transform m_localTransform;
        };

        struct CurrentInternal
        {
        };

        struct ActionInternal
        {
            StartInternal m_start;
            CurrentInternal m_current;
        };

        ActionInternal m_actionInternal;

        AZStd::shared_ptr<ManipulatorViewProjectedCircle> m_manipulatorView;
    };
} // namespace AzToolsFramework
