/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <StartingPointInput_precompiled.h>
#include <InputNode.h>

#include <AzCore/EBus/EBus.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace StartingPointInput
{
    void InputNode::OnPostActivate()
    {
        //if (GetExecutionType() == ScriptCanvas::ExecutionType::Runtime)
        //{
        //    const ScriptCanvas::SlotId eventNameSlotId = InputNodeProperty::GetEventNameSlotId(this);

        //    m_eventName = (*(FindDatum(eventNameSlotId)->GetAs<ScriptCanvas::Data::StringType>()));
        //    InputEventNotificationBus::Handler::BusConnect(InputEventNotificationId(m_eventName.c_str()));
        //}
    }

    void InputNode::OnDeactivate()
    {
        InputEventNotificationBus::Handler::BusDisconnect();
    }

    void InputNode::OnInputChanged(const ScriptCanvas::Datum& input, const ScriptCanvas::SlotId& slotId)
    {
        // we got a new event name, we need to drop our connection to the old and connect to the new event
        const ScriptCanvas::SlotId eventNameSlotId = InputNodeProperty::GetEventNameSlotId(this);

        if (slotId == eventNameSlotId)
        {
            InputEventNotificationBus::Handler::BusDisconnect();

            m_eventName = (*input.GetAs<ScriptCanvas::Data::StringType>());
            InputEventNotificationBus::Handler::BusConnect(InputEventNotificationId(m_eventName.c_str()));
        }
    }
    
    void InputNode::OnPressed(float value)
    {
        m_value = value;
        const ScriptCanvas::Datum output = ScriptCanvas::Datum(m_value);
        const ScriptCanvas::SlotId pressedSlotId = InputNodeProperty::GetPressedSlotId(this);
        const ScriptCanvas::SlotId valueId = InputNodeProperty::GetValueSlotId(this);

        if (auto* slot = GetSlot(valueId))
        {
            PushOutput(output, *slot);
        }

        SignalOutput(pressedSlotId);
    }

    void InputNode::OnHeld(float value)
    {
        m_value = value;
        const ScriptCanvas::Datum output = ScriptCanvas::Datum(m_value);
        const ScriptCanvas::SlotId heldSlotId = InputNodeProperty::GetHeldSlotId(this);
        const ScriptCanvas::SlotId valueId = InputNodeProperty::GetValueSlotId(this);
        if (auto* slot = GetSlot(valueId))
        {
            PushOutput(output, *slot);
        }
        SignalOutput(heldSlotId);
    }

    void InputNode::OnReleased(float value)
    {
        m_value = value;
        const ScriptCanvas::Datum output = ScriptCanvas::Datum(m_value);
        const ScriptCanvas::SlotId releasedSlotId = InputNodeProperty::GetReleasedSlotId(this);
        const ScriptCanvas::SlotId valueId = InputNodeProperty::GetValueSlotId(this);

        if (auto* slot = GetSlot(valueId))
        {
            PushOutput(output, *slot);
        }
        SignalOutput(releasedSlotId);
    }
}
