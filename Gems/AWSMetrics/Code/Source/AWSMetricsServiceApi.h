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
        //! Struct for storing event record from the response.
        struct MetricsEventSuccessResponseRecord
        {
            //! Identify the expected property type and provide a location where the property value can be stored.
            //! @param key Name of the property.
            //! @param reader JSON reader to read the property.
            bool OnJsonKey(const char* key, AWSCore::JsonReader& reader);

            AZStd::string errorCode; //!< Error code if the event is not sent successfully.
            AZStd::string result; //!< Processing result for the input record.
        };

        using MetricsEventSuccessResponsePropertyEvents = AZStd::vector<MetricsEventSuccessResponseRecord>;

        //! Struct for storing the success response.
        struct MetricsEventSuccessResponse
        {
            //! Identify the expected property type and provide a location where the property value can be stored.
            //! @param key Name of the property.
            //! @param reader JSON reader to read the property.
            bool OnJsonKey(const char* key, AWSCore::JsonReader& reader);

            int failedRecordCount{ 0 }; //!< Number of events that failed to be saved to metrics events stream.
            MetricsEventSuccessResponsePropertyEvents events; //! List of input event records.
            int total{ 0 }; //!< Total number of events that were processed in the request
        };

        //! Struct for storing the failure response.
        struct Error
        {
            //! Identify the expected property type and provide a location where the property value can be stored.
            //! @param key Name of the property.
            //! @param reader JSON reader to read the property.
            bool OnJsonKey(const char* key, AWSCore::JsonReader& reader);

            AZStd::string message; //!< Error message.
            AZStd::string type; //!< Error type.
        };

        // Service RequestJobs
        AWS_FEATURE_GEM_SERVICE(AWSMetrics);

        //! POST request defined by api_spec.json to send metrics to the backend.
        //! The path for this service API is "/producer/events".
        class PostProducerEventsRequest
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

                MetricsQueue data; //!< Data to send via the service API request.
            };

            MetricsEventSuccessResponse result; //! Success response.
            Error error; //! Failure response.
            Parameters parameters; //! Request parameter.
        };

        using PostProducerEventsRequestJob = AWSCore::ServiceRequestJob<PostProducerEventsRequest>;
    } // ServiceAPI
} // AWSMetrics
