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
        bool PostMetricsEventsResponseEntry::OnJsonKey(const char* key, AWSCore::JsonReader& reader)
        {
            if (strcmp(key, AwsMetricsPostMetricsEventsResponseEntryKeyErrorCode) == 0)
            {
                return reader.Accept(m_errorCode);
            }

            if (strcmp(key, AwsMetricsPostMetricsEventsResponseEntryKeyResult) == 0)
            {
                return reader.Accept(m_result);
            }

            return reader.Ignore();
        }

        bool PostMetricsEventsResponse::OnJsonKey(const char* key, AWSCore::JsonReader& reader)
        {
            if (strcmp(key, AwsMetricsPostMetricsEventsResponseKeyFailedRecordCount) == 0)
            {
                return reader.Accept(m_failedRecordCount);
            }

            if (strcmp(key, AwsMetricsPostMetricsEventsResponseKeyEvents) == 0)
            {
                return reader.Accept(m_responseEntries);
            }

            if (strcmp(key, AwsMetricsPostMetricsEventsResponseKeyTotal) == 0)
            {
                return reader.Accept(m_total);
            }

            return reader.Ignore();
        }

        bool PostMetricsEventsError::OnJsonKey(const char* key, AWSCore::JsonReader& reader)
        {
            if (strcmp(key, AwsMetricsPostMetricsEventsErrorKeyMessage) == 0)
            {
                return reader.Accept(message);
            }

            if (strcmp(key, AwsMetricsPostMetricsEventsErrorKeyType) == 0)
            {
                return reader.Accept(type);
            }

            return reader.Ignore();
        }

        // Generated request parameters
        bool PostMetricsEventsRequest::Parameters::BuildRequest(AWSCore::RequestBuilder& request)
        {
            bool buildResult = true;
            buildResult = buildResult && request.WriteJsonBodyParameter(*this);

            return buildResult;
        }

        bool PostMetricsEventsRequest::Parameters::WriteJson(AWSCore::JsonWriter& writer) const
        {
            bool writeResult = true;

            writeResult = writeResult && writer.StartObject();

            writeResult = writeResult && writer.Key(AwsMetricsPostMetricsEventsRequestParameterKeyEvents);
            writeResult = writeResult && m_metricsQueue.SerializeToJson(writer);

            writeResult = writeResult && writer.EndObject();

            return writeResult;
        }
    }
}
