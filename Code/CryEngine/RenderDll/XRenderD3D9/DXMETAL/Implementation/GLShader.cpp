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

// Description : Implements the shader related functions

#include "RenderDll_precompiled.h"
#include "GLShader.hpp"
#include "MetalDevice.hpp"
#include "GLExtensions.hpp"

//  Confetti BEGIN: Igor Lobanchikov
#include "hlslcc.hpp"
#include "hlslcc_bin.hpp"
//  Confetti End: Igor Lobanchikov

namespace NCryMetal
{
    SSource::SSource(const char* pData, uint32 uDataSize)
        : m_uDataSize(uDataSize)
        , m_pData(pData)
    {
    }

    SSource::~SSource()
    {
    }

    struct SDXBCInfo
    {
        uint32 m_uMajorVersion, m_uMinorVersion;
    };

    struct SDXBCParseContext
        : SDXBCInputBuffer
    {
        SDXBCInfo* m_pInfo;

        SDXBCParseContext(const uint8* pBegin, const uint8* pEnd, SDXBCInfo* pInfo)
            : SDXBCInputBuffer(pBegin, pEnd)
            , m_pInfo(pInfo)
        {
        }

        SDXBCParseContext(const SDXBCParseContext& kOther, uint32 uOffset)
            : SDXBCInputBuffer(kOther.m_pBegin + uOffset, kOther.m_pEnd)
            , m_pInfo(kOther.m_pInfo)
        {
            m_pIter = m_pBegin;
        }

        bool ReadString(char* pBuffer, uint32 uBufSize)
        {
            uint32 uSize((uint32)strlen(alias_cast<const char*>(m_pIter)) + 1);
            if (uSize > uBufSize)
            {
                return false;
            }
            return Read(pBuffer, uSize);
        }
    };

    bool DXBCRetrieveInfo(SDXBCParseContext& kContext)
    {
        uint32 uNumChunks;
        DXBCReadUint32(kContext, uNumChunks);

        uint32 uChunk;
        for (uChunk = 0; uChunk < uNumChunks; ++uChunk)
        {
            uint32 uChunkBegin;
            DXBCReadUint32(kContext, uChunkBegin);

            SDXBCParseContext kChunkHeaderContext(kContext, uChunkBegin);

            uint32 uChunkFourCC, uChunkSize;
            DXBCReadUint32(kChunkHeaderContext, uChunkFourCC);
            DXBCReadUint32(kChunkHeaderContext, uChunkSize);

            if (uChunkFourCC == FOURCC_SHDR || uChunkFourCC == FOURCC_SHEX)
            {
                uint32 uEncodedInfo;
                DXBCReadUint32(kChunkHeaderContext, uEncodedInfo);

                kContext.m_pInfo->m_uMajorVersion = (uEncodedInfo >> 4) & 0xF;
                kContext.m_pInfo->m_uMinorVersion = uEncodedInfo        & 0xF;

                return true;
            }
        }
        return false;
    }


    SShaderSource::SShaderSource()
    {
    }

    SShaderSource::~SShaderSource()
    {
        delete [] m_pData;
    }

    void SShaderSource::SetData(const char* pData, uint32 uDataSize)
    {
        delete [] m_pData;
        if (uDataSize > 0)
        {
            char* pBuffer = new char[uDataSize];
            memcpy(pBuffer, pData, uDataSize);
            m_pData = pBuffer;
        }
        else
        {
            m_pData = NULL;
        }
        m_uDataSize = uDataSize;
    }


    SGLShader::SGLShader()
    //  Confetti BEGIN: Igor Lobanchikov
        : m_pFunction(0)
        //  Confetti End: Igor Lobanchikov
    {
    }

    SGLShader::~SGLShader()
    {
        //  Confetti BEGIN: Igor Lobanchikov
        if (m_pFunction)
        {
            [m_pFunction release];
        }
        //  Confetti End: Igor Lobanchikov
    }

    SShader::SShader()
    {
    }

    SShader::~SShader()
    {
        // All pipelines with this shader bound must be removed from the cache and deleted
        TPipelines::const_iterator kPipelineIter(m_kBoundPipelines.begin());
        const TPipelines::const_iterator kPipelineEnd(m_kBoundPipelines.end());
        while (kPipelineIter != kPipelineEnd)
        {
            (*kPipelineIter)->m_pContext->RemovePipeline(*kPipelineIter, this);
            ++kPipelineIter;
        }
    }

    void SShader::AttachPipeline(SPipeline* pPipeline)
    {
        TPipelines::const_iterator kFound(std::find(m_kBoundPipelines.begin(), m_kBoundPipelines.end(), pPipeline));
        if (kFound == m_kBoundPipelines.end())
        {
            m_kBoundPipelines.push_back(pPipeline);
        }
    }

    void SShader::DetachPipeline(SPipeline* pPipeline)
    {
        TPipelines::iterator kFound(std::find(m_kBoundPipelines.begin(), m_kBoundPipelines.end(), pPipeline));
        if (kFound != m_kBoundPipelines.end())
        {
            m_kBoundPipelines.erase(kFound);
        }
        else
        {
            DXGL_ERROR("Could not find the pipeline to be detached from the shader");
        }
    }

    SPipeline::SPipeline(const SPipelineConfiguration& kConfiguration, CContext* pContext)
        : m_kConfiguration(kConfiguration)
        , m_pContext(pContext)
        //  Confetti BEGIN: Igor Lobanchikov
        , m_PipelineState(0)
        //  Confetti End: Igor Lobanchikov
    {
    }

    SPipeline::~SPipeline()
    {
        //  Confetti BEGIN: Igor Lobanchikov
        if (m_PipelineState)
        {
            [m_PipelineState release];
        }
        //  Confetti End: Igor Lobanchikov
    }

    //  Confetti BEGIN: Igor Lobanchikov
    SInputLayout::SInputLayout()
        : m_pVertexDescriptor([[MTLVertexDescriptor alloc] init])
    {
        ;
    }

    SInputLayout::~SInputLayout()
    {
        [(MTLVertexDescriptor*)m_pVertexDescriptor release];
    }
    SInputLayout::SInputLayout(const SInputLayout& other)
    {
        m_pVertexDescriptor = other.m_pVertexDescriptor;
        [(MTLVertexDescriptor*)m_pVertexDescriptor retain];
    }

    SInputLayout& SInputLayout::operator=(const SInputLayout& other)
    {
        [(MTLVertexDescriptor*)m_pVertexDescriptor release];
        m_pVertexDescriptor = other.m_pVertexDescriptor;
        [(MTLVertexDescriptor*)m_pVertexDescriptor retain];

        return *this;
    }
    //  Confetti End: Igor Lobanchikov

