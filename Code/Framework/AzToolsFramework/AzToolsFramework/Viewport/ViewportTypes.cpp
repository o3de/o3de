/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ViewportTypes.h"

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AzToolsFramework
{
    namespace ViewportInteraction
    {
        void ViewportInteractionReflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<KeyboardModifiers>()->Field("KeyboardModifiers", &KeyboardModifiers::m_keyModifiers);

                serializeContext->Class<MouseButtons>()->Field("MouseButtons", &MouseButtons::m_mouseButtons);

                serializeContext->Class<InteractionId>()
                    ->Field("CameraId", &InteractionId::m_cameraId)
                    ->Field("ViewportId", &InteractionId::m_viewportId);

                serializeContext->Class<MousePick>()
                    ->Field("RayOrigin", &MousePick::m_rayOrigin)
                    ->Field("RayDirection", &MousePick::m_rayDirection)
                    ->Field("ScreenCoordinates", &MousePick::m_screenCoordinates);

                serializeContext->Class<MouseInteraction>()
                    ->Field("MousePick", &MouseInteraction::m_mousePick)
                    ->Field("MouseButtons", &MouseInteraction::m_mouseButtons)
                    ->Field("InteractionId", &MouseInteraction::m_interactionId)
                    ->Field("KeyboardModifiers", &MouseInteraction::m_keyboardModifiers);

                MouseInteractionEvent::Reflect(*serializeContext);
            }
        }

        void MouseInteractionEvent::Reflect(AZ::SerializeContext& serializeContext)
        {
            serializeContext.Class<MouseInteractionEvent>()
                ->Field("MouseInteraction", &MouseInteractionEvent::m_mouseInteraction)
                ->Field("MouseEvent", &MouseInteractionEvent::m_mouseEvent)
                ->Field("WheelDelta", &MouseInteractionEvent::m_wheelDelta);
        }

        MouseInteraction BuildMouseInteraction(
            const MousePick& mousePick, const MouseButtons buttons, const InteractionId interactionId, const KeyboardModifiers modifiers)
        {
            MouseInteraction interaction;
            interaction.m_mousePick = mousePick;
            interaction.m_mouseButtons = buttons;
            interaction.m_interactionId = interactionId;
            interaction.m_keyboardModifiers = modifiers;
            return interaction;
        }

        MouseInteractionEvent BuildMouseInteractionEvent(
            const MouseInteraction& mouseInteraction, const MouseEvent event, const bool cursorCaptured /*= false*/)
        {
            return MouseInteractionEvent(mouseInteraction, event, cursorCaptured);
        }
    } // namespace ViewportInteraction
} // namespace AzToolsFramework
