/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

// script canvas
#include <ScriptCanvas/Core/Nodeable.h>
#include <ScriptCanvas/Core/NodeableNode.h>
#include <ScriptCanvas/CodeGen/NodeableCodegen.h>
#include <StartingPointInput/InputEventNotificationBus.h>

#include <Source/InputHandlerNodeable.generated.h>

namespace StartingPointInput
{
    //////////////////////////////////////////////////////////////////////////
    /// Input handles raw input from any source and outputs Pressed, Held, and Released input events
    class InputHandlerNodeable
        : public ScriptCanvas::Nodeable
        , protected InputEventNotificationBus::Handler
    {
        SCRIPTCANVAS_NODE(InputHandlerNodeable)

    public:
        AZ_CLASS_ALLOCATOR(InputHandlerNodeable, AZ::SystemAllocator);

        ~InputHandlerNodeable() override;

    protected:
        void OnDeactivate() override;

        //////////////////////////////////////////////////////////////////////////
        /// InputEventNotificationBus::Handler
        void OnPressed(float value) override;
        void OnHeld(float value) override;
        void OnReleased(float value) override;
    };
}


