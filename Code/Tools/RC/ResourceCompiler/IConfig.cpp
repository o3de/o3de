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

#include "IConfig.h"
#include "IRCLog.h"


bool IConfig::HasKey(const char* const key, const int ePriMask) const
{
    const char* pValue;
    return GetKeyValue(key, pValue, ePriMask);
}


IConfig::EResult IConfig::Get(const char* const key, bool& value, const int ePriMask) const
{
    const char* pValue;
    if (!GetKeyValue(key, pValue, ePriMask))
    {
        return eResult_KeyIsNotFound;
    }

    {
        int tmpInt;
        if (azsscanf(pValue, "%d", &tmpInt) == 1)
        {
            value = (tmpInt != 0);
            return eResult_Success;
        }
    }

    if (!azstricmp(pValue, "true") ||
        !azstricmp(pValue, "yes") ||
        !azstricmp(pValue, "enable") ||
        !azstricmp(pValue, "y") ||
        !azstricmp(pValue, "t"))
    {
        value = true;
        return eResult_Success;
    }

    if (!azstricmp(pValue, "false") ||
        !azstricmp(pValue, "no") ||
        !azstricmp(pValue, "disable") ||
        !azstricmp(pValue, "n") ||
        !azstricmp(pValue, "f"))
    {
        value = false;
        return eResult_Success;
    }

    return eResult_ValueIsEmptyOrBad;
}

IConfig::EResult IConfig::Get(const char* const key, int& value, const int ePriMask) const
{
    const char* pValue;
    if (!GetKeyValue(key, pValue, ePriMask))
    {
        return eResult_KeyIsNotFound;
    }

    int tmpValue;
    if (azsscanf(pValue, "%d", &tmpValue) == 1)
    {
        value = tmpValue;
        return eResult_Success;
    }

    return eResult_ValueIsEmptyOrBad;
}

IConfig::EResult IConfig::Get(const char* const key, float& value, const int ePriMask) const
{
    const char* pValue;
    if (!GetKeyValue(key, pValue, ePriMask))
    {
        return eResult_KeyIsNotFound;
    }

    float tmpValue;
    if (azsscanf(pValue, "%f", &tmpValue) == 1)
    {
        value = tmpValue;
        return eResult_Success;
    }

    return eResult_ValueIsEmptyOrBad;
}

IConfig::EResult IConfig::Get(const char* const key, string& value, const int ePriMask) const
{
    const char* pValue;
    if (!GetKeyValue(key, pValue, ePriMask))
    {
        return eResult_KeyIsNotFound;
    }

    if (pValue && pValue[0])
    {
        value = pValue;
        return eResult_Success;
    }

    return eResult_ValueIsEmptyOrBad;
}


bool IConfig::GetAsBool(const char* const key, const bool keyIsNotFoundValue, const bool emptyOrBadValue, const int ePriMask) const
{
    bool value;
    switch (Get(key, value, ePriMask))
    {
    case eResult_Success:
        return value;
    case eResult_KeyIsNotFound:
        return keyIsNotFoundValue;
    default:
        return emptyOrBadValue;
    }
}

int IConfig::GetAsInt(const char* const key, const int keyIsNotFoundValue, const int emptyOrBadValue, const int ePriMask) const
{
    int value;
    switch (Get(key, value, ePriMask))
    {
    case eResult_Success:
        return value;
    case eResult_KeyIsNotFound:
        return keyIsNotFoundValue;
    default:
        return emptyOrBadValue;
    }
}

float IConfig::GetAsFloat(const char* const key, const float keyIsNotFoundValue, const float emptyOrBadValue, const int ePriMask) const
{
    float value;
    switch (Get(key, value, ePriMask))
    {
    case eResult_Success:
        return value;
    case eResult_KeyIsNotFound:
        return keyIsNotFoundValue;
    default:
        return emptyOrBadValue;
    }
}

string IConfig::GetAsString(const char* const key, const char* const keyIsNotFoundValue, const char* const emptyOrBadValue, const int ePriMask) const
{
    string value;
    switch (Get(key, value, ePriMask))
    {
    case eResult_Success:
        return value;
    case eResult_KeyIsNotFound:
        return string(keyIsNotFoundValue);
    default:
        return string(emptyOrBadValue);
    }
}


static inline void SkipWhitespace(const char*& p)
{
    while (*p && *p <= ' ')
    {
        ++p;
    }
}

void IConfig::SetFromString(const EConfigPriority ePri, const char* p)
{
    assert(Util::isPowerOfTwo(ePri));

    if (p == 0)
    {
        return;
    }

    while (*p)
    {
        string sKey;
        string sValue;

        SkipWhitespace(p);

        if (*p != '/')
        {
            RCLog("Config string format is invalid ('/' expected): '%s'", p);
            break;
        }
        ++p;   // jump over '/'

        while (IsValidNameChar(*p))
        {
            sKey += *p++;
        }

        SkipWhitespace(p);

        if (*p != '=')
        {
            RCLog("Config string format is invalid ('=' expected): '%s'", p);
            break;
        }
        ++p;   // jump over '='

        SkipWhitespace(p);

        if (*p == '\"')   // in quotes
        {
            ++p;
            while (*p && *p != '\"')   // value
            {
                sValue += *p++;
            }
            if (*p == '\"')
            {
                ++p;
            }
        }
        else   // without quotes
        {
            while (IsValidNameChar(*p))   // value
            {
                sValue += *p++;
            }
        }

        SkipWhitespace(p);

        sKey.Trim();
        sValue.Trim();

        SetKeyValue(ePri, sKey, sValue);
    }
}

