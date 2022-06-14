/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Framework/ServiceRequestJob.h>

#include <MetricsQueue.h>

namespace AWSMetrics
{
    namespace ServiceAPI
    {
        //! Response for an individual metrics event from a PostMetricsEvents request.
        //! If the event is successfully sent to the backend, it receives an "Ok" result.
        //! If the event fails to be sent to the backend, the result includes an error code and an "Error" result.
        struct PostMetricsEventsResponseEntry
        {
            //! Identify the expected property type in the response entry for each individual metrics event and provide a location where the property value can be stored.
            //! @param key Name of the property.
            //! @param reader JSON reader to read the property.
            bool OnJsonKey(const char* key, AWSCore::JsonReader& reader);

            AZStd::string m_errorCode; //!< Error code if the individual metrics event failed to be sent.
            AZStd::string m_result; //!< Result for the processed individual metrics event. Expected value: "Error" or "Ok".
        };

        using PostMetricsEventsResponseEntries = AZStd::vector<PostMetricsEventsResponseEntry>;

        //! Response for all the processed metrics events from a PostMetricsEvents request.
        struct PostMetricsEventsResponse
        {
            //! Identify the expected property type in the response and provide a location where the property value can be stored.
            //! @param key Name of the property.
            //! @param reader JSON reader to read the property.
            bool OnJsonKey(const char* key, AWSCore::JsonReader& reader);

            int m_failedRecordCount{ 0 }; //!< Number of events that failed to be sent to the backend.
            PostMetricsEventsResponseEntries m_responseEntries; //! Response list for all the processed metrics events.
            int m_total{ 0 }; //!< Total number of events that were processed in the request.
        };

        //! Failure response for sending the PostMetricsEvents request.
        struct PostMetricsEventsError
        {
            //! Identify the expected property type in the failure response and provide a location where the property value can be stored.
            //! @param key Name of the property.
            //! @param reader JSON reader to read the property.
            bool OnJsonKey(const char* key, AWSCore::JsonReader& reader);

            //! Do not rename the following members since they are expected by the AWSCore dependency.
            AZStd::string message; //!< Error message.
            AZStd::string type; //!< Error type.
        };

        // Service RequestJobs
        AWS_FEATURE_GEM_SERVICE(AWSMetrics);

        //! POST request defined by api_spec.json to send metrics to the backend.
        //! The path for this service API is "/producer/events".
        class PostMetricsEventsRequest
            : public AWSCore::ServiceRequest
        {
        public:
            SERVICE_REQUEST(AWSMetrics, HttpMethod::HTTP_POST, "/producer/events");

            //! Request body for the service API request.
            struct Parameters
            {
                //! Build the service API request.
                //! @request Builder for generating the request.
                //! @return Whether the request is built successfully.
                bool BuildRequest(AWSCore::RequestBuilder& request);

                //! Write to the service API request body.
                //! @param writer JSON writer for the serialization.
                //! @return Whether the serialization is successful.
                bool WriteJson(AWSCore::JsonWriter& writer) const;

                MetricsQueue m_metricsQueue; //!< Metrics events to send via the service API request.
            };

            //! Do not rename the following members since they are expected by the AWSCore dependency.
            PostMetricsEventsResponse result; //! Success response.
            PostMetricsEventsError error; //! Failure response.
            Parameters parameters; //! Request parameter.
        };

        using PostMetricsEventsRequestJob = AWSCore::ServiceRequestJob<PostMetricsEventsRequest>;
    } // ServiceAPI
} // AWSMetrics
