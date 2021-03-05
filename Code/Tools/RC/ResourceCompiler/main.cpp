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

#include <AzCore/PlatformDef.h>

#if defined(AZ_PLATFORM_WINDOWS)
#include "MathHelpers.h"
#endif //AZ_PLATFORM_WINDOWS

#include <QApplication>
#include <QSettings>
#include <QDir>

#include <AzCore/Math/Sfmt.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzFramework/CommandLine/CommandLine.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <ResourceCompiler.h>
#include <IResourceCompilerHelper.h>
#include <CmdLine.h>
#include <ParseEngineConfig.h>
#include <CryLibrary.h>
#include <ZipEncryptor.h>

#if defined(AZ_PLATFORM_WINDOWS)
#include <CrashHandler_Windows.h>

CrashHandler& GetCrashHandler()
{
    static CrashHandler s_crashHandler;
    return s_crashHandler;
}
#endif

namespace
{
    volatile bool g_gotCTRLBreakSignalFromOS = false;
#if defined(AZ_PLATFORM_WINDOWS)
    BOOL WINAPI CtrlHandlerRoutine([[maybe_unused]] DWORD dwCtrlType)
    {
        // we got this.
        RCLogError("CTRL-BREAK was pressed!");
        g_gotCTRLBreakSignalFromOS = true;
        return TRUE;
    }
#else
    void CtrlHandlerRoutine(int signo)
    {
        if (signo == SIGINT)
        {
            RCLogError("CTRL-BREAK was pressed!");
            g_gotCTRLBreakSignalFromOS = true;
        }
    }
#endif
}


std::unique_ptr<QCoreApplication> CreateQApplication(int& argc, char** argv)
{
    //we are not going to start a mesg loop. so exec will not be called on the qapp

    // special circumsance  - if 'userDialog' is present on the command line, we need an interactive app:
    AzFramework::CommandLine cmdLine;
    cmdLine.Parse(argc, argv);
    bool userDialog = cmdLine.HasSwitch("userdialog") &&
        ((cmdLine.GetNumSwitchValues("userdialog") == 0) || (cmdLine.GetSwitchValue("userdialog", 0) == "1"));

    std::unique_ptr<QCoreApplication> qApplication = userDialog ? std::make_unique<QApplication>(argc, argv) : std::make_unique<QCoreApplication>(argc, argv);
    return qApplication;
}

#if 0
static void EnableCrtMemoryChecks()
{
    uint32 tmp = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
    tmp &= ~_CRTDBG_CHECK_ALWAYS_DF;
    tmp |= _CRTDBG_ALLOC_MEM_DF;
    tmp |= _CRTDBG_CHECK_CRT_DF;
    tmp |= _CRTDBG_DELAY_FREE_MEM_DF;
    tmp |= _CRTDBG_LEAK_CHECK_DF;    // used on exit
    // Set desired check frequency
    const uint32 eachX = 1;
    tmp = (tmp & 0xFFFF) | (eachX << 16);
    _CrtSetDbgFlag(tmp);
    // Check heap every
    //_CrtSetBreakAlloc(2031);
}
#endif


static void ShowAboutDialog(const ResourceCompiler& rc)
{
    const string newline("\r\n");

    const string s = rc.GetResourceCompilerGenericInfo(newline);

    string sSuffix;
    sSuffix += newline;
    sSuffix += newline;
    sSuffix += newline;
    sSuffix += "Use \"RC /help\" to list all available command-line options.";
    sSuffix += newline;
    sSuffix += newline;
    sSuffix += "Press [OK] to copy the info above to clipboard.";
#if defined(AZ_PLATFORM_WINDOWS)
    if (::MessageBoxA(NULL, (s + sSuffix).c_str(), "About", MB_OKCANCEL | MB_APPLMODAL | MB_SETFOREGROUND) == IDOK)
    {
        ResourceCompiler::CopyStringToClipboard(s);
    }
#else
    if (CryMessageBox((s + sSuffix).c_str(), "About", 0) == 1)
    {
        //TODO: CopyStringToClipboard needs cross platform support! Throwing an assert for now.
        assert(0);
    }
#endif
}