    bool InitializeShaderReflectionVariable(SShaderReflectionVariable* pVariable, SDXBCParseContext* pContext)
    {
        uint32 uNamePos;
        if (!DXBCReadUint32(*pContext, uNamePos) ||
            !SDXBCParseContext(*pContext, uNamePos).ReadString(pVariable->m_acName, DXGL_ARRAY_SIZE(pVariable->m_acName)))
        {
            return false;
        }

        uint32 uPosType;
        memset(&pVariable->m_kDesc, 0, sizeof(D3D11_SHADER_VARIABLE_DESC));
        pVariable->m_kDesc.Name = pVariable->m_acName;
        if (!DXBCReadUint32(*pContext, pVariable->m_kDesc.StartOffset) ||
            !DXBCReadUint32(*pContext, pVariable->m_kDesc.Size) ||
            !DXBCReadUint32(*pContext, pVariable->m_kDesc.uFlags) ||
            !DXBCReadUint32(*pContext, uPosType))
        {
            return false;
        }

        {
            SDXBCParseContext kTypeContext(*pContext);
            memset(&pVariable->m_kType, 0, sizeof(D3D11_SHADER_TYPE_DESC));

            if (!kTypeContext.SeekAbs(uPosType) ||
                !DXBCReadUint16(kTypeContext, pVariable->m_kType.Class) ||
                !DXBCReadUint16(kTypeContext, pVariable->m_kType.Type) ||
                !DXBCReadUint16(kTypeContext, pVariable->m_kType.Rows) ||
                !DXBCReadUint16(kTypeContext, pVariable->m_kType.Columns) ||
                !DXBCReadUint16(kTypeContext, pVariable->m_kType.Elements) ||
                !DXBCReadUint16(kTypeContext, pVariable->m_kType.Members) ||
                !DXBCReadUint32(kTypeContext, pVariable->m_kType.Offset))
            {
                return false;
            }
            pVariable->m_kType.Name = NULL;
        }

        uint32 uPosDefaultValue;
        if (!DXBCReadUint32(*pContext, uPosDefaultValue))
        {
            return false;
        }
        if (uPosDefaultValue != 0)
        {
            pVariable->m_kDefaultValue.resize(pVariable->m_kDesc.Size);
            if (!pContext->Read(pVariable->m_kDefaultValue.data(), pVariable->m_kDesc.Size))
            {
                return false;
            }
        }
        else
        {
            pVariable->m_kDesc.DefaultValue = NULL;
        }

        if (pContext->m_pInfo->m_uMajorVersion >= 5)
        {
            if (!DXBCReadUint32(*pContext, pVariable->m_kDesc.StartTexture) ||
                !DXBCReadUint32(*pContext, pVariable->m_kDesc.TextureSize) ||
                !DXBCReadUint32(*pContext, pVariable->m_kDesc.StartSampler) ||
                !DXBCReadUint32(*pContext, pVariable->m_kDesc.SamplerSize))
            {
                return false;
            }
        }

        return true;
    }

    bool InitializeShaderReflectionConstBuffer(SShaderReflectionConstBuffer* pConstBuffer, SDXBCParseContext* pContext)
    {
        uint32 uNamePos;
        if (!DXBCReadUint32(*pContext, uNamePos) ||
            !SDXBCParseContext(*pContext, uNamePos).ReadString(pConstBuffer->m_acName, DXGL_ARRAY_SIZE(pConstBuffer->m_acName)))
        {
            return false;
        }

        memset(&pConstBuffer->m_kDesc, 0, sizeof(pConstBuffer->m_kDesc));
        pConstBuffer->m_kDesc.Name = pConstBuffer->m_acName;

        uint32 uNumVariables, uPosVariables;
        if (!DXBCReadUint32(*pContext, uNumVariables) ||
            !DXBCReadUint32(*pContext, uPosVariables))
        {
            return false;
        }
        pConstBuffer->m_kDesc.Variables = uNumVariables;

        SDXBCParseContext kVariablesContext(*pContext);
        if (!kVariablesContext.SeekAbs(uPosVariables))
        {
            return false;
        }
        uint32 uVariable;
        pConstBuffer->m_kVariables.resize(pConstBuffer->m_kDesc.Variables);
        for (uVariable = 0; uVariable < pConstBuffer->m_kDesc.Variables; ++uVariable)
        {
            if (!InitializeShaderReflectionVariable(&pConstBuffer->m_kVariables.at(uVariable), &kVariablesContext))
            {
                return false;
            }
        }

        if (!DXBCReadUint32(*pContext, pConstBuffer->m_kDesc.Size) ||
            !DXBCReadUint32(*pContext, pConstBuffer->m_kDesc.uFlags) ||
            !DXBCReadUint32(*pContext, pConstBuffer->m_kDesc.Type))
        {
            return false;
        }

        return true;
    }

    bool InitializeShaderReflectionResource(SShaderReflectionResource* pParameter, SDXBCParseContext* pContext)
    {
        uint32 uNamePos;
        if (!DXBCReadUint32(*pContext, uNamePos) ||
            !SDXBCParseContext(*pContext, uNamePos).ReadString(pParameter->m_acName, DXGL_ARRAY_SIZE(pParameter->m_acName)))
        {
            return false;
        }

        memset(&pParameter->m_kDesc, 0, sizeof(pParameter->m_kDesc));

        pParameter->m_kDesc.Name = pParameter->m_acName;
        if (!DXBCReadUint32(*pContext, pParameter->m_kDesc.Type) ||
            !DXBCReadUint32(*pContext, pParameter->m_kDesc.ReturnType) ||
            !DXBCReadUint32(*pContext, pParameter->m_kDesc.Dimension) ||
            !DXBCReadUint32(*pContext, pParameter->m_kDesc.NumSamples) ||
            !DXBCReadUint32(*pContext, pParameter->m_kDesc.BindPoint) ||
            !DXBCReadUint32(*pContext, pParameter->m_kDesc.BindCount) ||
            !DXBCReadUint32(*pContext, pParameter->m_kDesc.uFlags))
        {
            return false;
        }

        return true;
    }

