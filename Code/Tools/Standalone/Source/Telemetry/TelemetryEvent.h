/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#ifndef TELEMETRY_TELEMETRYEVENT_H
#define TELEMETRY_TELEMETRYEVENT_H

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>

namespace Telemetry
{
    class TelemetryEvent
    {
    public:
        typedef AZStd::unordered_map< AZStd::string, AZStd::string > AttributesMap;
        typedef AZStd::unordered_map< AZStd::string, double > MetricsMap;

        TelemetryEvent(const char* eventName);

        void SetAttribute(const AZStd::string& name, const AZStd::string& value);
        const AZStd::string& GetAttribute(const AZStd::string& name);

        void SetMetric(const AZStd::string& name, double metric);
        double GetMetric(const AZStd::string& name);

        void Log();
        void ResetEvent();

        const char* GetEventName() const;
        const AttributesMap& GetAttributes() const;
        const MetricsMap& GetMetrics() const;

    private:
        AZStd::string m_eventName;
        AttributesMap m_attributes;
        MetricsMap m_metrics;
    };
}

#endif