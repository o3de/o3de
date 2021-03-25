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

#include <AWSMetricsServiceApi.h>
#include<AWSMetricsConstant.h>

#include <AWSCoreBus.h>

namespace AWSMetrics
{
    namespace ServiceAPI
    {
        bool MetricsEventSuccessResponseRecord::OnJsonKey(const char* key, AWSCore::JsonReader& reader)
        {
            if (strcmp(key, SUCCESS_RESPONSE_RECORD_KEY_ERROR_CODE) == 0) return reader.Accept(errorCode);

            if (strcmp(key, SUCCESS_RESPONSE_RECORD_KEY_RESULT) == 0) return reader.Accept(result);

            return reader.Ignore();
        }

        bool MetricsEventSuccessResponse::OnJsonKey(const char* key, AWSCore::JsonReader& reader)
        {
            if (strcmp(key, SUCCESS_RESPONSE_KEY_FAILED_RECORD_COUNT) == 0) return reader.Accept(failedRecordCount);

            if (strcmp(key, SUCCESS_RESPONSE_KEY_EVENTS) == 0) return reader.Accept(events);

            if (strcmp(key, SUCCESS_RESPONSE_KEY_TOTAL) == 0) return reader.Accept(total);

            return reader.Ignore();
        }

        bool Error::OnJsonKey(const char* key, AWSCore::JsonReader& reader)
        {
            if (strcmp(key, ERROR_KEY_MESSAGE) == 0) return reader.Accept(message);

            if (strcmp(key, ERROR_KEY_TYPE) == 0) return reader.Accept(type);

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

            ok = ok && writer.Key(REQUEST_PARAMETER_KEY_EVENTS);
            ok = ok && data.SerializeToJson(writer);

            ok = ok && writer.EndObject();

            return ok;
        }
    }
}