    bool InitializeShaderReflectionParameters(SShaderReflection::TParameters& kParameters, SDXBCParseContext* pContext, bool extended = false)
    {
        uint32 uNumElements, uVersion;
        if (!DXBCReadUint32(*pContext, uNumElements) ||
            !DXBCReadUint32(*pContext, uVersion))
        {
            return false;
        }

        if (uNumElements > 0)
        {
            uint32 uPrevNumElements(kParameters.size());
            kParameters.resize(kParameters.size() + uNumElements);

            uint32 uElement;
            for (uElement = uPrevNumElements; uElement < kParameters.size(); ++uElement)
            {
                SShaderReflectionParameter& kParameter(kParameters.at(uElement));
                
                memset(&kParameter.m_kDesc, 0, sizeof(kParameter.m_kDesc));
                
                // Only for extended parameters fxc adds two extra pieces of information, the stream index and the minimum precision.
                if (extended && !DXBCReadUint32(*pContext, kParameter.m_kDesc.Stream))
                {
                    return false;
                }
                
                uint32 uSemNamePosition;
                if (!DXBCReadUint32(*pContext, uSemNamePosition) ||
                    !SDXBCParseContext(*pContext, uSemNamePosition).ReadString(kParameter.m_acSemanticName, DXGL_ARRAY_SIZE(kParameter.m_acSemanticName)))
                {
                    return false;
                }
                
                kParameter.m_kDesc.SemanticName = kParameter.m_acSemanticName;
                if (!DXBCReadUint32(*pContext, kParameter.m_kDesc.SemanticIndex) ||
                    !DXBCReadUint32(*pContext, kParameter.m_kDesc.SystemValueType) ||
                    !DXBCReadUint32(*pContext, kParameter.m_kDesc.ComponentType) ||
                    !DXBCReadUint32(*pContext, kParameter.m_kDesc.Register) ||
                    !DXBCReadUint8(*pContext, kParameter.m_kDesc.Mask) ||
                    !DXBCReadUint8(*pContext, kParameter.m_kDesc.ReadWriteMask) ||
                    !pContext->SeekRel(sizeof(uint16_t)) || // These two bytes are part of the mask but are not used, so we skip them.
                    (extended && !DXBCReadUint32(*pContext, kParameter.m_kDesc.MinPrecision))) // Precision available only for extended parameters.
                {
                    return false;
                }
            }
        }
        return true;
    }

    bool InitializeShaderReflection(SShaderReflection* pReflection, const void* pvData)
    {
        enum
        {
            SIZE_POSITION = 6
        };

        uint32 uSize(alias_cast<const uint32*>(pvData)[SIZE_POSITION]);



        enum
        {
            CHUNKS_POSITION = sizeof(uint32) * 7
        };
        SDXBCInfo kInfo;
        const uint8* puData(alias_cast<const uint8*>(pvData));
        SDXBCParseContext kContext(puData, puData + uSize, &kInfo);

        if (!kContext.SeekAbs(CHUNKS_POSITION))
        {
            return false;
        }

        {
            SDXBCParseContext kParseContext(kContext);
            if (!DXBCRetrieveInfo(kParseContext))
            {
                return false;
            }
        }
        uint32 uNumChunks;
        if (!DXBCReadUint32(kContext, uNumChunks))
        {
            return false;
        }

        uint32 uChunk;
        for (uChunk = 0; uChunk < uNumChunks; ++uChunk)
        {
            uint32 uChunkBegin;
            if (!DXBCReadUint32(kContext, uChunkBegin))
            {
                return false;
            }

            SDXBCParseContext kChunkHeaderContext(kContext, uChunkBegin);

            uint32 uChunkFourCC, uChunkSize;
            if (!DXBCReadUint32(kChunkHeaderContext, uChunkFourCC) ||
                !DXBCReadUint32(kChunkHeaderContext, uChunkSize))
            {
                return false;
            }

            SDXBCParseContext kChunkContext(kChunkHeaderContext.m_pIter, kChunkHeaderContext.m_pEnd, &kInfo);
            switch (uChunkFourCC)
            {
            case FOURCC_RDEF:
            {
                uint32 uNumConstBuffers, uPosConstBuffers, uNumResources, uPosResources;
                if (!DXBCReadUint32(kChunkContext, uNumConstBuffers) ||
                    !DXBCReadUint32(kChunkContext, uPosConstBuffers) ||
                    !DXBCReadUint32(kChunkContext, uNumResources) ||
                    !DXBCReadUint32(kChunkContext, uPosResources))
                {
                    return false;
                }

                SDXBCParseContext kConstBufferContext(kChunkContext);
                if (!kConstBufferContext.SeekAbs(uPosConstBuffers))
                {
                    return false;
                }
                uint32 uConstBuffer;
                uint32 uPrevNumConstBuffers(pReflection->m_kConstantBuffers.size());
                pReflection->m_kConstantBuffers.resize(uNumConstBuffers);
                for (uConstBuffer = uPrevNumConstBuffers; uConstBuffer < uNumConstBuffers; ++uConstBuffer)
                {
                    if (!InitializeShaderReflectionConstBuffer(&pReflection->m_kConstantBuffers.at(uConstBuffer), &kConstBufferContext))
                    {
                        return false;
                    }
                }

                SDXBCParseContext kResourceContext(kChunkContext);
                if (!kResourceContext.SeekAbs(uPosResources))
                {
                    return false;
                }
                uint32 uResource;
                uint32 uPrevNumResources(pReflection->m_kResources.size());
                pReflection->m_kResources.resize(uNumResources);
                for (uResource = uPrevNumResources; uResource < uNumResources; ++uResource)
                {
                    if (!InitializeShaderReflectionResource(&pReflection->m_kResources.at(uResource), &kResourceContext))
                    {
                        return false;
                    }
                }
            }
            break;
            case FOURCC_ISGN:
            case FOURCC_ISG1:
                if (!InitializeShaderReflectionParameters(pReflection->m_kInputs, &kChunkContext, uChunkFourCC == FOURCC_ISG1))
                {
                    return false;
                }
                break;
            case FOURCC_OSGN:
            case FOURCC_OSG1:
                if (!InitializeShaderReflectionParameters(pReflection->m_kOutputs, &kChunkContext, uChunkFourCC == FOURCC_OSG1))
                {
                    return false;
                }
                break;
            case FOURCC_PCSG:
                if (!InitializeShaderReflectionParameters(pReflection->m_kPatchConstants, &kChunkContext))
                {
                    return false;
                }
                break;
            case FOURCC_GLSL:
                if (!DXBCReadUint32(kChunkContext, pReflection->m_uSamplerMapSize) ||
                    !DXBCReadUint32(kChunkContext, pReflection->m_uImportsSize) ||
                    !DXBCReadUint32(kChunkContext, pReflection->m_uExportsSize) ||
                    !DXBCReadUint32(kChunkContext, pReflection->m_uInputHash)   ||
                    !DXBCReadUint32(kChunkContext, pReflection->m_uUAVBindingAreaSize)   ||
                    !DXBCReadUint32(kChunkContext, pReflection->m_uThread_x)   ||
                    !DXBCReadUint32(kChunkContext, pReflection->m_uThread_y)   ||
                    !DXBCReadUint32(kChunkContext, pReflection->m_uThread_z))
                {
                    return false;
                }
                pReflection->m_uSamplerMapOffset = (uint32)(kChunkContext.m_pIter - kContext.m_pBegin);
                pReflection->m_uImportsOffset =
                    pReflection->m_uSamplerMapOffset +
                    pReflection->m_uSamplerMapSize * 2 * 4; // Each sampler is {uint32 uTexture; uint32 uSampler;}
                pReflection->m_uExportsOffset =
                    pReflection->m_uImportsOffset +
                    pReflection->m_uImportsSize * 3 * 4; // Each import is  {uint32 uType; uint32 uID; uint32 uValue;}
                pReflection->m_uUAVBindingAreaOffset =
                    pReflection->m_uExportsOffset +
                    pReflection->m_uExportsSize * 3 * 4; // Each export is  {uint32 uType; uint32 uID; uint32 uValue;}
                pReflection->m_uGLSLSourceOffset =
                    pReflection->m_uUAVBindingAreaOffset +
                    pReflection->m_uUAVBindingAreaSize * 2 * 4; // Each export is  {uint32 uResource; uint32 uValue;}

                break;
            }
        }

        SDXBCParseContext kConstUAVAreaContext(kContext);
        if (!kConstUAVAreaContext.SeekAbs(pReflection->m_uUAVBindingAreaOffset))
        {
            return false;
        }
        for (uint uResource = 0; uResource < pReflection->m_uUAVBindingAreaSize; uResource++)
        {
            uint32 uResourceIndex;
            if (!DXBCReadUint32(kConstUAVAreaContext, uResourceIndex))
            {
                return false;
            }
            if (!DXBCReadUint32(kConstUAVAreaContext, pReflection->m_kResources.at(uResourceIndex).m_kDesc.UAVBindingArea))
            {
                return false;
            }
        }

        memset(&pReflection->m_kDesc, 0, sizeof(D3D11_SHADER_DESC));
        pReflection->m_kDesc.ConstantBuffers         = pReflection->m_kConstantBuffers.size();
        pReflection->m_kDesc.BoundResources          = pReflection->m_kResources.size();
        pReflection->m_kDesc.InputParameters         = pReflection->m_kInputs.size();
        pReflection->m_kDesc.OutputParameters        = pReflection->m_kOutputs.size();
        pReflection->m_kDesc.PatchConstantParameters = pReflection->m_kPatchConstants.size();

        return true;
    }

