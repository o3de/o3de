/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <aws/core/utils/json/JsonSerializer.h>
#include <Activity/AWSGameLiftActivityUtils.h>

namespace AWSGameLift
{
    namespace AWSGameLiftActivityUtils
    {
        void GetGameProperties(
            const AZStd::unordered_map<AZStd::string, AZStd::string>& sessionProperties,
            Aws::Vector<Aws::GameLift::Model::GameProperty>& outGameProperties,
            AZStd::string& outGamePropertiesOutput)
        {
            for (auto iter = sessionProperties.begin(); iter != sessionProperties.end(); iter++)
            {
                Aws::GameLift::Model::GameProperty sessionProperty;
                sessionProperty.SetKey(iter->first.c_str());
                sessionProperty.SetValue(iter->second.c_str());
                outGameProperties.push_back(sessionProperty);
                outGamePropertiesOutput += AZStd::string::format("{Key=%s,Value=%s},", iter->first.c_str(), iter->second.c_str());
            }
            if (!outGamePropertiesOutput.empty())
            {
                outGamePropertiesOutput =
                    outGamePropertiesOutput.substr(0, outGamePropertiesOutput.size() - 1); // Trim last comma to fit array format
            }
        }

        void ConvertPlayerAttributes(
            const AZStd::unordered_map<AZStd::string, AZStd::string>& playerAttributes,
            Aws::Map<Aws::String, Aws::GameLift::Model::AttributeValue>& outPlayerAttributes)
        {
            outPlayerAttributes.clear();
            for (auto& playerAttribute : playerAttributes)
            {               
                Aws::Utils::Json::JsonValue keyJsonValue(playerAttribute.second.c_str());
                Aws::GameLift::Model::AttributeValue attribute(keyJsonValue);
                outPlayerAttributes[playerAttribute.first.c_str()] = attribute;
            }
        }

        void ConvertRegionToLatencyMap(
            const AZStd::unordered_map<AZStd::string, int>& regionToLatencyMap,
            Aws::Map<Aws::String, int>& outRegionToLatencyMap)
        {
            outRegionToLatencyMap.clear();
            for (auto& regionToLatencyPair : regionToLatencyMap)
            {
                outRegionToLatencyMap[regionToLatencyPair.first.c_str()] = regionToLatencyPair.second;
            }
        }

        bool ValidatePlayerAttributes(
            const AZStd::unordered_map<AZStd::string, AZStd::string>& playerAttributes)
        {
            for (auto& playerAttribute : playerAttributes)
            {
                Aws::Utils::Json::JsonValue keyJsonValue(playerAttribute.second.c_str());
                Aws::GameLift::Model::AttributeValue attribute(keyJsonValue);

                // Each AttributeValue object can use only one of the available properties:
                // 1) number values (N)
                // 2) single string values (S)
                // 3) string to double map (SDM)
                // 4) array of strings (SL)
                if (!attribute.SHasBeenSet() &&
                    !attribute.NHasBeenSet() &&
                    !attribute.SDMHasBeenSet() &&
                    !attribute.SLHasBeenSet())
                {
                    return false;
                }
            }
            return true;
        }
    } // namespace AWSGameLiftActivityUtils
} // namespace AWSGameLift
