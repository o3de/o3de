/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/Streamer/Statistics.h>
#include <AzCore/Debug/Profiler.h>

namespace AZ
{
    namespace IO
    {
        Statistic Statistic::CreateFloat(AZStd::string_view owner, AZStd::string_view name, double value)
        {
            Statistic result;
            result.m_owner = owner;
            result.m_name = name;
            result.m_value.m_floatingPoint = value;
            result.m_type = Type::FloatingPoint;
            return result;
        }

        Statistic Statistic::CreateInteger(AZStd::string_view owner, AZStd::string_view name, s64 value)
        {
            Statistic result;
            result.m_owner = owner;
            result.m_name = name;
            result.m_value.m_integer = value;
            result.m_type = Type::Integer;
            return result;
        }

        Statistic Statistic::CreatePercentage(AZStd::string_view owner, AZStd::string_view name, double value)
        {
            Statistic result;
            result.m_owner = owner;
            result.m_name = name;
            result.m_value.m_floatingPoint = value;
            result.m_type = Type::Percentage;
            return result;
        }

        void Statistic::PlotImmediate(
            [[maybe_unused]] AZStd::string_view owner,
            [[maybe_unused]] AZStd::string_view name,
            [[maybe_unused]] double value)
        {
            AZ_PROFILE_DATAPOINT(AzCore, value,
                "Streamer/%.*s/%.*s (Raw)",
                aznumeric_cast<int>(owner.size()), owner.data(),
                aznumeric_cast<int>(name.size()), name.data());
        }

        Statistic::Statistic(const Statistic& rhs)
            : m_owner(rhs.m_owner)
            , m_name(rhs.m_name)
            , m_type(rhs.m_type)
        {
            memcpy(&m_value, &rhs.m_value, sizeof(m_value));
        }

        Statistic::Statistic(Statistic&& rhs)
            : m_owner(AZStd::move(rhs.m_owner))
            , m_name(AZStd::move(rhs.m_name))
            , m_type(rhs.m_type)
        {
            memcpy(&m_value, &rhs.m_value, sizeof(m_value));
        }

        Statistic& Statistic::operator=(const Statistic& rhs)
        {
            if (this != &rhs)
            {
                m_owner = rhs.m_owner;
                m_name = rhs.m_name;
                m_type = rhs.m_type;
                memcpy(&m_value, &rhs.m_value, sizeof(m_value));
            }
            return *this;
        }

        Statistic& Statistic::operator=(Statistic&& rhs)
        {
            if (this != &rhs)
            {
                m_owner = AZStd::move(rhs.m_owner);
                m_name = AZStd::move(rhs.m_name);
                m_type = rhs.m_type;
                memcpy(&m_value, &rhs.m_value, sizeof(m_value));
            }
            return *this;
        }

        AZStd::string_view Statistic::GetOwner() const
        {
            return m_owner;
        }

        AZStd::string_view Statistic::GetName() const
        {
            return m_name;
        }

        Statistic::Type Statistic::GetType() const
        {
            return m_type;
        }

        double Statistic::GetFloatValue() const
        {
            AZ_Assert(m_type == Type::FloatingPoint, "Trying to get a floating point value from a statistic that doesn't store a floating point value.");
            return m_value.m_floatingPoint;
        }

        s64 Statistic::GetIntegerValue() const
        {
            AZ_Assert(m_type == Type::Integer, "Trying to get a integer value from a statistic that doesn't store a integer value.");
            return m_value.m_integer;
        }

        double Statistic::GetPercentage() const
        {
            AZ_Assert(m_type == Type::Percentage, "Trying to get a percentage value from a statistic that doesn't store a percentage value.");
            return m_value.m_floatingPoint * 100.0;
        }
    } // namespace IO
} // namespace AZ
