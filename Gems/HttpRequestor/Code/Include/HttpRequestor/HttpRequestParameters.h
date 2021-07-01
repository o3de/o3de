/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "HttpTypes.h"

namespace HttpRequestor
{
    /*
    **
    **  The Parameters needed to make a HTTP call and then receive the
    **  returned JSON in a meaningful place. Examples of use are in the
    **  HttpRequestCaller class.
    **
    */

    class Parameters
    {
    public:
        // Initializing ctor
        Parameters(const AZStd::string& URI, Aws::Http::HttpMethod method, const Callback& callback);
        Parameters(const AZStd::string& URI, Aws::Http::HttpMethod method, const Headers& headers, const Callback& callback);
        Parameters(const AZStd::string& URI, Aws::Http::HttpMethod method, const Headers& headers, const AZStd::string& body, const Callback& callback);

        // Defaults
        virtual ~Parameters() = default;
        Parameters(const Parameters&) = default;
        Parameters& operator=(const Parameters&) = default;

        Parameters(Parameters&&) = default;
        Parameters& operator=(Parameters&&) = default;

        //returns the URI in string form as an recipient of the HTTP connection
        const Aws::String& GetURI() const { return m_URI; }

        //returns the method of which the HTTP request will take. GET, POST, DELETE, PUT, or HEAD
        Aws::Http::HttpMethod GetMethod() const { return m_method; }

        //returns the list of extra headers to include in the request
        const Headers & GetHeaders() const { return m_headers; }

        //returns the stream for the body of the request
        const std::shared_ptr<std::stringstream> & GetBodyStream() const { return m_bodyStream; }

        //returns the function of which to feed back the JSON that the HTTP call resulted in. The function also requires the HTTPResponseCode indicating if the call was successful or failed
        const Callback & GetCallback() const { return m_callback; }

    private:
        Aws::String                             m_URI;
        Aws::Http::HttpMethod                   m_method;
        Headers                                 m_headers;
        std::shared_ptr<std::stringstream>      m_bodyStream;   // required by Aws::Http::HttpRequest
        Callback                                m_callback;
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

    inline Parameters::Parameters(const AZStd::string& URI, Aws::Http::HttpMethod method, const Headers& headers, const AZStd::string& body, const Callback& callback)
        : m_URI(URI.c_str())
        , m_method(method)
        , m_headers(headers)
        , m_bodyStream(std::make_shared<std::stringstream>(body.c_str()))
        , m_callback(callback)
    {
    }

}

