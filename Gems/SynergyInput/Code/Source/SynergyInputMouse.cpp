/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SynergyInputMouse.h>

#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/ViewportContextBus.h>

#define CONTROL_RELATIVE_INPUT_WITH_MIDDLE_BUTTON

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace SynergyInput
{
    using namespace AzFramework;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceMouse::Implementation* InputDeviceMouseSynergy::Create(InputDeviceMouse& inputDevice)
    {
        return aznew InputDeviceMouseSynergy(inputDevice);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceMouseSynergy::InputDeviceMouseSynergy(InputDeviceMouse& inputDevice)
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
        RawInputNotificationBusSynergy::Handler::BusConnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceMouseSynergy::~InputDeviceMouseSynergy()
    {
        RawInputNotificationBusSynergy::Handler::BusDisconnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceMouseSynergy::IsConnected() const
    {
        // We could check the validity of the socket connection to the synergy server
        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMouseSynergy::SetSystemCursorState(SystemCursorState systemCursorState)
    {
        // This doesn't apply when using synergy, but we'll store it so it can be queried
        m_systemCursorState = systemCursorState;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    SystemCursorState InputDeviceMouseSynergy::GetSystemCursorState() const
    {
        return m_systemCursorState;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMouseSynergy::SetSystemCursorPositionNormalized(AZ::Vector2 positionNormalized)
    {
        // This will simply get overridden by the next call to OnRawMousePositionEvent, but there's
        // not much we can do about it, and Synergy mouse input is only for debug purposes anyway.
        m_systemCursorPositionNormalized = positionNormalized;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::Vector2 InputDeviceMouseSynergy::GetSystemCursorPositionNormalized() const
    {
        return m_systemCursorPositionNormalized;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMouseSynergy::TickInputDevice()
    {
        {
            // Queue all mouse button events that were received in the other thread
            AZStd::lock_guard<AZStd::mutex> lock(m_threadAwareRawButtonEventQueuesByIdMutex);
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

        {
            // Queue all mouse movement events that were received in the other thread
            AZStd::lock_guard<AZStd::mutex> lock(m_threadAwareRawMovementEventQueuesByIdMutex);
            for (const auto& movementEventQueuesById : m_threadAwareRawMovementEventQueuesById)
            {
                const InputChannelId& inputChannelId = movementEventQueuesById.first;
                for (float rawMovementDelta : movementEventQueuesById.second)
                {
                    QueueRawMovementEvent(inputChannelId, rawMovementDelta);
                }
            }
            m_threadAwareRawMovementEventQueuesById.clear();
        }

        {
            // Update the system cursor position
            AZStd::lock_guard<AZStd::mutex> lock(m_threadAwareSystemCursorPositionMutex);

            auto atomViewportRequests = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
            AZ::RPI::ViewportContextPtr viewportContext = atomViewportRequests->GetDefaultViewportContext();
            if (viewportContext)
            {
                const AzFramework::WindowSize windowSize = viewportContext->GetViewportSize();
                const AZ::Vector2 normalizedPosition(m_threadAwareSystemCursorPosition.GetX() / static_cast<float>(windowSize.m_width),
                                                     m_threadAwareSystemCursorPosition.GetY() / static_cast<float>(windowSize.m_height));
                m_systemCursorPositionNormalized = normalizedPosition;
            }
        }

        // Process raw event queues once each frame
        ProcessRawEventQueues();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMouseSynergy::OnRawMouseButtonDownEvent(uint32_t buttonIndex)
    {
        ThreadSafeQueueRawButtonEvent(buttonIndex, true);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMouseSynergy::OnRawMouseButtonUpEvent(uint32_t buttonIndex)
    {
        ThreadSafeQueueRawButtonEvent(buttonIndex, false);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMouseSynergy::OnRawMouseMovementEvent(float movementX, float movementY)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_threadAwareRawMovementEventQueuesByIdMutex);
        m_threadAwareRawMovementEventQueuesById[InputDeviceMouse::Movement::X].push_back(movementX);
        m_threadAwareRawMovementEventQueuesById[InputDeviceMouse::Movement::Y].push_back(movementY);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMouseSynergy::OnRawMousePositionEvent(float positionX,
                                                          float positionY)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_threadAwareSystemCursorPositionMutex);
        m_threadAwareSystemCursorPosition = AZ::Vector2(positionX, positionY);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMouseSynergy::ThreadSafeQueueRawButtonEvent(uint32_t buttonIndex,
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
            AZStd::lock_guard<AZStd::mutex> lock(m_threadAwareRawButtonEventQueuesByIdMutex);
            m_threadAwareRawButtonEventQueuesById[*inputChannelId].push_back(rawButtonState);
        }
    }
} // namespace SynergyInput
