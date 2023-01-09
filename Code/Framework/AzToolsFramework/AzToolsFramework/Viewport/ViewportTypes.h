/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzFramework/Viewport/CameraState.h>
#include <AzFramework/Viewport/ViewportScreen.h>

#include <QPoint>

namespace AZ
{
    class ReflectContext;
    class SerializeContext;
} // namespace AZ

namespace AzToolsFramework
{
    //! Viewport related types that are used when interacting with the viewport.
    namespace ViewportInteraction
    {
        //! Flags to represent each modifier key.
        enum class KeyboardModifier : AZ::u32
        {
            None = 0, //!< No keyboard modifier.
            Alt = 0x01, //!< Alt keyboard modifier.
            Shift = 0x02, //!< Shift keyboard modifier.
            Ctrl = 0x04, //!< Ctrl keyboard modifier.
            Control = Ctrl //!< Alias for Ctrl modifier.
        };

        //! Flags to represent each mouse button.
        enum class MouseButton : AZ::u32
        {
            None = 0, //!< No mouse buttons.
            Left = 0x01, //!< Left mouse button.
            Middle = 0x02, //!< Middle mouse button.
            Right = 0x04 //!< Right mouse button.
        };

        //! The type of mouse event that occurred.
        enum class MouseEvent
        {
            Up, //!< Mouse up event,
            Down, //!< Mouse down event.
            DoubleClick, //!< Mouse double click event.
            Wheel, //!< Mouse wheel event.
            Move, //!< Mouse move event.
        };

        //! Interface over keyboard modifier to query which key is pressed.
        struct KeyboardModifiers
        {
            //! @cond
            AZ_TYPE_INFO(KeyboardModifiers, "{2635F4DF-E7DC-4919-A97B-9AE35FE086D8}");
            KeyboardModifiers() = default;
            //! @endcond

            //! Explicit constructor to create a KeyboardModifier struct.
            explicit KeyboardModifiers(const AZ::u32 keyModifiers)
                : m_keyModifiers(keyModifiers)
            {
            }

            //! Given the current keyboard modifiers, is the Alt key held.
            bool Alt() const
            {
                return (m_keyModifiers & static_cast<AZ::u32>(KeyboardModifier::Alt)) != 0;
            }

            //! Given the current keyboard modifiers, is the Shift key held.
            bool Shift() const
            {
                return (m_keyModifiers & static_cast<AZ::u32>(KeyboardModifier::Shift)) != 0;
            }

            //! Given the current keyboard modifiers, is the Ctrl key held.
            bool Ctrl() const
            {
                return (m_keyModifiers & static_cast<AZ::u32>(KeyboardModifier::Ctrl)) != 0;
            }

            //! Given the current keyboard modifiers, are none being held.
            bool None() const
            {
                return m_keyModifiers == static_cast<AZ::u32>(KeyboardModifier::None);
            }

            //! Given the current keyboard modifiers, is the specified modifier held.
            bool IsHeld(KeyboardModifier keyboardModifier) const
            {
                return m_keyModifiers & static_cast<AZ::u32>(keyboardModifier);
            }

            bool operator==(const KeyboardModifiers& keyboardModifiers) const
            {
                return m_keyModifiers == keyboardModifiers.m_keyModifiers;
            }

            bool operator!=(const KeyboardModifiers& keyboardModifiers) const
            {
                return m_keyModifiers != keyboardModifiers.m_keyModifiers;
            }

            AZ::u32 m_keyModifiers = 0; //!< Raw keyboard modifier state.
        };

        //! Interface over mouse buttons to query which button is pressed.
        struct MouseButtons
        {
            //! @cond
            AZ_TYPE_INFO(MouseButtons, "{1D137B5D-73BF-4FD9-BECA-85E6DC3786CB}");
            MouseButtons() = default;
            //! @endcond

            //! Explicit constructor to create a MouseButton struct.
            explicit MouseButtons(const AZ::u32 mouseButtons)
                : m_mouseButtons(mouseButtons)
            {
            }

            //! Given the current mouse state, is the left mouse button held.
            bool Left() const
            {
                return (m_mouseButtons & static_cast<AZ::u32>(MouseButton::Left)) != 0;
            }

            //! Given the current mouse state, is the middle mouse button held.
            bool Middle() const
            {
                return (m_mouseButtons & static_cast<AZ::u32>(MouseButton::Middle)) != 0;
            }

            //! Given the current mouse state, is the right mouse button held.
            bool Right() const
            {
                return (m_mouseButtons & static_cast<AZ::u32>(MouseButton::Right)) != 0;
            }

