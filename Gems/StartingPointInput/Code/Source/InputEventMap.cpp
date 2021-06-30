/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "StartingPointInput_precompiled.h"
#include "InputEventMap.h"
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

using namespace AzFramework;

namespace StartingPointInput
{
    void InputEventMap::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<InputEventMap>()
                ->Version(2)
                    ->Field("Input Device Type", &InputEventMap::m_inputDeviceType)
                    ->Field("Input Name", &InputEventMap::m_inputName)
                    ->Field("Event Value Multiplier", &InputEventMap::m_eventValueMultiplier)
                    ->Field("Dead Zone", &InputEventMap::m_deadZone);

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<InputEventMap>("InputEventMap", "Maps raw input to a game specific input event")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::NameLabelOverride, &InputEventMap::GetEditorText)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &InputEventMap::m_inputDeviceType, "Input Device Type", "The type of input device, ex keyboard")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &InputEventMap::OnDeviceSelected)
                        ->Attribute(AZ::Edit::Attributes::StringList, &InputEventMap::GetInputDeviceTypes)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &InputEventMap::m_inputName, "Input Name", "The name of the input you want to hold ex. space")
                        ->Attribute(AZ::Edit::Attributes::StringList, &InputEventMap::GetInputNamesBySelectedDevice)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                    ->DataElement(0, &InputEventMap::m_eventValueMultiplier, "Event value multiplier", "When the event fires, the value will be scaled by this multiplier")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                    ->DataElement(0, &InputEventMap::m_deadZone, "Dead zone", "An event will only be sent out if the value is above this threshold")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f);
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflection))
            {
                behaviorContext->EBus<InputEventNotificationBus>("InputEventNotificationBus")
                    ->Event("OnPressed", &InputEventNotificationBus::Events::OnPressed)
                    ->Event("OnHeld", &InputEventNotificationBus::Events::OnHeld)
                    ->Event("OnReleased", &InputEventNotificationBus::Events::OnReleased);
            }
        }
    }

    InputEventMap::InputEventMap()
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

    bool InputEventMap::OnInputChannelEventFiltered(const AzFramework::InputChannel& inputChannel)
    {
        const LocalUserId localUserIdOfEvent = static_cast<LocalUserId>(inputChannel.GetInputDevice().GetAssignedLocalUserId());
        const float value = CalculateEventValue(inputChannel);
        const bool isPressed = fabs(value) > m_deadZone;
        if (!m_wasPressed && isPressed)
        {
            SendEventsInternal(value, localUserIdOfEvent, m_outgoingBusId, &InputEventNotificationBus::Events::OnPressed);
        }
        else if (m_wasPressed && isPressed)
        {
            SendEventsInternal(value, localUserIdOfEvent, m_outgoingBusId, &InputEventNotificationBus::Events::OnHeld);
        }
        else if (m_wasPressed && !isPressed)
        {
            SendEventsInternal(value, localUserIdOfEvent, m_outgoingBusId, &InputEventNotificationBus::Events::OnReleased);
        }
        m_wasPressed = isPressed;

        // Return false so we don't consume the event. This should perhaps be a configurable option?
        return false;
    }

    float InputEventMap::CalculateEventValue(const AzFramework::InputChannel& inputChannel) const
    {
        return inputChannel.GetValue();
    }

    void InputEventMap::SendEventsInternal(float value, const AzFramework::LocalUserId& localUserIdOfEvent, const InputEventNotificationId busId, InputEventType eventType)
    {
        value *= m_eventValueMultiplier;

        InputEventNotificationId localUserBusId = InputEventNotificationId(localUserIdOfEvent, busId.m_actionNameCrc);
        InputEventNotificationBus::Event(localUserBusId, eventType, value);

        InputEventNotificationId wildCardBusId = InputEventNotificationId(AzFramework::LocalUserIdAny, busId.m_actionNameCrc);
        InputEventNotificationBus::Event(wildCardBusId, eventType, value);
    }

    void InputEventMap::Activate(const InputEventNotificationId& eventNotificationId)
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
    }

    void InputEventMap::Deactivate([[maybe_unused]] const InputEventNotificationId& eventNotificationId)
    {
        if (m_wasPressed)
        {
            InputEventNotificationBus::Event(m_outgoingBusId, &InputEventNotifications::OnReleased, 0.0f);
        }
        InputChannelEventListener::Disconnect();
    }

    AZStd::string InputEventMap::GetEditorText() const
    {
        return m_inputName.empty() ? "<Select input>" : m_inputName;
    }

    const AZStd::vector<AZStd::string> InputEventMap::GetInputDeviceTypes() const
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

    const AZStd::vector<AZStd::string> InputEventMap::GetInputNamesBySelectedDevice() const
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

    AZ::Crc32 InputEventMap::OnDeviceSelected()
    {
        auto&& inputList = GetInputNamesBySelectedDevice();
        if (!inputList.empty())
        {
            m_inputName = inputList[0];
        }
        return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
    }

    ThumbstickInputEventMap::ThumbstickInputEventMap()
    {
        m_inputDeviceType = AzFramework::InputDeviceGamepad::Name;
        OnDeviceSelected();
    }

    void ThumbstickInputEventMap::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<ThumbstickInputEventMap, InputEventMap>()
                ->Version(1, nullptr)
                    ->Field("Inner Dead Zone Radius", &ThumbstickInputEventMap::m_innerDeadZoneRadius)
                    ->Field("Outer Dead Zone Radius", &ThumbstickInputEventMap::m_outerDeadZoneRadius)
                    ->Field("Axis Dead Zone Value", &ThumbstickInputEventMap::m_axisDeadZoneValue)
                    ->Field("Sensitivity Exponent", &ThumbstickInputEventMap::m_sensitivityExponent)
                    ->Field("Output Axis", &ThumbstickInputEventMap::m_outputAxis)
            ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<ThumbstickInputEventMap>("ThumbstickInputEventMap", "Generate events from thumbstick input")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::NameLabelOverride, &ThumbstickInputEventMap::GetEditorText)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                    ->DataElement(0, &ThumbstickInputEventMap::m_innerDeadZoneRadius, "Inner Dead Zone Radius", "The thumbstick axes vector (x,y) will be normalized between this value and Outer Dead Zone Radius")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->DataElement(0, &ThumbstickInputEventMap::m_outerDeadZoneRadius, "Outer Dead Zone Radius", "The thumbstick axes vector (x,y) will be normalized between Inner Dead Zone Radius and this value")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->DataElement(0, &ThumbstickInputEventMap::m_axisDeadZoneValue, "Axis Dead Zone Value", "The individual axis values will be normalized between this and 1.0f")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->DataElement(0, &ThumbstickInputEventMap::m_sensitivityExponent, "Sensitivity Exponent", "The sensitivity exponent to apply to the normalized thumbstick components")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &ThumbstickInputEventMap::m_outputAxis, "Output Axis", "The axis value to output after peforming the dead-zone and sensitivity calculations")
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

    AZStd::string ThumbstickInputEventMap::GetEditorText() const
    {
        return m_inputName.empty() ?
               "<Select input>" :
               m_inputName + (m_outputAxis == OutputAxis::X ? " (x-axis)" : " (y-axis)");
    }

    const AZStd::vector<AZStd::string> ThumbstickInputEventMap::GetInputDeviceTypes() const
    {
        // Gamepads are currently the only device type that support thumbstick input.
        // We could (should) be more robust here by iterating over all input devices,
        // looking for any with associated input channels of type InputChannelAxis2D.
        AZStd::vector<AZStd::string> retval;
        retval.push_back(AzFramework::InputDeviceGamepad::Name);
        return retval;
    }

    const AZStd::vector<AZStd::string> ThumbstickInputEventMap::GetInputNamesBySelectedDevice() const
    {
        // Gamepads are currently the only device type that support thumbstick input.
        // We could (should) be more robust here by iterating over all input devices,
        // looking for any with associated input channels of type InputChannelAxis2D.
        AZStd::vector<AZStd::string> retval;
        retval.push_back(AzFramework::InputDeviceGamepad::ThumbStickAxis2D::L.GetName());
        retval.push_back(AzFramework::InputDeviceGamepad::ThumbStickAxis2D::R.GetName());
        return retval;
    }

    bool ThumbstickInputEventMap::OnInputChannelEventFiltered(const AzFramework::InputChannel& inputChannel)
    {
        // Because we are sending all thumbstick events regardless of if they are inside the dead-zone
        // (see InputChannelAxis2D::ProcessRawInputEvent)
        // ThumbstickInputEventMap components can effectively cancel themselves out if they happen to be setup
        // to receive input from a local user id that is signed into multiple controllers at the same
        // time. If the controller not being used is updated last, the (~0, ~0) events it sends every
        // frame cause the base InputEventMap::OnInputChannelEventFiltered function to determine that we need
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
        // the base InputEventMap::OnInputChannelEventFiltered function, because the 'one user logged into
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

        const bool shouldBeConsumed = InputEventMap::OnInputChannelEventFiltered(inputChannel);
        m_wasLastPressedByInputDeviceId = m_wasPressed ? inputDeviceId : nullptr;
        return shouldBeConsumed;
    }

    float ThumbstickInputEventMap::CalculateEventValue(const AzFramework::InputChannel& inputChannel) const
    {
        const AzFramework::InputChannelAxis2D::AxisData2D* axisData2D = inputChannel.GetCustomData<AzFramework::InputChannelAxis2D::AxisData2D>();
        if (axisData2D == nullptr)
        {
            AZ_Warning("ThumbstickInputEventMap", false, "InputChannel with id '%s' has no axis data 2D", inputChannel.GetInputChannelId().GetName());
            return 0.0f;
        }

        const AZ::Vector2 outputValues = ApplyDeadZonesAndSensitivity(axisData2D->m_preDeadZoneValues,
                                                                      m_innerDeadZoneRadius,
                                                                      m_outerDeadZoneRadius,
                                                                      m_axisDeadZoneValue,
                                                                      m_sensitivityExponent);

        // Ideally we would return both values here and allow each to be mapped to a different output
        // event, but that would require a greater re-factor of the StartingPointInput Gem, and there
        // is nothing preventing anyone from setting up one ThumbstickInputEventMap component for each
        // axis so it would only be a simplification/optimization.
        const float axisValueToReturn = (m_outputAxis == OutputAxis::X) ? outputValues.GetX() : outputValues.GetY();
        return axisValueToReturn;
    }

    AZ::Vector2 ThumbstickInputEventMap::ApplyDeadZonesAndSensitivity(const AZ::Vector2& inputValues, float innerDeadZone, float outerDeadZone, float axisDeadZone, float sensitivityExponent)
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
