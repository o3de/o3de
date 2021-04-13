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

#include "CrySimpleJob.hpp"
#include "CrySimpleFileGuard.hpp"
#include "CrySimpleServer.hpp"

#include <Core/StdTypes.hpp>
#include <Core/Error.hpp>
#include <Core/STLHelper.hpp>
#include <Core/Common.h>
#include <Core/WindowsAPIImplementation.h>
#include <tinyxml/tinyxml.h>

#include <thread>
#include <sstream>
#include <fstream>


AZStd::atomic_long CCrySimpleJob::m_GlobalRequestNumber = {0};

CCrySimpleJob::CCrySimpleJob(uint32_t requestIP)
    : m_State(ECSJS_NONE)
    , m_RequestIP(requestIP)
{
    ++m_GlobalRequestNumber;
}

CCrySimpleJob::~CCrySimpleJob()
{
}


bool CCrySimpleJob::ExecuteCommand(const std::string& rCmd, std::string& outError)
{
    const bool showStdOuput = false; // For Debug: Set to true if you want the compiler's standard ouput printed out as well.
    const bool showStdErrorOuput = SEnviropment::Instance().m_PrintWarnings;

#ifdef _MSC_VER
    bool    Ret = false;
    DWORD   ExitCode    =   0;

    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInfo;
    memset(&StartupInfo, 0, sizeof(StartupInfo));
    memset(&ProcessInfo, 0, sizeof(ProcessInfo));
    StartupInfo.cb = sizeof(StartupInfo);


    std::string Path = "";
    std::string::size_type Pt = rCmd.find_first_of(' ');
    if (Pt != std::string::npos)
    {
        std::string First   =   std::string(rCmd.c_str(), Pt);
        std::string::size_type Pt2 = First.find_last_of('/');
        if (Pt2 != std::string::npos)
        {
            Path    =   std::string(First.c_str(), Pt2);
        }
        else
        {
            Pt  =   std::string::npos;
        }
    }

    HANDLE hReadErr, hWriteErr;

    {
        CreatePipe(&hReadErr, &hWriteErr, NULL, 0);
        SetHandleInformation(hWriteErr, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);

        StartupInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        StartupInfo.hStdOutput = (showStdOuput) ? GetStdHandle(STD_OUTPUT_HANDLE) : NULL;
        StartupInfo.hStdError = hWriteErr;
        StartupInfo.dwFlags |= STARTF_USESTDHANDLES;

        BOOL processCreated = CreateProcess(NULL, (char*)rCmd.c_str(), 0, 0, TRUE, CREATE_DEFAULT_ERROR_MODE, 0, Pt != std::string::npos ? Path.c_str() : 0, &StartupInfo, &ProcessInfo) != false;

        if (!processCreated)
        {
            outError = "Couldn't create process - missing compiler for cmd?: '" + rCmd + "'";
        }
        else
        {
            std::string error;

            DWORD waitResult = 0;
            HANDLE waitHandles[] = { ProcessInfo.hProcess, hReadErr };
            while (true)
            {
                //waitResult = WaitForMultipleObjects(sizeof(waitHandles) / sizeof(waitHandles[0]), waitHandles, FALSE, 1000 );
                waitResult = WaitForSingleObject(ProcessInfo.hProcess, 1000);
                if (waitResult == WAIT_FAILED)
                {
                    break;
                }

                DWORD bytesRead, bytesAvailable;
                while (PeekNamedPipe(hReadErr, NULL, 0, NULL, &bytesAvailable, NULL) && bytesAvailable)
                {
                    char buff[4096];
                    ReadFile(hReadErr, buff, sizeof(buff) - 1, &bytesRead, 0);
                    buff[bytesRead] = '\0';
                    error += buff;
                }
                CSTLHelper::Trim(error," \t\r\n");

                //if (waitResult == WAIT_OBJECT_0 || waitResult == WAIT_TIMEOUT)
                //break;

                if (waitResult == WAIT_OBJECT_0)
                {
                    break;
                }
            }

            //if (waitResult != WAIT_TIMEOUT)
            {
                GetExitCodeProcess(ProcessInfo.hProcess, &ExitCode);
                if (ExitCode)
                {
                    Ret = false;
                    outError = error;
                }
                else
                {
                    if (showStdErrorOuput && !error.empty())
                    {
                        AZ_Printf(0, "\n%s\n", error.c_str());
                    }
                    Ret = true;
                }
            }
            /*
            else
            {
                Ret = false;
                outError = std::string("Timed out executing compiler: ") + rCmd;
                TerminateProcess(ProcessInfo.hProcess, 1);
            }
            */

            CloseHandle(ProcessInfo.hProcess);
            CloseHandle(ProcessInfo.hThread);
        }

        CloseHandle(hReadErr);
        if (hWriteErr)
        {
            CloseHandle(hWriteErr);
        }
    }

    return Ret;
#endif

#if defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_MAC)
    std::thread::id threadId = std::this_thread::get_id();
    std::stringstream threadIdStream;
    threadIdStream << threadId;
    
    // Multiple threads could execute a command, therefore the temporary file has to be unique per thread.
    AZ::IO::Path errorTempFilePath = SEnviropment::Instance().m_TempPath / AZStd::string::format("stderr_%s.log", threadIdStream.str().c_str());
    std::string stdErrorTempFilename{ errorTempFilePath.c_str(), errorTempFilePath.Native().size() };

    CCrySimpleFileGuard FGTmpOutput(stdErrorTempFilename); // Delete file at the end of this function
    
    std::string systemCmd = rCmd;
    if(!showStdOuput)
    {
        // Standard output redirected to null to disable it
        systemCmd += " > /dev/null";
    }
    // Standard error ouput redirected to the temporary file
    systemCmd += " 2> \"" + stdErrorTempFilename + "\"";
    
    int ret = system(systemCmd.c_str());
    
    // Obtain standard error output
    std::ifstream fileStream(stdErrorTempFilename.c_str());
    std::stringstream stdErrorStream;
    stdErrorStream << fileStream.rdbuf();
    std::string stdErrorString = stdErrorStream.str();
    CSTLHelper::Trim(stdErrorString," \t\r\n");
    
    if (ret != 0)
    {
        outError = stdErrorString;
        return false;
    }
    else
    {
        if (showStdErrorOuput && !stdErrorString.empty())
        {
            AZ_Printf(0, "\n%s\n", stdErrorString.c_str());
        }
        return true;
    }
#endif
}

