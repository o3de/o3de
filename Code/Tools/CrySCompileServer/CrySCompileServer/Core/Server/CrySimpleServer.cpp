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

#include <AzCore/base.h>
#include <AzCore/PlatformDef.h>
#include <AzCore/std/parallel/thread.h>

#include "CrySimpleServer.hpp"
#include "CrySimpleSock.hpp"
#include "CrySimpleJob.hpp"
#include "CrySimpleJobCompile1.hpp"
#include "CrySimpleJobCompile2.hpp"
#include "CrySimpleJobRequest.hpp"
#include "CrySimpleJobGetShaderList.hpp"
#include "CrySimpleCache.hpp"
#include "CrySimpleErrorLog.hpp"
#include "ShaderList.hpp"

#include <Core/StdTypes.hpp>
#include <Core/Error.hpp>
#include <Core/STLHelper.hpp>
#include <tinyxml/tinyxml.h>
#include <Core/WindowsAPIImplementation.h>
#include <Core/Mailer.h>

#include <AzFramework/StringFunc/StringFunc.h>

#include <AzCore/Debug/Trace.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Jobs/Job.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/time.h>
#include <AzCore/std/bind/bind.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Utils/Utils.h>

#if defined(AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS)
#undef AZ_RESTRICTED_SECTION
#define CRYSIMPLESERVER_CPP_SECTION_1 1
#define CRYSIMPLESERVER_CPP_SECTION_2 2
#endif

#if defined(AZ_PLATFORM_MAC)
#include <libproc.h>
#include <sys/stat.h>
#endif

#include <assert.h>
#include <algorithm>
#include <memory>
#include <unordered_map>


#ifdef WIN32
    #define EXTENSION ".exe"
#else
    #define EXTENSION ""
#endif

AZStd::atomic_long CCrySimpleServer::ms_ExceptionCount    = {0};

const static std::string SHADER_PROFILER            =   "NVShaderPerf" EXTENSION;

const static std::string SHADER_PATH_SOURCE         =   "Source";
const static std::string SHADER_PATH_BINARY         =   "Binary";
const static std::string SHADER_PATH_HALFSTRIPPED   =   "HalfStripped";
const static std::string SHADER_PATH_DISASSEMBLED   =   "DisAsm";
const static std::string SHADER_PATH_STRIPPPED      =   "Stripped";
const static std::string SHADER_PATH_CACHE          =   "Cache";

static const bool autoDeleteJobWhenDone = true;
static const int sleepTimeWhenWaiting = 10;

static AZStd::atomic_long g_ConnectionCount = {0};

SEnviropment* SEnviropment::m_instance=nullptr;

void SEnviropment::Create()
{
    if (!m_instance)
    {
        m_instance = new SEnviropment;
    }
}

void SEnviropment::Destroy()
{
    if (m_instance)
    {
        delete m_instance;
        m_instance = nullptr;
    }
}

SEnviropment& SEnviropment::Instance()
{
    AZ_Assert(m_instance, "Using SEnviropment::Instance() before calling SEnviropment::Create()");
    return *m_instance;
}

// Shader Compilers ID
// NOTE: Values must be in sync with CShaderSrv::GetShaderCompilerName() function in the engine side.
const char* SEnviropment::m_Orbis_DXC = "Orbis_DXC";
const char* SEnviropment::m_Jasper_FXC = "Jasper_FXC";
const char* SEnviropment::m_D3D11_FXC = "D3D11_FXC";
const char* SEnviropment::m_GLSL_HLSLcc = "GLSL_HLSLcc";
const char* SEnviropment::m_METAL_HLSLcc = "METAL_HLSLcc";
const char* SEnviropment::m_GLSL_LLVM_DXC = "GLSL_LLVM_DXC";
const char* SEnviropment::m_METAL_LLVM_DXC = "METAL_LLVM_DXC";

