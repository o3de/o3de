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

#ifndef __CRYSIMPLESERVER__
#define __CRYSIMPLESERVER__

#include <Core/Common.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/parallel/atomic.h>
#include <string>
#include <vector>

extern bool g_Success;

bool GetExecutableDirectory(AZStd::string& executableDir);
bool GetBaseDirectory(AZStd::string& baseDir);

void NormalizePath(AZStd::string& pathToNormalize);
void NormalizePath(std::string& pathToNormalize);

bool IsPathValid(const AZStd::string& path);
bool IsPathValid(const std::string& path);

namespace AZ {
    class JobManager;
}

class CCrySimpleSock;

class SEnviropment
{
public:
    std::string     m_Root;
    std::string     m_CompilerPath;
    std::string     m_CachePath;
    std::string     m_TempPath;
    std::string     m_ErrorPath;
    std::string     m_ShaderPath;

    std::string   m_FailEMail;
    std::string   m_MailServer;
    uint32_t      m_port;
    uint32_t      m_MailInterval; // seconds since last error to flush error mails

    bool                    m_Caching;
    bool                    m_PrintErrors = 1;
    bool                    m_PrintWarnings;
    bool                    m_PrintCommands;
    bool          m_PrintListUpdates;
    bool          m_DedupeErrors;
    bool          m_DumpShaders = false;
    bool          m_RunAsRoot = false;
    std::string     m_FallbackServer;
    int32_t                 m_FallbackTreshold;
    int32_t       m_MaxConnections;
    std::vector<AZStd::string> m_WhitelistAddresses;
    
    // Shader Compilers ID
    static const char* m_Orbis_DXC;
    static const char* m_Jasper_FXC;
    static const char* m_D3D11_FXC;
    static const char* m_GLSL_HLSLcc;
    static const char* m_METAL_HLSLcc;
    static const char* m_GLSL_LLVM_DXC;
    static const char* m_METAL_LLVM_DXC;

    int m_hardwareTarget = -1;

    static void Create();
    static void Destroy();
    static SEnviropment& Instance();
    
    void InitializePlatformAttributes();
    
    bool IsPlatformValid( const AZStd::string& platform ) const;
    bool IsShaderLanguageValid( const AZStd::string& shaderLanguage ) const;
    bool IsShaderCompilerValid( const AZStd::string& shaderCompilerID ) const;
    
    bool GetShaderCompilerExecutable( const AZStd::string& shaderCompilerID, AZStd::string& shaderCompilerExecutable ) const;
    
private:
    SEnviropment() = default;
    
    // The single instance of the environment
    static SEnviropment* m_instance;
    
    // Platforms
    AZStd::unordered_set<AZStd::string> m_Platforms;
    
    // Shader Languages
    AZStd::unordered_set<AZStd::string> m_ShaderLanguages;
    
    // Shader Compilers ID to Executable map
    AZStd::unordered_map<AZStd::string, AZStd::string> m_ShaderCompilersMap;
};

class CCrySimpleServer
{
    static AZStd::atomic_long             ms_ExceptionCount;
    CCrySimpleSock*                     m_pServerSocket;
    void            Init();
public:
    CCrySimpleServer(const char* pShaderModel, const char* pDst, const char* pSrc, const char* pEntryFunction);
    CCrySimpleServer();


    static long          GetExceptionCount() { return ms_ExceptionCount; }
    static void                             IncrementExceptionCount();
};

#endif