void GetCommandLineArguments(std::vector<string>& resArgs, int argc, char** argv)
{
    resArgs.clear();
    resArgs.reserve(argc);
    for (int i = 0; i < argc; ++i)
    {
        resArgs.push_back(string(argv[i]));
    }
}


void AddCommandLineArgumentsFromFile(std::vector<string>& args, const char* const pFilename)
{
    FILE* f = nullptr;
    azfopen(&f, pFilename, "rt");
    if (!f)
    {
        return;
    }

    char line[1024];
    while (fgets(line, sizeof(line), f) != 0)
    {
        if (line[0] == 0)
        {
            continue;
        }

        string sLine = line;
        sLine.Trim();
        if (sLine.empty())
        {
            continue;
        }

        args.push_back(sLine);
    }

    fclose(f);
}


static string GetTimeAsString(const time_t tm)
{
#if defined(AZ_PLATFORM_WINDOWS)
    char buffer[26];
    ctime_s(buffer, sizeof buffer, &tm);
#else
    char* buffer = ctime(&tm);
#endif
    string str = buffer;
    while (StringHelpers::EndsWith(str, "\n"))
    {
        str = str.substr(0, str.length() - 1);
    }

    return str;
}


static void ShowResourceCompilerVersionInfo(const ResourceCompiler& rc)
{
    const string newline("\n");
    const string info = rc.GetResourceCompilerGenericInfo(newline);
    std::vector<string> rows;
    StringHelpers::Split(info, newline, true, rows);
    for (size_t i = 0; i < rows.size(); ++i)
    {
        RCLog("%s", rows[i].c_str());
    }
}


static void ShowResourceCompilerLaunchInfo(const std::vector<string>& args, int originalArgc, const ResourceCompiler& rc)
{
    ShowResourceCompilerVersionInfo(rc);

    RCLog("");
    RCLog("Command line:");
    for (size_t i = 0; i < args.size(); ++i)
    {
        if ((int)i < originalArgc)
        {
            RCLog("  \"%s\"", args[i].c_str());
        }
        else
        {
            RCLog("  \"%s\"  (from %s)", args[i].c_str(), rc.m_filenameOptions);
        }
    }
    RCLog("");

    RCLog("Platforms specified in %s:", rc.m_filenameRcIni);
    for (int i = 0; i < rc.GetPlatformCount(); ++i)
    {
        const PlatformInfo* const p = rc.GetPlatformInfo(i);
        RCLog("  %s (%s)",
            p->GetCommaSeparatedNames().c_str(),
            (p->bBigEndian ? "big-endian" : "little-endian"));
    }
    RCLog("");

    RCLog("Started at: %s", GetTimeAsString(time(0)).c_str());
}


static void ShowWaitDialog(const ResourceCompiler& rc, const string& action, const std::vector<string>& args, int originalArgc)
{
    const string newline("\r\n");

    const string title = string("RC is about to ") + action;

    string sPrefix;
    sPrefix += title;
    sPrefix += " (/wait was specified).";
    sPrefix += newline;
    sPrefix += newline;
    sPrefix += newline;

    string s;
    s += rc.GetResourceCompilerGenericInfo(newline);
    s += "Command line:";
    s += newline;
    for (size_t i = 0; i < args.size(); ++i)
    {
        s += "  \"";
        s += args[i];
        s += "\"";
        if ((int)i >= originalArgc)
        {
            s += "  (from ";
            s += rc.m_filenameOptions;
            s += ")";
        }
        s += newline;
    }

    string sSuffix;
    sSuffix += newline;
    sSuffix += "Do you want to copy the info above to clipboard?";

#if defined(AZ_PLATFORM_WINDOWS)
    if (::MessageBoxA(NULL, (sPrefix + s + sSuffix).c_str(), title.c_str(), MB_YESNO | MB_ICONINFORMATION | MB_APPLMODAL | MB_SETFOREGROUND) == IDYES)
    {
        ResourceCompiler::CopyStringToClipboard(s);
    }
#else
    if (CryMessageBox((sPrefix + s + sSuffix).c_str(), title.c_str(), 0) == 1)
    {
        //TODO: CopyStringToClipboard needs cross platform support! Assert for now.
        assert(0);
    }
#endif
}


