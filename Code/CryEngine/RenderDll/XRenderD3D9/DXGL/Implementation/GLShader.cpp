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

// Description : Implements the shader related functions


#include "RenderDll_precompiled.h"
#include "GLShader.hpp"
#include "GLDevice.hpp"
#include <Common/RenderCapabilities.h>

struct SAutoBindProgram
{
    SAutoBindProgram(const GLuint uProgram)
    {
        GLint iBinding;
        glGetIntegerv(GL_CURRENT_PROGRAM, &iBinding);
        m_uPreviousBinding = (GLuint)iBinding;
        glUseProgram(uProgram);
    }

    ~SAutoBindProgram()
    {
        glUseProgram(m_uPreviousBinding);
    }

    GLuint m_uPreviousBinding;
};

#if DXGL_GLSL_FROM_HLSL2GLSL && DXGL_SUPPORT_NSIGHT_SINCE(4_1)
// NSight shader instrumentation throws an exception when compiling shaders with functions with many parameters.
// Shaders generated with HLSL2GLSL are simplified using glslOptimizer to exploit its function inlining and simplify instrumentation.
#include "../../Tools/glslOptimizer/src/glsl/glsl_optimizer.h"
struct SSimplifiedShader
{
    glslopt_shader* m_pOptimizedShader;

    SSimplifiedShader()
        : m_pOptimizedShader(NULL)
    {
    }

    ~SSimplifiedShader()
    {
        if (m_pOptimizedShader != NULL)
        {
            glslopt_shader_delete(m_pOptimizedShader);
        }
    }

    const char* Simplify(const char* szSource, NCryOpenGL::EShaderType eType)
    {
        struct SAutoContext
        {
            glslopt_ctx* m_pContext;
            SAutoContext() { m_pContext = glslopt_initialize(kGlslTargetOpenGL); }
            ~SAutoContext() { glslopt_cleanup(m_pContext); }
        };

        glslopt_shader_type eOptType;
        switch (eType)
        {
        case NCryOpenGL::eST_Vertex:
            eOptType = kGlslOptShaderVertex;
            break;
        case NCryOpenGL::eST_Fragment:
            eOptType = kGlslOptShaderFragment;
            break;
        default:
            DXGL_ERROR("Invalid shader type for glslOptimizer");
            return NULL;
        }

        static SAutoContext s_kContext;
        m_pOptimizedShader = glslopt_optimize(s_kContext.m_pContext, eOptType, szSource, 0);

        if (!glslopt_get_status(m_pOptimizedShader))
        {
            DXGL_ERROR("Error during glslOptimizer execution: %s", glslopt_get_log(m_pOptimizedShader));
            m_pOptimizedShader = NULL;
            return NULL;
        }

        return glslopt_get_output (m_pOptimizedShader);
    }
};
#endif

namespace NCryOpenGL
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
        : m_uName(0)
        , m_separableProgramsSupported(false)
    {
    }

    SGLShader::~SGLShader()
    {
        if (m_uName != 0)
        {
            if (m_separableProgramsSupported)
            {
                glDeleteProgram(m_uName);
            }
            else
            {
                glDeleteShader(m_uName);
            }
        }
    }

    SShader::SShader()
        : m_eType(eST_NUM)
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
#if DXGL_ENABLE_SHADER_TRACING
        , m_uTraceBufferUnit(0)
