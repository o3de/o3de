/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

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
    } // namespace AWSGameLiftActivityUtils
} // namespace AWSGameLift
