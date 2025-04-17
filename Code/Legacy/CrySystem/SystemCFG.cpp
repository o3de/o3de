/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : handles system cfg


#include "CrySystem_precompiled.h"
#include "System.h"
#include <time.h>
#include "XConsole.h"
#include "CryFile.h"
#include "CryPath.h"

#include <AzCore/Console/IConsole.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/StringFunc/StringFunc.h>

#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define SYSTEMCFG_CPP_SECTION_1 1
#define SYSTEMCFG_CPP_SECTION_2 2
#define SYSTEMCFG_CPP_SECTION_3 3
#endif

#if defined(LINUX) || defined(APPLE)
#include "ILog.h"

#endif

#ifndef EXE_VERSION_INFO_0
#define EXE_VERSION_INFO_0 1
#endif

#ifndef EXE_VERSION_INFO_1
#define EXE_VERSION_INFO_1 0
#endif

#ifndef EXE_VERSION_INFO_2
#define EXE_VERSION_INFO_2 0
#endif

#ifndef EXE_VERSION_INFO_3
#define EXE_VERSION_INFO_3 1
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION SYSTEMCFG_CPP_SECTION_1
#include AZ_RESTRICTED_FILE(SystemCFG_cpp)
#endif

//////////////////////////////////////////////////////////////////////////
const SFileVersion& CSystem::GetFileVersion()
{
    return m_fileVersion;
}

//////////////////////////////////////////////////////////////////////////
const SFileVersion& CSystem::GetProductVersion()
{
    return m_productVersion;
}

//////////////////////////////////////////////////////////////////////////
const SFileVersion& CSystem::GetBuildVersion()
{
    return m_buildVersion;
}

//////////////////////////////////////////////////////////////////////////
#ifndef _RELEASE
void CSystem::SystemVersionChanged(ICVar* pCVar)
{
    if (CSystem* pThis = static_cast<CSystem*>(gEnv->pSystem))
    {
        pThis->SetVersionInfo(pCVar->GetString());
    }
}

void CSystem::SetVersionInfo(const char* const szVersion)
{
    m_fileVersion.Set(szVersion);
    m_productVersion.Set(szVersion);
    m_buildVersion.Set(szVersion);
    CryLog("SetVersionInfo '%s'", szVersion);
    CryLog("FileVersion: %d.%d.%d.%d", m_fileVersion.v[3], m_fileVersion.v[2], m_fileVersion.v[1], m_fileVersion.v[0]);
    CryLog("ProductVersion: %d.%d.%d.%d", m_productVersion.v[3], m_productVersion.v[2], m_productVersion.v[1], m_productVersion.v[0]);
    CryLog("BuildVersion: %d.%d.%d.%d", m_buildVersion.v[3], m_buildVersion.v[2], m_buildVersion.v[1], m_buildVersion.v[0]);
}
#endif // #ifndef _RELEASE

