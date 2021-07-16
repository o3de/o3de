/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Input.h"
#include "LyToAzInputNameConversions.h"
#include <AzCore/EBus/EBus.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/sort.h>

#include <AzFramework/Input/Buses/Requests/InputDeviceRequestBus.h>
#include <AzFramework/Input/Devices/Gamepad/InputDeviceGamepad.h>

// CryCommon includes
#include <PlayerProfileRequestBus.h>
#include <InputTypes.h>


namespace Input
{
    static const int s_inputVersion = 2;

    bool ConvertInputVersion1To2(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        const int deviceTypeElementIndex = classElement.FindElement(AZ_CRC("Input Device Type"));
        if (deviceTypeElementIndex == -1)
        {
            AZ_Error("Input", false, "Could not find 'Input Device Type' element");
            return false;
        }

        AZ::SerializeContext::DataElementNode& deviceTypeElementNode = classElement.GetSubElement(deviceTypeElementIndex);
        AZStd::string deviceTypeElementValue;
        if (!deviceTypeElementNode.GetData(deviceTypeElementValue))
        {
            AZ_Error("Input", false, "Could not get 'Input Device Type' element as a string");
            return false;
        }

        const AZStd::string convertedDeviceType = ConvertInputDeviceName(deviceTypeElementValue);
        if (!deviceTypeElementNode.SetData(context, convertedDeviceType))
        {
            AZ_Error("Input", false, "Could not set 'Input Device Type' element as a string");
            return false;
        }

        const int eventNameElementIndex = classElement.FindElement(AZ_CRC("Input Name"));
        if (eventNameElementIndex == -1)
        {
            AZ_Error("Input", false, "Could not find 'Input Name' element");
            return false;
        }

        AZ::SerializeContext::DataElementNode& eventNameElementNode = classElement.GetSubElement(eventNameElementIndex);
        AZStd::string eventNameElementValue;
        if (!eventNameElementNode.GetData(eventNameElementValue))
        {
            AZ_Error("Input", false, "Could not get 'Input Name' element as a string");
            return false;
        }

        const AZStd::string convertedElementName = ConvertInputEventName(eventNameElementValue);
        if (!eventNameElementNode.SetData(context, convertedElementName))
        {
            AZ_Error("Input", false, "Could not set 'Input Name' element as a string");
            return false;
        }

        return true;
    }

    bool ConvertInputVersion(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        int currentUpgradedVersion = classElement.GetVersion();
        while (currentUpgradedVersion < s_inputVersion)
        {
            switch (currentUpgradedVersion)
            {
                case 1:
                {
                    if (ConvertInputVersion1To2(context, classElement))
                    {
                        currentUpgradedVersion = 2;
                    }
                    else
                    {
                        AZ_Warning("Input", false, "Failed to convert Input from version 1 to 2, its data will be lost on save");
                        return false;
                    }
                }
                break;
                case 0:
                default:
                {
                    AZ_Warning("Input", false, "Unable to convert Input: unsupported version %i, its data will be lost on save", currentUpgradedVersion);
                    return false;
                }
            }
        }
        return true;
    }

