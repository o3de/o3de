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

#include <Editor/Attribution/AWSCoreAttributionMetric.h>
#include <Editor/Attribution/AWSAttributionServiceApi.h>

namespace AWSCore
{
    namespace ServiceAPI
    {
        constexpr char AwsAttributionServiceResultResponseKey[] = "statusCode";

        bool AWSAtrributionSuccessResponse::OnJsonKey(const char* key, AWSCore::JsonReader& reader)
        {
            if (strcmp(key, AwsAttributionServiceResultResponseKey) == 0)
            {
                return reader.Accept(result);
            }
            return reader.Ignore();
        }

        bool AWSAttributionRequest::Parameters::BuildRequest(AWSCore::RequestBuilder& request)
        {
            bool ok = true;
            ok = ok && request.WriteJsonBodyParameter(*this);
            return ok;
        }

        bool AWSAttributionRequest::Parameters::WriteJson(AWSCore::JsonWriter& writer) const
        {
            bool ok = true;
            ok = ok && metric.SerializeToJson(writer);
            return ok;
        }
    }
}
