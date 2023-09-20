/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <BarrierInputMouse.h>

#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/ViewportContextBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace BarrierInput
{
    using namespace AzFramework;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceMouseBarrier::InputDeviceMouseBarrier(InputDeviceMouse& inputDevice)
        : InputDeviceMouse::Implementation(inputDevice)
        , m_systemCursorState(SystemCursorState::Unknown)
        , m_systemCursorPositionNormalized(0.5f, 0.5f)
        , m_threadAwareRawButtonEventQueuesById()
        , m_threadAwareRawButtonEventQueuesByIdMutex()
        , m_threadAwareRawMovementEventQueuesById()
        , m_threadAwareRawMovementEventQueuesByIdMutex()
        , m_threadAwareSystemCursorPosition(0.0f, 0.0f)
        , m_threadAwareSystemCursorPositionMutex()
    {
        RawInputNotificationBusBarrier::Handler::BusConnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceMouseBarrier::~InputDeviceMouseBarrier()
    {
        RawInputNotificationBusBarrier::Handler::BusDisconnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceMouseBarrier::IsConnected() const
    {
        // We could check the validity of the socket connection to the Barrier server
        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMouseBarrier::SetSystemCursorState(SystemCursorState systemCursorState)
    {
        // This doesn't apply when using Barrier, but we'll store it so it can be queried
        m_systemCursorState = systemCursorState;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    SystemCursorState InputDeviceMouseBarrier::GetSystemCursorState() const
    {
        return m_systemCursorState;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMouseBarrier::SetSystemCursorPositionNormalized(AZ::Vector2 positionNormalized)
    {
        // This will simply get overridden by the next call to OnRawMousePositionEvent, but there's
        // not much we can do about it, and Barrier mouse input is only for debug purposes anyway.
        m_systemCursorPositionNormalized = positionNormalized;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::Vector2 InputDeviceMouseBarrier::GetSystemCursorPositionNormalized() const
    {
        return m_systemCursorPositionNormalized;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMouseBarrier::TickInputDevice()
    {
        {
            // Queue all mouse button events that were received in the other thread
            AZStd::scoped_lock lock(m_threadAwareRawButtonEventQueuesByIdMutex);
            for (const auto& buttonEventQueuesById : m_threadAwareRawButtonEventQueuesById)
            {
                const InputChannelId& inputChannelId = buttonEventQueuesById.first;
                for (bool rawButtonState : buttonEventQueuesById.second)
                {
                    QueueRawButtonEvent(inputChannelId, rawButtonState);
                }
            }
            m_threadAwareRawButtonEventQueuesById.clear();
        }

        bool receivedRawMovementEvents = false;
        {
            // Queue all mouse movement events that were received in the other thread
            AZStd::scoped_lock lock(m_threadAwareRawMovementEventQueuesByIdMutex);
            for (const auto& movementEventQueuesById : m_threadAwareRawMovementEventQueuesById)
            {
                const InputChannelId& inputChannelId = movementEventQueuesById.first;
                for (float rawMovementDelta : movementEventQueuesById.second)
                {
                    QueueRawMovementEvent(inputChannelId, rawMovementDelta);
                    receivedRawMovementEvents = true;
                }
            }
            m_threadAwareRawMovementEventQueuesById.clear();
        }

        // Update the system cursor position
        auto atomViewportRequests = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
        AZ::RPI::ViewportContextPtr viewportContext = atomViewportRequests->GetDefaultViewportContext();
        if (viewportContext)
        {
            const AzFramework::WindowSize windowSize = viewportContext->GetViewportSize();
            const float windowWidth = static_cast<float>(windowSize.m_width);
            const float windowHeight = static_cast<float>(windowSize.m_height);
            const AZ::Vector2 oldSystemCursorPositionNormalized = m_systemCursorPositionNormalized;

            AZStd::scoped_lock lock(m_threadAwareSystemCursorPositionMutex);
            {
                const AZ::Vector2 normalizedPosition(m_threadAwareSystemCursorPosition.GetX() / windowWidth,
                                                     m_threadAwareSystemCursorPosition.GetY() / windowHeight);
                m_systemCursorPositionNormalized = normalizedPosition;
            }

            // In theory Barrier should send relative mouse movement events as 'DMRM' messages, which are
            // forwarded to InputDeviceMouseBarrier::OnRawMouseMovementEvent, but this does not appear to
            // be happening, so if we didn't receive any relative mouse movement events this frame we can
            // just approximate the movement ourselves. Unlike other mouse implementations where movement
            // events are sent 'raw' before any operating system ballistics/smoothing is applied, Barrier
            // seems to calculate relative mouse movement events by taking the delta between the previous
            // system cursor position and the current one, so we should obtain the same result regardless.
            if (!receivedRawMovementEvents)
            {
                const AZ::Vector2 mouseMovementDelta = m_systemCursorPositionNormalized - oldSystemCursorPositionNormalized;
                QueueRawMovementEvent(InputDeviceMouse::Movement::X, mouseMovementDelta.GetX() * windowWidth);
                QueueRawMovementEvent(InputDeviceMouse::Movement::Y, mouseMovementDelta.GetY() * windowHeight);
            }
        }

        // Process raw event queues once each frame
        ProcessRawEventQueues();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMouseBarrier::OnRawMouseButtonDownEvent(uint32_t buttonIndex)
    {
        ThreadSafeQueueRawButtonEvent(buttonIndex, true);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMouseBarrier::OnRawMouseButtonUpEvent(uint32_t buttonIndex)
    {
        ThreadSafeQueueRawButtonEvent(buttonIndex, false);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMouseBarrier::OnRawMouseMovementEvent(float movementX, float movementY)
    {
        AZStd::scoped_lock lock(m_threadAwareRawMovementEventQueuesByIdMutex);
        m_threadAwareRawMovementEventQueuesById[InputDeviceMouse::Movement::X].push_back(movementX);
        m_threadAwareRawMovementEventQueuesById[InputDeviceMouse::Movement::Y].push_back(movementY);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMouseBarrier::OnRawMousePositionEvent(float positionX,
                                                          float positionY)
    {
        AZStd::scoped_lock lock(m_threadAwareSystemCursorPositionMutex);
        m_threadAwareSystemCursorPosition = AZ::Vector2(positionX, positionY);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMouseBarrier::ThreadSafeQueueRawButtonEvent(uint32_t buttonIndex,
                                                                bool rawButtonState)
    {
        const InputChannelId* inputChannelId = nullptr;
        switch (buttonIndex)
        {
            case 1: { inputChannelId = &InputDeviceMouse::Button::Left; } break;
            case 2: { inputChannelId = &InputDeviceMouse::Button::Middle; } break;
            case 3: { inputChannelId = &InputDeviceMouse::Button::Right; } break;
        }

        if (inputChannelId)
        {
            AZStd::scoped_lock lock(m_threadAwareRawButtonEventQueuesByIdMutex);
            m_threadAwareRawButtonEventQueuesById[*inputChannelId].push_back(rawButtonState);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZStd::unique_ptr<InputDeviceMouse::Implementation> InputDeviceMouseBarrierImplFactory::Create(InputDeviceMouse& inputDevice)
    {
        return AZStd::make_unique<InputDeviceMouseBarrier>(inputDevice);
    }


} // namespace BarrierInput
