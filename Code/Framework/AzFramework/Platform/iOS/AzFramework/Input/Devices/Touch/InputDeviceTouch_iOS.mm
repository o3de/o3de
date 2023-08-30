/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Components/NativeUISystemComponent.h>
#include <AzFramework/Input/Devices/Touch/InputDeviceTouch.h>
#include <AzFramework/Input/Buses/Notifications/RawInputNotificationBus_Platform.h>

#include <UIKit/UIKit.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Platform specific implementation for ios touch input devices
    class InputDeviceTouchIos : public InputDeviceTouch::Implementation
                              , public RawInputNotificationBusIos::Handler
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(InputDeviceTouchIos, AZ::SystemAllocator);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] inputDevice Reference to the input device being implemented
        InputDeviceTouchIos(InputDeviceTouch& inputDevice);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Destructor
        ~InputDeviceTouchIos() override;

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceTouch::Implementation::IsConnected
        bool IsConnected() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceTouch::Implementation::TickInputDevice
        void TickInputDevice() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        ///@{
        //! Process raw touch events (that will always be dispatched on the main thread)
        //! \param[in] uiTouch The raw touch data
        void OnRawTouchEventBegan(const UITouch* uiTouch) override;
        void OnRawTouchEventMoved(const UITouch* uiTouch) override;
        void OnRawTouchEventEnded(const UITouch* uiTouch) override;
        ///@}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Convenience function to initialize a raw touch event
        static RawTouchEvent InitRawTouchEvent(const UITouch* touch, uint32_t index);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! ios does not provide us with the index of a touch, but it does persist UITouch objects
        //! throughout a multi-touch sequence, so we can keep track of the touch indices ourselves.
        AZStd::vector<const UITouch*> m_activeTouches;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceTouchIos::InputDeviceTouchIos(InputDeviceTouch& inputDevice)
        : InputDeviceTouch::Implementation(inputDevice)
        , m_activeTouches()
    {
        // The maximum number of active touches tracked by ios is actually device dependent, and at
        // this time appears to be 5 for iPhone/iPodTouch and 11 for iPad. There is no API to query
        // or set this, but ten seems more than sufficient for most applications, especially games.
        m_activeTouches.resize(InputDeviceTouch::Touch::All.size());

        RawInputNotificationBusIos::Handler::BusConnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceTouchIos::~InputDeviceTouchIos()
    {
        RawInputNotificationBusIos::Handler::BusDisconnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceTouchIos::IsConnected() const
    {
        // Touch input is always available on ios
        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceTouchIos::TickInputDevice()
    {
        // The ios event loop has just been pumped in InputSystemComponentIos::PreTickInputDevices,
        // so we now just need to process any raw events that have been queued since the last frame
        ProcessRawEventQueues();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceTouchIos::OnRawTouchEventBegan(const UITouch* uiTouch)
    {
        for (uint32_t i = 0; i < InputDeviceTouch::Touch::All.size(); ++i)
        {
            // Use the first available index.
            if (m_activeTouches[i] == nullptr)
            {
                m_activeTouches[i] = uiTouch;

                const RawTouchEvent rawTouchEvent = InitRawTouchEvent(uiTouch, i);
                QueueRawTouchEvent(rawTouchEvent);

                break;
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceTouchIos::OnRawTouchEventMoved(const UITouch* uiTouch)
    {
        for (uint32_t i = 0; i < InputDeviceTouch::Touch::All.size(); ++i)
        {
            if (m_activeTouches[i] == uiTouch)
            {
                const RawTouchEvent rawTouchEvent = InitRawTouchEvent(uiTouch, i);
                QueueRawTouchEvent(rawTouchEvent);

                break;
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceTouchIos::OnRawTouchEventEnded(const UITouch* uiTouch)
    {
        for (uint32_t i = 0; i < InputDeviceTouch::Touch::All.size(); ++i)
        {
            if (m_activeTouches[i] == uiTouch)
            {
                const RawTouchEvent rawTouchEvent = InitRawTouchEvent(uiTouch, i);
                QueueRawTouchEvent(rawTouchEvent);

                m_activeTouches[i] = nullptr;

                break;
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceTouch::Implementation::RawTouchEvent InputDeviceTouchIos::InitRawTouchEvent(
        const UITouch* touch,
        uint32_t index)
    {
        CGPoint touchLocation = [touch locationInView: touch.view];
        CGSize viewSize = [touch.view bounds].size;

        const float normalizedLocationX = touchLocation.x / viewSize.width;
        const float normalizedLocationY = touchLocation.y / viewSize.height;

        const bool supportsForceTouch = UIScreen.mainScreen.traitCollection.forceTouchCapability == UIForceTouchCapabilityAvailable;
        float pressure = supportsForceTouch ? touch.force / touch.maximumPossibleForce : 1.0f;

        RawTouchEvent::State state = RawTouchEvent::State::Began;
        switch (touch.phase)
        {
            case UITouchPhaseBegan:
            {
                state = RawTouchEvent::State::Began;
            }
            break;
            case UITouchPhaseMoved:
            case UITouchPhaseStationary: // Should never happen but if so treat it the same as moved
            {
                state = RawTouchEvent::State::Moved;
            }
            break;
            case UITouchPhaseEnded:
            case UITouchPhaseCancelled: // Should never happen but if so treat it the same as ended
            {
                state = RawTouchEvent::State::Ended;
                pressure = 0.0f;
            }
            break;
        }

        return RawTouchEvent(normalizedLocationX,
                             normalizedLocationY,
                             pressure,
                             index,
                             state);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    class IosDeviceTouchImplFactory
        : public InputDeviceTouch::ImplementationFactory
    {
    public:
        AZStd::unique_ptr<InputDeviceTouch::Implementation> Create(InputDeviceTouch& inputDevice) override
        {
            return AZStd::make_unique<InputDeviceTouchIos>(inputDevice);    
        }
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void NativeUISystemComponent::InitializeDeviceTouchImplentationFactory()
    {
        m_deviceTouchImplFactory = AZStd::make_unique<IosDeviceTouchImplFactory>();
        AZ::Interface<InputDeviceTouch::ImplementationFactory>::Register(m_deviceTouchImplFactory.get());
    }
} // namespace AzFramework
