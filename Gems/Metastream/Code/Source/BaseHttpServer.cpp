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
#include "Metastream_precompiled.h"
#include "BaseHttpServer.h"
#include "DataCache.h"

#include <sstream>

using namespace Metastream;

HttpResponse BaseHttpServer::GetDataTables() const
{
    HttpResponse response;
    response.code = 200;
    response.body = m_cache->GetDatabasesJSON().c_str();
    return response;
}

HttpResponse BaseHttpServer::GetDataKeys(const std::string& tableName) const
{
    int code = 404;
    std::string body (m_cache->GetTableKeysJSON(tableName) );

    if (!body.empty())
    {
        code = 200;
    }

    HttpResponse response;
    response.code = code;
    response.body = body.c_str();
    return response;
}

HttpResponse BaseHttpServer::GetDataValue(const std::string& tableName, const std::string& key) const
{
    std::vector<std::string> keys;
    keys.push_back(key);

    return GetDataValues(tableName, keys);
}

Metastream::HttpResponse Metastream::BaseHttpServer::GetDataValues(const std::string& tableName, const std::vector<std::string>& keys) const
{
    int code = 404;
    std::string body(m_cache->GetTableKeyValuesJSON(tableName, keys));

    if (!body.empty())
    {
        code = 200;
    }

    HttpResponse response;
    response.code = code;
    response.body = body.c_str();
    return response;
}

std::map<std::string, std::string> BaseHttpServer::TokenizeQuery(const char* queryString)
{
    std::map<std::string, std::string> queryMap;

    std::stringstream query(queryString);
    std::string pair;

    while (std::getline(query, pair, '&'))
    {
        auto pos = pair.find("=");
        if (pos != std::string::npos)
        {
            queryMap[pair.substr(0, pos)] = pair.substr(pos + 1, std::string::npos);
        }
    }

    return queryMap;
}

std::string BaseHttpServer::StrReplace(std::string srcString, const std::string & query, const std::string & replacement)
{
    if (!query.empty())
    {
        size_t pos = 0;
        while ((pos = srcString.find(query, pos)) != std::string::npos)
        {
            srcString.replace(pos, query.length(), replacement);
            pos += replacement.length();
        }
    }
    return srcString;
}


std::vector<std::string> Metastream::BaseHttpServer::SplitValueList(const std::string& value, char separator)
{
    std::vector<std::string> ret;
    size_t start = 0;
    size_t split;

    do
    {
        split = value.find(separator, start);
        ret.push_back(std::string(value, start, split-start));
        start = split + 1;
    } while (split != std::string::npos);

    return ret;
}


std::string BaseHttpServer::SerializeHeaders(const std::map<std::string, std::string>& headers)
{
    std::stringstream serializedHeaders;

    for (auto it = headers.begin(); it != headers.end(); it++)
    {
        serializedHeaders << it->first << ": " << it->second << "\r\n";
    }
    serializedHeaders << "\r\n";

    return std::string(serializedHeaders.str());
}

std::string BaseHttpServer::HttpStatus(int code)
{
    const char* description;

    switch (code)
    {
    case 100: description = "Continue"; break;
    case 101: description = "Switching Protocols"; break;
    case 200: description = "OK"; break;
    case 201: description = "Created"; break;
    case 202: description = "Accepted"; break;
    case 203: description = "Non-Authoritative Information"; break;
    case 204: description = "No Content"; break;
    case 205: description = "Reset Content"; break;
    case 206: description = "Partial Content"; break;
    case 300: description = "Multiple Choices"; break;
    case 301: description = "Moved Permanently"; break;
    case 302: description = "Moved Temporarily"; break;
    case 303: description = "See Other"; break;
    case 304: description = "Not Modified"; break;
    case 305: description = "Use Proxy"; break;
    case 400: description = "Bad Request"; break;
    case 401: description = "Unauthorized"; break;
    case 402: description = "Payment Required"; break;
    case 403: description = "Forbidden"; break;
    case 404: description = "Not Found"; break;
    case 405: description = "Method Not Allowed"; break;
    case 406: description = "Not Acceptable"; break;
    case 407: description = "Proxy Authentication Required"; break;
    case 408: description = "Request Time-out"; break;
    case 409: description = "Conflict"; break;
    case 410: description = "Gone"; break;
    case 411: description = "Length Required"; break;
    case 412: description = "Precondition Failed"; break;
    case 413: description = "Request Entity Too Large"; break;
    case 414: description = "Request-URI Too Large"; break;
    case 415: description = "Unsupported Media Type"; break;
    case 500: description = "Internal Server Error"; break;
    case 501: description = "Not Implemented"; break;
    case 502: description = "Bad Gateway"; break;
    case 503: description = "Service Unavailable"; break;
    case 504: description = "Gateway Time-out"; break;
    case 505: description = "HTTP Version not supported"; break;
    default:
        description = "";
    }

    std::stringstream httpStatus;
    httpStatus << "HTTP/1.1 " << code << " " << description << "\r\n";

    return std::string(httpStatus.str());
}