void SEnviropment::InitializePlatformAttributes()
{
    // Initialize valid Plaforms
    // NOTE: Values must be in sync with CShaderSrv::GetPlatformName() function in the engine side.
    m_Platforms.insert("Orbis");
    m_Platforms.insert("Nx");
    m_Platforms.insert("PC");
    m_Platforms.insert("Mac");
    m_Platforms.insert("iOS");
    m_Platforms.insert("Android");
    m_Platforms.insert("Linux");
    m_Platforms.insert("Jasper");

    // Initialize valid Shader Languages
    // NOTE: Values must be in sync with GetShaderLanguageName() function in the engine side.
    m_ShaderLanguages.insert("Orbis");
    m_ShaderLanguages.insert("D3D11");
    m_ShaderLanguages.insert("METAL");
    m_ShaderLanguages.insert("GL4");
    m_ShaderLanguages.insert("GLES3");
    m_ShaderLanguages.insert("Jasper");
    // These are added for legacy support (GLES3_0 and GLES3_1 are combined into just GLES3)
    m_ShaderLanguages.insert("GL4_1");
    m_ShaderLanguages.insert("GL4_4");
    m_ShaderLanguages.insert("GLES3_0");
    m_ShaderLanguages.insert("GLES3_1");
    
    // Initialize valid Shader Compilers ID and Executables.
    // Intentionally put a space after the executable name so that attackers can't try to change the executable name that we are going to run.
#if defined(AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS)
#if defined(TOOLS_SUPPORT_JASPER)
#define AZ_RESTRICTED_SECTION CRYSIMPLESERVER_CPP_SECTION_2
#include AZ_RESTRICTED_FILE_EXPLICIT(CrySimpleServer_cpp, jasper)
#endif
#if defined(TOOLS_SUPPORT_PROVO)
#define AZ_RESTRICTED_SECTION CRYSIMPLESERVER_CPP_SECTION_2
#include AZ_RESTRICTED_FILE_EXPLICIT(CrySimpleServer_cpp, provo)
#endif
#if defined(TOOLS_SUPPORT_SALEM)
#define AZ_RESTRICTED_SECTION CRYSIMPLESERVER_CPP_SECTION_2
#include AZ_RESTRICTED_FILE_EXPLICIT(CrySimpleServer_cpp, salem)
#endif
#endif

    m_ShaderCompilersMap[m_D3D11_FXC] = "PCD3D11/v006/fxc.exe ";
    m_ShaderCompilersMap[m_GLSL_HLSLcc] = "PCGL/V006/HLSLcc ";
    m_ShaderCompilersMap[m_METAL_HLSLcc] = "PCGMETAL/HLSLcc/HLSLcc ";
#if defined(_DEBUG)
    m_ShaderCompilersMap[m_GLSL_LLVM_DXC] = "LLVMGL/debug/dxcGL ";
    m_ShaderCompilersMap[m_METAL_LLVM_DXC] = "LLVMMETAL/debug/dxcMetal ";
#else
    m_ShaderCompilersMap[m_GLSL_LLVM_DXC] = "LLVMGL/release/dxcGL ";
    m_ShaderCompilersMap[m_METAL_LLVM_DXC] = "LLVMMETAL/release/dxcMetal ";
#endif
}

bool SEnviropment::IsPlatformValid( const AZStd::string& platform ) const
{
    return m_Platforms.find(platform) != m_Platforms.end();
}

bool SEnviropment::IsShaderLanguageValid( const AZStd::string& shaderLanguage ) const
{
    return m_ShaderLanguages.find(shaderLanguage) != m_ShaderLanguages.end();
}

bool SEnviropment::IsShaderCompilerValid( const AZStd::string& shaderCompilerID ) const
{
    bool validCompiler = (m_ShaderCompilersMap.find(shaderCompilerID) != m_ShaderCompilersMap.end());
    
    // Extra check for Mac: Only GL_LLVM_DXC and METAL_LLVM_DXC compilers are supported.
    #if defined(AZ_PLATFORM_MAC)
        if (validCompiler &&
            shaderCompilerID != m_GLSL_LLVM_DXC &&
            shaderCompilerID != m_METAL_LLVM_DXC)
        {
            printf("error: trying to use an unsupported compiler on Mac.\n");
            return false;
        }
    #endif

    return validCompiler;
}

