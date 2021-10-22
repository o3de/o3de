/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "HttpTypes.h"

namespace HttpRequestor
{
    //! Models the parameters needed to make a HTTP call and then receive the
    //! returned JSON in a meaningful place. Examples of use are in the HttpRequestCaller class.
    class Parameters
    {
    public:
        // Ctors

        //! @param URI A universal resource indicator representing an endpoint.
        //! @param method The http verb to use, for example HTTP_GET.
        //! @param callback The callback method to receive a HTTP call's response.
        Parameters(const AZStd::string& URI, Aws::Http::HttpMethod method, const Callback& callback);

        //! @param URI A universal resource indicator representing an endpoint.
        //! @param method The http verb to use, for example HTTP_GET.
        //! @param headers A map of header names and values to use.
        //! @param callback The callback method to receive a HTTP call's response.
        Parameters(const AZStd::string& URI, Aws::Http::HttpMethod method, const Headers& headers, const Callback& callback);

        //! @param URI A universal resource indicator representing an endpoint.
        //! @param method The http verb to use, for example HTTP_POST.
        //! @param headers A map of header names and values to use.
        //! @param body An data to associate with an HTTP call.
        //! @param callback The callback method to receive a HTTP call's response.
        Parameters(
            const AZStd::string& URI,
            Aws::Http::HttpMethod method,
            const Headers& headers,
            const AZStd::string& body,
            const Callback& callback);

        // Defaults
        virtual ~Parameters() = default;
        Parameters(const Parameters&) = default;
        Parameters& operator=(const Parameters&) = default;

        Parameters(Parameters&&) = default;
        Parameters& operator=(Parameters&&) = default;

        //! Get the URI in string form as an recipient of the HTTP connection.
        //! @return URI as string
        const Aws::String& GetURI() const
        {
            return m_URI;
        }

        //! Get the method of which the HTTP request will take. GET, POST, DELETE, PUT, or HEAD.
        //! @return The http method to use
        Aws::Http::HttpMethod GetMethod() const
        {
            return m_method;
        }

        //! Get the list of extra headers to include in the request
        //! @return map of header name and values to use.
        const Headers& GetHeaders() const
        {
            return m_headers;
        }

        //! Get the stream for the body of the request
        //! @return a content body stream that can be used to send the body to the endpoint.
        const std::shared_ptr<std::stringstream>& GetBodyStream() const
        {
            return m_bodyStream;
        }

        //! Get the callback function of which to feed back the JSON that the HTTP call resulted in.
        //! The function also requires the HTTPResponseCode indicating if the call was successful or failed.
        //! @return the callback function to use to return the response from the endpoint.
        const Callback& GetCallback() const
        {
            return m_callback;
        }

    private:
        Aws::String m_URI;
        Aws::Http::HttpMethod m_method;
        Headers m_headers;
        std::shared_ptr<std::stringstream> m_bodyStream; // required by Aws::Http::HttpRequest
        Callback m_callback;
    };

    inline Parameters::Parameters(const AZStd::string& URI, Aws::Http::HttpMethod method, const Callback& callback)
        : m_URI(URI.c_str())
        , m_method(method)
        , m_callback(callback)
    {
    }

    inline Parameters::Parameters(const AZStd::string& URI, Aws::Http::HttpMethod method, const Headers& headers, const Callback& callback)
        : m_URI(URI.c_str())
        , m_method(method)
        , m_headers(headers)
        , m_callback(callback)
    {
    }

    inline Parameters::Parameters(
        const AZStd::string& URI, Aws::Http::HttpMethod method, const Headers& headers, const AZStd::string& body, const Callback& callback)
        : m_URI(URI.c_str())
        , m_method(method)
        , m_headers(headers)
        , m_bodyStream(std::make_shared<std::stringstream>(body.c_str()))
        , m_callback(callback)
    {
    }
}

