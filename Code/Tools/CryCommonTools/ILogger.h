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

#ifndef CRYINCLUDE_CRYCOMMONTOOLS_ILOGGER_H
#define CRYINCLUDE_CRYCOMMONTOOLS_ILOGGER_H
#pragma once


#include <stdarg.h>
#include <stdio.h>

class ILogger
{
public:
    enum ESeverity
    {
        eSeverity_Debug,
        eSeverity_Info,
        eSeverity_Warning,
        eSeverity_Error
    };

    virtual ~ILogger()
    {
    }

    void Log(ESeverity eSeverity, const char* const format, ...)
    {
        char buffer[2048];
        {
            va_list args;
            va_start(args, format);
            _vsnprintf_s(buffer, sizeof(buffer), sizeof(buffer) - 1, format, args);
            va_end(args);
        }
        LogImpl(eSeverity, buffer);
    }

protected:
    virtual void LogImpl(ESeverity eSeverity, const char* text) = 0;
};

#endif // CRYINCLUDE_CRYCOMMONTOOLS_ILOGGER_H
