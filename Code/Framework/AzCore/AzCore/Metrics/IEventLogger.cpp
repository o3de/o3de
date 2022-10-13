/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Metrics/IEventLogger.h>

namespace AZ::Metrics
{
    EventDesc::EventDesc()
        : m_argsField{ ArgsKey, EventValue{ AZStd::in_place_type<EventObject> } }
    {
    }

    //! EventDesc implemetnation
    void EventDesc::SetName(AZStd::string_view name)
    {
        m_name = name;
    }
    AZStd::string_view EventDesc::GetName() const
    {
        return m_name;
    }

    void EventDesc::SetCategory(AZStd::string_view category)
    {
        m_cat = category;
    }
    AZStd::string_view EventDesc::GetCategory() const
    {
        return m_cat;
    }

    void EventDesc::SetEventPhase(EventPhase phase)
    {
        m_phase = phase;
    }
    EventPhase EventDesc::GetEventPhase() const
    {
        return m_phase;
    }

    void EventDesc::SetId(AZStd::optional<AZStd::string_view> id)
    {
        m_id = id;
    }
    AZStd::optional<AZStd::string_view> EventDesc::GetId() const
    {
        return m_id;
    }

    void EventDesc::SetTimestamp(AZStd::chrono::microseconds timestamp)
    {
        m_ts = timestamp;
    }
    AZStd::chrono::microseconds EventDesc::GetTimestamp() const
    {
        return m_ts;
    }

    void EventDesc::SetThreadTimestamp(AZStd::optional<AZStd::chrono::microseconds> threadTimestamp)
    {
        m_tts = threadTimestamp;
    }
    AZStd::optional<AZStd::chrono::microseconds> EventDesc::GetThreadTimestamp() const
    {
        return m_tts;
    }

    void EventDesc::SetProcessId(AZ::Platform::ProcessId processId)
    {
        m_pid = processId;
    }
    AZ::Platform::ProcessId EventDesc::GetProcessId() const
    {
        return m_pid;
    }

    void EventDesc::SetThreadId(AZStd::thread::id threadId)
    {
        m_tid = threadId;
    }
    AZStd::thread::id EventDesc::GetThreadId() const
    {
        return m_tid;
    }

    void EventDesc::SetArgs(AZStd::span<EventField> argsFields)
    {
        AZ_Assert(AZStd::holds_alternative<EventObject>(m_argsField.m_value.m_value), R"(The "args" field on the event description should be a JSON Object )");
        if (auto eventObject = AZStd::get_if<EventObject>(&m_argsField.m_value.m_value);
            eventObject != nullptr)
        {
            eventObject->SetObjectFields(argsFields);
        }
    }
    EventObject& EventDesc::GetArgs() &
    {
        AZ_Assert(AZStd::holds_alternative<EventObject>(m_argsField.m_value.m_value), R"(The "args" field on the event description should be a JSON Object )");
        return AZStd::get<EventObject>(m_argsField.m_value.m_value);
    }
    const EventObject& EventDesc::GetArgs() const&
    {
        return AZStd::get<EventObject>(m_argsField.m_value.m_value);
    }
    EventObject&& EventDesc::GetArgs() &&
    {
        return AZStd::get<EventObject>(AZStd::move(m_argsField.m_value.m_value));
    }

    void EventDesc::SetExtraParams(AZStd::span<EventField> extraParams)
    {
        m_extraParams = extraParams;
    }
    AZStd::span<EventField> EventDesc::GetExtraParams() const
    {
        return m_extraParams;
    }
} // namespace AZ::Metrics
