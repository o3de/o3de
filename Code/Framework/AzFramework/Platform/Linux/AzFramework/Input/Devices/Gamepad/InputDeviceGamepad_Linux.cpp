/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformIncl.h>

#include <linux/input.h>
#include <linux/input-event-codes.h>

#include <AzCore/IO/SystemFile.h>
#include <AzCore/Time/ITime.h>
#include <AzFramework/Input/LibEVDevWrapper.h>
#include <AzFramework/Input/Devices/Gamepad/InputDeviceGamepad.h>
#include <AzFramework/Input/Devices/Gamepad/InputDeviceGamepad_Linux.h>

#include <AzCore/Debug/Trace.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace GamepadLinuxPrivate
{
    using namespace AzFramework;

    // We don't currently use a monitor to check for hot plugged devices, so we poll them
    // every this many milliseconds (but only if they are not currently plugged in).
    // The cost to check is just a file exists check, not very expensive, and 2 seconds seems to feel ok
    // in terms of plugging a gamepad in and seeing it become usable.  Disconnecting one is instantaneous.
    constexpr const AZ::TimeMs s_retryDelayAfterFail = AZ::TimeMs(2000);

    class InternalState
    {
        public:
            int m_fileDescriptor = -1; //! The file descriptor of the actual device opened that represents this gamepad.
            struct libevdev *m_evdevDevice = nullptr; //! the libevdev handle to the event device belonging to the above handle.

            // O3DE expects the dpad to show up as discrete buttons, but many gamepad devices treat it like a digital hat axis.
            float m_dPadHatMax = 0.0f;

            // O3DE expects stick axes values to be negative to positive, with 0 in the center, but some devices use 
            // values like 0-255 instead, with 128 in the center.  To account for this, we will pretend all devices are 
            // normalized, that is, between -1.0 and 1.0, with 0.0 being the center, by scaling and offsetting the values.
            // we'll set these values when the device is detected based on what the system says their range is.
            float m_axisLeftScale = 1.0f;
            float m_axisLeftOffset = 1.0f;
            float m_axisRightScale = 1.0f;
            float m_axisRightOffset = 1.0f;

            AZStd::shared_ptr<LibEVDevWrapper> m_libevdevModule; // This comes from the factory, one is shared by all gamepads

            InternalState(AZStd::shared_ptr<LibEVDevWrapper> libEVDev)
            : m_libevdevModule(libEVDev)
            {
            }

            void CloseDevice() // closes the device, but not the dynamic module handle
            {
                if (m_evdevDevice)
                {
                    // it is not possible to create this device without having a valid
                    // module handle and function pointers.  No need to check here.
                    m_libevdevModule->m_libevdev_free(m_evdevDevice);
                    m_evdevDevice = nullptr;
                }
                if (m_fileDescriptor != -1)
                {
                    close(m_fileDescriptor);
                    m_fileDescriptor = -1;
                }
            }

            ~InternalState()
            {
                m_libevdevModule.reset();
            }
    };

    // The O3DE Input system works by index rather than by path, but the linux evdev system works by id or path and 
    // devices don't necessarily have an index that starts at 0.  To bridge this gap, use the fact that each evdevice has two
    // device files which are actually symlinks to real devices inside /dev/input: 
    // for example,
    // /dev/input/by-path/xxxxxxxxxxxxxxxxx-event-joystick SYMLINK TO /dev/input/event7 -- this is the evdev device we'll read from
    // /dev/input/by-path/xxxxxxxxxxxxxxxxx-joystick SYMLINK TO /dev/input/js0 -- this is the old linux joystick system.
    // xxxxxxxxxxxxxxxxx is the same in both cases.  The js0 - jsN devices are the older joystick system (not evdev) 
    // but the association is useful here to map an index to a an evdev path.

    // This function takes an index (as in the /dev/input/jsN index, where N is the index) and will return an empty string
    // unless it finds a matching evdev device.  If it does find a matching evdev device, it will return the path to the 
    // evdev device (not the jsN device).
    AZStd::string FindJoystickForIndex(int index)
    {
        AZ::IO::FixedMaxPathString expectedPath = AZ::IO::FixedMaxPathString::format("/dev/input/js%d", index);
        if (!AZ::IO::SystemFile::Exists(expectedPath.c_str()))
        {
            return AZStd::string(); // no point in proceeding.
        }

        // the joystick device matching this index does exist, figure out which event device corresponds to it.
        // This is a semi-expensive operation but will only called if the above device exists and has not been
        // yet attached to the O3DE system, so should only happen once for each device as it is connected or when
        // O3DE starts up and it is already connected to begin with.
        AZStd::string outputStr;
        AZ::IO::SystemFile::FindFileCB cb = [&outputStr, index](const char* fileName, bool isFile) -> bool
        {
            if (!isFile)
            {
                return true; // continue iterating
            }

            // we use the joystick device to decode the symlink to the currently installed joystick, to get the index.
            // these will never have '-event' in their name but will have '-joystick' in their name.
            if ( (strstr(fileName, "-event") == nullptr) && (strstr(fileName, "-joystick") != nullptr) )
            {
                AZStd::string fullPath = (AZ::IO::Path("/dev/input/by-path") / fileName).AsPosix();
                // this is a real joystick node, not an event node  - it should have a symlink to the ../jsX device
                char symlinkPath[AZ_MAX_PATH_LEN];
                ssize_t symlinkLength = readlink(fullPath.c_str(), symlinkPath, AZ_MAX_PATH_LEN);
                // note that the result of readlink is not null terminated!
                if (symlinkLength > 0)
                {
                    // outputStr will have the format /dev/input/jsX WITHOUT A NULL TERMINATOR
                    AZStd::string_view symString(symlinkPath,symlinkLength);
                    AZStd::string::size_type jsValue = symString.rfind("js");
                    if (jsValue != AZStd::string::npos)
                    {
                        AZStd::string indexStr = symString.substr(jsValue + 2); // this null terminates since it copies it into a string.
                        char *tokenEnd = nullptr;
                        int joyIndex = static_cast<int>(strtoul(indexStr.data(), &tokenEnd, 10));
                        // tokenEnd will point to the null terminator if the string was a valid number, since the string should
                        // only contain the number and nothing else.
                        if ((tokenEnd)&&(*tokenEnd == '\0') && (joyIndex == index)) 
                        {
                            // this is the joystick we're looking for - but we want the event interface, not the joystick symlink
                            // so we need to back to the beginning and get the event interface instead.
                            // this is done by replacing the '-joystick' with '-event-joystick'
                            AZStd::string::size_type joyPos = fullPath.rfind("-joystick");
                            if (joyPos != AZStd::string::npos)
                            {
                                AZStd::string eventPath = fullPath.substr(0, joyPos) + "-event-joystick";
                                outputStr = eventPath;
                                return false; // stop iterating
                            }
                        }
                    }
                }
            }
            return true; // continue iterating
        };
        AZ::IO::SystemFile::FindFiles("/dev/input/by-path/*-joystick", cb);
        return outputStr;
    }

    // the NORTH_SOUTH_EAST_WEST buttons are actually the four right hand side
    // pad buttons, not the dpad buttons.  They are also known as BTN_A, BTN_B, BTN_X, BTN_Y.

    constexpr const AZ::u32 BTN_NORTH_MASK = 1 << 0;
    constexpr const AZ::u32 BTN_SOUTH_MASK = 1 << 1;
    constexpr const AZ::u32 BTN_WEST_MASK = 1 << 2;
    constexpr const AZ::u32 BTN_EAST_MASK = 1 << 3;
    constexpr const AZ::u32 BTN_START_MASK = 1 << 4;
    constexpr const AZ::u32 BTN_SELECT_MASK = 1 << 5;
    constexpr const AZ::u32 BTN_THUMBL_MASK = 1 << 6;
    constexpr const AZ::u32 BTN_THUMBR_MASK = 1 << 7;
    constexpr const AZ::u32 BTN_TL_MASK = 1 << 8;
    constexpr const AZ::u32 BTN_TR_MASK = 1 << 9;

    // note that some devices don't express the DPAD as actual buttons
    // but as an axis instead.  If we find a dpad axis, we will internally translate
    // these to button presses.
    constexpr const AZ::u32 BTN_DPAD_UP_MASK = 1 << 10;
    constexpr const AZ::u32 BTN_DPAD_DOWN_MASK = 1 << 11;
    constexpr const AZ::u32 BTN_DPAD_LEFT_MASK = 1 << 12;
    constexpr const AZ::u32 BTN_DPAD_RIGHT_MASK = 1 << 13;

    const AZ::u32 GetButtonMaskForActualButton(AZ::u32 buttonCode)
    {
        // the linux evdev system uses an integer index for each button, but
        // the O3DE system wants a bitmask.
        switch (buttonCode)
        {
            case BTN_NORTH:      return BTN_NORTH_MASK;
            case BTN_SOUTH:      return BTN_SOUTH_MASK;
            case BTN_WEST:       return BTN_WEST_MASK;
            case BTN_EAST:       return BTN_EAST_MASK;
            case BTN_START:      return BTN_START_MASK;
            case BTN_SELECT:     return BTN_SELECT_MASK;
            case BTN_THUMBL:     return BTN_THUMBL_MASK;
            case BTN_THUMBR:     return BTN_THUMBR_MASK;
            case BTN_TL:         return BTN_TL_MASK;
            case BTN_TR:         return BTN_TR_MASK;
            case BTN_DPAD_UP:    return BTN_DPAD_UP_MASK;
            case BTN_DPAD_DOWN:  return BTN_DPAD_DOWN_MASK;
            case BTN_DPAD_LEFT:  return BTN_DPAD_LEFT_MASK;
            case BTN_DPAD_RIGHT: return BTN_DPAD_RIGHT_MASK;
        }
        return 0;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Map of digital button ids keyed by their xinput button bitmask
    const AZStd::unordered_map<AZ::u32, const InputChannelId*> GetDigitalButtonIdByBitMaskMap()
    {
        const AZStd::unordered_map<AZ::u32, const InputChannelId*> map =
        {
            { BTN_DPAD_UP_MASK,    &InputDeviceGamepad::Button::DU },
            { BTN_DPAD_DOWN_MASK,  &InputDeviceGamepad::Button::DD },
            { BTN_DPAD_LEFT_MASK,  &InputDeviceGamepad::Button::DL },
            { BTN_DPAD_RIGHT_MASK, &InputDeviceGamepad::Button::DR },

            { BTN_START_MASK,      &InputDeviceGamepad::Button::Start },
            { BTN_SELECT_MASK,     &InputDeviceGamepad::Button::Select },

            { BTN_THUMBL_MASK,     &InputDeviceGamepad::Button::L3 },
            { BTN_THUMBR_MASK,     &InputDeviceGamepad::Button::R3 },

            { BTN_TL_MASK,         &InputDeviceGamepad::Button::L1 },
            { BTN_TR_MASK,         &InputDeviceGamepad::Button::R1 },

            { BTN_SOUTH_MASK,      &InputDeviceGamepad::Button::A },
            { BTN_EAST_MASK,       &InputDeviceGamepad::Button::B },
            { BTN_WEST_MASK,       &InputDeviceGamepad::Button::X },
            { BTN_NORTH_MASK,      &InputDeviceGamepad::Button::Y }
        };
        return map;
    }

} // end namespace GamepadLinuxPrivate

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    using namespace GamepadLinuxPrivate;

    InputDeviceGamepad::Implementation* InputDeviceGamepadLinux::Create(InputDeviceGamepad& inputDevice, AZStd::shared_ptr<AzFramework::LibEVDevWrapper> libevdevWrapper)
    {
        return aznew InputDeviceGamepadLinux(inputDevice, libevdevWrapper);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceGamepadLinux::InputDeviceGamepadLinux(InputDeviceGamepad& inputDevice, 
    AZStd::shared_ptr<AzFramework::LibEVDevWrapper> libevdevWrapper)
        : InputDeviceGamepad::Implementation(inputDevice)
        , m_rawGamepadState(GetDigitalButtonIdByBitMaskMap())
        , m_isConnected(false)
    {
        // These values are adjusted when a joystick is detected.  So these just exist
        // to be placeholders until we actually compute them based on the data from the device.
        m_rawGamepadState.m_triggerDeadZoneValue = 0.1f;
        m_rawGamepadState.m_thumbStickLeftDeadZone = 0.1f;
        m_rawGamepadState.m_thumbStickRightDeadZone = 0.1f;

        // O3DE expects trigger values to always be 0.0 being not pressed at all, and 1.0 being pressed all the way.
        // so far, devices tested have always had a range of 0 - 255, so this does not need special treatment like the axes do,
        // and will get the normal maximum value straight from the device.
        m_rawGamepadState.m_triggerMaximumValue = 1.0f;

        // these values are always 1.0f, because we will normalize the value between -1.0 and 1.0.
        // O3DE expects axis 0.0 to be the center but some devices have a range like 0 - 255 with 128 being the center.
        // to account for that, this code will adjust and offset and scale.
        m_rawGamepadState.m_thumbStickMaximumValue = 1.0f;

        m_internalState.reset(new InternalState(libevdevWrapper));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceGamepadLinux::~InputDeviceGamepadLinux()
    {
        m_internalState.reset();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceGamepadLinux::IsConnected() const
    {
        return m_isConnected;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceGamepadLinux::SetVibration([[maybe_unused]]float leftMotorSpeedNormalized,
                                               [[maybe_unused]]float rightMotorSpeedNormalized)
    {
        if (m_isConnected)
        {
            // (TODO) This is for future implementors.  Better to have gamepad support without force feedback. 
            // than no gamepad support at all.  evdev does have the capability of doing force feedback.
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceGamepadLinux::GetPhysicalKeyOrButtonText(const InputChannelId& /*inputChannelId*/,
                                                               AZStd::string& /*o_keyOrButtonText*/) const
    {
        return false; // this will cause the base implementation to return the default names.
    }

    void InputDeviceGamepadLinux::BumpTryAgainTimeout()
    {
        m_tryAgainTimeout = s_retryDelayAfterFail + AZ::TimeMs(10 * GetInputDeviceIndex());
    }

    // Called periodically but only on devices that are not currently already connected.
    void InputDeviceGamepadLinux::TryConnect()
    {
        using namespace GamepadLinuxPrivate;

        if (m_isConnected)
        {
            return;
        }

        // if libevdev wasn't successfully loaded, no point in proceeding, either.
        if (!m_internalState->m_libevdevModule->m_libevdevHandle)
        {
            return;
        }

        if (m_tryAgainTimeout > AZ::Time::ZeroTimeMs)
        {
            m_tryAgainTimeout -= AZ::TimeUsToMs(AZ::GetRealTickDeltaTimeUs());
            return;
        }
        
        AZStd::string canFind = FindJoystickForIndex(GetInputDeviceIndex());
        if (canFind.empty())
        {
            BumpTryAgainTimeout();
            return;
        }

        m_internalState->m_fileDescriptor = open(canFind.c_str(), O_RDONLY|O_NONBLOCK);
        if (m_internalState->m_fileDescriptor  == -1)
        {
            BumpTryAgainTimeout();
            return;
        }
            
        if (m_internalState->m_libevdevModule->m_libevdev_new_from_fd(m_internalState->m_fileDescriptor, &m_internalState->m_evdevDevice) != 0)
        {
            m_internalState->CloseDevice();
            BumpTryAgainTimeout();
            return;
        }

        // if we get here, we successfully opened the device.
        // Show the gamepad's name and its index to the main log.
        // For debugging, output the buttons and axes available to trace, which won't show up on the regular console log.
        // note that the BTN_JOYSTICK, KEY_MAX, EV_KEY* all come from linux input, not from evdev.
        AZ_Info("Input", "Detected Gamepad at index %u, with name: \"%s\"\n", GetInputDeviceIndex(), 
            m_internalState->m_libevdevModule->m_libevdev_get_name(m_internalState->m_evdevDevice));

        for (int i = BTN_JOYSTICK; i < KEY_MAX; ++i) {
            if (m_internalState->m_libevdevModule->m_libevdev_has_event_code(m_internalState->m_evdevDevice, EV_KEY, i)) {
                AZ_Trace("Input", "    Button Detected: %s\n", m_internalState->m_libevdevModule->m_libevdev_event_code_get_name(EV_KEY, i));
            }
        }

        // output the axes, to the log but also, while grabbing them, use the data from the OS to calibrate their
        // dead zone and ranges.
        for (int i = 0; i < ABS_MAX; ++i) 
        {
            if (m_internalState->m_libevdevModule->m_libevdev_has_event_code(m_internalState->m_evdevDevice, EV_ABS, i))
            {
                const struct input_absinfo *absInfo = m_internalState->m_libevdevModule->m_libevdev_get_abs_info(m_internalState->m_evdevDevice, i);
                if (absInfo)
                {
                    AZ_Trace("Input", "Detected Gamepad axis: %s...\n", m_internalState->m_libevdevModule->m_libevdev_event_code_get_name(EV_ABS, i));
                    AZ_Trace("Input", "    Value: %d\n    Minimum: %d\n    Maximum: %d\n    Fuzz: %d\n    Flat:%d\n",
                        absInfo->value, absInfo->minimum, absInfo->maximum, absInfo->fuzz, absInfo->flat);
                    switch (i)
                    {
                        // note that the thumbsticks will be normalized between -1.0 and 1.0 and the deadzone
                        // has to be in that same unit.  Calculate the range and convert the deadzone into that range. 
                        case ABS_X:
                        case ABS_Y:
                        {
                            // Left thumbstick x and y
                            float range = static_cast<float>(absInfo->maximum - absInfo->minimum);
                            if (range != 0.0f)
                            {
                                m_internalState->m_axisLeftOffset = -1.0f * (static_cast<float>(absInfo->maximum + absInfo->minimum) / 2.0f);
                                m_internalState->m_axisLeftScale = 1.0f / (range / 2.0f);
                                m_rawGamepadState.m_thumbStickLeftDeadZone = static_cast<float>(absInfo->flat) * m_internalState->m_axisLeftScale;
                            }
                            
                            break;
                        }
                        case ABS_RX:
                        case ABS_RY:
                        {
                            // right thumbstick x and y
                            float range = static_cast<float>(absInfo->maximum - absInfo->minimum);
                            if (range > 0.0f)
                            {
                                m_internalState->m_axisRightOffset = -1.0f * (static_cast<float>(absInfo->maximum + absInfo->minimum) / 2.0f);
                                m_internalState->m_axisRightScale = 1.0f / (range / 2.0f);
                                m_rawGamepadState.m_thumbStickRightDeadZone = static_cast<float>(absInfo->flat) * m_internalState->m_axisRightScale;
                            }
                            break;
                        }
                        case ABS_Z: // this actually represents triggers.
                        case ABS_RZ:
                        {
                            // again, the underlying raw state only has room for 1 dead zone and 1 maximum value
                            // no matter how many triggers, so we'll just use all available axes, the final one
                            // being the one that sticks.  This means that even if the device reports only 1 trigger
                            // for example, maybe it only has ABS_RZ and not ABS_Z we still get some calibration
                            m_rawGamepadState.m_triggerMaximumValue = static_cast<float>(absInfo->maximum);
                            m_rawGamepadState.m_triggerDeadZoneValue = static_cast<float>(absInfo->flat);
                        }
                        case ABS_HAT0X: 
                        case ABS_HAT0Y:
                            // special case, some devices represent the dpad as an axis, not as buttons.
                            // note that devices which DO present the dpad as buttons will already work
                            // because they will have the BTN_DPAD_XXX buttons, which is already mapped 
                            // by the KEY events.
                            m_internalState->m_dPadHatMax = static_cast<float>(absInfo->maximum);

                    }
                }
            }
        }

        m_isConnected = true;
        ResetInputChannelStates();
        BroadcastInputDeviceConnectedEvent();
    }

    // one choice to be made here is whether sub-frame events should be immediately sent to the device
    // for example a person could flick a stick to the edge and back to the center all in the space of one frame,
    // and get many events within a single frame, but most games only care about where the stick is at the end of the frame
    // when they go to do their input computation.  Its also more expensive to respond to events for every movement
    // on axes, because the hardware might actually send many input events to represent one movement in one frame for an analog
    // device.

    // To make it as responsive as possible, for now, but not waste processing power, 
    // let's consider button presses to be things we don't want to miss within a sub-frame but stick movements to be things we only
    // care about knowing where it lands up when the event queue is dry.
    void InputDeviceGamepadLinux::UpdateButtonState(AZ::u32 buttonMask, bool pressed)
    {
        AZ::u32 priorState = m_rawGamepadState.m_digitalButtonStates;;
        if (pressed)
        {
            m_rawGamepadState.m_digitalButtonStates = m_rawGamepadState.m_digitalButtonStates | buttonMask;
        }
        else
        {
            m_rawGamepadState.m_digitalButtonStates = m_rawGamepadState.m_digitalButtonStates & ~buttonMask;
        }

        // send a button change immediately if one occurred.
        if (priorState != m_rawGamepadState.m_digitalButtonStates)
        {
            ProcessRawGamepadState(m_rawGamepadState);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceGamepadLinux::TickInputDevice()
    {
        // This happens for all device indices, even inactive ones.
        // If they are inactive, we should check if they have been activated, and vice-versa
        TryConnect();

        if (m_isConnected)
        {
            // we can only get into this block of code if the libevdevModule is valid in the first place
            // so we don't need to check that here.

            struct input_event ev; // note that this comes from linux/input.h, not evdev.

            bool updatedGamepadAxis = false;
            int libevdevResult = 0;
            auto nextEventFn = m_internalState->m_libevdevModule->m_libevdev_next_event;
            while ((libevdevResult = nextEventFn(m_internalState->m_evdevDevice, O3DEWRAPPER_LIBEVDEV_READ_FLAG_NORMAL, &ev)) == O3DEWRAPPER_LIBEVDEV_READ_STATUS_SUCCESS)
            {
                switch (ev.type)
                {
                    case EV_KEY:
                    {
                        AZ::u32 buttonMask = GetButtonMaskForActualButton(ev.code);
                        if (buttonMask != 0)
                        {
                            bool pressed = ev.value != 0;
                            UpdateButtonState(buttonMask, pressed);
                        }
                        break;
                    }
                    case EV_ABS:
                    {
                        int currentValue = static_cast<float>(ev.value);
                        switch (ev.code)
                        {
                            // the axes (X, Y, RX, RY) are special here because they must assume 0 is the center according to O3DE
                            // API - but some joysticks start at 0 and have some integer as the max, 
                            // with the center being the middle, so this requires us to offset and scale to translate for O3DE.
                            case ABS_X:
                            {
                                m_rawGamepadState.m_thumbStickLeftXState = (currentValue + m_internalState->m_axisLeftOffset) * m_internalState->m_axisLeftScale;
                                updatedGamepadAxis = true;
                                break;
                            }
                            case ABS_Y:
                            {
                                // note that O3DE expects positive Y to mean moving the stick "away from the user",
                                // ie, moving the stick towards the back of the controller where the triggers are.
                                // (I am avoiding using the ambiguous term 'up' here).
                                // libevdev outputs positive Y values when the user is pulling the stick towards them, so we need to invert the value.
                                // You can see the same kind of inversion happening in Gems/VirtualGamepad/Code/Source/VirtualGamepadThumbStickComponent.cpp
                                // You do NOT see this same inversion in the windows gamepad file since it uses XInput
                                // and XInput by default maps positive Y values to mean "away from the user".
                                m_rawGamepadState.m_thumbStickLeftYState = -1.0f * (currentValue + m_internalState->m_axisLeftOffset) * m_internalState->m_axisLeftScale;
                                updatedGamepadAxis = true;
                                break;
                            }
                            case ABS_RX:
                            {
                                m_rawGamepadState.m_thumbStickRightXState = (currentValue + m_internalState->m_axisRightOffset) * m_internalState->m_axisRightScale;
                                updatedGamepadAxis = true;
                                break;
                            }
                            case ABS_RY:
                            {
                                m_rawGamepadState.m_thumbStickRightYState = -1.0f * (currentValue + m_internalState->m_axisRightOffset) * m_internalState->m_axisRightScale;
                                updatedGamepadAxis = true;
                                break;
                            }
                            case ABS_Z:
                            {
                                m_rawGamepadState.m_triggerButtonLState = ev.value;
                                updatedGamepadAxis = true;
                                break;
                            }
                            case ABS_RZ:
                            {
                                m_rawGamepadState.m_triggerButtonRState = ev.value;
                                updatedGamepadAxis = true;
                                break;
                            }
                            case ABS_HAT0X:
                            {
                                if (m_internalState->m_dPadHatMax != 0.0f)
                                {
                                    if (ev.value > 0)
                                    {
                                        UpdateButtonState(BTN_DPAD_RIGHT_MASK, true);
                                        UpdateButtonState(BTN_DPAD_LEFT_MASK, false);

                                    }
                                    else if (ev.value < 0)
                                    {
                                        UpdateButtonState(BTN_DPAD_RIGHT_MASK, false);
                                        UpdateButtonState(BTN_DPAD_LEFT_MASK, true);
                                    }
                                    else // centered.
                                    {
                                        UpdateButtonState(BTN_DPAD_RIGHT_MASK, false);
                                        UpdateButtonState(BTN_DPAD_LEFT_MASK, false);
                                    }
                                }
                                break;
                            }
                            case ABS_HAT0Y:
                            {
                                if (m_internalState->m_dPadHatMax != 0.0f)
                                {
                                    if (ev.value > 0)
                                    {
                                        // > 0 actually indicates downwards, not upwards
                                        UpdateButtonState(BTN_DPAD_DOWN_MASK, true);
                                        UpdateButtonState(BTN_DPAD_UP_MASK, false);
                                    }
                                    else if (ev.value < 0)
                                    {
                                        UpdateButtonState(BTN_DPAD_DOWN_MASK, false);
                                        UpdateButtonState(BTN_DPAD_UP_MASK, true);
                                    }
                                    else
                                    {
                                        UpdateButtonState(BTN_DPAD_DOWN_MASK, false);
                                        UpdateButtonState(BTN_DPAD_UP_MASK, false);
                                    }
                                }
                                break;
                            }
                        }
                        break;
                    }
                }
            }

            if (libevdevResult == -ENODEV) // note that other results could just mean that there are no events ready.
            {
                // A device disconnected - 
                AZ_Info("Input", "Gamepad at index %u disconnected.\n", GetInputDeviceIndex());
                m_isConnected = false;
                m_rawGamepadState.Reset();
                m_internalState->CloseDevice();
                ResetInputChannelStates();
                BroadcastInputDeviceDisconnectedEvent();
                m_tryAgainTimeout = s_retryDelayAfterFail + AZ::TimeMs(10 * GetInputDeviceIndex());
            }
            else
            {
                if (updatedGamepadAxis)
                {
                    ProcessRawGamepadState(m_rawGamepadState);
                }
            }
        }
    }

} // namespace AzFramework
