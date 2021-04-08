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
        InputHandlerNodeable() = default;
        virtual ~InputHandlerNodeable();
        InputHandlerNodeable(const InputHandlerNodeable&) = default;
        InputHandlerNodeable& operator=(const InputHandlerNodeable&) = default;

    protected:
        void OnDeactivate() override;

        //////////////////////////////////////////////////////////////////////////
        /// InputEventNotificationBus::Handler
        void OnPressed(float value) override;
        void OnHeld(float value) override;
        void OnReleased(float value) override;
    };
}


