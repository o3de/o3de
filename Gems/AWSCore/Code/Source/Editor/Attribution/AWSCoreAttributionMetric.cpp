/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/Attribution/AWSCoreAttributionMetric.h>
#include <Editor/Attribution/AWSCoreAttributionConstant.h>
#include <Framework/JsonWriter.h>
#include <sstream>

#pragma warning(disable : 4996)

namespace AWSCore
{
    constexpr char AWSAttributionMetricDefaultO3DEVersion[] = "1.1";

    AttributionMetric::AttributionMetric(const AZStd::string& timestamp)
        : m_version(AWSAttributionMetricDefaultO3DEVersion)
        , m_timestamp(timestamp)
    {
    }

    AttributionMetric::AttributionMetric()
        : m_version(AWSAttributionMetricDefaultO3DEVersion)
    {
        m_timestamp = AttributionMetric::GenerateTimeStamp();
    }

    void AttributionMetric::SetO3DEVersion(const AZStd::string& version)
    {
        m_o3deVersion = version;
    }

    void AttributionMetric::SetPlatform(const AZStd::string& platform, const AZStd::string& platformVersion)
    {
        m_platform = platform;
        m_platformVersion = platformVersion;
    }

    void AttributionMetric::AddActiveGem(const AZStd::string& gemName)
    {
        m_activeAWSGems.push_back(gemName);
    }

    AZStd::string AttributionMetric::SerializeToJson()
    {
        std::stringstream stringStream;
        AWSCore::JsonOutputStream jsonStream{stringStream};
        AWSCore::JsonWriter writer{jsonStream};

        SerializeToJson(writer);

        return stringStream.str().c_str();
    }

    bool AttributionMetric::SerializeToJson(AWSCore::JsonWriter& writer) const
    {
        bool ok = true;
        ok = ok && writer.StartObject();

        writer.Write(AwsAttributionAttributeKeyVersion, m_version.c_str());
        writer.Write(AwsAttributionAttributeKeyO3DEVersion, m_o3deVersion.c_str());
        writer.Write(AwsAttributionAttributeKeyPlatform, m_platform.c_str());
        writer.Write(AwsAttributionAttributeKeyPlatformVersion, m_platformVersion.c_str());

        if (m_activeAWSGems.size() > 0)
        {
            writer.Key(AwsAttributionAttributeKeyActiveAWSGems);
            writer.StartArray(); // to store Array of objects
            for (auto& iter : m_activeAWSGems)
            {
                writer.String(iter.c_str());
            }
            writer.EndArray();
        }

        writer.Write(AwsAttributionAttributeKeyTimestamp, m_timestamp.c_str());

        ok = ok && writer.EndObject();
        return ok;
    }

    bool AttributionMetric::ReadFromJson(rapidjson::Value& metricsObjVal)
    {
        AZ_UNUSED(metricsObjVal);
        return false;
    }

    AZStd::string AttributionMetric::GenerateTimeStamp()
    {
        // Timestamp format is using the UTC ISO8601 format
        // TODO: Move to a general util as Metrics has similar requirement
        time_t now;
        time(&now);
        char buffer[50];
        strftime(buffer, sizeof(buffer), "%FT%TZ", gmtime(&now));

        return buffer;
    }

} // namespace AWSCore