    MTLLanguageVersion GetMetalLanguage()
    {
#if defined(AZ_COMPILER_CLANG) && AZ_COMPILER_CLANG >= 9    //@available was added in Xcode 9
#if defined(__MAC_10_13) || defined(__IPHONE_11_0) || defined(__TVOS_11_0)
        if(@available(macOS 10.13, iOS 11.0, tvOS 11.0, *))
        {
            return MTLLanguageVersion2_0;
        }
#endif
        
#if defined(__MAC_10_12) || defined(__IPHONE_10_0) || defined(__TVOS_10_0)
        if (@available(macOS 10.12, iOS 10.0, tvOS 10.0, *))
        {
            return MTLLanguageVersion1_2;
        }
#endif
        
#if defined(__MAC_10_11) || defined(__IPHONE_9_0) || defined(__TVOS_9_0)
        if(@available(macOS 10.11, iOS 9.0, tvOS 9.0, *))
        {
            
            return MTLLanguageVersion1_1;
        }
#endif
#else  // Xcode 8.
        
        // NSFoundationVersionNumber10_10_4 is to check for iOS 10.0+ and macOS 10.12+
        if (floor(NSFoundationVersionNumber) < NSFoundationVersionNumber10_10_4)
        {
            return MTLLanguageVersion1_1;
        }
        else
        {
            return MTLLanguageVersion1_2;
        }
 
#endif
        return MTLLanguageVersion1_2;
    }
    
    bool CompileShader(SShader* pShader, SSource* aSourceVersions, SGLShader* pGLShader, id<MTLDevice> mtlDevice)
    {
        
        NSMutableString* source = [[NSMutableString alloc] initWithBytes:aSourceVersions->m_pData
                                                     length:(aSourceVersions->m_uDataSize - 1)
                                                   encoding:NSASCIIStringEncoding];

        MTLCompileOptions* options = [MTLCompileOptions alloc];
        options.fastMathEnabled = CRenderer::CV_r_MetalShadersFastMath ? YES : NO;
        
        MTLLanguageVersion metalLang = GetMetalLanguage();
        if(metalLang == MTLLanguageVersion1_1)
        {
            //Our cross compiler doesnt have support for different mtllanguageversions. In order to ensure
            //that the game runs on ios 9.0 we simply have language specific macros in the metal shader
            //and inserting the define language MTLLanguageVersion1_1 to ensure correct code is used
            [source insertString:@"#define MTLLanguage1_1\n" atIndex:0];
        }
        options.languageVersion = metalLang;
        NSError* error = nil;

        id<MTLLibrary> lib = [mtlDevice newLibraryWithSource:source
                              options:options
                              error:&error];
        

        LOG_METAL_SHADER_SOURCE(@ "%@", source);

        if (error)
        {
            // Error code 4 is warning, but sometimes a 3 (compile error) is returned on warnings only.
            // The documentation indicates that if the lib is nil there is a compile error, otherwise anything
            // in the error is really a warning. Therefore, we check the lib instead of the error code
            if (!lib)
            {
                LOG_METAL_SHADER_ERRORS(@ "%@", error);
                assert(!"HLSLcc compiler should generate valid code.");
            }
            else
            {
                LOG_METAL_SHADER_WARNINGS(@ "%@", error);
            }

            //  Igor: error string is an autorelease object.
            error = 0;
        }

        if (lib)
        {
            pGLShader->m_pFunction = [lib newFunctionWithName:@ "metalMain"];
            [lib release];
        }

        [options release];
        [source release];

        if (!pGLShader->m_pFunction)
        {
            return false;
        }

        if (pGLShader->m_pFunction.functionType == MTLFunctionTypeVertex)
        {
            LOG_METAL_SHADER_REFLECTION_VALIDATION(@ "%@", pGLShader->m_pFunction.vertexAttributes);
        }

        return true;
        
    }

    //  Confetti BEGIN: Igor Lobanchikov
    void LogMTLArgument(MTLArgument* arg)
    {
#if DXMETAL_LOG_SHADER_REFLECTION_VALIDATION
        NSLog(@ "Name: %@", arg.name);

        NSLog(@ "\tActive: %@", arg.active ? @ "true" : @ "false");

        switch (arg.type)
        {
        case MTLArgumentTypeBuffer:
            NSLog(@ "\tType: %@", @ "MTLArgumentTypeBuffer");
            break;

        case MTLArgumentTypeThreadgroupMemory:
            NSLog(@ "\tType: %@", @ "MTLArgumentTypeThreadgroupMemory");
            break;

        case MTLArgumentTypeTexture:
            NSLog(@ "\tType: %@", @ "MTLArgumentTypeTexture");
            break;

        case MTLArgumentTypeSampler:
            NSLog(@ "\tType: %@", @ "MTLArgumentTypeSampler");
            break;
        }

        NSLog(@ "\tIndex: %lu", arg.index);

        switch (arg.access)
        {
        case MTLArgumentAccessReadOnly:
            NSLog(@ "\tAccess: %@", @ "MTLArgumentAccessReadOnly");
            break;
        case MTLArgumentAccessReadWrite:
            NSLog(@ "\tAccess: %@", @ "MTLArgumentAccessReadWrite");
            break;
        case MTLArgumentAccessWriteOnly:
            NSLog(@ "\tAccess: %@", @ "MTLArgumentAccessWriteOnly");
            break;
        }

        if (arg.type == MTLArgumentTypeBuffer)
        {
            NSLog(@ "StructType: %@", arg.bufferStructType);
        }
#endif // DXMETAL_LOG_SHADER_REFLECTION_VALIDATION
    }

