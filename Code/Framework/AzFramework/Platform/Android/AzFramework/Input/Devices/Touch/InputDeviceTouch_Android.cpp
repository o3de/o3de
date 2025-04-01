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

#include <AzCore/Android/Utils.h>
#include <AzCore/std/parallel/mutex.h>

#include <android/input.h>
#include <android/keycodes.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Platform specific implementation for android touch input devices
    class InputDeviceTouchAndroid : public InputDeviceTouch::Implementation
                                  , public RawInputNotificationBusAndroid::Handler
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(InputDeviceTouchAndroid, AZ::SystemAllocator);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] inputDevice Reference to the input device being implemented
        InputDeviceTouchAndroid(InputDeviceTouch& inputDevice);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Destructor
        ~InputDeviceTouchAndroid() override;

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceTouch::Implementation::IsConnected
        bool IsConnected() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceTouch::Implementation::TickInputDevice
        void TickInputDevice() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::RawInputNotificationsAndroid::OnRawInputEvent
        void OnRawInputEvent(const AInputEvent* rawInputEvent) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Convenience function to process raw touch events
        //! \param[in] rawInputEvent The raw input event data
        //! \param[in] pointerIndex The index of the touch event
        //! \param[in] rawTouchState The state of the touch event
        void OnRawTouchEvent(const AInputEvent* rawInputEvent,
                             int pointerIndex,
                             RawTouchEvent::State rawTouchState);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Raw input events on android can (and likely will) be dispatched from a thread other than
        //! main, so we can't immediately call InputDeviceTouch::Implementation::QueueRawTouchEvent.
        //! Instead, we'll store them in m_threadAwareRawTouchEvents then process in TickInputDevice
        //! on the main thread, ensuring all access is locked using m_threadAwareRawTouchEventsMutex.
        ///@{
        AZStd::vector<RawTouchEvent> m_threadAwareRawTouchEvents;
        AZStd::mutex                 m_threadAwareRawTouchEventsMutex;
        ///@}
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceTouchAndroid::InputDeviceTouchAndroid(InputDeviceTouch& inputDevice)
        : InputDeviceTouch::Implementation(inputDevice)
        , m_threadAwareRawTouchEvents()
        , m_threadAwareRawTouchEventsMutex()
    {
        RawInputNotificationBusAndroid::Handler::BusConnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceTouchAndroid::~InputDeviceTouchAndroid()
    {
        RawInputNotificationBusAndroid::Handler::BusDisconnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceTouchAndroid::IsConnected() const
    {
        // Touch input is always available on android
        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceTouchAndroid::TickInputDevice()
    {
        // The input event loop is pumped by another thread on android so all raw input events this
        // frame have already been dispatched. But they are all queued until ProcessRawEventQueues
        // is called below so that all raw input events are processed at the same time every frame.

        // Because m_threadAwareRawTouchEvents is updated from another thread, but also needs to be
        // processed and cleared in this function, swapping it with an empty vector kills two birds
        // with one stone in a very efficient and elegant manner (kudos to scottr for the idea).
        AZStd::vector<RawTouchEvent> rawTouchEvents;
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_threadAwareRawTouchEventsMutex);
            rawTouchEvents.swap(m_threadAwareRawTouchEvents);
        }

        // Queue the raw touch events that were received over the last frame
        for (const RawTouchEvent& rawTouchEvent : rawTouchEvents)
        {
            QueueRawTouchEvent(rawTouchEvent);
        }

        // Process the raw event queues once each frame
        ProcessRawEventQueues();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceTouchAndroid::OnRawInputEvent(const AInputEvent* rawInputEvent)
    {
        // Reference: https://developer.android.com/ndk/reference/group___input.html

        // Discard non-touch events
        const int eventType = AInputEvent_getType(rawInputEvent);
        if (eventType != AINPUT_EVENT_TYPE_MOTION)
        {
            return;
        }

        // Get the action code and pointer index
        const int action = AMotionEvent_getAction(rawInputEvent);
        const int actionCode = (action & AMOTION_EVENT_ACTION_MASK);
        const int pointerIndex = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;

        switch (actionCode)
        {
            case AMOTION_EVENT_ACTION_DOWN:
            case AMOTION_EVENT_ACTION_POINTER_DOWN:
            {
                OnRawTouchEvent(rawInputEvent, pointerIndex, RawTouchEvent::State::Began);
            }
            break;
            case AMOTION_EVENT_ACTION_MOVE:
            {
                // Android doesn't send individual move events for each separate touch like it does
                // for down and up events, instead sending them all as part of the same input event.
                const size_t pointerCount = AMotionEvent_getPointerCount(rawInputEvent);
                for (int i = 0; i < pointerCount; ++i)
                {
                    OnRawTouchEvent(rawInputEvent, i, RawTouchEvent::State::Moved);
                }
            }
            break;
            case AMOTION_EVENT_ACTION_UP:
            case AMOTION_EVENT_ACTION_POINTER_UP:
            case AMOTION_EVENT_ACTION_CANCEL:
            {
                OnRawTouchEvent(rawInputEvent, pointerIndex, RawTouchEvent::State::Ended);
            }
            break;
            default:
            {
                return;
            }
            break;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceTouchAndroid::OnRawTouchEvent(const AInputEvent* rawInputEvent,
                                                  int pointerIndex,
                                                  RawTouchEvent::State rawTouchState)
    {
        // Allow simulated touch events (generated by "adb shell monkey") or real finger touch events.
        // (but not stylus, mouse or eraser)
        const int toolType = AMotionEvent_getToolType(rawInputEvent, pointerIndex);
        if (toolType != AMOTION_EVENT_TOOL_TYPE_FINGER && toolType != AMOTION_EVENT_TOOL_TYPE_UNKNOWN)
        {
            return;
        }

        // Get the rest of the touch event data
        const float x = AMotionEvent_getX(rawInputEvent, pointerIndex);
        const float y = AMotionEvent_getY(rawInputEvent, pointerIndex);
        float pressure = AMotionEvent_getPressure(rawInputEvent, pointerIndex);
        const int pointerId = AMotionEvent_getPointerId(rawInputEvent, pointerIndex);

        // The simulated events always have a pressure of zero. An analog input channel is only active
        // if the value is != 0.0f. Touch input was implemented as an analog input channel because both
        // iOS and Android report touch input with a pressure value, and for anything that doesn't report
        // a pressure value you have to explicitly set it as 1.0f (or really anything != 0.0f) for 'active'
        // and 0.0f for not active.
        if (toolType == AMOTION_EVENT_TOOL_TYPE_UNKNOWN && pressure == 0.0f)
        {
            pressure = 1.0f;
        }

        // normalize the touch location
        int windowWidth = 0;
        int windowHeight = 0;
        if (!AZ::Android::Utils::GetWindowSize(windowWidth, windowHeight))
        {
            AZ_ErrorOnce("InputDeviceTouchAndroid", false, "Unable to get the window size, touch input may not behave correctly.");
            windowWidth = 1;
            windowHeight = 1;
        }

        // Push the raw touch event onto the thread safe queue for processing in TickInputDevice
        const RawTouchEvent rawTouchEvent(x / static_cast<float>(windowWidth),
                                          y / static_cast<float>(windowHeight),
                                          rawTouchState == RawTouchEvent::State::Ended ? 0.0f : pressure,
                                          pointerId,
                                          rawTouchState);
        AZStd::lock_guard<AZStd::mutex> lock(m_threadAwareRawTouchEventsMutex);
        m_threadAwareRawTouchEvents.push_back(rawTouchEvent);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    class AndroidDeviceTouchImplFactory
        : public InputDeviceTouch::ImplementationFactory
    {
    public:
        AZStd::unique_ptr<InputDeviceTouch::Implementation> Create(InputDeviceTouch& inputDevice) override
        {
            return AZStd::make_unique<InputDeviceTouchAndroid>(inputDevice);
        }
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void NativeUISystemComponent::InitializeDeviceTouchImplentationFactory()
    {
        m_deviceTouchImplFactory = AZStd::make_unique<AndroidDeviceTouchImplFactory>();
        AZ::Interface<InputDeviceTouch::ImplementationFactory>::Register(m_deviceTouchImplFactory.get());
    }
}
