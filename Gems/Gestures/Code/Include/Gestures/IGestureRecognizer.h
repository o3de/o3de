/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <stdint.h>
#include <IRenderer.h>
#include <Cry_Vector2.h>

#include <AzFramework/Input/Events/InputChannelEventListener.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzFramework/Input/Devices/Touch/InputDeviceTouch.h>

#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/ViewportContextBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace Gestures
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Base class for all gesture recognizers
    class IRecognizer : public AzFramework::InputChannelNotificationBus::Handler,
                        public AzFramework::InputChannel::PositionData2D
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(IRecognizer, AZ::SystemAllocator);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Type Info
        AZ_RTTI(IRecognizer, "{C3E00298-1953-465F-A360-EBC10B62BFE8}", CustomData);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Enable this gesture recognizer
        void Enable() { AzFramework::InputChannelNotificationBus::Handler::BusConnect(); }

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Disable this gesture recognizer
        void Disable() { AzFramework::InputChannelNotificationBus::Handler::BusDisconnect(); }

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputChannelNotifications::GetPriority
        AZ::s32 GetPriority() const override { return AzFramework::InputChannelEventListener::GetPriorityUI() + 1; }

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputChannelNotifications::OnInputChannelEvent
        void OnInputChannelEvent(const AzFramework::InputChannel& inputChannel,
                                 bool& o_hasBeenConsumed) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Override to be notified when a mouse button or finger on a touch screen is initially
        //! pressed, unless the input event was consumed by a higher priority gesture recognizer.
        //! \param[in] screenPositionPixels The screen position (in pixels) of the input event.
        //! \param[in] pointerIndex The pointer index of the input event.
        //! \return True to consume the underlying input event (preventing it from being sent on
        //! to other lower-priority gesture recognizers or input listeners), or false otherwise.
        virtual bool OnPressedEvent(const AZ::Vector2& screenPositionPixels, uint32_t pointerIndex) = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Override to be notified each frame a mouse button or finger on a touch screen remains
        //! pressed, unless the input event was consumed by a higher priority gesture recognizer.
        //! \param[in] screenPositionPixels The screen position (in pixels) of the input event.
        //! \param[in] pointerIndex The pointer index of the input event.
        //! \return True to consume the underlying input event (preventing it from being sent on
        //! to other lower-priority gesture recognizers or input listeners), or false otherwise.
        virtual bool OnDownEvent(const AZ::Vector2& screenPositionPixels, uint32_t pointerIndex) = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Override to be notified when a pressed mouse button or finger on a touch screen becomes
        //! released, unless the input event was consumed by a higher priority gesture recognizer.
        //! \param[in] screenPositionPixels The screen position (in pixels) of the input event.
        //! \param[in] pointerIndex The pointer index of the input event.
        //! \return True to consume the underlying input event (preventing it from being sent on
        //! to other lower-priority gesture recognizers or input listeners), or false otherwise.
        virtual bool OnReleasedEvent(const AZ::Vector2& screenPositionPixels, uint32_t pointerIndex) = 0;

    protected:

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Get the gesture pointer index associated with an input channel.
        //! \param[in] inputChannel The input channel to find the pointer index for.
        //! \return The gesture pointer index of the input channel, or INVALID_GESTURE_POINTER_INDEX.
        uint32_t GetGesturePointerIndex(const AzFramework::InputChannel& inputChannel);
        const uint32_t INVALID_GESTURE_POINTER_INDEX = static_cast<uint32_t>(-1);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Convenience function that converts back to a normalized position before calling through
        //! to the base AzFramework::InputChannel::PositionData2D::UpdateNormalizedPositionAndDelta
        //! \param[in] screenPositionPixels The screen position (in pixels) of the input event.
        void UpdateNormalizedPositionAndDeltaFromScreenPosition(const AZ::Vector2& screenPositionPixels);

        // Using Vec2 directly as a member of derived recognizer classes results in
        // linker warnings because Vec2 doesn't have dll import/export specifiers.
        struct ScreenPosition
        {
            ScreenPosition()
                : x(0.0f)
                , y(0.0f) {}
            ScreenPosition(const AZ::Vector2& vec2)
                : x(vec2.GetX())
                , y(vec2.GetY()) {}
            operator AZ::Vector2() const
            {
                return AZ::Vector2(x, y);
            }

            float x;
            float y;
        };
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Base class for all discrete gesture recognizers
    class RecognizerDiscrete : public IRecognizer
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(RecognizerDiscrete, AZ::SystemAllocator);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Type Info
        AZ_RTTI(RecognizerDiscrete, "{51258910-62B3-4830-AF7B-9DA3AD3585CC}", IRecognizer);

    protected:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Override to be notified when the discrete gesture is recognized.
        virtual void OnDiscreteGestureRecognized() = 0;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Base class for all continuous gesture recognizers
    class RecognizerContinuous : public IRecognizer
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(RecognizerContinuous, AZ::SystemAllocator);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Type Info
        AZ_RTTI(RecognizerContinuous, "{A8B16552-E1F3-4469-BEB8-5D209554924E}", IRecognizer);

    protected:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Override to be notified when the continuous gesture is initiated.
        virtual void OnContinuousGestureInitiated() = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Override to be notified when the continuous gesture is updated.
        virtual void OnContinuousGestureUpdated() = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Override to be notified when the continuous gesture is ended.
        virtual void OnContinuousGestureEnded() = 0;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void IRecognizer::OnInputChannelEvent(const AzFramework::InputChannel& inputChannel,
                                                 bool& o_hasBeenConsumed)
    {
        if (o_hasBeenConsumed)
        {
            return;
        }

        const AzFramework::InputChannel::PositionData2D* positionData2D = inputChannel.GetCustomData<AzFramework::InputChannel::PositionData2D>();
        if (!positionData2D)
        {
            // This input event is not associated with a position, so it is irrelevant for gestures.
            return;
        }

        const uint32_t pointerIndex = GetGesturePointerIndex(inputChannel);
        if (pointerIndex == INVALID_GESTURE_POINTER_INDEX)
        {
            // This input event is not associated with a pointer index, so it is irrelevant for gestures.
            return;
        }

        auto atomViewportRequests = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
        AZ::RPI::ViewportContextPtr viewportContext = atomViewportRequests->GetDefaultViewportContext();
        if (viewportContext == nullptr)
        {
            return;
        }

        AzFramework::WindowSize windowSize = viewportContext->GetViewportSize();
        const AZ::Vector2 eventScreenPositionPixels = positionData2D->ConvertToScreenSpaceCoordinates(static_cast<float>(windowSize.m_width),
                                                                                                      static_cast<float>(windowSize.m_height));
        if (inputChannel.IsStateBegan())
        {
            o_hasBeenConsumed = OnPressedEvent(eventScreenPositionPixels, pointerIndex);
        }
        else if (inputChannel.IsStateUpdated())
        {
            o_hasBeenConsumed = OnDownEvent(eventScreenPositionPixels, pointerIndex);
        }
        else if (inputChannel.IsStateEnded())
        {
            o_hasBeenConsumed = OnReleasedEvent(eventScreenPositionPixels, pointerIndex);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline uint32_t IRecognizer::GetGesturePointerIndex(const AzFramework::InputChannel& inputChannel)
    {
        // Only recognize gestures for the default mouse input device. The Editor may register synthetic
        // mouse input devices with the same mouse input channels, which can confuse gesture recognition.
        if (inputChannel.GetInputDevice().GetInputDeviceId() == AzFramework::InputDeviceMouse::Id)
        {
            const auto& mouseButtonIt = AZStd::find(AzFramework::InputDeviceMouse::Button::All.cbegin(),
                                                    AzFramework::InputDeviceMouse::Button::All.cend(),
                                                    inputChannel.GetInputChannelId());
            if (mouseButtonIt != AzFramework::InputDeviceMouse::Button::All.cend())
            {
                return static_cast<uint32_t>(mouseButtonIt - AzFramework::InputDeviceMouse::Button::All.cbegin());
            }
        }

        const auto& touchIndexIt = AZStd::find(AzFramework::InputDeviceTouch::Touch::All.cbegin(),
                                               AzFramework::InputDeviceTouch::Touch::All.cend(),
                                                inputChannel.GetInputChannelId());
        if (touchIndexIt != AzFramework::InputDeviceTouch::Touch::All.cend())
        {
            return static_cast<uint32_t>(touchIndexIt - AzFramework::InputDeviceTouch::Touch::All.cbegin());
        }

        return INVALID_GESTURE_POINTER_INDEX;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void IRecognizer::UpdateNormalizedPositionAndDeltaFromScreenPosition(const AZ::Vector2& screenPositionPixels)
    {
        auto atomViewportRequests = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
        AZ::RPI::ViewportContextPtr viewportContext = atomViewportRequests->GetDefaultViewportContext();
        if (viewportContext == nullptr)
        {
            return;
        }

        AzFramework::WindowSize windowSize = viewportContext->GetViewportSize();
        const AZ::Vector2 normalizedPosition(screenPositionPixels.GetX() / static_cast<float>(windowSize.m_width),
                                             screenPositionPixels.GetY() / static_cast<float>(windowSize.m_height));
        AzFramework::InputChannel::PositionData2D::UpdateNormalizedPositionAndDelta(normalizedPosition);
    }
}
