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

// Description : Implements the command line interface ICmdLine.

#ifndef CRYINCLUDE_CRYSYSTEM_CMDLINE_H
#define CRYINCLUDE_CRYSYSTEM_CMDLINE_H

#pragma once


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
    virtual const char* GetCommandLine() const { return m_sCmdLine; };

private:
    void PushCommand(const string& sCommand, const string& sParameter);
    string Next(char*& str);

    string m_sCmdLine;
    std::vector<CCmdLineArg>    m_args;
};


#endif // CRYINCLUDE_CRYSYSTEM_CMDLINE_H
