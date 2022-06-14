/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Framework/JsonWriter.h>

#include <AzCore/JSON/document.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/std/containers/variant.h>
#include <AzCore/std/string/string.h>

namespace AWSMetrics
{
    //! MetricsAttribute represents one attribute of the metrics.
    //! Attribute value can be int, double or string.
    //! e.g. name: event_name, value: login
    class MetricsAttribute
    {
    public:
        AZ_TYPE_INFO(MetricsAttribute, "{6483F481-0C18-4171-8B59-A44F2F28EAE5}")

        MetricsAttribute(const AZStd::string& name, int intVal);
        MetricsAttribute(const AZStd::string& name, double doubleVal);
        MetricsAttribute(const AZStd::string& name, const AZStd::string& strVal);
        MetricsAttribute();

        void SetName(const AZStd::string& name);
        AZStd::string GetName() const;

        void SetVal(const AZStd::string& val);
        void SetVal(int val);
        void SetVal(double val);

        AZStd::variant<int, double, AZStd::string> GetVal() const;

        //! Get the metrics attribute size serialized to json.
        //! @return metrics attribute size in bytes.
        size_t GetSizeInBytes() const;

        //! Check whether the attribute is one of the default attributes.
        //! @return Whether the attribute is a default one.
        bool IsDefault() const;

        //! Serialize the metrics attribute to JSON for sending requests.
        //! @param writer JSON writer for the serialization.
        //! @return Whether the metrics attribute is serialized successfully.
        bool SerializeToJson(AWSCore::JsonWriter& writer) const;

        //! Read from a JSON value to the metrics attribute.
        //! @param name JSON value used for the attribute name.
        //! @param name JSON value used for the attribute value.
        //! @return Whether the metrics attribute is created successfully
        bool ReadFromJson(const rapidjson::Value& name, const rapidjson::Value& val);

    private:
        //! Set the attribute as default if its name is in the default attributes list.
        //! @param name Name of the attribute.
        void SetIfDefault(const AZStd::string& name);

        enum class VAL_TYPE
        {
            INT,
            DOUBLE,
            STR
        };

        AZStd::string m_name; //!< Name of the attribute.
        AZStd::variant<int, double, AZStd::string> m_val; //!< Value of the attribute.
        bool m_isDefault; //!< Whether the attribute is one of the default attributes.
    };
} // namespace AWSMetrics
