/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once


namespace AZ::Metrics
{
    // EventValue implementation
    constexpr EventValue::EventValue() = default;

    template<class T, class Alt, class>
    constexpr EventValue::EventValue(T&& value)
        : EventValue(AZStd::in_place_type<Alt>, AZStd::forward<T>(value))
    {}

    template<class T, class... Args>
    constexpr EventValue::EventValue(AZStd::in_place_type_t<T>, Args&&... args)
        : m_value(AZStd::in_place_type<T>, AZStd::forward<Args>(args)...)
    {}

    template<class T, class U, class... Args>
    constexpr EventValue::EventValue(AZStd::in_place_type_t<T>, AZStd::initializer_list<U> iList, Args&&... args)
        : m_value(AZStd::in_place_type<T>, iList, AZStd::forward<Args>(args)...)
    {}


    // EventField implementation
    constexpr EventField::EventField()
    {}

    constexpr EventField::EventField(AZStd::string_view name, EventValue value)
        : m_name(name)
        , m_value(AZStd::move(value))
    {}

    constexpr EventArray::EventArray(AZStd::span<EventValue> arrayFields)
        : m_arrayAddress{ arrayFields.data()}
        , m_arraySize { arrayFields.size()}
    {}

    constexpr void EventArray::SetArrayValues(AZStd::span<EventValue> arrayValues)
    {
        m_arrayAddress = arrayValues.data();
        m_arraySize = arrayValues.size();
    }
    constexpr AZStd::span<EventValue> EventArray::GetArrayValues() const
    {
        return { m_arrayAddress, m_arraySize };
    }

    constexpr EventObject::EventObject(AZStd::span<EventField> objectFields)
        : m_objectAddress{ objectFields.data() }
        , m_objectSize{ objectFields.size() }
    {}

    constexpr void EventObject::SetObjectFields(AZStd::span<EventField> objectFields)
    {
        m_objectAddress = objectFields.data();
        m_objectSize = objectFields.size();
    }
    constexpr AZStd::span<EventField> EventObject::GetObjectFields() const
    {
        return { m_objectAddress, m_objectSize };
    }
} // namespace AZ::Metrics
