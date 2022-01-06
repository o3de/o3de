/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <MetricsEvent.h>
#include <AWSMetricsConstant.h>

#include <AzCore/JSON/schema.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/JsonUtils.h>

#include <sstream>

namespace AWSMetrics
{
    void MetricsEvent::AddAttribute(const MetricsAttribute& attribute)
    {
        AZStd::string attributeName = attribute.GetName();
        if (attributeName.empty())
        {
            AZ_Error("AWSMetrics", false, "Invalid metrics attribute. Attribute name is empty.");
            return;
        }
        if (AttributeExists(attributeName))
        {
            // Avoid overwriting the existing attribute value since it's not clear which one developers need to keep.
            AZ_Error("AWSMetrics", false, "Metrics attribute %s already exists.", attributeName.c_str());
            return;
        }

        m_sizeSerializedToJson += attribute.GetSizeInBytes();

        m_attributes.emplace_back(AZStd::move(attribute));
    }

    bool MetricsEvent::AttributeExists(const AZStd::string& attributeName) const
    {
        auto itr = AZStd::find_if(m_attributes.begin(), m_attributes.end(),
            [attributeName](const MetricsAttribute& existingAttribute) -> bool
        {
            return (attributeName == existingAttribute.GetName());
        });

        return itr != m_attributes.end();
    }

    void MetricsEvent::AddAttributes(const AZStd::vector<MetricsAttribute>& attributes)
    {
        for (const MetricsAttribute& attribute : attributes)
        {
            AddAttribute(attribute);
        }
    }

    int MetricsEvent::GetNumAttributes() const
    {
        return static_cast<int>(m_attributes.size());
    }

    size_t MetricsEvent::GetSizeInBytes() const
    {
        return m_sizeSerializedToJson;
    }

    bool MetricsEvent::SerializeToJson(AWSCore::JsonWriter& writer) const
    {
        bool ok = true;
        ok = ok && writer.StartObject();

        AZStd::vector<MetricsAttribute> customAttributes;
        for (const auto& attr : m_attributes)
        {
            if (attr.IsDefault())
            {
                ok = ok && writer.Key(attr.GetName().c_str());
                ok = ok && attr.SerializeToJson(writer);
            }
            else
            {
                customAttributes.emplace_back(attr);
            }
        }

        if (customAttributes.size() > 0)
        {
            // Wrap up the cutom event attributes in a separate event_data field
            ok = ok && writer.Key(AwsMetricsAttributeKeyEventData);
            ok = ok && writer.StartObject();
            for (const auto& attr : customAttributes)
            {
                ok = ok && writer.Key(attr.GetName().c_str());
                ok = ok && attr.SerializeToJson(writer);
            }
            ok = ok && writer.EndObject();
        }

        ok = ok && writer.EndObject();
        return ok;
    }

    bool MetricsEvent::ReadFromJson(rapidjson::Value& metricsObjVal)
    {
        if (!metricsObjVal.IsObject())
        {
            AZ_Error("AWSMetrics", false, "Invalid JSON value type. Expect an object");
            return false;
        }

        int attributeIndex = 0;
        for (auto it = metricsObjVal.MemberBegin(); it != metricsObjVal.MemberEnd(); ++it, ++attributeIndex)
        {
            if (strcmp(it->name.GetString(), AwsMetricsAttributeKeyEventData) == 0)
            {
                // The event_data field contains a flat json dictionary.
                // Read the JSON value of this field to add all the custom metrics attributes.
                if (!ReadFromJson(it->value))
                {
                    return false;
                }
            }
            else
            {
                MetricsAttribute attribute;
                // Read through each element in the array and add it as a new metrics attribute
                if (!attribute.ReadFromJson(it->name, it->value))
                {
                    AZ_Error("AWSMetrics", false, "Metrics attribute %s is invalid", it->name.GetString());
                    return false;
                }

                AddAttribute(attribute);
            }
        }

        return true;
    }

    bool MetricsEvent::ValidateAgainstSchema()
    {
        std::stringstream stringStream;
        AWSCore::JsonOutputStream jsonStream{stringStream};
        AWSCore::JsonWriter writer{jsonStream};

        if (!SerializeToJson(writer))
        {
            return false;
        }

        auto result = AZ::JsonSerializationUtils::ReadJsonString(stringStream.str().c_str());
        if (!result.IsSuccess())
        {
            return false;
        }

        rapidjson::Document jsonSchemaDocument;
        if (jsonSchemaDocument.Parse(AwsMetricsEventJsonSchema).HasParseError())
        {
            AZ_Error("AWSMetrics", false, "Invalid metrics event json schema.");
            return false;
        }

        auto jsonSchema = rapidjson::SchemaDocument(jsonSchemaDocument);
        rapidjson::SchemaValidator validator(jsonSchema);

        if (!result.GetValue().Accept(validator))
        {
            rapidjson::StringBuffer error;
            validator.GetInvalidSchemaPointer().StringifyUriFragment(error);
            AZ_Warning("AWSMetrics", false, "Failed to load the metrics event, invalid schema: %s.", error.GetString());
            AZ_Warning("AWSMetrics", false, "Failed to load the metrics event, invalid keyword: %s.", validator.GetInvalidSchemaKeyword());
            error.Clear();
            validator.GetInvalidDocumentPointer().StringifyUriFragment(error);
            AZ_Warning("AWSMetrics", false, "Failed to load the metrics event, invalid document: %s.", error.GetString());
            return false;
        }
        return true;
    }

    void MetricsEvent::MarkFailedSubmission()
    {
        ++m_numFailures;
    }


    int MetricsEvent::GetNumFailures() const
    {
        return m_numFailures;
    }


    void MetricsEvent::SetEventPriority(int priority)
    {
        m_eventPriority = priority;
    }


    int MetricsEvent::GetEventPriority() const
    {
        return m_eventPriority;
    }
}
