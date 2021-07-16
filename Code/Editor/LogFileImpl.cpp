/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorDefs.h"

#include "LogFileImpl.h"

// Interface which we expose via GetIEditor()
#include "LogFile.h"

const char* CLogFileImpl::GetLogFileName()
{
    return CLogFile::GetLogFileName();
}

QString CLogFileImpl::GetMemUsage()
{
    return CLogFile::GetMemUsage();
}

void CLogFileImpl::WriteString(const char* pszString)
{
    CLogFile::WriteString(pszString);
}

void CLogFileImpl::WriteLine(const char* pszLine)
{
    CLogFile::WriteLine(pszLine);
}

void CLogFileImpl::FormatLine(const char* pszMessage, ...)
{
    va_list ArgList;
    va_start(ArgList, pszMessage);
    CLogFile::FormatLineV(pszMessage, ArgList);
    va_end(ArgList);
}

void CLogFileImpl::AboutSystem()
{
    CLogFile::AboutSystem();
}

void CLogFileImpl::Warning(const char* format, ...)
{
    va_list ArgList;
    va_start(ArgList, format);
    ::WarningV(format, ArgList);
    va_end(ArgList);
}
