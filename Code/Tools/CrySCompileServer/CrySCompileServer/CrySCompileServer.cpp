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

#include "Core/StdTypes.hpp"
#include "Core/Server/CrySimpleServer.hpp"
#include "Core/Server/CrySimpleHTTP.hpp"

#include <AzFramework/StringFunc/StringFunc.h>

#include <AzCore/PlatformDef.h>
#include <AzCore/base.h>
#include <AzCore/Jobs/JobContext.h>
#include <AzCore/Jobs/JobManager.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Utils/Utils.h>

#include <iostream>
#include <string>
#include <regex>

#if AZ_TRAIT_OS_PLATFORM_APPLE
// Needed for geteuid()
#include <sys/types.h>
#include <unistd.h>
#endif

namespace
{
    const int STD_TCP_PORT = 61453;
    const int DEFAULT_MAX_CONNECTIONS = 255;
}

//////////////////////////////////////////////////////////////////////////
class CConfigFile
{
public:
    CConfigFile() {}
    //////////////////////////////////////////////////////////////////////////
    void OnLoadConfigurationEntry(const std::string& strKey, const std::string& strValue, [[maybe_unused]] const std::string& strGroup)
    {
        if (azstricmp(strKey.c_str(), "MailError") == 0)
        {
            SEnviropment::Instance().m_FailEMail = strValue;
        }
        if (azstricmp(strKey.c_str(), "port") == 0)
        {
            SEnviropment::Instance().m_port = atoi(strValue.c_str());
        }
        if (azstricmp(strKey.c_str(), "MailInterval") == 0)
        {
            SEnviropment::Instance().m_MailInterval = atoi(strValue.c_str());
        }
        if (azstricmp(strKey.c_str(), "TempDir") == 0)
        {
            SEnviropment::Instance().m_TempPath = AddSlash(strValue);
        }
        if (azstricmp(strKey.c_str(), "MailServer") == 0)
        {
            SEnviropment::Instance().m_MailServer = strValue;
        }
        if (azstricmp(strKey.c_str(), "Caching") == 0)
        {
            SEnviropment::Instance().m_Caching = atoi(strValue.c_str()) != 0;
        }
        if (azstricmp(strKey.c_str(), "PrintErrors") == 0)
        {
            SEnviropment::Instance().m_PrintErrors = atoi(strValue.c_str()) != 0;
        }
        if (azstricmp(strKey.c_str(), "PrintWarnings") == 0)
        {
            SEnviropment::Instance().m_PrintWarnings = atoi(strValue.c_str()) != 0;
        }
        if (azstricmp(strKey.c_str(), "PrintCommands") == 0)
        {
            SEnviropment::Instance().m_PrintCommands = atoi(strValue.c_str()) != 0;
        }
        if (azstricmp(strKey.c_str(), "PrintListUpdates") == 0)
        {
            SEnviropment::Instance().m_PrintListUpdates = atoi(strValue.c_str()) != 0;
        }
        if (azstricmp(strKey.c_str(), "DedupeErrors") == 0)
        {
            SEnviropment::Instance().m_DedupeErrors = atoi(strValue.c_str()) != 0;
        }
        if (azstricmp(strKey.c_str(), "FallbackServer") == 0)
        {
            SEnviropment::Instance().m_FallbackServer = strValue;
        }
        if (azstricmp(strKey.c_str(), "FallbackTreshold") == 0)
        {
            SEnviropment::Instance().m_FallbackTreshold = atoi(strValue.c_str());
        }
        if (azstricmp(strKey.c_str(), "DumpShaders") == 0)
        {
            SEnviropment::Instance().m_DumpShaders = atoi(strValue.c_str()) != 0;
        }
        if (azstricmp(strKey.c_str(), "MaxConnections") == 0)
        {
            int maxConnections = atoi(strValue.c_str());
            if (maxConnections <= 0)
            {
                printf("Warning: MaxConnections value is invalid. Using default value of %d\n", DEFAULT_MAX_CONNECTIONS);
            }
            else
            {
                SEnviropment::Instance().m_MaxConnections = maxConnections;
            }
        }
        if (azstricmp(strKey.c_str(), "whitelist") == 0 || azstricmp(strKey.c_str(), "white_list") == 0)
        {
            std::regex ip4_address_regex("^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])(\\/([0-9]|[1-2][0-9]|3[0-2]))?$");
            AZStd::vector<AZStd::string> addresses;
            AzFramework::StringFunc::Tokenize(strValue.c_str(), addresses, ',');
            for (const auto& address : addresses)
            {
                if (std::regex_match(address.c_str(), ip4_address_regex))
                {
                    SEnviropment::Instance().m_WhitelistAddresses.push_back(address);
                }
                else
                {
                    printf("Warning: invalid IP address in the whitelist field: %s", address.c_str());
                }
            }
        }
        if (azstricmp(strKey.c_str(), "AllowElevatedPermissions") == 0)
        {
            int runAsRoot = atoi(strValue.c_str());
            SEnviropment::Instance().m_RunAsRoot = (runAsRoot == 1);
        }

#if defined(TOOLS_SUPPORT_XENIA)
#include AZ_RESTRICTED_FILE_EXPLICIT(CrySCompileServer_cpp, xenia)
#endif
#if defined(TOOLS_SUPPORT_JASPER)
#include AZ_RESTRICTED_FILE_EXPLICIT(CrySCompileServer_cpp, jasper)
#endif
#if defined(TOOLS_SUPPORT_PROVO)
#include AZ_RESTRICTED_FILE_EXPLICIT(CrySCompileServer_cpp, provo)
#endif
#if defined(TOOLS_SUPPORT_SALEM)
#include AZ_RESTRICTED_FILE_EXPLICIT(CrySCompileServer_cpp, salem)
#endif
    }

