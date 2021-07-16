/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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