    void Input::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<Input>()
                ->Version(s_inputVersion, &ConvertInputVersion)
                    ->Field("Input Device Type", &Input::m_inputDeviceType)
                    ->Field("Input Name", &Input::m_inputName)
                    ->Field("Event Value Multiplier", &Input::m_eventValueMultiplier)
                    ->Field("Dead Zone", &Input::m_deadZone);

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<Input>("Input", "Hold an input to generate an event")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::NameLabelOverride, &Input::GetEditorText)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &Input::m_inputDeviceType, "Input Device Type", "The type of input device, ex keyboard")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &Input::OnDeviceSelected)
                        ->Attribute(AZ::Edit::Attributes::StringList, &Input::GetInputDeviceTypes)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &Input::m_inputName, "Input Name", "The name of the input you want to hold ex. space")
                        ->Attribute(AZ::Edit::Attributes::StringList, &Input::GetInputNamesBySelectedDevice)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                    ->DataElement(0, &Input::m_eventValueMultiplier, "Event value multiplier", "When the event fires, the value will be scaled by this multiplier")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                    ->DataElement(0, &Input::m_deadZone, "Dead zone", "An event will only be sent out if the value is above this threshold")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f);
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflection))
            {
                behaviorContext->EBus<AZ::InputEventNotificationBus>("InputEventNotificationBus")
                    ->Event("OnPressed", &AZ::InputEventNotificationBus::Events::OnPressed)
                    ->Event("OnHeld", &AZ::InputEventNotificationBus::Events::OnHeld)
                    ->Event("OnReleased", &AZ::InputEventNotificationBus::Events::OnReleased);
            }
        }
    }

    Input::Input()
    {
        if (m_inputDeviceType.empty())
        {
            auto&& deviceTypes = GetInputDeviceTypes();
            if (!deviceTypes.empty())
            {
                m_inputDeviceType = deviceTypes[0];
                OnDeviceSelected();
            }
        }
    }

    bool Input::OnInputChannelEventFiltered(const AzFramework::InputChannel& inputChannel)
    {
        const LocalUserId localUserIdOfEvent = static_cast<LocalUserId>(inputChannel.GetInputDevice().GetAssignedLocalUserId());
        const float value = CalculateEventValue(inputChannel);
        const bool isPressed = fabs(value) > m_deadZone;
        if (!m_wasPressed && isPressed)
        {
            SendEventsInternal(value, localUserIdOfEvent, m_outgoingBusId, &AZ::InputEventNotificationBus::Events::OnPressed);
        }
        else if (m_wasPressed && isPressed)
        {
            SendEventsInternal(value, localUserIdOfEvent, m_outgoingBusId, &AZ::InputEventNotificationBus::Events::OnHeld);
        }
        else if (m_wasPressed && !isPressed)
        {
            SendEventsInternal(value, localUserIdOfEvent, m_outgoingBusId, &AZ::InputEventNotificationBus::Events::OnReleased);
        }
        m_wasPressed = isPressed;

        // Return false so we don't consume the event. This should perhaps be a configurable option?
        return false;
    }

    float Input::CalculateEventValue(const AzFramework::InputChannel& inputChannel) const
    {
        return inputChannel.GetValue();
    }

    void Input::SendEventsInternal(float value, const AzFramework::LocalUserId& localUserIdOfEvent, const AZ::InputEventNotificationId busId, InputEventType eventType)
    {
        value *= m_eventValueMultiplier;

        AZ::InputEventNotificationId localUserBusId = AZ::InputEventNotificationId(localUserIdOfEvent, busId.m_actionNameCrc);
        AZ::InputEventNotificationBus::Event(localUserBusId, eventType, value);

        AZ::InputEventNotificationId wildCardBusId = AZ::InputEventNotificationId(AzFramework::LocalUserIdAny, busId.m_actionNameCrc);
        AZ::InputEventNotificationBus::Event(wildCardBusId, eventType, value);
    }

    void Input::Activate(const AZ::InputEventNotificationId& eventNotificationId)
    {
        const AzFramework::InputDevice* inputDevice = AzFramework::InputDeviceRequests::FindInputDevice(AzFramework::InputDeviceId(m_inputDeviceType.c_str()));
        if (!inputDevice || !inputDevice->IsSupported())
        {
            // The input device that this input binding would be listening for input from
            // is not supported on the current platform, so don't bother even activating.
            // Please note distinction between InputDevice::IsSupported and IsConnected.
            return;
        }

        const AZ::Crc32 channelNameFilter(m_inputName.c_str());
        const AZ::Crc32 deviceNameFilter(m_inputDeviceType.c_str());
        const AzFramework::LocalUserId localUserIdFilter(eventNotificationId.m_localUserId);
        AZStd::shared_ptr<InputChannelEventFilterInclusionList> filter = AZStd::make_shared<InputChannelEventFilterInclusionList>(channelNameFilter, deviceNameFilter, localUserIdFilter);
        InputChannelEventListener::SetFilter(filter);
        InputChannelEventListener::Connect();
        m_wasPressed = false;

        m_outgoingBusId = eventNotificationId;
        AZ::GlobalInputRecordRequestBus::Handler::BusConnect();
        AZ::EditableInputRecord editableRecord;
        editableRecord.m_localUserId = eventNotificationId.m_localUserId;
        editableRecord.m_deviceName = m_inputDeviceType;
        editableRecord.m_eventGroup = eventNotificationId.m_actionNameCrc;
        editableRecord.m_inputName = m_inputName;
        AZ::InputRecordRequestBus::Handler::BusConnect(editableRecord);
    }

    void Input::Deactivate(const AZ::InputEventNotificationId& eventNotificationId)
    {
        if (m_wasPressed)
        {
            AZ::InputEventNotificationBus::Event(m_outgoingBusId, &AZ::InputEventNotifications::OnReleased, 0.0f);
        }
        InputChannelEventListener::Disconnect();
        AZ::GlobalInputRecordRequestBus::Handler::BusDisconnect();
        AZ::InputRecordRequestBus::Handler::BusDisconnect();
    }

    AZStd::string Input::GetEditorText() const
    {
        return m_inputName.empty() ? "<Select input>" : m_inputName;
    }

    const AZStd::vector<AZStd::string> Input::GetInputDeviceTypes() const
    {
        AZStd::set<AZStd::string> uniqueInputDeviceTypes;
        AzFramework::InputDeviceRequests::InputDeviceIdSet availableInputDeviceIds;
        AzFramework::InputDeviceRequestBus::Broadcast(&AzFramework::InputDeviceRequests::GetInputDeviceIds,
                                                      availableInputDeviceIds);
        for (const AzFramework::InputDeviceId& inputDeviceId : availableInputDeviceIds)
        {
            uniqueInputDeviceTypes.insert(inputDeviceId.GetName());
        }
        return AZStd::vector<AZStd::string>(uniqueInputDeviceTypes.begin(), uniqueInputDeviceTypes.end());
    }

    const AZStd::vector<AZStd::string> Input::GetInputNamesBySelectedDevice() const
    {
        AZStd::vector<AZStd::string> retval;
        AzFramework::InputDeviceId selectedDeviceId(m_inputDeviceType.c_str());
        AzFramework::InputDeviceRequests::InputChannelIdSet availableInputChannelIds;
        AzFramework::InputDeviceRequestBus::Event(selectedDeviceId,
                                                  &AzFramework::InputDeviceRequests::GetInputChannelIds,
                                                  availableInputChannelIds);
        for (const AzFramework::InputChannelId& inputChannelId : availableInputChannelIds)
        {
            retval.push_back(inputChannelId.GetName());
        }
        AZStd::sort(retval.begin(), retval.end());
        return retval;
    }

    AZ::Crc32 Input::OnDeviceSelected()
    {
        auto&& inputList = GetInputNamesBySelectedDevice();
        if (!inputList.empty())
        {
            m_inputName = inputList[0];
        }
        return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
    }

    void Input::GatherEditableInputRecords(AZ::EditableInputRecords& outResults)
    {
        AZ::EditableInputRecord inputRecord;
        inputRecord.m_deviceName = m_inputDeviceType;
        inputRecord.m_inputName = m_inputName;
        inputRecord.m_eventGroup = m_outgoingBusId.m_actionNameCrc;
        inputRecord.m_localUserId = m_outgoingBusId.m_localUserId;
        outResults.push_back(inputRecord);
    }

    void Input::SetInputRecord(const AZ::EditableInputRecord& newInputRecord)
    {
        Deactivate(m_outgoingBusId);
        m_inputName = newInputRecord.m_inputName;
        m_inputDeviceType = newInputRecord.m_deviceName;
        Activate(m_outgoingBusId);
    }

    ThumbstickInput::ThumbstickInput()
    {
        m_inputDeviceType = AzFramework::InputDeviceGamepad::Name;
        OnDeviceSelected();
    }

    void ThumbstickInput::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<ThumbstickInput, Input>()
                ->Version(1, nullptr)
                    ->Field("Inner Dead Zone Radius", &ThumbstickInput::m_innerDeadZoneRadius)
                    ->Field("Outer Dead Zone Radius", &ThumbstickInput::m_outerDeadZoneRadius)
                    ->Field("Axis Dead Zone Value", &ThumbstickInput::m_axisDeadZoneValue)
                    ->Field("Sensitivity Exponent", &ThumbstickInput::m_sensitivityExponent)
                    ->Field("Output Axis", &ThumbstickInput::m_outputAxis)
            ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<ThumbstickInput>("ThumbstickInput", "Generate events from thumbstick input")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::NameLabelOverride, &ThumbstickInput::GetEditorText)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                    ->DataElement(0, &ThumbstickInput::m_innerDeadZoneRadius, "Inner Dead Zone Radius", "The thumbstick axes vector (x,y) will be normalized between this value and Outer Dead Zone Radius")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->DataElement(0, &ThumbstickInput::m_outerDeadZoneRadius, "Outer Dead Zone Radius", "The thumbstick axes vector (x,y) will be normalized between Inner Dead Zone Radius and this value")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->DataElement(0, &ThumbstickInput::m_axisDeadZoneValue, "Axis Dead Zone Value", "The individual axis values will be normalized between this and 1.0f")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->DataElement(0, &ThumbstickInput::m_sensitivityExponent, "Sensitivity Exponent", "The sensitivity exponent to apply to the normalized thumbstick components")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &ThumbstickInput::m_outputAxis, "Output Axis", "The axis value to output after peforming the dead-zone and sensitivity calculations")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                        ->Attribute(AZ::Edit::Attributes::EnumValues, AZStd::vector<AZ::Edit::EnumConstant<OutputAxis>>
                        {
                            AZ::Edit::EnumConstant<OutputAxis>(OutputAxis::X, "x"),
                            AZ::Edit::EnumConstant<OutputAxis>(OutputAxis::Y, "y")
                        })
                ;
            }
        }
    }

    AZStd::string ThumbstickInput::GetEditorText() const
    {
        return m_inputName.empty() ?
               "<Select input>" :
               m_inputName + (m_outputAxis == OutputAxis::X ? " (x-axis)" : " (y-axis)");
    }

    const AZStd::vector<AZStd::string> ThumbstickInput::GetInputDeviceTypes() const
    {
        // Gamepads are currently the only device type that support thumbstick input.
        // We could (should) be more robust here by iterating over all input devices,
        // looking for any with associated input channels of type InputChannelAxis2D.
        AZStd::vector<AZStd::string> retval;
        retval.push_back(AzFramework::InputDeviceGamepad::Name);
        return retval;
    }

    const AZStd::vector<AZStd::string> ThumbstickInput::GetInputNamesBySelectedDevice() const
    {
        // Gamepads are currently the only device type that support thumbstick input.
        // We could (should) be more robust here by iterating over all input devices,
        // looking for any with associated input channels of type InputChannelAxis2D.
        AZStd::vector<AZStd::string> retval;
        retval.push_back(AzFramework::InputDeviceGamepad::ThumbStickAxis2D::L.GetName());
        retval.push_back(AzFramework::InputDeviceGamepad::ThumbStickAxis2D::R.GetName());
        return retval;
    }

    bool ThumbstickInput::OnInputChannelEventFiltered(const AzFramework::InputChannel& inputChannel)
    {
        // Because we are sending all thumbstick events regardless of if they are inside the dead-zone
        // (see InputChannelAxis2D::ProcessRawInputEvent)
        // ThumbstickInput components can effectively cancel themselves out if they happen to be setup
        // to receive input from a local user id that is signed into multiple controllers at the same
        // time. If the controller not being used is updated last, the (~0, ~0) events it sends every
        // frame cause the base Input::OnInputChannelEventFiltered function to determine that we need
        // to send an InputEventNotificationBus::Events::OnReleased event because m_wasPressed is set
        // to true by the other controller that is actually in use (and that is being updated first).
        //
        // To combat this, anytime we enter the m_wasPressed == true state we'll store a reference to
        // the input device id that sent the event (see below). Each time we receive an event we will
        // then check whether it's originating from the same input device id, and if not we will just
        // ignore it. Please note that while taking the address of the device id is a little sketchy,
        // we can do this because it's lifecycle is guaranteed to be longer than that of this object
        // UNLESS we ever start calling InputSystemComponent::RecreateEnabledInputDevices somewhere.
        //
        // Now in this case, the old InputDevice that owns the InputChannelId will be destroyed, but
        // not before the InputChannels it owns are destroyed first meaning InputChannel::ResetState
        // will be called, the internal state of the input channels will be reset, and an event will
        // be broadcast that will ultimately result in m_wasPressed being set to false and therefore
        // m_wasLastPressedByInputDeviceId being set to nullptr below instead of becoming a dangling
        // pointer. I definitely don't like this much, as it is risky behavior entirely dependent on
        // the internal workings of the AzFramework input system, but it's the fastest way to perform
        // this additional check. The alternative is to store the last pressed InputDeviceId by value,
        // but this would involve a (slightly) more expensive check (see InputDeviceId::operator==),
        // along with a string copy each time we set/reset the value (see InputDeviceId::operator=).
        //
        // At some point it may be worth looking at doing this check (or a safer version of it) in
        // the base Input::OnInputChannelEventFiltered function, because the 'one user logged into
        // multiple controllers' situation could conceivably cause strange behaviour for all types
        // of input bindings (albeit only if the user were to actively use both controllers at the
        // same time, in which case who can even really say what the correct behaviour should be?).
        // But that would be a far riskier change, and because it's only a problem for thumb-stick
        // input we're sending even when the controller is completely idle this fix will do for now.
        const InputDeviceId* inputDeviceId = &(inputChannel.GetInputDevice().GetInputDeviceId());
        if (m_wasLastPressedByInputDeviceId && m_wasLastPressedByInputDeviceId != inputDeviceId)
        {
            return false;
        }

        const bool shouldBeConsumed = Input::OnInputChannelEventFiltered(inputChannel);
        m_wasLastPressedByInputDeviceId = m_wasPressed ? inputDeviceId : nullptr;
        return shouldBeConsumed;
    }

    float ThumbstickInput::CalculateEventValue(const AzFramework::InputChannel& inputChannel) const
    {
        const AzFramework::InputChannelAxis2D::AxisData2D* axisData2D = inputChannel.GetCustomData<AzFramework::InputChannelAxis2D::AxisData2D>();
        if (axisData2D == nullptr)
        {
            AZ_Warning("ThumbstickInput", false, "InputChannel with id '%s' has no axis data 2D", inputChannel.GetInputChannelId().GetName());
            return 0.0f;
        }

        const AZ::Vector2 outputValues = ApplyDeadZonesAndSensitivity(axisData2D->m_preDeadZoneValues,
                                                                      m_innerDeadZoneRadius,
                                                                      m_outerDeadZoneRadius,
                                                                      m_axisDeadZoneValue,
                                                                      m_sensitivityExponent);

        // Ideally we would return both values here and allow each to be mapped to a different output
        // event, but that would require a greater re-factor of both the InputManagementFramework Gem
        // and the StartingPointInput Gem, and there is nothing preventing anyone from setting up one
        // ThumbstickInput component for each axis so it would only be a simplification/optimization.
        const float axisValueToReturn = (m_outputAxis == OutputAxis::X) ? outputValues.GetX() : outputValues.GetY();
        return axisValueToReturn;
    }

    AZ::Vector2 ThumbstickInput::ApplyDeadZonesAndSensitivity(const AZ::Vector2& inputValues, float innerDeadZone, float outerDeadZone, float axisDeadZone, float sensitivityExponent)
    {
        static const AZ::Vector2 zeroVector = AZ::Vector2::CreateZero();
        const AZ::Vector2 rawAbsValues(fabsf(inputValues.GetX()), fabsf(inputValues.GetY()));
        const float rawLength = rawAbsValues.GetLength();
        if (rawLength == 0.0f)
        {
            return zeroVector;
        }

        // Apply the circular dead zones
        const AZ::Vector2 normalizedValues = rawAbsValues / rawLength;
        const float postCircularDeadZoneLength = AZ::GetClamp((rawLength - innerDeadZone) / (outerDeadZone - innerDeadZone), 0.0f, 1.0f);
        AZ::Vector2 absValues = normalizedValues * postCircularDeadZoneLength;

        // Apply the per-axis dead zone
        const AZ::Vector2 absAxisValues = zeroVector.GetMax(rawAbsValues - AZ::Vector2(axisDeadZone, axisDeadZone)) / (outerDeadZone - axisDeadZone);

        // Merge the circular and per-axis dead zones. The resulting values are the smallest ones (dead zone takes priority). And restore the components sign.
        const AZ::Vector2 signValues(AZ::GetSign(inputValues.GetX()), AZ::GetSign(inputValues.GetY()));
        AZ::Vector2 values = absValues.GetMin(absAxisValues) * signValues;

        // Rescale the vector using the post circular dead zone length, which is the real stick vector length,
        // to avoid any jump in values when the stick is fully pushed along an axis and slowly getting out of the axis dead zone
        // Additionally, apply the sensitivity curve to the final stick vector length
        const float postAxisDeadZoneLength = values.GetLength();
        if (postAxisDeadZoneLength > 0.0f)
        {
            values /= postAxisDeadZoneLength;

            const float postSensitivityLength = powf(postCircularDeadZoneLength, sensitivityExponent);
            values *= postSensitivityLength;
        }

        return values;
    }
} // namespace Input
