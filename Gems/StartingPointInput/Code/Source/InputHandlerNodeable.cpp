/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

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
        SCRIPT_CANVAS_PERFORMANCE_SCOPE_LATENT_NODEABLE
        CallPressed(value);
    }

    void InputHandlerNodeable::OnHeld(float value)
    {
        SCRIPT_CANVAS_PERFORMANCE_SCOPE_LATENT_NODEABLE
        CallHeld(value);
    }

    void InputHandlerNodeable::OnReleased(float value)
    {
        SCRIPT_CANVAS_PERFORMANCE_SCOPE_LATENT_NODEABLE
        CallReleased(value);
    }
}
