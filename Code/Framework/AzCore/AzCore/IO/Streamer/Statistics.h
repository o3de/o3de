/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/containers/variant.h>
#include <AzCore/std/typetraits/is_arithmetic.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>

namespace AZ::IO
{
    class Statistic
    {
    public:
        using TimeValue = AZStd::chrono::microseconds;

        enum class GraphType : u8
        {
            None,
            Lines,
            Histogram
        };

        struct FloatRange
        {
            AZ_TYPE_INFO(AZ::IO::Statistic::FloatRange, "{5B63F857-16C5-44C9-A5DB-3122FA06B775}");
            double m_value;
            double m_min;
            double m_max;
        };
        struct IntegerRange
        {
            AZ_TYPE_INFO(AZ::IO::Statistic::IntegerRange, "{BABF319D-43D7-44A4-9DC1-08565CF8A554}");
            s64 m_value;
            s64 m_min;
            s64 m_max;
        };
        struct Percentage
        {
            AZ_TYPE_INFO(AZ::IO::Statistic::Percentage, "{F09F7416-8CBA-48FE-B724-FFE49C2A2D12}");
            double m_value;
        };
        struct PercentageRange
        {
            AZ_TYPE_INFO(AZ::IO::Statistic::PercentageRange, "{E5686477-A1FB-41FB-80E5-EC20B4EC5972}");
            double m_value;
            double m_min;
            double m_max;
        };
        struct ByteSize
        {
            AZ_TYPE_INFO(AZ::IO::Statistic::ByteSize, "{7AFA2881-44A3-46F0-8893-A588377DFC07}");
            u64 m_value;
        };
        struct ByteSizeRange
        {
            AZ_TYPE_INFO(AZ::IO::Statistic::ByteSizeRange, "{740E1196-9F9B-4868-BC73-6ED1249C036C}");
            u64 m_value;
            u64 m_min;
            u64 m_max;
        };
        struct Time
        {
            AZ_TYPE_INFO(AZ::IO::Statistic::Time, "{8E7AFC65-6653-45D2-A952-0A5D14CC5C45}");
            TimeValue m_value;
        };
        struct TimeRange
        {
            AZ_TYPE_INFO(AZ::IO::Statistic::TimeRange, "{7DB3C1AB-30E7-458F-835C-8F84075AD9A7}");
            TimeValue m_value;
            TimeValue m_min;
            TimeValue m_max;
        };
        struct BytesPerSecond
        {
            AZ_TYPE_INFO(AZ::IO::Statistic::BytesPerSecond, "{ADE39EB4-1040-43EB-B0A8-CD20CD011C5E}");
            double m_value;
        };

        using Value = AZStd::variant<
            AZStd::monostate,
            bool,
            double, FloatRange,
            s64, IntegerRange,
            Percentage, PercentageRange,
            ByteSize, ByteSizeRange,
            Time, TimeRange,
            BytesPerSecond,
            AZStd::string, AZStd::string_view>;

        static Statistic CreateBoolean(
            AZStd::string_view owner,
            AZStd::string_view name,
            bool value,
            AZStd::string_view description = "",
            GraphType graphType = GraphType::Histogram);

        static Statistic CreateFloat(
            AZStd::string_view owner,
            AZStd::string_view name,
            double value,
            AZStd::string_view description = "",
            GraphType graphType = GraphType::Lines);

        static Statistic CreateFloatRange(
            AZStd::string_view owner,
            AZStd::string_view name,
            double value,
            double min,
            double max,
            AZStd::string_view description = "",
            GraphType graphType = GraphType::Lines);

        static Statistic CreateInteger(
            AZStd::string_view owner,
            AZStd::string_view name,
            s64 value,
            AZStd::string_view description = "",
            GraphType graphType = GraphType::Lines);

        static Statistic CreateIntegerRange(
            AZStd::string_view owner,
            AZStd::string_view name,
            s64 value,
            s64 min,
            s64 max,
            AZStd::string_view description = "",
            GraphType graphType = GraphType::Lines);

        static Statistic CreatePercentage(
            AZStd::string_view owner,
            AZStd::string_view name,
            double value,
            AZStd::string_view description = "",
            GraphType graphType = GraphType::Lines);

        static Statistic CreatePercentageRange(
            AZStd::string_view owner,
            AZStd::string_view name,
            double value,
            double min,
            double max,
            AZStd::string_view description = "",
            GraphType graphType = GraphType::Lines);

        static Statistic CreateByteSize(
            AZStd::string_view owner,
            AZStd::string_view name,
            u64 value,
            AZStd::string_view description = "",
            GraphType graphType = GraphType::Histogram);

        static Statistic CreateByteSizeRange(
            AZStd::string_view owner,
            AZStd::string_view name,
            u64 value,
            u64 min,
            u64 max,
            AZStd::string_view description = "",
            GraphType graphType = GraphType::Histogram);

        static Statistic CreateTime(
            AZStd::string_view owner,
            AZStd::string_view name,
            TimeValue value,
            AZStd::string_view description = "",
            GraphType graphType = GraphType::Lines);

        static Statistic CreateTimeRange(
            AZStd::string_view owner,
            AZStd::string_view name,
            TimeValue value,
            TimeValue min,
            TimeValue max,
            AZStd::string_view description = "",
            GraphType graphType = GraphType::Lines);

