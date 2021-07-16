/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AWSMetrics
{
    //! Default metrics attribute keys
    static constexpr char AwsMetricsAttributeKeyClientId[] = "application_id";
    static constexpr char AwsMetricsAttributeKeyEventId[] = "event_id";
    static constexpr char AwsMetricsAttributeKeyEventName[] = "event_name";
    static constexpr char AwsMetricsAttributeKeyEventType[] = "event_type";
    static constexpr char AwsMetricsAttributeKeyEventSource[] = "event_source";
    static constexpr char AwsMetricsAttributeKeyEventTimestamp[] = "event_timestamp";

    static constexpr char AwsMetricsAttributeKeyEventData[] = "event_data";

    //! Service API request and response object keys
    static constexpr char AwsMetricsSuccessResponseRecordKeyErrorCode[] = "error_code";
    static constexpr char AwsMetricsSuccessResponseRecordKeyResult[] = "result";
    static constexpr char AwsMetricsSuccessResponseKeyFailedRecordCount[] = "failed_record_count";
    static constexpr char AwsMetricsSuccessResponseKeyEvents[] = "events";
    static constexpr char AwsMetricsSuccessResponseKeyTotal[] = "total";
    static constexpr char AwsMetricsErrorKeyMessage[] = "message";
    static constexpr char AwsMetricsErrorKeyType[] = "type";
    static constexpr char AwsMetricsRequestParameterKeyEvents[] = "events";

    static constexpr char AwsMetricsSuccessResponseRecordResult[] = "Ok";

    //! Service API limits
    //! https://docs.aws.amazon.com/apigateway/latest/developerguide/limits.html
    static constexpr int AwsMetricsMaxRestApiPayloadSizeInMb = 10;
    //! https://docs.aws.amazon.com/kinesis/latest/APIReference/API_PutRecords.html
    static constexpr int AwsMetricsMaxKinesisBatchedRecordCount = 500;

    //! Metrics event JSON schema
    static constexpr const char AwsMetricsEventJsonSchema[] =
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
