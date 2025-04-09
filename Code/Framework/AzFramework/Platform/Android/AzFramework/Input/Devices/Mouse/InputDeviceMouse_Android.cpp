/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Components/NativeUISystemComponent.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzFramework/Input/Buses/Notifications/RawInputNotificationBus_Platform.h>

#include <AzCore/Android/JNI/Object.h>

#include <AzCore/Android/ApiLevel.h>
#include <AzCore/Android/Utils.h>

#include <android/input.h>


namespace
{
    // the following values were added in later APIs however their native counterparts
    // were only added to the unified headers in NDK r14+

    // dedicated mouse button events were added in API 23
    const int EVENT_ACTION_BUTTON_PRESS   = 11;  // AMOTION_EVENT_ACTION_BUTTON_PRESS
    const int EVENT_ACTION_BUTTON_RELEASE = 12;  // AMOTION_EVENT_ACTION_BUTTON_RELEASE

    // relative pointer queries were added in API 24
    const int EVENT_AXIS_RELATIVE_X = 27; // AMOTION_EVENT_AXIS_RELATIVE_X
    const int EVENT_AXIS_RELATIVE_Y = 28; // AMOTION_EVENT_AXIS_RELATIVE_Y


    class RawMouseConnectionNotificationsAndroid
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

        virtual ~RawMouseConnectionNotificationsAndroid() = default;

        virtual void OnMouseConnected() = 0;
        virtual void OnMouseDisconnected() = 0;
    };

    using RawMouseConnectionNotificationsBusAndroid = AZ::EBus<RawMouseConnectionNotificationsAndroid>;


    void JNI_OnMouseConnected(JNIEnv* jniEnv, jobject objectRef)
    {
        RawMouseConnectionNotificationsBusAndroid::Broadcast(&RawMouseConnectionNotificationsAndroid::OnMouseConnected);
    }

    void JNI_OnMouseDisconnected(JNIEnv* jniEnv, jobject objectRef)
    {
        RawMouseConnectionNotificationsBusAndroid::Broadcast(&RawMouseConnectionNotificationsAndroid::OnMouseDisconnected);
    }
}


namespace AzFramework
{
    //! Platform specific implementation for Android physical mouse input devices.  This
    //! includes devices connected through USB or Bluetooth.
    class InputDeviceMouseAndroid
        : public InputDeviceMouse::Implementation
        , public RawInputNotificationBusAndroid::Handler
        , public RawMouseConnectionNotificationsBusAndroid::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(InputDeviceMouseAndroid, AZ::SystemAllocator);


        InputDeviceMouseAndroid(InputDeviceMouse& inputDevice);
        ~InputDeviceMouseAndroid() override;


        //! \ref AzFramework::RawInputNotificationsAndroid::OnRawInputEvent
        void OnRawInputEvent(const AInputEvent* rawInputEvent) override;


    private:
        //! \ref AzFramework::InputDeviceMouse::Implementation::IsConnected
        bool IsConnected() const override { return m_isConnected; }

        //! \ref AzFramework::InputDeviceMouse::Implementation::SetSystemCursorState
        void SetSystemCursorState(SystemCursorState systemCursorState) override;

        //! \ref AzFramework::InputDeviceMouse::Implementation::GetSystemCursorState
        SystemCursorState GetSystemCursorState() const override;

        //! \ref AzFramework::InputDeviceMouse::Implementation::SetSystemCursorPositionNormalized
        void SetSystemCursorPositionNormalized(AZ::Vector2 positionNormalized) override;

        //! \ref AzFramework::InputDeviceMouse::Implementation::GetSystemCursorPositionNormalized
        AZ::Vector2 GetSystemCursorPositionNormalized() const override;

        //! \ref AzFramework::InputDeviceMouse::Implementation::TickInputDevice
        void TickInputDevice() override;

        //!@{
        //! \brief Connection state callbacks from java
        void OnMouseConnected() override { m_isConnected = true; }
        void OnMouseDisconnected() override { m_isConnected = false; }
        //!@}

        //! Special case processing of key events for certain devices that treat some mouse buttons
        //! as simulated key events
        void HandleKeyEvent(const AInputEvent* rawInputEvent);

        //! Helper for processing the common mouse button event processing
        void ProcessMouseButtonEvent(int buttonState, bool isPressed);


        AZStd::unique_ptr<AZ::Android::JNI::Object> m_mouseDevice; //!< Java interface for getting connection state info

        AZ::Vector2 m_mouseWindowPixelPosition; //!< Cached location of the mouse, in pixel window space
        int m_mouseButtonState; //!< Cached flags of which mouse buttons are currently active

