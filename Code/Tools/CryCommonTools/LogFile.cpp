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

#include <platform.h>
#include "LogFile.h"

LogFile::LogFile(const char* const filename)
    : m_file(0)
    , m_hasWarnings(false)
    , m_hasErrors(false)
{
    m_file = std::fopen(filename, "w");
}

LogFile::~LogFile()
{
    if (m_file)
    {
        fclose(m_file);
    }
}

bool LogFile::IsOpen() const
{
    return m_file != 0;
}

bool LogFile::HasWarningsOrErrors() const
{
    return m_hasWarnings || m_hasErrors;
}

void LogFile::LogImpl(ESeverity eSeverity, const char* const text)
{
    const char* severityMessage = 0;
    switch (eSeverity)
    {
    case eSeverity_Debug:
        severityMessage = "   ";
        break;
    case eSeverity_Info:
        severityMessage = "   ";
        break;
    case eSeverity_Warning:
        severityMessage = "W: ";
        break;
    case eSeverity_Error:
        severityMessage = "E: ";
        break;
    default:
        severityMessage = "?: ";
        break;
    }

    if (eSeverity == eSeverity_Warning)
    {
        m_hasWarnings = true;
    }
    if (eSeverity == eSeverity_Error)
    {
        m_hasErrors = true;
    }

    if (m_file)
    {
        fprintf(m_file, "%s%s\n", severityMessage, text);
        fflush(m_file);
    }
}