bool SEnviropment::GetShaderCompilerExecutable( const AZStd::string& shaderCompilerID, AZStd::string& shaderCompilerExecutable ) const
{
    auto it = m_ShaderCompilersMap.find(shaderCompilerID);
    if (it != m_ShaderCompilersMap.end())
    {
        shaderCompilerExecutable = it->second;
        return true;
    }
    else
    {
        return false;
    }
}

class CThreadData
{
    uint32_t        m_Counter;
    CCrySimpleSock* m_pSock;
public:
    CThreadData(uint32_t        Counter, CCrySimpleSock* pSock)
        : m_Counter(Counter)
        , m_pSock(pSock){}

    ~CThreadData(){delete m_pSock; }

    CCrySimpleSock* Socket(){return m_pSock; }
    uint32_t        ID() const{return m_Counter; }
};

//////////////////////////////////////////////////////////////////////////

bool CopyFileOnPlatform(const char* nameOfFileToCopy, const char* copiedFileName, bool failIfFileExists)
{
    if (AZ::IO::SystemFile::Exists(copiedFileName) && failIfFileExists)
    {
        AZ_Warning("CrySimpleServer", false, ("File to copy to, %s, already exists."), copiedFileName);
        return false;
    }

    AZ::IO::SystemFile fileToCopy;
    if (!fileToCopy.Open(nameOfFileToCopy, AZ::IO::SystemFile::SF_OPEN_READ_ONLY))
    {
        AZ_Warning("CrySimpleServer", false, ("Unable to open file: %s for copying."), nameOfFileToCopy);
        return false;
    }

    AZ::IO::SystemFile::SizeType fileLength = fileToCopy.Length();

    AZ::IO::SystemFile newFile;
    if (!newFile.Open(copiedFileName, AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY | AZ::IO::SystemFile::SF_OPEN_CREATE))
    {
        AZ_Warning("CrySimpleServer", false, ("Unable to open new file: %s for copying."), copiedFileName);
        return false;
    }

    char* fileContents = new char[fileLength];
    fileToCopy.Read(fileLength, fileContents);
    newFile.Write(fileContents, fileLength);
    delete[] fileContents;

    return true;
}

void MakeErrorVec(const std::string& errorText, tdDataVector& Vec)
{
    Vec.resize(errorText.size() + 1);
    for (size_t i = 0; i < errorText.size(); i++)
    {
        Vec[i] = errorText[i];
    }
    Vec[errorText.size()] = 0;

    // Compress output data
    tdDataVector rDataRaw;
    rDataRaw.swap(Vec);
    if (!CSTLHelper::Compress(rDataRaw, Vec))
    {
        Vec.resize(0);
    }
}

//////////////////////////////////////////////////////////////////////////
class CompileJob
    : public AZ::Job
{
public:
    CompileJob()
        : Job(autoDeleteJobWhenDone, nullptr) { }
    void SetThreadData(CThreadData* threadData) { m_pThreadData.reset(threadData); }
protected:
    void Process() override;
    bool ValidatePlatformAttributes(EProtocolVersion Version, const TiXmlElement* pElement);
private:
    std::unique_ptr<CThreadData> m_pThreadData;
};