    MTLArgumentType DX11SITToMetal(D3D_SHADER_INPUT_TYPE dx11Type)
    {
        switch (dx11Type)
        {
        case D3D_SIT_CBUFFER:
        case D3D_SIT_TBUFFER:
            return MTLArgumentTypeBuffer;

        case D3D_SIT_TEXTURE:
            return MTLArgumentTypeTexture;

        case D3D_SIT_SAMPLER:
            return MTLArgumentTypeSampler;

        case D3D_SIT_STRUCTURED:
            return MTLArgumentTypeBuffer;
        case D3D_SIT_UAV_RWTYPED:
            return MTLArgumentTypeBuffer;
        case D3D_SIT_UAV_RWSTRUCTURED:
            return MTLArgumentTypeBuffer;
        default:
            assert(!"Not supported for this platform yet");
            return (MTLArgumentType)99;
        }

        //MTLArgumentTypeThreadgroupMemory= 1,
    }

    bool ValidateBindPoint(MTLArgument* arg, SShaderReflection& cryReflection)
    {
        SShaderReflection::TResources kResources = cryReflection.m_kResources;

        char acName[DXGL_MAX_REFLECT_STRING_LENGTH];
        strncpy(acName, [arg.name UTF8String], DXGL_MAX_REFLECT_STRING_LENGTH - 1);
        acName[DXGL_MAX_REFLECT_STRING_LENGTH - 1] = 0;

        if (strstr(acName, "vertexBuffer.") == acName)
        {
            LOG_METAL_SHADER_REFLECTION_VALIDATION(@ "Resource %@ is attached via input assembler.", arg.name);
            return true;
        }

        size_t strArrLen = kResources.size();

        size_t i = 0;
        for (; i < strArrLen; ++i)
        {
            uint32 offset = 0;
            if (kResources[i].m_kDesc.Type == D3D_SIT_UAV_RWTYPED || kResources[i].m_kDesc.Type == D3D_SIT_UAV_RWSTRUCTURED)
            {
                // UAV resources are after the constants buffers so set the offset appropriately
                offset = SBufferStateStageCache::s_MaxConstantBuffersPerStage;
            }
            if (strstr(acName, kResources[i].m_acName) == acName)
            {
                if (((kResources[i].m_kDesc.BindPoint + offset) == arg.index && DX11SITToMetal(kResources[i].m_kDesc.Type) == arg.type))
                {
                    break;
                }
            }
        }

        if (i == strArrLen)
        {
            LOG_METAL_SHADER_ERRORS(@ "Resource \"%@\" or similar was not found in DX11 reflection or bind points do not match.", arg.name);
            assert(!"HLSLcc code must pass validator");
            return false;
        }

        LOG_METAL_SHADER_REFLECTION_VALIDATION(@ "Resource %s is detected as \"%@\" in the metal shader.", kResources[i].m_acName, arg.name);

        return true;
    }

    bool ValidateBufferResource(MTLArgument* arg, SShaderReflection& cryReflection)
    {
        assert(arg.type == MTLArgumentTypeBuffer);

        char acName[DXGL_MAX_REFLECT_STRING_LENGTH];
        strncpy(acName, [arg.name UTF8String], DXGL_MAX_REFLECT_STRING_LENGTH - 1);
        acName[DXGL_MAX_REFLECT_STRING_LENGTH - 1] = 0;

        if (strstr(acName, "vertexBuffer.") == acName)
        {
            LOG_METAL_SHADER_REFLECTION_VALIDATION(@ "Resource %@ is attached via input assembler.", arg.name);
            return true;
        }

        SShaderReflection::TConstantBuffers& kConstantBuffers = cryReflection.m_kConstantBuffers;

        size_t strArrLen = kConstantBuffers.size();

        size_t i = 0;
        for (; i < strArrLen; ++i)
        {
            if (strstr(acName, kConstantBuffers[i].m_acName) == acName)
            {
                break;
            }
        }

        if (i == strArrLen)
        {
            LOG_METAL_SHADER_ERRORS(@ "Buffer \"%@\" description or similar was not found in DX11 reflection.", arg.name);
            assert(!"HLSLcc code must pass validator");
            return false;
        }

        SShaderReflectionConstBuffer::TVariables& kDX11Vars = kConstantBuffers[i].m_kVariables;
        size_t dx11VarNum = kDX11Vars.size();

        LOG_METAL_SHADER_REFLECTION_VALIDATION(@ "Buffer %s is detected as \"%@\" in the metal shader.", kConstantBuffers[i].m_acName, arg.name);
        LOG_METAL_SHADER_REFLECTION_VALIDATION(@ "%@", arg.bufferStructType);

        for (MTLStructMember* var in arg.bufferStructType.members)
        {
            if ([var.name rangeOfString:@ "offsetDummy"].location != NSNotFound)
            {
                continue;
            }

            char acVarName[DXGL_MAX_REFLECT_STRING_LENGTH];
            strncpy(acVarName, [var.name UTF8String], DXGL_MAX_REFLECT_STRING_LENGTH - 1);
            acVarName[DXGL_MAX_REFLECT_STRING_LENGTH - 1] = 0;

            size_t j = 0;
            int varOffset = var.offset;
            for (; j < dx11VarNum; ++j)
            {
                if ((strstr(acVarName, kDX11Vars[j].m_acName) == acVarName) && (varOffset == kDX11Vars[j].m_kDesc.StartOffset) ||
                    (strstr(kDX11Vars[j].m_acName, "$Element") == kDX11Vars[j].m_acName))
                {
                    break;
                }
            }

            if (j == dx11VarNum)
            {
                LOG_METAL_SHADER_ERRORS(@ "Variable \"%@::%@\" or similar was not found in DX11 reflection or offsets in buffers do not match", arg.name, var.name);
                assert(!"HLSLcc code must pass validator");
                return false;
            }

            LOG_METAL_SHADER_REFLECTION_VALIDATION(@ "Variable %s::%s is detected as \"%@::%@\" in the metal shader.", kConstantBuffers[i].m_acName, kDX11Vars[j].m_acName, arg.name, var.name);
        }

        return true;
    }


