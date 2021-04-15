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

#include "RenderDll_precompiled.h"
#include "RemoteCompiler.h"
#include "../RenderCapabilities.h"

#include <unordered_map>

#include <AzCore/Socket/AzSocket.h>
#include <AzCore/NativeUI/NativeUIRequests.h>
#include <AzCore/PlatformId/PlatformId.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzFramework/Network/SocketConnection.h>
#include <AzFramework/Asset/AssetSystemTypes.h>

#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define REMOTECOMPILER_CPP_SECTION_1 1
#define REMOTECOMPILER_CPP_SECTION_2 2
#define REMOTECOMPILER_CPP_SECTION_3 3
#define REMOTECOMPILER_CPP_SECTION_4 4
#endif

#if defined(AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS)
#undef AZ_RESTRICTED_SECTION
#define REMOTECOMPILER_CPP_SECTION_2 2
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION REMOTECOMPILER_CPP_SECTION_3
    #include AZ_RESTRICTED_FILE(RemoteCompiler_cpp)
#endif

namespace NRemoteCompiler
{
    //Debugging the network connection problems can be tricky without verbose logging, more verbose
    //than anyone would like on by default. This is a special situation in which if logging is off by default
    //we would normally use a define or cvar to turn verbose logging on. However you really can't do
    //that here because of networking, you can't easily break or open the console and enter the
    //cvar command to turn verbose logging on fast enough to catch logs around the error condition without
    //causing different code paths due to breaking. So this is an automatic verbose logging var that is off
    //by default and turns on for a limited number of log lines after the error is logged then automaticly
    //turns itself off so we are not spammed for to long so as to not adversely affect the rest of the systems.
    int s_verboselogging = 0;
    bool VerboseLogging(bool start = false)
    {
        if(start)
        {
            s_verboselogging = 100;
            return true;
        }

        if(s_verboselogging > 0)
        {
            s_verboselogging--;
            return true;
        }
        return false;
    }

    // Note:  Cry's original source uses little endian as their internal communication endianness
    // so this new code will do the same.


    // RemoteProxyState: store the current state of things required to communicate to the remote server
    // via the Engine Connection, so that its outside this interface and protected from the details of it.
    class RemoteProxyState
    {
    public:
        unsigned int m_remoteRequestCRC;
        unsigned int m_remoteResponseCRC;
        unsigned int m_nextAssignedToken;
        bool m_unitTestMode;
        bool m_engineConnectionCallbackInstalled; // lazy-install it.

        typedef AZStd::function<void(const void* payload, unsigned int payloadSize)> TResponseCallback;

        RemoteProxyState()
        {
            m_engineConnectionCallbackInstalled = false;
            m_unitTestMode = false;
            m_remoteRequestCRC = AZ_CRC("ShaderCompilerProxyRequest");
            m_remoteResponseCRC = AZ_CRC("ShaderCompilerProxyResponse");
            m_nextAssignedToken = 0;
        }

        void SetUnitTestMode(bool newMode)
        {
            m_unitTestMode = newMode;
        }

