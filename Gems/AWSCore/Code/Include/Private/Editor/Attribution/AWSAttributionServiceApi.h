/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Framework/ServiceRequestJob.h>
#include <Editor/Attribution/AWSCoreAttributionMetric.h>

namespace AWSCore
{
    namespace ServiceAPI
    {
        //! Struct for storing the success response.
        struct AWSAtrributionSuccessResponse
        {
            //! Identify the expected property type and provide a location where the property value can be stored.
            //! @param key Name of the property.
            //! @param reader JSON reader to read the property.
            bool OnJsonKey(const char* key, AWSCore::JsonReader& reader);

            AZStd::string result; //!< Processing result for the input record.
        };

        // Service RequestJobs
        AWS_FEATURE_GEM_SERVICE(AWSAttribution);

        //! POST request to send attribution metric to the backend.
        //! The path for this service API is "/prod/metrics".
        class AWSAttributionRequest
            : public AWSCore::ServiceRequest
        {
        public:
            SERVICE_REQUEST(AWSAttribution, HttpMethod::HTTP_POST, "/metrics");

            bool UseAWSCredentials()
            {
                return true;
            }

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

                AttributionMetric metric;
            };

            AWSAtrributionSuccessResponse result;
            Parameters parameters; //! Request parameter.
        };

        using AWSAttributionRequestJob = AWSCore::ServiceRequestJob<AWSAttributionRequest>;
    } // ServiceAPI
} // AWSMetrics
