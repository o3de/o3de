/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <BarrierInput/RawInputNotificationBus_Barrier.h>

#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>

#include <AzCore/std/parallel/mutex.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace BarrierInput
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Barrier specific implementation for mouse input devices.
    class InputDeviceMouseBarrier : public AzFramework::InputDeviceMouse::Implementation
                                  , public RawInputNotificationBusBarrier::Handler
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(InputDeviceMouseBarrier, AZ::SystemAllocator);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Custom factory create function
        //! \param[in] inputDevice Reference to the input device being implemented
        static Implementation* Create(AzFramework::InputDeviceMouse& inputDevice);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] inputDevice Reference to the input device being implemented
        InputDeviceMouseBarrier(AzFramework::InputDeviceMouse& inputDevice);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Destructor
        ~InputDeviceMouseBarrier() override;

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceMouse::Implementation::IsConnected
        bool IsConnected() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceMouse::Implementation::SetSystemCursorState
        void SetSystemCursorState(AzFramework::SystemCursorState systemCursorState) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceMouse::Implementation::GetSystemCursorState
        AzFramework::SystemCursorState GetSystemCursorState() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceMouse::Implementation::SetSystemCursorPositionNormalized
        void SetSystemCursorPositionNormalized(AZ::Vector2 positionNormalized) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceMouse::Implementation::GetSystemCursorPositionNormalized
        AZ::Vector2 GetSystemCursorPositionNormalized() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceMouse::Implementation::TickInputDevice
        void TickInputDevice() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref RawInputNotificationsBarrier::OnRawMouseButtonDownEvent
        void OnRawMouseButtonDownEvent(uint32_t buttonIndex) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref RawInputNotificationsBarrier::OnRawMouseButtonUpEvent
        void OnRawMouseButtonUpEvent(uint32_t buttonIndex) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref RawInputNotificationsBarrier::OnRawMouseMovementEvent
        void OnRawMouseMovementEvent(float movementX, float movementY) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref RawInputNotificationsBarrier::OnRawMousePositionEvent
        void OnRawMousePositionEvent(float positionX, float positionY) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Thread safe method to queue raw button events to be processed in the main thread update
        //! \param[in] buttonIndex The index of the button
        //! \param[in] rawButtonState The raw button state
        void ThreadSafeQueueRawButtonEvent(uint32_t buttonIndex, bool rawButtonState);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Variables
        AzFramework::SystemCursorState m_systemCursorState;
        AZ::Vector2                    m_systemCursorPositionNormalized;

        RawButtonEventQueueByIdMap     m_threadAwareRawButtonEventQueuesById;
        AZStd::mutex                   m_threadAwareRawButtonEventQueuesByIdMutex;

        RawMovementEventQueueByIdMap   m_threadAwareRawMovementEventQueuesById;
        AZStd::mutex                   m_threadAwareRawMovementEventQueuesByIdMutex;

        AZ::Vector2                    m_threadAwareSystemCursorPosition;
        AZStd::mutex                   m_threadAwareSystemCursorPositionMutex;
    };

    struct InputDeviceMouseBarrierImplFactory 
        : public AzFramework::InputDeviceMouse::ImplementationFactory
    {
        AZStd::unique_ptr<AzFramework::InputDeviceMouse::Implementation> Create(AzFramework::InputDeviceMouse& inputDevice) override;
    };

} // namespace BarrierInput
