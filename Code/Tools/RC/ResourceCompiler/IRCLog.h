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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_IRCLOG_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_IRCLOG_H
#pragma once


#include <stdarg.h>


// This interface is used by RC and convertors to log events
struct IRCLog
{
    enum EType
    {
        eType_Info,
        eType_Warning,
        eType_Error,
        eType_Context,
        eType_Summary
    };

    virtual ~IRCLog()
    {
    }

    virtual void LogV(const EType eType, const char* szFormat, va_list args) = 0;
    virtual void Log(const EType eType, const char* szMessage) = 0;
};


void SetRCLog(IRCLog* pRCLog);
void RCLog(const char* szFormat, ...);
void RCLogWarning(const char* szFormat, ...);
void RCLogError(const char* szFormat, ...);
void RCLogContext(const char* szMessage);
void RCLogSummary(const char* szFormat, ...);

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_IRCLOG_H