#endif //DXGL_ENABLE_SHADER_TRACING
    {
    }

    SPipeline::~SPipeline()
    {
        if (m_pContext->GetDevice()->IsFeatureSupported(eF_SeparablePrograms))
        {
            glDeleteProgramPipelines(1, &m_uName);
        }
        else
        {
            glDeleteProgram(m_uName);
        }
    }

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

    bool InitializeShaderReflection(SShaderReflection* pReflection, const void* pvData, uint32 uSize)
    {
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

        uint32 uOffset;
        for (uint32 uChunk = 0; uChunk < uNumChunks; ++uChunk)
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
                if (!DXBCReadUint32(kChunkContext, pReflection->m_uNumSamplers) ||
                    !DXBCReadUint32(kChunkContext, pReflection->m_uNumImages) ||
                    !DXBCReadUint32(kChunkContext, pReflection->m_uNumStorageBuffers) ||
                    !DXBCReadUint32(kChunkContext, pReflection->m_uNumUniformBuffers) ||
                    !DXBCReadUint32(kChunkContext, pReflection->m_uImportsSize) ||
                    !DXBCReadUint32(kChunkContext, pReflection->m_uExportsSize) ||
                    !DXBCReadUint32(kChunkContext, pReflection->m_uInputHash) ||
                    !DXBCReadUint32(kChunkContext, pReflection->m_uSymbolsOffset))
                {
                    return false;
                }

                uOffset = (uint32)(kChunkContext.m_pIter - kContext.m_pBegin);
                pReflection->m_uSamplersOffset       = uOffset;
                uOffset += GLSL_SAMPLER_SIZE  * pReflection->m_uNumSamplers;
                pReflection->m_uImagesOffset         = uOffset;
                uOffset += GLSL_RESOURCE_SIZE * pReflection->m_uNumImages;
                pReflection->m_uStorageBuffersOffset = uOffset;
                uOffset += GLSL_RESOURCE_SIZE * pReflection->m_uNumStorageBuffers;
                pReflection->m_uUniformBuffersOffset = uOffset;
                uOffset += GLSL_RESOURCE_SIZE * pReflection->m_uNumUniformBuffers;
                pReflection->m_uImportsOffset        = uOffset;
                uOffset += GLSL_SYMBOL_SIZE   * pReflection->m_uImportsSize;
                pReflection->m_uExportsOffset        = uOffset;
                uOffset += GLSL_SYMBOL_SIZE   * pReflection->m_uExportsSize;
                pReflection->m_uGLSLSourceOffset     = uOffset;
                break;
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


#if !DXGL_INPUT_GLSL

    struct SGLSLConversionOutput
    {
#if DXGL_GLSL_FROM_HLSLCROSSCOMPILER
        GLSLShader m_kGLSLShader;
#endif //DXGL_GLSL_FROM_HLSLCROSSCOMPILER

        SGLSLConversionOutput()
        {
#if DXGL_GLSL_FROM_HLSLCROSSCOMPILER
            memset(&m_kGLSLShader, 0, sizeof(m_kGLSLShader));
#endif //DXGL_GLSL_FROM_HLSLCROSSCOMPILER
        }

        ~SGLSLConversionOutput()
        {
#if DXGL_GLSL_FROM_HLSLCROSSCOMPILER
            FreeGLSLShader(&m_kGLSLShader);
#endif //DXGL_GLSL_FROM_HLSLCROSSCOMPILER
        }
    };

    bool ConvertDXBCToGLSL(const void* pvSourceData, size_t uSourceSize, EShaderVersion eVersion, SGLSLConversionOutput* pOutput)
    {
#if DXGL_GLSL_FROM_HLSLCROSSCOMPILER
        unsigned int uFlags(HLSLCC_FLAG_UNIFORM_BUFFER_OBJECT);
        GlExtensions kExtensions = {0};

#if CRY_OPENGL_ADAPT_CLIP_SPACE
        uFlags |= HLSLCC_FLAG_CONVERT_CLIP_SPACE_Z;
    #if CRY_OPENGL_FLIP_Y
        DXGL_TODO("Only do this if the shader is expected to be used to render into a texture so that a final blit for flipping is not required")
        uFlags |= HLSLCC_FLAG_INVERT_CLIP_SPACE_Y;
    #else
        uFlags |= HLSLCC_FLAG_ORIGIN_UPPER_LEFT;
    #endif
#endif //CRY_OPENGL_ADAPT_CLIP_SPACE

        DXGL_TODO("Currently we always specify bindings at run-time (slot ranges are determined during startup), and leave the default location for resources (mainly to prevent collisions between resources with the same register index in different non-separable shader stages). Evaluate the possibility of coding them into the shader source.");
        uFlags |= HLSLCC_FLAG_AVOID_RESOURCE_BINDINGS_AND_LOCATIONS;

        DXGL_TODO("This is currently the only correct setting as register aliasing is buggy when branches are present.");
        uFlags |= HLSLCC_FLAG_AVOID_TEMP_REGISTER_ALIASING;

#if DXGL_HLSL_TO_GLSL_DEBUG
        uFlags |= HLSLCC_FLAG_HASH_INPUT;
        uFlags |= HLSLCC_FLAG_ADD_DEBUG_HEADER;
#endif //DXGL_HLSL_TO_GLSL_DEBUG

#if DXGL_ENABLE_SHADER_TRACING
        if (eVersion == eSV_Tracing)
        {
            uFlags |= HLSLCC_FLAG_TRACING_INSTRUMENTATION;
        }
#endif //DXGL_ENABLE_SHADER_TRACING

        // Some drivers (e.g. AMD Catalyst) have issues with certain version strings so we append a different version string at runtime as workaround
        uFlags |= HLSLCC_FLAG_NO_VERSION_STRING;

        //#if DXGLES
        //  uFlags |= HLSLCC_FLAG_INOUT_SEMANTIC_NAMES;
        //#endif

        GLLang eLanguage;
#if DXGL_REQUIRED_VERSION >= DXGL_VERSION_44
        eLanguage = LANG_440;
#elif DXGL_REQUIRED_VERSION >= DXGL_VERSION_43
        eLanguage = LANG_430;
#elif DXGL_REQUIRED_VERSION >= DXGL_VERSION_42
        eLanguage = LANG_420;
#elif DXGL_REQUIRED_VERSION >= DXGL_VERSION_41
        eLanguage = LANG_410;
#elif DXGLES
        uint32 glVersion = RenderCapabilities::GetDeviceGLVersion();
        if (glVersion == DXGLES_VERSION_30)
        {
            eLanguage = LANG_ES_300;
        }
        else
        {
            eLanguage = LANG_ES_310;
        }
#else
    #error "Shading language revision not defined for this GL version"
#endif

        HLSLcc_SetMemoryFunctions(&Malloc, &Calloc, &Free, &Realloc);

        if (TranslateHLSLFromMem(static_cast<const char*>(pvSourceData), uSourceSize, uFlags, eLanguage, &kExtensions, &pOutput->m_kGLSLShader) != 1)
        {
            DXGL_ERROR("HLSL to GLSL conversion failed");
            return false;
        }

#else
#pragma error "Not implemented on this configuration"
#endif

        return true;
    }

    bool InitializeTranslatedShaderSource(SShaderSource* pSource, const void* pvDXBCData, size_t uDXBCSize, const SGLSLConversionOutput& kConversionOutput)
    {
#if DXGL_GLSL_FROM_HLSLCROSSCOMPILER
        const uint8* puDXBC(static_cast<const uint8*>(pvDXBCData));

        SDXBCInputBuffer kInputBuffer(puDXBC, puDXBC + uDXBCSize);
        size_t uCombinedSize(DXBCGetCombinedSize(kInputBuffer, &kConversionOutput.m_kGLSLShader));
        if (uCombinedSize == 0 || uCombinedSize > 0xFFFFFFFF)
        {
            return false;
        }

        uint8* puCombined(static_cast<uint8*>(Malloc(uCombinedSize)));
        pSource->m_pData = alias_cast<const char*>(puCombined);
        pSource->m_uDataSize = (uint32)uCombinedSize;

        kInputBuffer = SDXBCInputBuffer(puDXBC, puDXBC + uDXBCSize);
        SDXBCOutputBuffer kOutputBuffer(puCombined, puCombined + uCombinedSize);
        return DXBCCombineWithGLSL(kInputBuffer, kOutputBuffer, &kConversionOutput.m_kGLSLShader);
#else
#pragma error "Not implemented on this configuration"
#endif
    }

#endif // !DXGL_INPUT_GLSL

    bool InitializeShaderReflectionFromInput(SShaderReflection* pReflection, const void* pvInputData)
    {
        enum
        {
            SIZE_POSITION = 6
        };
        uint32 uInputDataSize(alias_cast<const uint32*>(pvInputData)[SIZE_POSITION]);
#if DXGL_INPUT_GLSL
        return InitializeShaderReflection(pReflection, pvInputData, uInputDataSize);
#else
        SShaderSource kTempShaderSource;
        SGLSLConversionOutput kTempConversionOutput;
        return
            ConvertDXBCToGLSL(pvInputData, uInputDataSize, eSV_Normal, &kTempConversionOutput) &&
            InitializeTranslatedShaderSource(&kTempShaderSource, pvInputData, uInputDataSize, kTempConversionOutput) &&
            InitializeShaderReflection(pReflection, kTempShaderSource.m_pData, kTempShaderSource.m_uDataSize);
#endif
    }


    bool CompileShaderFromSources(SGLShader& shader, const GLchar* aszSources[], GLint aiSourceLengths[], uint32 uNumSources, GLenum eGLShaderType, bool isSeparableProgramsSupported)
    {
        shader.m_separableProgramsSupported = isSeparableProgramsSupported;
        if (isSeparableProgramsSupported)
        {
            shader.m_uName = glCreateShaderProgramv(eGLShaderType, uNumSources, aszSources);

            if (!shader.m_uName)
            {
                return false;
            }

            if (!VerifyProgramStatus(shader.m_uName, GL_LINK_STATUS))
            {
                return false;
            }
        }
        else
        {
            shader.m_uName = glCreateShader(eGLShaderType);
            if (!shader.m_uName)
            {
                return false;
            }
            glShaderSource(shader.m_uName, uNumSources, aszSources, aiSourceLengths);
            glCompileShader(shader.m_uName);

            if (!VerifyShaderStatus(shader.m_uName, GL_COMPILE_STATUS))
            {
                return false;
            }

#if defined(glReleaseShaderCompiler)
            DXGL_TODO("Check whether this is actually useful")
            glReleaseShaderCompiler();
#endif //defined(glReleaseShaderCompiler)
        }
        return true;
    }

    SShader::SVersion& GetPipelineShaderVersion(const SPipelineConfiguration& kPipelineConfig, EShaderType eStage)
    {
#if DXGL_ENABLE_SHADER_TRACING
        EShaderVersion eVersion(kPipelineConfig.m_aeShaderVersions[eStage]);
#else
        EShaderVersion eVersion(eSV_Normal);
#endif
        return kPipelineConfig.m_apShaders[eStage]->m_akVersions[eVersion];
    }

    bool FindShaderSymbolByType(const uint32* puSymbols, uint32 uNumSymbols, SYMBOL_TYPE eType, uint32* puID, uint32* puValue)
    {
        const uint32* puSymbolsEnd = puSymbols + uNumSymbols * 3;
        for (const uint32* puSymbol = puSymbols; puSymbol != puSymbolsEnd; puSymbol += 3)
        {
            if ((SYMBOL_TYPE)puSymbol[0] == eType)
            {
                *puID = puSymbol[1];
                *puValue = puSymbol[2];
                return true;
            }
        }
        return false;
    }

    bool FindShaderSymbolByTypeAndID(const uint32* puSymbols, uint32 uNumSymbols, SYMBOL_TYPE eType, uint32 uID, uint32* puValue)
    {
        const uint32* puSymbolsEnd = puSymbols + uNumSymbols * 3;
        for (const uint32* puSymbol = puSymbols; puSymbol != puSymbolsEnd; puSymbol += 3)
        {
            if ((SYMBOL_TYPE)puSymbol[0] == eType && puSymbol[1] == uID)
            {
                *puValue = puSymbol[2];
                return true;
            }
        }
        return false;
    }

    template <uint32 uMaxDigits>
    uint32 WriteShaderDecimal(char* acOutput, uint32 uValue)
    {
        char acDigits[uMaxDigits];
        char* pcDigit(acDigits + uMaxDigits - 1);
        while (true)
        {
            uint32 uNext(uValue / 10);
            uint32 uDigit(uValue % 10);
            *pcDigit = '0' + uDigit;
            if (uNext == 0 || pcDigit == acDigits)
            {
                break;
            }
            uValue = uNext;
            --pcDigit;
        }
        uint32 uNumDigits((uint32)(acDigits + uMaxDigits - pcDigit));
        memcpy(acOutput, pcDigit, uNumDigits);
        return uNumDigits;
    }

    void BuildVersionString(SPipelineCompilationBuffer* pBuffer, CDevice* pDevice)
    {
#if DXGL_REQUIRED_VERSION >= DXGL_VERSION_44
        const char* szVersion = "#version 440\n";
        // Workaround for AMD Catalyst - as of version 15.7.1 using "#version 440" causes std140 uniform buffers
        // to have random layouts (see https://community.amd.com/thread/185272)
        if (pDevice->GetAdapter()->m_eDriverVendor == RenderCapabilities::s_gpuVendorIdAMD)
        {
            szVersion = "#version 430\n";
        }
#elif DXGL_REQUIRED_VERSION >= DXGL_VERSION_43
        const char* szVersion = "#version 430\n";
#elif DXGL_REQUIRED_VERSION >= DXGL_VERSION_42
        const char* szVersion = "#version 420\n";
#elif DXGL_REQUIRED_VERSION >= DXGL_VERSION_41
        const char* szVersion = "#version 410\n";
#elif DXGLES
        uint32 glVersion = RenderCapabilities::GetDeviceGLVersion();
        const char* szVersion;
        if (glVersion == DXGLES_VERSION_30)
        {
            szVersion = "#version 300 es\n";
        }
        else
        {
            szVersion = "#version 310 es\n";
        }
#else
    #error "Shading language revision not defined for this GL version"
#endif
        pBuffer->m_iVersionStringLength = min(strlen(szVersion), sizeof(pBuffer->m_acVersionString));
        memcpy(pBuffer->m_acVersionString, szVersion, pBuffer->m_iVersionStringLength);
    }

    void BuildShaderImportHeader(SPipelineCompilationBuffer* pBuffer, const uint32* auImports, uint32 uNumImports, const SPipelineConfiguration& kConfiguration)
    {
        static const char acImportPrefix[] = "#define IMPORT_";
        enum
        {
            IMPORT_INDEX_OFFSET = sizeof(acImportPrefix) - 1,
            IMPORT_INDEX_MAX_DIGITS = 10,
            IMPORT_VALUE_OFFSET = IMPORT_INDEX_OFFSET + IMPORT_INDEX_MAX_DIGITS + 1,
            IMPORT_VALUE_MAX_DIGITS = 10,
            IMPORT_NEWLINE_OFFSET = IMPORT_VALUE_OFFSET + IMPORT_VALUE_MAX_DIGITS,
            IMPORT_STRIDE = IMPORT_NEWLINE_OFFSET + 1
        };

        if (pBuffer->m_uImportsHeaderCapacity < uNumImports)
        {
            pBuffer->m_acImportsHeader = static_cast<char*>(Realloc(pBuffer->m_acImportsHeader, IMPORT_STRIDE * uNumImports));
            memset(pBuffer->m_acImportsHeader + IMPORT_STRIDE * pBuffer->m_uImportsHeaderCapacity, (int)' ', IMPORT_STRIDE * (uNumImports - pBuffer->m_uImportsHeaderCapacity));
            for (uint32 uImport = pBuffer->m_uImportsHeaderCapacity; uImport < uNumImports; ++uImport)
            {
                char* acIter(pBuffer->m_acImportsHeader + IMPORT_STRIDE * uImport);
                memcpy(acIter, acImportPrefix, IMPORT_INDEX_OFFSET);
                WriteShaderDecimal<IMPORT_INDEX_MAX_DIGITS>(acIter + IMPORT_INDEX_OFFSET, uImport);
                acIter[IMPORT_NEWLINE_OFFSET] = '\n';
            }
            pBuffer->m_uImportsHeaderCapacity = uNumImports;
        }

        pBuffer->m_uImportsBeginOffset = 0;
        pBuffer->m_uImportsEndOffset = 0;

        for (uint32 uImport = 0; uImport < uNumImports; ++uImport)
        {
            SYMBOL_TYPE eType((SYMBOL_TYPE)auImports[uImport * 3]);
            uint32 uID(auImports[uImport * 3 + 1]);
            uint32 uDefaultValue(auImports[uImport * 3 + 2]);

            uint32 uReqValue = uDefaultValue;
            switch (eType)
            {
#if DXGL_SUPPORT_TESSELLATION
            case SYMBOL_TESSELLATOR_PARTITIONING:
            case SYMBOL_TESSELLATOR_OUTPUT_PRIMITIVE:
                if (kConfiguration.m_apShaders[eST_TessControl])
                {
                    const SShader::SVersion& kLinkVersion(GetPipelineShaderVersion(kConfiguration, eST_TessControl));
                    const uint32* auLinkExports(alias_cast<const uint32*>(kLinkVersion.m_kSource.m_pData + kLinkVersion.m_kReflection.m_uExportsOffset));
                    uint32 uNumLinkExports(kLinkVersion.m_kReflection.m_uExportsSize);

                    uint32 uExportID;
                    if (!FindShaderSymbolByType(auLinkExports, uNumLinkExports, eType, &uExportID, &uReqValue))
                    {
                        uReqValue = uDefaultValue;
                    }
                }
                break;
#endif //!DXGL_SUPPORT_TESSELLATION
            case SYMBOL_INPUT_INTERPOLATION_MODE:
                if (kConfiguration.m_apShaders[eST_Fragment])
                {
                    const SShader::SVersion& kLinkVersion(GetPipelineShaderVersion(kConfiguration, eST_Fragment));
                    const uint32* auLinkExports(alias_cast<const uint32*>(kLinkVersion.m_kSource.m_pData + kLinkVersion.m_kReflection.m_uExportsOffset));
                    uint32 uNumLinkExports(kLinkVersion.m_kReflection.m_uExportsSize);

                    if (!FindShaderSymbolByTypeAndID(auLinkExports, uNumLinkExports, SYMBOL_INPUT_INTERPOLATION_MODE, uID, &uReqValue))
                    {
                        uReqValue = uDefaultValue;
                    }
                }
                break;
            case SYMBOL_EMULATE_DEPTH_CLAMP:
#if DXGL_SUPPORT_DEPTH_CLAMP
                uReqValue = 0;
#else
                uReqValue = kConfiguration.m_bEmulateDepthClamp;
#endif
                break;
            default:
                DXGL_ERROR("Unexpected shader import type");
                break;
            }

            if (uReqValue != uDefaultValue || pBuffer->m_uImportsEndOffset > 0)
            {
                char* acIter(pBuffer->m_acImportsHeader + IMPORT_STRIDE * uImport + IMPORT_VALUE_OFFSET);
                uint32 uNumDigits(WriteShaderDecimal<IMPORT_VALUE_MAX_DIGITS>(acIter, uReqValue));
                if (uNumDigits < IMPORT_VALUE_MAX_DIGITS)
                {
                    memset(acIter + uNumDigits, ' ', IMPORT_VALUE_MAX_DIGITS - uNumDigits);
                }
                if (pBuffer->m_uImportsEndOffset == 0)
                {
                    pBuffer->m_uImportsBeginOffset = uImport * IMPORT_STRIDE;
                }
                pBuffer->m_uImportsEndOffset = (uImport + 1) * IMPORT_STRIDE;
            }
        }
    }

#if DXGL_DUMP_GLSL_SOURCES

    void DumpShaderSources(const GLchar** aszSources, const GLint* aiSourceLengths, uint32 uNumSources, uint32 uHash, uint32 uVersion)
    {
        STraceFile kTraceFile;
        char acBuffer[32];
        if (uVersion == 0)
        {
            sprintf_s(acBuffer, "0x%08X.txt", uHash);
        }
        else
        {
            sprintf_s(acBuffer, "0x%08X_%1d.txt", uHash, uVersion);
        }
        if (!kTraceFile.Open(acBuffer, false))
        {
            DXGL_ERROR("Could not open file \"%s\" for writing", acBuffer);
            return;
        }
        for (uint32 uSource = 0; uSource < uNumSources; ++uSource)
        {
            kTraceFile.Write(aszSources[uSource], aiSourceLengths[uSource] - 1);
        }
    }

#endif //DXGL_DUMP_GLSL_SOURCES

    bool CompilePipeline(SPipeline* pPipeline, SPipelineCompilationBuffer* pBuffer, CDevice* pDevice)
    {
        bool isSeparableProgramsSupported = pDevice->IsFeatureSupported(eF_SeparablePrograms);
        if (isSeparableProgramsSupported)
        {
            glGenProgramPipelines(1, &pPipeline->m_uName);
        }
        else
        {
            pPipeline->m_uName = glCreateProgram();
        }

        for (uint32 uShader = 0; uShader < eST_NUM; ++uShader)
        {
            SShader* pShader(pPipeline->m_kConfiguration.m_apShaders[uShader]);

            if (IsPipelineStageUsed(pPipeline->m_kConfiguration.m_eMode, (EShaderType)uShader) && pShader != NULL)
            {
                SShader::SVersion& kVersion(GetPipelineShaderVersion(pPipeline->m_kConfiguration, (EShaderType)uShader));
                GLenum eGLShaderType;
                if (!GetGLShaderType(eGLShaderType, (EShaderType)uShader))
                {
                    return false;
                }

                if (pBuffer->m_iVersionStringLength < 0)
                {
                    BuildVersionString(pBuffer, pDevice);
                }

                uint32 uGLSLOffset(kVersion.m_kReflection.m_uGLSLSourceOffset);
                uint32 uSymbolsOffset(kVersion.m_kReflection.m_uSymbolsOffset);
                SSource kGLSLSource(kVersion.m_kSource.m_pData + uGLSLOffset, kVersion.m_kSource.m_uDataSize - uGLSLOffset);
                const uint32* auImports(alias_cast<const uint32*>(kVersion.m_kSource.m_pData + kVersion.m_kReflection.m_uImportsOffset));
                uint32 uNumImports(kVersion.m_kReflection.m_uImportsSize);
                BuildShaderImportHeader(pBuffer, auImports, uNumImports, pPipeline->m_kConfiguration);

                const GLchar* aszSources[4];
                GLint aiSourceLengths[4];
                uint32 uNumSources = 0;

                aszSources[uNumSources]      = pBuffer->m_acVersionString;
                aiSourceLengths[uNumSources] = pBuffer->m_iVersionStringLength;
                ++uNumSources;

                if (pBuffer->m_uImportsEndOffset > pBuffer->m_uImportsBeginOffset)
                {
                    if (uSymbolsOffset > 0)
                    {
                        aszSources[uNumSources]      = kGLSLSource.m_pData;
                        aiSourceLengths[uNumSources] = static_cast<GLint>(uSymbolsOffset);
                        ++uNumSources;
                    }

                    aszSources[uNumSources]      = pBuffer->m_acImportsHeader + pBuffer->m_uImportsBeginOffset;
                    aiSourceLengths[uNumSources] = static_cast<GLint>(pBuffer->m_uImportsEndOffset - pBuffer->m_uImportsBeginOffset);
                    ++uNumSources;

                    aszSources[uNumSources]      = kGLSLSource.m_pData + uSymbolsOffset;
                    aiSourceLengths[uNumSources] = static_cast<GLint>(kGLSLSource.m_uDataSize - 1 - uSymbolsOffset);
                    ++uNumSources;
                }
                else
                {
                    aszSources[uNumSources]      = kGLSLSource.m_pData;
                    aiSourceLengths[uNumSources] = static_cast<GLint>(kGLSLSource.m_uDataSize - 1);
                    ++uNumSources;
                }

#if DXGL_DUMP_GLSL_SOURCES
                DumpShaderSources(
                    aszSources + uFirstSource,
                    aiSourceLengths + uFirstSource,
                    uNumSources,
                    kVersion.m_kReflection.m_uInputHash,
                    pPipeline->m_kConfiguration.m_aeShaderVersions[uShader]);
#endif //DXGL_DUMP_GLSL_SOURCES

                if (!CompileShaderFromSources(
                        pPipeline->m_akGLShaders[uShader],
                        aszSources,
                        aiSourceLengths,
                        uNumSources,
                        eGLShaderType,
                        isSeparableProgramsSupported))
                {
                    DXGL_ERROR("Shader 0x%08X failed to compile", kVersion.m_kReflection.m_uInputHash);
                    return false;
                }

                if (isSeparableProgramsSupported)
                {
                    GLbitfield uProgramStageBit(0);
                    if (!GetGLProgramStageBit(uProgramStageBit, pShader->m_eType))
                    {
                        return false;
                    }
                    glUseProgramStages(pPipeline->m_uName, uProgramStageBit, pPipeline->m_akGLShaders[uShader].m_uName);
                }
                else
                {
                    glAttachShader(pPipeline->m_uName, pPipeline->m_akGLShaders[uShader].m_uName);
                }
            }
        }

        if (!isSeparableProgramsSupported)
        {
            glLinkProgram(pPipeline->m_uName);

            if (!VerifyProgramStatus(pPipeline->m_uName, GL_LINK_STATUS))
            {
                CryLogAlways("Shader did not link properly !! ");
                return false;
            }
        }

        return true;
    }

    bool InitializeShader(SShader* pShader, const void* pvSourceData, size_t uSourceSize, const GLchar* szSourceOverride)
    {
#if DXGL_INPUT_GLSL
        SShader::SVersion& kVersion(pShader->m_akVersions[eSV_Normal]);
        kVersion.m_kSource.SetData(reinterpret_cast<const char*>(pvSourceData), uSourceSize);
#else
        SGLSLConversionOutput akConversionOutputs[eSV_NUM];
        for (uint32 uVersion = 0; uVersion < eSV_NUM; ++uVersion)
        {
            SShader::SVersion& kVersion(pShader->m_akVersions[uVersion]);

            if (!ConvertDXBCToGLSL(pvSourceData, uSourceSize, (EShaderVersion)uVersion, akConversionOutputs + uVersion) ||
                !InitializeTranslatedShaderSource(&kVersion.m_kSource, pvSourceData, uSourceSize, akConversionOutputs[uVersion]))
            {
                return false;
            }
#endif

        uint32 uGLSLOffset;
        if (!InitializeShaderReflection(&kVersion.m_kReflection, kVersion.m_kSource.m_pData, kVersion.m_kSource.m_uDataSize) ||
            (uGLSLOffset = kVersion.m_kReflection.m_uGLSLSourceOffset) >= (uint32)uSourceSize)
        {
            DXGL_ERROR("Could not retrieve shader reflection data");
            return false;
        }

#if !DXGL_INPUT_GLSL
    }
#endif

#if DXGL_ENABLE_SHADER_TRACING
        const ShaderInfo* pTracingInfo(&akConversionOutputs[eSV_Tracing].m_kGLSLShader.reflection);
        uint32 uOffset = 0;
        for (uint32 uStep = 0; uStep < pTracingInfo->ui32NumTraceSteps; ++uStep)
        {
            const StepTraceInfo* pStep(pTracingInfo->psTraceSteps + uStep);
            for (uint32 uVariable = 0; uVariable < pStep->ui32NumVariables; ++uVariable)
            {
                pShader->m_kTraceIndex.m_kTraceVariables.push_back(pStep->psVariables[uVariable]);
            }
            pShader->m_kTraceIndex.m_kTraceStepOffsets.push_back(uOffset);
            pShader->m_kTraceIndex.m_kTraceStepSizes.push_back(pStep->ui32NumVariables);
            uOffset += pStep->ui32NumVariables;
        }
#endif //DXGL_ENABLE_SHADER_TRACING

        return true;
    }

    GLuint GetProgramName(SPipeline* pPipeline, SShader* pShader)
    {
        if (!pPipeline->m_pContext->GetDevice()->IsFeatureSupported(eF_SeparablePrograms))
        {
            return pPipeline->m_uName;
        }

        return pPipeline->m_akGLShaders[pShader->m_eType].m_uName;
    }

    bool BindUniformBufferToUnit(GLuint uProgramName, uint32 uUnit, const char* szName, CContext* pContext)
    {
        GLuint uUniformBlockIndex(glGetUniformBlockIndex(uProgramName, szName));
        if (uUniformBlockIndex == GL_INVALID_INDEX)
        {
            DXGL_WARNING("Could not find uniform buffer \"%s\" in program, probably optimized out by the GLSL compiler, ignoring binding", szName);
            return true;
        }

        glUniformBlockBinding(uProgramName, uUniformBlockIndex, uUnit);
        return true;
    }

#if DXGL_SUPPORT_SHADER_STORAGE_BLOCKS && !DXGLES
    // GLES doesn't have the function glShaderStorageBlockBinding.
    // Binding locations for shader storage is specified in the shader and can't be change.
    bool BindStorageBufferToUnit(GLuint uProgramName, uint32 uUnit, const char* szName, CContext* pContext)
    {
        GLuint uStorageBlockIndex(glGetProgramResourceIndex(uProgramName, GL_SHADER_STORAGE_BLOCK, szName));
        if (uStorageBlockIndex == GL_INVALID_INDEX)
        {
            DXGL_WARNING("Could not find storage buffer \"%s\" in program, probably optimized out by the GLSL compiler, ignoring binding", szName);
            return true;
        }

        glShaderStorageBlockBinding(uProgramName, uStorageBlockIndex, uUnit);
        return true;
    }
#endif //DXGL_SUPPORT_SHADER_STORAGE_BLOCKS && !DXGLES

    bool BindUniformToUnit(GLuint uProgramName, uint32 uUnit, const char* szName, CContext* pContext)
    {
        GLint iUniformBlockIndex(glGetUniformLocation(uProgramName, szName));
        if (iUniformBlockIndex < 0)
        {
            DXGL_WARNING("Could not find uniform \"%s\" in program, probably optimized out by the GLSL compiler, ignoring binding", szName);
            return true;
        }

        if (pContext->GetDevice()->IsFeatureSupported(eF_SeparablePrograms))
        {
            if (iUniformBlockIndex >= 0)
            {
                glProgramUniform1i(uProgramName, iUniformBlockIndex, uUnit);
            }
        }
        else
        {
            SAutoBindProgram kBindProgram(uProgramName);
            if (iUniformBlockIndex >= 0)
            {
                glUniform1i(iUniformBlockIndex, uUnit);
            }
        }

        return true;
    }

#if DXGL_ENABLE_SHADER_TRACING

    uint32 FindUnusedShaderStorageBufferUnit(SPipeline* pPipeline, CDevice* pDevice)
    {
        SBitMask<MAX_STORAGE_BUFFER_UNITS, SUnsafeBitMaskWord> kUsedUnitsMask;

        SUnitMap* pStorageBufferMap = pPipeline->m_aspResourceUnitMaps[eRUT_StorageBuffer];
        if (pStorageBufferMap == NULL)
        {
            return 0;
        }

        const SUnitMap::SElement* pElementsBegin = pStorageBufferMap->m_akUnits;
        const SUnitMap::SElement* pElementsEnd   = pElementsBegin + pStorageBufferMap->m_uNumUnits;
        for (const SUnitMap::SElement* pElement = pElementsBegin; pElement < pElementsEnd; ++pElement)
        {
            kUsedUnitsMask.Set(pElement->GetResourceUnit(), true);
        }

        uint32 uNumSupportedStorageSlots((uint32)pDevice->GetAdapter()->m_kCapabilities.m_akResourceUnits[eRUT_StorageBuffer].m_aiMaxTotal);
        for (uint32 uSlot = 0; uSlot < uNumSupportedStorageSlots; ++uSlot)
        {
            if (!kUsedUnitsMask.Get(uSlot))
            {
                return uSlot;
            }
        }

        return MAX_STORAGE_BUFFER_UNITS;
    }

    bool BindShaderTracingUnit(SPipeline* pPipeline, SShader* pShader, CDevice* pDevice)
    {
        uint32 uUnusedUnit = FindUnusedShaderStorageBufferUnit(pPipeline, pDevice);
        if (uUnusedUnit == MAX_STORAGE_BUFFER_UNITS)
        {
            DXGL_ERROR("Could not find an extra storage buffer unit for shader tracing");
            return false;
        }

        GLuint uProgramName(GetProgramName(pPipeline, pShader));

        GLuint uStorageBlockIndex(glGetProgramResourceIndex(uProgramName, GL_SHADER_STORAGE_BLOCK, "Trace"));
        if (uStorageBlockIndex == GL_INVALID_INDEX)
        {
            DXGL_ERROR("Could not find \"Trace\" shader storage block in program");
            return false;
        }

        pPipeline->m_uTraceBufferUnit = uUnusedUnit;
        glShaderStorageBlockBinding(uProgramName, uStorageBlockIndex, uUnusedUnit);

        return true;
    }

#endif //DXGL_ENABLE_SHADER_TRACING

    struct SResourceIndexRange
    {
        uint32 m_uMin;
        uint32 m_uMax;

        SResourceIndexRange()
            : m_uMin(0xFFFFFFFF)
            , m_uMax(0)
        {
        }

        bool Fits(const SIndexPartitionRange& kPartitionRange)
        {
            return m_uMin >= kPartitionRange.m_uFirstIn && m_uMax <= kPartitionRange.m_uFirstIn + kPartitionRange.m_uCount;
        }
    };

    enum
    {
        ENCODED_SAMPLER_STRIDE = 3, ENCODED_RESOURCE_STRIDE = 2
    };

    template <uint32 (* DecodeFunc)(uint32), uint32 uStride>
    void GetResourceIndexRange(SResourceIndexRange* pRange, const uint32* puResources, uint32 uNumResources)
    {
        pRange->m_uMin = 0xFFFFFFFF;
        pRange->m_uMax = 0;
        if (uNumResources > 0)
        {
            const uint32* puResourcesEnd = puResources + (uNumResources * uStride);
            for (const uint32* puResource = puResources; puResource < puResourcesEnd; puResource += uStride)
            {
                uint32 uIndex = DecodeFunc(*puResource);
                pRange->m_uMin = min(pRange->m_uMin, uIndex);
                pRange->m_uMax = max(pRange->m_uMax, uIndex);
            }
        }
    }

    struct SPipelineResourceRequirements
    {
        struct SResourceType
        {
            enum
            {
                INVALID_PARTITION_ID = 0xFFFFFFFF
            };

            uint32 m_uNumUnits;
            uint32 m_uCompatiblePartitionID; // INVALID_PARTITION_ID if no partition is compatible

            SResourceType()
                : m_uNumUnits(0)
                , m_uCompatiblePartitionID(0xFFFFFFFF)
            {
            }
        };

        SResourceType m_akTypes[eRUT_NUM];
    };

    void GetPipelineResourceRequirements(SPipelineResourceRequirements* pRequirements, const SPipeline* pPipeline, CContext* pContext)
    {
        DXGL_TODO("Instead of calculating the resource input ranges here, do it at translation-time and store them in the reflection");
        SResourceIndexRange akInputRanges[eST_NUM][eRUT_NUM];
        CDevice* pDevice = pContext->GetDevice();

        for (uint32 uShader = 0; uShader < eST_NUM; ++uShader)
        {
            SShader* pShader(pPipeline->m_kConfiguration.m_apShaders[uShader]);
            if (IsPipelineStageUsed(pPipeline->m_kConfiguration.m_eMode, (EShaderType)uShader) && pShader != NULL)
            {
                const SShader::SVersion& kVersion(GetPipelineShaderVersion(pPipeline->m_kConfiguration, (EShaderType)uShader));
                const SShaderReflection& kReflection(kVersion.m_kReflection);

                pRequirements->m_akTypes[eRUT_Texture].m_uNumUnits += kReflection.m_uNumSamplers;
                GetResourceIndexRange<DXBC::DecodeTextureUnitIndex, ENCODED_SAMPLER_STRIDE>(
                    &akInputRanges[uShader][eRUT_Texture],
                    reinterpret_cast<const uint32*>(kVersion.m_kSource.m_pData + kReflection.m_uSamplersOffset),
                    kReflection.m_uNumSamplers);

                pRequirements->m_akTypes[eRUT_UniformBuffer].m_uNumUnits += kReflection.m_uNumUniformBuffers;
                GetResourceIndexRange<DXBC::DecodeResourceIndex, ENCODED_RESOURCE_STRIDE>(
                    &akInputRanges[uShader][eRUT_UniformBuffer],
                    reinterpret_cast<const uint32*>(kVersion.m_kSource.m_pData + kReflection.m_uUniformBuffersOffset),
                    kReflection.m_uNumUniformBuffers);

#if DXGL_SUPPORT_SHADER_STORAGE_BLOCKS
                pRequirements->m_akTypes[eRUT_StorageBuffer].m_uNumUnits += kReflection.m_uNumStorageBuffers;
                GetResourceIndexRange<DXBC::DecodeResourceIndex, ENCODED_RESOURCE_STRIDE>(
                    &akInputRanges[uShader][eRUT_StorageBuffer],
                    reinterpret_cast<const uint32*>(kVersion.m_kSource.m_pData + kReflection.m_uStorageBuffersOffset),
                    kReflection.m_uNumStorageBuffers);
#endif

#if DXGL_SUPPORT_SHADER_IMAGES
                if (pDevice->IsFeatureSupported(eF_ShaderImages))
                {
                    pRequirements->m_akTypes[eRUT_Image].m_uNumUnits += kReflection.m_uNumImages;
                    GetResourceIndexRange<DXBC::DecodeResourceIndex, ENCODED_RESOURCE_STRIDE>(
                        &akInputRanges[uShader][eRUT_Image],
                        reinterpret_cast<const uint32*>(kVersion.m_kSource.m_pData + kReflection.m_uImagesOffset),
                        kReflection.m_uNumImages);
                }
#endif
            }
        }

        for (uint32 uResourceType = 0; uResourceType < eRUT_NUM; ++uResourceType)
        {
            uint32 uNumPartitions = pDevice->GetNumResourceUnitPartitions((EResourceUnitType)uResourceType);
            for (uint32 uPartition = 0; uPartition < uNumPartitions; ++uPartition)
            {
                const SIndexPartition& kPartition(pDevice->GetResourceUnitPartition((EResourceUnitType)uResourceType, uPartition));
                uint32 uStage;
                for (uStage = 0; uStage < eST_NUM; ++uStage)
                {
                    if (!akInputRanges[uStage][uResourceType].Fits(kPartition.m_akStages[uStage]))
                    {
                        break;
                    }
                }
                if (uStage == eST_NUM)
                {
                    pRequirements->m_akTypes[uResourceType].m_uCompatiblePartitionID = uPartition;
                    break;
                }
            }
        }
    }

    bool InitializeTextureBindings(
        SUnitMap* pMap,
        uint32 uMapPosition,
        const uint32* pSamplers,
        uint32 uNumSamplers,
        EShaderType eStage,
        const char* szGLSL,
        GLint iProgramName,
        const SIndexPartition* pPartition,
        CContext* pContext)
    {
        char acNameBuffer[MAX_REFLECT_STRING_LENGTH];
        uint32 uNextTextureUnit = uMapPosition;
        SUnitMap::SElement* pMapElementOut = pMap->m_akUnits + uMapPosition;
        for (uint32 uElement = 0; uElement < uNumSamplers; ++uElement)
        {
            uint32 uSamplerField         = pSamplers[uElement * 3 + 0];
            uint32 uEmbeddedNormalName   = pSamplers[uElement * 3 + 1];
            uint32 uEmbeddedCompareName  = pSamplers[uElement * 3 + 2];

            uint32 uTextureIndex     = DXBC::DecodeTextureIndex(uSamplerField);
            uint32 uSamplerIndex     = DXBC::DecodeSamplerIndex(uSamplerField);
            uint32 uTextureUnitIndex = DXBC::DecodeTextureUnitIndex(uSamplerField);
            bool bNormalSample       = DXBC::DecodeNormalSample(uSamplerField);
            bool bCompareSample      = DXBC::DecodeCompareSample(uSamplerField);

            uint32 uTextureSlot = TextureSlot(eStage, uTextureIndex);
            uint32 uSamplerSlot = SamplerSlot(eStage, uSamplerIndex);
            uint32 uTextureUnit = pPartition ? pPartition->m_akStages[eStage](uTextureUnitIndex) : uMapPosition + uElement;

            bool bSample(bNormalSample || bCompareSample);
            if (bSample)
            {
                *pMapElementOut = SUnitMap::SElement::TextureWithSampler(uTextureSlot, uSamplerSlot, uTextureUnit);
            }
            else
            {
                *pMapElementOut = SUnitMap::SElement::TextureWithoutSampler(uTextureSlot, uTextureUnit);
            }

            if (bNormalSample || !bSample) // Both normal sampling and texture loads use the "normal sampler name" field
            {
                if (!DXBC::GetEmbeddedName(acNameBuffer, szGLSL, uEmbeddedNormalName) ||
                    !BindUniformToUnit(iProgramName, uTextureUnit, acNameBuffer, pContext))
                {
                    return false;
                }
            }

            if (bCompareSample)
            {
                if (!DXBC::GetEmbeddedName(acNameBuffer, szGLSL, uEmbeddedCompareName) ||
                    !BindUniformToUnit(iProgramName, uTextureUnit, acNameBuffer, pContext))
                {
                    return false;
                }
            }

            ++pMapElementOut;
        }
        return true;
    }

    bool InitializeResourceBindings(
        AZStd::function<bool(GLuint, uint32, const char*, CContext*)> pfBindResourceToUnit,
        AZStd::function<uint32(EShaderType, uint32)> pfIndexToSlot,
        SUnitMap* pMap,
        uint32 uMapPosition,
        const uint32* pResources,
        uint32 uNumResources,
        EShaderType eStage,
        const char* szGLSL,
        GLint iProgramName,
        const SIndexPartition* pPartition,
        CContext* pContext)
    {
        char acNameBuffer[MAX_REFLECT_STRING_LENGTH];
        uint32 uNextResourceUnit = uMapPosition;
        SUnitMap::SElement* pMapElementOut = pMap->m_akUnits + uMapPosition;
        for (uint32 uElement = 0; uElement < uNumResources; ++uElement)
        {
            uint32 uResourceField = pResources[uElement * 2 + 0];
            uint32 uResourceName  = pResources[uElement * 2 + 1];

            uint32 uResourceIndex = DXBC::DecodeResourceIndex(uResourceField);

            uint32 uResourceSlot = pfIndexToSlot(eStage, uResourceIndex);
            uint32 uResourceUnit;
            if (pfBindResourceToUnit)
            {
                uResourceUnit = pPartition ? pPartition->m_akStages[eStage](uResourceIndex) : uMapPosition + uElement;
                if (!DXBC::GetEmbeddedName(acNameBuffer, szGLSL, uResourceName) ||
                !pfBindResourceToUnit(iProgramName, uResourceUnit, acNameBuffer, pContext))
                {
                    return false;
                }
            }
            else
            {
                // Some resources, like Storage Buffers in OpenGLES, don't allow changing binding point.
                // Because of this we have to use the binding that was set in the shader (we get the location from the reflection info).
                uResourceUnit = uResourceIndex;
            }

            *pMapElementOut = SUnitMap::SElement::Resource(uResourceSlot, uResourceUnit);
            ++pMapElementOut;
        }
        return true;
    }

    bool InitializePipelineResources(SPipeline* pPipeline, CContext* pContext)
    {
        SPipelineResourceRequirements kRequirements;
        GetPipelineResourceRequirements(&kRequirements, pPipeline, pContext);

        CDevice* pDevice = pContext->GetDevice();

        const SIndexPartition* apUnitPartitions[eRUT_NUM];
        SUnitMapPtr aspUnitMaps[eRUT_NUM];
        uint32 auUnitMapPositions[eRUT_NUM];
        for (uint32 uUnitType = 0; uUnitType < eRUT_NUM; ++uUnitType)
        {
            SPipelineResourceRequirements::SResourceType kType = kRequirements.m_akTypes[uUnitType];
            if (kType.m_uCompatiblePartitionID == SPipelineResourceRequirements::SResourceType::INVALID_PARTITION_ID)
            {
                apUnitPartitions[uUnitType] = NULL;
            }
            else
            {
                apUnitPartitions[uUnitType] = &pDevice->GetResourceUnitPartition((EResourceUnitType)uUnitType, kType.m_uCompatiblePartitionID);
            }
            if (kType.m_uNumUnits > 0)
            {
                aspUnitMaps[uUnitType] = new SUnitMap(kType.m_uNumUnits);
            }
            auUnitMapPositions[uUnitType] = 0;
        }

        bool bSuccess = true;
        for (uint32 uShader = 0; uShader < eST_NUM; ++uShader)
        {
            SShader* pShader(pPipeline->m_kConfiguration.m_apShaders[uShader]);
            if (!IsPipelineStageUsed(pPipeline->m_kConfiguration.m_eMode, (EShaderType)uShader) || pShader == NULL)
            {
                continue;
            }

            GLuint uProgramName = GetProgramName(pPipeline, pShader);
            const SShader::SVersion& kVersion(GetPipelineShaderVersion(pPipeline->m_kConfiguration, (EShaderType)uShader));
            const char* szGLSL = kVersion.m_kSource.m_pData + kVersion.m_kReflection.m_uGLSLSourceOffset;

            // Initialize texture unit map and bind texture units to shader uniforms
            uint32 uNumSamplers(kVersion.m_kReflection.m_uNumSamplers);
            if (uNumSamplers > 0)
            {
                const uint32* pSamplers(reinterpret_cast<const uint32*>(kVersion.m_kSource.m_pData + kVersion.m_kReflection.m_uSamplersOffset));
                SUnitMap* pTextureUnitMap = aspUnitMaps[eRUT_Texture].get();

                bSuccess &= InitializeTextureBindings(
                        pTextureUnitMap, auUnitMapPositions[eRUT_Texture],
                        pSamplers, uNumSamplers, (EShaderType)uShader,
                        szGLSL, uProgramName, apUnitPartitions[eRUT_Texture],
                        pContext);

                auUnitMapPositions[eRUT_Texture] += uNumSamplers;
            }

            // Initialize uniform buffer unit map and bind uniform buffer units to shader uniform blocks
            uint32 uNumUniformBuffers(kVersion.m_kReflection.m_uNumUniformBuffers);
            if (uNumUniformBuffers > 0)
            {
                const uint32* pUniformBuffers(reinterpret_cast<const uint32*>(kVersion.m_kSource.m_pData + kVersion.m_kReflection.m_uUniformBuffersOffset));
                SUnitMap* pUniformBufferMap = aspUnitMaps[eRUT_UniformBuffer].get();

                bSuccess &= InitializeResourceBindings(&BindUniformBufferToUnit, &ConstantBufferSlot,
                        pUniformBufferMap, auUnitMapPositions[eRUT_UniformBuffer],
                        pUniformBuffers, uNumUniformBuffers, (EShaderType)uShader,
                        szGLSL, uProgramName, apUnitPartitions[eRUT_UniformBuffer],
                        pContext);

                auUnitMapPositions[eRUT_UniformBuffer] += uNumUniformBuffers;
            }

#if DXGL_SUPPORT_SHADER_STORAGE_BLOCKS
            // Initialize shader storage unit map and bind shader storage units to shader storage blocks
            uint32 uNumStorageBuffers(kVersion.m_kReflection.m_uNumStorageBuffers);
            if (uNumStorageBuffers > 0)
            {
                const uint32* pStorageBuffers(reinterpret_cast<const uint32*>(kVersion.m_kSource.m_pData + kVersion.m_kReflection.m_uStorageBuffersOffset));
                SUnitMap* pStorageBufferMap = aspUnitMaps[eRUT_StorageBuffer].get();

#if DXGLES
                // OpenGL ES doesn't support changing the binding point of storage buffers
                auto bindStorageToUintFunc = nullptr;
#else
                auto bindStorageToUintFunc = &BindStorageBufferToUnit;
#endif // DXGLES
                    bSuccess &= InitializeResourceBindings(bindStorageToUintFunc, &StorageBufferSlot,
                        pStorageBufferMap, auUnitMapPositions[eRUT_StorageBuffer],
                        pStorageBuffers, uNumStorageBuffers, (EShaderType)uShader,
                        szGLSL, uProgramName, apUnitPartitions[eRUT_StorageBuffer],
                        pContext);

                auUnitMapPositions[eRUT_StorageBuffer] += uNumStorageBuffers;
            }
#endif //DXGL_SUPPORT_SHADER_STORAGE_BLOCKS

#if DXGL_SUPPORT_SHADER_IMAGES
            if (pDevice->IsFeatureSupported(eF_ShaderImages))
            {
                // Initialize image unit map and bind image units to shader uniforms
                uint32 uNumImages(kVersion.m_kReflection.m_uNumImages);
                if (uNumImages > 0)
                {
                    const uint32* pImages(reinterpret_cast<const uint32*>(kVersion.m_kSource.m_pData + kVersion.m_kReflection.m_uImagesOffset));
                    SUnitMap* pImageMap = aspUnitMaps[eRUT_Image].get();

                    bSuccess &= InitializeResourceBindings(&BindUniformToUnit, &ImageSlot,
                        pImageMap, auUnitMapPositions[eRUT_Image],
                        pImages, uNumImages, (EShaderType)uShader,
                        szGLSL, uProgramName, apUnitPartitions[eRUT_Image],
                        pContext);

                    auUnitMapPositions[eRUT_Image] += uNumImages;
                }
            }
#endif //DXGL_SUPPORT_SHADER_IMAGES

#if DXGL_ENABLE_SHADER_TRACING
            if (pPipeline->m_kConfiguration.m_aeShaderVersions[uShader] == eSV_Tracing)
            {
                bSuccess &= BindShaderTracingUnit(pPipeline, pShader, pDevice);
            }
#endif //DXGL_ENABLE_SHADER_TRACING
        }

        if (!bSuccess)
        {
            return false;
        }

        for (uint32 uUnitType = 0; uUnitType < eRUT_NUM; ++uUnitType)
        {
            if (aspUnitMaps[uUnitType] != NULL)
            {
                pPipeline->m_aspResourceUnitMaps[uUnitType] = pContext->AllocateUnitMap(aspUnitMaps[uUnitType]);
                bSuccess &= pPipeline->m_aspResourceUnitMaps[uUnitType] != NULL;
            }
        }

        return bSuccess;
    }

    SPipelineConfiguration::SPipelineConfiguration()
        : m_eMode(ePM_Graphics)
#if !DXGL_SUPPORT_DEPTH_CLAMP
        , m_bEmulateDepthClamp(false)
#endif //!DXGL_SUPPORT_DEPTH_CLAMP
    {
        memset(m_apShaders, 0, sizeof(m_apShaders));
#if DXGL_ENABLE_SHADER_TRACING
        memset(m_aeShaderVersions, 0, sizeof(m_aeShaderVersions));
#endif //DXGL_ENABLE_SHADER_TRACING
    }

    SPipelineCompilationBuffer::SPipelineCompilationBuffer()
        : m_uImportsBeginOffset(0)
        , m_uImportsEndOffset(0)
        , m_uImportsHeaderCapacity(0)
        , m_iVersionStringLength(-1)
        , m_acImportsHeader(NULL)
    {
        memset(m_acVersionString, 0, sizeof(m_acVersionString));
    }

    SPipelineCompilationBuffer::~SPipelineCompilationBuffer()
    {
        if (m_acImportsHeader)
        {
            Free(m_acImportsHeader);
        }
    }

    SUnitMap::SUnitMap(uint16 uNumUnits)
        : m_uNumUnits(uNumUnits)
    {
        if (uNumUnits == 0)
        {
            m_akUnits = NULL;
        }
        else
        {
            m_akUnits = new SElement[uNumUnits];
        }
    }

    SUnitMap::SUnitMap(const SUnitMap& kOther)
        : m_uNumUnits(kOther.m_uNumUnits)
    {
        if (m_uNumUnits == 0)
        {
            m_akUnits = NULL;
        }
        else
        {
            m_akUnits = new SElement[m_uNumUnits];
            memcpy(m_akUnits, kOther.m_akUnits, sizeof(SElement) * m_uNumUnits);
        }
    }

    SUnitMap::~SUnitMap()
    {
        delete [] m_akUnits;
    }

    bool IsPipelineStageUsed(EPipelineMode eMode, EShaderType eType)
    {
        switch (eType)
        {
        case eST_Vertex:
        case eST_Fragment:
#if DXGL_SUPPORT_GEOMETRY_SHADERS
        case eST_Geometry:
#endif //DXGL_SUPPORT_GEOMETRY_SHADERS
#if DXGL_SUPPORT_TESSELLATION
        case eST_TessControl:
        case eST_TessEvaluation:
#endif //DXGL_SUPPORT_TESSELLATION
            return eMode == ePM_Graphics;
#if DXGL_SUPPORT_COMPUTE
        case eST_Compute:
            return eMode == ePM_Compute;
#endif //DXGL_SUPPORT_COMPUTE
        default:
            DXGL_ERROR("Invalid shader type");
            return false;
        }
    }

    bool GetGLShaderType(GLenum& eGLShaderType, EShaderType eType)
    {
        switch (eType)
        {
        case eST_Vertex:
            eGLShaderType = GL_VERTEX_SHADER;
            break;
        case eST_Fragment:
            eGLShaderType = GL_FRAGMENT_SHADER;
            break;
#if DXGL_SUPPORT_GEOMETRY_SHADERS
        case eST_Geometry:
            eGLShaderType = GL_GEOMETRY_SHADER;
            break;
#endif //DXGL_SUPPORT_GEOMETRY_SHADERS
#if DXGL_SUPPORT_TESSELLATION
        case eST_TessControl:
            eGLShaderType = GL_TESS_CONTROL_SHADER;
            break;
        case eST_TessEvaluation:
            eGLShaderType = GL_TESS_EVALUATION_SHADER;
            break;
#endif //DXGL_SUPPORT_TESSELLATION
#if DXGL_SUPPORT_COMPUTE
        case eST_Compute:
            eGLShaderType = GL_COMPUTE_SHADER;
            break;
#endif //DXGL_SUPPORT_COMPUTE
        default:
            DXGL_ERROR("Invalid shader type");
            return false;
        }
        return true;
    }

    bool GetGLProgramStageBit(GLbitfield& uProgramStageBit, EShaderType eType)
    {
        switch (eType)
        {
        case eST_Vertex:
            uProgramStageBit = GL_VERTEX_SHADER_BIT;
            break;
        case eST_Fragment:
            uProgramStageBit = GL_FRAGMENT_SHADER_BIT;
            break;
#if DXGL_SUPPORT_GEOMETRY_SHADERS
        case eST_Geometry:
            uProgramStageBit = GL_GEOMETRY_SHADER_BIT;
            break;
#endif //DXGL_SUPPORT_GEOMETRY_SHADERS
#if DXGL_SUPPORT_TESSELLATION
        case eST_TessControl:
            uProgramStageBit = GL_TESS_CONTROL_SHADER_BIT;
            break;
        case eST_TessEvaluation:
            uProgramStageBit = GL_TESS_EVALUATION_SHADER_BIT;
            break;
#endif //DXGL_SUPPORT_TESSELLATION
#if DXGL_SUPPORT_COMPUTE
        case eST_Compute:
            uProgramStageBit = GL_COMPUTE_SHADER_BIT;
            break;
#endif //DXGL_SUPPORT_COMPUTE
        default:
            DXGL_ERROR("Invalid shader type");
            return false;
        }
        return true;
    }

    bool GetDataTypeSize(GLenum eDataType, GLuint& uSize)
    {
        DXGL_TODO("Verify that the sizes are correct according to the packing in layout structures");
        switch (eDataType)
        {
        case GL_BYTE:
        case GL_UNSIGNED_BYTE:
            uSize = 1;
            return true;
        case GL_SHORT:
        case GL_UNSIGNED_SHORT:
        case GL_HALF_FLOAT:
            uSize = 2;
            return true;
        case GL_INT:
        case GL_UNSIGNED_INT:
        case GL_INT_2_10_10_10_REV:
        case GL_UNSIGNED_INT_2_10_10_10_REV:
        case GL_FLOAT:
        case GL_FIXED:
            uSize = 4;
            return true;
#if !DXGLES
        // There is no GL_DOUBLE on OpenGL ES 3
        case GL_DOUBLE:
            uSize = 8;
            return true;
#endif //!DXGLES
        }
        return false;
    }

    bool GetFormatAttributeDimension(const SGIFormatInfo* pFormatInfo, GLint& iDimensions)
    {
        switch (pFormatInfo->m_pTexture->m_eBaseFormat)
        {
        case GL_RGBA:
            iDimensions = 4;
            break;
        case GL_RGB:
            iDimensions = 3;
            break;
        case GL_RG:
            iDimensions = 2;
            break;
        case GL_RED:
            iDimensions = 1;
            break;
#if (DXGLES && (GL_APPLE_texture_format_BGRA8888 || GL_IMG_read_format))
        case GL_BGRA_IMG:
            iDimensions = GL_BGRA_IMG;  // Special value for packed reverse formats (A2B10G10R10*)
            break;
#elif !DXGLES
        case GL_BGRA:
            iDimensions = GL_BGRA;  // Special value for packed reverse formats (A2B10G10R10*)
            break;
#endif
        default:
            return false;
        }
        return true;
    }

    bool GetFormatNormalized(const SUncompressedLayout* pLayout, GLboolean& bNormalized)
    {
        if (pLayout->m_eDepthType != eGICT_UNUSED || pLayout->m_eStencilType != eGICT_UNUSED) // As of D3D11.1 no format with depth or stencil channels supports buffers
        {
            return false;
        }

        switch (pLayout->m_eColorType)
        {
        case eGICT_UNORM:
        case eGICT_SNORM:
            bNormalized = true;
            break;
        case eGICT_UINT:
        case eGICT_SINT:
        case eGICT_FLOAT:
            bNormalized = false;
            break;
        default:
            return false;
        }
        return true;
    }

    bool PushInputLayoutAttribute(
        SInputLayout& kLayout,
        uint32 auSlotOffsets[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT],
        const D3D11_INPUT_ELEMENT_DESC& kDesc,
        uint32 uIndex,
        bool bInteger,
        const CDevice* pDevice)
    {
        SInputLayout::SAttributeParameters kParams;
        SInputLayout::TAttributes& kSlotAttributes(kLayout.m_akSlotAttributes[kDesc.InputSlot]);

        // The format must have texture and uncompressed layout information in order to determine
        // the input layout
        NCryOpenGL::EGIFormat eGIFormat(GetGIFormat(kDesc.Format));
        const NCryOpenGL::SGIFormatInfo* pFormatInfo(NULL);
        if (eGIFormat == eGIF_NUM || (pFormatInfo = (GetGIFormatInfo(eGIFormat))) == NULL ||
            pFormatInfo->m_pTexture == NULL || pFormatInfo->m_pUncompressed == NULL)
        {
            DXGL_ERROR("Invalid DXGI format for vertex attribute");
            return false;
        }

        kParams.m_uAttributeIndex = uIndex;

        if (!GetFormatAttributeDimension(pFormatInfo, kParams.m_iDimension))
        {
            DXGL_ERROR("Invalid dimension for vertex attribute");
            return false;
        }

        kParams.m_eType = pFormatInfo->m_pTexture->m_eDataType;

        if (!GetFormatNormalized(pFormatInfo->m_pUncompressed, kParams.m_bNormalized))
        {
            DXGL_ERROR("Invalid layout for vertex attribute");
            return false;
        }

        uint32& uSlotOffset(auSlotOffsets[kDesc.InputSlot]);
        kParams.m_uPointerOffset = (kDesc.AlignedByteOffset == D3D11_APPEND_ALIGNED_ELEMENT) ? uSlotOffset : kDesc.AlignedByteOffset;

        uint32 uSize;
        if (!GetDataTypeSize(pFormatInfo->m_pTexture->m_eDataType, uSize))
        {
            DXGL_ERROR("Invalid data type for vertex attribute");
            return false;
        }

        kParams.m_bInteger = bInteger;
        kParams.m_uVertexAttribDivisor = kDesc.InstanceDataStepRate;

        kSlotAttributes.push_back(kParams);

#if DXGL_SUPPORT_VERTEX_ATTRIB_BINDING
        if (pDevice->IsFeatureSupported(eF_VertexAttribBinding))
        {
            SInputLayout::SAttributeFormats kParamsVAB;
            SInputLayout::TAttributeFormats& kSlotAttributesVAB(kLayout.m_akVertexAttribFormats);

            kParamsVAB.m_bInteger = kParams.m_bInteger;
            kParamsVAB.m_iDimension = kParams.m_iDimension;
            kParamsVAB.m_uAttributeIndex = kParams.m_uAttributeIndex;
            kParamsVAB.m_bNormalized = kParams.m_bNormalized;
            kParamsVAB.m_uPointerOffset = kParams.m_uPointerOffset;
            kParamsVAB.m_eType = kParams.m_eType;
            kParamsVAB.m_uVertexAttribDivisor = kParams.m_uVertexAttribDivisor;

            kParamsVAB.m_uBindingIndex = kDesc.InputSlot;
            kSlotAttributesVAB.push_back(kParamsVAB);
        }
#endif //DXGL_SUPPORT_VERTEX_ATTRIB_BINDING

        uSlotOffset += uSize;

        return true;
    }

    SInputLayoutPtr CreateInputLayout(const D3D11_INPUT_ELEMENT_DESC* pInputElementDescs, uint32 uNumElements, const TShaderReflection& kReflection, const CDevice* pDevice)
    {
        SInputLayoutPtr spLayout(new SInputLayout());
        uint32 auSlotOffsets[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
        memset(auSlotOffsets, 0, sizeof(auSlotOffsets));

        int attrMatchedNum = 0;
        int reflectionAttrNum = 0;

        //Calculate the number of attributes in the shader that does not start with 'SV_'
        TShaderReflection::TParameters::const_iterator kInputSemanticIter(kReflection.m_kInputs.begin());
        const TShaderReflection::TParameters::const_iterator kInputSemanticEnd(kReflection.m_kInputs.end());
        while (kInputSemanticIter != kInputSemanticEnd)
        {
            if (strstr(kInputSemanticIter->m_kDesc.SemanticName, "SV_") == 0)
            {
                reflectionAttrNum++;
            }
            ++kInputSemanticIter;
        }

        for (uint32 uElement = 0; uElement < uNumElements; ++uElement)
        {
            const D3D11_INPUT_ELEMENT_DESC& kDesc(pInputElementDescs[uElement]);
            // Find a match within the input signatures of the shader reflection data
            TShaderReflection::TParameters::const_iterator kInputSemanticIter(kReflection.m_kInputs.begin());
            const TShaderReflection::TParameters::const_iterator kInputSemanticEnd(kReflection.m_kInputs.end());
            while (kInputSemanticIter != kInputSemanticEnd)
            {
                if (kInputSemanticIter->m_kDesc.SemanticIndex == kDesc.SemanticIndex &&
                    strcmp(kInputSemanticIter->m_kDesc.SemanticName, kDesc.SemanticName) == 0)
                {
                    bool bInteger;
                    switch (kInputSemanticIter->m_kDesc.ComponentType)
                    {
                    case D3D_REGISTER_COMPONENT_UNKNOWN:
                        assert(false);
                    case D3D_REGISTER_COMPONENT_FLOAT32:
                        bInteger = false;
                        break;
                    case D3D_REGISTER_COMPONENT_UINT32:
                    case D3D_REGISTER_COMPONENT_SINT32:
                        bInteger = true;
                        break;
                    default:
                        DXGL_ERROR("Invalid input signature component register type");
                        return NULL;
                    }

                    if (!PushInputLayoutAttribute(*spLayout, auSlotOffsets, kDesc, kInputSemanticIter->m_kDesc.Register, bInteger, pDevice))
                    {
                        DXGL_ERROR("Failed to Push Input Layout");
                        return NULL;
                    }
                    attrMatchedNum++;
                    break;
                }
                ++kInputSemanticIter;
            }
        }

        if (attrMatchedNum != reflectionAttrNum)
        {
            DXGL_ERROR("Shader input attributes count doesn not match with vertex format attributes count");
            return NULL;
        }

        return spLayout;
    }

#if defined(CODEGEN_FUNCPTR)
#define GL_ENTRY_POINT CODEGEN_FUNCPTR
#elif defined(GLAPIENTRY)
#define GL_ENTRY_POINT GLAPIENTRY
#elif defined(GL_APIENTRY)
#define GL_ENTRY_POINT GL_APIENTRY
#elif defined(APIENTRY)
#define GL_ENTRY_POINT APIENTRY
#else
#define GL_ENTRY_POINT
#endif

    bool VerifyStatus(GLuint uName, GLenum eStatus, void (GL_ENTRY_POINT * fGetIV)(GLuint, GLenum, GLint*), void (GL_ENTRY_POINT * fGetInfoLog)(GLuint, GLsizei, GLsizei*, GLchar*))
    {
        GLint iStatus;
        fGetIV(uName, eStatus, &iStatus);
        if (iStatus == GL_FALSE)
        {
            GLint iInfoLogLength;
            fGetIV(uName, GL_INFO_LOG_LENGTH, &iInfoLogLength);
            if (iInfoLogLength > 0)
            {
                GLchar* pBuffer = new GLchar[iInfoLogLength];
                GLsizei iLength;
                fGetInfoLog(uName, iInfoLogLength, &iLength, pBuffer);
                if (iStatus == GL_FALSE)
                {
                    DXGL_ERROR("%s", pBuffer);
                }
                delete [] pBuffer;
            }
            return false;
        }

        return true;
    }

    bool VerifyShaderStatus(GLuint uShader, GLenum eStatus)
    {
        return VerifyStatus(uShader, eStatus, glGetShaderiv, glGetShaderInfoLog);
    }

    bool VerifyProgramStatus(GLuint uProgram, GLenum eStatus)
    {
        return VerifyStatus(uProgram, eStatus, glGetProgramiv, glGetProgramInfoLog);
    }

    bool VerifyProgramPipelineStatus(GLuint uPipelin, GLenum eStatus)
    {
        return VerifyStatus(uPipelin, eStatus, glGetProgramPipelineiv, glGetProgramPipelineInfoLog);
    }
}
