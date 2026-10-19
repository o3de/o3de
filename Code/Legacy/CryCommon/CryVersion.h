/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// Description : Defines File version structure.
#include <AzCore/StringFunc/StringFunc.h>

//////////////////////////////////////////////////////////////////////////
/** This class keeps file version information.
*/
struct SFileVersion
{
    int v[4];

    SFileVersion()
    {
        v[0] = v[1] = v[2] = v[3] = 0;
    }
    SFileVersion(const int vers[])
    {
        v[0] = vers[0];
        v[1] = vers[1];
        v[2] = vers[2];
        v[3] = 1;
    }

    void Set(const char* s)
    {
        v[0] = v[1] = v[2] = v[3] = 0;

        AZStd::vector<AZStd::string> tokens;
        AZ::StringFunc::Tokenize(s, tokens, '.', true, true);
        if (tokens.size()>0)
        {
            v[0] = AZ::StringFunc::ToInt(tokens[0].c_str());
        }
        if (tokens.size()>1)
        {
            v[1] = AZ::StringFunc::ToInt(tokens[1].c_str());
        }
        if (tokens.size()>2)
        {
            v[2] = AZ::StringFunc::ToInt(tokens[2].c_str());
        }
        if (tokens.size()>3)
        {
            v[3] = AZ::StringFunc::ToInt(tokens[3].c_str());
        }
    }

    explicit SFileVersion(const char* s)
    {
        Set(s);
    }

    bool operator <(const SFileVersion& v2) const
    {
        if (v[3] < v2.v[3])
        {
            return true;
        }
        if (v[3] > v2.v[3])
        {
            return false;
        }

        if (v[2] < v2.v[2])
        {
            return true;
        }
        if (v[2] > v2.v[2])
        {
            return false;
        }

        if (v[1] < v2.v[1])
        {
            return true;
        }
        if (v[1] > v2.v[1])
        {
            return false;
        }

        if (v[0] < v2.v[0])
        {
            return true;
        }
        if (v[0] > v2.v[0])
        {
            return false;
        }
        return false;
    }
    bool operator ==(const SFileVersion& v1) const
    {
        if (v[0] == v1.v[0] && v[1] == v1.v[1] &&
            v[2] == v1.v[2] && v[3] == v1.v[3])
        {
            return true;
        }
        return false;
    }
    bool operator >(const SFileVersion& v1) const
    {
        return !(*this < v1);
    }
    bool operator >=(const SFileVersion& v1) const
    {
        return (*this == v1) || (*this > v1);
    }
    bool operator <=(const SFileVersion& v1) const
    {
        return (*this == v1) || (*this < v1);
    }

    int& operator[](int i)       { return v[i]; }
    int  operator[](int i) const { return v[i]; }

    template <size_t size>
    void ToShortString(char(&buffer)[size]) const
    {
        azsnprintf(buffer, size, "%d.%d.%d", v[2], v[1], v[0]);
    }

    void ToShortString(char* s, size_t bufferSize) const
    {
        azsnprintf(s, bufferSize, "%d.%d.%d", v[2], v[1], v[0]);
    }

    template <size_t size>
    void ToString(char(&buffer)[size]) const
    {
        azsnprintf(buffer, size, "%d.%d.%d.%d", v[3], v[2], v[1], v[0]);
    }

    void ToString(char* s, size_t bufferSize) const
    {
        azsnprintf(s, bufferSize, "%d.%d.%d.%d", v[3], v[2], v[1], v[0]);
    }
};
