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

#ifndef CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_SHADERS_REMOTECOMPILER_H
#define CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_SHADERS_REMOTECOMPILER_H
#pragma once

#include <AzCore/Socket/AzSocket_fwd.h>

namespace NRemoteCompiler
{
    typedef std::vector<string> tdEntryVec;

    enum EServerError
    {
        ESOK,
        ESFailed,
        ESInvalidState,
        ESCompileError,
        ESNetworkError,
        ESSendFailed,
        ESRecvFailed,
    };

    enum EShaderCompiler
    {
        eSC_Unknown,
        eSC_Orbis_DXC,
        eSC_Durango_FXC,
        eSC_D3D11_FXC,
        eSC_GLSL_HLSLcc,
        eSC_METAL_HLSLcc,
        eSC_GLSL_LLVM_DXC,
        eSC_METAL_LLVM_DXC,
        eSC_Jasper_FXC,
        eSC_MAX
    };

    class RemoteProxyState;

    class CShaderSrv
    {
    public:
        // the main entry into this system
        // on return, rVec contains the response vector, or an error string, if failed
        EServerError Compile(std::vector<uint8>& rVec, const char* pProfile, const char* pProgram, const char* pEntry, const char* pCompileFlags, const char* pIdent) const;

        EServerError GetShaderList(std::vector<uint8>& rVec) const;

        bool CommitPLCombinations(std::vector<SCacheCombination>& rVec);

        // RequestLine causes the remote compiler to compile without expecting a response.
        bool RequestLine(const string& rList, const string& rString) const;

        EShaderCompiler GetShaderCompiler() const;
        const char *GetShaderCompilerName() const;

        AZStd::string GetShaderCompilerFlags(EHWShaderClass eClass, UPipelineState pipelineState, uint32 MDVMask) const;

        static CShaderSrv& Instance();


    private:
        // UNIT TEST things can go here.
        friend class ShaderSrvUnitTestAccessor;

        bool m_unitTestMode;
        void EnableUnitTestingMode(bool mode); // used during cry unit test launcher

        EServerError SendRequestViaEngineConnection(std::vector<uint8>&  rCompileData) const;

        // socket implementation here
        EServerError SendRequestViaSocket(std::vector<uint8>& rCompileData) const;
        bool Send(AZSOCKET Socket, const char* pBuffer, uint32 Size) const;
        bool Send(AZSOCKET Socket, std::vector<uint8>& rCompileData) const;
        EServerError Recv(AZSOCKET Socket, std::vector<uint8>& rCompileData) const;

        // internal utilities
        bool CreateRequest(std::vector<uint8>& rVec, std::vector<std::pair<string, string> >& rNodes) const;

        bool EncapsulateRequestInEngineConnectionProtocol(std::vector<uint8>& rCompileData) const;

        const char* GetPlatformName() const;

        RemoteProxyState* m_remoteState;

        // root path added to each request line to store the data per game (eg. MyGame\)
        string m_RequestLineRootFolder;

        CShaderSrv();
        EServerError Send(std::vector<uint8>& rCompileData) const;

        void Tokenize(tdEntryVec& rRet, const string& Tokens, const string& Separator) const;
        string TransformToXML(const string& rIn) const;
        string CreateXMLNode(const string& rTag, const string& rValue)  const;
        bool RequestLine(const SCacheCombination& cmb, const string& rLine) const;

        void Init();
        void Terminate();

        // given a payload of response data from the actual shader server
        // decompress and parse it.
        EServerError ProcessResponse(std::vector<uint8>& rCompileData) const;
    };
}
#endif // CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_SHADERS_REMOTECOMPILER_H

