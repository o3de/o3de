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

#include <ResourceMapping/AWSResourceMappingConstants.h>
#include <ResourceMapping/AWSResourceMappingUtils.h>

namespace AWSCore
{
    namespace AWSResourceMappingUtils
    {
        // https://docs.aws.amazon.com/general/latest/gr/apigateway.html
        static constexpr char RESTAPI_URL_FORMAT[] = "https://%s.execute-api.%s.amazonaws.com/%s";
        static constexpr char RESTAPI_CHINA_URL_FORMAT[] = "https://%s.execute-api.%s.amazonaws.com.cn/%s";

        AZStd::string FormatRESTApiUrl(
            const AZStd::string& restApiId, const AZStd::string& restApiRegion, const AZStd::string& restApiStage)
        {
            // https://docs.aws.amazon.com/apigateway/latest/developerguide/how-to-call-api.html
            if (!restApiId.empty() && !restApiRegion.empty() && !restApiStage.empty())
            {
                if (restApiRegion.rfind(AWS_CHINA_REGION_PREFIX, 0) == 0)
                {
                    return AZStd::string::format(RESTAPI_CHINA_URL_FORMAT,
                        restApiId.c_str(), restApiRegion.c_str(), restApiStage.c_str());
                }
                else
                {
                    return AZStd::string::format(RESTAPI_URL_FORMAT,
                        restApiId.c_str(), restApiRegion.c_str(), restApiStage.c_str());
                }
            }
            return "";
        }
    } // namespace AWSResourceMappingUtils
} // namespace AWSCore
