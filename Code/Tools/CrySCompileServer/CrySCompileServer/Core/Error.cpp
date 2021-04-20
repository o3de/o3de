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

#include "StdTypes.hpp"
#include "Error.hpp"
#include <tinyxml/tinyxml.h>
#include "Server/CrySimpleErrorLog.hpp"
#include "Server/CrySimpleJob.hpp"

#include <AzCore/base.h>
#include <AzCore/Debug/Trace.h>
#include <time.h>

ICryError::ICryError(EErrorType t)
    : m_eType(t)
    , m_numDupes(0)
{
}

void logmessage(const char* text, ...)
{
    va_list arg;
    va_start(arg, text);

    char szBuffer[256];
    char* error = szBuffer;
    int bufferlen = sizeof(szBuffer) - 1;
    memset(szBuffer, 0, sizeof(szBuffer));

    long req = CCrySimpleJob::GlobalRequestNumber();

    int ret = azsnprintf(error, bufferlen, "%8ld | ", req);

    if (ret <= 0)
    {
        return;
    }
    error += ret;
    bufferlen -= ret;

    time_t ltime;
    time(&ltime);
    tm today;
#if defined(AZ_PLATFORM_WINDOWS)
    localtime_s(&today, &ltime);
#else
    localtime_r(&ltime, &today);
#endif
    ret = (int)strftime(error, bufferlen, "%d/%m %H:%M:%S | ", &today);

    if (ret <= 0)
    {
        return;
    }
    error += ret;
    bufferlen -= ret;

    vsnprintf(error, bufferlen, text, arg);

    AZ_TracePrintf(0, szBuffer);

    va_end(arg);
}
