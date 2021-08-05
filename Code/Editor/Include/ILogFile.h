/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