    //////////////////////////////////////////////////////////////////////////
    bool ParseConfig(const char* filename)
    {
        FILE* file = nullptr;
        azfopen(&file, filename, "rb");
        if (!file)
        {
            std::cout << "Config file not found" << std::endl;
            return false;
        }

        fseek(file, 0, SEEK_END);
        int nLen = ftell(file);
        fseek(file, 0, SEEK_SET);

        char* sAllText = new char [nLen + 16];

        fread(sAllText, 1, nLen, file);

        sAllText[nLen] = '\0';
        sAllText[nLen + 1] = '\0';

        std::string strGroup;           // current group e.g. "[General]"

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


            std::string strLine = s;

            // detect groups e.g. "[General]"   should set strGroup="General"
            {
                std::string strTrimmedLine(RemoveWhiteSpaces(strLine));
                size_t size = strTrimmedLine.size();

                if (size >= 3)
                {
                    if (strTrimmedLine[0] == '[' && strTrimmedLine[size - 1] == ']')       // currently no comments are allowed to be behind groups
                    {
                        strGroup = &strTrimmedLine[1];
                        strGroup.resize(size - 2);                                  // remove [ and ]
                        continue;                                                                                                   // next line
                    }
                }
            }

            // skip comments
            if (0 < strLine.find("--"))
            {
                // extract key
                std::string::size_type posEq(strLine.find("=", 0));
                if (std::string::npos != posEq)
                {
                    std::string stemp(strLine, 0, posEq);
                    std::string strKey(RemoveWhiteSpaces(stemp));

                    //              if (!strKey.empty())
                    {
                        // extract value
                        std::string::size_type posValueStart(strLine.find("\"", posEq + 1) + 1);
                        // std::string::size_type posValueEnd( strLine.find( "\"", posValueStart ) );
                        std::string::size_type posValueEnd(strLine.rfind('\"'));

                        std::string strValue;

                        if (std::string::npos != posValueStart && std::string::npos != posValueEnd)
                        {
                            strValue = std::string(strLine, posValueStart, posValueEnd - posValueStart);
                        }
                        else
                        {
                            std::string strTmp(strLine, posEq + 1, strLine.size() - (posEq + 1));
                            strValue = RemoveWhiteSpaces(strTmp);
                        }
                        OnLoadConfigurationEntry(strKey, strValue, strGroup);
                    }
                }
            } //--
        }
        delete []sAllText;
        fclose(file);

        return true;
    }
    std::string RemoveWhiteSpaces(std::string& str)
    {
        std::string::size_type pos1 = str.find_first_not_of(' ');
        std::string::size_type pos2 = str.find_last_not_of(' ');
        str = str.substr(pos1 == std::string::npos ? 0 : pos1, pos2 == std::string::npos ? str.length() - 1 : pos2 - pos1 + 1);
        return str;
    }
    std::string AddSlash(const std::string& str)
    {
        if (!str.empty() && 
            (str[str.size() - 1] != '\\') && 
            (str[str.size() - 1] != '/'))
        {
            return str + "/";
        }
        return str;
    }
};

namespace
{
    AZ::JobManager* jobManager;
    AZ::JobContext* globalJobContext;

#if defined(AZ_PLATFORM_WINDOWS)
    BOOL ControlHandler([[maybe_unused]] DWORD controlType)
    {
        AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Destroy();
        AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
        return FALSE;
    }
#endif
}

