/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Framework/RequestBuilder.h>

namespace AWSCore
{
    RequestBuilder::RequestBuilder()
        : m_httpMethod(Aws::Http::HttpMethod::HTTP_GET)
    {
    }

    bool RequestBuilder::SetPathParameterUnescaped(const char* key, const char* value)
    {
        size_t start_pos = m_requestUrl.find(key);
        if (start_pos == Aws::String::npos)
        {
            AZ_Error("TODO", false, "Key \"%s\" not found in url \"%s\".", key, m_requestUrl.c_str());
            return false;
        }
        else
        {
            m_requestUrl.replace(start_pos, strlen(key), value);
            return true;
        }
    }

    bool RequestBuilder::SetPathParameter(const char* key, const AZStd::string& value)
    {
        return SetPathParameterUnescaped(key, escape(value.c_str()).c_str());
    }

    bool RequestBuilder::SetPathParameter(const char* key, const char* value)
    {
        return SetPathParameterUnescaped(key, escape(value).c_str());
    }

    bool RequestBuilder::SetPathParameter(const char* key, double value)
    {
        return SetPathParameterUnescaped(key, AZStd::string::format("%f", value).c_str());
    }

    bool RequestBuilder::SetPathParameter(const char* key, bool value)
    {
        return SetPathParameterUnescaped(key, value ? "true" : "false");
    }

    bool RequestBuilder::SetPathParameter(const char* key, int value)
    {
        return SetPathParameterUnescaped(key, AZStd::string::format("%i", value).c_str());
    }

    bool RequestBuilder::SetPathParameter(const char* key, unsigned value)
    {
        return SetPathParameterUnescaped(key, AZStd::string::format("%u", value).c_str());
    }

    bool RequestBuilder::SetPathParameter(const char* key, int64_t value)
    {
        return SetPathParameterUnescaped(key, AZStd::string::format("%" PRId64, static_cast<int64_t>(value)).c_str());
    }

    bool RequestBuilder::SetPathParameter(const char* key, uint64_t value)
    {
        return SetPathParameterUnescaped(key, AZStd::string::format("%" PRIu64, static_cast<uint64_t>(value)).c_str());
    }

    bool RequestBuilder::AddQueryParameterUnescaped(const char* name, const char* value)
    {
        m_requestUrl.append(m_requestUrl.find('?') == Aws::String::npos ? "?" : "&");
        m_requestUrl.append(name);
        m_requestUrl.append("=");
        m_requestUrl.append(value);
        return true;
    }

    bool RequestBuilder::AddQueryParameter(const char* name, const AZStd::string& value)
    {
        return AddQueryParameterUnescaped(name, escape(value.c_str()).c_str());
    }

    bool RequestBuilder::AddQueryParameter(const char* name, const char* value)
    {
        return AddQueryParameterUnescaped(name, escape(value).c_str());
    }

    bool RequestBuilder::AddQueryParameter(const char* name, double value)
    {
        return AddQueryParameterUnescaped(name, AZStd::string::format("%f", value).c_str());
    }

    bool RequestBuilder::AddQueryParameter(const char* name, bool value)
    {
        return AddQueryParameterUnescaped(name, value ? "true" : "false");
    }

    bool RequestBuilder::AddQueryParameter(const char* name, int value)
    {
        return AddQueryParameterUnescaped(name, AZStd::string::format("%d", value).c_str());
    }

    bool RequestBuilder::AddQueryParameter(const char* name, unsigned value)
    {
        return AddQueryParameterUnescaped(name, AZStd::string::format("%u", value).c_str());
    }

    bool RequestBuilder::AddQueryParameter(const char* name, int64_t value)
    {
        return AddQueryParameterUnescaped(name, AZStd::string::format("%" PRId64, static_cast<int64_t>(value)).c_str());
    }

    bool RequestBuilder::AddQueryParameter(const char* name, uint64_t value)
    {
        return AddQueryParameterUnescaped(name, AZStd::string::format("%" PRIu64, static_cast<uint64_t>(value)).c_str());
    }

    Aws::String RequestBuilder::escape(const char* value)
    {

        Aws::String target;

        while (*value != '\0')
        {
            switch (*value)
            {

            case ' ':
                target.append("%20", 3);
                break;

            case '!':
                target.append("%21", 3);
                break;

            case '#':
                target.append("%23", 3);
                break;

            case '$':
                target.append("%24", 3);
                break;

            case '%':
                target.append("%25", 3);
                break;

            case '&':
                target.append("%26", 3);
                break;

            case '\'':
                target.append("%27", 3);
                break;

            case '(':
                target.append("%28", 3);
                break;

            case ')':
                target.append("%29", 3);
                break;

            case '*':
                target.append("%2A", 3);
                break;

            case '+':
                target.append("%2B", 3);
                break;

            case ',':
                target.append("%2C", 3);
                break;

            case '/':
                target.append("%2F", 3);
                break;

            case ':':
                target.append("%3A", 3);
                break;

            case ';':
                target.append("%3B", 3);
                break;

            case '=':
                target.append("%3D", 3);
                break;

            case '?':
                target.append("%3F", 3);
                break;

            case '@':
                target.append("%40", 3);
                break;

            case '[':
                target.append("%5B", 3);
                break;

            case ']':
                target.append("%5D", 3);
                break;

            default:
                target.push_back(*value);
                break;

            }

            ++value;

        }

        return target;
    }

} // namespace AWSCore