    bool ValidateReflection(EShaderType shaderType, NSArray* metalReflection, SShaderReflection& cryReflection)
    {
        LOG_METAL_SHADER_REFLECTION_VALIDATION(shaderType == eST_Vertex ? @ "Vertex Shader inputs validation" :
            (shaderType == eST_Fragment ? @ "Fragment Shader inputs validation" : @ "Compute Shader inputs validation"));

        bool res = true;

        for (MTLArgument* arg in metalReflection)
        {
            if (!arg.active)
            {
                continue;
            }
            if (arg.access == MTLArgumentAccessWriteOnly)
            {
                continue;
            }

            if (!ValidateBindPoint(arg, cryReflection))
            {
                res = false;
                LOG_METAL_SHADER_REFLECTION_VALIDATION(@ "%@", arg);
                LogMTLArgument(arg);
            }

            if (shaderType != eST_Compute)
            {
                switch (arg.type)
                {
                case MTLArgumentTypeBuffer:
                {
                    if (!ValidateBufferResource(arg, cryReflection))
                    {
                        res = false;
                        LogMTLArgument(arg);
                    }
                    break;
                }
                case MTLArgumentTypeTexture:
                {
                    break;
                }
                }
            }
        }

        return res;
    }
    //  Confetti End: Igor Lobanchikov

    MTLVertexDescriptor* CleanUnusedInputs(MTLVertexDescriptor* pIn, const TShaderReflection& kReflection)
    {
        MTLVertexDescriptor* pOut = nil;

        bool bBufferUsed[DXMETAL_MAX_ENTRIES_BUFFER_ARG_TABLE];
        memset(bBufferUsed, 0, sizeof(bBufferUsed));

        //  TODO: Igor: figure out why input layout can come with vertex buffer being used without any inputs attached.
        //  Note: disable the following line and metal pipeline will explaine what's wrong.
        pOut = [pIn copyWithZone:nil];

        //  Igor: For unused inputs clean input attribute record
        //  Igor: For used inputs mark input buffer as used
        TShaderReflection::TParameters::const_iterator kInputSemanticIter(kReflection.m_kInputs.begin());
        const TShaderReflection::TParameters::const_iterator kInputSemanticEnd(kReflection.m_kInputs.end());
        while (kInputSemanticIter != kInputSemanticEnd)
        {
            if (kInputSemanticIter->m_kDesc.ReadWriteMask)
            {
                bBufferUsed[pIn.attributes[kInputSemanticIter->m_kDesc.Register].bufferIndex] = true;
            }
            else
            {
                if (!pOut)
                {
                    pOut = [pIn copyWithZone:nil];
                }

                [pOut.attributes setObject : 0 atIndexedSubscript: kInputSemanticIter->m_kDesc.Register];
            }

            ++kInputSemanticIter;
        }


        //  pOut is not nil if had to patch at least single input
        if (pOut)
        {
            const int iAttrCount = 31;
            for (int i = 0; i < iAttrCount; ++i)
            {
                uint32 VBIndex = pOut.attributes[i].bufferIndex;
                if ((pOut.attributes[i].format != MTLVertexFormatInvalid) && (!bBufferUsed[VBIndex]))
                {
                    [pOut.attributes setObject : 0 atIndexedSubscript: i];
                }
            }

            const int bufCount = DXMETAL_MAX_ENTRIES_BUFFER_ARG_TABLE;
            for (int VBIndex = 0; VBIndex < bufCount; ++VBIndex)
            {
                if (!bBufferUsed[VBIndex])
                {
                    [pOut.layouts setObject : 0 atIndexedSubscript: VBIndex];
                }
            }

            return pOut;
        }
        else
        {
            //  Igor: the caller is responsible for releasing the result
            [pIn retain];
            return pIn;
        }
    }

    bool CompilePipeline(SPipeline* pPipeline, CDevice* pDevice)
    {
        id<MTLDevice> mtlDevice = pDevice->GetMetalDevice();
        if (pPipeline->m_kConfiguration.m_apShaders[eST_Compute] != NULL)
        {
            MTLComputePipelineDescriptor* desc = [[MTLComputePipelineDescriptor alloc] init];
            desc.computeFunction = pPipeline->m_kConfiguration.m_apShaders[eST_Compute]->m_kGLShader.m_pFunction;

            NSError* error = 0;
            MTLComputePipelineReflection* ref;
            pPipeline->m_ComputePipelineState = [mtlDevice newComputePipelineStateWithDescriptor:desc options : MTLPipelineOptionBufferTypeInfo reflection : &ref error : &error];

            if (!pPipeline->m_ComputePipelineState)
            {
                LOG_METAL_PIPELINE_ERRORS(@ "Error generation compute pipeline object: %@", error);
                LOG_METAL_PIPELINE_ERRORS(@ "Descriptor: %@", desc);
                assert(!"Compute pipeline object shouldn't fail to be created.");
                return false;
            }

            bool res = ValidateReflection(eST_Compute, ref.arguments, pPipeline->m_kConfiguration.m_apShaders[eST_Compute]->m_kReflection);

            assert(res || !"compute shader must pass validation");

            [desc release];

        }

        if (pPipeline->m_kConfiguration.m_apShaders[eST_Vertex] != NULL)
        {
            MTLRenderPipelineDescriptor* desc = [[MTLRenderPipelineDescriptor alloc] init];

            for (int i = 0; i < SAttachmentConfiguration::s_ColorAttachmentDescCount; ++i)
            {
                desc.colorAttachments[i].pixelFormat = pPipeline->m_kConfiguration.m_AttachmentConfiguration.colorAttachments[i].pixelFormat;

                desc.colorAttachments[i].writeMask = pPipeline->m_kConfiguration.m_AttachmentConfiguration.colorAttachments[i].writeMask;
                desc.colorAttachments[i].blendingEnabled = pPipeline->m_kConfiguration.m_AttachmentConfiguration.colorAttachments[i].blendingEnabled;
                desc.colorAttachments[i].alphaBlendOperation = pPipeline->m_kConfiguration.m_AttachmentConfiguration.colorAttachments[i].alphaBlendOperation;
                desc.colorAttachments[i].rgbBlendOperation = pPipeline->m_kConfiguration.m_AttachmentConfiguration.colorAttachments[i].rgbBlendOperation;
                desc.colorAttachments[i].destinationAlphaBlendFactor = pPipeline->m_kConfiguration.m_AttachmentConfiguration.colorAttachments[i].destinationAlphaBlendFactor;
                desc.colorAttachments[i].destinationAlphaBlendFactor = pPipeline->m_kConfiguration.m_AttachmentConfiguration.colorAttachments[i].destinationAlphaBlendFactor;
                desc.colorAttachments[i].destinationRGBBlendFactor = pPipeline->m_kConfiguration.m_AttachmentConfiguration.colorAttachments[i].destinationRGBBlendFactor;
                desc.colorAttachments[i].sourceAlphaBlendFactor = pPipeline->m_kConfiguration.m_AttachmentConfiguration.colorAttachments[i].sourceAlphaBlendFactor;
                desc.colorAttachments[i].sourceRGBBlendFactor = pPipeline->m_kConfiguration.m_AttachmentConfiguration.colorAttachments[i].sourceRGBBlendFactor;
            }
            desc.depthAttachmentPixelFormat = pPipeline->m_kConfiguration.m_AttachmentConfiguration.depthAttachmentPixelFormat;
            desc.stencilAttachmentPixelFormat = pPipeline->m_kConfiguration.m_AttachmentConfiguration.stencilAttachmentPixelFormat;

            MTLVertexDescriptor* pCleanedDesctiptor = CleanUnusedInputs((MTLVertexDescriptor*)pPipeline->m_kConfiguration.m_pVertexDescriptor,
                    pPipeline->m_kConfiguration.m_apShaders[eST_Vertex]->m_kReflection);

            // set the bound shader state settings
            desc.vertexDescriptor = pCleanedDesctiptor;
            desc.vertexFunction = pPipeline->m_kConfiguration.m_apShaders[eST_Vertex]->m_kGLShader.m_pFunction;
            desc.fragmentFunction = pPipeline->m_kConfiguration.m_apShaders[eST_Fragment] ? pPipeline->m_kConfiguration.m_apShaders[eST_Fragment]->m_kGLShader.m_pFunction : 0;

            [pCleanedDesctiptor release];

            NSError* error = 0;
            MTLRenderPipelineReflection* ref;
            pPipeline->m_PipelineState = [mtlDevice newRenderPipelineStateWithDescriptor:desc options : MTLPipelineOptionBufferTypeInfo reflection : &ref error : &error];

            if (!pPipeline->m_PipelineState)
            {
                LOG_METAL_PIPELINE_ERRORS(@ "Error generation pipeline object: %@", error);
                LOG_METAL_PIPELINE_ERRORS(@ "Descriptor: %@", desc);
                assert(!"Pipeline object shouldn't fail to be created.");
                return false;
            }

            bool res = ValidateReflection(eST_Vertex, ref.vertexArguments, pPipeline->m_kConfiguration.m_apShaders[eST_Vertex]->m_kReflection);
            res &= ValidateReflection(eST_Fragment, ref.fragmentArguments, pPipeline->m_kConfiguration.m_apShaders[eST_Fragment]->m_kReflection);

            assert(res || !"shader must pass validation");

            [desc release];
        }

        return true;
    }