static bool RegisterConvertors(ResourceCompiler* pRc)
{
    string strDir = pRc->GetExePath();

    strDir.append(ResourceCompiler::m_rcPluginSubfolder);
    strDir.append(AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING);

    AZ::IO::LocalFileIO localFile;
    bool foundOK = localFile.FindFiles(strDir.c_str(), CryLibraryDefName("ResourceCompiler*"), [&](const char* pluginFilename) -> bool
    {
#if defined(AZ_PLATFORM_WINDOWS)
        HMODULE hPlugin = CryLoadLibrary(pluginFilename);
#elif AZ_TRAIT_OS_PLATFORM_APPLE || defined(AZ_PLATFORM_LINUX)
        HMODULE hPlugin = CryLoadLibrary(pluginFilename, false, false);
#endif
        if (!hPlugin)
        {
            const DWORD errCode = GetLastError();
            char messageBuffer[1024] = { '?', 0 };
#if defined(AZ_PLATFORM_WINDOWS)
            FormatMessageA(
                FORMAT_MESSAGE_FROM_SYSTEM,
                NULL,
                errCode,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                messageBuffer,
                sizeof(messageBuffer) - 1,
                NULL);
#endif
            RCLogError("Couldn't load plug-in module \"%s\"", pluginFilename);
            RCLogError("Error code: 0x%x = %s", errCode, messageBuffer);
            // this return controls whether to keep going on other converters or stop the entire process here.
            // it is NOT AN ERROR if one resource compiler dll fails to load
            // it might not be the DLL that is required for this particular compile.
            return true;
        }


        FnRegisterConvertors fnRegister =
            hPlugin ? (FnRegisterConvertors)CryGetProcAddress(hPlugin, "RegisterConvertors") : NULL;
        if (!fnRegister)
        {
            RCLog("Error: plug-in module \"%s\" doesn't have RegisterConvertors function", pluginFilename);
            CryFreeLibrary(hPlugin);
            // this return controls whether to keep going on other converters or stop the entire process here.
            // it is NOT AN ERROR if one resource compiler dll fails to load
            // it might not be the DLL that is required for this particular compile.
            return true;
        }

        RCLog("  Loaded \"%s\"", pluginFilename);

        pRc->AddPluginDLL(hPlugin);

        const int oldErrorCount = pRc->GetNumErrors();
        fnRegister(pRc);
        const int newErrorCount = pRc->GetNumErrors();
        if (newErrorCount > oldErrorCount)
        {
            RCLog("Error: plug-in module \"%s\" emitted errors during register", pluginFilename);
            pRc->RemovePluginDLL(hPlugin);
            FnBeforeUnloadDLL fnBeforeUnload = (FnBeforeUnloadDLL)CryGetProcAddress(hPlugin, "BeforeUnloadDLL");
            if (fnBeforeUnload)
            {
                (*fnBeforeUnload)();
            }
            CryFreeLibrary(hPlugin);
            // this return controls whether to keep going on other converters or stop the entire process here.
            return true;
        }

        return true; // continue iterating to all plugins
    });

    return true;
}


