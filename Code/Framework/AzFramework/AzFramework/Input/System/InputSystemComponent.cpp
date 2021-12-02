/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Input/System/InputSystemComponent.h>

#include <AzFramework/Input/Buses/Notifications/InputSystemNotificationBus.h>

#include <AzFramework/Input/Devices/Gamepad/InputDeviceGamepad.h>
#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>
#include <AzFramework/Input/Devices/Motion/InputDeviceMotion.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzFramework/Input/Devices/Touch/InputDeviceTouch.h>
#include <AzFramework/Input/Devices/VirtualKeyboard/InputDeviceVirtualKeyboard.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Settings/SettingsRegistry.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZStd::vector<AZStd::string> GetAllMotionChannelNames()
    {
        AZStd::vector<AZStd::string> allMotionChannelNames;
        for (AZ::u32 i = 0; i < InputDeviceMotion::Acceleration::All.size(); ++i)
        {
            const InputChannelId& channelId = InputDeviceMotion::Acceleration::All[i];
            allMotionChannelNames.push_back(channelId.GetName());
        }
        for (AZ::u32 i = 0; i < InputDeviceMotion::RotationRate::All.size(); ++i)
        {
            const InputChannelId& channelId = InputDeviceMotion::RotationRate::All[i];
            allMotionChannelNames.push_back(channelId.GetName());
        }
        for (AZ::u32 i = 0; i < InputDeviceMotion::MagneticField::All.size(); ++i)
        {
            const InputChannelId& channelId = InputDeviceMotion::MagneticField::All[i];
            allMotionChannelNames.push_back(channelId.GetName());
        }
        for (AZ::u32 i = 0; i < InputDeviceMotion::Orientation::All.size(); ++i)
        {
            const InputChannelId& channelId = InputDeviceMotion::Orientation::All[i];
            allMotionChannelNames.push_back(channelId.GetName());
        }
        return allMotionChannelNames;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    class InputSystemNotificationBusBehaviorHandler
        : public InputSystemNotificationBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        AZ_EBUS_BEHAVIOR_BINDER(InputSystemNotificationBusBehaviorHandler, "{2F3417A3-41FD-4FBB-B0B6-F154F068F4F8}", AZ::SystemAllocator
            , OnPreInputUpdate
            , OnPostInputUpdate
        );

        ////////////////////////////////////////////////////////////////////////////////////////////
        void OnPreInputUpdate() override
        {
            Call(FN_OnPreInputUpdate);
        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        void OnPostInputUpdate() override
        {
            Call(FN_OnPostInputUpdate);
        }
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<InputSystemComponent, AZ::Component>()
                ->Version(1)
                ->Field("MouseMovementSampleRateHertz", &InputSystemComponent::m_mouseMovementSampleRateHertz)
                ->Field("GamepadsEnabled", &InputSystemComponent::m_gamepadsEnabled)
                ->Field("KeyboardEnabled", &InputSystemComponent::m_keyboardEnabled)
                ->Field("MotionEnabled", &InputSystemComponent::m_motionEnabled)
                ->Field("MouseEnabled", &InputSystemComponent::m_mouseEnabled)
                ->Field("TouchEnabled", &InputSystemComponent::m_touchEnabled)
                ->Field("VirtualKeyboardEnabled", &InputSystemComponent::m_virtualKeyboardEnabled)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<InputSystemComponent>(
                    "Input System", "Controls which core input devices are made available")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Engine")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                    ->DataElement(AZ::Edit::UIHandlers::SpinBox, &InputSystemComponent::m_mouseMovementSampleRateHertz,
                        "Mouse Movement Sample Rate", "The mouse movement sample rate in Hertz (cycles per second), which directly\n"
                                                      "correlates to the max number of mouse movement events dispatched each frame.\n"
                                                      "Increasing this may improve responsiveness, but could impact performance.\n"
                                                      "Decreasing it may improve performance, but could make it less responsive.")
                        ->Attribute(AZ::Edit::Attributes::Min, 1)
                    ->DataElement(AZ::Edit::UIHandlers::SpinBox, &InputSystemComponent::m_gamepadsEnabled,
                        "Gamepads", "The number of game-pads enabled.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0)
                        ->Attribute(AZ::Edit::Attributes::Max, 8)
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &InputSystemComponent::m_keyboardEnabled,
                        "Keyboard", "Is keyboard input enabled?")
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &InputSystemComponent::m_motionEnabled,
                        "Motion", "Is motion input enabled?")
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &InputSystemComponent::m_mouseEnabled,
                        "Mouse", "Is mouse input enabled?")
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &InputSystemComponent::m_touchEnabled,
                        "Touch", "Is touch enabled?")
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &InputSystemComponent::m_virtualKeyboardEnabled,
                        "Virtual Keyboard", "Is the virtual keyboard enabled?")
                ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<InputSystemNotificationBus>("InputSystemNotificationBus")
                ->Attribute(AZ::Script::Attributes::Category, "Input")
                ->Handler<InputSystemNotificationBusBehaviorHandler>()
                ;

            behaviorContext->EBus<InputSystemRequestBus>("InputSystemRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "Input")
                ->Event("RecreateEnabledInputDevices", &InputSystemRequestBus::Events::RecreateEnabledInputDevices)
                ;
        }

        InputChannelId::Reflect(context);
        InputDeviceId::Reflect(context);
        InputChannel::Reflect(context);
        InputDevice::Reflect(context);
        LocalUserIdReflect(context);

        InputDeviceGamepad::Reflect(context);
        InputDeviceKeyboard::Reflect(context);
        InputDeviceMotion::Reflect(context);
        InputDeviceMouse::Reflect(context);
        InputDeviceTouch::Reflect(context);
        InputDeviceVirtualKeyboard::Reflect(context);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("InputSystemService", 0x5438d51a));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("InputSystemService", 0x5438d51a));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputSystemComponent::InputSystemComponent()
        : m_gamepads()
        , m_keyboard()
        , m_motion()
        , m_mouse()
        , m_touch()
        , m_virtualKeyboard()
        , m_mouseMovementSampleRateHertz(InputDeviceMouse::MovementSampleRateDefault)
        , m_gamepadsEnabled(4)
        , m_keyboardEnabled(true)
        , m_motionEnabled(true)
        , m_mouseEnabled(true)
        , m_touchEnabled(true)
        , m_virtualKeyboardEnabled(true)
        , m_currentlyUpdatingInputDevices(false)
        , m_recreateInputDevicesAfterUpdate(false)
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputSystemComponent::~InputSystemComponent()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputSystemComponent::Activate()
    {
        const auto* settingsRegistry = AZ::SettingsRegistry::Get();
        if (settingsRegistry)
        {
            AZ::u64 value = 0;
            if (settingsRegistry->Get(value, "/O3DE/InputSystem/MouseMovementSampleRateHertz"))
            {
                m_mouseMovementSampleRateHertz = aznumeric_caster(value);
            }
            if (settingsRegistry->Get(value, "/O3DE/InputSystem/GamepadsEnabled"))
            {
                m_gamepadsEnabled = aznumeric_caster(value);
            }
            settingsRegistry->Get(m_keyboardEnabled, "/O3DE/InputSystem/KeyboardEnabled");
            settingsRegistry->Get(m_motionEnabled, "/O3DE/InputSystem/MotionEnabled");
            settingsRegistry->Get(m_mouseEnabled, "/O3DE/InputSystem/MouseEnabled");
            settingsRegistry->Get(m_touchEnabled, "/O3DE/InputSystem/TouchEnabled");
            settingsRegistry->Get(m_virtualKeyboardEnabled, "/O3DE/InputSystem/VirtualKeyboardEnabled");
        }

        // Create all enabled input devices
        CreateEnabledInputDevices();

        InputSystemRequestBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputSystemComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
        InputSystemRequestBus::Handler::BusDisconnect();

        // Destroy all enabled input devices
        DestroyEnabledInputDevices();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    int InputSystemComponent::GetTickOrder()
    {
        return AZ::ComponentTickBus::TICK_INPUT;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputSystemComponent::OnTick(float /*deltaTime*/, AZ::ScriptTimePoint /*scriptTimePoint*/)
    {
        TickInput();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputSystemComponent::TickInput()
    {
        InputSystemNotificationBus::Broadcast(&InputSystemNotifications::OnPreInputUpdate);
        m_currentlyUpdatingInputDevices = true;
        InputDeviceRequestBus::Broadcast(&InputDeviceRequests::TickInputDevice);
        m_currentlyUpdatingInputDevices = false;
        InputSystemNotificationBus::Broadcast(&InputSystemNotifications::OnPostInputUpdate);

        if (m_recreateInputDevicesAfterUpdate)
        {
            CreateEnabledInputDevices();
            m_recreateInputDevicesAfterUpdate = false;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputSystemComponent::RecreateEnabledInputDevices()
    {
        if (m_currentlyUpdatingInputDevices)
        {
            // Delay the request until we've finished updating to protect against getting called in
            // response to an input event, in which case calling CreateEnabledInputDevices here will
            // cause a crash (when the stack unwinds back up to the device which dispatced the event
            // but was then destroyed). An unlikely (but possible) scenario we must protect against.
            m_recreateInputDevicesAfterUpdate = true;
        }
        else
        {
            CreateEnabledInputDevices();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputSystemComponent::CreateEnabledInputDevices()
    {
        const AZ::u32 maxSupportedGamepads = InputDeviceGamepad::GetMaxSupportedGamepads();
        m_gamepadsEnabled = AZStd::clamp<AZ::u32>(m_gamepadsEnabled, 0, maxSupportedGamepads);

        DestroyEnabledInputDevices();

        m_gamepads.resize(m_gamepadsEnabled);
        for (AZ::u32 i = 0; i < m_gamepadsEnabled; ++i)
        {
            m_gamepads[i].reset(aznew InputDeviceGamepad(i));
        }

        m_keyboard.reset(m_keyboardEnabled ? aznew InputDeviceKeyboard() : nullptr);
        m_motion.reset(m_motionEnabled ? aznew InputDeviceMotion() : nullptr);
        m_mouse.reset(m_mouseEnabled ? aznew InputDeviceMouse() : nullptr);
        m_touch.reset(m_touchEnabled ? aznew InputDeviceTouch() : nullptr);
        m_virtualKeyboard.reset(m_virtualKeyboardEnabled ? aznew InputDeviceVirtualKeyboard() : nullptr);

        if (m_mouse)
        {
            m_mouse->SetRawMovementSampleRate(m_mouseMovementSampleRateHertz);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputSystemComponent::DestroyEnabledInputDevices()
    {
        m_virtualKeyboard.reset(nullptr);
        m_touch.reset(nullptr);
        m_mouse.reset(nullptr);
        m_motion.reset(nullptr);
        m_keyboard.reset(nullptr);
        m_gamepads.clear();
    }
} // namespace AzFramework
