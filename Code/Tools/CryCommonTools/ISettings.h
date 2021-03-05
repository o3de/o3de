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

#ifndef CRYINCLUDE_CRYCOMMONTOOLS_ISETTINGS_H
#define CRYINCLUDE_CRYCOMMONTOOLS_ISETTINGS_H
#pragma once


class ISettings
{
public:
    virtual bool GetSettingString(char* buffer, int bufferSizeInBytes, const char* key) = 0;
    virtual bool GetSettingInt(int& value, const char* key) = 0;
};

inline bool GetSettingByRef(ISettings* settings, const string& key, string& value)
{
    char buffer[1024];
    bool success = false;
    if (settings)
    {
        success = settings->GetSettingString(buffer, sizeof(buffer), key.c_str());
    }
    if (success)
    {
        value = buffer;
    }
    return success;
}

inline bool GetSettingByRef(ISettings* settings, const string& key, int& value)
{
    return settings->GetSettingInt(value, key.c_str());
}

template <typename T>
inline T GetSetting(ISettings* settings, const string& key, const T& dflt)
{
    T value;
    if (!GetSettingByRef(settings, key, value))
    {
        value = dflt;
    }
    return value;
}

#endif // CRYINCLUDE_CRYCOMMONTOOLS_ISETTINGS_H
