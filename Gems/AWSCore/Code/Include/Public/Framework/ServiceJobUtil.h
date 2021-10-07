/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/tokenize.h>

#include <Framework/ServiceJob.h>

namespace AWSCore
{
    inline void ConfigureJsonServiceRequest(HttpRequestJob& request, AZStd::string jsonBody)
    {
        size_t len = jsonBody.length();

        if (len > 0)
        {
            AZStd::string lenStr = AZStd::string::format("%zu", len);
            request.SetContentLength(lenStr);
            request.SetContentType("application/json");
            request.SetBody(std::move(jsonBody));
        }

        request.SetAccept("application/json");
        request.SetAcceptCharSet("utf-8");
    }

    inline Aws::String DetermineRegionFromServiceUrl(const Aws::String& serviceUrl)
    {
        // Assumes that API Gateway URLs have either of the following two forms:
        // https://{custom_domain_name}/{region}.{stage}.{rest-api-id}/{path}
        // https://{rest-api-id}.execute-api.{region}.amazonaws.com/{stage}/{path}
        const int ExpectedUrlSections = 3;

        AZStd::vector<AZStd::string> urlSections;
        AZStd::string url(serviceUrl.c_str());
        AZStd::tokenize(url, AZStd::string("/"), urlSections);

        if (urlSections.size() > ExpectedUrlSections)
        {
            int i = static_cast<int>(urlSections[ExpectedUrlSections - 1].find('.'));
            if (i != -1)
            {
                // Handle APIGateway URLs with custom domains:
                // https://{custom_domain_name}/{region}.{stage}.{rest-api-id}/{path}
                return urlSections[ExpectedUrlSections - 1].substr(0, i).c_str();
            }
            else
            {
                // API Gateway URLs have the form:
                // https://{rest-api-id}.execute-api.{region}.amazonaws.com/{stage}/{path}
                const int RegionIndex = 2;

                AZStd::vector<AZStd::string> domainSections;
                AZStd::tokenize(urlSections[ExpectedUrlSections - 2], AZStd::string("."), domainSections);
                return domainSections[RegionIndex].c_str();
            }
        }

        return "";

    }

} // namespace AWSCore