//////////////////////////////////////////////////////////////////////////
void CSystem::QueryVersionInfo()
{
#ifndef WIN32
    //do we need some other values here?
    m_fileVersion.v[0] = m_productVersion.v[0] = EXE_VERSION_INFO_3;
    m_fileVersion.v[1] = m_productVersion.v[1] = EXE_VERSION_INFO_2;
    m_fileVersion.v[2] = m_productVersion.v[2] = EXE_VERSION_INFO_1;
    m_fileVersion.v[3] = m_productVersion.v[3] = EXE_VERSION_INFO_0;
    m_buildVersion = m_fileVersion;
#else  //WIN32
    char moduleName[_MAX_PATH];
    DWORD dwHandle;
    UINT len;

    char ver[1024 * 8];

    AZ::Utils::GetExecutablePath(moduleName, _MAX_PATH);  //retrieves the PATH for the current module

#ifdef AZ_MONOLITHIC_BUILD
    AZ::Utils::GetExecutablePath(moduleName, _MAX_PATH);  //retrieves the PATH for the current module
#else // AZ_MONOLITHIC_BUILD
    azstrcpy(moduleName, AZ_ARRAY_SIZE(moduleName), "CrySystem.dll"); // we want to version from the system dll
#endif // AZ_MONOLITHIC_BUILD

    AZStd::wstring moduleNameW;
    AZStd::to_wstring(moduleNameW, moduleName);
    int verSize = GetFileVersionInfoSizeW(moduleNameW.c_str(), &dwHandle);
    if (verSize > 0)
    {
        GetFileVersionInfoW(moduleNameW.c_str(), dwHandle, 1024 * 8, ver);
        VS_FIXEDFILEINFO* vinfo;
        VerQueryValueW(ver, L"\\", (void**)&vinfo, &len);

        const uint32 verIndices[4] = {0, 1, 2, 3};
        m_fileVersion.v[verIndices[0]] = m_productVersion.v[verIndices[0]] = vinfo->dwFileVersionLS & 0xFFFF;
        m_fileVersion.v[verIndices[1]] = m_productVersion.v[verIndices[1]] = vinfo->dwFileVersionLS >> 16;
        m_fileVersion.v[verIndices[2]] = m_productVersion.v[verIndices[2]] = vinfo->dwFileVersionMS & 0xFFFF;
        m_fileVersion.v[verIndices[3]] = m_productVersion.v[verIndices[3]] = vinfo->dwFileVersionMS >> 16;
        m_buildVersion = m_fileVersion;

        struct LANGANDCODEPAGE
        {
            WORD wLanguage;
            WORD wCodePage;
        }* lpTranslate;

        UINT count = 0;
        wchar_t path[256];
        char* version = NULL;

        VerQueryValueW(ver, L"\\VarFileInfo\\Translation", (LPVOID*)&lpTranslate, &count);
        if (lpTranslate != NULL)
        {
            azsnwprintf(path, sizeof(path), L"\\StringFileInfo\\%04x%04x\\InternalName", lpTranslate[0].wLanguage, lpTranslate[0].wCodePage);
            VerQueryValueW(ver, path, (LPVOID*)&version, &count);
            if (version)
            {
                m_buildVersion.Set(version);
            }
        }
    }
#endif //WIN32
}

//////////////////////////////////////////////////////////////////////////
void CSystem::LogVersion()
{
    // Get time.
    time_t ltime;
    time(&ltime);

#ifdef AZ_COMPILER_MSVC
    tm today;
    localtime_s(&today, &ltime);
    char s[1024];
    strftime(s, 128, "%d %b %y (%H %M %S)", &today);
#else
    char s[1024];
    auto today = localtime(&ltime);
    strftime(s, 128, "%d %b %y (%H %M %S)", today);
#endif

    [[maybe_unused]] const SFileVersion& ver = GetFileVersion();

    CryLogAlways("BackupNameAttachment=\" Build(%d) %s\"  -- used by backup system\n", ver.v[0], s);          // read by CreateBackupFile()

    // Use strftime to build a customized time string.
#ifdef AZ_COMPILER_MSVC
    strftime(s, 128, "Log Started at %c", &today);
#else
    strftime(s, 128, "Log Started at %c", today);
#endif
    CryLogAlways("%s", s);

    CryLogAlways("Built on " __DATE__ " " __TIME__);

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION SYSTEMCFG_CPP_SECTION_2
#include AZ_RESTRICTED_FILE(SystemCFG_cpp)
#elif defined(ANDROID)
    CryLogAlways("Running 32 bit Android version API VER:%d", __ANDROID_API__);
#elif defined(IOS)
    CryLogAlways("Running 64 bit iOS version");
#elif defined(WIN64)
    CryLogAlways("Running 64 bit Windows version");
#elif defined(WIN32)
    CryLogAlways("Running 32 bit Windows version");
#elif defined(LINUX64)
    CryLogAlways("Running 64 bit Linux version");
#elif defined(LINUX32)
    CryLogAlways("Running 32 bit Linux version");
#elif defined(MAC)
    CryLogAlways("Running 64 bit Mac version");
#endif
#if AZ_LEGACY_CRYSYSTEM_TRAIT_SYSTEMCFG_MODULENAME
    AZ::Utils::GetExecutablePath(s, sizeof(s));

    // Log EXE filename only if possible (not full EXE path which could contain sensitive info)
    AZStd::string exeName;
    if (AzFramework::StringFunc::Path::GetFullFileName(s, exeName)) {
        CryLogAlways("Executable: %s", exeName.c_str());
    }
#endif

    CryLogAlways("FileVersion: %d.%d.%d.%d", m_fileVersion.v[3], m_fileVersion.v[2], m_fileVersion.v[1], m_fileVersion.v[0]);
#if defined(LY_BUILD)
    CryLogAlways("ProductVersion: %d.%d.%d.%d - Build %d", m_productVersion.v[3], m_productVersion.v[2], m_productVersion.v[1], m_productVersion.v[0], LY_BUILD);
#else // defined(LY_BUILD)
    CryLogAlways("ProductVersion: %d.%d.%d.%d", m_productVersion.v[3], m_productVersion.v[2], m_productVersion.v[1], m_productVersion.v[0]);
#endif // defined(LY_BUILD)



#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION SYSTEMCFG_CPP_SECTION_3
#include AZ_RESTRICTED_FILE(SystemCFG_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(_MSC_VER)
    CryLogAlways("Using Microsoft (tm) C++ Standard Library implementation\n");
#elif defined(__clang__)
    CryLogAlways("Using CLANG C++ Standard Library implementation\n");
#elif defined(__GNUC__)
    CryLogAlways("Using GNU C++ Standard Library implementation\n");
#else
#error "Please specify C++ STL library"
#endif
}