        bool SubmitRequestAndBlockForResponse(std::vector<uint8>& inout)
        {
            unsigned int chosenToken = m_nextAssignedToken++;
            AzFramework::SocketConnection* engineConnection = AzFramework::SocketConnection::GetInstance();

            if (!m_unitTestMode)
            {
                // if we're not in unit test mode, we NEED an engine connection
                if (!engineConnection)
                {
                    AZ_Error("RemoteCompiler", false, "CShaderSrv::Compile: no engine connection present, but r_AssetProcessorShaderCompiler is set in config!\n");
                    VerboseLogging(true);
                    return false;
                }

                // install the callback the first time its needed:
                if (!m_engineConnectionCallbackInstalled)
                {
                    // (AddTypeCallback is assumed to be thread safe.)

                    m_engineConnectionCallbackInstalled = true;
                    engineConnection->AddMessageHandler(m_remoteResponseCRC, std::bind(&RemoteProxyState::OnReceiveRemoteResponse, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
                }
            }

            // the plan:  Create a wait event
            // then raise the event when we get a response from the server
            // then wait for the event to be raised.

            CryEvent waitEvent;

            {
                CryAutoLock<CryMutex> protector(m_mapProtector); // scoped lock on the map

                // now create an anonymous function that will copy the data and set the waitevent when the callback is triggered:
                m_responsesAwaitingCallback[chosenToken] = [&inout, &waitEvent](const void* payload, unsigned int payloadSize)
                    {
                        inout.resize(payloadSize);
                        memcpy(inout.data(), payload, payloadSize);
                        waitEvent.Set();
                    };
            }

            if (m_unitTestMode)
            {
                // if we're unit testing, there WILL BE NO ENGINE CONNECTION
                // we must respond as if we're the engine connection
                // you can assume the payload has already been unit tested to conform.
                // instead, just write the data to the buffer as expected:

                std::vector<uint8> newData;

                if (memcmp(inout.data(), "empty", 5) == 0)
                {
                    // unit test to send empty
                }
                else if (memcmp(inout.data(), "incomplete", 10) == 0)
                {
                    // unit test to send incomplete data.
                    newData.push_back('x');
                }
                else if (memcmp(inout.data(), "corrupt", 7) == 0)
                {
                    // unit test to send corrupt data
                    std::string testString("CDCDCDCDCDCDCDCD");
                    newData.assign(testString.begin(), testString.end());
                }
                else if ((memcmp(inout.data(), "compile_failure", 15) == 0) || (memcmp(inout.data(), "success", 7) == 0))
                {
                    // simulate compile failure response
                    // [payload length 4 bytes (LITTLE ENDIAN) ] [status 1 byte] [payload]
                    // and payload consists of
                    //   [ uncompressed size (NETWORK BYTE ORDER) ] [payload (compressed)]
                    bool isFail = (memcmp(inout.data(), "compile_failure", 15) == 0);
                    std::string failreason("decompressed_plaintext");

                    size_t uncompressed_size = failreason.size();
                    size_t compressed_size = uncompressed_size;
                    std::vector<char> compressedData;
                    compressed_size = uncompressed_size * 2;
                    compressedData.resize(compressed_size);

                    gEnv->pSystem->CompressDataBlock(failreason.data(), failreason.size(), compressedData.data(), compressed_size);
                    compressedData.resize(compressed_size);

                    // first four bytes are payload size.
                    // firth byte is status
                    unsigned int payloadSize = 4 + compressed_size;
                    newData.resize(4 + 1 + payloadSize);
                    uint8 status_code = isFail ? 0x05 : 0x01; // 5 is fail, 1 is ok

                    unsigned int uncompressed_size_le = uncompressed_size;
                    SwapEndian(uncompressed_size_le);

                    memcpy(newData.data(), &payloadSize, 4);
                    memcpy(newData.data() + 4, &status_code, 0x01); // 0x05 = error compiling

                    memcpy(newData.data() + 4 + 1, &uncompressed_size_le, 4);
                    memcpy(newData.data() + 4 + 1 + 4, compressedData.data(), compressedData.size());
                }
                else
                {
                    newData.clear();
                }

                // place the messageID at the end:
                newData.resize(newData.size() + 4); // for the messageID
                unsigned int swappedToken = chosenToken;
                SwapEndian(swappedToken);
                memcpy(newData.data() + newData.size() - 4, &swappedToken, 4);

                OnReceiveRemoteResponse(m_remoteResponseCRC, AzFramework::AssetSystem::DEFAULT_SERIAL, newData.data(), newData.size());
            }
            else // note:  This else is inside the endif so that it only takes the second branch if UNIT TEST MODE is present.
            {
                // append the messageID:
                inout.resize(inout.size() + 4); // for the messageID
                unsigned int swappedToken = chosenToken;
                SwapEndian(swappedToken);
                memcpy(inout.data() + inout.size() - 4, &swappedToken, 4);

                if (!engineConnection->SendMsg(m_remoteRequestCRC, inout.data(), inout.size()))
                {
                    AZ_Error("RemoteCompiler", false, "CShaderSrv::SubmitRequestAndBlockForResponse() : unable to send via engine connection, but r_AssetProcessorShaderCompiler is set in config!\n");
                    VerboseLogging(true);
                    return false;
                }
            }

            if (!waitEvent.Wait(10000))
            {
                // failure to get response!
                AZ_Error("RemoteCompiler", false, "CShaderSrv::SubmitRequestAndBlockForResponse() : no response received!\n");
                VerboseLogging(true);
                CryAutoLock<CryMutex> protector(m_mapProtector);
                m_responsesAwaitingCallback.erase(chosenToken);

                if(!gEnv->IsInToolMode())
                {
                    EBUS_EVENT(AZ::NativeUI::NativeUIRequestBus, DisplayOkDialog, "Remote Shader Compiler", "Unable to connect to Remote Shader Compiler", false);
                }
                return false;
            }

            // wait succeeded! We got a response!  Unblock and return!
            return true;
        }

    private:

        CryMutex m_mapProtector;
        std::unordered_map<unsigned int, TResponseCallback> m_responsesAwaitingCallback;

        void OnReceiveRemoteResponse([[maybe_unused]] unsigned int messageID, unsigned int /*serial*/, const void* payload, unsigned int payloadSize)
        {
            // peel back to inner payload:
            if (payloadSize < 4)
            {
                // indicate error!
                AZ_Error("RemoteCompiler", false, " CShaderSrv::OnReceiveRemoteREsponse() : truncated message from shader compiler proxy");
                VerboseLogging(true);
                return;
            }

            // last four bytes are expected to be the response ID
            const uint8* payload_start = reinterpret_cast<const uint8*>(payload);
            const uint8* end_payload = payload_start + payloadSize;
            const uint8* messageIDPtr = end_payload - 4;

            unsigned int responseId = *reinterpret_cast<const unsigned int*>(messageIDPtr);
            SwapEndian(responseId);

            CryAutoLock<CryMutex> protector(m_mapProtector);
            auto callbackToCall = m_responsesAwaitingCallback.find(responseId);
            if (callbackToCall == m_responsesAwaitingCallback.end())
            {
                AZ_Error("RemoteCompiler", false, "CShaderSrv::OnReceiveRemoteResponse() : Unexpected response from shader compiler proxy.");
                VerboseLogging(true);
                return;
            }
            // give only the inner payload back to the callee!

            callbackToCall->second(payload_start, payloadSize - sizeof(unsigned int));
            m_responsesAwaitingCallback.erase(callbackToCall);
        }
    };
    CShaderSrv::CShaderSrv()
    {
        m_unitTestMode = false;
        Init();
    }

    void CShaderSrv::Init()
    {
        static RemoteProxyState proxyState;
        m_remoteState = &proxyState;

        int result = AZ::AzSock::Startup();
        if (AZ::AzSock::SocketErrorOccured(result))
        {
            AZ_Error("RemoteCompiler", false, "CShaderSrv::Init() : Could not init root socket\n");
            VerboseLogging(true);
            return;
        }

        m_RequestLineRootFolder = "";

        AZ::IO::FixedMaxPathString projectUserPath;
        if (auto settingsRegistry{ AZ::SettingsRegistry::Get() }; settingsRegistry != nullptr)
        {
            settingsRegistry->Get(projectUserPath, AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectUserPath);
        }

        if (!projectUserPath.empty())
        {
            m_RequestLineRootFolder.assign(projectUserPath.c_str(), projectUserPath.size());
        }

        if (m_RequestLineRootFolder.empty())
        {
            AZ_Error("RemoteCompiler", false, "CShaderSrv::Init() : Game folder has not been specified\n");
            VerboseLogging(true);
        }
    }

    CShaderSrv& CShaderSrv::Instance()
    {
        static CShaderSrv g_ShaderSrv;
        return g_ShaderSrv;
    }
    
    EShaderCompiler CShaderSrv::GetShaderCompiler() const
    {
        EShaderCompiler shaderCompiler = eSC_Unknown;

        EShaderLanguage shaderLanguage = GetShaderLanguage();
        switch (shaderLanguage)
        {
        case eSL_Orbis:
            shaderCompiler = eSC_Orbis_DXC;
            break;

        case eSL_Jasper:
            shaderCompiler = eSC_Jasper_FXC;
            break;

        case eSL_D3D11:
            shaderCompiler = eSC_D3D11_FXC;
            break;

        case eSL_GL4_1:
        case eSL_GL4_4:
        case eSL_GLES3_0:
        case eSL_GLES3_1:
            shaderCompiler = gRenDev->m_cEF.HasStaticFlag(HWSST_LLVM_DIRECTX_SHADER_COMPILER) ? eSC_GLSL_LLVM_DXC : eSC_GLSL_HLSLcc;
            break;

        case eSL_METAL:
            shaderCompiler = gRenDev->m_cEF.HasStaticFlag(HWSST_LLVM_DIRECTX_SHADER_COMPILER) ? eSC_METAL_LLVM_DXC : eSC_METAL_HLSLcc;
            break;
        }

        return shaderCompiler;
    }

    const char *CShaderSrv::GetShaderCompilerName() const
    {
        // NOTE: These strings are used in the CrySCompilerServer tool as IDs.
        static const char *shaderCompilerNames[eSC_MAX] =
        {
            "Unknown",
            "Orbis_DXC",
            "D3D11_FXC",
            "GLSL_HLSLcc",
            "METAL_HLSLcc",
            "GLSL_LLVM_DXC",
            "METAL_LLVM_DXC",
            "Jasper_FXC"
        };

        EShaderCompiler shaderCompiler = GetShaderCompiler();
        return shaderCompilerNames[shaderCompiler];
    }

    const char* CShaderSrv::GetPlatformName() const
    {
        const char *platformName = "Unknown";

        switch (CParserBin::m_targetPlatform)
        {
        case AZ::PlatformID::PLATFORM_WINDOWS_64:
            platformName = "PC";
            break;
        case AZ::PlatformID::PLATFORM_ANDROID_64:
            platformName = "Android";
            break;
        case AZ::PlatformID::PLATFORM_APPLE_OSX:
            platformName = "Mac";
            break;
        case AZ::PlatformID::PLATFORM_APPLE_IOS:
            platformName = "iOS";
            break;
        case AZ::PlatformID::PLATFORM_LINUX_64:
            platformName = "Linux";
            break;
#if defined(AZ_EXPAND_FOR_RESTRICTED_PLATFORM) || defined(AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS)
#define AZ_RESTRICTED_PLATFORM_EXPANSION(CodeName, CODENAME, codename, PrivateName, PRIVATENAME, privatename, PublicName, PUBLICNAME, publicname, PublicAuxName1, PublicAuxName2, PublicAuxName3)\
        case AZ::PlatformID::PLATFORM_##PUBLICNAME: \
            platformName = #PrivateName;\
            break;
#if defined(AZ_EXPAND_FOR_RESTRICTED_PLATFORM)
            AZ_EXPAND_FOR_RESTRICTED_PLATFORM
#else
            AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS
#endif
#undef AZ_RESTRICTED_PLATFORM_EXPANSION
#endif
        default:
            AZ_Assert(false, "Unknown shader platform");
            break;
        }

        return platformName;
    }

    AZStd::string CShaderSrv::GetShaderCompilerFlags([[maybe_unused]] EHWShaderClass eClass, [[maybe_unused]] UPipelineState pipelineState, [[maybe_unused]] uint32 MDVMask) const
    {
        AZStd::string flags = "";

        EShaderCompiler shaderCompiler = GetShaderCompiler();
        switch (shaderCompiler)
        {
        // ----------------------------------------
        case eSC_Orbis_DXC:
        {
            flags = "%s %s \"%s\" \"%s\"";

            #if defined(AZ_RESTRICTED_PLATFORM)
                #define AZ_RESTRICTED_SECTION REMOTECOMPILER_CPP_SECTION_1
                #include AZ_RESTRICTED_FILE(RemoteCompiler_cpp)
            #endif

            #if defined(AZ_RESTRICTED_PLATFORM)
                #define AZ_RESTRICTED_SECTION REMOTECOMPILER_CPP_SECTION_2
                #include AZ_RESTRICTED_FILE(RemoteCompiler_cpp)
            #endif

            #if defined(TOOLS_SUPPORT_PROVO)
                #define AZ_RESTRICTED_SECTION REMOTECOMPILER_CPP_SECTION_2
                #include AZ_RESTRICTED_FILE_EXPLICIT(Shaders/RemoteCompiler_cpp, provo)
            #endif
            #if defined(TOOLS_SUPPORT_JASPER)
                #define AZ_RESTRICTED_SECTION REMOTECOMPILER_CPP_SECTION_2
                #include AZ_RESTRICTED_FILE_EXPLICIT(Shaders/RemoteCompiler_cpp, jasper)
            #endif
            #if defined(TOOLS_SUPPORT_SALEM)
                #define AZ_RESTRICTED_SECTION REMOTECOMPILER_CPP_SECTION_2
                #include AZ_RESTRICTED_FILE_EXPLICIT(Shaders/RemoteCompiler_cpp, salem)
            #endif
        }
        break;

        // ----------------------------------------
        case eSC_Jasper_FXC:
        case eSC_D3D11_FXC:
        {
            const char* extraFlags = "";

            const char* debugFlags = "";
            if IsCVarConstAccess(constexpr) (CRenderer::CV_r_shadersdebug == 3)
            {
                debugFlags = " /Zi /Od";  // Debug information
            }
            else if IsCVarConstAccess(constexpr) (CRenderer::CV_r_shadersdebug == 4)
            {
                debugFlags = " /Zi /O3";  // Debug information, optimized shaders
            }

            flags = AZStd::move(AZStd::string::format("/nologo /E %%s /T %%s /Zpr /Gec %s %s /Fo \"%%s\" \"%%s\"", extraFlags, debugFlags));
        }
        break;

        // ----------------------------------------
        case eSC_GLSL_HLSLcc:
        {
            // Translate flags for HLSLCrossCompiler compiler. All flags come from 'Code\Tools\HLSLCrossCompiler\include\hlslcc.h'
            unsigned int translateFlags =
                0x1 |    // Each constant buffer will have its own uniform block
                0x100 |  // Invert clip space position Y
                0x200 |  // Convert clip sapce position Z
                0x400 |  // Avoid resource bindings and locations
                0x800 |  // Do not use an array for temporary registers
                0x8000 | // Do not add GLSL version macro
                0x10000; // Avoid shader image load store extension

            EShaderLanguage shaderLanguage = GetShaderLanguage();
            switch (shaderLanguage)
            {
            case eSL_GL4_1:
            case eSL_GL4_4:
            {
                const char* glVer = (shaderLanguage == eSL_GL4_1) ? "410" : "440";
                flags = AZStd::move(AZStd::string::format("-lang=%s -flags=%u -fxc=\"%%s /nologo /E %%s /T %%s /Zpr /Gec /Fo\" -out=\"%%s\" -in=\"%%s\"", glVer, translateFlags));
            }
            break;

            case eSL_GLES3_0:
            {
                translateFlags |=
                    0x20000 | // Syntactic workarounds for driver bugs found in Qualcomm devices running OpenGL ES 3.0
                    0x40000;  // Add half support

                flags = AZStd::move(AZStd::string::format("-lang=es300 -flags=%u -fxc=\"%%s /nologo /E %%s /T %%s /Zpr /Gec /Fo\" -out=\"%%s\" -in=\"%%s\"", translateFlags));
            }
            break;

            case eSL_GLES3_1:
            {
                translateFlags |=
                    0x40000;  // Add half support

                flags = AZStd::move(AZStd::string::format("-lang=es310 -flags=%u -fxc=\"%%s /nologo /E %%s /T %%s /Zpr /Gec /Fo\" -out=\"%%s\" -in=\"%%s\"", translateFlags));
            }
            break;

            default:
                AZ_Assert(false, "Non-GLSL shader language used with the GLSL HLSLcc compiler.");
                break;
            }
        }
        break;

        // ----------------------------------------
        case eSC_METAL_HLSLcc:
        {
            // Translate flags for HLSLCrossCompilerMETAL compiler. All flags come from 'Code\Tools\HLSLCrossCompilerMETAL\include\hlslcc.h'
            unsigned int translateFlags =
                //0x40000 |// Add half support
                0x1 |    // Each constant buffer will have its own uniform block
                0x100 |  // Declare inputs and outputs with their semantic name appended
                0x200 |  // Combine texture/sampler pairs used together into samplers named "texturename_X_samplernamC"
                0x400 |  // Attribute and uniform explicit location qualifiers are disabled (even if the language version supports that)
                0x800;   // Global uniforms are not stored in a struct

            flags = AZStd::move(AZStd::string::format("-lang=metal -flags=%u -fxc=\"%%s /nologo /E %%s /T %%s /Zpr /Gec /Fo\" -out=\"%%s\" -in=\"%%s\"", translateFlags));
        }
        break;

        // ----------------------------------------
        case eSC_GLSL_LLVM_DXC:
        {
            // Translate flags for DirectXShaderCompiler GLSL compiler. All flags come from 'DirectXShaderCompiler\src\tools\clang\tools\dxcGL\HLSLCrossCompiler\include\hlslcc.h'
            unsigned int translateFlags =
                0x1 |    // Each constant buffer will have its own uniform block
                0x100 |  // Invert clip space position Y
                0x200 |  // Convert clip sapce position Z
                0x400 |  // Avoid resource bindings and locations
                0x800 |  // Do not use an array for temporary registers
                0x8000 | // Do not add GLSL version macro
                0x10000| // Avoid shader image load store extension
                0x20000; // Declare dynamically indexed constant buffers as an array of floats

            EShaderLanguage shaderLanguage = GetShaderLanguage();
            switch (shaderLanguage)
            {
            case eSL_GL4_1:
            case eSL_GL4_4:
            {
                const char* glVer = (shaderLanguage == eSL_GL4_1) ? "410" : "440";
                flags = AZStd::move(AZStd::string::format("-translate_flags %u -translate %s -E %%s -T %%s -Zpr -not_use_legacy_cbuf_load -Gfa -Fo \"%%s\" \"%%s\"", translateFlags, glVer));
            }
            break;

            case eSL_GLES3_0:
            case eSL_GLES3_1:
            {
                const char* glesVer = (shaderLanguage == eSL_GLES3_0) ? "es300" : "es310";
                flags = AZStd::move(AZStd::string::format("-translate_flags %u -translate %s -E %%s -T %%s -Zpr -not_use_legacy_cbuf_load -Gfa -Fo \"%%s\" \"%%s\"", translateFlags, glesVer));
            }
            break;

            default:
                AZ_Assert(false, "Non-GLSL shader language used with the LLVM DXC compiler.");
                break;
            }
        }
        break;

        // ----------------------------------------
        case eSC_METAL_LLVM_DXC:
        {
            // Translate flags for DirectXShaderCompiler Metal compiler. All flags come from 'DirectXShaderCompiler\src\tools\clang\tools\dxcMetal\HLSLCrossCompilerMETAL\include\hlslcc.h'
            unsigned int translateFlags =
                0x1 |    // Each constant buffer will have its own uniform block
                0x100 |  // Declare inputs and outputs with their semantic name appended
                0x200 |  // Combine texture/sampler pairs used together into samplers named "texturename_X_samplername"
                0x400 |  // Attribute and uniform explicit location qualifiers are disabled (even if the language version supports that)
                0x800 |  // Global uniforms are not stored in a struct
                0x2000;  // Do not use an array for temporary registers

            #if defined(AZ_PLATFORM_MAC)
                translateFlags |= 0x1000; // Declare dynamically indexed constant buffers as an array of floats
            #endif

            flags = AZStd::move(AZStd::string::format("-translate_flags %u -translate metal -E %%s -T %%s -Zpr -not_use_legacy_cbuf_load -Gfa -Fo \"%%s\" \"%%s\"", translateFlags));
        }
        break;

        // ----------------------------------------
        case eSC_Unknown:
        default:
            AZ_Assert(false, "Unknown shader compiler");
            break;
        }

        return flags;
    }

    string CShaderSrv::CreateXMLNode(const string& rTag, const string& rValue)   const
    {
        string Tag = rTag;
        Tag += "=\"";
        Tag += rValue;
        Tag += "\" ";
        return Tag;
    }

    string CShaderSrv::TransformToXML(const string& rIn)    const
    {
        string Out;
        for (size_t a = 0, Size = rIn.size(); a < Size; a++)
        {
            const char C = rIn.c_str()[a];
            if (C == '&')
            {
                Out += "&amp;";
            }
            else
            if (C == '<')
            {
                Out += "&lt;";
            }
            else
            if (C == '>')
            {
                Out += "&gt;";
            }
            else
            if (C == '\"')
            {
                Out += "&quot;";
            }
            else
            if (C == '\'')
            {
                Out += "&apos;";
            }
            else
            {
                Out += C;
            }
        }
        return Out;
    }

    bool CShaderSrv::CreateRequest(std::vector<uint8>& rVec,
        std::vector<std::pair<string, string> >& rNodes) const
    {
        string Request = "<?xml version=\"1.0\"?><Compile ";
        Request +=  CreateXMLNode("Version", TransformToXML("2.3"));
        for (size_t a = 0; a < rNodes.size(); a++)
        {
            Request +=  CreateXMLNode(rNodes[a].first, TransformToXML(rNodes[a].second));
        }

        Request +=  " />";
        rVec    =   std::vector<uint8>(Request.c_str(), &Request.c_str()[Request.size() + 1]);
        return true;
    }

    bool CShaderSrv::RequestLine(const SCacheCombination& cmb, const string& rLine) const
    {
        const string List(string(GetShaderLanguageName()) + "/" + cmb.Name.c_str() + "ShaderList.txt");
        return RequestLine(List, rLine);
    }

    bool CShaderSrv::CommitPLCombinations(std::vector<SCacheCombination>&   rVec)
    {
        const uint32 STEPSIZE = 32;
#if defined(AZ_ENABLE_TRACING)
        float T0 = iTimer->GetAsyncCurTime();
#endif
        for (uint32 i = 0; i < rVec.size(); i += STEPSIZE)
        {
            string Line;
            string levelRequest;

            levelRequest.Format("<%d>%s", rVec[i].nCount, rVec[i].CacheName.c_str());
            Line = levelRequest;
            for (uint32 j = 1; j < STEPSIZE && i + j < rVec.size(); j++)
            {
                Line += string(";");
                levelRequest.Format("<%d>%s", rVec[i + j].nCount, rVec[i + j].CacheName.c_str());
                Line += levelRequest;
            }
            if (!RequestLine(rVec[i], Line))
            {
                return false;
            }
        }
#if defined(AZ_ENABLE_TRACING)
        float T1    =   iTimer->GetAsyncCurTime();
#endif
        if(VerboseLogging())
        {
            AZ_TracePrintf("RemoteCompiler", "CShaderSrv::CommitPLCombinations() : %3.3f to commit %" PRISIZE_T " Combinations\n", T1 - T0, rVec.size());
        }

        return true;
    }

    EServerError CShaderSrv::Compile(std::vector<uint8>& rVec,
        const char* pProfile,
        const char* pProgram,
        const char* pEntry,
        const char* pCompileFlags,
        const char* pIdent) const
    {
        std::vector<uint8>  CompileData;
        std::vector<std::pair<string, string> > Nodes;

        Nodes.push_back(std::pair<string, string>(string("JobType"), string("Compile")));
        Nodes.push_back(std::pair<string, string>(string("Profile"), string(pProfile)));
        Nodes.push_back(std::pair<string, string>(string("Program"), string(pProgram)));
        Nodes.push_back(std::pair<string, string>(string("Entry"), string(pEntry)));
        Nodes.push_back(std::pair<string, string>(string("CompileFlags"), string(pCompileFlags)));

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION REMOTECOMPILER_CPP_SECTION_4
    #include AZ_RESTRICTED_FILE(RemoteCompiler_cpp)
#endif

        // Any fields coming after "HashStop" will not contribute to the hash calculated on the Remote Shader Compiler Server for its local cache.
        Nodes.push_back(std::pair<string, string>(string("HashStop"), string("1")));
        Nodes.push_back(std::pair<string, string>(string("ShaderRequest"), string(pIdent)));
        Nodes.push_back(std::pair<string, string>(string("Project"), string(m_RequestLineRootFolder.c_str())));
        Nodes.push_back(std::pair<string, string>(string("Platform"), string(GetPlatformName())));
        Nodes.push_back(std::pair<string, string>(string("Compiler"), string(GetShaderCompilerName())));
        Nodes.push_back(std::pair<string, string>(string("Language"), string(GetShaderLanguageName())));

        if (gRenDev->CV_r_ShaderEmailTags && gRenDev->CV_r_ShaderEmailTags->GetString() &&
            strlen(gRenDev->CV_r_ShaderEmailTags->GetString()) > 0)
        {
            Nodes.push_back(std::pair<string, string>(string("Tags"), string(gRenDev->CV_r_ShaderEmailTags->GetString())));
        }

        if (gRenDev->CV_r_ShaderEmailCCs && gRenDev->CV_r_ShaderEmailCCs->GetString() &&
            strlen(gRenDev->CV_r_ShaderEmailCCs->GetString()) > 0)
        {
            Nodes.push_back(std::pair<string, string>(string("EmailCCs"), string(gRenDev->CV_r_ShaderEmailCCs->GetString())));
        }

        if (gRenDev->CV_r_ShaderCompilerDontCache)
        {
            Nodes.push_back(std::pair<string, string>(string("Caching"), string("0")));
        }

        //Nodes.push_back(std::pair<string,string>(string("ShaderRequest",string(pShaderRequestLine)));

        EServerError errCompile = ESOK;
        int nRetries = 3;
        do
        {
            if (errCompile != ESOK)
            {
                Sleep(5000);
            }

            if (!CreateRequest(CompileData, Nodes))
            {
                AZ_Error("RemoteCompiler", false, "CShaderSrv::Compile() : failed composing Request XML\n");
                VerboseLogging(true);
                return ESFailed;
            }

            errCompile = Send(CompileData);
        } while (errCompile == ESRecvFailed && nRetries-- > 0);

        rVec    =   CompileData;

        if (errCompile != ESOK)
        {
            bool logError = true;
            const char* why = "";
            switch (errCompile)
            {
            case ESNetworkError:
                why = "Network Error";
                break;
            case ESSendFailed:
                why = "Send Failed";
                break;
            case ESRecvFailed:
                why = "Receive Failed";
                break;
            case ESInvalidState:
                why = "Invalid Return State (compile issue ?!?)";
                break;
            case ESCompileError:
                logError = false;
                why = "";
                break;
            case ESFailed:
                why = "";
                break;
            }
            if (logError)
            {
                AZ_Error("RemoteCompiler", false, "CShaderSrv::Compile() : failed to compile %s (%s)", pEntry, why);
                VerboseLogging(true);
            }
        }
        return errCompile;
    }

    EServerError CShaderSrv::GetShaderList(std::vector<uint8>& rVec) const
    {
        std::vector<uint8>  GetShaderListData;
        std::vector<std::pair<string, string> > Nodes;

        Nodes.push_back(std::pair<string, string>(string("JobType"), string("GetShaderList")));
        Nodes.push_back(std::pair<string, string>(string("Project"), string(m_RequestLineRootFolder.c_str())));
        Nodes.push_back(std::pair<string, string>(string("Platform"), string(GetPlatformName())));
        Nodes.push_back(std::pair<string, string>(string("Compiler"), string(GetShaderCompilerName())));
        Nodes.push_back(std::pair<string, string>(string("Language"), string(GetShaderLanguageName())));
        Nodes.push_back(std::pair<string, string>(string("ShaderList"), string(GetShaderListFilename().c_str())));

        #if defined(AZ_RESTRICTED_PLATFORM)
            #define AZ_RESTRICTED_SECTION REMOTECOMPILER_CPP_SECTION_4
            #include AZ_RESTRICTED_FILE(RemoteCompiler_cpp)
        #endif

        EServerError errShaderGetList = ESOK;
        int nRetries = 3;
        do
        {
            if (errShaderGetList != ESOK)
            {
                Sleep(5000);
            }

            if (!CreateRequest(GetShaderListData, Nodes))
            {
                AZ_Error("RemoteCompler", false, "ERROR: CShaderSrv::GetShaderList(): failed composing Request XML\n");
                VerboseLogging(true);
                return ESFailed;
            }

            errShaderGetList = Send(GetShaderListData);
        } while (errShaderGetList == ESRecvFailed && nRetries-- > 0);

        rVec = GetShaderListData;

        if (errShaderGetList != ESOK)
        {
            bool logError = true;
            const char* why = "";
            switch (errShaderGetList)
            {
            case ESNetworkError:
                why = "Network Error";
                break;
            case ESSendFailed:
                why = "Send Failed";
                break;
            case ESRecvFailed:
                why = "Receive Failed";
                break;
            case ESInvalidState:
                why = "Invalid Return State (compile issue ?!?)";
                break;
            case ESFailed:
                why = "";
                break;
            }
            if (logError)
            {
                AZ_Error("RemoteCompiler", false, "ERROR: CShaderSrv::GetShaderList(): failed to get shader list (%s)", why);
                VerboseLogging(true);
            }
        }
        return errShaderGetList;
    }

    bool CShaderSrv::RequestLine(const string& rList, const string& rString) const
    {
        if (!gRenDev->CV_r_shaderssubmitrequestline)
        {
            return true;
        }

        std::vector<uint8>  CompileData;
        std::vector<std::pair<string, string> > Nodes;

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION REMOTECOMPILER_CPP_SECTION_4
    #include AZ_RESTRICTED_FILE(RemoteCompiler_cpp)
#endif

        Nodes.push_back(std::pair<string, string>(string("JobType"), string("RequestLine")));
        Nodes.push_back(std::pair<string, string>(string("ShaderRequest"), rString));
        Nodes.push_back(std::pair<string, string>(string("Project"), m_RequestLineRootFolder));
        Nodes.push_back(std::pair<string, string>(string("Platform"), string(GetPlatformName())));
        Nodes.push_back(std::pair<string, string>(string("Compiler"), string(GetShaderCompilerName())));
        Nodes.push_back(std::pair<string, string>(string("Language"), string(GetShaderLanguageName())));
        Nodes.push_back(std::pair<string, string>(string("ShaderList"), rList));

        if (!CreateRequest(CompileData, Nodes))
        {
            AZ_Error("RemoteCompiler", false, "CShaderSrv::RequestLine() : failed composing Request XML\n");
            VerboseLogging(true);
            return false;
        }

        return (Send(CompileData) == ESOK);
    }

    bool CShaderSrv::Send(AZSOCKET Socket, const char* pBuffer, uint32 Size) const
    {
        //size_t w;
        size_t wTotal = 0;
        while (wTotal < Size)
        {
            int result = AZ::AzSock::Send(Socket, pBuffer + wTotal, Size - wTotal, 0);
            if (AZ::AzSock::SocketErrorOccured(result))
            {
                AZ_Error("RemoteCompiler", false, "CShaderSrv::Send() : failed (%s)\n", AZ::AzSock::GetStringForError(result));
                VerboseLogging(true);
                return false;
            }
            wTotal += (size_t)result;
        }
        return true;
    }

    bool CShaderSrv::Send(AZSOCKET Socket, std::vector<uint8>& rCompileData)   const
    {
        const uint64 Size   =   static_cast<uint32>(rCompileData.size());
        if(Size == 0)
        {
            return Send(Socket, (const char*) &Size, sizeof(Size));//send 0... if Size is 0 then (const char*)&rCompileData[0] cannot be accessed.
        }

        return Send(Socket, (const char*)&Size, sizeof(Size)) &&
               Send(Socket, (const char*)&rCompileData[0], static_cast<uint32>(Size));
    }

#define MAX_TIME_TO_WAIT 100000

    EServerError CShaderSrv::Recv(AZSOCKET Socket, std::vector<uint8>& rCompileData)   const
    {
        const size_t Offset =   5;//version 2 has 4byte size and 1 byte state
        //  const uint32 Size   =   static_cast<uint32>(rCompileData.size());
        //  return  Send(Socket,(const char*)&Size,4) ||
        //      Send(Socket,(const char*)&rCompileData[0],Size);


        //  delete[] optionsBuffer;
        uint32 nMsgLength = 0;
        uint32 nTotalRecived = 0;
        const size_t    BLOCKSIZE   =   4 * 1024;
        const size_t    SIZELIMIT   =   1024 * 1024;
        rCompileData.resize(0);
        rCompileData.reserve(64 * 1024);
        int CurrentPos  =   0;
        while (rCompileData.size() < SIZELIMIT)
        {
            rCompileData.resize(CurrentPos + BLOCKSIZE);

            int Recived = SOCKET_ERROR;
            int waitingtime = 0;
            while (Recived < 0)
            {
                Recived = AZ::AzSock::Recv(Socket, reinterpret_cast<char*>(&rCompileData[CurrentPos]), BLOCKSIZE, 0);
                if (AZ::AzSock::SocketErrorOccured(Recived))
                {
                    AZ::AzSock::AzSockError error = AZ::AzSock::AzSockError(Recived);
                    if (error == AZ::AzSock::AzSockError::eASE_EWOULDBLOCK)
                    {
                        // are we out of time
                        if (waitingtime > MAX_TIME_TO_WAIT)
                        {
                            AZ_Error("RemoteCompiler", false, "CShaderSrv::Recv() : Out of time during waiting %d seconds on block, sys_net_errno=%s\n", MAX_TIME_TO_WAIT, AZ::AzSock::GetStringForError(Recived));
                            VerboseLogging(true);
                            return ESRecvFailed;
                        }

                        waitingtime += 5;

                        // sleep a bit and try again
                        Sleep(5);
                    }
                    else
                    {
                        // count on retry to fix this after a small sleep
                        AZ_Error("RemoteCompiler", false, "CShaderSrv::Recv() : at offset %lu: sys_net_errno=%s\n", (unsigned long)rCompileData.size(), AZ::AzSock::GetStringForError(Recived));
                        VerboseLogging(true);
                        return ESRecvFailed;
                    }
                }
            }

            if (Recived >= 0)
            {
                nTotalRecived += Recived;
            }

            if (nTotalRecived >= 4)
            {
                nMsgLength = *(uint32*)&rCompileData[0] + Offset;
            }

            if (Recived == 0 || nTotalRecived == nMsgLength)
            {
                rCompileData.resize(nTotalRecived);
                break;
            }
            CurrentPos  +=  Recived;
        }

        return ProcessResponse(rCompileData);
    }

    // given a data vector, check to see if its an error or a success situation.
    // if its an error, replace the buffer with the uncompressed error string if possible.
    EServerError CShaderSrv::ProcessResponse(std::vector<uint8>& rCompileData) const
    {
        // so internally the message is like this
        // [payload length 4 bytes] [status 1 byte] [payload]
        // note that the length of the payload is given, not the total length of the message
        // which is actually payload length + [4 bytes + 1 byte status]

        const size_t OffsetToPayload = sizeof(unsigned int) + sizeof(uint8); // probably 5 bytes

        if (rCompileData.size() < OffsetToPayload)
        {
            AZ_Error("RemoteCompiler", false, "CShaderSrv::ProcessResponse() : data incomplete from server (only %i bytes received)\n", static_cast<int>(rCompileData.size()));
            VerboseLogging(true);
            rCompileData.clear();
            return ESRecvFailed;
        }

        uint32 payloadSize = *(uint32*)&rCompileData[0];
        uint8 state = rCompileData[4];

        if (payloadSize + OffsetToPayload != rCompileData.size())
        {
            AZ_Error("RemoteCompiler", false, "CShaderSrv::ProcessResponse() : data incomplete from server - expected %i bytes, got %i bytes\n", static_cast<int>(payloadSize + OffsetToPayload), static_cast<int>(rCompileData.size()));
            VerboseLogging(true);
            rCompileData.clear();
            return ESRecvFailed;
        }

        if (rCompileData.size() > OffsetToPayload) // ie, not a zero-byte payload
        {
            // move the payload to the beginning of the array, so that the first byte is the first byte of the payload
            memmove(&rCompileData[0], &rCompileData[OffsetToPayload], rCompileData.size() - OffsetToPayload);
            rCompileData.resize(rCompileData.size() - OffsetToPayload);
        }
        else
        {
            rCompileData.clear();
        }

        // decompress datablock if available.
        if (rCompileData.size() > sizeof(unsigned int)) // theres a datablock, which is compressed and there's enough data in there for a header of 4 bytes and payload
        {
            // [4 bytes - size of uncompressed message in network order][compressed payload message]
            // Decompress incoming payload
            std::vector<uint8> rCompressedData;
            rCompressedData.swap(rCompileData);

            uint32 nSrcUncompressedLen = *(uint32*)&rCompressedData[0];
            SwapEndian(nSrcUncompressedLen);

            size_t nUncompressedLen = (size_t)nSrcUncompressedLen;

            // Maximum size allowed for a shader in bytes
            static const size_t maxShaderSize = 10ull * (1024ull * 1024ull); // 10 MB

            if (nUncompressedLen > maxShaderSize)
            {
                // Shader too big, something is wrong.
                rCompileData.clear(); // don't propogate "something is wrong" data
                return ESFailed;
            }

            rCompileData.resize(nUncompressedLen);
            if (nUncompressedLen > 0)
            {
                if (!gEnv->pSystem->DecompressDataBlock(&rCompressedData[4], rCompressedData.size() - 4, &rCompileData[0], nUncompressedLen))
                {
                    rCompileData.clear(); // don't propogate corrupted data
                    return ESFailed;
                }
            }
        }

        if (state != 1) //1==ECSJS_DONE state on server, dont change!
        {
            // getting here means SOME sort of error occurred.
            // don't print compile errors here, they'll be handled later
            if (state == 5) //5==ECSJS_COMPILE_ERROR state on server, dont change!
            {
                return ESCompileError;
            }

            AZ_Error("RemoteCompiler", false, "CShaderSrv::ProcessResponse() : data contains invalid return status: state = %d \n", state);
            VerboseLogging(true);

            return ESInvalidState;
        }
        return ESOK;
    }

    void CShaderSrv::Tokenize(tdEntryVec& rRet, const string& Tokens, const string& Separator)    const
    {
        rRet.clear();
        string::size_type Pt;
        string::size_type Start = 0;
        string::size_type SSize =   Separator.size();

        while ((Pt = Tokens.find(Separator, Start)) != string::npos)
        {
            string  SubStr  =   Tokens.substr(Start, Pt - Start);
            rRet.push_back(SubStr);
            Start = Pt + SSize;
        }

        rRet.push_back(Tokens.substr(Start));
    }

    EServerError CShaderSrv::Send(std::vector<uint8>& rCompileData) const
    {
        if (rCompileData.size() > std::numeric_limits<int>::max())
        {
            AZ_Error("RemoteCompiler", false, "CShaderSrv::Send() : compile data too big to send.\n");
            VerboseLogging(true);
            return ESFailed;
        }

        // this function expects to block until a response is received or failure occurs.
        AzFramework::SocketConnection* engineConnection = AzFramework::SocketConnection::GetInstance();
        bool useAssetProcessor = ((CRenderer::CV_r_AssetProcessorShaderCompiler != 0) && (engineConnection) && (engineConnection->IsConnected()));

        if (m_unitTestMode)
        {
            useAssetProcessor = true; // always test asset processor-based code
        }

        if (useAssetProcessor)
        {
            EServerError resultFromConnection = SendRequestViaEngineConnection(rCompileData);
            if (resultFromConnection != ESOK)
            {
                return resultFromConnection;
            }
        }
        else
        {
            EServerError resultFromSocket = SendRequestViaSocket(rCompileData);

            if (resultFromSocket != ESOK)
            {
                return resultFromSocket;
            }
        }

        if (rCompileData.size() < 4)
        {
            return ESFailed;
        }

        return ESOK;
    }

    EServerError CShaderSrv::SendRequestViaSocket(std::vector<uint8>& rCompileData) const
    {
        AZSOCKET Socket = AZ_SOCKET_INVALID;
        int Err = SOCKET_ERROR;

        // generate the list of servers to make the request to:
        tdEntryVec ServerVec;
        if (gEnv->pConsole->GetCVar("r_ShaderCompilerServer"))
        {
            Tokenize(ServerVec, gEnv->pConsole->GetCVar("r_ShaderCompilerServer")->GetString(), ",");
        }

        if (ServerVec.empty())
        {
            ServerVec.push_back("localhost");
        }

        if(VerboseLogging())
        {
            AZ_TracePrintf("RemoteCompler", "INFO: CShaderSrv::SendRequestViaSocket(): connect to remote shader compiler server: %s...\n", gRenDev->CV_r_ShaderCompilerServer->GetString());
        }

        //connect
        //try each entry in the list from front to back
        bool didconnect = false;
        bool sent = false;
        bool received = false;
        for (uint32 nServer = 0; nServer < ServerVec.size(); nServer++)
        {
            string Server = ServerVec[nServer];

            //try 3 times each in turn
            for (uint32 nRetries = 0; nRetries < 3; nRetries++)
            {
                if(nRetries)
                {
                    AZ_Warning("RemoteCompiler", false, "WARN: CShaderSrv::SendRequestViaSocket(): retry % i to connect to: %s...\n", nRetries, Server.c_str());
                    VerboseLogging(true);
                }
                else
                {
                    if(VerboseLogging())
                    {
                        AZ_TracePrintf("RemoteCompler", "INFO: CShaderSrv::SendRequestViaSocket(): connect to: %s...\n", Server.c_str());
                    }
                }

                //create the socket
                Socket = AZ::AzSock::Socket();

                //if anything went wrong creating the socket, this was not a valid try, so try again
                if (!AZ::AzSock::IsAzSocketValid(Socket))
                {
                    if (nRetries)
                    {
                        nRetries--;
                    }
                    AZ_Warning("RemoteCompiler", false, "WARN: CShaderSrv::SendRequestViaSocket(): can't create client socket: error %s\n", AZ::AzSock::GetStringForError(Socket));
                    VerboseLogging(true);
                }
                else
                {
                    //we have a socket, try to connect
                    AZ::AzSock::SetSocketOption(Socket, AZ::AzSock::AzSocketOption::REUSEADDR, true);
                    AZ::AzSock::AzSocketAddress socketAddress;
                    socketAddress.SetAddress(Server.c_str(), gRenDev->CV_r_ShaderCompilerPort);

                    Err = AZ::AzSock::Connect(Socket, socketAddress);
                    if (AZ::AzSock::SocketErrorOccured(Err))
                    {
                        //connect failed, see if it failed because we don't have enough buffer, if so its not a legit fail of this server, retry

                        // if buffer is full try sleeping a bit before retrying
                        // (if you keep getting this issue then try using same shutdown mechanism as server is doing (see server code))
                        // (for more info on windows side check : http://www.proxyplus.cz/faq/articles/EN/art10002.htm)
                        if (Err == static_cast<AZ::s32>(AZ::AzSock::AzSockError::eASE_ENOBUFS))
                        {
                            AZ_Warning("RemoteCompiler", false, "WARN: CShaderSrv::SendRequestViaSocket(): ENOBUFS: the buffer is full, try again in 5 seconds. %s (sys_net_errno=%s, retrying %d)\n", Server.c_str(), AZ::AzSock::GetStringForError(Err), nRetries);
                            VerboseLogging(true);
                            if (nRetries)
                            {
                                nRetries--;
                            }
                            //wait 5 seconds before retry
                            Sleep(5000);
                        }
                        else
                        {
                            //legit fail to connect, retry
                            AZ_Warning("RemoteCompiler", false, "WARN: CShaderSrv::SendRequestViaSocket(): could not connect to %s (sys_net_errno=%s, retrying %d)\n", Server.c_str(), AZ::AzSock::GetStringForError(Err), nRetries);
                            VerboseLogging(true);

                            //wait 1 second before retry
                            Sleep(1000);
                        }

                        //close the socket for a retry
                        AZ::AzSock::CloseSocket(Socket);
                        Socket = AZ_SOCKET_INVALID;
                    }
                    else
                    {
                        if(VerboseLogging())
                        {
                            AZ_TracePrintf("RemoteCompiler", "INFO: CShaderSrv::SendRequestViaSocket(): connected to: %s...\n", Server.c_str());
                        }

                        didconnect = true;
                        //we connected, send
                        if (!Send(Socket, rCompileData))
                        {
                            //send failed
                            AZ_Warning("RemoteCompiler", false, "WARN: CShaderSrv::SendRequestViaSocket(): failed to send: sys_net_errno=%s\n", AZ::AzSock::GetStringForError(Err));
                            VerboseLogging(true);

                            //wait 1 second before retry
                            Sleep(1000);
                            
                            AZ::AzSock::CloseSocket(Socket);
                            Socket = AZ_SOCKET_INVALID;
                        }
                        else
                        {
                            sent = true;
                            //send succeeded, wait for recv
                            EServerError Error = Recv(Socket, rCompileData);
                            if (Error != ESOK)
                            {
                                //recv failed
                                AZ_Warning("RemoteCompiler", false, "WARN: CShaderSrv::SendRequestViaSocket(): failed to recv: EServerError=%i\n", Error);
                                VerboseLogging(true);

                                //wait 1 second before retry
                                Sleep(1000);

                                AZ::AzSock::CloseSocket(Socket);
                                Socket = AZ_SOCKET_INVALID;
                            }
                            else
                            {
                                received = true;
                                //we are done, it succeeded
                                //shutdown the client side of the socket because we are done listening
                                if(VerboseLogging())
                                {
                                    AZ_TracePrintf("RemoteCompler", "INFO: CShaderSrv::SendRequestViaSocket(): shader request succeeded.\n");
                                }

                                Err = AZ::AzSock::Shutdown(Socket, SD_BOTH);
                                if (Err == SOCKET_ERROR)
                                {
                                    AZ_Warning("RemoteCompiler", false, "WARN: CShaderSrv::SendRequestViaSocket(): succeeded but and got error shutting down socket: sys_net_errno=%s\n", AZ::AzSock::GetStringForError(Err));
                                    VerboseLogging(true);
                                }
                                else
                                {
                                    //put this in the else because OSX can have a problem calling closesocket on a failed shutdown of a socket
                                    AZ::AzSock::CloseSocket(Socket);
                                }
                                Socket = AZ_SOCKET_INVALID;
                                return ESOK;
                            }
                        }
                    }
                }
            }
        }

        //we failed
        AZ::AzSock::CloseSocket(Socket);
        Socket = AZ_SOCKET_INVALID;
        rCompileData.resize(0);
        
        if (didconnect)
        {
            const AZStd::string title = "Remote Shader Compiler";
            const AZStd::string message = AZStd::string::format("We connected to the server but failed to compile the shader!");
            AZ_Error("RemoteCompiler", false, "ERROR: CShaderSrv::SendRequestViaSocket(): %s\n", message.c_str());
            VerboseLogging(true);
            if(!gEnv->IsInToolMode())
            {
                EBUS_EVENT(AZ::NativeUI::NativeUIRequestBus, DisplayOkDialog, title, message, false);
            }
        }
        else
        {
            const AZStd::string title = "Remote Shader Compiler";
            const AZStd::string message = AZStd::string::format("Unable to connect to Remote Shader Compiler at %s", gRenDev->CV_r_ShaderCompilerServer->GetString());
            AZ_Error("RemoteCompiler", false, "ERROR: CShaderSrv::SendRequestViaSocket(): %s\n", message.c_str());
            VerboseLogging(true);
            if(!gEnv->IsInToolMode())
            {
                AZStd::vector<AZStd::string> options;
                options.push_back("OK");
                EBUS_EVENT(AZ::NativeUI::NativeUIRequestBus, DisplayBlockingDialog, title, message, options);
            }
        }
        return ESNetworkError;
    }

    bool CShaderSrv::EncapsulateRequestInEngineConnectionProtocol(std::vector<uint8>& rCompileData) const
    {
        if (rCompileData.empty())
        {
            AZ_Error("RemoteCompiler", false, "CShaderSrv::EncapsulateRequestInEngineConnectionProtocol() : Engine Connection was unable to send the message - zero bytes size.");
            VerboseLogging(true);
            return false;
        }

        string serverList = gEnv->pConsole->GetCVar("r_ShaderCompilerServer")->GetString();
        unsigned int serverListLength = static_cast<unsigned int>(serverList.size());
        unsigned short serverPort = static_cast<unsigned short>(gEnv->pConsole->GetCVar("r_ShaderCompilerPort")->GetIVal());

        if (serverListLength == 0)
        {
            AZ_Error("RemoteCompiler", false, "r_ShaderCompilerServer cvar is empty - no servers to send to.  This CVAR should contain the list of servers to send shader compiler requests to.");
            return false;
        }

        // we're packing at the end because sometimes, you don't need to copy the data in that case.
        std::size_t originalSize = rCompileData.size();

        //                                 a null     the string        a null       the  port         the length of the string
        rCompileData.resize(originalSize +   1    + serverListLength +    1    +  sizeof(serverPort) + sizeof(unsigned int));
        uint8* dataStart = rCompileData.data() + originalSize;
        *dataStart = 0; // null
        ++dataStart;
        memcpy(dataStart, serverList.c_str(), serverList.size()); // server list data
        dataStart += serverList.size();
        *dataStart = 0; // null
        ++dataStart;

        SwapEndian(serverPort);
        SwapEndian(serverListLength);

        memcpy(dataStart, &serverPort, sizeof(serverPort)); // server port
        dataStart += sizeof(serverPort);
        memcpy(dataStart, &serverListLength, sizeof(serverListLength)); // server list length
        dataStart += sizeof(serverListLength);

        // check for buffer overrun
        assert(reinterpret_cast<ptrdiff_t>(dataStart) - reinterpret_cast<ptrdiff_t>(rCompileData.data()) == rCompileData.size());
        return true;
    }

    EServerError CShaderSrv::SendRequestViaEngineConnection(std::vector<uint8>& rCompileData)   const
    {
        // use the asset processor instead of direct socket.
        // wrap it up in a protocol structure - very straight forward - the requestID followed by the data
        // the protocol already takes care of the data size, underneath, so no need to send that

        // what we need include the information about what server(s) to connect to.
        // we can append to the end of the compile data so as to avoid copying unless we need to

        if (!EncapsulateRequestInEngineConnectionProtocol(rCompileData))
        {
            return ESFailed;
        }

        if (!m_remoteState->SubmitRequestAndBlockForResponse(rCompileData))
        {
            rCompileData.clear();
            AZ_Error("RemoteCompiler", false, "CShaderSrv::SendRequestViaEngineConnection() : Engine Connection was unable to send the message.");
            VerboseLogging(true);
            return ESNetworkError;
        }

        if (rCompileData.empty())
        {
            AZ_Error("RemoteCompiler", false, "CShaderSrv::SendRequestViaEngineConnection() : Recv data empty from server (didn't receive anything)\n");
            VerboseLogging(true);

            const AZStd::string title = "Remote Shader Compiler";
            const AZStd::string message = "Unable to connect to Remote Shader Compiler";
            if(!gEnv->IsInToolMode())
            {
                EBUS_EVENT(AZ::NativeUI::NativeUIRequestBus, DisplayOkDialog, title, message, false);
            }
            return ESRecvFailed;
        }

        // Check for error embedded in the response!
        return ProcessResponse(rCompileData);
    }

    void CShaderSrv::EnableUnitTestingMode(bool mode)
    {
        m_unitTestMode = mode;
        m_remoteState->SetUnitTestMode(mode);
    }

} // end namespace

