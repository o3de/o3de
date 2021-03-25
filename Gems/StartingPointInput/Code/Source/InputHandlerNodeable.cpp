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
#include <StartingPointInput_precompiled.h>
#include <InputHandlerNodeable.h>

#include <ScriptCanvas/Utils/SerializationUtils.h>

using namespace ScriptCanvas;

namespace StartingPointInput
{
    InputHandlerNodeable::~InputHandlerNodeable()
    {
        InputEventNotificationBus::Handler::BusDisconnect();
    }

    void InputHandlerNodeable::ConnectEvent(AZStd::string eventName)
    {
        InputEventNotificationBus::Handler::BusDisconnect();

        InputEventNotificationBus::Handler::BusConnect(InputEventNotificationId(eventName.c_str()));
    }

    void InputHandlerNodeable::OnDeactivate()
    {
        InputEventNotificationBus::Handler::BusDisconnect();
    }

    void InputHandlerNodeable::OnPressed(float value)
    {
        SCRIPT_CANVAS_PERFORMANCE_SCOPE_LATENT(GetScriptCanvasId(), GetAssetId());
        CallPressed(value);
    }

    void InputHandlerNodeable::OnHeld(float value)
    {
        SCRIPT_CANVAS_PERFORMANCE_SCOPE_LATENT(GetScriptCanvasId(), GetAssetId());
        CallHeld(value);
    }

    void InputHandlerNodeable::OnReleased(float value)
    {
        SCRIPT_CANVAS_PERFORMANCE_SCOPE_LATENT(GetScriptCanvasId(), GetAssetId());
        CallReleased(value);
    }
}
