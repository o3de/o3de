/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef CRYINCLUDE_EDITOR_LOGFILEIMPL_H
#define CRYINCLUDE_EDITOR_LOGFILEIMPL_H
#pragma once

#include <Include/ILogFile.h>

class CLogFileImpl
    : public ILogFile
{
public:
    virtual const char* GetLogFileName() override;

    //! Write to log spanpshot of current process memory usage.
    virtual QString GetMemUsage() override;

    virtual void WriteString(const char* pszString) override;
    virtual void WriteLine(const char* pszLine) override;
    virtual void FormatLine(const char* pszMessage, ...) override;

    // logs some useful information
    // should be called after CryLog() is available
    virtual void AboutSystem() override;

    virtual void Warning(const char* format, ...) override;
};

#endif // CRYINCLUDE_EDITOR_LOGFILEIMPL_H
