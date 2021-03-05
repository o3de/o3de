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

#ifndef CRYINCLUDE_CRYCOMMONTOOLS_LOGFILE_H
#define CRYINCLUDE_CRYCOMMONTOOLS_LOGFILE_H
#pragma once


#include "ILogger.h"

class LogFile
    : public ILogger
{
public:
    LogFile(const char* filename);
    ~LogFile();

    bool IsOpen() const;
    bool HasWarningsOrErrors() const;

    // ILogger
    virtual void LogImpl(ESeverity eSeverity, const char* message);

private:
    std::FILE* m_file;
    bool m_hasWarnings;
    bool m_hasErrors;
};

#endif // CRYINCLUDE_CRYCOMMONTOOLS_LOGFILE_H
