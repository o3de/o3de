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

// #include <platform.h>
// #include <ProjectDefines.h>

// Insert your headers here
// #include <AzCore/PlatformIncl.h>
// #include <algorithm>
// #include <vector>

#include <platform.h>
#include <ISystem.h>

#include <StringUtils.h>

#include <AzFramework/Application/Application.h>
#include <AzCore/Module/DynamicModuleHandle.h>
#include <AzCore/PlatformId/PlatformId.h>
#include <AzCore/std/string/osstring.h>
#include <AzCore/IPC/SharedMemory.h>

#include <CryLibrary.h>
#include <IConsole.h>

#if defined(AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS)
#undef AZ_RESTRICTED_SECTION
#define SHADERCACHEGEN_CPP_SECTION_1 1
#endif

#if defined(AZ_PLATFORM_MAC)
#include <fcntl.h>
#include <errno.h>
#include <semaphore.h>
#include <CoreFoundation/CoreFoundation.h>
#endif

static bool s_displayMessageBox = true;

struct COutputPrintSink
    : public IOutputPrintSink
{
    virtual void Print(const char* inszText)
    {
        printf("%s\n", inszText);
    }
};

#if defined(AZ_PLATFORM_MAC)
CFOptionFlags MessageBox(const char* title, const char* message, CFStringRef defaultButton, CFStringRef alternateButton)
{
    CFStringRef strHeader = CFStringCreateWithCString(NULL, title, kCFStringEncodingMacRoman);
    CFStringRef strMsg = CFStringCreateWithCString(NULL, message, kCFStringEncodingMacRoman);
    
    CFOptionFlags result;
    CFUserNotificationDisplayAlert(
                                   0,
                                   kCFUserNotificationNoteAlertLevel,
                                   NULL,
                                   NULL,
                                   NULL,
                                   strHeader,
                                   strMsg,
                                   defaultButton,
                                   alternateButton,
                                   NULL,
                                   &result
                                   );
    
    if (strHeader)
    {
        CFRelease(strHeader);
    }

    if (strMsg)
    {
        CFRelease(strMsg);
    }
    return result;
}
#endif // defined(AZ_PLATFORM_MAC)

bool DisplayYesNoMessageBox(const char* title, const char* message)
{
    if (!s_displayMessageBox)
    {
        return false;
    }

#if defined(AZ_PLATFORM_WINDOWS)
    return MessageBox(0, message, title, MB_YESNO) == IDYES;
#elif defined(AZ_PLATFORM_MAC)
    return MessageBox(title, message, CFSTR("Yes"), CFSTR("No")) == kCFUserNotificationDefaultResponse;
#else
    return false;
#endif
}

void DisplayErrorMessageBox(const char* message)
{
    if (!s_displayMessageBox)
    {
        printf("Error: %s\n", message);
        return;
    }

#if defined(AZ_PLATFORM_WINDOWS)
    MessageBox(0, message, "Error", MB_OK | MB_DEFAULT_DESKTOP_ONLY);
#elif defined(AZ_PLATFORM_MAC)
    MessageBox("Error", message, NULL, NULL);
#endif
}

void ClearPlatformCVars(ISystem* pISystem)
{
    pISystem->GetIConsole()->ExecuteString("r_ShadersDX11 = 0");
    pISystem->GetIConsole()->ExecuteString("r_ShadersMETAL = 0");
    pISystem->GetIConsole()->ExecuteString("r_ShadersGL4 = 0");
    pISystem->GetIConsole()->ExecuteString("r_ShadersGLES3 = 0");
    pISystem->GetIConsole()->ExecuteString("r_ShadersOrbis = 0");
}

bool IsO3DERunning()
{
    bool isRunning = false;
    const char* mutexName = "O3DEApplication";
#if defined(AZ_PLATFORM_WINDOWS)
    HANDLE mutex = CreateMutex(NULL, TRUE, mutexName);
    isRunning = GetLastError() == ERROR_ALREADY_EXISTS;
    if (mutex)
    {
        CloseHandle(mutex);
    }
#elif defined(AZ_PLATFORM_MAC)
    sem_t* mutex = sem_open(mutexName, O_CREAT | O_EXCL);
    if (mutex == SEM_FAILED && errno == EEXIST)
    {
        isRunning = true;
    }
    else
    {
        sem_close(mutex);
        sem_unlink(mutexName);
    }
#endif

    return isRunning;
}

static AZ::EnvironmentVariable<IMemoryManager*> s_cryMemoryManager = nullptr;
HMODULE s_crySystemDll = nullptr;