void CompileJob::Process()
{
    std::vector<uint8_t> Vec;
    std::unique_ptr<CCrySimpleJob> Job;
    EProtocolVersion Version    =   EPV_V001;
    ECrySimpleJobState State    =   ECSJS_JOBNOTFOUND;
    try
    {
        if (m_pThreadData->Socket()->Recv(Vec))
        {
            std::string Request(reinterpret_cast<const char*>(&Vec[0]), Vec.size());
            TiXmlDocument ReqParsed("Request.xml");
            ReqParsed.Parse(Request.c_str());

            if (ReqParsed.Error())
            {
                CrySimple_ERROR("failed to parse request XML");
                return;
            }
            const TiXmlElement* pElement = ReqParsed.FirstChildElement();
            if (!pElement)
            {
                CrySimple_ERROR("failed to extract First Element of the request");
                return;
            }

            const char* pPing = pElement->Attribute("Identify");
            if (pPing)
            {
                const std::string& rData("ShaderCompilerServer");
                m_pThreadData->Socket()->Send(rData);
                return;
            }

            const char* pVersion        = pElement->Attribute("Version");
            const char* pPlatform       = pElement->Attribute("Platform");
            const char* pHardwareTarget = nullptr;

            //new request type?
            if (pVersion)
            {
                if (std::string(pVersion) == "2.3")
                {
                    Version = EPV_V0023;
                }
                else if (std::string(pVersion) == "2.2")
                {
                    Version =   EPV_V0022;
                }
                else if (std::string(pVersion) == "2.1")
                {
                    Version =   EPV_V0021;
                }
                else if (std::string(pVersion) == "2.0")
                {
                    Version =   EPV_V002;
                }
            }
            

            // If the job type is 'GetShaderList', then we dont need to perform a validation on the platform
            // attributes, since the command doesnt use them, and the incoming request will not have 'compiler' or 'language' 
            // attributes.
            const char* pJobType = pElement->Attribute("JobType");
            if ((!pJobType) || (azstricmp(pJobType,"GetShaderList")!=0))
            {
                if (!ValidatePlatformAttributes(Version, pElement))
                {
                    return;
                }
            }

            if (Version >= EPV_V002)
            {
                const std::string JobType(pJobType);

                if (Version >= EPV_V0023)
                {
                    pHardwareTarget = pElement->Attribute("HardwareTarget");
                }

                if (Version >= EPV_V0021)
                {
                    m_pThreadData->Socket()->WaitForShutDownEvent(true);
                }

#if defined(AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS)
    #if defined(TOOLS_SUPPORT_JASPER)
        #define AZ_RESTRICTED_SECTION CRYSIMPLESERVER_CPP_SECTION_1
#include AZ_RESTRICTED_FILE_EXPLICIT(CrySimpleServer_cpp, jasper)
    #endif
    #if defined(TOOLS_SUPPORT_PROVO)
        #define AZ_RESTRICTED_SECTION CRYSIMPLESERVER_CPP_SECTION_1
#include AZ_RESTRICTED_FILE_EXPLICIT(CrySimpleServer_cpp, provo)
    #endif
    #if defined(TOOLS_SUPPORT_SALEM)
        #define AZ_RESTRICTED_SECTION CRYSIMPLESERVER_CPP_SECTION_1
#include AZ_RESTRICTED_FILE_EXPLICIT(CrySimpleServer_cpp, salem)
    #endif
#endif

                if (pJobType)
                {
                    if (JobType == "RequestLine")
                    {
                        Job = std::make_unique<CCrySimpleJobRequest>(Version, m_pThreadData->Socket()->PeerIP());
                        Job->Execute(pElement);
                        State = Job->State();
                        Vec.resize(0);
                    }
                    else
                    if (JobType == "Compile")
                    {
                        Job = std::make_unique<CCrySimpleJobCompile2>(Version, m_pThreadData->Socket()->PeerIP(), &Vec);
                        Job->Execute(pElement);
                        State   =   Job->State();
                    }
                    else
                    if (JobType == "GetShaderList")
                    {
                        Job = std::make_unique<CCrySimpleJobGetShaderList>(m_pThreadData->Socket()->PeerIP(), &Vec);
                        Job->Execute(pElement);
                        State = Job->State();
                    }
                    else
                    {
                        printf("\nRequested unkown job %s\n", pJobType);
                    }
                }
                else
                {
                    printf("\nVersion 2.0 or higher but has no JobType tag\n");
                }
            }
            else
            {
                //legacy request
                Version =   EPV_V001;
                Job = std::make_unique<CCrySimpleJobCompile1>(m_pThreadData->Socket()->PeerIP(), &Vec);
                Job->Execute(pElement);
            }
            m_pThreadData->Socket()->Send(Vec, State, Version);

            if (Version >= EPV_V0021)
            {
                /*
                // wait until message has been succesfully delived before shutting down the connection
                if(!m_pThreadData->Socket()->RecvResult())
                {
                    printf("\nInvalid result from client\n");
                }
                */
            }
        }
    }
    catch (const ICryError* err)
    {
        CCrySimpleServer::IncrementExceptionCount();

        CRYSIMPLE_LOG("<Error> " + err->GetErrorName());

        std::string returnStr = err->GetErrorDetails(ICryError::OUTPUT_TTY);

        // Send error back
        MakeErrorVec(returnStr, Vec);

        if (Job.get())
        {
            State   =   Job->State();

            if (State == ECSJS_ERROR_COMPILE && SEnviropment::Instance().m_PrintErrors)
            {
                printf("\nXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n");
                printf("%s\n", err->GetErrorName().c_str());
                printf("%s\n", returnStr.c_str());
                printf("\nXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n\n");
            }
        }

        bool added = CCrySimpleErrorLog::Instance().Add((ICryError*)err);

        // error log hasn't taken ownership, delete this error.
        if (!added)
        {
            delete err;
        }

        m_pThreadData->Socket()->Send(Vec, State, Version);
    }
    --g_ConnectionCount;
}