            //! Given the current mouse state, are no mouse buttons held.
            bool None() const
            {
                return m_mouseButtons == static_cast<AZ::u32>(MouseButton::None);
            }

            //! Given the current mouse state, are any mouse buttons held.
            bool Any() const
            {
                return m_mouseButtons != static_cast<AZ::u32>(MouseButton::None);
            }

            AZ::u32 m_mouseButtons = 0; //!< Current mouse button state (flags).
        };

        //! Information relevant when interacting with a particular viewport.
        struct InteractionId
        {
            //! @cond
            AZ_TYPE_INFO(InteractionId, "{35593FC2-846F-4AAD-8044-4CD84EC84F9A}");
            InteractionId() = default;
            //! @endcond

            InteractionId(AZ::EntityId cameraId, int viewportId)
                : m_cameraId(cameraId)
                , m_viewportId(viewportId)
            {
            }

            AZ::EntityId m_cameraId; //!< The entity id of the viewport camera.
            int m_viewportId = 0; //!< The id of the viewport being interacted with.
        };

        //! Data representing a mouse pick ray.
        struct MousePick
        {
            //! @cond
            AZ_TYPE_INFO(MousePick, "{A69B9562-FC8C-4DE7-9137-0FF867B1513D}");
            MousePick() = default;
            MousePick(const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDirection, const AzFramework::ScreenPoint& screenPoint)
                : m_rayOrigin(rayOrigin)
                , m_rayDirection(rayDirection)
                , m_screenCoordinates(screenPoint)
            {
            }
            //! @endcond

            AZ::Vector3 m_rayOrigin = AZ::Vector3::CreateZero(); //!< World space.
            AZ::Vector3 m_rayDirection = AZ::Vector3::CreateZero(); //!< World space - normalized.
            AzFramework::ScreenPoint m_screenCoordinates = {}; //!< Screen space.
        };

        //! State relating to an individual mouse interaction.
        struct MouseInteraction
        {
            //! @cond
            AZ_TYPE_INFO(MouseInteraction, "{E67357C3-DFE1-40DF-921F-9CBCFE63A68C}");
            MouseInteraction() = default;
            //! @endcond

            MousePick m_mousePick; //!< The mouse pick ray in world space and screen coordinates in screen space.
            MouseButtons m_mouseButtons; //!< The current state of the mouse buttons.
            InteractionId m_interactionId; /**< The EntityId of the camera this click came from
                                            *  and the id of the viewport it originated from. */
            KeyboardModifiers m_keyboardModifiers; //!< The state of the keyboard modifiers (Alt/Ctrl/Shift).
        };

        //! Structure to compose MouseInteraction (mouse state) and
        //! MouseEvent (MouseEvent::MouseUp/MouseEvent::DownMove etc.)
        struct MouseInteractionEvent
        {
            //! @cond
            AZ_TYPE_INFO(MouseInteractionEvent, "{67FE0826-DD59-4B5B-BEFE-421E83EA7F31}");
            MouseInteractionEvent() = default;
            //! @endcond

            static void Reflect(AZ::SerializeContext& context);

            //! Constructor to create a default MouseInteractionEvent
            MouseInteractionEvent(MouseInteraction mouseInteraction, const MouseEvent mouseEvent, const bool captured)
                : m_mouseInteraction(std::move(mouseInteraction))
                , m_mouseEvent(mouseEvent)
                , m_captured(captured)
            {
            }

            //! Special constructor for mouse wheel event.
            MouseInteractionEvent(MouseInteraction mouseInteraction, const float wheelDelta)
                : m_mouseInteraction(std::move(mouseInteraction))
                , m_mouseEvent(MouseEvent::Wheel)
                , m_wheelDelta(wheelDelta)
            {
            }

            MouseInteraction m_mouseInteraction; //!< Mouse state.
            MouseEvent m_mouseEvent; //!< Mouse event.
            bool m_captured = false; //!< Is the mouse cursor being captured during the event.

            //! Special friend function to return the mouse wheel delta (scroll amount)
            //! if the event was of type MouseEvent::Wheel.
            friend float MouseWheelDelta(const MouseInteractionEvent& mouseInteractionEvent);

        private:
            float m_wheelDelta = 0.0f; //!< The amount the mouse wheel moved during a mouse wheel event.
        };

        //! Checked access to mouse wheel delta - ensure event originated from the mouse wheel.
        inline float MouseWheelDelta(const MouseInteractionEvent& mouseInteractionEvent)
        {
            AZ_Assert(
                mouseInteractionEvent.m_mouseEvent == MouseEvent::Wheel,
                "Attempting to access mouse wheel delta when mouse interaction event was not mouse wheel");

            return mouseInteractionEvent.m_wheelDelta;
        }

