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
#ifndef CRYINCLUDE_EDITORCORE_INCLUDE_ILOGFILE_H
#define CRYINCLUDE_EDITORCORE_INCLUDE_ILOGFILE_H

#pragma once

struct ILogFile
{
    virtual ~ILogFile() = default;
    
    virtual const char* GetLogFileName() = 0;

    //! Write to log spanpshot of current process memory usage.
    virtual QString GetMemUsage() = 0;

    virtual void WriteString(const char* pszString) = 0;
    virtual void WriteLine(const char* pszLine) = 0;
    virtual void FormatLine(const char* pszMessage, ...) = 0;

    // logs some useful information
    // should be called after CryLog() is available
    virtual void AboutSystem() = 0;

    virtual void Warning(const char* format, ...) = 0;
};

#endif // CRYINCLUDE_EDITORCORE_INCLUDE_ILOGFILE_H
