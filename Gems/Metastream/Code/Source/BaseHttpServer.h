/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <map>
#include <string>
#include <vector>

namespace Metastream
{
    class DataCache;

    // Currently supported HTTP methods
    enum class HttpMethod
    {
        GET
    };

    typedef struct
    {
        HttpMethod method;
        std::map<std::string, std::string> headers;
        std::string uri;
        std::map<std::string, std::string> query;
        std::string body;
    } HttpRequest;

    typedef struct 
    {
        int code;
        std::map<std::string, std::string> headers;
        std::string body;
    } HttpResponse;
    
    class BaseHttpServer
    {
    public:
        BaseHttpServer(const DataCache* cache)
            : m_cache(cache)
        {
        }
        virtual ~BaseHttpServer() {}

        // Start the HTTP server with the given options
        virtual bool Start(const std::string & civetOptions) = 0;

        // Stop the HTTP Server
        virtual void Stop() = 0;

        // Return a JSON list of all data tables that are exposed.
        HttpResponse GetDataTables() const;

        // Return a JSON list of all data keys that are exposed for a specific table.
        HttpResponse GetDataKeys(const std::string& tableName) const;

        // Return a JSON object containing a particular value.
        HttpResponse GetDataValue(const std::string& tableName, const std::string& key) const;

        // Return a JSON object containing a set of values.
        HttpResponse GetDataValues(const std::string& tableName, const std::vector<std::string>& keys) const;

        //---------------------------------------------------------------------
        // Helper functions

        // Split query into key-value pairs
        static std::map<std::string, std::string> TokenizeQuery(const char* queryString);

        // simple string substitution
        static std::string StrReplace(std::string srcString, const std::string & query, const std::string & replacement);

        // Split a query value if it is a comma separated list (e.g. key=value1,value2,...)
        static std::vector<std::string> SplitValueList(const std::string& value, char separator);

        // Serialize header name-value pairs into a string for writing back to response
        static std::string SerializeHeaders(const std::map<std::string, std::string>& headers);

        // Prepare HTTP status for writing back to response (e.g. "HTTP/1.1 200 OK")
        static std::string HttpStatus(int code);

    private:
        const DataCache* m_cache;
    };
} // namespace Metastream
