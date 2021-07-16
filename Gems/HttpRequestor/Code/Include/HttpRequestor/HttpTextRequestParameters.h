/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "HttpTypes.h"
#include <sstream>

namespace HttpRequestor
{
    /*
    **
    **  The Parameters needed to make a HTTP call and then receive the
    **  returned TEXT from the web request without parsing it.
    **
    */

    class TextParameters
    {
    public:
        // Initializing ctor
        TextParameters(const AZStd::string& URI, Aws::Http::HttpMethod method, const TextCallback& callback);
        TextParameters(const AZStd::string& URI, Aws::Http::HttpMethod method, const Headers& headers, const TextCallback& callback);
        TextParameters(const AZStd::string& URI, Aws::Http::HttpMethod method, const Headers& headers, const AZStd::string& body, const TextCallback& callback);

        // Defaults
        ~TextParameters() = default;
        TextParameters(const TextParameters&) = default;
        TextParameters& operator=(const TextParameters&) = default;

        TextParameters(TextParameters&&) = default;
        TextParameters& operator=(TextParameters&&) = default;

        //returns the URI in string form as an recipient of the HTTP connection
        const Aws::String& GetURI() const { return m_URI; }

        //returns the method of which the HTTP request will take. GET, POST, DELETE, PUT, or HEAD
        Aws::Http::HttpMethod GetMethod() const { return m_method; }

        //returns the list of extra headers to include in the request
        const Headers & GetHeaders() const { return m_headers; }

        //returns the stream for the body of the request
        const std::shared_ptr<std::stringstream> & GetBodyStream() const { return m_bodyStream; }

        //returns the function of which to feed back the TEXT that the HTTP call resulted in. The function also requires the HTTPResponseCode indicating if the call was successful or failed
        const TextCallback & GetCallback() const { return m_callback; }

    private:
        Aws::String                             m_URI;
        Aws::Http::HttpMethod                   m_method;
        Headers                                 m_headers;
        std::shared_ptr<std::stringstream>      m_bodyStream;
        TextCallback                            m_callback;
    };
    
    inline TextParameters::TextParameters(const AZStd::string& URI, Aws::Http::HttpMethod method, const TextCallback& callback)
        : m_URI(URI.c_str())
        , m_method(method)
        , m_callback(callback)
    {
    }

    inline TextParameters::TextParameters(const AZStd::string& URI, Aws::Http::HttpMethod method, const Headers& headers, const TextCallback& callback)
        : m_URI(URI.c_str())
        , m_method(method)
        , m_headers(headers)
        , m_callback(callback)
    {
    }

    inline TextParameters::TextParameters(const AZStd::string& URI, Aws::Http::HttpMethod method, const Headers& headers, const AZStd::string& body, const TextCallback& callback)
        : m_URI(URI.c_str())
        , m_method(method)
        , m_headers(headers)
        , m_bodyStream( std::make_shared<std::stringstream>(body.c_str()) )
        , m_callback(callback)
    {
    }
}
