/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AWSMetricsServiceApi.h>
#include<AWSMetricsConstant.h>

#include <AWSCoreBus.h>

namespace AWSMetrics
{
    namespace ServiceAPI
    {
        bool MetricsEventSuccessResponseRecord::OnJsonKey(const char* key, AWSCore::JsonReader& reader)
        {
            if (strcmp(key, AwsMetricsSuccessResponseRecordKeyErrorCode) == 0) return reader.Accept(errorCode);

            if (strcmp(key, AwsMetricsSuccessResponseRecordKeyResult) == 0) return reader.Accept(result);

            return reader.Ignore();
        }

        bool MetricsEventSuccessResponse::OnJsonKey(const char* key, AWSCore::JsonReader& reader)
        {
            if (strcmp(key, AwsMetricsSuccessResponseKeyFailedRecordCount) == 0) return reader.Accept(failedRecordCount);

            if (strcmp(key, AwsMetricsSuccessResponseKeyEvents) == 0) return reader.Accept(events);

            if (strcmp(key, AwsMetricsSuccessResponseKeyTotal) == 0) return reader.Accept(total);

            return reader.Ignore();
        }

        bool Error::OnJsonKey(const char* key, AWSCore::JsonReader& reader)
        {
            if (strcmp(key, AwsMetricsErrorKeyMessage) == 0) return reader.Accept(message);

            if (strcmp(key, AwsMetricsErrorKeyType) == 0) return reader.Accept(type);

            return reader.Ignore();
        }

        // Generated Function Parameters
        bool PostProducerEventsRequest::Parameters::BuildRequest(AWSCore::RequestBuilder& request)
        {
            bool ok = true;

            ok = ok && request.WriteJsonBodyParameter(*this);
            return ok;
        }

        bool PostProducerEventsRequest::Parameters::WriteJson(AWSCore::JsonWriter& writer) const
        {
            bool ok = true;

            ok = ok && writer.StartObject();

            ok = ok && writer.Key(AwsMetricsRequestParameterKeyEvents);
            ok = ok && data.SerializeToJson(writer);

            ok = ok && writer.EndObject();

            return ok;
        }
    }
}
