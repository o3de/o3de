/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ResourceMapping/AWSResourceMappingConstants.h>
#include <ResourceMapping/AWSResourceMappingUtils.h>

namespace AWSCore
{
    namespace AWSResourceMappingUtils
    {
        // https://docs.aws.amazon.com/general/latest/gr/apigateway.html
        static constexpr char RESTApiUrlFormat[] = "https://%s.execute-api.%s.amazonaws.com/%s";
        static constexpr char RESTApiChinaUrlFormat[] = "https://%s.execute-api.%s.amazonaws.com.cn/%s";

        AZStd::string FormatRESTApiUrl(
            const AZStd::string& restApiId, const AZStd::string& restApiRegion, const AZStd::string& restApiStage)
        {
            // https://docs.aws.amazon.com/apigateway/latest/developerguide/how-to-call-api.html
            if (!restApiId.empty() && !restApiRegion.empty() && !restApiStage.empty())
            {
                if (restApiRegion.rfind(AWSChinaRegionPrefix, 0) == 0)
                {
                    return AZStd::string::format(RESTApiChinaUrlFormat,
                        restApiId.c_str(), restApiRegion.c_str(), restApiStage.c_str());
                }
                else
                {
                    return AZStd::string::format(RESTApiUrlFormat,
                        restApiId.c_str(), restApiRegion.c_str(), restApiStage.c_str());
                }
            }
            return "";
        }
    } // namespace AWSResourceMappingUtils
} // namespace AWSCore
