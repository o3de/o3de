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
