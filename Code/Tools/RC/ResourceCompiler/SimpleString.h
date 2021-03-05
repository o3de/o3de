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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_SIMPLESTRING_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_SIMPLESTRING_H
#pragma once

#include <cstring>      // memcpy()

// Implementation note: we cannot use name CSimpleString because it is already used by ATL
class SimpleString
{
public:
    SimpleString()
        : m_str((char*)0)
        , m_length(0)
    {
    }

    explicit SimpleString(const char* s)
        : m_str((char*)0)
        , m_length(0)
    {
        *this = s;
    }

    SimpleString(const SimpleString& str)
        : m_str((char*)0)
        , m_length(0)
    {
        *this = str;
    }

    ~SimpleString()
    {
        if (m_str)
        {
            delete [] m_str;
        }
    }

    SimpleString& operator=(const SimpleString& str)
    {
        if (this != &str)
        {
            *this = str.m_str;
        }
        return *this;
    }

    SimpleString& operator=(const char* s)
    {
        char* const oldStr = m_str;
        m_str = (char*)0;
        m_length = 0;
        if (s && s[0])
        {
            m_length = strlen(s);
            m_str = new char[m_length + 1];
            memcpy(m_str, s, m_length + 1);
        }
        if (oldStr)
        {
            delete [] oldStr;
        }
        return *this;
    }

    operator const char*() const
    {
        return c_str();
    }

    const char* c_str() const
    {
        return (m_str) ? m_str : "";
    }

    size_t length() const
    {
        return m_length;
    }

private:
    char* m_str;
    size_t m_length;
};

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_SIMPLESTRING_H
