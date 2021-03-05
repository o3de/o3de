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

// This file should only be included Once in DLL module.

#include <platform.h>

#if defined(AZ_MONOLITHIC_BUILD)
#   error It is not allowed to have AZ_MONOLITHIC_BUILD defined
#endif

#include <platform_implRC.h>

#include <Random.h>

#include <ISystem.h>


#if defined(AZ_PLATFORM_WINDOWS)
#define TLSALLOC(k)     (*(k)=TlsAlloc(), TLS_OUT_OF_INDEXES==*(k))
#define TLSFREE(k)      (!TlsFree(k))
#define TLSGET(k)       TlsGetValue(k)
#define TLSSET(k, a)    (!TlsSetValue(k, a))
#elif AZ_TRAIT_OS_PLATFORM_APPLE || defined(AZ_PLATFORM_LINUX)
#define TLSALLOC(k)     pthread_key_create(k, 0)
#define TLSFREE(k)       pthread_key_delete(k)
#define TLSGET(k)       pthread_getspecific(k)
#define TLSSET(k, a)    pthread_setspecific(k, a)
#else
#error TLS Not supported!!
#endif

IRCLog* g_pRCLog = 0;

void SetRCLog(IRCLog* pRCLog)
{
    g_pRCLog = pRCLog;
}

void RCLog(const char* szFormat, ...)
{
    va_list args;
    va_start(args, szFormat);
    if (g_pRCLog)
    {
        g_pRCLog->LogV(IRCLog::eType_Info, szFormat, args);
    }
    else
    {
        vprintf(szFormat, args);
        printf("\n");
        fflush(stdout);
    }
    va_end(args);
}

void RCLogWarning(const char* szFormat, ...)
{
    va_list args;
    va_start(args, szFormat);
    if (g_pRCLog)
    {
        g_pRCLog->LogV(IRCLog::eType_Warning, szFormat, args);
    }
    else
    {
        vprintf(szFormat, args);
        printf("\n");
        fflush(stdout);
    }
    va_end(args);
}

void RCLogError(const char* szFormat, ...)
{
    va_list args;
    va_start (args, szFormat);
    if (g_pRCLog)
    {
        g_pRCLog->LogV(IRCLog::eType_Error, szFormat, args);
    }
    else
    {
        vprintf(szFormat, args);
        printf("\n");
        fflush(stdout);
    }
    va_end(args);
}

void RCLogContext(const char* szMessage)
{
    if (g_pRCLog)
    {
        g_pRCLog->Log(IRCLog::eType_Context, szMessage);
    }
    else
    {
        printf("%s\n", szMessage);
        fflush(stdout);
    }
}

void RCLogSummary(const char* szFormat, ...)
{
    va_list args;
    va_start(args, szFormat);
    if (g_pRCLog)
    {
        g_pRCLog->LogV(IRCLog::eType_Summary, szFormat, args);
    }
    else
    {
        vprintf(szFormat, args);
        printf("\n");
        fflush(stdout);
    }
    va_end(args);
}

//////////////////////////////////////////////////////////////////////////
// Log important data that must be printed regardless verbosity.

void PlatformLogOutput(const char*, ...) PRINTF_PARAMS(1, 2);

inline void PlatformLogOutput(const char* format, ...)
{
    assert(g_pRCLog);
    if (g_pRCLog)
    {
        va_list args;
        va_start(args, format);
        g_pRCLog->LogV(IRCLog::eType_Error,  format, args);
        va_end(args);
    }
}