        volatile bool m_isConnected; //!< Cached connection state, ie. at least 1 mouse is connected
    };


    InputDeviceMouseAndroid::InputDeviceMouseAndroid(InputDeviceMouse& inputDevice)
        : InputDeviceMouse::Implementation(inputDevice)
        , m_mouseWindowPixelPosition(AZ::Vector2::CreateZero())
        , m_mouseButtonState(0)
        , m_isConnected(false)
    {
        // Initialize the mouse device handler
        m_mouseDevice.reset(aznew AZ::Android::JNI::Object("com/amazon/lumberyard/input/MouseDevice"));
        m_mouseDevice->RegisterMethod("IsConnected", "()Z");
        m_mouseDevice->RegisterNativeMethods(
        {
            { "OnMouseConnected", "()V", (void*)JNI_OnMouseConnected },
            { "OnMouseDisconnected", "()V", (void*)JNI_OnMouseDisconnected }
        });

        // create the java instance
        bool ret = m_mouseDevice->CreateInstance("(Landroid/app/Activity;)V", AZ::Android::Utils::GetActivityRef());
        AZ_Assert(ret, "Failed to create the MouseDevice Java instance.");

        if (ret)
        {
            m_isConnected = m_mouseDevice->InvokeBooleanMethod("IsConnected");
        }

        RawInputNotificationBusAndroid::Handler::BusConnect();
        RawMouseConnectionNotificationsBusAndroid::Handler::BusConnect();
    }

    InputDeviceMouseAndroid::~InputDeviceMouseAndroid()
    {
        RawMouseConnectionNotificationsBusAndroid::Handler::BusDisconnect();
        RawInputNotificationBusAndroid::Handler::BusDisconnect();
    }


    void InputDeviceMouseAndroid::OnRawInputEvent(const AInputEvent* rawInputEvent)
    {
        if (!m_isConnected)
        {
            return;
        }

        // special case for some devices that treat the right click as a back button event
        const int eventType = AInputEvent_getType(rawInputEvent);
        if (eventType == AINPUT_EVENT_TYPE_KEY)
        {
            HandleKeyEvent(rawInputEvent);
            return;
        }

        // Get the action code and pointer index
        const int action = AMotionEvent_getAction(rawInputEvent);
        const int actionCode = (action & AMOTION_EVENT_ACTION_MASK);
        const int pointerIndex = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;

        // ensure we are only dispatching pure mouse events and not simulated ones
        const int toolType = AMotionEvent_getToolType(rawInputEvent, pointerIndex);
        if (toolType != AMOTION_EVENT_TOOL_TYPE_MOUSE)
        {
            return;
        }

        const AZ::Android::ApiLevel sdkVersion = AZ::Android::GetRuntimeApiLevel();

        const int buttonState = AMotionEvent_getButtonState(rawInputEvent);
        switch (actionCode)
        {
            case AMOTION_EVENT_ACTION_POINTER_DOWN:
                // guard against duplicate events when running on API 23 or above
                if (sdkVersion >= AZ::Android::ApiLevel::Marshmallow)
                {
                    break;
                }
                // fallthrough
            case EVENT_ACTION_BUTTON_PRESS:
            {
                // extract the mouse button that was added to the flags
                int newButtonState = buttonState & ~m_mouseButtonState;
                ProcessMouseButtonEvent(newButtonState, true);
                m_mouseButtonState = buttonState;
                break;
            }

            case AMOTION_EVENT_ACTION_POINTER_UP:
                // guard against duplicate events when running on API 23 or above
                if (sdkVersion >= AZ::Android::ApiLevel::Marshmallow)
                {
                    break;
                }
                // fallthrough
            case EVENT_ACTION_BUTTON_RELEASE:
            {
                // extract the mouse button that was removed from the flags
                int removedButtonState = m_mouseButtonState & ~buttonState;
                ProcessMouseButtonEvent(removedButtonState, false);
                m_mouseButtonState = buttonState;
                break;
            }

            case AMOTION_EVENT_ACTION_SCROLL:
            {
                const float verticalScroll = AMotionEvent_getAxisValue(rawInputEvent, AMOTION_EVENT_AXIS_VSCROLL, pointerIndex);
                QueueRawMovementEvent(InputDeviceMouse::Movement::Z, verticalScroll);
                break;
            }

            case AMOTION_EVENT_ACTION_MOVE:
            case AMOTION_EVENT_ACTION_HOVER_MOVE:
            {
                const AZ::Vector2 mousePostion {
                    AMotionEvent_getX(rawInputEvent, pointerIndex),
                    AMotionEvent_getY(rawInputEvent, pointerIndex)
                };

                AZ::Vector2 mouseDeltaPostion(AZ::Vector2::CreateZero());
                if (sdkVersion >= AZ::Android::ApiLevel::Nougat)
                {
                    mouseDeltaPostion.SetX(AMotionEvent_getAxisValue(rawInputEvent, EVENT_AXIS_RELATIVE_X, pointerIndex));
                    mouseDeltaPostion.SetY(AMotionEvent_getAxisValue(rawInputEvent, EVENT_AXIS_RELATIVE_Y, pointerIndex));
                }
                else
                {
                    mouseDeltaPostion  = mousePostion - m_mouseWindowPixelPosition;
                }

                QueueRawMovementEvent(InputDeviceMouse::Movement::X, mouseDeltaPostion.GetX());
                QueueRawMovementEvent(InputDeviceMouse::Movement::Y, mouseDeltaPostion.GetY());

                m_mouseWindowPixelPosition = mousePostion;
                break;
            }

            default:
                // do nothing
                break;
        }
    }


    void InputDeviceMouseAndroid::SetSystemCursorState(SystemCursorState systemCursorState)
    {
        if (m_isConnected)
        {
            AZ_WarningOnce("InputDeviceMouseAndroid", false, "Calls to SetSystemCursorState are unsupported on Android.");
        }
    }

    SystemCursorState InputDeviceMouseAndroid::GetSystemCursorState() const
    {
        // Returning the correct state here is a bit tricky because the system will auto show and hide
        // the cursor based on usage of the physical mouse.  The application has no control over
        // the visibily of the cursor, nor do we have control of constraining the cursor to inside the
        // applicaiton window.  Always returning UnconstrainedAndVisible is going to be the closest
        // to accurate value we can send when a physical mouse is connected.
        return (m_isConnected ? SystemCursorState::UnconstrainedAndVisible : SystemCursorState::Unknown);
    }

    void InputDeviceMouseAndroid::SetSystemCursorPositionNormalized(AZ::Vector2 positionNormalized)
    {
        if (m_isConnected)
        {
            AZ_WarningOnce("InputDeviceMouseAndroid", false, "Calls to SetSystemCursorPositionNormalized are unsupported on Android.");
        }
    }

    AZ::Vector2 InputDeviceMouseAndroid::GetSystemCursorPositionNormalized() const
    {
        int windowWidth = 0;
        int windowHeight = 0;
        if (!AZ::Android::Utils::GetWindowSize(windowWidth, windowHeight))
        {
            AZ_ErrorOnce("InputDeviceMouseAndroid", false, "Unable to get the window size, the mouse location will NOT be normalized.");
            windowWidth = 1;
            windowHeight = 1;
        }

        return AZ::Vector2(m_mouseWindowPixelPosition.GetX() / static_cast<float>(windowWidth),
                           m_mouseWindowPixelPosition.GetY() / static_cast<float>(windowHeight));
    }

    void InputDeviceMouseAndroid::TickInputDevice()
    {
        if (m_isConnected)
        {
            ProcessRawEventQueues();
        }
    }


    void InputDeviceMouseAndroid::HandleKeyEvent(const AInputEvent* rawInputEvent)
    {
        const int inputSource = AInputEvent_getSource(rawInputEvent);
        if ((inputSource & AINPUT_SOURCE_MOUSE) == AINPUT_SOURCE_MOUSE)
        {
            const int keyCode = AKeyEvent_getKeyCode(rawInputEvent);
            if (keyCode == AKEYCODE_BACK)
            {
                const int keyAction = AKeyEvent_getAction(rawInputEvent);
                switch (keyAction)
                {
                    case AKEY_EVENT_ACTION_DOWN:
                        // guard against duplicate events when holding down the button
                        if (AKeyEvent_getRepeatCount(rawInputEvent) == 0)
                        {
                            QueueRawButtonEvent(InputDeviceMouse::Button::Right, true);
                        }
                        break;

                    case AKEY_EVENT_ACTION_UP:
                        QueueRawButtonEvent(InputDeviceMouse::Button::Right, false);
                        break;

                    default:
                        // do nothing
                        break;
                }
            }
        }
    }

    void InputDeviceMouseAndroid::ProcessMouseButtonEvent(int buttonState, bool isPressed)
    {
        if ((buttonState & AMOTION_EVENT_BUTTON_PRIMARY) == AMOTION_EVENT_BUTTON_PRIMARY)
        {
            QueueRawButtonEvent(InputDeviceMouse::Button::Left, isPressed);
        }
        else if ((buttonState & AMOTION_EVENT_BUTTON_SECONDARY) == AMOTION_EVENT_BUTTON_SECONDARY)
        {
            QueueRawButtonEvent(InputDeviceMouse::Button::Right, isPressed);
        }
        else if ((buttonState & AMOTION_EVENT_BUTTON_TERTIARY) == AMOTION_EVENT_BUTTON_TERTIARY)
        {
            QueueRawButtonEvent(InputDeviceMouse::Button::Middle, isPressed);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    class AndroidDeviceMouseImplFactory
        : public InputDeviceMouse::ImplementationFactory
    {
    public:
        AZStd::unique_ptr<InputDeviceMouse::Implementation> Create(InputDeviceMouse& inputDevice) override
        {
            return AZStd::make_unique<InputDeviceMouseAndroid>(inputDevice);
        }
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void NativeUISystemComponent::InitializeDeviceMouseImplentationFactory()
    {
        m_deviceMouseImplFactory = AZStd::make_unique<AndroidDeviceMouseImplFactory>();
        AZ::Interface<InputDeviceMouse::ImplementationFactory>::Register(m_deviceMouseImplFactory.get());
    }
} // namespace AzFramework