bool CompileJob::ValidatePlatformAttributes(EProtocolVersion Version, const TiXmlElement* pElement)
{
    if (Version >= EPV_V0023)
    {
        const char* platform = pElement->Attribute("Platform"); // eg. PC, Mac...
        const char* compiler = pElement->Attribute("Compiler"); // key to shader compiler executable
        const char* language = pElement->Attribute("Language"); // eg. D3D11, GL4_1, GL3_1, METAL...

        if (!platform || !SEnviropment::Instance().IsPlatformValid(platform))
        {
            CrySimple_ERROR("invalid Platform attribute from request.");
            return false;
        }
        if (!compiler || !SEnviropment::Instance().IsShaderCompilerValid(compiler))
        {
            CrySimple_ERROR("invalid Compiler attribute from request.");
            return false;
        }
        if (!language || !SEnviropment::Instance().IsShaderLanguageValid(language))
        {
            CrySimple_ERROR("invalid Language attribute from request.");
            return false;
        }
    }
    else
    {
        // In older versions the attribute Platform was used differently depending on the JobType
        //   - JobType Compile: Platform is the shader language
        //   - JobType RequestLine: Platform is the shader list filename
        const char* platformLegacy = pElement->Attribute("Platform");

        // The only check we can do here is if the attribute exists. Each JobType will check it has a valid value.
        if (!platformLegacy)
        {
            CrySimple_ERROR("failed to extract required platform attribute from request.");
            return false;
        }
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
void TickThread()
{
    AZ::u64 t0 = AZStd::GetTimeUTCMilliSecond();

    while (true)
    {
        CrySimple_SECURE_START

        AZ::u64 t1 = AZStd::GetTimeUTCMilliSecond();
        if ((t1 < t0) || (t1 - t0 > 100))
        {
            t0 = t1;
            const int maxStringSize = 512;
            char str[maxStringSize] = { 0 };
            azsnprintf(str, maxStringSize, "Amazon Shader Compiler Server (%ld compile tasks | %ld open sockets | %ld exceptions)",
                CCrySimpleJobCompile::GlobalCompileTasks(), CCrySimpleSock::GetOpenSockets() + CSMTPMailer::GetOpenSockets(),
                CCrySimpleServer::GetExceptionCount());
#if defined(AZ_PLATFORM_WINDOWS)
            SetConsoleTitle(str);
#endif
        }

        const AZ::u64 T1    =   AZStd::GetTimeUTCMilliSecond();
        CCrySimpleErrorLog::Instance().Tick();
        CShaderList::Instance().Tick();
        CCrySimpleCache::Instance().ThreadFunc_SavePendingCacheEntries();
        const AZ::u64 T2    =   AZStd::GetTimeUTCMilliSecond();
        if (T2 - T1 < 100)
        {
            Sleep(static_cast<DWORD>(100 - T2 + T1));
        }

        CrySimple_SECURE_END
    }
}

//////////////////////////////////////////////////////////////////////////
void LoadCache()
{
    const std::string& cachePath = SEnviropment::Instance().m_CachePath;
    if (CCrySimpleCache::Instance().LoadCacheFile(cachePath + "Cache.dat"))
    {
        printf("Creating cache backup...\n");
        AZ::IO::SystemFile::Delete((cachePath + "Cache.bak2").c_str());
        printf("Move %s to %s\n", (cachePath + "Cache.bak").c_str(), (cachePath + "Cache.bak2").c_str());
        AZ::IO::SystemFile::Rename((cachePath + "Cache.bak").c_str(), (cachePath + "Cache.bak2").c_str());
        printf("Copy %s to %s\n", (cachePath + "Cache.dat").c_str(), (cachePath + "Cache.bak").c_str());
        CopyFileOnPlatform((cachePath + "Cache.dat").c_str(), (cachePath + "Cache.bak").c_str(), FALSE);
        printf("Cache backup done.\n");
    }
    else
    {
        // Restoring backup cache!
        printf("Cache file corrupted!!!\n");
        printf("Restoring backup cache...\n");
        AZ::IO::SystemFile::Delete((cachePath + "Cache.dat").c_str());
        printf("Copy %s to %s\n", (cachePath + "Cache.bak").c_str(), (cachePath + "Cache.dat").c_str());
        CopyFileOnPlatform((cachePath + "Cache.bak").c_str(), (cachePath + "Cache.dat").c_str(), FALSE);
        if (!CCrySimpleCache::Instance().LoadCacheFile(cachePath + "Cache.dat"))
        {
            // Backup file corrupted too!
            printf("Backup file corrupted too!!!\n");
            printf("Deleting cache completely\n");
            AZ::IO::SystemFile::Delete((cachePath + "Cache.dat").c_str());
        }
    }

    CCrySimpleCache::Instance().Finalize();
    printf("Ready\n");
}


//////////////////////////////////////////////////////////////////////////
CCrySimpleServer::CCrySimpleServer([[maybe_unused]] const char* pShaderModel, [[maybe_unused]] const char* pDst, [[maybe_unused]] const char* pSrc, [[maybe_unused]] const char* pEntryFunction)
    : m_pServerSocket(nullptr)
{
    Init();
}

CCrySimpleServer::CCrySimpleServer()
    : m_pServerSocket(nullptr)
{
    CrySimple_SECURE_START

    uint32_t Port = SEnviropment::Instance().m_port;

    m_pServerSocket =   new CCrySimpleSock(Port, SEnviropment::Instance().m_WhitelistAddresses);
    Init();
    m_pServerSocket->Listen();

    AZ::Job* tickThreadJob = AZ::CreateJobFunction(&TickThread, autoDeleteJobWhenDone);
    tickThreadJob->Start();

    uint32_t JobCounter = 0;
    while (1)
    {
        // New client message, receive new client socket connection.
        CCrySimpleSock* newClientSocket = m_pServerSocket->Accept();
        if(!newClientSocket)
        {
            continue;
        }
        
        // Thread Data for new job
        CThreadData* pData = new CThreadData(JobCounter++, newClientSocket);

        // Increase connection count and start new job.
        // NOTE: CompileJob will be auto deleted when done, deleting thread data and client socket as well.
        ++g_ConnectionCount;
        CompileJob* compileJob = new CompileJob();
        compileJob->SetThreadData(pData);
        compileJob->Start();

        bool printedMessage = false;
        while (g_ConnectionCount >= SEnviropment::Instance().m_MaxConnections)
        {
            if (!printedMessage)
            {
                logmessage("Waiting for a request to finish before accepting another connection...\n");
                printedMessage = true;
            }

            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(sleepTimeWhenWaiting));
        };
    }
    CrySimple_SECURE_END
}

bool GetBaseDirectory(AZStd::string& baseDir)
{
    char executableDir[AZ_MAX_PATH_LEN];
    if (AZ::Utils::GetExecutableDirectory(executableDir, AZ_MAX_PATH_LEN) == AZ::Utils::ExecutablePathResult::Success)
    {
        AZStd::string_view executableDirView(executableDir);
        if (executableDirView.size() > 1 && !executableDirView.ends_with(AZ_CORRECT_FILESYSTEM_SEPARATOR) && executableDirView.size() < AZStd::size(executableDir) - 1)
        {
            executableDir[executableDirView.size()] = AZ_CORRECT_FILESYSTEM_SEPARATOR;
            executableDir[executableDirView.size() + 1] = '\0';
            executableDirView = { executableDir, executableDirView.size() + 1 };
        }
        baseDir = AZStd::string(executableDir);

        return true;

    }
    else
    {
        return false;
    }
}

void NormalizePath(AZStd::string& pathToNormalize)
{
    AzFramework::StringFunc::Root::Normalize(pathToNormalize);
}

void NormalizePath(std::string& pathToNormalize)
{
    AZStd::string tempString = pathToNormalize.c_str();
    NormalizePath(tempString);
    pathToNormalize = tempString.c_str();
}

bool IsPathValid(const AZStd::string& path)
{
    // Calculating base directory every time.
    // It's slower than using a cached value, but safer.
    AZStd::string baseDir;
    if (GetBaseDirectory(baseDir))
    {
        AZStd::string basePath(AZ::IO::Path(baseDir).LexicallyNormal().Native());
        AZStd::string subPath(AZ::IO::Path(path).LexicallyNormal().Native());
        return strncmp(basePath.c_str(), subPath.c_str(), basePath.size()) == 0;
    }
    else
    {
        return false;
    }
}

bool IsPathValid(const std::string& path)
{
    const AZStd::string tempString = path.c_str();
    return IsPathValid(tempString);
}

void CCrySimpleServer::Init()
{
    char executableDir[AZ_MAX_PATH_LEN];
    AZ::Utils::GetExecutableDirectory(executableDir, AZ_MAX_PATH_LEN);
    AZStd::string_view executableDirView(executableDir);
    if (executableDirView.size() > 1 && !executableDirView.ends_with(AZ_CORRECT_FILESYSTEM_SEPARATOR) && executableDirView.size() < AZStd::size(executableDir) - 1)
    {
        executableDir[executableDirView.size()] = AZ_CORRECT_FILESYSTEM_SEPARATOR;
        executableDir[executableDirView.size() + 1] = '\0';
        executableDirView = { executableDir, executableDirView.size() + 1 };
    }
    SEnviropment::Instance().m_Root = std::string(executableDir);

    AZStd::string baseDir;
    GetBaseDirectory(baseDir);
    SEnviropment::Instance().m_CompilerPath  = baseDir.c_str();
    SEnviropment::Instance().m_CompilerPath += "/Compiler/";

    SEnviropment::Instance().m_CachePath = SEnviropment::Instance().m_Root + "Cache/";
    
    if (SEnviropment::Instance().m_TempPath.empty())
    {
        SEnviropment::Instance().m_TempPath = SEnviropment::Instance().m_Root + "Temp/";
    }
    if (SEnviropment::Instance().m_ErrorPath.empty())
    {
        SEnviropment::Instance().m_ErrorPath = SEnviropment::Instance().m_Root + "Error/";
    }
    if (SEnviropment::Instance().m_ShaderPath.empty())
    {
        SEnviropment::Instance().m_ShaderPath = SEnviropment::Instance().m_Root + "Shaders/";
    }

    NormalizePath(SEnviropment::Instance().m_Root);
    NormalizePath(SEnviropment::Instance().m_CompilerPath);
    NormalizePath(SEnviropment::Instance().m_CachePath);
    NormalizePath(SEnviropment::Instance().m_ErrorPath);
    NormalizePath(SEnviropment::Instance().m_TempPath);
    NormalizePath(SEnviropment::Instance().m_ShaderPath);

    AZ::IO::SystemFile::CreateDir(SEnviropment::Instance().m_ErrorPath.c_str());
    AZ::IO::SystemFile::CreateDir(SEnviropment::Instance().m_TempPath.c_str());
    AZ::IO::SystemFile::CreateDir(SEnviropment::Instance().m_CachePath.c_str());
    AZ::IO::SystemFile::CreateDir(SEnviropment::Instance().m_ShaderPath.c_str());

    if (SEnviropment::Instance().m_Caching)
    {
        AZ::Job* loadCacheJob = AZ::CreateJobFunction(&LoadCache, autoDeleteJobWhenDone);
        loadCacheJob->Start();
    }
    else
    {
        printf("\nNO CACHING, disabled by config\n");
    }
}

void CCrySimpleServer::IncrementExceptionCount()
{
    ++ms_ExceptionCount;
}