    bool InitializeShader(SShader* pShader, const void* pvSourceData, size_t uSourceSize, id<MTLDevice> mtlDevice)
    {
        pShader->m_kSource.SetData(reinterpret_cast<const char*>(pvSourceData), uSourceSize);

        uint32 uGLSLOffset;
        if (!InitializeShaderReflection(&pShader->m_kReflection, pShader->m_kSource.m_pData) ||
            (uGLSLOffset = pShader->m_kReflection.m_uGLSLSourceOffset) >= (uint32)uSourceSize)
        {
            DXGL_ERROR("Could not retrieve shader reflection data");
            return false;
        }

        SSource kGLSLSource(pShader->m_kSource.m_pData + uGLSLOffset, pShader->m_kSource.m_uDataSize - uGLSLOffset);
        if (!CompileShader(pShader, &kGLSLSource, &pShader->m_kGLShader, mtlDevice))
        {
            return false;
        }

        return true;
    }

    SPipelineConfiguration::SPipelineConfiguration()
    //  Confetti BEGIN: Igor Lobanchikov
        : m_pVertexDescriptor([[MTLVertexDescriptor alloc] init])
        //  Confetti End: Igor Lobanchikov
    {
        memset(m_apShaders, 0, sizeof(m_apShaders));
    }

    //  Confetti BEGIN: Igor Lobanchikov
    SPipelineConfiguration::~SPipelineConfiguration()
    {
        [(MTLVertexDescriptor*)m_pVertexDescriptor release];
    }

    SPipelineConfiguration::SPipelineConfiguration(const SPipelineConfiguration& other)
    {
        m_pVertexDescriptor = other.m_pVertexDescriptor;
        [(MTLVertexDescriptor*)m_pVertexDescriptor retain];

        memcpy(&m_apShaders, &other.m_apShaders, sizeof(m_apShaders));
        memcpy(&m_AttachmentConfiguration, &other.m_AttachmentConfiguration, sizeof(m_AttachmentConfiguration));
    }

    SPipelineConfiguration& SPipelineConfiguration::operator=(const SPipelineConfiguration& other)
    {
        [(MTLVertexDescriptor*)m_pVertexDescriptor release];
        m_pVertexDescriptor = other.m_pVertexDescriptor;
        [(MTLVertexDescriptor*)m_pVertexDescriptor retain];

        memcpy(&m_apShaders, &other.m_apShaders, sizeof(m_apShaders));
        memcpy(&m_AttachmentConfiguration, &other.m_AttachmentConfiguration, sizeof(m_AttachmentConfiguration));

        return *this;
    }

    void SPipelineConfiguration::SetVertexDescriptor(void* vertexDescriptor)
    {
        [(MTLVertexDescriptor*)m_pVertexDescriptor release];

        m_pVertexDescriptor = [(MTLVertexDescriptor*)vertexDescriptor copyWithZone:nil];
    }
    //  Confetti End: Igor Lobanchikov

    //  Confetti BEGIN: Igor Lobanchikov
    bool GetMetalVertexFormatSize(const SGIFormatInfo* pFormatInfo, uint32& size)
    {
        switch (pFormatInfo->m_pTexture->m_eMetalVertexFormat)
        {
        case MTLVertexFormatInvalid:
            return false;

        case MTLVertexFormatUChar2:
        case MTLVertexFormatChar2:
        case MTLVertexFormatUChar2Normalized:
        case MTLVertexFormatChar2Normalized:
            size = 2;
            break;

        case MTLVertexFormatUChar3:
        case MTLVertexFormatChar3:
        case MTLVertexFormatUChar3Normalized:
        case MTLVertexFormatChar3Normalized:
            size = 3;
            break;

        case MTLVertexFormatUChar4:
        case MTLVertexFormatChar4:
        case MTLVertexFormatUChar4Normalized:
        case MTLVertexFormatChar4Normalized:
        case MTLVertexFormatUShort2:
        case MTLVertexFormatShort2:
        case MTLVertexFormatUShort2Normalized:
        case MTLVertexFormatShort2Normalized:
        case MTLVertexFormatHalf2:
        case MTLVertexFormatFloat:
        case MTLVertexFormatInt:
        case MTLVertexFormatUInt:
        case MTLVertexFormatInt1010102Normalized:
        case MTLVertexFormatUInt1010102Normalized:
            size = 4;
            break;

        case MTLVertexFormatUShort3:
        case MTLVertexFormatShort3:
        case MTLVertexFormatUShort3Normalized:
        case MTLVertexFormatShort3Normalized:
        case MTLVertexFormatHalf3:
            size = 6;
            break;

        case MTLVertexFormatUShort4:
        case MTLVertexFormatShort4:
        case MTLVertexFormatUShort4Normalized:
        case MTLVertexFormatShort4Normalized:
        case MTLVertexFormatHalf4:
        case MTLVertexFormatFloat2:
        case MTLVertexFormatInt2:
        case MTLVertexFormatUInt2:
            size = 8;
            break;

        case MTLVertexFormatFloat3:
        case MTLVertexFormatInt3:
        case MTLVertexFormatUInt3:
            size = 12;
            break;

        case MTLVertexFormatFloat4:
        case MTLVertexFormatInt4:
        case MTLVertexFormatUInt4:
            size = 16;
            break;

        default:
            assert(!"Unknown metal vertex format");
            return false;
        }
        return true;
    }
    //  Confetti End: Igor Lobanchikov


