/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AWSMetricsConstant.h>
#include <MetricsAttribute.h>

#include <AzCore/std/containers/vector.h>

namespace AWSMetrics
{
    MetricsAttribute::MetricsAttribute(const AZStd::string& name, int intVal)
        : m_val(intVal)
    {
        SetName(name);
    }

    MetricsAttribute::MetricsAttribute(const AZStd::string& name, double doubleVal)
        : m_val(doubleVal)
    {
        SetName(name);
    }

    MetricsAttribute::MetricsAttribute(const AZStd::string& name, const AZStd::string& strVal)
        : m_val(strVal)
    {
        SetName(name);
    }

    MetricsAttribute::MetricsAttribute()
        : m_name("")
        , m_val("")
        , m_isDefault(false)
    {
    }

    void MetricsAttribute::SetName(const AZStd::string& name)
    {
        m_name = name;

        SetIfDefault(name);
    }

    void MetricsAttribute::SetIfDefault(const AZStd::string& name)
    {
        static const AZStd::array<AZStd::string, 7> DefaultAttributeNames =
        {
            AwsMetricsAttributeKeyClientId, AwsMetricsAttributeKeyEventId,
            AwsMetricsAttributeKeyEventName, AwsMetricsAttributeKeyEventType,
            AwsMetricsAttributeKeyEventSource, AwsMetricsAttributeKeyEventTimestamp
        };

        m_isDefault = AZStd::find(DefaultAttributeNames.begin(), DefaultAttributeNames.end(), name) != DefaultAttributeNames.end();
    }

    AZStd::string MetricsAttribute::GetName() const
    {
        return m_name;
    }

    void MetricsAttribute::SetVal(const AZStd::string& val)
    {
        m_val = val;
    }

    void MetricsAttribute::SetVal(int val)
    {
        m_val = val;
    }

    void MetricsAttribute::SetVal(double val)
    {
        m_val = val;
    }

    AZStd::variant<int, double, AZStd::string> MetricsAttribute::GetVal() const
    {
        return m_val;
    }

    bool MetricsAttribute::IsDefault() const
    {
        return m_isDefault;
    }

    size_t MetricsAttribute::GetSizeInBytes() const
    {
        size_t nameSize = m_name.size() * sizeof(char);

        // Calculate the value size based on the value type
        size_t valSize = 0;
        switch (static_cast<VAL_TYPE>(m_val.index()))
        {
            case VAL_TYPE::INT:
                valSize = sizeof(int);
                break;
            case VAL_TYPE::DOUBLE:
                valSize = sizeof(double);
                break;
            default:
                valSize = AZStd::get<AZStd::string>(m_val).size() * sizeof(char);
        }

        return nameSize + valSize;
    }

    bool MetricsAttribute::SerializeToJson(AWSCore::JsonWriter& writer) const
    {
        switch (static_cast<VAL_TYPE>(m_val.index()))
        {
        case VAL_TYPE::INT:
            return writer.Int(AZStd::get<int>(m_val));
        case VAL_TYPE::DOUBLE:
            return writer.Double(AZStd::get<double>(m_val));
        default:
            return writer.String(AZStd::get<AZStd::string>(m_val));
        }
    }

    bool MetricsAttribute::ReadFromJson(const rapidjson::Value& name, const rapidjson::Value& val)
    {
        if (val.IsInt())
        {
            m_val = val.GetInt();
        }
        else if (val.IsDouble())
        {
            m_val = val.GetDouble();
        }
        else if (val.IsString())
        {
            m_val = val.GetString();
        }
        else
        {
            return false;
        }

        SetName(name.GetString());

        return true;
    }
}