        static Statistic CreateBytesPerSecond(
            AZStd::string_view owner,
            AZStd::string_view name,
            double value,
            AZStd::string_view description = "",
            GraphType graphType = GraphType::Histogram);

        static Statistic CreatePersistentString(
            AZStd::string_view owner, AZStd::string_view name, AZStd::string value, AZStd::string_view description = "");
        static Statistic CreateReferenceString(
            AZStd::string_view owner, AZStd::string_view name, AZStd::string_view value, AZStd::string_view description = "");
        static void PlotImmediate(AZStd::string_view owner, AZStd::string_view name, double value);

        AZStd::string_view GetOwner() const;
        AZStd::string_view GetName() const;
        AZStd::string_view GetDescription() const;
        const Value& GetValue() const;
        GraphType GetGraphType() const;

    private:
        Statistic() = default;

        template<typename T>
        static Statistic Create(
            AZStd::string_view owner, AZStd::string_view name, T&& value, AZStd::string_view description, GraphType graphType);

        Value m_value;
        AZStd::string_view m_owner;
        AZStd::string_view m_name;
        AZStd::string_view m_description;
        GraphType m_graphType;
    };

    namespace Statistics::Internal
    {
        template<class T>
        inline constexpr bool is_duration = false;
        template<class Rep, class Period>
        inline constexpr bool is_duration<AZStd::chrono::duration<Rep, Period>> = true;
    }

    //! AverageWindow keeps track of the average of values in a sliding window.
    //! @StorageType The type of the value in the sliding window. Need to be a number of a AZStd::chrono::duration.
    //! @AverageType The type CalculateAverage will return. StorageType needs to be able to be converted to AverageType.
    //! @WindowSize The maximum number of entries kept in the window.
    template<typename StorageType, typename AverageType, size_t WindowSize>
    class AverageWindow
    {
        static_assert(AZStd::is_arithmetic_v<StorageType> || Statistics::Internal::is_duration<StorageType>,
            "AverageWindow only support numbers and AZStd::chrono::durations.");
        static_assert(
            AZStd::is_convertible_v<StorageType, AverageType>,
            "The storage type for the AverageWindow needs to be convertible to the average type.");
        static_assert(IsPowerOfTwo(WindowSize), "The WindowSize of AverageWindow needs to be a power of 2.");
    public:
        static const size_t s_windowSize = WindowSize;

        AverageWindow()
        {
            memset(&m_values, 0, sizeof(m_values));
            memset(&m_runningTotal, 0, sizeof(m_runningTotal));
            if constexpr (AZStd::is_same_v<StorageType, Statistic::TimeValue>)
            {
                m_minValue = Statistic::TimeValue::max();
                m_maxValue = Statistic::TimeValue::min();
            }
        }

        //! Push a new entry into the window. If the window is full, the oldest value will be removed.
        void PushEntry(StorageType value)
        {
            size_t index = m_count & (WindowSize - 1);
            m_runningTotal += value;
            m_runningTotal -= m_values[index];
            m_values[index] = value;
            ++m_count;

            m_minValue = AZStd::min(m_minValue, value);
            m_maxValue = AZStd::max(m_maxValue, value);
        }

        //! Calculates the average value for the window.
        AverageType CalculateAverage() const
        {
            size_t count = m_count > 0 ? m_count : 1;
            return static_cast<AverageType>(static_cast<AverageType>(m_runningTotal) / static_cast<AverageType>(count < WindowSize ? count : WindowSize));
        }
        //! Gets the running total of the values in the window.
        StorageType GetTotal() const { return m_runningTotal; }
        //! Returns the smallest value that was recorded.
        StorageType GetMinimum() const { return m_minValue; }
        //! Returns the largest value that was recorded.
        StorageType GetMaximum() const { return m_maxValue; }
        //! Returns the total number of entries that have been passed in. This is not the total number of entries that are stored however.
        size_t GetNumRecorded() const { return m_count; }

    private:
        StorageType m_values[WindowSize];
        StorageType m_minValue = AZStd::numeric_limits<StorageType>::max();
        StorageType m_maxValue = AZStd::numeric_limits<StorageType>::min();
        StorageType m_runningTotal;
        size_t m_count = 0;
    };

    template<size_t WindowSize>
    using TimedAverageWindow = AverageWindow<Statistic::TimeValue, Statistic::TimeValue, WindowSize>;

    template<size_t WindowSize>
    class TimedAverageWindowScope
    {
    public:
        explicit TimedAverageWindowScope(TimedAverageWindow<WindowSize>& window)
            : m_window(window)
        {
            m_startTime = AZStd::chrono::steady_clock::now();
        }

        ~TimedAverageWindowScope()
        {
            AZStd::chrono::steady_clock::time_point now = AZStd::chrono::steady_clock::now();
            Statistic::TimeValue duration = AZStd::chrono::duration_cast<Statistic::TimeValue>(now - m_startTime);
            m_window.PushEntry(duration);
        }

    private:
        TimedAverageWindow<WindowSize>& m_window;
        AZStd::chrono::steady_clock::time_point m_startTime;
    };

#define TIMED_AVERAGE_WINDOW_SCOPE(window) TimedAverageWindowScope<decltype(window)::s_windowSize> TIMED_AVERAGE_WINDOW##__COUNTER__ (window)

} // namespace AZ::IO