    bool PushInputLayoutAttribute(
        SInputLayout& kLayout,
        uint32 auSlotOffsets[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT],
        const D3D11_INPUT_ELEMENT_DESC& kDesc,
        uint32 uIndex)
    {
        // The format must have texture and uncompressed layout information in order to determine
        // the input layout
        NCryMetal::EGIFormat eGIFormat(GetGIFormat(kDesc.Format));
        const NCryMetal::SGIFormatInfo* pFormatInfo(NULL);
        if (eGIFormat == eGIF_NUM || (pFormatInfo = (GetGIFormatInfo(eGIFormat))) == NULL ||
            pFormatInfo->m_pTexture == NULL || pFormatInfo->m_pUncompressed == NULL)
        {
            DXGL_ERROR("Invalid DXGI format for vertex attribute");
            return false;
        }

        uint32& uSlotOffset(auSlotOffsets[kDesc.InputSlot]);
        uint32 uSize;

        //  Confetti BEGIN: Igor Lobanchikov
        if (pFormatInfo->m_pTexture->m_eMetalVertexFormat == MTLVertexFormatInvalid)
        {
            DXGL_ERROR("Invalid data type for vertex attribute");
            return false;
        }

        GetMetalVertexFormatSize(pFormatInfo, uSize);

        assert(uSize == pFormatInfo->m_pTexture->m_uNumBlockBytes);

        uint32 VBIndex = (DXMETAL_MAX_ENTRIES_BUFFER_ARG_TABLE - 1) - kDesc.InputSlot;
        MTLVertexAttributeDescriptor* MetalDesc = ((MTLVertexDescriptor*)kLayout.m_pVertexDescriptor).attributes[uIndex];
        MetalDesc.bufferIndex = VBIndex;
        MetalDesc.offset = (kDesc.AlignedByteOffset == D3D11_APPEND_ALIGNED_ELEMENT) ? uSize : kDesc.AlignedByteOffset;

        MetalDesc.format = pFormatInfo->m_pTexture->m_eMetalVertexFormat;

        uSlotOffset = max(uSlotOffset, (uint32)MetalDesc.offset + uSize);

        //  Igor: We calculate stride here assuming that the vertex structure won't have any additional padding at the end
        //  which is not 100% true, since DX11 api allows to change vertex stride at vertex buffer bind time.
        //  We override this padding later at vertex buffer bind time anyway. Just keep it here as the reference.
        uint32 stride = ((uSlotOffset + 3) / 4) * 4;
        ((MTLVertexDescriptor*)kLayout.m_pVertexDescriptor).layouts[VBIndex].stride = stride;
        //  Confetti End: Igor Lobanchikov


        return true;
    }

    SInputLayoutPtr CreateInputLayout(const D3D11_INPUT_ELEMENT_DESC* pInputElementDescs, uint32 uNumElements, const TShaderReflection& kReflection)
    {
        SInputLayoutPtr spLayout(new SInputLayout());
        uint32 auSlotOffsets[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
        memset(auSlotOffsets, 0, sizeof(auSlotOffsets));
        
        int attrMatchedNum = 0;
        int reflectionAttrNum = 0;
        
        //Calculate the number of attributes in the vertex shader that dont start with 'SV_'.
        TShaderReflection::TParameters::const_iterator kInputSemanticIter(kReflection.m_kInputs.begin());
        const TShaderReflection::TParameters::const_iterator kInputSemanticEnd(kReflection.m_kInputs.end());
        while (kInputSemanticIter != kInputSemanticEnd)
        {
            if(strstr(kInputSemanticIter->m_kDesc.SemanticName, "SV_") == 0)
            {
                reflectionAttrNum++;
            }
            ++kInputSemanticIter;
        }

        for (uint32 uElement = 0; uElement < uNumElements; ++uElement)
        {
            const D3D11_INPUT_ELEMENT_DESC& kDesc(pInputElementDescs[uElement]);

            //  Confetti BEGIN: Igor Lobanchikov
            {
                uint32 VBIndex = (DXMETAL_MAX_ENTRIES_BUFFER_ARG_TABLE - 1) - kDesc.InputSlot;

                //  TODO: do check here so that all values for the same buffer has the same parameters

                //  Igor: set layout here
                // Don't set stride here. Set it when the actual buffer is bound, since in dx11 this is the place wherer we know the stride
                // Initial auto-calculated stride value will be set once the input layout is complete
                //spLayout->layouts[VBIndex].stride = kDesk.;
                ((MTLVertexDescriptor*)spLayout->m_pVertexDescriptor).layouts[VBIndex].stepFunction = (kDesc.InputSlotClass == D3D11_INPUT_PER_VERTEX_DATA) ? MTLVertexStepFunctionPerVertex : MTLVertexStepFunctionPerInstance;
                ((MTLVertexDescriptor*)spLayout->m_pVertexDescriptor).layouts[VBIndex].stepRate = (kDesc.InputSlotClass == D3D11_INPUT_PER_VERTEX_DATA) ? 1 : kDesc.InstanceDataStepRate;
            }
            //  Confetti End: Igor Lobanchikov

            
            // Find a match within the input signatures of the shader reflection data
            TShaderReflection::TParameters::const_iterator kInputSemanticIter(kReflection.m_kInputs.begin());
            const TShaderReflection::TParameters::const_iterator kInputSemanticEnd(kReflection.m_kInputs.end());
            while (kInputSemanticIter != kInputSemanticEnd)
            {
                if (kInputSemanticIter->m_kDesc.SemanticIndex == kDesc.SemanticIndex &&
                    strcmp(kInputSemanticIter->m_kDesc.SemanticName, kDesc.SemanticName) == 0)
                {
                    if (!PushInputLayoutAttribute(*spLayout, auSlotOffsets, kDesc, kInputSemanticIter->m_kDesc.Register))
                    {
                        AZ_Assert(false, "Failed to Push Input Layout");
                        return NULL;
                    }
                    attrMatchedNum++;
                    break;
                }
                ++kInputSemanticIter;
            }
        }

        if( attrMatchedNum != reflectionAttrNum)
        {
            AZ_Assert(false, "Shader input attributes count doesn not match with vertex format attributes count");
            return NULL;
        }

        return spLayout;
    }
}
