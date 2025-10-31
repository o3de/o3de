/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/Streamer/Statistics.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/std/string/conversions.h>

namespace AZ::IO
{
    template<typename T>
    Statistic Statistic::Create(
        AZStd::string_view owner, AZStd::string_view name, T&& value, AZStd::string_view description, GraphType graphType)
    {
        Statistic result;
        result.m_owner = owner;
        result.m_name = name;
        result.m_description = description;
        result.m_value = AZStd::forward<T>(value);
        result.m_graphType = graphType;
        return result;
    }
    
    Statistic Statistic::CreateBoolean(
        AZStd::string_view owner, AZStd::string_view name, bool value, AZStd::string_view description, GraphType graphType)
    {
        return Create(owner, name, value, description, graphType);
    }

    Statistic Statistic::CreateFloat(
        AZStd::string_view owner, AZStd::string_view name, double value, AZStd::string_view description, GraphType graphType)
    {
        return Create(owner, name, value, description, graphType);
    }

    Statistic Statistic::CreateFloatRange(
        AZStd::string_view owner,
        AZStd::string_view name,
        double value,
        double min,
        double max,
        AZStd::string_view description,
        GraphType graphType)
    {
        return Create(owner, name, FloatRange{ value, min, max }, description, graphType);
    }

    Statistic Statistic::CreateInteger(
        AZStd::string_view owner, AZStd::string_view name, s64 value, AZStd::string_view description, GraphType graphType)
    {
        return Create(owner, name, value, description, graphType);
    }

    Statistic Statistic::CreateIntegerRange(
        AZStd::string_view owner, AZStd::string_view name, s64 value, s64 min, s64 max, AZStd::string_view description, GraphType graphType)
    {
        return Create(owner, name, IntegerRange{ value, min, max }, description, graphType);
    }

    Statistic Statistic::CreatePercentage(
        AZStd::string_view owner, AZStd::string_view name, double value, AZStd::string_view description, GraphType graphType)
    {
        return Create(owner, name, Percentage{ value }, description, graphType);
    }

    Statistic Statistic::CreatePercentageRange(
        AZStd::string_view owner,
        AZStd::string_view name,
        double value,
        double min,
        double max,
        AZStd::string_view description,
        GraphType graphType)
    {
        return Create(owner, name, PercentageRange{ value, min, max }, description, graphType);
    }

    Statistic Statistic::CreateByteSize(
        AZStd::string_view owner, AZStd::string_view name, u64 value, AZStd::string_view description, GraphType graphType)
    {
        return Create(owner, name, ByteSize{ value }, description, graphType);
    }

    Statistic Statistic::CreateByteSizeRange(
        AZStd::string_view owner, AZStd::string_view name, u64 value, u64 min, u64 max, AZStd::string_view description, GraphType graphType)
    {
        return Create(owner, name, ByteSizeRange{ value, min, max }, description, graphType);
    }

    Statistic Statistic::CreateTime(
        AZStd::string_view owner,
        AZStd::string_view name,
        TimeValue value,
        AZStd::string_view description,
        GraphType graphType)
    {
        return Create(owner, name, Time{ value }, description, graphType);
    }

    Statistic Statistic::CreateTimeRange(
        AZStd::string_view owner,
        AZStd::string_view name,
        TimeValue value,
        TimeValue min,
        TimeValue max,
        AZStd::string_view description,
        GraphType graphType)
    {
        return Create(owner, name, TimeRange{ value, min, max }, description, graphType);
    }

    Statistic Statistic::CreateBytesPerSecond(
        AZStd::string_view owner,
        AZStd::string_view name,
        double value,
        AZStd::string_view description,
        GraphType graphType)
    {
        return Create(owner, name, BytesPerSecond{ value }, description, graphType);
    }

    Statistic Statistic::CreatePersistentString(
        AZStd::string_view owner, AZStd::string_view name, AZStd::string value, AZStd::string_view description)
    {
        return Create(owner, name, AZStd::move(value), description, GraphType::None);
    }

    Statistic Statistic::CreateReferenceString(
        AZStd::string_view owner, AZStd::string_view name, AZStd::string_view value, AZStd::string_view description)
    {
        return Create(owner, name, value, description, GraphType::None);
    }

    void Statistic::PlotImmediate(
        [[maybe_unused]] AZStd::string_view owner,
        [[maybe_unused]] AZStd::string_view name,
        [[maybe_unused]] double value)
    {
        // AZ_PROFILE_DATAPOINT requires a wstring as input, but you can't feed non-wstring parameters to wstring::format.
        const size_t MaxStatNameLength = 256;
        wchar_t statBufferStack[MaxStatNameLength];
        statBufferStack[MaxStatNameLength - 1] = 0; // make sure it is null terminated

        AZStd::to_wstring(statBufferStack, MaxStatNameLength - 1,
            AZStd::string::format("Streamer/%.*s/%.*s (Raw)",
                aznumeric_cast<int>(owner.size()), owner.data(),
                aznumeric_cast<int>(name.size()), name.data()));
        
        AZ_PROFILE_DATAPOINT(AzCore, value,statBufferStack);
    }

    AZStd::string_view Statistic::GetOwner() const
    {
        return m_owner;
    }

    AZStd::string_view Statistic::GetName() const
    {
        return m_name;
    }

    AZStd::string_view Statistic::GetDescription() const
    {
        return m_description;
    }

    auto Statistic::GetValue() const -> const Value&
    {
        return m_value;
    }

    auto Statistic::GetGraphType() const -> GraphType
    {
        return m_graphType;
    }
} // namespace AZ::IO
