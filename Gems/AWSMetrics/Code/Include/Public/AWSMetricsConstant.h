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

#pragma once

namespace AWSMetrics
{
    //! Default metrics attribute keys
    static constexpr char METRICS_ATTRIBUTE_KEY_CLIENT_ID[] = "application_id";
    static constexpr char METRICS_ATTRIBUTE_KEY_EVENT_ID[] = "event_id";
    static constexpr char METRICS_ATTRIBUTE_KEY_EVENT_NAME[] = "event_name";
    static constexpr char METRICS_ATTRIBUTE_KEY_EVENT_TYPE[] = "event_type";
    static constexpr char METRICS_ATTRIBUTE_KEY_EVENT_SOURCE[] = "event_source";
    static constexpr char METRICS_ATTRIBUTE_KEY_EVENT_TIMESTAMP[] = "event_timestamp";

    static constexpr char METRICS_ATTRIBUTE_KEY_EVENT_DATA[] = "event_data";

    //! Service API request and response object keys
    static constexpr char SUCCESS_RESPONSE_RECORD_KEY_ERROR_CODE[] = "error_code";
    static constexpr char SUCCESS_RESPONSE_RECORD_KEY_RESULT[] = "result";
    static constexpr char SUCCESS_RESPONSE_KEY_FAILED_RECORD_COUNT[] = "failed_record_count";
    static constexpr char SUCCESS_RESPONSE_KEY_EVENTS[] = "events";
    static constexpr char SUCCESS_RESPONSE_KEY_TOTAL[] = "total";
    static constexpr char ERROR_KEY_MESSAGE[] = "message";
    static constexpr char ERROR_KEY_TYPE[] = "type";
    static constexpr char REQUEST_PARAMETER_KEY_EVENTS[] = "events";

    static constexpr char SUCCESS_RESPONSE_RECORD_RESULT[] = "Ok";

    //! Service API limits
    //! https://docs.aws.amazon.com/apigateway/latest/developerguide/limits.html
    static constexpr int MAX_REST_API_PAYLOAD_SIZE_IN_MB = 10;
    //! https://docs.aws.amazon.com/firehose/latest/APIReference/API_PutRecordBatch.html
    static constexpr int MAX_FIREHOSE_BATCHED_RECORDS_COUNT = 500;

    //! Metrics event JSON schema
    static constexpr const char METRICS_EVENT_JSON_SCHEMA[] =
        R"({
        "$schema": "http://json-schema.org/draft-04/schema",
        "title": "AWSMetrics API Event Schema",
        "description": "Metrics Event sent to the service API",
        "type": "object",
        "additionalProperties": false,
        "properties": {
            "application_id": {
                "type": "string",
                "pattern": "^[0-9-.]+-\\{[0-9A-F]{8}-[0-9A-F]{4}-[1-5][0-9A-F]{3}-[89AB][0-9A-F]{3}-[0-9A-F]{12}\\}$",
                "description": "Identifier for the application that generated the event."
            },
            "event_id": {
                "type": "string",
                "pattern": "^\\{[0-9A-F]{8}-[0-9A-F]{4}-[1-5][0-9A-F]{3}-[89AB][0-9A-F]{3}-[0-9A-F]{12}\\}$",
                "description": "A random UUID that uniquely identifies an event."
            },
            "event_type": {
                "type": "string",
                "pattern": "^[A-Za-z0-9-_.]+$",
                "description": "Identifies the type of event."
            },
            "event_name": {
                "type": "string",
                "pattern": "^[A-Za-z0-9-_.]+$",
                "description": "Name of the event that occurred."
            },
            "event_timestamp": {
                "type": "string",
                "pattern": "^(\\d{4})-(\\d{2})-(\\d{2})T(\\d{2})\\:(\\d{2})\\:(\\d{2})Z$",
                "description": "Timestamp of the event in the UTC ISO8601 format."
            },
            "event_version": {
                "type": "string",
                "pattern": "^[A-Za-z0-9-_.]+$",
                "description": "An API version for this event format."
            },
            "event_source": {
                "type": "string",
                "pattern": "^[A-Za-z0-9-_.]+$",
                "description": "Source of the event."
            },
            "event_data": {
                "type": "object",
                "description": "Custom metrics attributes defined by this event."
            }
        },
        "required": [ "application_id", "event_id", "event_name", "event_timestamp" ]
    })";

}