//////////////////////////////////////////////////////////////////////////
void CSystem::LogBuildInfo()
{
    [[maybe_unused]] auto projectName = AZ::Utils::GetProjectName();
    CryLogAlways("GameName: %s", projectName.c_str());
    CryLogAlways("BuildTime: " __DATE__ " " __TIME__);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::SaveConfiguration()
{
}

//////////////////////////////////////////////////////////////////////////
static bool ParseSystemConfig(const AZStd::string& strSysConfigFilePath, ILoadConfigurationEntrySink* pSink, bool warnIfMissing)
{
    assert(pSink);
    AZStd::string filename = strSysConfigFilePath;
    if (strlen(PathUtil::GetExt(filename.c_str())) == 0)
    {
        filename = PathUtil::ReplaceExtension(filename, "cfg");
    }

    CCryFile file;
    AZStd::string filenameLog;
    {
        if (filename[0] == '@')
        {
            // this is used when theres a very specific file to read, like @user@/game.cfg which is read
            // IN ADDITION to the one in the game folder, and afterwards to override values in it.
            // if the file is missing and its already prefixed with an alias, there is no need to look any further.
            if (!(file.Open(filename.c_str(), "rb")))
            {
                if (warnIfMissing)
                {
                    CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "Config file %s not found!", filename.c_str());
                }
                return false;
            }
        }
        else
        {
            // otherwise, if the file isn't prefixed with an alias, then its likely one of the convenience mappings
            // to either root or assets/config.  this is done so that code can just request a simple file name and get its data
            if (
                !(file.Open(filename.c_str(), "rb")) &&
                !(file.Open((AZStd::string("@products@/") + filename).c_str(), "rb")) &&
                !(file.Open((AZStd::string("@products@/") + filename).c_str(), "rb")) &&
                !(file.Open((AZStd::string("@products@/config/") + filename).c_str(), "rb")) &&
                !(file.Open((AZStd::string("@products@/config/spec/") + filename).c_str(), "rb"))
                )
            {
                if (warnIfMissing)
                {
                    CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "Config file %s not found!", filename.c_str());
                }
                return false;
            }
        }

        AZ::IO::FixedMaxPath resolvedFilePath;
        AZ::IO::FileIOBase::GetInstance()->ResolvePath(resolvedFilePath, file.GetFilename());
        filenameLog = resolvedFilePath.String();
    }

    INDENT_LOG_DURING_SCOPE();

    int nLen = static_cast<int>(file.GetLength());
    if (nLen == 0)
    {
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "Couldn't get length for Config file %s", filename.c_str());
        return false;
    }
    char* sAllText = new char [nLen + 16];
    if (file.ReadRaw(sAllText, nLen) < (size_t)nLen)
    {
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "Couldn't read Config file %s", filename.c_str());
        return false;
    }
    sAllText[nLen] = '\0';
    sAllText[nLen + 1] = '\0';

    AZStd::string strGroup;            // current group e.g. "[General]"

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
        AZ::StringFunc::TrimWhiteSpace(strLine, true, true);

        // detect groups e.g. "[General]"   should set strGroup="General"
        {
            size_t size = strLine.size();

            if (size >= 3)
            {
                if (strLine[0] == '[' && strLine[size - 1] == ']')  // currently no comments are allowed to be behind groups
                {
                    strGroup = &strLine[1];
                    strGroup.resize(size - 2);                      // remove [ and ]
                    continue;                                       // next line
                }
            }
        }

        //trim all whitespace characters at the beginning and the end of the current line and store its size
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

        //if line contains a '=' try to read and assign console variable
        AZStd::string::size_type posEq(strLine.find("=", 0));
        if (AZStd::string::npos != posEq)
        {
            AZStd::string stemp;
            AZStd::string strKey(strLine, 0, posEq);
            AZ::StringFunc::TrimWhiteSpace(strKey, true, true);

            {
                // extract value
                AZStd::string::size_type posValueStart(strLine.find("\"", posEq + 1) + 1);
                AZStd::string::size_type posValueEnd(strLine.rfind('\"'));

                AZStd::string strValue;

                if (AZStd::string::npos != posValueStart && AZStd::string::npos != posValueEnd)
                {
                    strValue = AZStd::string(strLine, posValueStart, posValueEnd - posValueStart);
                }
                else
                {
                    strValue = AZStd::string(strLine, posEq + 1, strLine.size() - (posEq + 1));
                    AZ::StringFunc::TrimWhiteSpace(strValue, true, true);
                }

                {
                    // replace '\\\\' with '\\' and '\\\"' with '\"'
                    AZ::StringFunc::Replace(strValue, "\\\\", "\\");
                    AZ::StringFunc::Replace(strValue, "\\\"", "\"");

                    pSink->OnLoadConfigurationEntry(strKey.c_str(), strValue.c_str(), strGroup.c_str());
                }
            }
        }
        else
        {
            gEnv->pLog->LogWithType(ILog::eWarning, "%s -> invalid configuration line: %s", filename.c_str(), strLine.c_str());
        }
    }

    delete []sAllText;

    CryLog("Loading Config file %s (%s)", filename.c_str(), filenameLog.c_str());

    pSink->OnLoadConfigurationEntry_End();

    return true;
}


//////////////////////////////////////////////////////////////////////////
void CSystem::OnLoadConfigurationEntry(const char* szKey, const char* szValue, [[maybe_unused]] const char* szGroup)
{
    bool azConsoleProcessed = false;
    auto console = AZ::Interface<AZ::IConsole>::Get();
    if (console)
    {
        AZStd::string command(AZStd::string::format("%s %s", szKey, szValue));

        azConsoleProcessed = static_cast<bool>(console->PerformCommand(command.c_str()));
    }

    if (!azConsoleProcessed)
    {
        if (!gEnv->pConsole)
        {
            return;
        }

        if (*szKey != 0)
        {
            gEnv->pConsole->LoadConfigVar(szKey, szValue);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CSystem::LoadConfiguration(const char* sFilename, ILoadConfigurationEntrySink* pSink, bool warnIfMissing)
{
    if (sFilename && strlen(sFilename) > 0)
    {
        if (!pSink)
        {
            pSink = this;
        }

        ParseSystemConfig(sFilename, pSink, warnIfMissing);
    }
}

