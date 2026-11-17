/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// Description : Implements the command line interface ICmdLine.


#include <ICmdLine.h>
#include "CmdLineArg.h"


class CCmdLine
    : public ICmdLine
{
public:
    CCmdLine(const char* commandLine);
    virtual ~CCmdLine();

    virtual const ICmdLineArg* GetArg(int n) const;
    virtual int GetArgCount() const;
    virtual const ICmdLineArg* FindArg(const ECmdLineArgType ArgType, const char* name, bool caseSensitive = false) const;
    virtual const char* GetCommandLine() const { return m_sCmdLine.c_str(); };

private:
    void PushCommand(const AZStd::string& sCommand, const AZStd::string& sParameter);
    AZStd::string Next(char*& str);

    AZStd::string m_sCmdLine;
    std::vector<CCmdLineArg>    m_args;
};
