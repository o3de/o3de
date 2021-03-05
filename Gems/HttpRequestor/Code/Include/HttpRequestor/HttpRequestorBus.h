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

#include <AzCore/EBus/EBus.h>
#include "HttpTypes.h"

namespace HttpRequestor
{
    class HttpRequestorRequests
        : public AZ::EBusTraits
    {

    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        // Public functions
        virtual void AddRequest(const AZStd::string& URI, Aws::Http::HttpMethod method, const Callback& callback) = 0;
        virtual void AddRequestWithHeaders(const AZStd::string& URI, Aws::Http::HttpMethod method, const Headers & headers, const Callback& callback) = 0;
        virtual void AddRequestWithHeadersAndBody(const AZStd::string& URI, Aws::Http::HttpMethod method, const Headers & headers, const AZStd::string& body, const Callback& callback) = 0;
        
        virtual void AddTextRequest(const AZStd::string& URI, Aws::Http::HttpMethod method, const TextCallback& callback) = 0;
        virtual void AddTextRequestWithHeaders(const AZStd::string& URI, Aws::Http::HttpMethod method, const Headers & headers, const TextCallback& callback) = 0;
        virtual void AddTextRequestWithHeadersAndBody(const AZStd::string& URI, Aws::Http::HttpMethod method, const Headers & headers, const AZStd::string& body, const TextCallback& callback) = 0;
    };

    using HttpRequestorRequestBus = AZ::EBus<HttpRequestorRequests>;
} // namespace HttpRequestor

