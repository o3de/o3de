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

// Description : Executes an ASCII batch file of console commands...


#ifndef CRYINCLUDE_CRYSYSTEM_CONSOLEBATCHFILE_H
#define CRYINCLUDE_CRYSYSTEM_CONSOLEBATCHFILE_H

#pragma once

struct IConsoleCmdArgs;
struct IConsole;

class CConsoleBatchFile
{
public:
    static void Init();
    static bool ExecuteConfigFile(const char* filename);

private:
    static void ExecuteFileCmdFunc(IConsoleCmdArgs* args);
    static IConsole*        m_pConsole;
};

#endif // CRYINCLUDE_CRYSYSTEM_CONSOLEBATCHFILE_H
