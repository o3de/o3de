/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
