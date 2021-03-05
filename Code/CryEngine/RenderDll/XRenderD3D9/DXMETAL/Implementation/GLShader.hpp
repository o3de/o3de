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

// Description : Declares the shader types and related functions

#ifndef __GLSHADER__
#define __GLSHADER__

#include "GLCommon.hpp"

//  Confetti BEGIN: Igor Lobanchikov
@protocol MTLFunction;
@protocol MTLRenderPipelineState;
@class MTLArgument;
@class MTLStructType;

#include "GLState.hpp"
#import <Metal/MTLPixelFormat.h>
//  Confetti End: Igor Lobanchikov

namespace NCryMetal
{
    enum
    {
        MAX_CONSTANT_BUFFER_BIND_POINTS = D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT
    };

    enum EShaderType
    {
        eST_Vertex,
        eST_Fragment,
#if DXGL_SUPPORT_COMPUTE
        eST_Compute,
#endif //DXGL_SUPPORT_COMPUTE
        eST_NUM
    };

    struct SPipeline;
    class CContext;

    struct SSource
    {
        const char* m_pData;
        uint32 m_uDataSize;

        SSource(const char* pData = NULL, uint32 uDataSize = 0);
        ~SSource();
    };

    struct SShaderSource
        : SSource
    {
        SShaderSource();
        ~SShaderSource();

        void SetData(const char* pData = NULL, uint32 uDataSize = 0);
    };

    enum
    {
        DXGL_MAX_REFLECT_STRING_LENGTH = 128
    };

    struct SShaderReflectionVariable
    {
        D3D11_SHADER_VARIABLE_DESC m_kDesc;
        D3D11_SHADER_TYPE_DESC m_kType;
        char m_acName[DXGL_MAX_REFLECT_STRING_LENGTH];
        std::vector<uint8> m_kDefaultValue;
    };

    struct SShaderReflectionConstBuffer
    {
        typedef std::vector<SShaderReflectionVariable> TVariables;

        TVariables m_kVariables;
        D3D11_SHADER_BUFFER_DESC m_kDesc;
        char m_acName[DXGL_MAX_REFLECT_STRING_LENGTH];
    };

    struct SShaderReflectionResource
    {
        D3D11_SHADER_INPUT_BIND_DESC m_kDesc;
        char m_acName[DXGL_MAX_REFLECT_STRING_LENGTH];
    };

    struct SShaderReflectionParameter
    {
        D3D11_SIGNATURE_PARAMETER_DESC m_kDesc;
        char m_acSemanticName[DXGL_MAX_REFLECT_STRING_LENGTH];
    };

    struct SShaderReflectionArgument
    {
        char m_acName[DXGL_MAX_REFLECT_STRING_LENGTH];
        bool m_bActive;
        uint32 m_uIndex;
        uint32 m_eType;
        uint32 m_eAccess;
        uint32 m_uBufferAlignment;
        uint32 m_uBufferDataSize;
        uint32 m_eDataType;
        uint32 m_eTextureType;
    };

    struct SShaderReflection
    {
        typedef std::vector<SShaderReflectionConstBuffer> TConstantBuffers;
        typedef std::vector<SShaderReflectionResource> TResources;
        typedef std::vector<SShaderReflectionParameter> TParameters;

        uint32 m_uGLSLSourceOffset;
        uint32 m_uSamplerMapOffset;
        uint32 m_uSamplerMapSize;
        uint32 m_uImportsOffset;
        uint32 m_uImportsSize;
        uint32 m_uExportsOffset;
        uint32 m_uExportsSize;
        uint32 m_uUAVBindingAreaOffset;
        uint32 m_uUAVBindingAreaSize;
        uint32 m_uInputHash;
        uint32 m_uThread_x;
        uint32 m_uThread_y;
        uint32 m_uThread_z;

        TConstantBuffers m_kConstantBuffers;
        TResources m_kResources;
        TParameters m_kInputs, m_kOutputs, m_kPatchConstants;
        D3D11_SHADER_DESC m_kDesc;
    };

    struct SGLShader
    {
        SGLShader();
        ~SGLShader();

        //  Confetti BEGIN: Igor Lobanchikov
        id<MTLFunction> m_pFunction;
        //  Confetti End: Igor Lobanchikov
    };

