/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Defines File version structure.


#ifndef CRYINCLUDE_CRYCOMMON_CRYVERSION_H
#define CRYINCLUDE_CRYCOMMON_CRYVERSION_H
#pragma once

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

        char t[50];
        const size_t len = (std::min)(strlen(s), sizeof(t) - 1);
        memcpy(t, s, len);
        t[len] = 0;

        char* p;
        [[maybe_unused]] char* next = nullptr;
        [[maybe_unused]] size_t strmax = sizeof(t);
        p = azstrtok(t, &strmax, ".", &next);
        if (!p)
        {
            return;
        }
        v[3] = atoi(p);
        p = azstrtok(nullptr, &strmax, ".", &next);
        if (!p)
        {
            return;
        }
        v[2] = atoi(p);
        p = azstrtok(nullptr, &strmax, ".", &next);
        if (!p)
        {
            return;
        }
        v[1] = atoi(p);
        p = azstrtok(nullptr, &strmax, ".", &next);
        if (!p)
        {
            return;
        }
        v[0] = atoi(p);
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
        sprintf_s(buffer, size, "%d.%d.%d", v[2], v[1], v[0]);
    }

    void ToShortString(char* s, size_t bufferSize) const
    {
        sprintf_s(s, bufferSize, "%d.%d.%d", v[2], v[1], v[0]);
    }

    template <size_t size>
    void ToString(char(&buffer)[size]) const
    {
        sprintf_s(buffer, size, "%d.%d.%d.%d", v[3], v[2], v[1], v[0]);
    }

    void ToString(char* s, size_t bufferSize) const
    {
        sprintf_s(s, bufferSize, "%d.%d.%d.%d", v[3], v[2], v[1], v[0]);
    }
};

#endif // CRYINCLUDE_CRYCOMMON_CRYVERSION_H