void AcquireCryMemoryManager()
{
    using GetMemoryManagerInterfaceFunction = decltype(CryGetIMemoryManagerInterface)*;

    IMemoryManager* cryMemoryManager = nullptr;
    GetMemoryManagerInterfaceFunction getMemoryManager = nullptr;

    s_crySystemDll = CryLoadLibraryDefName("CrySystem");
    AZ_Assert(s_crySystemDll, "Unable to load CrySystem to resolve memory manager");
    getMemoryManager = reinterpret_cast<GetMemoryManagerInterfaceFunction>(CryGetProcAddress(s_crySystemDll, "CryGetIMemoryManagerInterface"));
    AZ_Assert(getMemoryManager, "Unable to resolve CryGetIMemoryInterface via CrySystem");
    getMemoryManager((void**)&cryMemoryManager);
    AZ_Assert(cryMemoryManager, "Unable to resolve CryMemoryManager");
    s_cryMemoryManager = AZ::Environment::CreateVariable<IMemoryManager*>("CryIMemoryManagerInterface", cryMemoryManager);
}

void ReleaseCryMemoryManager()
{
    s_cryMemoryManager.Reset();
    if (s_crySystemDll)
    {
        while (CryFreeLibrary(s_crySystemDll))
        {
            // keep freeing until free.
            // this is unfortunate, but CryMemoryManager currently in its internal impl grabs an extra handle to this and does not free it nor
            // does it have an interface to do so.
            // until we can rewrite the memory manager init and shutdown code, we need to do this here because we need crysystem GONE
            // before we attempt to destroy the memory managers which it uses.
            ;
        }
        s_crySystemDll = nullptr;
    }
}

