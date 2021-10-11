/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Executes an ASCII batch file of console commands...


#include "CrySystem_precompiled.h"
#include "ConsoleBatchFile.h"
#include "IConsole.h"
#include "ISystem.h"
#include "XConsole.h"
#include <CryPath.h>
#include <stdio.h>
#include "System.h"
#include <AzCore/IO/FileIO.h>

IConsole* CConsoleBatchFile::m_pConsole = NULL;

void CConsoleBatchFile::Init()
{
    m_pConsole = gEnv->pConsole;
    REGISTER_COMMAND("exec", (ConsoleCommandFunc)ExecuteFileCmdFunc, 0, "executes a batch file of console commands");
}

//////////////////////////////////////////////////////////////////////////
void CConsoleBatchFile::ExecuteFileCmdFunc(IConsoleCmdArgs* args)
{
    if (!m_pConsole)
    {
        Init();
    }

    if (!args->GetArg(1))
    {
        return;
    }

    ExecuteConfigFile(args->GetArg(1));
}

//////////////////////////////////////////////////////////////////////////
bool CConsoleBatchFile::ExecuteConfigFile(const char* sFilename)
{
    if (!sFilename)
    {
        return false;
    }

    if (!m_pConsole)
    {
        Init();
    }

    AZStd::string filename;

    if (sFilename[0] != '@') // console config files are actually by default in @products@ instead of @products@
    {
        // However, if we've passed in a relative or absolute path that matches an existing file name,
        // don't change it.  Only change it to "@products@/filename" and strip off any relative paths
        // if the given pattern *didn't* match a file.
        if (AZ::IO::FileIOBase::GetDirectInstance()->Exists(sFilename))
        {
            filename = sFilename;
        }
        else
        {
            filename = PathUtil::Make("@products@", PathUtil::GetFile(sFilename));
        }
    }
    else
    {
        filename = sFilename;
    }

    if (strlen(PathUtil::GetExt(filename.c_str())) == 0)
    {
        filename = PathUtil::ReplaceExtension(filename, "cfg");
    }

    //////////////////////////////////////////////////////////////////////////
    CCryFile file;

    {
        [[maybe_unused]] const char* szLog = "Executing console batch file (try game,config,root):";
        AZStd::string filenameLog;
        AZStd::string sfn = PathUtil::GetFile(filename);

        if (file.Open(filename.c_str(), "rb"))
        {
            filenameLog = AZStd::string("game/") + sfn;
        }
        else if (file.Open((AZStd::string("config/") + sfn).c_str(), "rb"))
        {
            filenameLog = AZStd::string("game/config/") + sfn;
        }
        else if (file.Open((AZStd::string("./") + sfn).c_str(), "rb"))
        {
            filenameLog = AZStd::string("./") + sfn;
        }
        else
        {
            CryLog("%s \"%s\" not found!", szLog, filename.c_str());
            return false;
        }

        CryLog("%s \"%s\" found in %s ...", szLog, PathUtil::GetFile(filenameLog.c_str()), PathUtil::GetPath(filenameLog).c_str());
    }

    size_t nLen = file.GetLength();
    char* sAllText = new char [nLen + 16];
    file.ReadRaw(sAllText, nLen);
    sAllText[nLen] = '\0';
    sAllText[nLen + 1] = '\0';

    /*
        This can't work properly as ShowConsole() can be called during the execution of the scripts,
        which means bConsoleStatus is outdated and must not be set at the end of the function

        bool bConsoleStatus = ((CXConsole*)m_pConsole)->GetStatus();
        ((CXConsole*)m_pConsole)->SetStatus(false);
    */

    char* strLast = sAllText + nLen;
    char* str = sAllText;
    while (str < strLast)
    {
        char* s = str;
        while (str < strLast && *str != '\n' && *str != '\r')
        {
            str++;
        }
        *str = '\0';
        str++;
        while (str < strLast && (*str == '\n' || *str == '\r'))
        {
            str++;
        }

        AZStd::string strLine = s;


        //trim all whitespace characters at the beginning and the end of the current line and store its size
        AZ::StringFunc::TrimWhiteSpace(strLine, true, true);
        size_t strLineSize = strLine.size();

        //skip comments, comments start with ";" or "--" but may have preceding whitespace characters
        if (strLineSize > 0)
        {
            if (strLine[0] == ';')
            {
                continue;
            }
            else if (strLine.find("--") == 0)
            {
                continue;
            }
        }
        //skip empty lines
        else
        {
            continue;
        }

        {
            m_pConsole->ExecuteString(strLine.c_str());
        }
    }
    // See above
    //  ((CXConsole*)m_pConsole)->SetStatus(bConsoleStatus);

    delete []sAllText;
    return true;
}
