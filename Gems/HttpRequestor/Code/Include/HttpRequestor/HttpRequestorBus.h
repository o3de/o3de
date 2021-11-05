/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include "HttpTypes.h"

namespace HttpRequestor
{
    //! Defines request APIs for Gem. Supports making HTTP requests.
    //! See [HTTP RFC](https://www.w3.org/Protocols/rfc2616/rfc2616-sec4.html) for expectations around methods, headers, and body.
    class HttpRequestorRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        //! Make a RESTful call to a HTTP(s) endpoint. Receive the response, via the supplied callback as JSON.
        //! @param URI The universal resource indicator representing the endpoint to make the request to.
        //! @param method The HTTP method to use, for example HTTP_GET.
        //! @param callback The callback method to receive the JSON response object.
        virtual void AddRequest(const AZStd::string& URI, Aws::Http::HttpMethod method, const Callback& callback) = 0;

        //! Make a RESTful call to a HTTP(s) endpoint with customized headers. Receive the response, via the supplied callback as JSON.
        //! @param URI The universal resource indicator representing the endpoint to make the request to.
        //! @param method The HTTP method to use, for example HTTP_GET.
        //! @param headers A map of header names and values to set on the request.
        //! @param callback The callback method to receive the JSON response object.
        virtual void AddRequestWithHeaders(
            const AZStd::string& URI, Aws::Http::HttpMethod method, const Headers& headers, const Callback& callback) = 0;

        //! Make a RESTful call to a HTTP(s) endpoint with customized headers and a body. Receive the response, via the supplied callback as JSON.
        //! @param URI The universal resource indicator representing the endpoint to make the request to.
        //! @param method The HTTP method to use, for example HTTP_POST.
        //! @param headers A map of header names and values to set on the request.
        //! @param body Any HTTP request data to include in the request. Use Content-Type and Content-Length headers to specify the nature
        //! of the body payload.
        //! @param callback The callback method to receive the JSON response object.
        virtual void AddRequestWithHeadersAndBody(
            const AZStd::string& URI,
            Aws::Http::HttpMethod method,
            const Headers& headers,
            const AZStd::string& body,
            const Callback& callback) = 0;

        //! Make a RESTful call to a HTTP(s) endpoint. Receive the response, via the supplied callback as text.
        //! @param URI The universal resource indicator representing the endpoint to make the request to.
        //! @param method The http method to use, for example HTTP_GET.
        //! @param callback The callback method to receive the JSON response object.
        virtual void AddTextRequest(const AZStd::string& URI, Aws::Http::HttpMethod method, const TextCallback& callback) = 0;

        //! Make a RESTful call to a HTTP(s) endpoint with customized headers. Receive the response, via the supplied callback as text.
        //! @param URI The universal resource indicator representing the endpoint to make the request to.
        //! @param method The HTTP method to use, for example HTTP_GET.
        //! @param headers A map of header names and values to set on the request.
        //! @param callback The callback method to receive the JSON response object.
        virtual void AddTextRequestWithHeaders(
            const AZStd::string& URI, Aws::Http::HttpMethod method, const Headers& headers, const TextCallback& callback) = 0;

        //! Make a RESTful call to a HTTP(s) endpoint with customized headers and a body. Receive the response, via the supplied callback as text.
        //! @param URI The universal resource indicator representing the endpoint to make the request to.
        //! @param method The HTTP method to use, for example HTTP_POST.
        //! @param headers A map of header names and values to set on the request.
        //! @param body Any HTTP request data to include in the request. Use Content-Type and Content-Length headers to specify the nature of the body payload.
        //! @param callback The callback method to receive the JSON response object.
        virtual void AddTextRequestWithHeadersAndBody(
            const AZStd::string& URI,
            Aws::Http::HttpMethod method,
            const Headers& headers,
            const AZStd::string& body,
            const TextCallback& callback) = 0;
    };

    using HttpRequestorRequestBus = AZ::EBus<HttpRequestorRequests>;

} // namespace HttpRequestor