// wrapped main so that it can be inside the lifetime of an AZCore Application in the real main()
// and all memory created on the stack here in main_wrapped can go out of scope before we stop the application
int main_wrapped(int argc, char* argv[])
{
    const int errorCode = 1;
    if (argc < 2)
    {
        printf("\nInvalid number of arguments. Usage:\n\
                ShaderGen BuildGlobalCache [NoCompile] | BuildLevelCache\n\
                ShadersPlatform={D3D11/GLES3/GL4/METAL}\n\
                TargetPlatform={pc/es3/ios/osx_gl}\n\
                [-nopromt][-devmode]\n");
        return errorCode;
    }

    AZ::OSString cmdLineString;
    for (int i = 0; i < argc - 1; ++i)
    {
        cmdLineString += argv[i];
        cmdLineString += " ";
    }
    cmdLineString += argv[argc - 1];
    const char* commandLine = cmdLineString.c_str();
    
    if (CryStringUtils::stristr(commandLine, "-noprompt"))
    {
        s_displayMessageBox = false;
    }

    if (IsO3DERunning())
    {
        if (CryStringUtils::stristr(commandLine, "-devmode") == 0)
        {
            DisplayErrorMessageBox("There is already a Open 3D Engine application running. Cannot start another one!");
            return errorCode;
        }

        if (s_displayMessageBox)
        {
            if (!DisplayYesNoMessageBox("Too many apps", "There is already a Open 3D Engine application running\nDo you want to start another one?"))
            {
                return errorCode;
            }
        }
    }

#if defined(AZ_PLATFORM_WINDOWS)
    InitRootDir();
#endif

    PFNCREATESYSTEMINTERFACE CreateSystemInterface = reinterpret_cast<PFNCREATESYSTEMINTERFACE>(CryGetProcAddress(s_crySystemDll, "CreateSystemInterface"));

    COutputPrintSink printSink;
    SSystemInitParams sip;

    using PlatformMap = AZStd::unordered_map<AZ::OSString, AZ::PlatformID, AZStd::hash<AZ::OSString>>;
    using PlatformMapElement = PlatformMap::value_type;

    const PlatformMap platforms =
    {
        { "pc", AZ::PlatformID::PLATFORM_WINDOWS_64 },
        { "es3", AZ::PlatformID::PLATFORM_ANDROID_64 },
        { "ios", AZ::PlatformID::PLATFORM_APPLE_IOS },
        { "osx_gl", AZ::PlatformID::PLATFORM_APPLE_OSX },
#if defined(AZ_EXPAND_FOR_RESTRICTED_PLATFORM) || defined(AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS)
#define AZ_RESTRICTED_PLATFORM_EXPANSION(CodeName, CODENAME, codename, PrivateName, PRIVATENAME, privatename, PublicName, PUBLICNAME, publicname, PublicAuxName1, PublicAuxName2, PublicAuxName3)\
        { #codename,    AZ::PlatformID::PLATFORM_##PUBLICNAME },
#if defined(AZ_EXPAND_FOR_RESTRICTED_PLATFORM)
        AZ_EXPAND_FOR_RESTRICTED_PLATFORM
#else
        AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS
#endif
#undef AZ_RESTRICTED_PLATFORM_EXPANSION
#endif
        
    };

    AZ::PlatformID platform = AZ::PlatformID::PLATFORM_MAX;
    AZ::OSString shaderTypeCommand;

    // Parse the args that have to do with shader type and target platform
    if (CryStringUtils::stristr(commandLine, "ShadersPlatform"))
    {
        if (CryStringUtils::stristr(commandLine, "ShadersPlatform=PC") || CryStringUtils::stristr(commandLine, "ShadersPlatform=D3D11") != 0)
        {
            shaderTypeCommand = "r_ShadersDX11 = 1";
            platform = AZ::PlatformID::PLATFORM_WINDOWS_64;
        }
#if defined(AZ_EXPAND_FOR_RESTRICTED_PLATFORM) || defined(AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS)
#define AZ_RESTRICTED_PLATFORM_EXPANSION(CodeName, CODENAME, codename, PrivateName, PRIVATENAME, privatename, PublicName, PUBLICNAME, publicname, PublicAuxName1, PublicAuxName2, PublicAuxName3)\
        else if (CryStringUtils::stristr(commandLine, "ShadersPlatform=" #PrivateName) != 0)       \
        {                                                                                       \
            shaderTypeCommand = "r_Shaders" #PrivateName " = 1";                                \
            platform = AZ::PlatformID::PLATFORM_##PUBLICNAME;                                               \
        }
#if defined(AZ_EXPAND_FOR_RESTRICTED_PLATFORM)
        AZ_EXPAND_FOR_RESTRICTED_PLATFORM
#else
        AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS
#endif
#undef AZ_RESTRICTED_PLATFORM_EXPANSION
#endif
        else if (CryStringUtils::stristr(commandLine, "ShadersPlatform=GL4") != 0)
        {
            shaderTypeCommand = "r_ShadersGL4 = 1";
            platform = AZ::PlatformID::PLATFORM_WINDOWS_64;
        }
        else if (CryStringUtils::stristr(commandLine, "ShadersPlatform=GLES3") != 0)
        {
            shaderTypeCommand = "r_ShadersGLES3 = 1";
            platform = AZ::PlatformID::PLATFORM_ANDROID_64;
        }
        else if (CryStringUtils::stristr(commandLine, "ShadersPlatform=METAL") != 0)
        {
            shaderTypeCommand = "r_ShadersMETAL = 1";
            // For metal the platform has to be specified in the command line
        }
    }

    const char* platformArg = "TargetPlatform=";
    if (const char* platformString = CryStringUtils::stristr(commandLine, platformArg))
    {
        // Get the platform value
        platformString += strlen(platformArg);
        // Search for the next white space
        const char* platformStringEnd = strchr(platformString, ' ');
        size_t stringLen = platformStringEnd ? platformStringEnd - platformString : strlen(platformString);
        AZ::OSString platformAsString(platformString, stringLen);
        AZStd::to_lower(platformAsString.begin(), platformAsString.end());
        auto it = platforms.find(platformAsString);
        if (it != platforms.end())
        {
            platform = it->second;
        }
    }

    // Search for the platform name using the enum value
    PlatformMap::const_iterator foundPlatform = AZStd::find_if(platforms.begin(), platforms.end(), [&platform](const PlatformMapElement& element) { return element.second == platform; });
    if (foundPlatform == platforms.end())
    {
        DisplayErrorMessageBox("Invalid target platform");
        return errorCode;
    }

    sip.bShaderCacheGen = true;
    sip.bDedicatedServer = false;
    sip.bPreview = false;
    sip.bTestMode = false;
    sip.bMinimal = true;
    sip.bToolMode = true;
    sip.pSharedEnvironment = AZ::Environment::GetInstance();

    sip.pPrintSync = &printSink;
    cry_strcpy(sip.szSystemCmdLine, commandLine);

    // this actually detects our root as part of construction.
    // we will load an alternate bootstrap file so that this tool never turns on VFS.

    sip.sLogFileName = "@log@/ShaderCacheGen.log";
    sip.bSkipFont = true;

    ISystem* pISystem = CreateSystemInterface(sip);
    if (!pISystem)
    {
        DisplayErrorMessageBox("CreateSystemInterface Failed");
        return false;
    }

    ////////////////////////////////////
    // Current command line options

    pISystem->ExecuteCommandLine();

    // Set the shader type
    if (!shaderTypeCommand.empty())
    {
        ClearPlatformCVars(pISystem);

        pISystem->GetIConsole()->ExecuteString(shaderTypeCommand.c_str());

#if defined(AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS)
#if defined(TOOLS_SUPPORT_JASPER)
#define AZ_RESTRICTED_SECTION SHADERCACHEGEN_CPP_SECTION_1
#include AZ_RESTRICTED_FILE_EXPLICIT(ShaderCacheGen_cpp, jasper)
#endif
#if defined(TOOLS_SUPPORT_PROVO)
#define AZ_RESTRICTED_SECTION SHADERCACHEGEN_CPP_SECTION_1
#include AZ_RESTRICTED_FILE_EXPLICIT(ShaderCacheGen_cpp, provo)
#endif
#if defined(TOOLS_SUPPORT_SALEM)
#define AZ_RESTRICTED_SECTION SHADERCACHEGEN_CPP_SECTION_1
#include AZ_RESTRICTED_FILE_EXPLICIT(ShaderCacheGen_cpp, salem)
#endif
#endif

    }

    // Set the target platform
    if (platform != AZ::PlatformID::PLATFORM_MAX)
    {
        pISystem->GetIConsole()->ExecuteString(AZStd::string::format("r_ShadersPlatform = %d", static_cast<AZ::u32>(platform)).c_str());
    }
    
    if (CryStringUtils::stristr(commandLine, "BuildGlobalCache") != 0)
    {
        // to only compile shader explicitly listed in global list, call PrecacheShaderList
        if (CryStringUtils::stristr(commandLine, "NoCompile") != 0)
        {
            pISystem->GetIConsole()->ExecuteString("r_StatsShaderList");
        }
        else
        {
            pISystem->GetIConsole()->ExecuteString("r_PrecacheShaderList");
        }
    }
    else if (CryStringUtils::stristr(commandLine, "BuildLevelCache") != 0)
    {
        pISystem->GetIConsole()->ExecuteString("r_PrecacheShadersLevels");
    }
    else if (CryStringUtils::stristr(commandLine, "GetShaderList") != 0)
    {
        pISystem->GetIConsole()->ExecuteString("r_GetShaderList");
    }

    ////////////////////////////////////
    // Deprecated command line options
    if (strstr(commandLine, "PrecacheShaderList") != 0)
    {
        pISystem->GetIConsole()->ExecuteString("r_PrecacheShaderList");
    }
    else if (strstr(commandLine, "StatsShaderList") != 0)
    {
        pISystem->GetIConsole()->ExecuteString("r_StatsShaderList");
    }
    else if (strstr(commandLine, "StatsShaders") != 0)
    {
        pISystem->GetIConsole()->ExecuteString("r_StatsShaders");
    }
    else if (strstr(commandLine, "PrecacheShadersLevels") != 0)
    {
        pISystem->GetIConsole()->ExecuteString("r_PrecacheShadersLevels");
    }
    else if (strstr(commandLine, "PrecacheShaders") != 0)
    {
        pISystem->GetIConsole()->ExecuteString("r_PrecacheShaders");
    }
    else if (strstr(commandLine, "MergeShaders") != 0)
    {
        pISystem->GetIConsole()->ExecuteString("r_MergeShaders");
    }

    if (pISystem)
    {
        pISystem->Quit();
        pISystem->Release();
    }
    pISystem = nullptr;

    return 0;
}

//////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    AZ::AllocatorInstance<AZ::SystemAllocator>::Create();
    AZ::AllocatorInstance<AZ::LegacyAllocator>::Create();
    AZ::AllocatorInstance<CryStringAllocator>::Create();

    AzFramework::Application app(&argc, &argv);
    AzFramework::Application::StartupParameters startupParams;
    AzFramework::Application::Descriptor descriptor;
    app.Start(descriptor, startupParams);
    AcquireCryMemoryManager();

    AZ::Debug::Trace::Instance().Init();

    int returncode = main_wrapped(argc, argv);

    app.Stop();

    AZ::Debug::Trace::Instance().Destroy();

    ReleaseCryMemoryManager();
    AZ::AllocatorInstance<CryStringAllocator>::Destroy();
    AZ::AllocatorInstance<AZ::LegacyAllocator>::Destroy();
    AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
    return returncode;
}