int rcmain(int argc, char** argv, [[maybe_unused]] char** envp)
{
#if defined(AZ_PLATFORM_WINDOWS)
    GetCrashHandler(); // just to initialize
#endif

    std::unique_ptr<QCoreApplication> qApplication = CreateQApplication(argc, argv);

#if 0
    EnableCrtMemoryChecks();
#endif

#if defined(AZ_PLATFORM_WINDOWS)
    MathHelpers::EnableFloatingPointExceptions(~(_CW_DEFAULT));
#endif

    ResourceCompiler rc;

    rc.QueryVersionInfo();
    rc.InitPaths();

    if (argc <= 1)
    {
        ShowAboutDialog(rc);
        return eRcExitCode_Success;
    }

    std::vector<string> args;
    {
        GetCommandLineArguments(args, argc, argv);

        const string filename = string(rc.GetExePath()) + rc.m_filenameOptions;
        AddCommandLineArgumentsFromFile(args, filename.c_str());
    }

    rc.RegisterDefaultKeys();

    string fileSpec;

    // Initialization, showing startup info, loading configs
    {
        Config mainConfig;
        mainConfig.SetConfigKeyRegistry(&rc);

        QSettings settings("HKEY_CURRENT_USER\\Software\\Amazon\\Lumberyard\\Settings", QSettings::NativeFormat);
        bool enableSourceControl = settings.value("RC_EnableSourceControl", true).toBool();
        mainConfig.SetKeyValue(eCP_PriorityCmdline, "nosourcecontrol", enableSourceControl ? "0" : "1");

        CmdLine::Parse(args, &mainConfig, fileSpec);

        // initialize rc (also initializes logs)
        rc.Init(mainConfig);

        AZ::Debug::Trace::HandleExceptions(true);
#if defined(AZ_PLATFORM_WINDOWS)
        GetCrashHandler().SetDumpFile(rc.FormLogFileName(ResourceCompiler::m_filenameCrashDump));
#endif

        if (mainConfig.GetAsBool("version", false, true))
        {
            ShowResourceCompilerVersionInfo(rc);
            return eRcExitCode_Success;
        }

        switch (mainConfig.GetAsInt("wait", 0, 1))
        {
        case 3:
        case 4:
            ShowWaitDialog(rc, "start", args, argc);
            break;
        default:
            break;
        }

        ShowResourceCompilerLaunchInfo(args, argc, rc);

        rc.SetTimeLogging(mainConfig.GetAsBool("logtime", true, true));

        rc.LogMemoryUsage(false);

        RCLog("");

        if (!rc.LoadIniFile())
        {
            return eRcExitCode_FatalError;
        }

        // Make sure that rc.ini doesn't have obsolete settings
        for (int i = 0;; ++i)
        {
            const char* const pName = rc.GetIniFile()->GetSectionName(i);
            if (!pName)
            {
                break;
            }

            Config cfg;
            rc.GetIniFile()->CopySectionKeysToConfig(eCP_PriorityRcIni, i, 0, &cfg);

            if (cfg.HasKeyMatchingWildcards("srgb") || cfg.HasKeyMatchingWildcards("srgb:*"))
            {
                RCLogError("Obsolete setting 'srgb' found in %s", rc.m_filenameRcIni);
                RCLog(
                    "\n"
                    "Please replace all occurences of 'srgb' by corresponding\n"
                    "'colorspace' settings. Use the following table as the reference:\n"
                    "  srgb=0 -> colorspace=linear,linear\n"
                    "  srgb=1 -> colorspace=sRGB,auto\n"
                    "  srgb=2 -> colorspace=sRGB,sRGB\n"
                    "  srgb=3 -> colorspace=linear,sRGB\n"
                    "  srgb=4 -> colorspace=sRGB,linear");
                return eRcExitCode_FatalError;
            }
        }

        // Load list of platforms
        {
            for (int i = 0;; ++i)
            {
                const char* const pName = rc.GetIniFile()->GetSectionName(i);
                if (!pName)
                {
                    break;
                }
                if (!StringHelpers::Equals(pName, "_platform"))
                {
                    continue;
                }

                Config cfg;
                rc.GetIniFile()->CopySectionKeysToConfig(eCP_PriorityRcIni, i, "", &cfg);

                const string names = StringHelpers::MakeLowerCase(cfg.GetAsString("name", "", ""));
                const bool bBigEndian = cfg.GetAsBool("bigendian", false, true);
                const int pointerSize = cfg.GetAsInt("pointersize", 4, 0);

                if (!rc.AddPlatform(names, bBigEndian, pointerSize))
                {
                    RCLogError("Bad platform data in %s", rc.m_filenameRcIni);
                    return eRcExitCode_FatalError;
                }
            }

            if (rc.GetPlatformCount() <= 0)
            {
                RCLogError("Missing [_platform] in %s", rc.m_filenameRcIni);
                return eRcExitCode_FatalError;
            }
        }

        // Obtain target platform
        int platform;
        {
            string platformStr = mainConfig.GetAsString("p", "", "");
            if (platformStr.empty())
            {
                if (!mainConfig.GetAsBool("version", false, true))
                {
                    RCLog("Platform (/p) not specified, defaulting to 'pc'.");
                    RCLog("");
                }
                platformStr = "pc";
                mainConfig.SetKeyValue(eCP_PriorityCmdline, "p", platformStr.c_str());
            }

            platform = rc.FindPlatform(platformStr.c_str());
            if (platform < 0)
            {
                RCLogError("Unknown platform specified: '%s'", platformStr.c_str());
                return eRcExitCode_FatalError;
            }
        }

        // Load configs for every platform
        rc.GetMultiplatformConfig().init(rc.GetPlatformCount(), platform, &rc);
        for (int i = 0; i < rc.GetPlatformCount(); ++i)
        {
            IConfig& cfg = rc.GetMultiplatformConfig().getConfig(i);
            rc.GetIniFile()->CopySectionKeysToConfig(eCP_PriorityRcIni, 0, rc.GetPlatformInfo(i)->GetCommaSeparatedNames().c_str(), &cfg);
            cfg.AddConfig(&mainConfig);
        }
    }

    const IConfig& config = rc.GetMultiplatformConfig().getConfig();

    {
        RCLog("Initializing pak management");
        rc.InitPakManager();
        RCLog("");

        RCLog("Initializing System");


        string appRootInput = config.GetAsString("approot", "", "");
        if (!appRootInput.empty())
        {
            rc.SetAppRootPath(appRootInput);
        }
        else
        {
            // Create a local SettingsRegistry to read the bootstrap.cfg settings if the appRoot hasn't been overriden
            // on the command line
            AZ::SettingsRegistryImpl settingsRegistry;
            AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_Bootstrap(settingsRegistry);
            string gameName = config.GetAsString("gamesubdirectory", "", "");
            if (!gameName.empty())
            {
                const auto gameFolderKey = AZ::SettingsRegistryInterface::FixedValueString::format("%s/sys_game_folder",
                    AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey);
                settingsRegistry.Set(gameFolderKey, gameName);
            }
            AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddRuntimeFilePaths(settingsRegistry);
            // and because we're a tool, add the tool folders:
            if (AZ::SettingsRegistryInterface::FixedValueString appRoot; settingsRegistry.Get(appRoot, AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder))
            {
                rc.SetAppRootPath(string{ appRoot.c_str(), appRoot.size() });
            }
        }

        // only after installing and setting those up, do we install our handler because perforce does this too...
#if defined(AZ_PLATFORM_WINDOWS)
        SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandlerRoutine, TRUE);
#elif AZ_TRAIT_OS_PLATFORM_APPLE || defined(AZ_PLATFORM_LINUX)
        signal(SIGINT, CtrlHandlerRoutine);
#endif
        RCLog("");

        RCLog("Loading compiler plug-ins (ResourceCompiler*.dll)");

        // Force the current working directory to be the same as the executable so
        // that we can load any shared libraries that don't have run-time paths in
        // them (I'm looking at you AWS SDK libraries!).
        QString currentDir = QDir::currentPath();
        QDir::setCurrent(QCoreApplication::applicationDirPath());

        if (!RegisterConvertors(&rc))
        {
            RCLogError("A fatal error occurred when loading plug-ins (see error message(s) above). RC cannot continue.");
            rc.UnregisterConvertors();
            return eRcExitCode_FatalError;
        }
        // Restore currentDir so that file paths will work that are relative to
        // where the user executed RC
        QDir::setCurrent(currentDir);
        RCLog("");

        RCLog("Loading zip & pak compiler module");
        rc.RegisterConvertor("zip & pak compiler", new ZipEncryptor(&rc));
        RCLog("");

        rc.LogMemoryUsage(false);
    }

    const bool bJobMode = config.HasKey("job");
    // Don't even bother setting up if we aren't going to do anything
    if (!bJobMode && !ResourceCompiler::CheckCommandLineOptions(config, 0))
    {
        return eRcExitCode_Error;
    }

    bool bExitCodeIsReady = false;
    int exitCode = eRcExitCode_Success;

    bool bShowUsage = false;
    if (bJobMode)
    {
        const int tmpResult = rc.ProcessJobFile();
        if (tmpResult)
        {
            exitCode = tmpResult;
            bExitCodeIsReady = true;
        }
        rc.PostBuild();      // e.g. writing statistics files
    }
    else if (!fileSpec.empty())
    {
        rc.RemoveOutputFiles();
        std::vector<RcFile> files;
        if (rc.CollectFilesToCompile(fileSpec, files) && !files.empty())
        {
            rc.CompileFilesBySingleProcess(files);
        }
        rc.PostBuild();      // e.g. writing statistics files
    }
    else
    {
        bShowUsage = true;
    }

    rc.UnregisterConvertors();

    rc.SetTimeLogging(false);

    if (bShowUsage && !rc.m_bQuiet)
    {
        rc.ShowHelp(false);
    }

    if (config.GetAsBool("help", false, true))
    {
        rc.ShowHelp(true);
    }

    rc.LogMemoryUsage(false);

    RCLog("");
    RCLog("Finished at: %s", GetTimeAsString(time(0)).c_str());

    if (rc.GetNumErrors() || rc.GetNumWarnings())
    {
        RCLog("");
        RCLogSummary("%d errors, %d warnings.", rc.GetNumErrors(), rc.GetNumWarnings());
    }

    if (!bExitCodeIsReady)
    {
        const bool bFail = rc.GetNumErrors() || (rc.GetNumWarnings() && config.GetAsBool("failonwarnings", false, true));
        exitCode = (bFail ? eRcExitCode_Error : eRcExitCode_Success);
        bExitCodeIsReady = true;
    }

    switch (config.GetAsInt("wait", 0, 1))
    {
    case 1:
        RCLog("");
        RCLog("    Press <RETURN> (/wait was specified)");
        getchar();
        break;
    case 2:
    case 4:
        ShowWaitDialog(rc, "finish", args, argc);
        break;
    default:
        break;
    }

    return exitCode;
}


//////////////////////////////////////////////////////////////////////////
int __cdecl main(int argc, char** argv, char** envp)
{
    AZ::Sfmt::Create();

    AZ::AllocatorInstance<AZ::SystemAllocator>::Create();
    AZ::AllocatorInstance<AZ::LegacyAllocator>::Create();
    AZ::AllocatorInstance<CryStringAllocator>::Create();

    int exitCode = 1;
    {
        exitCode = rcmain(argc, argv, envp);
    }

    AZ::AllocatorInstance<CryStringAllocator>::Destroy();
    AZ::AllocatorInstance<AZ::LegacyAllocator>::Destroy();
    AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();

    AZ::Sfmt::Destroy();

    //////////////////////////////////////////////////////////////////////////
    AZ::AllocatorManager::Destroy();
    return exitCode;
}
