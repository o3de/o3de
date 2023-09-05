/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Input/Devices/Gamepad/InputDeviceGamepad.h>
#include <AzFramework/Input/Utils/AdjustAnalogInputForDeadZone.h>

#include <AzCore/RTTI/BehaviorContext.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceGamepad::IsGamepadDevice(const InputDeviceId& inputDeviceId)
    {
        return (inputDeviceId.GetNameCrc32() == IdForIndex0.GetNameCrc32());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceGamepad::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            // Unfortunately it doesn't seem possible to reflect anything through BehaviorContext
            // using lambdas which capture variables from the enclosing scope. So we are manually
            // reflecting all input channel names, instead of just iterating over them like this:
            //
            //  auto classBuilder = behaviorContext->Class<InputDeviceGamepad>();
            //  for (const InputChannelId& channelId : Button::All)
            //  {
            //      const char* channelName = channelId.GetName();
            //      classBuilder->Constant(channelName, [channelName]() { return channelName; });
            //  }

            behaviorContext->Class<InputDeviceGamepad>()
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::RuntimeOwn)
                ->Constant("name", BehaviorConstant(IdForIndex0.GetName()))

                ->Constant(Button::A.GetName(), BehaviorConstant(Button::A.GetName()))
                ->Constant(Button::B.GetName(), BehaviorConstant(Button::B.GetName()))
                ->Constant(Button::X.GetName(), BehaviorConstant(Button::X.GetName()))
                ->Constant(Button::Y.GetName(), BehaviorConstant(Button::Y.GetName()))
                ->Constant(Button::L1.GetName(), BehaviorConstant(Button::L1.GetName()))
                ->Constant(Button::R1.GetName(), BehaviorConstant(Button::R1.GetName()))
                ->Constant(Button::L3.GetName(), BehaviorConstant(Button::L3.GetName()))
                ->Constant(Button::R3.GetName(), BehaviorConstant(Button::R3.GetName()))
                ->Constant(Button::DU.GetName(), BehaviorConstant(Button::DU.GetName()))
                ->Constant(Button::DD.GetName(), BehaviorConstant(Button::DD.GetName()))
                ->Constant(Button::DL.GetName(), BehaviorConstant(Button::DL.GetName()))
                ->Constant(Button::DR.GetName(), BehaviorConstant(Button::DR.GetName()))
                ->Constant(Button::Start.GetName(), BehaviorConstant(Button::Start.GetName()))
                ->Constant(Button::Select.GetName(), BehaviorConstant(Button::Select.GetName()))

                ->Constant(Trigger::L2.GetName(), BehaviorConstant(Trigger::L2.GetName()))
                ->Constant(Trigger::R2.GetName(), BehaviorConstant(Trigger::R2.GetName()))

                ->Constant(ThumbStickAxis2D::L.GetName(), BehaviorConstant(ThumbStickAxis2D::L.GetName()))
                ->Constant(ThumbStickAxis2D::R.GetName(), BehaviorConstant(ThumbStickAxis2D::R.GetName()))

                ->Constant(ThumbStickAxis1D::LX.GetName(), BehaviorConstant(ThumbStickAxis1D::LX.GetName()))
                ->Constant(ThumbStickAxis1D::LY.GetName(), BehaviorConstant(ThumbStickAxis1D::LY.GetName()))
                ->Constant(ThumbStickAxis1D::RX.GetName(), BehaviorConstant(ThumbStickAxis1D::RX.GetName()))
                ->Constant(ThumbStickAxis1D::RY.GetName(), BehaviorConstant(ThumbStickAxis1D::RY.GetName()))

                ->Constant(ThumbStickDirection::LU.GetName(), BehaviorConstant(ThumbStickDirection::LU.GetName()))
                ->Constant(ThumbStickDirection::LD.GetName(), BehaviorConstant(ThumbStickDirection::LD.GetName()))
                ->Constant(ThumbStickDirection::LL.GetName(), BehaviorConstant(ThumbStickDirection::LL.GetName()))
                ->Constant(ThumbStickDirection::LR.GetName(), BehaviorConstant(ThumbStickDirection::LR.GetName()))
                ->Constant(ThumbStickDirection::RU.GetName(), BehaviorConstant(ThumbStickDirection::RU.GetName()))
                ->Constant(ThumbStickDirection::RD.GetName(), BehaviorConstant(ThumbStickDirection::RD.GetName()))
                ->Constant(ThumbStickDirection::RL.GetName(), BehaviorConstant(ThumbStickDirection::RL.GetName()))
                ->Constant(ThumbStickDirection::RR.GetName(), BehaviorConstant(ThumbStickDirection::RR.GetName()))
            ;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceGamepad::InputDeviceGamepad()
        : InputDeviceGamepad(0) // Delegated constructor
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceGamepad::InputDeviceGamepad(AZ::u32 index)
        : InputDeviceGamepad(InputDeviceId(Name, index)) // Delegated constructor
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceGamepad::InputDeviceGamepad(const InputDeviceId& inputDeviceId,
                                           ImplementationFactory* implementationFactory)
        : InputDevice(inputDeviceId)
        , m_allChannelsById()
        , m_buttonChannelsById()
        , m_triggerChannelsById()
        , m_thumbStickAxis1DChannelsById()
        , m_thumbStickAxis2DChannelsById()
        , m_thumbStickDirectionChannelsById()
        , m_pimpl()
        , m_implementationRequestHandler(*this)
    {
        // Create all digital button input channels
        for (const InputChannelId& channelId : Button::All)
        {
            InputChannelDigital* channel = aznew InputChannelDigital(channelId, *this);
            m_allChannelsById[channelId] = channel;
            m_buttonChannelsById[channelId] = channel;
        }

        // Create all analog trigger input channels
        for (const InputChannelId& channelId : Trigger::All)
        {
            InputChannelAnalog* channel = aznew InputChannelAnalog(channelId, *this);
            m_allChannelsById[channelId] = channel;
            m_triggerChannelsById[channelId] = channel;
        }

        // Create all thumb-stick 1D axis input channels
        for (const InputChannelId& channelId : ThumbStickAxis1D::All)
        {
            InputChannelAxis1D* channel = aznew InputChannelAxis1D(channelId, *this);
            m_allChannelsById[channelId] = channel;
            m_thumbStickAxis1DChannelsById[channelId] = channel;
        }

        // Create all thumb-stick 2D axis input channels
        for (const InputChannelId& channelId : ThumbStickAxis2D::All)
        {
            InputChannelAxis2D* channel = aznew InputChannelAxis2D(channelId, *this);
            m_allChannelsById[channelId] = channel;
            m_thumbStickAxis2DChannelsById[channelId] = channel;
        }

        // Create all thumb-stick direction input channels
        for (const InputChannelId& channelId : ThumbStickDirection::All)
        {
            InputChannelAnalog* channel = aznew InputChannelAnalog(channelId, *this);
            m_allChannelsById[channelId] = channel;
            m_thumbStickDirectionChannelsById[channelId] = channel;
        }

        // Create the platform specific or custom implementation
        m_pimpl = (implementationFactory != nullptr) ? implementationFactory->Create(*this) : nullptr;

        // Connect to the haptic feedback request bus
        InputHapticFeedbackRequestBus::Handler::BusConnect(GetInputDeviceId());

        // Connect to the light bar request bus
        InputLightBarRequestBus::Handler::BusConnect(GetInputDeviceId());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceGamepad::~InputDeviceGamepad()
    {
        // Disconnect from the light bar request bus
        InputLightBarRequestBus::Handler::BusDisconnect(GetInputDeviceId());

        // Disconnect from the haptic feedback request bus
        InputHapticFeedbackRequestBus::Handler::BusDisconnect(GetInputDeviceId());

        // Destroy the platform specific implementation
        m_pimpl.reset();

        // Destroy all thumb-stick direction input channels
        for (const auto& channelById : m_thumbStickDirectionChannelsById)
        {
            delete channelById.second;
        }

        // Destroy all thumb-stick 2D axis input channels
        for (const auto& channelById : m_thumbStickAxis2DChannelsById)
        {
            delete channelById.second;
        }

        // Destroy all thumb-stick 1D axis input channels
        for (const auto& channelById : m_thumbStickAxis1DChannelsById)
        {
            delete channelById.second;
        }

        // Destroy all analog trigger input channels
        for (const auto& channelById : m_triggerChannelsById)
        {
            delete channelById.second;
        }

        // Destroy all digital button input channels
        for (const auto& channelById : m_buttonChannelsById)
        {
            delete channelById.second;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    LocalUserId InputDeviceGamepad::GetAssignedLocalUserId() const
    {
        return m_pimpl ? m_pimpl->GetAssignedLocalUserId() : InputDevice::GetAssignedLocalUserId();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceGamepad::PromptLocalUserSignIn() const
    {
        if (m_pimpl)
        {
            m_pimpl->PromptLocalUserSignIn();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const InputDevice::InputChannelByIdMap& InputDeviceGamepad::GetInputChannelsById() const
    {
        return m_allChannelsById;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceGamepad::IsSupported() const
    {
        return m_pimpl != nullptr;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceGamepad::IsConnected() const
    {
        return m_pimpl ? m_pimpl->IsConnected() : false;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceGamepad::GetPhysicalKeyOrButtonText(const InputChannelId& inputChannelId,
                                                        AZStd::string& o_keyOrButtonText) const
    {
        // First see if the button has platform specific text 
        if (m_pimpl && m_pimpl->GetPhysicalKeyOrButtonText(inputChannelId, o_keyOrButtonText))
        {
            return;
        }

        if      (inputChannelId == Button::A) { o_keyOrButtonText = "A"; }
        else if (inputChannelId == Button::B) { o_keyOrButtonText = "B"; }
        else if (inputChannelId == Button::X) { o_keyOrButtonText = "X"; }
        else if (inputChannelId == Button::Y) { o_keyOrButtonText = "Y"; }
        else if (inputChannelId == Button::L1) { o_keyOrButtonText = "L1"; }
        else if (inputChannelId == Button::R1) { o_keyOrButtonText = "R1"; }
        else if (inputChannelId == Button::L3) { o_keyOrButtonText = "L3"; }
        else if (inputChannelId == Button::R3) { o_keyOrButtonText = "R3"; }
        else if (inputChannelId == Button::DU) { o_keyOrButtonText = "D-pad Up"; }
        else if (inputChannelId == Button::DD) { o_keyOrButtonText = "D-pad Down"; }
        else if (inputChannelId == Button::DL) { o_keyOrButtonText = "D-pad Left"; }
        else if (inputChannelId == Button::DR) { o_keyOrButtonText = "D-pad Right"; }
        else if (inputChannelId == Button::Start) { o_keyOrButtonText = "Start"; }
        else if (inputChannelId == Button::Select) { o_keyOrButtonText = "Select"; }
        else if (inputChannelId == Trigger::L2) { o_keyOrButtonText = "L2"; }
        else if (inputChannelId == Trigger::R2) { o_keyOrButtonText = "R2"; }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceGamepad::TickInputDevice()
    {
        if (m_pimpl)
        {
            m_pimpl->TickInputDevice();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceGamepad::SetVibration(float leftMotorSpeedNormalized,
                                          float rightMotorSpeedNormalized)
    {
        if (m_pimpl)
        {
            m_pimpl->SetVibration(leftMotorSpeedNormalized, rightMotorSpeedNormalized);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceGamepad::SetLightBarColor(const AZ::Color& color)
    {
        if (m_pimpl)
        {
            m_pimpl->SetLightBarColor(color);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceGamepad::ResetLightBarColor()
    {
        if (m_pimpl)
        {
            m_pimpl->ResetLightBarColor();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceGamepad::Implementation::Implementation(InputDeviceGamepad& inputDevice)
        : m_inputDevice(inputDevice)
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceGamepad::Implementation::~Implementation()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    LocalUserId InputDeviceGamepad::Implementation::GetAssignedLocalUserId() const
    {
        return GetInputDeviceIndex();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceGamepad::Implementation::BroadcastInputDeviceConnectedEvent() const
    {
        m_inputDevice.BroadcastInputDeviceConnectedEvent();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceGamepad::Implementation::BroadcastInputDeviceDisconnectedEvent() const
    {
        m_inputDevice.BroadcastInputDeviceDisconnectedEvent();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceGamepad::Implementation::RawGamepadState::RawGamepadState(
        const DigitalButtonIdByBitMaskMap& digitalButtonIdsByBitMask)
    : m_digitalButtonIdsByBitMask(digitalButtonIdsByBitMask)
    , m_digitalButtonStates(0)
    , m_triggerButtonLState(0.0f)
    , m_triggerButtonRState(0.0f)
    , m_thumbStickLeftXState(0.0f)
    , m_thumbStickLeftYState(0.0f)
    , m_thumbStickRightXState(0.0f)
    , m_thumbStickRightYState(0.0f)
    , m_triggerMaximumValue(1.0f)
    , m_triggerDeadZoneValue(0.0f)
    , m_thumbStickMaximumValue(1.0f)
    , m_thumbStickLeftDeadZone(0.0f)
    , m_thumbStickRightDeadZone(0.0f)
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceGamepad::Implementation::RawGamepadState::Reset()
    {
        m_digitalButtonStates = 0;
        m_triggerButtonLState = 0.0f;
        m_triggerButtonRState = 0.0f;
        m_thumbStickLeftXState = 0.0f;
        m_thumbStickLeftYState = 0.0f;
        m_thumbStickRightXState = 0.0f;
        m_thumbStickRightYState = 0.0f;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    float InputDeviceGamepad::Implementation::RawGamepadState::GetLeftTriggerAdjustedForDeadZoneAndNormalized() const
    {
        return AdjustForDeadZoneAndNormalizeAnalogInput(m_triggerButtonLState,
                                                        m_triggerDeadZoneValue,
                                                        m_triggerMaximumValue);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    float InputDeviceGamepad::Implementation::RawGamepadState::GetRightTriggerAdjustedForDeadZoneAndNormalized() const
    {
        return AdjustForDeadZoneAndNormalizeAnalogInput(m_triggerButtonRState,
                                                        m_triggerDeadZoneValue,
                                                        m_triggerMaximumValue);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::Vector2 InputDeviceGamepad::Implementation::RawGamepadState::GetLeftThumbStickAdjustedForDeadZoneAndNormalized() const
    {
        return AdjustForDeadZoneAndNormalizeThumbStickInput(m_thumbStickLeftXState,
                                                            m_thumbStickLeftYState,
                                                            m_thumbStickLeftDeadZone,
                                                            m_thumbStickMaximumValue);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::Vector2 InputDeviceGamepad::Implementation::RawGamepadState::GetRightThumbStickAdjustedForDeadZoneAndNormalized() const
    {
        return AdjustForDeadZoneAndNormalizeThumbStickInput(m_thumbStickRightXState,
                                                            m_thumbStickRightYState,
                                                            m_thumbStickRightDeadZone,
                                                            m_thumbStickMaximumValue);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::Vector2 InputDeviceGamepad::Implementation::RawGamepadState::GetLeftThumbStickNormalizedValues() const
    {
        return AZ::Vector2(m_thumbStickLeftXState / m_thumbStickMaximumValue,
                           m_thumbStickLeftYState / m_thumbStickMaximumValue);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::Vector2 InputDeviceGamepad::Implementation::RawGamepadState::GetRightThumbStickNormalizedValues() const
    {
        return AZ::Vector2(m_thumbStickRightXState / m_thumbStickMaximumValue,
                           m_thumbStickRightYState / m_thumbStickMaximumValue);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceGamepad::Implementation::ProcessRawGamepadState(
        const RawGamepadState& rawGamepadState)
    {
        // Update digital button channels
        for (const auto& digitalButtonIdByBitMaskPair : rawGamepadState.m_digitalButtonIdsByBitMask)
        {
            const AZ::u32 buttonState = (rawGamepadState.m_digitalButtonStates & digitalButtonIdByBitMaskPair.first);
            const InputChannelId& channelId = *(digitalButtonIdByBitMaskPair.second);
            m_inputDevice.m_buttonChannelsById[channelId]->ProcessRawInputEvent(buttonState != 0);
        }

        // Update the left analog trigger button channel
        const float valueL2 = rawGamepadState.GetLeftTriggerAdjustedForDeadZoneAndNormalized();
        m_inputDevice.m_triggerChannelsById[InputDeviceGamepad::Trigger::L2]->ProcessRawInputEvent(valueL2);

        // Update the right analog trigger button channel
        const float valueR2 = rawGamepadState.GetRightTriggerAdjustedForDeadZoneAndNormalized();
        m_inputDevice.m_triggerChannelsById[InputDeviceGamepad::Trigger::R2]->ProcessRawInputEvent(valueR2);

        // Update the left thumb-stick channels
        const AZ::Vector2 valuesLeftThumb = rawGamepadState.GetLeftThumbStickAdjustedForDeadZoneAndNormalized();
        const AZ::Vector2 valuesLeftThumbPreDeadZone = rawGamepadState.GetLeftThumbStickNormalizedValues();
        m_inputDevice.m_thumbStickAxis2DChannelsById[InputDeviceGamepad::ThumbStickAxis2D::L]->ProcessRawInputEvent(valuesLeftThumb, &valuesLeftThumbPreDeadZone);
        m_inputDevice.m_thumbStickAxis1DChannelsById[InputDeviceGamepad::ThumbStickAxis1D::LX]->ProcessRawInputEvent(valuesLeftThumb.GetX());
        m_inputDevice.m_thumbStickAxis1DChannelsById[InputDeviceGamepad::ThumbStickAxis1D::LY]->ProcessRawInputEvent(valuesLeftThumb.GetY());

        const float leftStickUp = AZ::GetClamp(valuesLeftThumb.GetY(), 0.0f, 1.0f);
        const float leftStickDown = fabsf(AZ::GetClamp(valuesLeftThumb.GetY(), -1.0f, 0.0f));
        const float leftStickLeft = fabsf(AZ::GetClamp(valuesLeftThumb.GetX(), -1.0f, 0.0f));
        const float leftStickRight = AZ::GetClamp(valuesLeftThumb.GetX(), 0.0f, 1.0f);
        m_inputDevice.m_thumbStickDirectionChannelsById[InputDeviceGamepad::ThumbStickDirection::LU]->ProcessRawInputEvent(leftStickUp);
        m_inputDevice.m_thumbStickDirectionChannelsById[InputDeviceGamepad::ThumbStickDirection::LD]->ProcessRawInputEvent(leftStickDown);
        m_inputDevice.m_thumbStickDirectionChannelsById[InputDeviceGamepad::ThumbStickDirection::LL]->ProcessRawInputEvent(leftStickLeft);
        m_inputDevice.m_thumbStickDirectionChannelsById[InputDeviceGamepad::ThumbStickDirection::LR]->ProcessRawInputEvent(leftStickRight);

        // Update the right thumb-stick channels
        const AZ::Vector2 valuesRightThumb = rawGamepadState.GetRightThumbStickAdjustedForDeadZoneAndNormalized();
        const AZ::Vector2 valuesRightThumbPreDeadZone = rawGamepadState.GetRightThumbStickNormalizedValues();
        m_inputDevice.m_thumbStickAxis2DChannelsById[InputDeviceGamepad::ThumbStickAxis2D::R]->ProcessRawInputEvent(valuesRightThumb, &valuesRightThumbPreDeadZone);
        m_inputDevice.m_thumbStickAxis1DChannelsById[InputDeviceGamepad::ThumbStickAxis1D::RX]->ProcessRawInputEvent(valuesRightThumb.GetX());
        m_inputDevice.m_thumbStickAxis1DChannelsById[InputDeviceGamepad::ThumbStickAxis1D::RY]->ProcessRawInputEvent(valuesRightThumb.GetY());

        const float rightStickUp = AZ::GetClamp(valuesRightThumb.GetY(), 0.0f, 1.0f);
        const float rightStickDown = fabsf(AZ::GetClamp(valuesRightThumb.GetY(), -1.0f, 0.0f));
        const float rightStickLeft = fabsf(AZ::GetClamp(valuesRightThumb.GetX(), -1.0f, 0.0f));
        const float rightStickRight = AZ::GetClamp(valuesRightThumb.GetX(), 0.0f, 1.0f);
        m_inputDevice.m_thumbStickDirectionChannelsById[InputDeviceGamepad::ThumbStickDirection::RU]->ProcessRawInputEvent(rightStickUp);
        m_inputDevice.m_thumbStickDirectionChannelsById[InputDeviceGamepad::ThumbStickDirection::RD]->ProcessRawInputEvent(rightStickDown);
        m_inputDevice.m_thumbStickDirectionChannelsById[InputDeviceGamepad::ThumbStickDirection::RL]->ProcessRawInputEvent(rightStickLeft);
        m_inputDevice.m_thumbStickDirectionChannelsById[InputDeviceGamepad::ThumbStickDirection::RR]->ProcessRawInputEvent(rightStickRight);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceGamepad::Implementation::ResetInputChannelStates()
    {
        m_inputDevice.ResetInputChannelStates();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::u32 InputDeviceGamepad::Implementation::GetInputDeviceIndex() const
    {
        return m_inputDevice.GetInputDeviceId().GetIndex();
    }
} // namespace AzFramework