void InitDefaults()
{
    SEnviropment::Instance().m_port                             = STD_TCP_PORT;
    SEnviropment::Instance().m_MaxConnections                   = DEFAULT_MAX_CONNECTIONS;
    SEnviropment::Instance().m_FailEMail                    = "";
    SEnviropment::Instance().m_MailInterval             = 10;
    SEnviropment::Instance().m_MailServer               = "example.com";
    SEnviropment::Instance().m_Caching                      =   true;
    SEnviropment::Instance().m_PrintErrors              = true;
    SEnviropment::Instance().m_PrintWarnings            = false;
    SEnviropment::Instance().m_PrintCommands            = false;
    SEnviropment::Instance().m_DedupeErrors             = true;
    SEnviropment::Instance().m_PrintListUpdates     = true;
    SEnviropment::Instance().m_FallbackTreshold     =   16;
    SEnviropment::Instance().m_FallbackServer           =   "";
    SEnviropment::Instance().m_WhitelistAddresses.push_back("127.0.0.1");
    SEnviropment::Instance().m_RunAsRoot = false;
    SEnviropment::Instance().InitializePlatformAttributes();
}

bool ReadConfigFile()
{
    char executableDir[AZ_MAX_PATH_LEN];
    if (AZ::Utils::GetExecutableDirectory(executableDir, AZ_MAX_PATH_LEN) == AZ::Utils::ExecutablePathResult::Success)
    {
        auto configFilename = AZ::IO::Path(executableDir).Append("config.ini");
        CConfigFile config;
        config.ParseConfig(configFilename.c_str());
        return true;
    }
    else
    {
        printf("error: failed to get executable directory.\n");
        return false;
    }
}

void RunServer(bool isRunningAsRoot)
{
    if (isRunningAsRoot)
    {
        printf("\nWARNING: Attempting to run the CrySCompileServer as a user that has admininstrator permissions. This is a security risk and not recommended. Please run the service with a user account that does not have administrator permissions.\n\n");
    }

    if (!isRunningAsRoot || SEnviropment::Instance().m_RunAsRoot)
    {
        CCrySimpleHTTP HTTP;
        CCrySimpleServer();
    }
    else
    {
        printf("If you need to run CrySCompileServer with administrator permisions you can create/edit the config.ini file in the same directory as this executable and add the following line to it:\n\tAllowElevatedPermissions=1\n");
    }
}

int main(int argc, [[maybe_unused]] char* argv[])
{
    if (argc != 1)
    {
        printf("usage: run without arguments\n");
        return 0;
    }

    bool isRunningAsRoot = false;

#if defined(AZ_PLATFORM_WINDOWS)
    // Check to see if we are running as root...
    SID_IDENTIFIER_AUTHORITY ntAuthority = { SECURITY_NT_AUTHORITY };
    PSID administratorsGroup;
    BOOL sidAllocated = AllocateAndInitializeSid(
            &ntAuthority,
            2,
            SECURITY_BUILTIN_DOMAIN_RID,
            DOMAIN_ALIAS_RID_ADMINS,
            0, 0, 0, 0, 0, 0,
            &administratorsGroup);

    if(sidAllocated)
    {
        BOOL isRoot = FALSE;
        if (!CheckTokenMembership( NULL, administratorsGroup, &isRoot))
        {
            isRoot = FALSE;
        }
        FreeSid(administratorsGroup);

        isRunningAsRoot = (isRoot == TRUE);
    }

#if defined(_DEBUG)
    int tmpFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
    tmpFlag |= _CRTDBG_LEAK_CHECK_DF;
    //  tmpFlag &= ~_CRTDBG_CHECK_CRT_DF;
    _CrtSetDbgFlag(tmpFlag);
#endif
    AZ_Verify(SetConsoleCtrlHandler(ControlHandler, TRUE), "Unable to setup windows console control handler");
#else
    // if either the effective user id or effective group id is root, then we
    // are running as root
    isRunningAsRoot = (geteuid() == 0 || getegid() == 0);
#endif

    AZ::AllocatorInstance<AZ::SystemAllocator>::Create();
    AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Create();

    AZ::JobManagerDesc jobManagerDescription;

    int workers = AZStd::GetMin(AZStd::thread::hardware_concurrency(), static_cast<unsigned int>(8));
    for (int idx = 0; idx < workers; ++idx)
    {
        jobManagerDescription.m_workerThreads.push_back(AZ::JobManagerThreadDesc());
    }

    jobManager = aznew AZ::JobManager(jobManagerDescription);
    globalJobContext = aznew AZ::JobContext(*jobManager);
    AZ::JobContext::SetGlobalContext(globalJobContext);
    
    SEnviropment::Create();

    InitDefaults();
    
    if (ReadConfigFile())
    {
        RunServer(isRunningAsRoot);
    }
    
    SEnviropment::Destroy();

    AZ::JobContext::SetGlobalContext(nullptr);
    delete globalJobContext;
    delete jobManager;

    AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Destroy();
    AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();

    return 0;
}