    DXGL_DECLARE_REF_COUNTED(struct, SShader)
    {
        EShaderType m_eType;

        SShaderSource m_kSource;

        SGLShader m_kGLShader;
        SShaderReflection m_kReflection;

        typedef std::vector<SPipeline*> TPipelines;
        TPipelines m_kBoundPipelines;

        void AttachPipeline(SPipeline * pPipeline);
        void DetachPipeline(SPipeline * pPipeline);

        SShader();
        ~SShader();
    };

    //  Confetti BEGIN: Igor Lobanchikov
    struct SColorAttachmentDesc
        : public SMetalBlendState
    {
        MTLPixelFormat pixelFormat;
    };

    struct SAttachmentConfiguration
    {
        //  Igor: this is required so that all padding bits are set to 0 so that we can do memcmp to compare the structures.
        SAttachmentConfiguration() {memset(this, 0, sizeof(*this)); }

        static const int s_ColorAttachmentDescCount = 8;

        SColorAttachmentDesc colorAttachments[8];
        MTLPixelFormat depthAttachmentPixelFormat;
        MTLPixelFormat stencilAttachmentPixelFormat;
    };
    //  Confetti End: Igor Lobanchikov

    struct SPipelineConfiguration
    {
        SShader* m_apShaders[eST_NUM];

        SPipelineConfiguration();

        //  Confetti BEGIN: Igor Lobanchikov
        ~SPipelineConfiguration();
        SPipelineConfiguration(const SPipelineConfiguration& other);
        SPipelineConfiguration& operator=(const SPipelineConfiguration& other);
        void    SetVertexDescriptor(void*);

        void*   m_pVertexDescriptor;
        SAttachmentConfiguration    m_AttachmentConfiguration;
        //  Confetti End: Igor Lobanchikov
    };

    DXGL_DECLARE_REF_COUNTED(struct, SPipeline)
    {
        CContext* m_pContext;
        SPipelineConfiguration m_kConfiguration;

        SPipeline(const SPipelineConfiguration&kConfiguration, CContext * pContext);
        ~SPipeline();

        //  Confetti BEGIN: Igor Lobanchikov
        id<MTLRenderPipelineState> m_PipelineState;
        //  Confetti End: Igor Lobanchikov
        id<MTLComputePipelineState> m_ComputePipelineState;
    };

    DXGL_DECLARE_REF_COUNTED(struct, SInputLayout)
    {
        //  Confetti BEGIN: Igor Lobanchikov
        SInputLayout();
        ~SInputLayout();
        SInputLayout(const SInputLayout&other);
        SInputLayout& operator=(const SInputLayout& other);

        void*   m_pVertexDescriptor;
        //  Confetti End: Igor Lobanchikov
    };

    struct SResourceSlotMapping
    {
        enum
        {
            INVALID_INDEX = 0xFFFFFFFFu
        };

        uint32 m_uFirst;
        uint32 m_uCount;

        uint32 GetIndex(uint32 uSlot) const { return uSlot >= m_uCount ? INVALID_INDEX : m_uFirst + uSlot; }
    };

    typedef SShaderReflection TShaderReflection;

    bool InitializeShaderReflection(SShaderReflection* pReflection, const void* pvData);

    bool CompileShader(SShader* pShader, SSource* aSourceVersions, SGLShader* pGLShader, id<MTLDevice> mtlDevice);
    bool CompilePipeline(SPipeline* pPipeline, CDevice* pDevice);
    bool InitializeShader(SShader* pShader, const void* pvSourceData, size_t uSourceSize, id<MTLDevice> mtlDevice);
    bool InitializeShaderUniformBufferSlots(SPipeline* pPipeline, SShader* pShader, const TShaderReflection& kReflection, SResourceSlotMapping kSlotMap);
    bool InitializeShaderTextureUnitSlots(SPipeline* pPipeline, SShader* pShader, const TShaderReflection& kReflection, SResourceSlotMapping kSlotMap);
    SInputLayoutPtr CreateInputLayout(const D3D11_INPUT_ELEMENT_DESC* pInputElementDescs, uint32 uNumElements, const TShaderReflection& kReflection);
}

#endif //__GLSHADER__

