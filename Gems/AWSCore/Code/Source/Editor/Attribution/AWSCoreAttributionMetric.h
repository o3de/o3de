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
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace AWSCore
{
    //! Defines the operational metric sent periodically
    class AttributionMetric
    {
    public:
        AZ_TYPE_INFO(MetricsAttribute, "{6483F481-0C18-4171-8B59-A44F2F28EAE5}")

        AttributionMetric();
        AttributionMetric(const AZStd::string& timestamp);
        ~AttributionMetric() = default;

        void SetO3DEVersion(const AZStd::string& version);

        const AZStd::string& GetPlatform() const { return m_platform;}
        void SetPlatform(const AZStd::string& platform, const AZStd::string& platformVersion);

        void AddActiveGem(const AZStd::string& gemName);

        //! Serialize the metrics object queue to a string.
        //! @return Serialized string.
        AZStd::string SerializeToJson();

        //! Serialize the metrics object to JSON for the sending requests.
        //! @param writer JSON writer for the serialization.
        //! @return Whether the metrics event is serialized successfully.
        bool SerializeToJson(AWSCore::JsonWriter& writer) const;

        //! Read from a JSON value to the metrics event.
        //! @param metricsObjVal JSON value to read from.
        //! @return Whether the metrics event is created successfully.
        bool ReadFromJson(rapidjson::Value& metricsObjVal);

        //! Generates a UTC 8601 formatted timestamp
        static AZStd::string GenerateTimeStamp();
    private:
        AZStd::string m_version;        //!< Schema version in use
        AZStd::string m_o3deVersion;    //!< O3DE editor version in use
        AZStd::string m_platform;       //!< OS type
        AZStd::string m_platformVersion; //!< OS subtype
        AZStd::string m_timestamp;       //!< Metric generation time
        AZStd::vector<AZStd::string> m_activeAWSGems; //!< Active AWS Gems in project
    };
} // namespace AWSCore