        //! A ray projection, originating from a point and extending in a direction specified as a normal.
        struct ProjectedViewportRay
        {
            AZ::Vector3 m_origin;
            AZ::Vector3 m_direction;
        };

        //! Utility function to return a viewport ray.
        inline ProjectedViewportRay ViewportScreenToWorldRay(
            const AzFramework::CameraState& cameraState, const AzFramework::ScreenPoint& screenPoint)
        {
            const AZ::Vector3 rayOrigin = AzFramework::ScreenToWorld(screenPoint, cameraState);
            const AZ::Vector3 rayDirection = (rayOrigin - cameraState.m_position).GetNormalized();
            return ProjectedViewportRay{ rayOrigin, rayDirection };
        }

        //! Return QPoint from AzFramework::ScreenPoint.
        inline QPoint QPointFromScreenPoint(const AzFramework::ScreenPoint& screenPoint)
        {
            return { screenPoint.m_x, screenPoint.m_y };
        }

        //! Return AzFramework::ScreenPoint from QPoint.
        inline AzFramework::ScreenPoint ScreenPointFromQPoint(const QPoint& qpoint)
        {
            return AzFramework::ScreenPoint{ qpoint.x(), qpoint.y() };
        }

        //! Map from Qt -> Open 3D Engine buttons.>>>>>>> main
        inline AZ::u32 TranslateMouseButtons(const Qt::MouseButtons buttons)
        {
            AZ::u32 result = 0;
            result |= buttons & Qt::MouseButton::LeftButton ? static_cast<AZ::u32>(MouseButton::Left) : 0;
            result |= buttons & Qt::MouseButton::RightButton ? static_cast<AZ::u32>(MouseButton::Right) : 0;
            result |= buttons & Qt::MouseButton::MiddleButton ? static_cast<AZ::u32>(MouseButton::Middle) : 0;
            return result;
        }

        //! Map from Qt -> Open 3D Engine modifiers.
        inline AZ::u32 TranslateKeyboardModifiers(const Qt::KeyboardModifiers modifiers)
        {
            AZ::u32 result = 0;
            result |= modifiers & Qt::KeyboardModifier::ShiftModifier ? static_cast<AZ::u32>(KeyboardModifier::Shift) : 0;
            result |= modifiers & Qt::KeyboardModifier::ControlModifier ? static_cast<AZ::u32>(KeyboardModifier::Ctrl) : 0;
            result |= modifiers & Qt::KeyboardModifier::AltModifier ? static_cast<AZ::u32>(KeyboardModifier::Alt) : 0;
            return result;
        }

        //! Interface to translate Qt modifiers to Open 3D Engine modifiers.
        inline KeyboardModifiers BuildKeyboardModifiers(const Qt::KeyboardModifiers modifiers)
        {
            return KeyboardModifiers(TranslateKeyboardModifiers(modifiers));
        }

        //! Interface to translate Qt buttons to Open 3D Engine buttons.
        inline MouseButtons BuildMouseButtons(const Qt::MouseButtons buttons)
        {
            return MouseButtons(TranslateMouseButtons(buttons));
        }

        //! Generate mouse buttons from single button enum.
        inline MouseButtons MouseButtonsFromButton(MouseButton button)
        {
            MouseButtons mouseButtons;
            mouseButtons.m_mouseButtons = static_cast<AZ::u32>(button);
            return mouseButtons;
        }

        //! Build a mouse pick from the specified mouse position and camera state.
        inline MousePick BuildMousePick(const AzFramework::CameraState& cameraState, const AzFramework::ScreenPoint& screenPoint)
        {
            const auto ray = ViewportScreenToWorldRay(cameraState, screenPoint);
            return MousePick(ray.m_origin, ray.m_direction, screenPoint);
        }

        //! Create a mouse interaction from the specified pick, buttons, interaction id and keyboard modifiers.
        MouseInteraction BuildMouseInteraction(
            const MousePick& mousePick, MouseButtons buttons, InteractionId interactionId, KeyboardModifiers modifiers);

        //! Create a mouse buttons from the specified mouse button.
        inline MouseButtons BuildMouseButtons(const MouseButton button)
        {
            return MouseButtons(aznumeric_cast<AZ::u32>(button));
        }

        //! Create a mouse interaction event from the specified interaction and event.
        MouseInteractionEvent BuildMouseInteractionEvent(
            const MouseInteraction& mouseInteraction, MouseEvent event, bool cursorCaptured = false);

        //! Reflect all viewport related types.
        void ViewportInteractionReflect(AZ::ReflectContext* context);
    } // namespace ViewportInteraction
} // namespace AzToolsFramework
