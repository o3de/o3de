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

// Description : Direct3D rendering pipeline.


#include "RenderDll_precompiled.h"
#include "DriverD3D.h"
#include <I3DEngine.h>
#include <IMovieSystem.h>
#include <CryHeaders.h>

#include "RenderBus.h"
#include "D3DPostProcess.h"
#include "D3DStereo.h"
#include "D3DHWShader.h"
#include "D3DTiledShading.h"
#include <Pak/CryPakUtils.h>
#include "Shaders/CShader.h"
#include "Shaders/RemoteCompiler.h"
#include "ReverseDepth.h"
#include "MultiLayerAlphaBlendPass.h"
#include "Textures/TextureManager.h"


#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define D3DRENDPIPELINE_CPP_SECTION_1 1
#define D3DRENDPIPELINE_CPP_SECTION_2 2
#define D3DRENDPIPELINE_CPP_SECTION_3 3
#define D3DRENDPIPELINE_CPP_SECTION_4 4
#define D3DRENDPIPELINE_CPP_SECTION_5 5
#endif

#if defined(FEATURE_SVO_GI)
#include "D3D_SVO.h"
#endif
#include "RenderCapabilities.h"
#include <ISystem.h>
#include "GraphicsPipeline/Common/GraphicsPipelinePass.h"
#include "Common/RenderView.h"
#include "GraphicsPipeline/FurBendData.h"
#include "GraphicsPipeline/FurPasses.h"

#include <HMDBus.h>
#include "MathConversion.h"

#include <AzCore/Jobs/LegacyJobExecutor.h>
#include <AzCore/std/algorithm.h>

extern SHWOccZBuffer HWZBuffer;

const static int BEFORE_WATER = 0;
const static int AFTER_WATER  = 1;

const int CD3D9Renderer::s_gmemRendertargetSlots[eGT_PathCount][eGT_RenderTargetCount] =
{
//  { eGT_Diffuse, eGT_Specular, eGT_Normals, eGT_DepthStencil, eGT_DiffuseLight, eGT_SpecularLight, eGT_VelocityBuffer }
    {  -1,           -1,             -1,             -1,             -1,                 -1,             -1             },  // eGT_REGULAR_PATH
    {   1,            2,              5,              3,              4,                  0,             -1             },  // eGT_256bpp_PATH
    {   1,            2,              0,              3,              1,                  0,              4             }   // eGT_128bpp_PATH
};

//============================================================================================
// Shaders rendering
//============================================================================================

//============================================================================================
// Init Shaders rendering
void CD3D9Renderer::EF_InitWaveTables()
{
    int i;

    //Init wave Tables
    for (i = 0; i < SRenderPipeline::sSinTableCount; i++)
    {
        float f = (float)i;

        m_RP.m_tSinTable[i] = sin_tpl(f * (360.0f / (float)SRenderPipeline::sSinTableCount) * (float)M_PI / 180.0f);
    }
}

static DXGI_FORMAT AttributeTypeDXGIFormatTable[(unsigned int)AZ::Vertex::AttributeType::NumTypes] =
{
    DXGI_FORMAT::DXGI_FORMAT_R16_FLOAT, //Float16_1"
    DXGI_FORMAT::DXGI_FORMAT_R16G16_FLOAT, //Float16_2"
    DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT, //Float16_4"

    DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT, //Float32_1"
    DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT, //Float32_2"
    DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, //Float32_3"
    DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT, //Float32_4"

    DXGI_FORMAT::DXGI_FORMAT_R8_UNORM, //Byte_1"
    DXGI_FORMAT::DXGI_FORMAT_R8G8_UNORM, //Byte_2"
    DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM, //Byte_4"

    DXGI_FORMAT::DXGI_FORMAT_R16_TYPELESS, //Short_1"
    DXGI_FORMAT::DXGI_FORMAT_R16G16_TYPELESS, //Short_2"
    DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_TYPELESS, //Short_4"

    DXGI_FORMAT::DXGI_FORMAT_R16_UINT, //UInt16_1"
    DXGI_FORMAT::DXGI_FORMAT_R16G16_UINT, //UInt16_2"
    DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_UINT, //UInt16_4"

    DXGI_FORMAT::DXGI_FORMAT_R32_UINT, //UInt32_1"
    DXGI_FORMAT::DXGI_FORMAT_R32G32_UINT, //UInt32_2"
    DXGI_FORMAT::DXGI_FORMAT_R32G32B32_UINT, //UInt32_3"
    DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_UINT //UInt32_4"
};

AZStd::vector<D3D11_INPUT_ELEMENT_DESC> GetD3D11Declaration(const AZ::Vertex::Format& vertexFormat)
{
    AZStd::vector<D3D11_INPUT_ELEMENT_DESC> declaration;
    uint offset = 0;
    // semanticIndices is a vector of zeros that will be incremented for each attribute that shares a usage/semantic name
    uint semanticIndices[(uint)AZ::Vertex::AttributeUsage::NumUsages] = { 0 };

    uint32 attributeCount = 0;
    const uint8* vertexAttributes = vertexFormat.GetAttributes(attributeCount);
    for (uint ii = 0; ii < attributeCount; ++ii)
    {
        const uint8 attribute = vertexAttributes[ii];

        D3D11_INPUT_ELEMENT_DESC elementDescription;
        AZ::Vertex::AttributeUsage attributeUsage = AZ::Vertex::Attribute::GetUsage(attribute);
        AZ::Vertex::AttributeType attributeType = AZ::Vertex::Attribute::GetType(attribute);
        // TEXCOORD semantic name used for Tangents and BiTangents.
        if (attributeUsage == AZ::Vertex::AttributeUsage::Tangent || attributeUsage == AZ::Vertex::AttributeUsage::BiTangent)
        {
            attributeUsage = AZ::Vertex::AttributeUsage::TexCoord;
        }
        elementDescription.SemanticName = AZ::Vertex::Attribute::GetSemanticName(attribute).c_str();

        // Get the number of inputs with this usage up to this point, then increment that number
        elementDescription.SemanticIndex = semanticIndices[(uint)attributeUsage];
        semanticIndices[(uint)attributeUsage]++;

        elementDescription.Format = AttributeTypeDXGIFormatTable[(uint)attributeType];

        elementDescription.AlignedByteOffset = offset;
        offset += AZ::Vertex::Attribute::GetByteLength(attribute);

        elementDescription.InputSlot = 0;
        elementDescription.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
        elementDescription.InstanceDataStepRate = 0;
        declaration.push_back(elementDescription);
    }
    declaration.shrink_to_fit();
    return declaration;
}

// build vertex declarations on demand (for programmable pipeline)
void CD3D9Renderer::EF_OnDemandVertexDeclaration(SOnDemandD3DVertexDeclaration& out,
    const int nStreamMask, const AZ::Vertex::Format& vertexFormat, const bool bMorph, const bool bInstanced)
{
    uint32 j;

    AZStd::vector<D3D11_INPUT_ELEMENT_DESC>& declarationElements = m_RP.m_D3DVertexDeclarations[vertexFormat.GetEnum()].m_Declaration;

    if (bInstanced)
    {
        // Create instanced vertex declaration
        for (j = 0; j <declarationElements.size(); j++)
        {
            D3D11_INPUT_ELEMENT_DESC elem = declarationElements[j];
            elem.InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA;
            elem.InstanceDataStepRate = 1;
            out.m_Declaration.push_back(elem);
        }
    }
    else
    {
        for (j = 0; j < declarationElements.size(); j++)
        {
            out.m_Declaration.push_back(declarationElements[j]);
        }
    }

    for (j = 1; j < VSF_NUM; j++)
    {
        if (!(nStreamMask & (1 << (j - 1))))
        {
            continue;
        }
        int n;
        for (n = 0; n < m_RP.m_D3DStreamProperties[j].m_nNumElements; n++)
        {
            out.m_Declaration.push_back(m_RP.m_D3DStreamProperties[j].m_pElements[n]);
        }
    }

    if (bMorph)
    {
        uint32 dwNumWithoutMorph = out.m_Declaration.size();

        for (j = 0; j < dwNumWithoutMorph; j++)
        {
            D3D11_INPUT_ELEMENT_DESC El = out.m_Declaration[j];
            El.InputSlot += VSF_MORPHBUDDY;
            El.SemanticIndex += 8;
            out.m_Declaration.push_back(El);
        }
        D3D11_INPUT_ELEMENT_DESC El = { "BLENDWEIGHT", 1, DXGI_FORMAT_R32G32_FLOAT, VSF_MORPHBUDDY_WEIGHTS, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }; // BlendWeight
        out.m_Declaration.push_back(El);
    }
}


void CD3D9Renderer::EF_InitD3DVertexDeclarations()
{
    for (int nFormat = 1; nFormat < eVF_Max; ++nFormat)
    {
        AZ::Vertex::Format vertexFormat = AZ::Vertex::Format((EVertexFormat)nFormat);
        m_RP.m_D3DVertexDeclarations[nFormat].m_Declaration = GetD3D11Declaration(vertexFormat);
        m_RP.m_vertexFormats[nFormat] = vertexFormat;
    }

    //=============================================================================
    // Additional streams declarations:

    // Tangents stream
    static D3D11_INPUT_ELEMENT_DESC VElemTangents[] =
    {
#ifdef TANG_FLOATS
        { "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, VSF_TANGENTS, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },   // Binormal
        { "BINORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, VSF_TANGENTS, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // Tangent
#else
        { "TANGENT", 0, DXGI_FORMAT_R16G16B16A16_SNORM, VSF_TANGENTS, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },   // Binormal
        { "BINORMAL", 0, DXGI_FORMAT_R16G16B16A16_SNORM, VSF_TANGENTS, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 },  // Tangent
#endif
    };
    // Tangents stream
    static D3D11_INPUT_ELEMENT_DESC VElemQTangents[] =
    {
#ifdef TANG_FLOATS
        { "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, VSF_QTANGENTS, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },   // Binormal
#else
        { "TANGENT", 0, DXGI_FORMAT_R16G16B16A16_SNORM, VSF_QTANGENTS, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },   // Binormal
#endif
    };

    //HW Skin stream
    static D3D11_INPUT_ELEMENT_DESC VElemHWSkin[] =
    {
        { "BLENDWEIGHT", 0, DXGI_FORMAT_R8G8B8A8_UNORM, VSF_HWSKIN_INFO, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // BlendWeight
        { "BLENDINDICES", 0, DXGI_FORMAT_R16G16B16A16_SINT, VSF_HWSKIN_INFO, 4, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // BlendIndices
    };

#if ENABLE_NORMALSTREAM_SUPPORT
    static D3D11_INPUT_ELEMENT_DESC VElemNormals[] =
    {
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, VSF_NORMALS, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
#endif

    static D3D11_INPUT_ELEMENT_DESC VElemVelocity[] =
    {
        { "POSITION", 3, DXGI_FORMAT_R32G32B32_FLOAT, VSF_VERTEX_VELOCITY, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // Velocity
    };


    // stream 1 (Tangent basis vectors)
    // stream 2 (QTangents info)
    // stream 3 (HW skin info)
    // stream 4 (Velocity)
    // stream 5 (Normals)
    m_RP.m_D3DStreamProperties[VSF_GENERAL].m_pElements = NULL;
    m_RP.m_D3DStreamProperties[VSF_GENERAL].m_nNumElements = 0;
    m_RP.m_D3DStreamProperties[VSF_TANGENTS].m_pElements = VElemTangents;
    m_RP.m_D3DStreamProperties[VSF_TANGENTS].m_nNumElements = sizeof(VElemTangents) / sizeof(D3D11_INPUT_ELEMENT_DESC);
    m_RP.m_D3DStreamProperties[VSF_QTANGENTS].m_pElements = VElemQTangents;
    m_RP.m_D3DStreamProperties[VSF_QTANGENTS].m_nNumElements = sizeof(VElemQTangents) / sizeof(D3D11_INPUT_ELEMENT_DESC);
    m_RP.m_D3DStreamProperties[VSF_HWSKIN_INFO].m_pElements = VElemHWSkin;
    m_RP.m_D3DStreamProperties[VSF_HWSKIN_INFO].m_nNumElements = sizeof(VElemHWSkin) / sizeof(D3D11_INPUT_ELEMENT_DESC);
    m_RP.m_D3DStreamProperties[VSF_VERTEX_VELOCITY].m_pElements = VElemVelocity;
    m_RP.m_D3DStreamProperties[VSF_VERTEX_VELOCITY].m_nNumElements = sizeof(VElemVelocity) / sizeof(D3D11_INPUT_ELEMENT_DESC);
#if ENABLE_NORMALSTREAM_SUPPORT
    m_RP.m_D3DStreamProperties[VSF_NORMALS].m_pElements = VElemNormals;
    m_RP.m_D3DStreamProperties[VSF_NORMALS].m_nNumElements = sizeof(VElemNormals) / sizeof(D3D11_INPUT_ELEMENT_DESC);
#endif


    m_CurVertBufferSize = 0;
    m_CurIndexBufferSize = 0;
}

_inline static void* sAlign0x20(byte* vrts)
{
    return (void*)(((INT_PTR)vrts + 0x1f) & ~0x1f);
}

// Init shaders pipeline
void CD3D9Renderer::EF_Init()
{
    // Ensure only one call to EF_Init per call to FX_PipelineShutdown
    if (m_shaderPipelineInitialized)
    {
        return;
    }

    bool nv = 0;

    if IsCVarConstAccess(constexpr) (CV_r_logTexStreaming && m_logFileStrHandle == AZ::IO::InvalidHandle)
    {
        m_logFileStrHandle = fxopen("Direct3DLogStreaming.txt", "w");
        if (m_logFileStrHandle != AZ::IO::InvalidHandle)
        {
            iLog->Log("Direct3D texture streaming log file '%s' opened", "Direct3DLogStreaming.txt");
            char time[128];
            char date[128];

            azstrtime(time);
            azstrdate(date);

            AZ::IO::Print(m_logFileStrHandle, "\n==========================================\n");
            AZ::IO::Print(m_logFileStrHandle, "Direct3D Textures streaming Log file opened: %s (%s)\n", date, time);
            AZ::IO::Print(m_logFileStrHandle, "==========================================\n");
        }
    }

    m_RP.m_MaxVerts = 16384;
    m_RP.m_MaxTris = 16384 * 3;

    iLog->Log("Allocate render buffer for particles (%d verts, %d tris)...", m_RP.m_MaxVerts, m_RP.m_MaxTris);

    int n = 0;

    int nSizeV = sizeof(SVF_P3F_C4B_T4B_N3F2);//this is the vertex format used for particles

    n += nSizeV * m_RP.m_MaxVerts + 32;

    n += sizeof(SPipTangents) * m_RP.m_MaxVerts + 32;

    //m_RP.mRendIndices;
    n += sizeof(uint16) * 3 * m_RP.m_MaxTris + 32;

    {
        byte* buf = new byte[n];
        m_RP.m_SizeSysArray = n;
        m_RP.m_SysArray = buf;
        if (!buf)
        {
            iConsole->Exit("Can't allocate buffers for RB");
        }

        memset(buf, 0, n);

        m_RP.m_StreamPtr.Ptr = sAlign0x20(buf);
        buf += sizeof(SVF_P3F_C4B_T4B_N3F2) * m_RP.m_MaxVerts + 32;

        m_RP.m_StreamPtrTang.Ptr = sAlign0x20(buf);
        buf += sizeof(SPipTangents) * m_RP.m_MaxVerts + 32;

        m_RP.m_RendIndices = (uint16*)sAlign0x20(buf);
        m_RP.m_SysRendIndices = m_RP.m_RendIndices;
        buf += sizeof(uint16) * 3 * m_RP.m_MaxTris + 32;
    }

    EF_Restore();

    EF_InitWaveTables();
    EF_InitD3DVertexDeclarations();
    CHWShader_D3D::mfInit();

    {
        for (int i = 0; i < RT_COMMAND_BUF_COUNT; i++)
        {
            for (int j = 0; j < MAX_RECURSION_LEVELS; j++)
            {
                m_RP.m_DLights[i][j].Reserve(MAX_LIGHTS_NUM);
            }
        }
    }

    // Init RenderObjects
    {
        m_RP.m_nNumObjectsInPool = SRenderPipeline::sNumObjectsInPool;

        if (m_RP.m_ObjectsPool != nullptr)
        {
            for (int j = 0; j < (int)(m_RP.m_nNumObjectsInPool * RT_COMMAND_BUF_COUNT); j++)
            {
                CRenderObject* pRendObj = &m_RP.m_ObjectsPool[j];
                pRendObj->~CRenderObject();
            }
            CryModuleMemalignFree(m_RP.m_ObjectsPool);
        }

        // we use a plain allocation and placement new here to garantee the alignment, when using array new, the compiler can store it's size and break the alignment
        m_RP.m_ObjectsPool = (CRenderObject*)CryModuleMemalign(sizeof(CRenderObject) * (m_RP.m_nNumObjectsInPool * RT_COMMAND_BUF_COUNT), 16);
        for (int j = 0; j < (int)(m_RP.m_nNumObjectsInPool * RT_COMMAND_BUF_COUNT); j++)
        {
            CRenderObject* pRendObj = new(&m_RP.m_ObjectsPool[j])CRenderObject();
        }


        CRenderObject** arrPrefill = (CRenderObject**)(alloca(m_RP.m_nNumObjectsInPool * sizeof(CRenderObject*)));
        for (int j = 0; j < RT_COMMAND_BUF_COUNT; j++)
        {
            for (int k = 0; k < m_RP.m_nNumObjectsInPool; ++k)
            {
                arrPrefill[k] = &m_RP.m_ObjectsPool[j * m_RP.m_nNumObjectsInPool + k];
            }

            m_RP.m_TempObjects[j].PrefillContainer(arrPrefill, m_RP.m_nNumObjectsInPool);
            m_RP.m_TempObjects[j].resize(0);
        }
    }
    // Init identity RenderObject
    SAFE_DELETE(m_RP.m_pIdendityRenderObject);
    m_RP.m_pIdendityRenderObject = aznew CRenderObject();
    m_RP.m_pIdendityRenderObject->Init();
    m_RP.m_pIdendityRenderObject->m_II.m_AmbColor = Col_White;
    m_RP.m_pIdendityRenderObject->m_II.m_Matrix.SetIdentity();
    m_RP.m_pIdendityRenderObject->m_RState = 0;
    m_RP.m_pIdendityRenderObject->m_ObjFlags |= FOB_RENDERER_IDENDITY_OBJECT;

    // create hdr element
    m_RP.m_pREHDR = (CREHDRProcess*)EF_CreateRE(eDATA_HDRProcess);
    // create deferred shading element
    m_RP.m_pREDeferredShading = (CREDeferredShading*)EF_CreateRE(eDATA_DeferredShading);

    // Create post process render element
    m_RP.m_pREPostProcess = (CREPostProcess*)EF_CreateRE(eDATA_PostProcess);

    // Initialize posteffects manager
    if (!m_pPostProcessMgr)
    {
        m_pPostProcessMgr = new CPostEffectsMgr;
        m_pPostProcessMgr->Init();
    }

    if (!m_pWaterSimMgr)
    {
        m_pWaterSimMgr = new CWater;
    }

    //SDynTexture::CreateShadowPool();

    m_RP.m_fLastWaterFOVUpdate = 0;
    m_RP.m_LastWaterViewdirUpdate = Vec3(0, 0, 0);
    m_RP.m_LastWaterUpdirUpdate = Vec3(0, 0, 0);
    m_RP.m_LastWaterPosUpdate = Vec3(0, 0, 0);
    m_RP.m_fLastWaterUpdate = 0;
    m_RP.m_nLastWaterFrameID = 0;
    m_RP.m_nCommitFlags = FC_ALL;

    m_nMaterialAnisoHighSampler = CTexture::GetTexState(STexState(FILTER_ANISO16X, false));
    m_nMaterialAnisoLowSampler = CTexture::GetTexState(STexState(FILTER_ANISO4X, false));
    m_nMaterialAnisoSamplerBorder = CTexture::GetTexState(STexState(FILTER_ANISO16X, TADDR_BORDER, TADDR_BORDER, TADDR_BORDER, 0x0));

    CDeferredShading::CreateDeferredShading();

    if (m_pStereoRenderer)
    {
        m_pStereoRenderer->CreateResources();
        m_pStereoRenderer->Update();
    }

    MultiLayerAlphaBlendPass::InstallInstance();
    FurPasses::InstallInstance();
    
    // Initialize occlusion data
    InvalidateCoverageBufferData();

    AZ_Assert(m_pBackBuffer == m_pBackBuffers[CD3D9Renderer::GetCurrentBackBufferIndex(m_pSwapChain)], "Swap chain was not properly swapped");

    GetDeviceContext().OMSetRenderTargets(1, &m_pBackBuffer, m_pNativeZBuffer);

    ResetToDefault();

    m_shaderPipelineInitialized = true;
}

// Invalidate shaders pipeline
void CD3D9Renderer::FX_Invalidate()
{
    for (int i = 0; i < SRenderPipeline::nNumParticleVertexIndexBuffer; ++i)
    {
        SAFE_DELETE(m_RP.m_pParticleVertexBuffer[i]);
        SAFE_DELETE(m_RP.m_pParticleIndexBuffer[i]);
    }
}

void CD3D9Renderer::FX_UnbindStreamSource(D3DBuffer* buffer)
{
    IF (!buffer, 0)
    {
        return;
    }

    for (int i = 0; i < MAX_STREAMS; i++)
    {
        IF (m_RP.m_VertexStreams[i].pStream == buffer, 0)
        {
            ID3D11Buffer* pNullBuffer = NULL;
            uint32 zeroStrideOffset = 0;
            m_DevMan.BindVB(i, 1, &pNullBuffer, &zeroStrideOffset, &zeroStrideOffset);
            m_RP.m_VertexStreams[i].pStream = NULL;
        }
    }
    IF (m_RP.m_pIndexStream == buffer, 0)
    {
        m_DevMan.BindIB(NULL, 0, DXGI_FORMAT_R16_UINT);
        m_RP.m_pIndexStream = NULL;
    }

    // commit state changes a second time to really unbind right now, not during the next DrawXXX or Commit
    m_DevMan.CommitDeviceStates();
}

// Restore shaders pipeline
void CD3D9Renderer::EF_Restore()
{
    if (!m_RP.m_MaxTris)
    {
        return;
    }

    FX_Invalidate();

    for (int i = 0; i < RT_COMMAND_BUF_COUNT; ++i)
    {
        m_ComputeVerticesJobExecutors[i].WaitForCompletion();
    }

    // preallocate video memory buffer for particles when using the job system
    for (int i = 0; i < SRenderPipeline::nNumParticleVertexIndexBuffer; ++i)
    {
        m_RP.m_pParticleVertexBuffer[i] = new FencedVB<byte>(CV_r_ParticleVerticePoolSize, sizeof(SVF_P3F_C4B_T4B_N3F2));
        m_RP.m_pParticleIndexBuffer[i] = new FencedIB<uint16>(CV_r_ParticleVerticePoolSize * 3, sizeof(uint16));

        m_RP.m_pParticleVertexVideoMemoryBase[i] = NULL;
        m_RP.m_pParticleindexVideoMemoryBase[i] = NULL;

        m_RP.m_nParticleVertexOffset[i] = 0;
        m_RP.m_nParticleIndexOffset[i] = 0;

        m_RP.m_nParticleVertexBufferAvailableMemory = CV_r_ParticleVerticePoolSize * sizeof(SVF_P3F_C4B_T4B_N3F2);
        m_RP.m_nParticleIndexBufferAvailableMemory = CV_r_ParticleVerticePoolSize * 3 * sizeof(uint16);
    }
}

void CD3D9Renderer::OnRendererFreeResources(int flags)
{
    // If texture resources are about to be freed by the renderer
    if (flags & FRR_TEXTURES)
    {
        // Release the occlusion readback textures before CTexture::Shutdown is called
        for (size_t idx = 0; idx < s_numOcclusionReadbackTextures; idx++)
        {
            m_occlusionData[idx].Destroy();
        }
    }
}

// Shutdown shaders pipeline
void CD3D9Renderer::FX_PipelineShutdown(bool bFastShutdown)
{
    if (!m_shaderPipelineInitialized)
    {
        return;
    }

    uint32 i, j, n;

    FX_Invalidate();

    MultiLayerAlphaBlendPass::ReleaseInstance();
    FurPasses::ReleaseInstance();

    SAFE_DELETE_ARRAY(m_RP.m_SysArray);
    m_RP.m_SysVertexPool[0].Free();
    m_RP.m_SysIndexPool[0].Free();
#if !defined(STRIP_RENDER_THREAD)
    m_RP.m_SysVertexPool[1].Free();
    m_RP.m_SysIndexPool[1].Free();
#endif
    for (int index=0; index<eVF_Max; ++index)
    {
        m_RP.m_D3DVertexDeclarations[index].m_Declaration.clear();
    }

    // Loop through the 2D array of hash maps
    for (auto& stream : m_RP.m_D3DVertexDeclarationCache)
    {
        for (auto& vertexFormatHashMap : stream)
        {
            for (auto& crcVertexFormatPair : vertexFormatHashMap)
            {
                // Release the vertex format declaration
                SAFE_RELEASE(crcVertexFormatPair.second.m_pDeclaration);
            }
        }
    }

    for (n = 0; n < RT_COMMAND_BUF_COUNT; n++)
    {
        for (j = 0; j < MAX_RECURSION_LEVELS; j++)
        {
            for (i = 0; i < CREClientPoly::m_PolysStorage[n][j].Num(); i++)
            {
                CREClientPoly::m_PolysStorage[n][j][i]->Release(true);
            }
            CREClientPoly::m_PolysStorage[n][j].Free();
        }
    }

    SAFE_RELEASE(m_RP.m_pREHDR);
    SAFE_RELEASE(m_RP.m_pREDeferredShading);
    SAFE_RELEASE(m_RP.m_pREPostProcess);
    SAFE_DELETE(m_pPostProcessMgr);
    SAFE_DELETE(m_pWaterSimMgr);
    
    for (size_t idx = 0; idx < s_numOcclusionReadbackTextures; idx++)
    {
        m_occlusionData[idx].Destroy();
    }

    //if (m_pStereoRenderer)
    //  m_pStereoRenderer->ReleaseResources();

#if defined(ENABLE_RENDER_AUX_GEOM)
    if (m_pRenderAuxGeomD3D)
    {
        m_pRenderAuxGeomD3D->ReleaseShader();
    }
#endif

    if (!bFastShutdown)
    {
        CHWShader_D3D::ShutDown();
    }

    m_RP.m_pCurTechnique = 0;

    if (m_RP.m_ObjectsPool != nullptr)
    {
        for (int objIdx = 0; objIdx < (int)(m_RP.m_nNumObjectsInPool * RT_COMMAND_BUF_COUNT); objIdx++)
        {
            CRenderObject* pRendObj = &m_RP.m_ObjectsPool[objIdx];
            pRendObj->~CRenderObject();
        }
        CryModuleMemalignFree(m_RP.m_ObjectsPool);
    }
    m_RP.m_ObjectsPool = NULL;
    for (int k = 0; k < RT_COMMAND_BUF_COUNT; ++k)
    {
        m_RP.m_TempObjects[k].clear();
    }

    m_DevMan.SetBlendState(nullptr, nullptr, 0);
    m_DevMan.SetRasterState(nullptr);
    m_DevMan.SetDepthStencilState(nullptr, 0);

    for (i = 0; i < m_StatesDP.Num(); ++i)
    {
        SAFE_RELEASE(m_StatesDP[i].pState);
    }
    for (i = 0; i < m_StatesRS.Num(); ++i)
    {
        SAFE_RELEASE(m_StatesRS[i].pState);
    }
    for (i = 0; i < m_StatesBL.Num(); ++i)
    {
        SAFE_RELEASE(m_StatesBL[i].pState);
    }
    m_StatesBL.Free();
    m_StatesRS.Free();
    m_StatesDP.Free();
    m_nCurStateRS = ~0U;
    m_nCurStateDP = ~0U;
    m_nCurStateBL = ~0U;

    CDeferredShading::DestroyDeferredShading();

    for (unsigned int a = 0; a < m_OcclQueries.size(); a++)
    {
        m_OcclQueries[a].Release();
    }

    m_shaderPipelineInitialized = false;
}

void CD3D9Renderer::FX_ResetPipe()
{
    int i;

    FX_SetState(GS_NODEPTHTEST);
    D3DSetCull(eCULL_None);
    m_RP.m_FlagsStreams_Decl = 0;
    m_RP.m_FlagsStreams_Stream = 0;
    m_RP.m_FlagsPerFlush = 0;
    m_RP.m_FlagsShader_RT = 0;
    m_RP.m_FlagsShader_MD = 0;
    m_RP.m_FlagsShader_MDV = 0;
    m_RP.m_FlagsShader_LT = 0;
    m_RP.m_nCommitFlags = FC_ALL;
    m_RP.m_PersFlags2 |= RBPF2_COMMIT_PF | RBPF2_COMMIT_CM;
    
    m_RP.m_nDeferredPrimitiveID = SHAPE_PROJECTOR;

    HRESULT h = FX_SetIStream(NULL, 0, Index16);

    EF_Scissor(false, 0, 0, 0, 0);
    m_RP.m_pShader = NULL;
    m_RP.m_pCurTechnique = NULL;
    for (i = 1; i < VSF_NUM; i++)
    {
        if (m_RP.m_PersFlags1 & (RBPF1_USESTREAM << i))
        {
            m_RP.m_PersFlags1 &= ~(RBPF1_USESTREAM << i);
            h = FX_SetVStream(i, NULL, 0, 0);
        }
    }

    CHWShader_D3D::mfSetGlobalParams();
}

void DrawFullScreenQuad(float fLeftU, float fTopV, float fRightU, float fBottomV);


//==========================================================================
// Calculate current scene node matrices
void CD3D9Renderer::EF_SetCameraInfo()
{
    m_pRT->RC_SetCamera();
}

void CD3D9Renderer::RT_SetCameraInfo()
{
    GetModelViewMatrix(&m_ViewMatrix(0, 0));
    m_CameraMatrix = m_ViewMatrix;

    GetProjectionMatrix(&m_ProjMatrix(0, 0));

    SThreadInfo* const pShaderThreadInfo = &(m_RP.m_TI[m_RP.m_nProcessThreadID]);

    if (pShaderThreadInfo->m_PersFlags & RBPF_OBLIQUE_FRUSTUM_CLIPPING)
    {
        Matrix44A mObliqueProjMatrix;
        mObliqueProjMatrix.SetIdentity();

        mObliqueProjMatrix.m02 = pShaderThreadInfo->m_pObliqueClipPlane.n[0];
        mObliqueProjMatrix.m12 = pShaderThreadInfo->m_pObliqueClipPlane.n[1];
        mObliqueProjMatrix.m22 = pShaderThreadInfo->m_pObliqueClipPlane.n[2];
        mObliqueProjMatrix.m32 = pShaderThreadInfo->m_pObliqueClipPlane.d;

        m_ProjMatrix = m_ProjMatrix * mObliqueProjMatrix;
    }

    bool bApplySubpixelShift = !(m_RP.m_PersFlags2 & RBPF2_NOPOSTAA);
    bApplySubpixelShift &= !(pShaderThreadInfo->m_PersFlags & (RBPF_DRAWTOTEXTURE | RBPF_SHADOWGEN));

    m_ProjNoJitterMatrix = m_ProjMatrix;
    m_ViewProjNoJitterMatrix = m_CameraMatrix * m_ProjMatrix;

    if (bApplySubpixelShift)
    {
        m_ProjMatrix.m20 += m_TemporalJitterClipSpace.x;
        m_ProjMatrix.m21 += m_TemporalJitterClipSpace.y;
    }

    m_ViewProjMatrix = m_CameraMatrix * m_ProjMatrix;
    m_ViewProjNoTranslateMatrix = m_CameraZeroMatrix[m_RP.m_nProcessThreadID] * m_ProjMatrix;

    // specialized matrix inversion for enhanced precision
    Matrix44_tpl<f64> mProjInv;
    if (mathMatrixPerspectiveFovInverse(&mProjInv, &m_ProjMatrix))
    {
        Matrix44_tpl<f64>  mViewInv;
        mathMatrixLookAtInverse(&mViewInv, &m_CameraMatrix);
        m_ViewProjInverseMatrix = mProjInv * mViewInv;
    }
    else
    {
        m_ViewProjInverseMatrix = m_ViewProjMatrix.GetInverted();
    }

    if (m_RP.m_ObjFlags & FOB_NEAREST)
    {
        m_CameraMatrixNearest = m_CameraMatrix;
    }

    pShaderThreadInfo->m_PersFlags |= RBPF_FP_DIRTY;
    m_RP.m_ObjFlags = 0;

    m_NewViewport.fMinZ = pShaderThreadInfo->m_cam.GetZRangeMin();
    m_NewViewport.fMaxZ = pShaderThreadInfo->m_cam.GetZRangeMax();
    m_bViewportDirty = true;

    CHWShader_D3D::mfSetCameraParams();
}

/*
    Applies the correct HMD tracking pose to the camera. This is done
    on the render thread to ensure that we are rendering with the most
    up to date poses.
*/
void CD3D9Renderer::RT_SetStereoCamera()
{
    int threadId = m_RP.m_nProcessThreadID;

    if (m_pStereoRenderer->IsRenderingToHMD())
    {
        CCamera camera = m_RP.m_TI[threadId].m_cam;

        const AZ::VR::TrackingState* trackingState = nullptr;
        EBUS_EVENT_RESULT(trackingState, AZ::VR::HMDDeviceRequestBus, GetTrackingState);
        if (trackingState)
        {
            const Vec3 position = camera.GetEntityPos();
            Quat rotation = camera.GetEntityRotation();

            const Vec3 trackedPosition = rotation * AZVec3ToLYVec3(trackingState->pose.position);
            rotation = rotation * AZQuaternionToLYQuaternion(trackingState->pose.orientation);

            Matrix34 camMat = Matrix34(rotation);
            camMat.SetTranslation(position + trackedPosition);

            AZ::VR::PerEyeCameraInfo cameraInfo;
            EBUS_EVENT(AZ::VR::HMDDeviceRequestBus, GetPerEyeCameraInfo, static_cast<EStereoEye>(gRenDev->m_CurRenderEye), camera.GetNearPlane(), camera.GetFarPlane(), cameraInfo);

            const float asymmetricHorizontalTranslation = cameraInfo.frustumPlane.horizontalDistance * camera.GetNearPlane();
            const float asymmetricVerticalTranslation = cameraInfo.frustumPlane.verticalDistance * camera.GetNearPlane();

            const Vec3 eyeOffset = AZVec3ToLYVec3(cameraInfo.eyeOffset);

            const Matrix34 stereoMat = Matrix34::CreateTranslationMat(eyeOffset);
            camera.SetMatrix(camMat * stereoMat);
            camera.SetFrustum(1, 1, cameraInfo.fov, camera.GetNearPlane(), camera.GetFarPlane(), 1.0f / cameraInfo.aspectRatio);
            camera.SetAsymmetry(asymmetricHorizontalTranslation, asymmetricHorizontalTranslation, asymmetricVerticalTranslation, asymmetricVerticalTranslation);

            SetCamera(camera);
        }
        else
        {
            AZ_Warning("VR", false, "Failed to set stereo camera: No tracking state")
        }
    }
}

// Set object transform for fixed pipeline shader
void CD3D9Renderer::FX_SetObjectTransform(CRenderObject* obj, [[maybe_unused]] CShader* pSH, [[maybe_unused]] int nTransFlags)
{
    assert(m_pRT->IsRenderThread());

    m_ViewMatrix = (Matrix44A(obj->m_II.m_Matrix).GetTransposed() * m_CameraMatrix);

    SThreadInfo* const pShaderThreadInfo = &(m_RP.m_TI[m_RP.m_nProcessThreadID]);
    pShaderThreadInfo->m_matView = m_ViewMatrix;
}


//==============================================================================
// Shader Pipeline
//=======================================================================

void CD3D9Renderer::EF_SetFogColor(const ColorF& Color)
{
    const int nThreadID = m_pRT->GetThreadList();

    m_uLastBlendFlagsPassGroup = PackBlendModeAndPassGroup();

    m_RP.m_TI[nThreadID].m_FS.m_CurColor = Color;
}

// Set current texture color op modes (used in fixed pipeline shaders)
void CD3D9Renderer::SetColorOp(byte eCo, byte eAo, byte eCa, byte eAa)
{
    if (m_bDeviceLost)
    {
        return;
    }

    // Check for the presence of a D3D device
    assert(m_Device);

    m_pRT->RC_SetColorOp(eCo, eAo, eCa, eAa);
}

void CD3D9Renderer::EF_SetColorOp(byte eCo, byte eAo, byte eCa, byte eAa)
{
    const int nThreadID = m_pRT->GetThreadList();
    SThreadInfo* const pShaderThreadInfo = &(m_RP.m_TI[nThreadID]);

    if (eCo != 255 && pShaderThreadInfo->m_eCurColorOp != eCo)
    {
        pShaderThreadInfo->m_eCurColorOp = eCo;
        pShaderThreadInfo->m_PersFlags |= RBPF_FP_DIRTY;
    }
    if (eAo != 255 && pShaderThreadInfo->m_eCurAlphaOp != eAo)
    {
        pShaderThreadInfo->m_eCurAlphaOp = eAo;
        pShaderThreadInfo->m_PersFlags |= RBPF_FP_DIRTY;
    }
    if (eCa != 255 && pShaderThreadInfo->m_eCurColorArg != eCa)
    {
        pShaderThreadInfo->m_eCurColorArg = eCa;
        pShaderThreadInfo->m_PersFlags |= RBPF_FP_DIRTY;
    }
    if (eAa != 255 && pShaderThreadInfo->m_eCurAlphaArg != eAa)
    {
        pShaderThreadInfo->m_eCurAlphaArg = eAa;
        pShaderThreadInfo->m_PersFlags |= RBPF_FP_DIRTY;
    }
}

// Set whether fixed pipeline shaders should convert linear color space to sRGB on write
void CD3D9Renderer::SetSrgbWrite(bool srgbWrite)
{
    if (m_bDeviceLost)
    {
        return;
    }

    // Check for the presence of a D3D device
    assert(m_Device);

    m_pRT->RC_SetSrgbWrite(srgbWrite);
}

void CD3D9Renderer::EF_SetSrgbWrite(bool sRGBWrite)
{
    const int nThreadID = m_pRT->GetThreadList();
    SThreadInfo* const pShaderThreadInfo = &(m_RP.m_TI[nThreadID]);

    if (pShaderThreadInfo->m_sRGBWrite != sRGBWrite)
    {
        pShaderThreadInfo->m_sRGBWrite = sRGBWrite;
        pShaderThreadInfo->m_PersFlags |= RBPF_FP_DIRTY;
    }
}

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DRENDPIPELINE_CPP_SECTION_1
    #include AZ_RESTRICTED_FILE(D3DRendPipeline_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
// <DEPRECATED>
void CD3D9Renderer::CopyFramebufferDX11(CTexture* pDst, ID3D11Resource* pSrcResource, D3DFormat srcFormat)
{
    // Simulated texture copy to overcome the format dismatch issue for texture-blit.
    // TODO: use partial update.
    CShader* pShader = CShaderMan::s_shPostEffects;
    static CCryNameTSCRC techName("TextureToTexture");
    pShader->FXSetTechnique(techName);

    // Try get the pointer to the actual backbuffer
    ID3D11Texture2D* pBackBufferTex = (ID3D11Texture2D*)pSrcResource;

    // create the shader res view on the fly
    D3DShaderResourceView* shaderResView;   // released at the end of this func
    D3D11_SHADER_RESOURCE_VIEW_DESC svDesc;
    ZeroStruct(svDesc);
    svDesc.Format = srcFormat;
    svDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    svDesc.Texture2D.MipLevels = 1;
    svDesc.Texture2D.MostDetailedMip = 0;
    HRESULT hr;
    if (!SUCCEEDED(hr = GetDevice().CreateShaderResourceView(pBackBufferTex, &svDesc, &shaderResView)))
    {
        iLog->LogError("Creating shader resource view has failed.  Code: %d", hr);
    }

    // render
    uint32 nPasses = 0;
    pShader->FXBegin(&nPasses, FEF_DONTSETTEXTURES);
    FX_PushRenderTarget(0, pDst, NULL);
    ID3D11RenderTargetView* pNullRTV = NULL;
    GetDeviceContext().OMSetRenderTargets(1, &pNullRTV, NULL);
    pShader->FXBeginPass(0);
    FX_SetState(GS_NODEPTHTEST);

    // Set shader resource
    m_DevMan.BindSRV(eHWSC_Pixel, shaderResView, 0);

    // Set sampler state:
    int tsIdx = CTexture::GetTexState(STexState(FILTER_LINEAR, true));  // get the sampler state cache line index
    ID3D11SamplerState* linearSampler = static_cast<ID3D11SamplerState*> (CTexture::s_TexStates[tsIdx].m_pDeviceState);
    m_DevMan.BindSampler(eHWSC_Pixel, &linearSampler, 0, 1);
    SPostEffectsUtils::DrawFullScreenTri(pDst->GetWidth(), pDst->GetHeight());
    // unbind backbuffer:
    D3DShaderResourceView* pNullSTV = NULL;
    m_DevMan.BindSRV(eHWSC_Pixel, pNullSTV, 0);
    CTexture::s_TexStages[0].m_DevTexture = NULL;

    pShader->FXEndPass();
    FX_PopRenderTarget(0);
    pShader->FXEnd();

    GetDeviceContext(); // explicit flush as temp target gets released in next line
    SAFE_RELEASE(shaderResView);
}
#endif

// <DEPRECATED> This function must be refactored post C3
void CD3D9Renderer::FX_ScreenStretchRect(CTexture* pDst, CTexture* pHDRSrc)
{
    PROFILE_LABEL_SCOPE("SCREEN_STRETCH_RECT");
    if (CTexture::IsTextureExist(pDst))
    {
        int iTempX, iTempY, iWidth, iHeight;
        gcpRendD3D->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);

        uint64 nPrevFlagsShaderRT = gRenDev->m_RP.m_FlagsShader_RT;
        gRenDev->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0] | g_HWSR_MaskBit[HWSR_SAMPLE2] | g_HWSR_MaskBit[HWSR_SAMPLE5]);

        {
            // update scene target before using it for water rendering
            CDeviceTexture* pDstResource = pDst->GetDevTexture();
            ID3D11RenderTargetView* pOrigRT = m_pNewTarget[0]->m_pTarget;
            ID3D11Resource* pSrcResource;

            // This is a subrect to subrect copy with no resolving or stretching
            D3D11_BOX box;
            ZeroStruct(box);
            box.right = pDst->GetWidth();
            box.bottom = pDst->GetHeight();
            box.back = 1;

            //Allow for scissoring to happen
            int sX, sY, sWdt, sHgt;
            if (EF_GetScissorState(sX, sY, sWdt, sHgt))
            {
                box.left = sX;
                box.right = sX + sWdt;
                box.top = sY;
                box.bottom = sY + sHgt;

                // Align the RECT boundaries to GPU memory layout
                box.left = box.left & 0xfffffff8;
                box.top = box.top & 0xfffffff8;
                box.right = min((int)((box.right + 8) & 0xfffffff8), iWidth);
                box.bottom = min((int)((box.bottom + 8) & 0xfffffff8), iHeight);
            }

            D3D11_RENDER_TARGET_VIEW_DESC backbufferDesc;
            if (pOrigRT)
            {
                pOrigRT->GetResource(&pSrcResource);
                pOrigRT->GetDesc(&backbufferDesc);
                if (backbufferDesc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2DMS || pHDRSrc)
                {
                    // No API side for ResolveSubresourceRegion from MS target to non-ms. Need to perform custom resolve step
                    if ((CTexture::s_ptexSceneTarget && (CTexture::s_ptexHDRTarget || pHDRSrc) && CTexture::s_ptexCurrentSceneDiffuseAccMap))
                    {
                        if (backbufferDesc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2DMS)
                        {
                            gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];
                        }

                        CTexture* pHDRTarget = pHDRSrc ? pHDRSrc : CTexture::s_ptexHDRTarget;
                        pHDRTarget->SetResolved(true);

                        FX_PushRenderTarget(0, pDst, 0);
                        FX_SetActiveRenderTargets();

                        RT_SetViewport(0, 0, pDst->GetWidth(), pDst->GetHeight());

                        static CCryNameTSCRC pTechName("TextureToTexture");
                        SPostEffectsUtils::ShBeginPass(CShaderMan::s_shPostEffects, pTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
                        FX_SetState(GS_NODEPTHTEST);

                        pHDRTarget->Apply(0, CTexture::GetTexState(STexState(FILTER_POINT, true)), EFTT_UNKNOWN, -1, m_RP.m_MSAAData.Type ? SResourceView::DefaultViewMS : SResourceView::DefaultView);

                        SPostEffectsUtils::DrawFullScreenTri(pDst->GetWidth(), pDst->GetHeight());
                        SPostEffectsUtils::ShEndPass();

                        // Restore previous viewport
                        FX_PopRenderTarget(0);
                        RT_SetViewport(iTempX, iTempY, iWidth, iHeight);

                        pHDRTarget->SetResolved(false);
                    }
                    else
                    {
                        GetDeviceContext().ResolveSubresource(pDstResource->Get2DTexture(), 0, pSrcResource, 0, backbufferDesc.Format);
                    }
                }
                else
                {
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DRENDPIPELINE_CPP_SECTION_2
    #include AZ_RESTRICTED_FILE(D3DRendPipeline_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
                    // Check if the format match (or the copysubregionresource call would fail)
                    const D3DFormat dstFmt = CTexture::DeviceFormatFromTexFormat(pDst->GetDstFormat());
                    const D3DFormat srcFmt = backbufferDesc.Format;
                    if (dstFmt == srcFmt)
                    {
#   if !defined(_RELEASE)
                        D3D11_RESOURCE_DIMENSION type;
                        pSrcResource->GetType(&type);
                        if (type != D3D11_RESOURCE_DIMENSION_TEXTURE2D)
                        {
                            __debugbreak();
                        }
#endif
                        ID3D11Texture2D* pSrcTex2D = (ID3D11Texture2D*)pSrcResource;
                        D3D11_TEXTURE2D_DESC srcTex2desc;
                        pSrcTex2D->GetDesc(&srcTex2desc);

                        box.left = min(box.left, srcTex2desc.Width);
                        box.right = min(box.right, srcTex2desc.Width);
                        box.top = min(box.top, srcTex2desc.Height);
                        box.bottom = min(box.bottom, srcTex2desc.Height);

                        GetDeviceContext().CopySubresourceRegion(pDstResource->Get2DTexture(), 0, box.left, box.top, 0, pSrcResource, 0, &box);
                    }
                    else
                    {
                        // deal with format mismatch case:
                        EF_Scissor(false, 0, 0, 0, 0); // TODO: optimize. dont use full screen pass.
                        CopyFramebufferDX11(pDst, pSrcResource, backbufferDesc.Format);
                        EF_Scissor(true, sX, sY, sWdt, sHgt);
                    }
#endif
                }
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DRENDPIPELINE_CPP_SECTION_3
    #include AZ_RESTRICTED_FILE(D3DRendPipeline_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
                SAFE_RELEASE(pSrcResource);
#endif
            }
        }

        gRenDev->m_RP.m_FlagsShader_RT = nPrevFlagsShaderRT;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////

bool CD3D9Renderer::FX_SkinRendering(bool bEnable)
{
    if (bEnable)
    {
        FX_ScreenStretchRect(CTexture::s_ptexCurrentSceneDiffuseAccMap, CTexture::s_ptexHDRTarget);

        RT_SetViewport(0, 0, CTexture::s_ptexSceneTarget->GetWidth(), CTexture::s_ptexSceneTarget->GetHeight());
    }
    else
    {
        FX_ResetPipe();
        gcpRendD3D->RT_SetViewport(0, 0, gcpRendD3D->GetWidth(), gcpRendD3D->GetHeight());
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
void CD3D9Renderer::FX_ProcessSkinRenderLists(int nList, void(* RenderFunc)(), bool bLighting)
{
    // Forward SSS completely disabled, except for the character editor where we just do a simple forward pass
    if (m_RP.m_PersFlags2 & RBPF2_ALLOW_DEFERREDSHADING)
    {
        return;
    }

    const int recursiveLevel = SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID];
    const bool bUseDeferredSkin = ((m_RP.m_nRendFlags & SHDF_ALLOWPOSTPROCESS) && recursiveLevel <= 0) && CV_r_DeferredShadingDebug != 2 && CV_r_measureoverdraw == 0;

    //if ((m_RP.m_nRendFlags & SHDF_ALLOWPOSTPROCESS) && nR <= 1) && CV_r_DeferredShadingDebug != 2)
    {
        uint32 nBatchMask = SRendItem::BatchFlags(nList, m_RP.m_pRLD);
        if (nBatchMask & FB_SKIN)
        {
#ifdef DO_RENDERLOG
            if (CV_r_log)
            {
                Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], "*** Begin skin pass ***\n");
            }
#endif

            {
                PROFILE_LABEL_SCOPE("SKIN_GEN_PASS");

                if (bUseDeferredSkin)
                {
                    m_RP.m_PersFlags2 |= RBPF2_SKIN;
                }

                FX_ProcessRenderList(nList, BEFORE_WATER, RenderFunc, bLighting);
                FX_ProcessRenderList(nList, AFTER_WATER , RenderFunc, bLighting);

                if (bUseDeferredSkin)
                {
                    m_RP.m_PersFlags2 &= ~RBPF2_SKIN;
                }
            }

            if (bUseDeferredSkin)
            {
                PROFILE_LABEL_SCOPE("SKIN_APPLY_PASS");

                FX_SkinRendering(true);

                FX_ProcessRenderList(nList, BEFORE_WATER, RenderFunc, bLighting);
                FX_ProcessRenderList(nList, AFTER_WATER , RenderFunc, bLighting);

                FX_SkinRendering(false);
            }

#ifdef DO_RENDERLOG
            if (CV_r_log)
            {
                Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], "*** End skin pass ***\n");
            }
#endif
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
void CD3D9Renderer::FX_ProcessEyeOverlayRenderLists(int nList, void(* RenderFunc)(), bool bLighting)
{
    int recursiveLevel = SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID];
    if ((m_RP.m_nRendFlags & SHDF_ALLOWPOSTPROCESS) && recursiveLevel <= 0)
    {
        int iTempX, iTempY, iWidth, iHeight;
        gcpRendD3D->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);

        PROFILE_LABEL_SCOPE("EYE_OVERLAY");

        SDepthTexture* pCurrDepthBuffer = (gRenDev->m_RP.m_MSAAData.Type) ? &gcpRendD3D->m_DepthBufferOrigMSAA : &gcpRendD3D->m_DepthBufferOrig;

        FX_PushRenderTarget(0, CTexture::s_ptexSceneDiffuse, pCurrDepthBuffer);

        FX_ProcessRenderList(nList, BEFORE_WATER, RenderFunc, bLighting);
        FX_ProcessRenderList(nList, AFTER_WATER , RenderFunc, bLighting);

        FX_PopRenderTarget(0);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
void CD3D9Renderer::FX_ProcessHalfResParticlesRenderList(int nList, void(* RenderFunc)(), bool bLighting)
{
    int recursiveLevel = SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID];
    if ((m_RP.m_nRendFlags & SHDF_ALLOWPOSTPROCESS) && recursiveLevel <= 0)
    {
        const int nums = m_RP.m_pRLD->m_nStartRI[1][nList];
        if (m_RP.m_pRLD->m_nEndRI[1][nList] - nums > 0)
        {
            const auto& ri = CRenderView::CurrentRenderView()->GetRenderItems(1, nList)[nums];
            const bool bAlphaBased = CV_r_ParticlesHalfResBlendMode == 0;

#ifdef DO_RENDERLOG
            if (CV_r_log)
            {
                Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], "*** Begin half res transparent pass ***\n");
            }
#endif

            CTexture* pHalfResTarget = CTexture::s_ptexHDRTargetScaled[CV_r_ParticlesHalfResAmount];
            assert(CTexture::IsTextureExist(pHalfResTarget));
            if (CTexture::IsTextureExist(pHalfResTarget))
            {
                const int nHalfWidth = pHalfResTarget->GetWidth();
                const int nHalfHeight = pHalfResTarget->GetHeight();

                PROFILE_LABEL_SCOPE("TRANSP_HALF_RES_PASS");

                // Get current viewport
                int iTempX, iTempY, iWidth, iHeight;
                GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);

                FX_PushRenderTarget(0, pHalfResTarget, NULL);
                FX_SetColorDontCareActions(0);
                FX_ClearTarget(pHalfResTarget, Clr_Empty);
                RT_SetViewport(0, 0, nHalfWidth, nHalfHeight);

                m_RP.m_PersFlags2 |= RBPF2_HALFRES_PARTICLES;
                const uint32 nOldForceStateAnd = m_RP.m_ForceStateAnd;
                const uint32 nOldForceStateOr = m_RP.m_ForceStateOr;
                m_RP.m_ForceStateOr = GS_NODEPTHTEST;
                if (bAlphaBased)
                {
                    m_RP.m_ForceStateAnd = GS_BLSRC_SRCALPHA;
                    m_RP.m_ForceStateOr |= GS_BLSRC_SRCALPHA_A_ZERO;
                }
                FX_ProcessRenderList(nList, AFTER_WATER, RenderFunc, bLighting);
                m_RP.m_ForceStateAnd = nOldForceStateAnd;
                m_RP.m_ForceStateOr = nOldForceStateOr;
                m_RP.m_PersFlags2 &= ~RBPF2_HALFRES_PARTICLES;

#if defined(CRY_USE_METAL)
                //In metal Clear calls are cached until a draw call is made. If nothing is rendered do a manual clear
                if (m_RP.m_RendNumVerts == 0)
                {
                    FX_Commit();
                    FX_ClearTargetRegion();
                }
#endif

                FX_PopRenderTarget(0);

                {
                    PROFILE_LABEL_SCOPE("UPSAMPLE_PASS");
                    CShader* pSH = CShaderMan::s_shPostEffects;
                    CTexture* pHalfResSrc = pHalfResTarget;
                    CTexture* pZTarget = CTexture::s_ptexZTarget;
                    CTexture* pZTargetScaled = CV_r_ParticlesHalfResAmount > 0 ? CTexture::s_ptexZTargetScaled2 : CTexture::s_ptexZTargetScaled;

                    uint32 nStates = GS_NODEPTHTEST | GS_COLMASK_RGB;
                    if (bAlphaBased)
                    {
                        nStates |= GS_BLSRC_ONE | GS_BLDST_SRCALPHA;
                    }
                    else
                    {
                        nStates |= GS_BLSRC_ONE | GS_BLDST_ONE;
                    }

                    RT_SetViewport(iTempX, iTempY, iWidth, iHeight);
                    static const CCryNameTSCRC pTechNameNearestDepth("NearestDepthUpsample");
                    PostProcessUtils().ShBeginPass(CShaderMan::s_shPostEffects, pTechNameNearestDepth, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

                    static CCryNameR pParam0Name("texToTexParams0");
                    Vec4 vParam0(pZTarget->GetWidth(), pZTarget->GetHeight(), pZTargetScaled->GetWidth(), pZTargetScaled->GetHeight());
                    CShaderMan::s_shPostEffects->FXSetPSFloat(pParam0Name, &vParam0, 1);

                    PostProcessUtils().SetTexture(pHalfResSrc, 1, FILTER_LINEAR);
                    PostProcessUtils().SetTexture(pZTarget, 2, FILTER_POINT);
                    PostProcessUtils().SetTexture(pZTargetScaled, 3, FILTER_POINT);

                    FX_SetState(nStates);
#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
                    PostProcessUtils().DrawFullScreenTri(GetWidth(), GetHeight());
#else
                    PostProcessUtils().DrawFullScreenTri(m_width, m_height);
#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED

                    PostProcessUtils().ShEndPass();
                }
            }

#ifdef DO_RENDERLOG
            if (CV_r_log)
            {
                Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], "*** End half res transparent pass ***\n");
            }
#endif
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Checks if we need to enable velocity pass.
bool CD3D9Renderer::IsVelocityPassEnabled() const
{
    bool takingScreenShot = (m_screenShotType != 0);
    bool bUseMotionVectors = (CV_r_MotionBlur || (FX_GetAntialiasingType() & eAT_TEMPORAL_MASK) != 0) && CV_r_MotionVectors && (!takingScreenShot || CV_r_MotionBlurScreenShot);
    return bUseMotionVectors && CV_r_MotionBlurGBufferVelocity;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Output g-buffer
bool CD3D9Renderer::FX_ZScene(bool bEnable, bool bClearZBuffer, bool bRenderNormalsOnly, bool bZPrePass)
{
    AZ_TRACE_METHOD();

    uint32 nDiffuseTargetID = 1;
    SThreadInfo* const pShaderThreadInfo = &(m_RP.m_TI[m_RP.m_nProcessThreadID]);

    if (bEnable)
    {
        pShaderThreadInfo->m_PersFlags |= RBPF_ZPASS;

        int nStates = GS_DEPTHWRITE;
        FX_SetState(nStates);

        const int nWidth = m_MainViewport.nWidth;
        const int nHeight = m_MainViewport.nHeight;
        if (bClearZBuffer)
        {
            const float clearDepth = (m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & RBPF_REVERSE_DEPTH) ? 0.f : 1.f;
            const uint clearStencil = 1;
            RECT rect = { 0, 0, nWidth, nHeight };

            // Stencil initialized to 1 - 0 is reserved for MSAAed samples
            FX_ClearTarget(&m_DepthBufferOrigMSAA, CLEAR_ZBUFFER | CLEAR_STENCIL, clearDepth, clearStencil, 1, &rect, true);
            m_nStencilMaskRef = 1;
        }

        m_RP.m_PersFlags2 |= RBPF2_NOALPHABLEND | (bZPrePass ? (RBPF2_ZPREPASS | RBPF2_DISABLECOLORWRITES) : RBPF2_NOALPHATEST);
        m_RP.m_StateAnd &= ~(GS_BLEND_MASK | GS_ALPHATEST_MASK);
        m_RP.m_StateAnd |= bZPrePass ? GS_ALPHATEST_MASK : 0;

        if (m_logFileHandle != AZ::IO::InvalidHandle)
        {
            Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], " +++ Start Z scene +++ \n");
        }

        // RTs resolves/restores occur in CD3D9Renderer::FX_GmemTransition(...)
        if (FX_GetEnabledGmemPath(nullptr))
        {
            if (IsVelocityPassEnabled() && FX_GetEnabledGmemPath(nullptr) == CD3D9Renderer::eGT_128bpp_PATH )
            {
                m_RP.m_PersFlags2 |= RBPF2_MOTIONBLURPASS;
            }
            return true;
        }

        if (!CTexture::s_ptexZTarget
            || CTexture::s_ptexZTarget->IsMSAAChanged()
            || CTexture::s_ptexZTarget->GetDstFormat() != CTexture::s_eTFZ
            || CTexture::s_ptexZTarget->GetWidth() != nWidth
            || CTexture::s_ptexZTarget->GetHeight() != nHeight)
        {
            FX_Commit(); // Flush to unset the Z target before regenerating
            CTexture::GenerateZMaps();
        }

        bool bClearRT = false;
        bClearRT |= CV_r_wireframe != 0;
        bClearRT |= !bRenderNormalsOnly;
        static ICVar* skyBoxCVar = gEnv->pConsole->GetCVar("e_SkyBox");
        bClearRT |= skyBoxCVar->GetIVal() == 0;
        bClearRT |= m_clearBackground;
        if (bClearRT)
        {
            EF_ClearTargetsLater(FRT_CLEAR_COLOR);
            // If we don't have a skybox do a clear of the scene normal map. When
            // we have a sky box every texel in the normal map is rendered to.
            // But when the sky box is disabled then it is possible that left over
            // data remains in the normal texture from previous passes and this
            // causes rendering artifacts. The check for bClearZBuffer is used to
            // make sure we clear the normal map only once and not twice per frame.
            if (bClearZBuffer)
            {
                FX_ClearTarget(CTexture::s_ptexSceneNormalsMap);
            }
        }
        FX_PushRenderTarget(0, CTexture::s_ptexSceneNormalsMap, &m_DepthBufferOrigMSAA, -1, true);

        // CONFETTI BEGIN: David Srour
        // Note that the GBUFFER cannot have don't care actions or
        // it'll break deferred decals & other similar passes.
        FX_SetColorDontCareActions(0, false, false);
        // CONFETTI END

#ifndef CRY_USE_METAL
        if (!bZPrePass)
#endif
        {
            FX_PushRenderTarget(nDiffuseTargetID, CTexture::s_ptexSceneDiffuse, NULL);

            CTexture* pSceneSpecular = CTexture::s_ptexSceneSpecular;
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DRENDPIPELINE_CPP_SECTION_4
    #include AZ_RESTRICTED_FILE(D3DRendPipeline_cpp)
#endif
            FX_PushRenderTarget(nDiffuseTargetID + 1, pSceneSpecular, NULL);

            
            FX_SetColorDontCareActions(nDiffuseTargetID, false, false);
            FX_SetColorDontCareActions(nDiffuseTargetID + 1, false, false);

            if (IsVelocityPassEnabled())
            {
                m_RP.m_PersFlags2 |= RBPF2_MOTIONBLURPASS;
                FX_PushRenderTarget(nDiffuseTargetID + 2, GetUtils().GetVelocityObjectRT(), NULL);
            }
        }

        RT_SetViewport(0, 0, nWidth, nHeight);

        FX_SetActiveRenderTargets();
    }
    else if (pShaderThreadInfo->m_PersFlags & RBPF_ZPASS)
    {
        pShaderThreadInfo->m_PersFlags &= ~RBPF_ZPASS;

        m_RP.m_PersFlags2 &= ~(RBPF2_NOALPHABLEND | RBPF2_NOALPHATEST | RBPF2_ZPREPASS | RBPF2_DISABLECOLORWRITES);
        m_RP.m_StateAnd |= (GS_BLEND_MASK | GS_ALPHATEST_MASK);

        if (m_logFileHandle != AZ::IO::InvalidHandle)
        {
            Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], " +++ End Z scene +++ \n");
        }

        // RTs resolves/restores occur in CD3D9Renderer::FX_GmemTransition(...)
        if (FX_GetEnabledGmemPath(nullptr))
        {
            return true;
        }

        FX_PopRenderTarget(0);

#ifndef CRY_USE_METAL
        if (!bZPrePass)
#endif
        {
            FX_PopRenderTarget(nDiffuseTargetID);
            FX_PopRenderTarget(nDiffuseTargetID + 1);
            if (m_RP.m_PersFlags2 & RBPF2_MOTIONBLURPASS)
            {
                FX_PopRenderTarget(nDiffuseTargetID + 2);
                m_RP.m_PersFlags2 &= ~RBPF2_MOTIONBLURPASS;
            }
        }
        if (bRenderNormalsOnly)
        {
            CTexture::s_ptexZTarget->Resolve();
        }
    }
    else
    {
        if (!CV_r_usezpass)
        {
            CTexture::DestroyZMaps();
        }
    }

    return true;
}

void CD3D9Renderer::FX_GmemTransition([[maybe_unused]] const EGmemTransitions transition)
{
#if defined(OPENGL_ES) || defined(CRY_USE_METAL)
    /* Resources used during the GMEM render paths:
     *
     * CTexture::s_ptexSceneNormalsMap              // 32 bits
     * CTexture::s_ptexSceneDiffuse                 // 32 bits
     * CTexture::s_ptexSceneSpecular                // 32 bits
     * CTexture::s_ptexGmemStenLinDepth             // 32 bits
     * CTexture::s_ptexCurrentSceneDiffuseAccMap    // 64 bits
     * CTexture::s_ptexSceneSpecularAccMap          // 64 bits
     */

    if (SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID] != 0)
    {
        return;
    }

    CTexture* gmemSceneTarget = CTexture::s_ptexSceneSpecularAccMap;

    int const currentGmemPath = FX_GetEnabledGmemPath(nullptr);
    assert(FX_GetEnabledGmemPath(nullptr));

    // COMMON FUNCTIONS ///////////////////////////////////////////////////////////////////////////
    auto UnbindGmemRts = [this](int const startRT, int const endRT)
        {
            assert(startRT  >= 0 &&
                endRT    >= 0 &&
                startRT  <= 5 &&
                endRT    <= 5 &&
                startRT  <= endRT);

            for (auto rt = startRT; rt <= endRT; rt++)
            {
                FX_PopRenderTarget(rt);
            }
        };

    auto BindGBufferRts = [this, gmemSceneTarget](int currentGmemPath, int* outVelocityRT, int* outDepthStencilRT, bool forceLoad = false)
    {
        /* Bind RTs
        *
        * 256bpp:
        * (0) Specular L-Buffer (used as scene-target during GMEM sections)
        * (1) Diffuse
        * (2) Spec
        * (3) Stencil / Linear Depth
        * (4) Diffuse L-Buffer
        * (5) Normals
        *
        * 128bpp:
        * (0) Normals
        * (1) Diffuse
        * (2) Spec
        * (3) Stencil / Linear Depth
        */
        const int invalidRT = -1;
        const int defaultDepthRT = 3;
        const int maxGmemRTCount = 6;

        int velocityBufferRT = invalidRT;
        int depthStencilRT = defaultDepthRT;
        AZStd::vector<bool> dontCareColorLoad;
        AZStd::vector<bool> dontCareColorSave;
        AZStd::vector<bool> dontCareDepthStencilLoad = { false, false };
        AZStd::vector<bool> dontCareDepthStencilSave = { false, false };

        dontCareColorLoad.reserve(maxGmemRTCount);
        dontCareColorSave.reserve(maxGmemRTCount);
        if (eGT_256bpp_PATH == currentGmemPath)
        {
            FX_PushRenderTarget(0, gmemSceneTarget, &m_DepthBufferOrigMSAA, -1, true);
            FX_PushRenderTarget(1, CTexture::s_ptexSceneDiffuse, NULL);
            FX_PushRenderTarget(2, CTexture::s_ptexSceneSpecular, NULL);
            FX_PushRenderTarget(3, CTexture::s_ptexGmemStenLinDepth, NULL);
            FX_PushRenderTarget(4, CTexture::s_ptexCurrentSceneDiffuseAccMap, NULL);
            FX_PushRenderTarget(5, CTexture::s_ptexSceneNormalsMap, NULL);

            dontCareColorLoad = { true,  true, true, true,  true, true };
            dontCareColorSave = { false, true, true, false, true, true };
        }
        else if (eGT_128bpp_PATH == currentGmemPath)
        {
            FX_PushRenderTarget(0, CTexture::s_ptexSceneNormalsMap, &m_DepthBufferOrigMSAA, -1, true);
            FX_PushRenderTarget(1, CTexture::s_ptexSceneDiffuse, NULL);
            FX_PushRenderTarget(2, CTexture::s_ptexSceneSpecular, NULL);

            dontCareColorLoad = { true,  true, true, true };
            dontCareColorSave = { false, false, false, false };

            if (IsVelocityPassEnabled())
            {
                if (RenderCapabilities::SupportsRenderTargets(s_gmemLargeRTCount))
                {
                    dontCareColorLoad.resize(5);
                    dontCareColorSave.resize(5);
                    depthStencilRT = 3;
                    velocityBufferRT = 4;
                }
                else
                {
                    depthStencilRT = -1;
                    velocityBufferRT = 3;
                }
            }

            if (velocityBufferRT != invalidRT)
            {
                FX_PushRenderTarget(velocityBufferRT, GetUtils().GetVelocityObjectRT(), NULL);
                dontCareColorLoad[velocityBufferRT] = true;
                dontCareColorSave[velocityBufferRT] = false;
            }

            if (depthStencilRT != invalidRT)
            {
                FX_PushRenderTarget(depthStencilRT, CTexture::s_ptexGmemStenLinDepth, NULL);
                dontCareColorLoad[depthStencilRT] = true;
                dontCareColorSave[depthStencilRT] = false;
            }
        }

        if (forceLoad)
        {
            std::fill(dontCareColorLoad.begin(), dontCareColorLoad.end(), false);
            std::fill(dontCareDepthStencilLoad.begin(), dontCareDepthStencilLoad.end(), false);
        }

        for (int i = 0; i < dontCareColorLoad.size(); ++i)
        {
            FX_SetColorDontCareActions(i, dontCareColorLoad[i], dontCareColorSave[i]);
        }

        FX_SetDepthDontCareActions(0, dontCareDepthStencilLoad[0], dontCareDepthStencilSave[0]);
        FX_SetStencilDontCareActions(0, dontCareDepthStencilLoad[1], dontCareDepthStencilSave[1]);
        
        if (outVelocityRT)
        {
            *outVelocityRT = velocityBufferRT;
        }

        if (outDepthStencilRT)
        {
            *outDepthStencilRT = depthStencilRT;
        }
    };

    auto ProcessPassesThatDontFitGMEM = [this](bool const linearizeDepth, bool const downsampleDepth, bool const deferredPasses)
        {
            if (linearizeDepth)
            {
                FX_LinearizeDepth(CTexture::s_ptexGmemStenLinDepth);
            }

            if (downsampleDepth)
            {
                GetUtils().DownsampleDepth(CTexture::s_ptexGmemStenLinDepth, CTexture::s_ptexZTargetScaled, true);
                GetUtils().DownsampleDepth(CTexture::s_ptexZTargetScaled, CTexture::s_ptexZTargetScaled2, true);
                static ICVar* checkOcclusion = gEnv->pConsole->GetCVar("e_CheckOcclusion");
                if(checkOcclusion->GetIVal())
                {
                    //Downsample to the occlusion buffer dimensions
                    GetUtils().DownsampleDepth(CTexture::s_ptexZTargetScaled2, m_occlusionData[m_occlusionBufferIndex].m_zTargetReadback, true);
                }
            }

            if (deferredPasses)
            {
                CDeferredShading::Instance().DirectionalOcclusionPass();
                CDeferredShading::Instance().ScreenSpaceReflectionPass();
            }
        };

    auto ResetGMEMDontCareActions = [this](int const endRT)
        {
            assert(endRT >= 0);

            for (auto rt = 0; rt <= endRT; rt++)
            {
                FX_SetColorDontCareActions(rt, false, false);
            }

            FX_SetDepthDontCareActions(0, false, false);
            FX_SetStencilDontCareActions(0, false, false);
        };

    ///////////////////////////////////////////////////////////////////////////////////////////////


    SThreadInfo* const pShaderThreadInfo = &(m_RP.m_TI[m_RP.m_nProcessThreadID]);

    switch (transition)
    {
    case eGT_PRE_Z:
    {
        // Setup deferred renderer's lights and shadows for GMEM path
        assert(CDeferredShading::IsValid());
        if (IsShadowPassEnabled())
        {
            CDeferredShading::Instance().SetupGmemPath();
        }

        RT_SetViewport(0, 0, m_MainViewport.nWidth, m_MainViewport.nHeight);
        int velocityRT, depthStencilRT;
        BindGBufferRts(currentGmemPath, &velocityRT, &depthStencilRT);

        // Clear depth stencil
        EF_ClearTargetsImmediately(FRT_CLEAR_DEPTH | FRT_CLEAR_STENCIL, 1.0f, 1);
        m_nStencilMaskRef = 1;

        // Custom clear GMEM G-Buffer if requested
        if (depthStencilRT >= 0)
        {
            if (CRenderer::CV_r_ClearGMEMGBuffer == 1)
            {
                PROFILE_LABEL_SCOPE("GMEM G-BUFFER CLEAR");
                FX_SetState(GS_NODEPTHTEST | GS_COLMASK_RGB | GS_BLSRC_ONE | GS_BLDST_ZERO);
                RT_SetViewport(0, 0, m_MainViewport.nWidth, m_MainViewport.nHeight);
                PostProcessUtils().ClearGmemGBuffer();
            }
            else if (CRenderer::CV_r_ClearGMEMGBuffer == 2)
            {
                //Linear depth is set to be cleared to 1.0f. x (linear depth) = 1, y(stencil id) = 0.
                FX_SetColorDontCareActions(depthStencilRT, false, false);
                FX_ClearTarget(CTexture::s_ptexGmemStenLinDepth, ColorF(1.000f, 0.000f, 0.000f));

                if (velocityRT > 0)
                {
                    // Clear out the velocity buffer to half2(1.0, 1.0)
                    FX_SetColorDontCareActions(velocityRT, false, false);
                    FX_ClearTarget(GetUtils().GetVelocityObjectRT(), Clr_White);
                }
            }
        }

        break;
    }
    case eGT_POST_GBUFFER:
    {
        if (FX_GmemGetDepthStencilMode() == eGDSM_Texture)
        {
            // Since we can't fetch the depth/stencil from the buffer we need to linearize it now.
            // The linearized depth is used by the deferred decal, snow and rain passes.
            // This will flush the Gbuffer out.
            int renderTargetsToUnbind = IsVelocityPassEnabled() && RenderCapabilities::SupportsRenderTargets(s_gmemLargeRTCount) ? 4 : 3;
            UnbindGmemRts(0, renderTargetsToUnbind);
            ProcessPassesThatDontFitGMEM(true, true, false);
            BindGBufferRts(currentGmemPath, nullptr, nullptr, true);
        }
        break;
    }
    case eGT_POST_Z_PRE_DEFERRED:
    {
        /* Resolve RTs for 128bpp path.
         *
         * Bind RTs
         * 128bpp:
         * (0) Specular L-Buffer (used as scene-target during GMEM sections)
         * (1) Diffuse L-Buffer
         */
        if (eGT_128bpp_PATH == currentGmemPath)
        {
            int renderTargetsToUnbind = IsVelocityPassEnabled() && RenderCapabilities::SupportsRenderTargets(s_gmemLargeRTCount) ? 4 : 3;
           
            ResetGMEMDontCareActions(renderTargetsToUnbind);
            UnbindGmemRts(0, renderTargetsToUnbind);

            EGmemDepthStencilMode depthStencilMode = FX_GmemGetDepthStencilMode();
            ProcessPassesThatDontFitGMEM(depthStencilMode == eGDSM_DepthStencilBuffer, depthStencilMode != eGDSM_Texture, true);

            // Bind RTs
            FX_PushRenderTarget(s_gmemRendertargetSlots[currentGmemPath][eGT_SpecularLight], gmemSceneTarget, &m_DepthBufferOrigMSAA, -1, true);
            FX_SetColorDontCareActions(s_gmemRendertargetSlots[currentGmemPath][eGT_SpecularLight], true, false);

            // Don't push more than 1 RT if using PLS extension
            if (!RenderCapabilities::SupportsPLSExtension())
            {
                FX_PushRenderTarget(s_gmemRendertargetSlots[currentGmemPath][eGT_DiffuseLight], CTexture::s_ptexCurrentSceneDiffuseAccMap, NULL);
                FX_SetColorDontCareActions(s_gmemRendertargetSlots[currentGmemPath][eGT_DiffuseLight], true, false);
            }
            else
            {
                FX_TogglePLS(true);
            }

            FX_SetDepthDontCareActions(0, false, false);
            FX_SetStencilDontCareActions(0, false, false);
        }
        break;
    }
    case eGT_POST_DEFERRED_PRE_FORWARD:
    {
        ResetGMEMDontCareActions(eGT_256bpp_PATH == currentGmemPath ? 5 : 1);

        // Unbind all but the scene target
        // Scene target already bound if using PLS... just need to toggle PLS off
        if (RenderCapabilities::SupportsPLSExtension())
        {
            FX_TogglePLS(false);
        }
        else
        {
            UnbindGmemRts(1, eGT_256bpp_PATH == currentGmemPath ? 5 : 1);
        }

        if (eGT_256bpp_PATH == currentGmemPath)
        {
            ProcessPassesThatDontFitGMEM(false, true, false);
        }
        break;
    }
    case eGT_POST_AW_TRANS_PRE_POSTFX:
    {
        // Unbind scene target
        UnbindGmemRts(0, 0);

        // TODO: Behavior for HDR/PostFX passes
        break;
    }
    default:
        CRY_ASSERT(0);
        break;
    }

    FX_SetActiveRenderTargets();
#endif
}

CD3D9Renderer::EGmemPath CD3D9Renderer::FX_GetEnabledGmemPath(CD3D9Renderer::EGmemPathState* const gmemPathStateOut) const
{
    // Using local statics since this check should only be done once per run-time
    static EGmemPath enabledPath = eGT_REGULAR_PATH;
    static EGmemPathState gmemState = eGT_OK;

#if defined(OPENGL_ES) || defined(CRY_USE_METAL)
    static bool enabledGmemPathAlreadyChecked = false;

    if (!enabledGmemPathAlreadyChecked)
    {
        switch (CRenderer::CV_r_EnableGMEMPath)
        {
        case eGT_REGULAR_PATH:
            break;
        case eGT_256bpp_PATH:
        {
            // Does device support this path?
            if (!RenderCapabilities::Supports256bppGmemPath())
            {
                gmemState = eGT_DEV_UNSUPPORTED;

                // Check if device supports 128bpp path instead
                if (RenderCapabilities::Supports128bppGmemPath())
                {
                    enabledPath = eGT_128bpp_PATH;
                }
            }
            // Check for unsupported rendering features on this path otherwise
            else if (CRenderer::CV_r_ssdo || CRenderer::CV_r_SSReflections ||
                     CRenderer::CV_r_MotionBlur > 0 || (FX_GetAntialiasingType() & eAT_TEMPORAL_MASK) != 0)
            {
                // Force 128bpp path
                gmemState = eGT_FEATURES_UNSUPPORTED;
                enabledPath = eGT_128bpp_PATH;
            }
            else
            {
                enabledPath = eGT_256bpp_PATH;
            }
            break;
        }
        case eGT_128bpp_PATH:
        {
            // Does device support this path?
            if (!RenderCapabilities::Supports128bppGmemPath())
            {
                gmemState = eGT_DEV_UNSUPPORTED;
            }
            else
            {
                enabledPath = eGT_128bpp_PATH;
            }
            break;
        }
        default:
            CRY_ASSERT(0);
            break;
        }

        enabledGmemPathAlreadyChecked = true;
    }
#endif

    if (gmemPathStateOut)
    {
        *gmemPathStateOut = gmemState;
    }

    return enabledPath;
}

CD3D9Renderer::EGmemDepthStencilMode CD3D9Renderer::FX_GmemGetDepthStencilMode() const
{
    if (m_gmemDepthStencilMode == eGDSM_Invalid)
    {
        switch (FX_GetEnabledGmemPath(nullptr))
        {
        case eGT_256bpp_PATH:
            m_gmemDepthStencilMode = eGDSM_RenderTarget;
            break;
        case eGT_128bpp_PATH:
        {
            bool hasEnoughRTs = true;
#if defined(OPENGL_ES) || defined(CRY_USE_METAL)
            hasEnoughRTs &= RenderCapabilities::SupportsRenderTargets(s_gmemLargeRTCount);
#endif
            if (IsVelocityPassEnabled() && !hasEnoughRTs)
            {
                m_gmemDepthStencilMode = RenderCapabilities::GetFrameBufferFetchCapabilities().test(RenderCapabilities::FBF_DEPTH) ? eGDSM_DepthStencilBuffer : eGDSM_Texture;
            }
            else
            {
                m_gmemDepthStencilMode = eGDSM_RenderTarget;
            }
            break;
        }
        default:
            m_gmemDepthStencilMode = eGDSM_Texture;
            break;
        }
    }

    return m_gmemDepthStencilMode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
void CD3D9Renderer::FX_RenderForwardOpaque(void(* RenderFunc)(), const bool bLighting, [[maybe_unused]] const bool bAllowDeferred)
{
    if (FX_GetEnabledGmemPath(nullptr))
    {
#ifdef SUPPORTS_MSAA
        // Not supported in GMEM path
        CRY_ASSERT(0);
#endif
    }

    // Note: MSAA for deferred lighting requires extra pass using per-sample frequency for tagged undersampled regions
    //  for future: this could be avoided (while maintaining current architecture), by using MRT output then a composite step
    int recursiveLevel = SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID];
    SThreadInfo* const pShaderThreadInfo = &(m_RP.m_TI[m_RP.m_nProcessThreadID]);

    if (!FX_GetEnabledGmemPath(nullptr)) // Can't reclear buffers during GMEM path
    {
        if IsCVarConstAccess(constexpr) (CV_r_measureoverdraw == 4)
        {
            SetClearColor(Vec3_Zero);
            EF_ClearTargetsLater(FRT_CLEAR_COLOR, Clr_Empty);
        }
    }

    PROFILE_LABEL_SCOPE("OPAQUE_PASSES");

    m_RP.m_depthWriteStateUsed = false;

    const bool bShadowGenSpritePasses = (pShaderThreadInfo->m_PersFlags & (RBPF_SHADOWGEN)) != 0;

    if ((m_RP.m_PersFlags2 & RBPF2_ALLOW_DEFERREDSHADING) && !bShadowGenSpritePasses && recursiveLevel == 0 && !m_wireframe_mode)
    {
        m_RP.m_PersFlags2 |= RBPF2_FORWARD_SHADING_PASS;
    }

    if (!FX_GetEnabledGmemPath(nullptr))
    {
        // This unbinds/binds new RTs which isn't supported in GMEM path.
        // TODO: rework FX_ProcessEyeOverlayRenderLists() for GMEM
        if (!bShadowGenSpritePasses)
        {
            // Note: Eye overlay writes to diffuse color buffer for eye shader reading
            PROFILE_PS_TIME_SCOPE(fTimeDIPs[EFSLIST_EYE_OVERLAY]);
            FX_ProcessEyeOverlayRenderLists(EFSLIST_EYE_OVERLAY, RenderFunc, bLighting);
        }
    }

    {
        PROFILE_LABEL_SCOPE("GENERAL");
        PROFILE_PS_TIME_SCOPE_COND(fTimeDIPs[EFSLIST_GENERAL], !bShadowGenSpritePasses);

        GetTiledShading().BindForwardShadingResources(NULL);

        FX_ProcessRenderList(EFSLIST_GENERAL, BEFORE_WATER, RenderFunc, bLighting);
        FX_ProcessRenderList(EFSLIST_GENERAL, AFTER_WATER , RenderFunc, bLighting);

        GetTiledShading().UnbindForwardShadingResources();
    }

    {
        PROFILE_LABEL_SCOPE("FORWARD_DECALS");
        PROFILE_PS_TIME_SCOPE_COND(fTimeDIPs[EFSLIST_DECAL], !bShadowGenSpritePasses);

        FX_ProcessRenderList(EFSLIST_DECAL, BEFORE_WATER, RenderFunc, bLighting);
        FX_ProcessRenderList(EFSLIST_DECAL, AFTER_WATER , RenderFunc, bLighting);
    }

    {
        PROFILE_LABEL_SCOPE("DEFERRED_EMISSIVE_DECALS");
        FX_DeferredDecalsEmissive();
    }

    if (!FX_GetEnabledGmemPath(nullptr)) // not supported in GMEM path as it resolves buffers
    {
        if (!bShadowGenSpritePasses)
        {
            // Note: Do not swap render order with decals it breaks light acc buffer.
            //  -   PC could actually work via accumulation into light acc target
            {
                PROFILE_PS_TIME_SCOPE(fTimeDIPs[EFSLIST_SKIN]);
                FX_ProcessSkinRenderLists(EFSLIST_SKIN, RenderFunc, bLighting);
            }
        }
    }

    if (CRenderer::CV_r_FurFinPass != 0)
    {
        FurPasses::GetInstance().ExecuteFinPass();
    }

    if (m_RP.m_depthWriteStateUsed && !FX_GetEnabledGmemPath(nullptr))
    {
        // If any forward opaque passes wrote depth, recapture linear depth for upcoming passes such as fog
        FX_LinearizeDepth(CTexture::s_ptexZTarget);
    }

    m_RP.m_PersFlags2 &= ~RBPF2_FORWARD_SHADING_PASS;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CD3D9Renderer::FX_RenderFog()
{
    PROFILE_PS_TIME_SCOPE(fTimeDIPsDeferredLayers);

    FX_ResetPipe();
    FX_FogScene();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

inline static float expf_s(float arg)
{
    return expf(clamp_tpl(arg, -80.0f, 80.0f));
}

inline static float MaxChannel(const Vec4& col)
{
    return max(max(col.x, col.y), col.z);
}

bool CD3D9Renderer::FX_FogScene()
{
    if (m_logFileHandle != AZ::IO::InvalidHandle)
    {
        Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], " +++ Fog scene +++ \n");
    }
    m_RP.m_PersFlags2 &= ~(RBPF2_NOSHADERFOG);

    FX_SetVStream(3, NULL, 0, 0);

    SThreadInfo* const pShaderThreadInfo = &(m_RP.m_TI[m_RP.m_nProcessThreadID]);

    if (pShaderThreadInfo->m_FS.m_bEnable && CV_r_usezpass)
    {
        PROFILE_SHADER_SCOPE;
        PROFILE_LABEL_SCOPE("FOG_GLOBAL");

        int x = 0, y = 0, width = GetWidth(), height = GetHeight();

        m_pNewTarget[0]->m_ClearFlags = 0;
        RT_SetViewport(x, y, width, height);

        CShader* pSH = CShaderMan::s_shHDRPostProcess;

        float modelMatrix[16];
        float projMatrix[16];
        int viewport[4] = { x, y, width, height };
        GetModelViewMatrix(modelMatrix);
        GetProjectionMatrix(projMatrix);

        Vec3 vFarPlaneVerts[4];
        const float fFar = (pShaderThreadInfo->m_PersFlags & RBPF_REVERSE_DEPTH) ? 0.0f : 1.0f;
        UnProject(width, height, fFar, &vFarPlaneVerts[0].x, &vFarPlaneVerts[0].y, &vFarPlaneVerts[0].z, modelMatrix, projMatrix, viewport);
        UnProject(0, height, fFar, &vFarPlaneVerts[1].x, &vFarPlaneVerts[1].y, &vFarPlaneVerts[1].z, modelMatrix, projMatrix, viewport);
        UnProject(0, 0, fFar, &vFarPlaneVerts[2].x, &vFarPlaneVerts[2].y, &vFarPlaneVerts[2].z, modelMatrix, projMatrix, viewport);
        UnProject(width, 0, fFar, &vFarPlaneVerts[3].x, &vFarPlaneVerts[3].y, &vFarPlaneVerts[3].z, modelMatrix, projMatrix, viewport);

        const float camZFar = GetCamera().GetFarPlane();
        const Vec3 camPos = GetCamera().GetPosition();
        const Vec3 camDir = GetCamera().GetViewdir();

        const Vec3 vRT = vFarPlaneVerts[0] - camPos;
        const Vec3 vLT = vFarPlaneVerts[1] - camPos;
        const Vec3 vLB = vFarPlaneVerts[2] - camPos;
        const Vec3 vRB = vFarPlaneVerts[3] - camPos;

        const uint64 nFlagsShaderRTSave = m_RP.m_FlagsShader_RT;

        //////////////////////////////////////////////////////////////////////////

#if defined(VOLUMETRIC_FOG_SHADOWS)
        const bool renderFogShadow = m_bVolFogShadowsEnabled && (CV_r_VolumetricFog == 0);

        Vec4 volFogShadowRange;
        {
            Vec3 volFogShadowRangeP;
            gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_VOLFOG_SHADOW_RANGE, volFogShadowRangeP);
            volFogShadowRangeP.x = clamp_tpl(volFogShadowRangeP.x, 0.01f, 1.0f);
            volFogShadowRange = Vec4(volFogShadowRangeP.x, 1.0f / volFogShadowRangeP.x, 0, 0);
        }

        if (renderFogShadow)
        {
            // Recreate render targets if quality was changed
            bool halfRes = (CV_r_FogShadows == 1), quarterRes = (CV_r_FogShadows == 2);
            if ((halfRes && CTexture::s_ptexVolFogShadowBuf[0]->GetWidth() != GetWidth() / 2) ||
                (quarterRes && CTexture::s_ptexVolFogShadowBuf[0]->GetWidth() != GetWidth() / 4))
            {
                uint32 width2 = GetWidth() / (halfRes ? 2 : 4);
                uint32 height2 = GetHeight() / (halfRes ? 2 : 4);
                for (uint32 i = 0; i < 2; ++i)
                {
                    ETEX_Format fmt = CTexture::s_ptexVolFogShadowBuf[i]->GetDstFormat();
                    CTexture::s_ptexVolFogShadowBuf[i]->Invalidate(width2, height2, fmt);
                    CTexture::s_ptexVolFogShadowBuf[i]->CreateRenderTarget(fmt, Clr_Transparent);
                }
            }


            int oldWidth, oldHeight;
            {
                int dummy0, dummy1;
                GetViewport(&dummy0, &dummy1, &oldWidth, &oldHeight);
            }

            TempDynVB<SVF_P3F_T3F> vb(gcpRendD3D);
            vb.Allocate(4);
            SVF_P3F_T3F* pQuad = vb.Lock();

            pQuad[0].p = Vec3(-1, -1, 0);
            pQuad[0].st = vLB;

            pQuad[1].p = Vec3(1, -1, 0);
            pQuad[1].st = vRB;

            pQuad[2].p = Vec3(-1, 1, 0);
            pQuad[2].st = vLT;

            pQuad[3].p = Vec3(1, 1, 0);
            pQuad[3].st = vRT;

            vb.Unlock();
            vb.Bind(0);
            vb.Release();

            //////////////////////////////////////////////////////////////////////////
            // interleave pass
            {
                FX_SetupShadowsForFog();

                FX_PushRenderTarget(0, CTexture::s_ptexVolFogShadowBuf[0], 0);
                RT_SetViewport(0, 0, CTexture::s_ptexVolFogShadowBuf[0]->GetWidth(), CTexture::s_ptexVolFogShadowBuf[0]->GetHeight());

                const bool renderFogCloudShadow = m_bVolFogCloudShadowsEnabled;
                m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[HWSR_SAMPLE5];
                if (renderFogCloudShadow)
                {
                    m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE5];
                }

                static CCryNameTSCRC TechName0("FogPassVolShadowsInterleavePass");
                static CCryNameTSCRC TechName1("MultiGSMShadowedFog");
                pSH->FXSetTechnique(CRenderer::CV_r_FogShadowsMode == 1 ? TechName1 : TechName0);

                uint32 nPasses;
                pSH->FXBegin(&nPasses, FEF_DONTSETSTATES);
                pSH->FXBeginPass(0);

                static CCryNameR volFogShadowRangeN("volFogShadowRange");
                pSH->FXSetPSFloat(volFogShadowRangeN, &volFogShadowRange, 1);

                FX_Commit();

                const uint32 nRS = GS_NODEPTHTEST;
                FX_SetState(nRS);
                D3DSetCull(eCULL_None);

                if (!FAILED(FX_SetVertexDeclaration(0, eVF_P3F_T3F)))
                {
                    FX_DrawPrimitive(eptTriangleStrip, 0, 4);
                }

                pSH->FXEndPass();

                FX_PopRenderTarget(0);
                m_RP.m_FlagsShader_RT = nFlagsShaderRTSave;
            }

            //////////////////////////////////////////////////////////////////////////
            // gather pass
            {
                static CCryNameTSCRC TechName("FogPassVolShadowsGatherPass");
                static CCryNameR volFogShadowBufSampleOffsetsN("volFogShadowBufSampleOffsets");
                static const int texStatePoint = CTexture::GetTexState(STexState(FILTER_POINT, true));

                Vec4 sampleOffsets[8];

                // horizontal
                {
                    FX_PushRenderTarget(0, CTexture::s_ptexVolFogShadowBuf[1], 0);
                    RT_SetViewport(0, 0, CTexture::s_ptexVolFogShadowBuf[1]->GetWidth(), CTexture::s_ptexVolFogShadowBuf[1]->GetHeight());

                    pSH->FXSetTechnique(TechName);

                    uint32 nPasses;
                    pSH->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
                    pSH->FXBeginPass(0);

                    CTexture::s_ptexVolFogShadowBuf[0]->Apply(0, texStatePoint, EFTT_UNKNOWN, -1, SResourceView::DefaultView);

                    const float tU = 1.0f / (float)CTexture::s_ptexVolFogShadowBuf[0]->GetWidth();
                    for (int x2 = -4, index = 0; x2 < 4; ++x2, ++index)
                    {
                        sampleOffsets[index] = Vec4(x2 * tU, 0, 0, 1);
                    }

                    pSH->FXSetPSFloat(volFogShadowBufSampleOffsetsN, sampleOffsets, 8);

                    FX_Commit();

                    const uint32 nRS = GS_NODEPTHTEST;
                    FX_SetState(nRS);
                    D3DSetCull(eCULL_None);

                    if (!FAILED(FX_SetVertexDeclaration(0, eVF_P3F_T3F)))
                    {
                        FX_DrawPrimitive(eptTriangleStrip, 0, 4);
                    }

                    pSH->FXEndPass();

                    FX_PopRenderTarget(0);
                }

                // vertical
                {
                    FX_PushRenderTarget(0, CTexture::s_ptexVolFogShadowBuf[0], 0);
                    RT_SetViewport(0, 0, CTexture::s_ptexVolFogShadowBuf[0]->GetWidth(), CTexture::s_ptexVolFogShadowBuf[0]->GetHeight());

                    uint32 nPasses;
                    pSH->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
                    pSH->FXBeginPass(0);

                    CTexture::s_ptexVolFogShadowBuf[1]->Apply(0, texStatePoint, EFTT_UNKNOWN, -1, SResourceView::DefaultView);

                    const float tV = 1.0f / (float)CTexture::s_ptexVolFogShadowBuf[1]->GetHeight();
                    for (int y2 = -4, index = 0; y2 < 4; ++y2, ++index)
                    {
                        sampleOffsets[index] = Vec4(0, y2 * tV, 0, 1);
                    }

                    pSH->FXSetPSFloat(volFogShadowBufSampleOffsetsN, sampleOffsets, 8);

                    FX_Commit();

                    if (!FAILED(FX_SetVertexDeclaration(0, eVF_P3F_T3F)))
                    {
                        FX_DrawPrimitive(eptTriangleStrip, 0, 4);
                    }

                    pSH->FXEndPass();

                    FX_PopRenderTarget(0);
                }
            }

            RT_SetViewport(0, 0, oldWidth, oldHeight);
        }
#endif // #if defined(VOLUMETRIC_FOG_SHADOWS)

        //////////////////////////////////////////////////////////////////////////

        if (m_RP.m_PersFlags2 & RBPF2_HDR_FP16)
        {
            m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_HDR_MODE];
        }

        float fogDepth = 0.0f;
        if (CV_r_FogDepthTest != 0.0f && CV_r_VolumetricFog == 0)
        {
            if (CV_r_FogDepthTest < 0.0f)
            {
                Vec4 fogColGradColBase(0, 0, 0, 0), fogColGradColDelta(0, 0, 0, 0);
                CHWShader_D3D::GetFogColorGradientConstants(fogColGradColBase, fogColGradColDelta);

                const Vec4 fogColGradRadial = CHWShader_D3D::GetFogColorGradientRadial();

                const float fogColorIntensityBase = MaxChannel(fogColGradColBase);
                const float fogColorIntensityTop = MaxChannel(fogColGradColBase + fogColGradColDelta);
                const float fogColorIntensityRadial = MaxChannel(fogColGradRadial);
                const float fogColorIntensity = max(fogColorIntensityBase, fogColorIntensityTop) + fogColorIntensityRadial;

                const float threshold = -CV_r_FogDepthTest;

                const Vec4 volFogParams = CHWShader_D3D::GetVolumetricFogParams();
                const Vec4 volFogRampParams = CHWShader_D3D::GetVolumetricFogRampParams();

                const float atmosphereScale = volFogParams.x;
                const float volFogHeightDensityAtViewer = volFogParams.y;
                const float finalClamp = 1.0f - volFogParams.w;

                Vec3 lookDir = vRT;
                if (lookDir.z * atmosphereScale < vLT.z * atmosphereScale)
                {
                    lookDir = vLT;
                }
                if (lookDir.z * atmosphereScale < vLB.z * atmosphereScale)
                {
                    lookDir = vLB;
                }
                if (lookDir.z * atmosphereScale < vRB.z * atmosphereScale)
                {
                    lookDir = vRB;
                }

                lookDir.Normalize();
                const float viewDirAdj = lookDir.Dot(camDir);

                float depth = camZFar * 0.5f;
                float step = depth * 0.5f;
                uint32 numSteps = 16;

                while (numSteps)
                {
                    Vec3 cameraToWorldPos = lookDir * depth;

                    float fogInt = 1.0f;

                    const float t = atmosphereScale * cameraToWorldPos.z;
                    const float slopeThreshold = 0.01f;
                    if (fabsf(t) > slopeThreshold)
                    {
                        fogInt *= (expf_s(t) - 1.0f) / t;
                    }

                    const float l = depth; // length(cameraToWorldPos);
                    const float u = l * volFogHeightDensityAtViewer;
                    fogInt = fogInt * u;

                    float f = clamp_tpl(expf_s(0.69314719f * -fogInt), 0.0f, 1.0f);

                    float r = clamp_tpl(l * volFogRampParams.x + volFogRampParams.y, 0.0f, 1.0f);
                    r = r * (2.0f - r);
                    r = r * volFogRampParams.z + volFogRampParams.w;

                    f = (1.0f - f) * r;
                    assert(f >= 0.0f && f <= 1.0f);

                    f = min(f, finalClamp);
                    f *= fogColorIntensity;

                    if (f > threshold)
                    {
                        depth -= step;
                    }
                    else
                    {
                        fogDepth = depth * viewDirAdj;
                        depth += step;
                    }
                    step *= 0.5f;

                    --numSteps;
                }
            }
            else
            {
                fogDepth = CV_r_FogDepthTest;
            }
        }

        m_fogCullDistance = fogDepth;

        int nSUnitZTarget = -2; // FogPassPS doesn't need a sampler for ZTarget.

        const bool useFogDepthTest = fogDepth >= 0.01f;
        uint32 nFlags = FEF_DONTSETTEXTURES | FEF_DONTSETSTATES;

#if defined(VOLUMETRIC_FOG_SHADOWS)
        m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[HWSR_SAMPLE0];
        if (renderFogShadow)
        {
            m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];
        }
#endif

        if (CV_r_VolumetricFog != 0)
        {
            m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_VOLUMETRIC_FOG];
            nFlags &= ~FEF_DONTSETTEXTURES;
        }

        static CCryNameTSCRC TechName("FogPass");
        pSH->FXSetTechnique(TechName);

        uint32 nPasses;
        pSH->FXBegin(&nPasses, nFlags);
        pSH->FXBeginPass(0);

        STexState TexStatePoint = STexState(FILTER_POINT, true);

        CTexture* depthRT = CTexture::s_ptexZTarget;

        if (FX_GetEnabledGmemPath(nullptr))
        {
            depthRT = CTexture::s_ptexGmemStenLinDepth;
        }

        depthRT->Apply(0, CTexture::GetTexState(TexStatePoint), EFTT_UNKNOWN, nSUnitZTarget, m_RP.m_MSAAData.Type ? SResourceView::DefaultViewMS : SResourceView::DefaultView); // bind as msaa target (if valid)

#if defined(VOLUMETRIC_FOG_SHADOWS)
        if (renderFogShadow)
        {
            static int texStatePoint = CTexture::GetTexState(STexState(FILTER_POINT, true));
            CTexture::s_ptexVolFogShadowBuf[0]->Apply(2, texStatePoint, EFTT_UNKNOWN, -1, SResourceView::DefaultView);
        }
#endif

#if defined(FEATURE_SVO_GI)
        // bind SVO atmosphere
        {
            static CCryNameR sSVO_AirTextureScale("SVO_AirTextureScale");
            Vec4 vSVO_AirTextureScale(0, 0, 0, 0);
            pSH->FXSetPSFloat(sSVO_AirTextureScale, &vSVO_AirTextureScale, 1);
        }
#endif

        TempDynVB<SVF_P3F_T3F> vb(gcpRendD3D);
        vb.Allocate(4);
        SVF_P3F_T3F* Verts = vb.Lock();

        const Matrix44A& projMat = pShaderThreadInfo->m_matProj;
        float clipZ = 0;
        if (useFogDepthTest)
        {
            // projMat.m23 is -1 or 1 depending on whether we use a RH or LH coord system
            // done in favor of an if check to make homogeneous divide by fogDepth (which is always positive) work
            clipZ = projMat.m23 * fogDepth * projMat.m22 + projMat.m32;
            clipZ /= fogDepth;
            clipZ = clamp_tpl(clipZ, 0.f, 1.f);
        }

        Verts[0].p = Vec3(-1, -1, clipZ);
        Verts[0].st = vLB;

        Verts[1].p = Vec3(1, -1, clipZ);
        Verts[1].st = vRB;

        Verts[2].p = Vec3(-1, 1, clipZ);
        Verts[2].st = vLT;

        Verts[3].p = Vec3(1, 1, clipZ);
        Verts[3].st = vRT;

        vb.Unlock();
        vb.Bind(0);
        vb.Release();

#if defined(VOLUMETRIC_FOG_SHADOWS)
        if (renderFogShadow)
        {
            Vec3 volFogShadowDarkeningP;
            gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_VOLFOG_SHADOW_DARKENING, volFogShadowDarkeningP);

            Vec4 volFogShadowDarkening(volFogShadowDarkeningP, 0);
            static CCryNameR volFogShadowDarkeningN("volFogShadowDarkening");
            pSH->FXSetPSFloat(volFogShadowDarkeningN, &volFogShadowDarkening, 1);

            const float aSun = (1.0f - clamp_tpl(volFogShadowDarkeningP.y, 0.0f, 1.0f)) * 1.0f;
            const float bSun = 1.0f - aSun;
            const float aAmb = (1.0f - clamp_tpl(volFogShadowDarkeningP.z, 0.0f, 1.0f)) * 0.4f;
            const float bAmb = 1.0f - aAmb;

            Vec4 volFogShadowDarkeningSunAmb(aSun, bSun, aAmb, bAmb);
            static CCryNameR volFogShadowDarkeningSunAmbN("volFogShadowDarkeningSunAmb");
            pSH->FXSetPSFloat(volFogShadowDarkeningSunAmbN, &volFogShadowDarkeningSunAmb, 1);

            static CCryNameR volFogShadowRangeN("volFogShadowRange");
            pSH->FXSetPSFloat(volFogShadowRangeN, &volFogShadowRange, 1);

            Vec4 sampleOffsets[5];
            {
                const float tU = 1.0f / (float)CTexture::s_ptexVolFogShadowBuf[0]->GetWidth();
                const float tV = 1.0f / (float)CTexture::s_ptexVolFogShadowBuf[0]->GetHeight();

                sampleOffsets[0] = Vec4(0, 0, 0, 0);
                sampleOffsets[1] = Vec4(0, -tV, 0, 0);
                sampleOffsets[2] = Vec4(-tU, 0, 0, 0);
                sampleOffsets[3] = Vec4(tU, 0, 0, 0);
                sampleOffsets[4] = Vec4(0, tU, 0, 0);
            }
            static CCryNameR volFogShadowBufSampleOffsetsN("volFogShadowBufSampleOffsets");
            pSH->FXSetPSFloat(volFogShadowBufSampleOffsetsN, sampleOffsets, 5);
        }
#endif

        FX_Commit();

        // using GS_BLDST_SRCALPHA because GS_BLDST_ONEMINUSSRCALPHA causes banding artifact when alpha value is very low.
        uint32 nRS = GS_BLSRC_ONE | GS_BLDST_SRCALPHA | (useFogDepthTest ? GS_DEPTHFUNC_LEQUAL : GS_NODEPTHTEST);

        // Draw a fullscreen quad to sample the RT
        FX_SetState(nRS);
        D3DSetCull(eCULL_None);

        if (!FAILED(FX_SetVertexDeclaration(0, eVF_P3F_T3F)))
        {
            FX_DrawPrimitive(eptTriangleStrip, 0, 4);
        }

        pSH->FXEndPass();

        //////////////////////////////////////////////////////////////////////////

        Vec3 lCol;
        gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_SKY_HIGHLIGHT_COLOR, lCol);

        bool useFogPassWithLightning(lCol.x > 1e-4f || lCol.y > 1e-4f || lCol.z > 1e-4f);
        if (useFogPassWithLightning)
        {
            static CCryNameTSCRC TechNameAlt("FogPassWithLightning");
            if (pSH->FXSetTechnique(TechNameAlt))
            {
                pSH->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
                pSH->FXBeginPass(0);

                Vec3 lPos;
                gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_SKY_HIGHLIGHT_POS, lPos);
                Vec4 lightningPosition(lPos.x, lPos.y, lPos.z, 0.0f);
                static CCryNameR Param1Name("LightningPos");
                pSH->FXSetPSFloat(Param1Name, &lightningPosition, 1);

                Vec3 lSize;
                gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_SKY_HIGHLIGHT_SIZE, lSize);
                Vec4 lightningColorSize(lCol.x, lCol.y, lCol.z, lSize.x * 0.01f);
                static CCryNameR Param2Name("LightningColSize");
                pSH->FXSetPSFloat(Param2Name, &lightningColorSize, 1);

                FX_Commit();

                FX_SetState(GS_NODEPTHTEST | GS_BLSRC_ONE | GS_BLDST_ONE);

                if (!FAILED(FX_SetVertexDeclaration(0, eVF_P3F_T3F)))
                {
                    FX_DrawPrimitive(eptTriangleStrip, 0, 4);
                }

                pSH->FXEndPass();
            }
        }

        //////////////////////////////////////////////////////////////////////////

        m_RP.m_FlagsShader_RT = nFlagsShaderRTSave;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void SnapVector(Vec3& vVector, const float fSnapRange)
{
    Vec3 vSnapped = vVector / fSnapRange;
    vSnapped.Set(floor_tpl(vSnapped.x), floor_tpl(vSnapped.y), floor_tpl(vSnapped.z));
    vSnapped *= fSnapRange;
    vVector = vSnapped;
}

void CD3D9Renderer::FX_WaterVolumesCausticsPreprocess(N3DEngineCommon::SCausticInfo& causticInfo)
{
    PROFILE_LABEL_SCOPE("PREPROCESS");

    AZ_Assert((SRendItem::BatchFlags(EFSLIST_WATER, m_RP.m_pRLD) & FB_WATER_CAUSTIC) == 0, "Water volume found in the wrong render list");

    const uint32 waterRenderList = EFSLIST_REFRACTIVE_SURFACE;
    const int32 waterSortGroup = 0;

    const int recursiveLevel = SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID];
    SThreadInfo* const pShaderThreadInfo = &(m_RP.m_TI[m_RP.m_nProcessThreadID]);

    // Pre-process water ripples
    if (!recursiveLevel && (m_RP.m_nRendFlags & SHDF_ALLOWPOSTPROCESS))
    {
        FX_ResetPipe();
        CWaterRipples* pWaterRipples = (CWaterRipples*)PostEffectMgr()->GetEffect(ePFX_WaterRipples);
        CEffectParam* pParam = PostEffectMgr()->GetByName("WaterRipples_Amount");
        pParam->SetParam(1.0f);
        if (pWaterRipples->Preprocess()) // Preprocess here will clear the list and skip the one in FX_RenderWater.
        {
            m_RP.m_PersFlags2 |= RBPF2_WATERRIPPLES;
            gcpRendD3D->FX_ResetPipe();

            TransformationMatrices backupSceneMatrices;
            gcpRendD3D->Set2DMode(1, 1, backupSceneMatrices);

            pWaterRipples->Render();

            gcpRendD3D->Unset2DMode(backupSceneMatrices);

            gcpRendD3D->FX_ResetPipe();

            FX_Commit();
        }
    }

    PostProcessUtils().Log(" +++ Begin watervolume caustics preprocessing +++ \n");

    const float fMaxDistance = CRenderer::CV_r_watervolumecausticsmaxdistance;
    CCamera origCam = GetCamera();

    float fWidth = CTexture::s_ptexWaterCaustics[0]->GetWidth();
    float fHeight = CTexture::s_ptexWaterCaustics[0]->GetHeight();

    Vec3 vDir = gRenDev->GetViewParameters().ViewDir();
    Vec3 vPos = gRenDev->GetViewParameters().vOrigin;

    const float fOffsetDist = fMaxDistance * 0.25f;
    vPos += Vec3(vDir.x * fOffsetDist, vDir.y * fOffsetDist, 0.0f); // Offset in viewing direction to maximimze view distance.

    // Snap to avoid some aliasing.
    const float fSnapRange = CRenderer::CV_r_watervolumecausticssnapfactor;
    if (fSnapRange > 0.05f) // don't bother snapping if the value is low.
    {
        SnapVector(vPos, fSnapRange);
    }

    Vec3 vEye = vPos + Vec3(0.0f, 0.0f, 10.0f);

    // Create the matrices.
    Matrix44A mOrthoMatr;
    Matrix44A mViewMatr;
    mOrthoMatr.SetIdentity();
    mViewMatr.SetIdentity();
    mathMatrixOrtho(&mOrthoMatr, fMaxDistance, fMaxDistance, 0.25f, 100.0f);
    mathMatrixLookAt(&mViewMatr, vEye, vPos, Vec3(0, 1, 0));

    // Push the matrices.
    Matrix44A origMatView = pShaderThreadInfo->m_matView;
    Matrix44A origMatProj = pShaderThreadInfo->m_matProj;

    Matrix44A* m = &pShaderThreadInfo->m_matProj;
    m->SetIdentity();
    *m = mOrthoMatr;

    m = &pShaderThreadInfo->m_matView;
    m->SetIdentity();
    *m = mViewMatr;

    // Store for projection onto the scene.
    causticInfo.m_mCausticMatr = mViewMatr * mOrthoMatr;
    causticInfo.m_mCausticMatr.Transpose();

    pShaderThreadInfo->m_PersFlags |= RBPF_DRAWTOTEXTURE;

    FX_ClearTarget(CTexture::s_ptexWaterCaustics[0], Clr_Transparent);
    FX_PushRenderTarget(0, CTexture::s_ptexWaterCaustics[0], 0);
    RT_SetViewport(0, 0, (int)fWidth, (int)fHeight);

    FX_PreRender(3);

    m_RP.m_pRenderFunc = FX_FlushShader_General;
    m_RP.m_nPassGroupID = waterRenderList;
    m_RP.m_nPassGroupDIP = waterRenderList;

    PROFILE_DIPS_START;

    m_RP.m_nSortGroupID = waterSortGroup;
    FX_ProcessBatchesList(m_RP.m_pRLD->m_nStartRI[m_RP.m_nSortGroupID][waterRenderList], m_RP.m_pRLD->m_nEndRI[m_RP.m_nSortGroupID][waterRenderList], FB_WATER_CAUSTIC);

    PROFILE_DIPS_END(waterRenderList);

    FX_PopRenderTarget(0);

    FX_PostRender();

    pShaderThreadInfo->m_matView = origMatView;
    pShaderThreadInfo->m_matProj = origMatProj;

    FX_ResetPipe();
    RT_SetViewport(0, 0, gcpRendD3D->GetWidth(), gcpRendD3D->GetHeight());

    pShaderThreadInfo->m_PersFlags &= ~RBPF_DRAWTOTEXTURE;

    FX_Commit();

    PostProcessUtils().Log(" +++ End watervolume caustics preprocessing +++ \n");
}

bool CD3D9Renderer::FX_WaterVolumesCausticsUpdateGrid(N3DEngineCommon::SCausticInfo& causticInfo)
{
    // 16 bit index limit, can only do max 256x256 grid.
    // could use hardware tessellation to reduce memory and increase tessellation amount for higher precision.
    const uint32 nCausticMeshWidth = clamp_tpl(CRenderer::CV_r_watervolumecausticsdensity, 16, 255);
    const uint32 nCausticMeshHeight = clamp_tpl(CRenderer::CV_r_watervolumecausticsdensity, 16, 255);

    // Update the grid mesh if required.
    if ((!causticInfo.m_pCausticQuadMesh || causticInfo.m_nCausticMeshWidth != nCausticMeshWidth || causticInfo.m_nCausticMeshHeight != nCausticMeshHeight))
    {
        // Make sure we aren't recreating the mesh.
        causticInfo.m_pCausticQuadMesh = NULL;

        const uint32 nCausticVertexCount = (nCausticMeshWidth + 1) * (nCausticMeshHeight + 1);
        const uint32 nCausticIndexCount = (nCausticMeshWidth * nCausticMeshHeight) * 6;

        // Store the new resolution and vertex/index counts.
        causticInfo.m_nCausticMeshWidth = nCausticMeshWidth;
        causticInfo.m_nCausticMeshHeight = nCausticMeshHeight;
        causticInfo.m_nVertexCount = nCausticVertexCount;
        causticInfo.m_nIndexCount = nCausticIndexCount;

        // Reciprocal for scaling.
        float fRecipW = 1.0f / (float)nCausticMeshWidth;
        float fRecipH = 1.0f / (float)nCausticMeshHeight;

        // Buffers.
        SVF_P3F_C4B_T2F* pCausticQuads = new SVF_P3F_C4B_T2F[nCausticVertexCount];
        vtx_idx* pCausticIndices = new vtx_idx[nCausticIndexCount];

        // Fill vertex buffer.
        for (uint32 y = 0; y <= nCausticMeshHeight; ++y)
        {
            for (uint32 x = 0; x <= nCausticMeshWidth; ++x)
            {
                pCausticQuads[y * (nCausticMeshWidth + 1) + x].xyz = Vec3(((float)x) * fRecipW, ((float)y) * fRecipH, 0.0f);
            }
        }

        // Fill index buffer.
        for (uint32 y = 0; y < nCausticMeshHeight; ++y)
        {
            for (uint32 x = 0; x < nCausticMeshWidth; ++x)
            {
                pCausticIndices[(y * (nCausticMeshWidth) + x) * 6] = y * (nCausticMeshWidth + 1) + x;
                pCausticIndices[(y * (nCausticMeshWidth) + x) * 6 + 1] = y * (nCausticMeshWidth + 1) + x + 1;
                pCausticIndices[(y * (nCausticMeshWidth) + x) * 6 + 2] = (y + 1) * (nCausticMeshWidth + 1) + x + 1;
                pCausticIndices[(y * (nCausticMeshWidth) + x) * 6 + 3] = (y + 1) * (nCausticMeshWidth + 1) + x + 1;
                pCausticIndices[(y * (nCausticMeshWidth) + x) * 6 + 4] = (y + 1) * (nCausticMeshWidth + 1) + x;
                pCausticIndices[(y * (nCausticMeshWidth) + x) * 6 + 5] = y * (nCausticMeshWidth + 1) + x;
            }
        }

        // Create the mesh.
        causticInfo.m_pCausticQuadMesh = gRenDev->CreateRenderMeshInitialized(pCausticQuads, nCausticVertexCount, eVF_P3F_C4B_T2F, pCausticIndices, nCausticIndexCount, prtTriangleList, "WaterCausticMesh", "WaterCausticMesh");

        // Delete the temporary buffers.
        delete[] pCausticQuads;
        delete[] pCausticIndices;
    }

    // If we created the mesh, return true.
    if (causticInfo.m_pCausticQuadMesh)
    {
        return true;
    }

    return false;
}

void CD3D9Renderer::FX_WaterVolumesCaustics()
{
    uint64 nPrevFlagsShaderRT = gRenDev->m_RP.m_FlagsShader_RT;

    const uint32 waterRenderList = EFSLIST_REFRACTIVE_SURFACE;
    const int32 waterSortGroup = 0;

    uint32 nBatchMask = SRendItem::BatchFlags(waterRenderList, m_RP.m_pRLD);

    bool isEmpty = SRendItem::IsListEmpty(waterRenderList, m_RP.m_nProcessThreadID, m_RP.m_pRLD) && SRendItem::IsListEmpty(EFSLIST_WATER_VOLUMES, m_RP.m_nProcessThreadID, m_RP.m_pRLD);

    // Check if there are any water volumes that have caustics enabled
    if (!isEmpty)
    {
        auto& RESTRICT_REFERENCE RI = CRenderView::CurrentRenderView()->GetRenderItems(waterSortGroup, waterRenderList);

        int endRI = m_RP.m_pRLD->m_nEndRI[waterSortGroup][waterRenderList];
        int curRI = m_RP.m_pRLD->m_nStartRI[waterSortGroup][waterRenderList];

        isEmpty = true;

        while (curRI < endRI)
        {
            IRenderElement* pRE = RI[curRI++].pElem;
            if (pRE->mfGetType() == eDATA_WaterVolume && ((CREWaterVolume*)pRE)->m_pParams && ((CREWaterVolume*)pRE)->m_pParams->m_caustics == true)
            {
                isEmpty = false;
                break;
            }
        }
    }

    // Pre-process refraction
    // @NOTE: CV_r_watercaustics will be removed when the infinite ocean component feature toggle is removed.
    if (!isEmpty && (nBatchMask & FB_WATER_CAUSTIC) && CTexture::IsTextureExist(CTexture::s_ptexWaterCaustics[0]) && CTexture::IsTextureExist(CTexture::s_ptexWaterCaustics[1])
        && CRenderer::CV_r_watercaustics && CRenderer::CV_r_watervolumecaustics)
    {
        PROFILE_LABEL_SCOPE("WATERVOLUME_CAUSTICS");

        // Caustics info.
        N3DEngineCommon::SCausticInfo& causticInfo = gcpRendD3D->m_p3DEngineCommon.m_CausticInfo;

        float fWidth = CTexture::s_ptexWaterCaustics[0]->GetWidth();
        float fHeight = CTexture::s_ptexWaterCaustics[0]->GetHeight();

        // Preprocess (render all visible volumes to caustic gbuffer)
        FX_WaterVolumesCausticsPreprocess(causticInfo);

        gRenDev->m_cEF.mfRefreshSystemShader("DeferredCaustics", CShaderMan::s_ShaderDeferredCaustics);

        // Dilate the gbuffer.
        static CCryNameTSCRC pTechNameDilate("WaterCausticsInfoDilate");

        {
            PROFILE_LABEL_SCOPE("DILATION");

            PostProcessUtils().Log(" +++ Begin watervolume caustics dilation +++ \n");
        }

        FX_Commit();
        gcpRendD3D->FX_SetActiveRenderTargets(false);
        FX_PushRenderTarget(0, CTexture::s_ptexWaterCaustics[1], NULL);
        RT_SetViewport(0, 0, CTexture::s_ptexWaterCaustics[1]->GetWidth(), CTexture::s_ptexWaterCaustics[1]->GetHeight());

        TransformationMatrices backupSceneMatrices;
        Set2DMode(1, 1, backupSceneMatrices);

        PostProcessUtils().ShBeginPass(CShaderMan::s_ShaderDeferredCaustics, pTechNameDilate, FEF_DONTSETSTATES | FEF_DONTSETTEXTURES);
        FX_SetState(GS_NODEPTHTEST);

        PostProcessUtils().SetTexture(CTexture::s_ptexWaterCaustics[0], 0);
        PostProcessUtils().DrawFullScreenTri(CTexture::s_ptexWaterCaustics[1]->GetWidth(), CTexture::s_ptexWaterCaustics[1]->GetHeight());
        PostProcessUtils().ShEndPass();
        FX_PopRenderTarget(0);

        PostProcessUtils().Log(" +++ End watervolume caustics dilation +++ \n");


        // Super blur for alpha to mask edges of volumes.
        PostProcessUtils().TexBlurGaussian(CTexture::s_ptexWaterCaustics[1], 1, 1.0, 10.0, true, 0, false, CTexture::s_ptexWaterCaustics[0]);

        // Get current viewport
        int iTempX, iTempY, iWidth, iHeight;
        GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);

        ////////////////////////////////////////////////
        // Procedural caustic generation

        // Generate the caustics map using the grid mesh.
        // for future:
        // - merge this somehow with shadow gen for correct projection/intersection with geometry (and lighting) - can use shadow map for position reconstruction of world around volume and project caustic geometry to it.
        // - try hardware tessellation to increase quality and reduce memory (perhaps do projection per volume instead of as a single pass, that way it's basically screen-space)
        if (FX_WaterVolumesCausticsUpdateGrid(causticInfo)) // returns true if the mesh is valid
        {
            static CCryNameTSCRC pTechNameCaustics("WaterCausticsGen");
            PROFILE_LABEL_SCOPE("CAUSTICS_GEN");
            PostProcessUtils().Log(" +++ Begin watervolume caustics generation +++ \n");

            FX_PushRenderTarget(0, CTexture::s_ptexWaterCaustics[0], NULL);
            FX_SetActiveRenderTargets(false);// Avoiding invalid d3d error (due to deferred rt setup, when ping-pong'ing between RTs we can bump into RTs still bound when binding it as a SRV)
            RT_SetViewport(0, 0, CTexture::s_ptexWaterCaustics[0]->GetWidth(), CTexture::s_ptexWaterCaustics[0]->GetHeight());

            PostProcessUtils().ShBeginPass(CShaderMan::s_ShaderDeferredCaustics, pTechNameCaustics, FEF_DONTSETSTATES | FEF_DONTSETTEXTURES);
            FX_SetState(GS_NODEPTHTEST | GS_NOCOLMASK_R | GS_NOCOLMASK_G | GS_NOCOLMASK_A);

            // Set vertex textures.
            CTexture::s_ptexWaterCaustics[1]->SetVertexTexture(true);
            PostProcessUtils().SetTexture(CTexture::s_ptexWaterCaustics[1], 0, FILTER_TRILINEAR);

            FX_Commit();
            // Render the grid mesh.
            if (!FAILED(gcpRendD3D->FX_SetVertexDeclaration(0, eVF_P3F_C4B_T2F)))
            {
                size_t voffset(0);
                size_t ioffset(0);
                CRenderMesh* pCausticQuadMesh = static_cast<CRenderMesh*>(causticInfo.m_pCausticQuadMesh.get());
                pCausticQuadMesh->CheckUpdate(0);
                D3DBuffer* pVB = gcpRendD3D->m_DevBufMan.GetD3D(pCausticQuadMesh->GetVBStream(VSF_GENERAL), &voffset);
                D3DBuffer* pIB = gcpRendD3D->m_DevBufMan.GetD3D(pCausticQuadMesh->GetIBStream(), &ioffset);
                FX_SetVStream(0, pVB, voffset, pCausticQuadMesh->GetStreamStride(VSF_GENERAL));
                FX_SetIStream(pIB, ioffset, (sizeof(vtx_idx) == 2 ? Index16 : Index32));

                FX_DrawIndexedPrimitive(eptTriangleList, 0, 0, causticInfo.m_nVertexCount, 0, causticInfo.m_nIndexCount);
            }

            PostProcessUtils().ShEndPass();

            // Unset vertex textures.
            CTexture::s_ptexWaterCaustics[1]->SetVertexTexture(false);


            FX_PopRenderTarget(0);
            RT_SetViewport(0, 0, iWidth, iHeight);

            RT_UnbindTMUs();// Avoid d3d error due to rtv (s_ptexWaterCaustics[0]) still bound as shader input.

            // Smooth out any inconsistencies in the caustic map (pixels, etc).
            PostProcessUtils().TexBlurGaussian(CTexture::s_ptexWaterCaustics[0], 1, 1.0, 1.0, false, NULL, false, CTexture::s_ptexWaterCaustics[1]);

            PostProcessUtils().Log(" +++ End watervolume caustics generation +++ \n");

            FX_DeferredWaterVolumeCaustics(causticInfo);
        }

        Unset2DMode(backupSceneMatrices);
    }

    gRenDev->m_RP.m_FlagsShader_RT = nPrevFlagsShaderRT;
}

void CD3D9Renderer::FX_WaterVolumesPreprocess()
{
    AZ_Assert((SRendItem::BatchFlags(EFSLIST_WATER, m_RP.m_pRLD) & FB_WATER_REFL) == 0, "Water volume found in the wrong render list");

    const uint32 waterRenderList = EFSLIST_REFRACTIVE_SURFACE;
    const int32 waterSortGroup = 0;

    uint32 nBatchMask = SRendItem::BatchFlags(waterRenderList, m_RP.m_pRLD);
    if ((nBatchMask & FB_WATER_REFL) && CTexture::IsTextureExist(CTexture::s_ptexWaterVolumeRefl[0]))
    {
        PROFILE_LABEL_SCOPE("WATER_PREPROCESS");
#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
        const uint32 nCurrWaterVolID = gRenDev->GetCameraFrameID() % 2;
#else
        const uint32 nCurrWaterVolID = gRenDev->GetFrameID(false) % 2;
#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED

        CTexture* pCurrWaterVolRefl = CTexture::s_ptexWaterVolumeRefl[nCurrWaterVolID];

        PostProcessUtils().Log(" +++ Begin water volumes preprocessing +++ \n");

        bool bRgbkSrc = false;

        int nWidth = int(pCurrWaterVolRefl->GetWidth() * m_RP.m_CurDownscaleFactor.x);
        int nHeight = int(pCurrWaterVolRefl->GetHeight() * m_RP.m_CurDownscaleFactor.y);

        PostProcessUtils().StretchRect(CTexture::s_ptexCurrSceneTarget, CTexture::s_ptexHDRTargetPrev, false, bRgbkSrc, false, false, SPostEffectsUtils::eDepthDownsample_None, false, &gcpRendD3D->m_FullResRect);

        RECT rect = { 0, pCurrWaterVolRefl->GetHeight() - nHeight, nWidth, nHeight };
        FX_ClearTarget(pCurrWaterVolRefl, Clr_Transparent, 1, &rect, true);
        FX_PushRenderTarget(0, pCurrWaterVolRefl, 0);
        RT_SetViewport(0, pCurrWaterVolRefl->GetHeight() - nHeight, nWidth, nHeight);

        FX_PreRender(3);

        m_RP.m_pRenderFunc = FX_FlushShader_General;
        m_RP.m_nPassGroupID = waterRenderList;
        m_RP.m_nPassGroupDIP = waterRenderList;

        PROFILE_DIPS_START;

        m_RP.m_nSortGroupID = waterSortGroup;
        FX_ProcessBatchesList(m_RP.m_pRLD->m_nStartRI[m_RP.m_nSortGroupID][waterRenderList], m_RP.m_pRLD->m_nEndRI[m_RP.m_nSortGroupID][waterRenderList], FB_WATER_REFL);

        PROFILE_DIPS_END(waterRenderList);

        FX_PostRender();

        FX_PopRenderTarget(0);

        pCurrWaterVolRefl->GenerateMipMaps();

        FX_ResetPipe();

        RT_SetViewport(0, 0, gcpRendD3D->GetWidth(), gcpRendD3D->GetHeight());

        PostProcessUtils().Log(" +++ End water volumes preprocessing +++ \n");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CD3D9Renderer::FX_RenderWater(void(* RenderFunc)())
{
    PROFILE_LABEL_SCOPE("WATER");

    SThreadInfo* const pShaderThreadInfo = &(m_RP.m_TI[m_RP.m_nProcessThreadID]);

    PROFILE_PS_TIME_SCOPE_COND(fTimeDIPs[EFSLIST_WATER_VOLUMES], !(pShaderThreadInfo->m_PersFlags & (RBPF_SHADOWGEN)));
    const int recursiveLevel = SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID];

    if (!recursiveLevel)
    {
        // Pre-process refraction
        const bool isEmpty = SRendItem::IsListEmpty(EFSLIST_REFRACTIVE_SURFACE, m_RP.m_nProcessThreadID, m_RP.m_pRLD) && SRendItem::IsListEmpty(EFSLIST_WATER, m_RP.m_nProcessThreadID, m_RP.m_pRLD) && SRendItem::IsListEmpty(EFSLIST_WATER_VOLUMES, m_RP.m_nProcessThreadID, m_RP.m_pRLD);
        if (!isEmpty && CTexture::IsTextureExist(CTexture::s_ptexCurrSceneTarget))
        {
            if IsCVarConstAccess(constexpr) (!CRenderer::CV_r_debugrefraction)
            {
                FX_ScreenStretchRect(CTexture::s_ptexCurrSceneTarget);
            }
            else
            {
#if defined(CRY_USE_METAL)
                // On metal we have to submit a draw call in order for a clear to take effect.
                // Doing the commit/clear target region will produce the needed draw call for the clear.
                FX_PushRenderTarget(0, CTexture::s_ptexCurrSceneTarget, nullptr);
                FX_SetColorDontCareActions(0);
                CTexture::s_ptexCurrSceneTarget->Clear(ColorF(1, 0, 0, 1));
                RT_SetViewport(0, 0, CTexture::s_ptexCurrSceneTarget->GetWidth(), CTexture::s_ptexCurrSceneTarget->GetHeight());
                FX_Commit();
                FX_ClearTargetRegion();
                FX_PopRenderTarget(0);
#else
                CTexture::s_ptexCurrSceneTarget->Clear(ColorF(1, 0, 0, 1));
#endif
            }
        }

        // Pre-process rain ripples
        if (!isEmpty && (m_RP.m_nRendFlags & SHDF_ALLOWPOSTPROCESS))
        {
            FX_ResetPipe();
            CWaterRipples* pWaterRipples = (CWaterRipples*)PostEffectMgr()->GetEffect(ePFX_WaterRipples);
            CEffectParam* pParam = PostEffectMgr()->GetByName("WaterRipples_Amount");
            pParam->SetParam(1.0f);
            if (pWaterRipples->Preprocess())
            {
                m_RP.m_PersFlags2 |= RBPF2_WATERRIPPLES;
                gcpRendD3D->FX_ResetPipe();

                TransformationMatrices backupSceneMatrices;

                gcpRendD3D->Set2DMode(1, 1, backupSceneMatrices);

                pWaterRipples->Render();

                gcpRendD3D->Unset2DMode(backupSceneMatrices);

                gcpRendD3D->FX_ResetPipe();

                FX_Commit();
            }
        }
    }

    FX_WaterVolumesPreprocess();

    FX_ProcessRenderList(EFSLIST_WATER, BEFORE_WATER, RenderFunc, false);
    
    // We render opaque refractive surface, before the after water objects.
    // Because they are in EFSLIST_REFRACTIVE_SURFACE, they don't have sorting issues 
    // with EFSLIST_WATER objects.
    {
        PROFILE_LABEL_SCOPE("REFRACTIVE_SURFACE");
        PROFILE_PS_TIME_SCOPE_COND(fTimeDIPs[EFSLIST_REFRACTIVE_SURFACE], !(pShaderThreadInfo->m_PersFlags & (RBPF_SHADOWGEN)));
        FX_ProcessRenderList(EFSLIST_REFRACTIVE_SURFACE, BEFORE_WATER, RenderFunc, false);
    } 
    
    FX_ProcessRenderList(EFSLIST_WATER, AFTER_WATER , RenderFunc, false);

    m_RP.m_PersFlags2 &= ~(RBPF2_WATERRIPPLES | RBPF2_RAINRIPPLES);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CD3D9Renderer::FX_LinearizeDepth(CTexture* ptexZ)
{
    {
        PROFILE_LABEL_SCOPE("LINEARIZE_DEPTH");

        bool isRenderingFur = FurPasses::GetInstance().IsRenderingFur();

#ifdef SUPPORTS_MSAA
        if (FX_GetMSAAMode())
        {
            FX_MSAASampleFreqStencilSetup(MSAA_SAMPLEFREQ_PASS);
        }
#endif
        SDepthTexture* depthBuffer = nullptr;
        if (FX_GetEnabledGmemPath(nullptr))
        {
            switch (FX_GmemGetDepthStencilMode())
            {
            case eGDSM_RenderTarget:
                AZ_Assert(false, "Depth is already linearized in the render target");
                return;
            case eGDSM_DepthStencilBuffer:
                depthBuffer = &m_DepthBufferOrigMSAA;
                break;
            default:
                break;
            }
        }

        FX_PushRenderTarget(0, ptexZ, depthBuffer);

        static const CCryNameTSCRC pTechName("LinearizeDepth");
        PostProcessUtils().ShBeginPass(CShaderMan::s_shPostEffects, pTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

        FX_SetState(GS_NODEPTHTEST);

        m_DevMan.BindSRV(eHWSC_Pixel, &m_pZBufferDepthReadOnlySRV, 15, 1);

        RECT rect;
        rect.left = rect.top = 0;
        rect.right = LONG(ptexZ->GetWidth() * m_RP.m_CurDownscaleFactor.x);
        rect.bottom = LONG(ptexZ->GetHeight() * m_RP.m_CurDownscaleFactor.y);

        PostProcessUtils().DrawFullScreenTri(ptexZ->GetWidth(), ptexZ->GetHeight(), 0, &rect);

        D3DShaderResourceView* pNullSRV[1] = { NULL };
        m_DevMan.BindSRV(eHWSC_Pixel, pNullSRV, 15, 1);

        PostProcessUtils().ShEndPass();

        FX_PopRenderTarget(0);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
// Depth Fixup is  two-stage pass technique to fix the linear depth buffer to include
// additional depth that was not register in the opaque pass, i.e., transparency objects.
// This pass will initialize the buffer to default set value to be compared when finalizing.
void CD3D9Renderer::FX_DepthFixupPrepare()
{
    PROFILE_LABEL_SCOPE("PREPARE_DEPTH_FIXUP");

    // Merge linear depth with depth values written for transparent objects
    FX_PushRenderTarget(0, CTexture::s_ptexHDRTarget, NULL);

    // CONFETTI BEGIN: David Srour
    // Metal Load/Store Actions
    FX_SetDepthDontCareActions(0, false, true);
    FX_SetStencilDontCareActions(0, false, true);
    // CONFETTI END

    RT_SetViewport(0, 0, CTexture::s_ptexHDRTarget->GetWidth(), CTexture::s_ptexHDRTarget->GetHeight());
    static const CCryNameTSCRC pTechName("TranspDepthFixupPrepare");
    PostProcessUtils().ShBeginPass(CShaderMan::s_shPostEffects, pTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
    FX_SetState(GS_NODEPTHTEST | GS_BLSRC_ZERO | GS_BLDST_ONE | GS_BLALPHA_MAX);
    PostProcessUtils().DrawFullScreenTri(CTexture::s_ptexHDRTarget->GetWidth(), CTexture::s_ptexHDRTarget->GetHeight());
    PostProcessUtils().ShEndPass();
    FX_PopRenderTarget(0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
// Depth Fixup is  two-stage pass technique to fix the linear depth buffer to include
// additional depth that was not register in the opaque pass, i.e., transparency objects.
// This method will run the pass that will write the pixels with non default value.
void CD3D9Renderer::FX_DepthFixupMerge()
{
    PROFILE_LABEL_SCOPE("MERGE_DEPTH");

    // Merge linear depth with depth values written for transparent objects
    FX_PushRenderTarget(0, CTexture::s_ptexZTarget, NULL);

    // CONFETTI BEGIN: David Srour
    // Metal Load/Store Actions
    FX_SetDepthDontCareActions(0, false, true);
    FX_SetStencilDontCareActions(0, false, true);
    // CONFETTI END

    RT_SetViewport(0, 0, CTexture::s_ptexZTarget->GetWidth(), CTexture::s_ptexZTarget->GetHeight());
    static const CCryNameTSCRC pTechName("TranspDepthFixupMerge");
    PostProcessUtils().ShBeginPass(CShaderMan::s_shPostEffects, pTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
    PostProcessUtils().SetTexture(CTexture::s_ptexHDRTarget, 0, FILTER_POINT);
    FX_SetState(GS_NODEPTHTEST | GS_BLSRC_ONE | GS_BLDST_ONE | GS_BLOP_MIN);
    PostProcessUtils().DrawFullScreenTri(CTexture::s_ptexZTarget->GetWidth(), CTexture::s_ptexZTarget->GetHeight());
    PostProcessUtils().ShEndPass();
    FX_PopRenderTarget(0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CD3D9Renderer::FX_SRGBConversion()
{
    PROFILE_LABEL_SCOPE("SRGB CONVERSION");
    CRY_ASSERT(gcpRendD3D->FX_GetCurrentRenderTarget(0) == CTexture::s_ptexHDRTarget);
    gcpRendD3D->FX_PopRenderTarget(0);
    CTexture* targetText = GetUtils().AcquireFinalCompositeTarget(false);
    gcpRendD3D->RT_SetViewport(0, 0, targetText->GetWidth(), targetText->GetHeight());
    gcpRendD3D->FX_PushRenderTarget(0, targetText, NULL);
    GetUtils().CopyTextureToScreen(CTexture::s_ptexHDRTarget, nullptr, -1, true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

bool CD3D9Renderer::FX_HDRScene(bool bEnableHDR, bool bClear)
{
    SThreadInfo* const pShaderThreadInfo = &(m_RP.m_TI[m_RP.m_nProcessThreadID]);

    if (bEnableHDR)
    {
        if (m_logFileHandle != AZ::IO::InvalidHandle)
        {
            Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], " +++ Start HDR scene +++ \n");
        }

        if (!CTexture::s_ptexHDRTarget || CTexture::s_ptexHDRTarget->IsMSAAChanged() || CTexture::s_ptexHDRTarget->GetWidth() != GetWidth() || CTexture::s_ptexHDRTarget->GetHeight() != GetHeight())
        {
            CTexture::GenerateHDRMaps();
        }

        bool bEmpty = SRendItem::IsListEmpty(EFSLIST_HDRPOSTPROCESS, m_RP.m_nProcessThreadID, m_RP.m_pRLD);
        if (bEmpty)
        {
            return false;
        }

        if (!FX_GetEnabledGmemPath(nullptr)) // GMEM buffers are already bound
        {
            if (bClear || (pShaderThreadInfo->m_PersFlags & RBPF_MIRRORCULL) || (m_RP.m_nRendFlags & SHDF_CUBEMAPGEN))
            {
                FX_ClearTarget(CTexture::s_ptexHDRTarget);
                FX_ClearTarget(&m_DepthBufferOrigMSAA);
            }

            FX_PushRenderTarget(0, CTexture::s_ptexHDRTarget, &m_DepthBufferOrigMSAA, -1, true);
        }
        pShaderThreadInfo->m_PersFlags |= RBPF_HDR;

        if (m_logFileHandle != AZ::IO::InvalidHandle)
        {
            Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], " +++ End HDR scene +++ \n");
        }
    }
    return true;
}

// Draw overlay geometry in wireframe mode
void CD3D9Renderer::FX_DrawWire()
{
    float fColor = 1.f;
    int nState = GS_WIREFRAME;

    if IsCVarConstAccess(constexpr) (CV_r_showlines == 1)
    {
        nState |= GS_NODEPTHTEST;
    }

    if IsCVarConstAccess(constexpr) (CV_r_showlines == 3)
    {
        if (!gcpRendD3D->m_RP.m_pRE || !gcpRendD3D->m_RP.m_pRE->GetCustomData())
        {
            return; // draw only terrain
        }
        nState |= GS_BLSRC_DSTCOL | GS_BLDST_ONE;
        fColor = .25f;
    }

    gcpRendD3D->FX_SetState(nState);
    gcpRendD3D->SetMaterialColor(fColor, fColor, fColor, 1.f);
    CTextureManager::Instance()->GetWhiteTexture()->Apply(0);
    gcpRendD3D->EF_SetColorOp(eCO_MODULATE, eCO_MODULATE, (eCA_Texture | (eCA_Constant << 3)), (eCA_Texture | (eCA_Constant << 3)));
    gcpRendD3D->EF_SetSrgbWrite(false);
    CRenderObject* pObj = gcpRendD3D->m_RP.m_pCurObject;
    gcpRendD3D->FX_SetFPMode();
    gcpRendD3D->m_RP.m_pCurObject = pObj;

    uint32 i;
    if (gcpRendD3D->m_RP.m_pCurPass)
    {
        for (int nRE = 0; nRE <= gcpRendD3D->m_RP.m_nLastRE; nRE++)
        {
            gcpRendD3D->m_RP.m_pRE = gcpRendD3D->m_RP.m_RIs[nRE][0]->pElem;
            if (gcpRendD3D->m_RP.m_pRE)
            {
                EDataType t = gcpRendD3D->m_RP.m_pRE->mfGetType();
                if (t != eDATA_Mesh && t != eDATA_Terrain && t != eDATA_ClientPoly)
                {
                    continue;
                }
                gcpRendD3D->m_RP.m_pRE->mfPrepare(false);
                gcpRendD3D->m_RP.m_pRE->mfCheckUpdate(0, gcpRendD3D->m_RP.m_TI[gcpRendD3D->m_RP.m_nProcessThreadID].m_nFrameUpdateID);
            }

            CHWShader_D3D* curVS = (CHWShader_D3D*)gcpRendD3D->m_RP.m_pCurPass->m_VShader;
            for (i = 0; i < gcpRendD3D->m_RP.m_RIs[nRE].Num(); i++)
            {
                SRendItem* pRI = gcpRendD3D->m_RP.m_RIs[nRE][i];
                gcpRendD3D->FX_SetObjectTransform(pRI->pObj, NULL, pRI->pObj->m_ObjFlags);
                curVS->UpdatePerInstanceConstantBuffer();
                gcpRendD3D->FX_Commit();
                gcpRendD3D->FX_DrawRE(gcpRendD3D->m_RP.m_pShader, NULL);
            }
        }
    }
}

// Draw geometry normal vectors
void CD3D9Renderer::FX_DrawNormals()
{
    HRESULT h = S_OK;

    float len = CRenderer::CV_r_normalslength;
    int StrVrt, StrTan, StrNorm;

    for (int nRE = 0; nRE <= gcpRendD3D->m_RP.m_nLastRE; nRE++)
    {
        gcpRendD3D->m_RP.m_pRE = gcpRendD3D->m_RP.m_RIs[nRE][0]->pElem;
        if (gcpRendD3D->m_RP.m_pRE)
        {
            if (nRE)
            {
                gcpRendD3D->m_RP.m_pRE->mfPrepare(false);
            }
            gcpRendD3D->m_RP.m_pRE->mfCheckUpdate(-1, gcpRendD3D->m_RP.m_TI[gcpRendD3D->m_RP.m_nProcessThreadID].m_nFrameUpdateID);
        }

        const byte* verts = (const byte*)gcpRendD3D->EF_GetPointer(eSrcPointer_Vert, &StrVrt, eType_FLOAT, eSrcPointer_Vert, FGP_SRC | FGP_REAL);
        const byte* normals = (const byte*)gcpRendD3D->EF_GetPointer(eSrcPointer_Normal, &StrNorm, eType_FLOAT, eSrcPointer_Normal, FGP_SRC | FGP_REAL);
        const byte* tangents = (const byte*)gcpRendD3D->EF_GetPointer(eSrcPointer_Tangent, &StrTan, eType_FLOAT, eSrcPointer_Tangent, FGP_SRC | FGP_REAL);

        verts = ((INT_PTR)verts > 256 && StrVrt >= sizeof(Vec3)) ? verts : 0;
        normals = ((INT_PTR)normals > 256 && StrNorm >= sizeof(SPipNormal)) ? normals : 0;
        tangents = ((INT_PTR)tangents > 256 && (StrTan == sizeof(SPipQTangents) || StrTan == sizeof(SPipTangents))) ? tangents : 0;

        if (verts && (normals || tangents))
        {
            gcpRendD3D->FX_SetVertexDeclaration(0, eVF_P3F_C4B_T2F);
            gcpRendD3D->EF_SetColorOp(eCO_REPLACE, eCO_REPLACE, (eCA_Diffuse | (eCA_Diffuse << 3)), (eCA_Diffuse | (eCA_Diffuse << 3)));
            gcpRendD3D->EF_SetSrgbWrite(false);
            gcpRendD3D->FX_SetFPMode();
            CTextureManager::Instance()->GetWhiteTexture()->Apply(0);
            int nStateFlags = 0;
            if (gcpRendD3D->m_wireframe_mode == R_SOLID_MODE)
            {
                nStateFlags = GS_DEPTHWRITE;
            }
            if IsCVarConstAccess(constexpr) (CV_r_shownormals == 2)
            {
                nStateFlags = GS_NODEPTHTEST;
            }
            gcpRendD3D->FX_SetState(nStateFlags);
            gcpRendD3D->D3DSetCull(eCULL_None);

            //gcpRendD3D->GetDevice().SetVertexShader(NULL);

            // We must limit number of vertices, because TempDynVB (see code below)
            // uses transient pool that has *limited* size. See DevBuffer.cpp for details.
            // Note that one source vertex produces *two* buffer vertices (endpoints of
            // a normal vector).
            const size_t maxBufferSize = (size_t)NextPower2(gRenDev->CV_r_transient_pool_size) << 20;
            const size_t maxVertexCount = maxBufferSize / (2 * sizeof(SVF_P3F_C4B_T2F));
            const int numVerts = (int)(AZStd::GetMin)((size_t)gcpRendD3D->m_RP.m_RendNumVerts, maxVertexCount);

            TempDynVB<SVF_P3F_C4B_T2F> vb(gcpRendD3D);
            vb.Allocate(numVerts * 2);
            SVF_P3F_C4B_T2F* Verts = vb.Lock();

            uint32 col0 = 0x000000ff;
            uint32 col1 = 0x00ffffff;

            const bool bHasNormals = (normals != 0);

            for (int v = 0; v < numVerts; v++, verts += StrVrt, normals += StrNorm, tangents += StrTan)
            {
                const float* const fverts = (const float*)verts;

                Vec3 vNorm;
                if (bHasNormals)
                {
                    vNorm = ((const SPipNormal*)normals)->GetN();
                }
                else if (StrTan == sizeof(SPipQTangents))
                {
                    vNorm = ((const SPipQTangents*)tangents)->GetN();
                }
                else
                {
                    vNorm = ((const SPipTangents*)tangents)->GetN();
                }
                vNorm.Normalize();

                Verts[v * 2].xyz = Vec3(fverts[0], fverts[1], fverts[2]);
                Verts[v * 2].color.dcolor = col0;

                Verts[v * 2 + 1].xyz = Vec3(fverts[0] + vNorm[0] * len, fverts[1] + vNorm[1] * len, fverts[2] + vNorm[2] * len);
                Verts[v * 2 + 1].color.dcolor = col1;
            }

            vb.Unlock();
            vb.Bind(0);
            vb.Release();

            if (gcpRendD3D->m_RP.m_pCurPass)
            {
                CHWShader_D3D* curVS = (CHWShader_D3D*)gcpRendD3D->m_RP.m_pCurPass->m_VShader;
                for (uint32 i = 0; i < gcpRendD3D->m_RP.m_RIs[nRE].Num(); i++)
                {
                    SRendItem* pRI = gcpRendD3D->m_RP.m_RIs[nRE][i];
                    gcpRendD3D->FX_SetObjectTransform(pRI->pObj, NULL, pRI->pObj->m_ObjFlags);
                    curVS->UpdatePerInstanceConstantBuffer();
                    gcpRendD3D->FX_Commit();

                    gcpRendD3D->FX_DrawPrimitive(eptLineList, 0, numVerts * 2);
                }
            }

            gcpRendD3D->m_RP.m_VertexStreams[0].pStream = NULL;
        }
    }
}

// Draw geometry tangent vectors
void CD3D9Renderer::FX_DrawTangents()
{
    HRESULT h = S_OK;

    float len = CRenderer::CV_r_normalslength;

    for (int nRE = 0; nRE <= gcpRendD3D->m_RP.m_nLastRE; nRE++)
    {
        gcpRendD3D->m_RP.m_pRE = gcpRendD3D->m_RP.m_RIs[nRE][0]->pElem;
        if (gcpRendD3D->m_RP.m_pRE)
        {
            if (nRE)
            {
                gcpRendD3D->m_RP.m_pRE->mfPrepare(false);
            }
            gcpRendD3D->m_RP.m_pRE->mfCheckUpdate(-1, gcpRendD3D->m_RP.m_TI[gcpRendD3D->m_RP.m_nProcessThreadID].m_nFrameUpdateID);
        }

        int StrVrt, StrTan;
        const int flags = (CRenderer::CV_r_showtangents == 1)
            ? FGP_SRC | FGP_REAL
            : FGP_REAL;

        const byte* verts = (const byte*)gcpRendD3D->EF_GetPointer(eSrcPointer_Vert, &StrVrt, eType_FLOAT, eSrcPointer_Vert, flags);
        const byte* tangents = (const byte*)gcpRendD3D->EF_GetPointer(eSrcPointer_Tangent, &StrTan, eType_FLOAT, eSrcPointer_Tangent, FGP_SRC | FGP_REAL);

        verts = ((INT_PTR)verts > 256 && StrVrt >= sizeof(Vec3)) ? verts : 0;
        tangents = ((INT_PTR)tangents > 256 && (StrTan == sizeof(SPipQTangents) || StrTan == sizeof(SPipTangents))) ? tangents : 0;

        if (verts && tangents)
        {
            CTextureManager::Instance()->GetWhiteTexture()->Apply(0);
            gcpRendD3D->EF_SetColorOp(eCO_REPLACE, eCO_REPLACE, (eCA_Diffuse | (eCA_Diffuse << 3)), (eCA_Diffuse | (eCA_Diffuse << 3)));
            gcpRendD3D->EF_SetSrgbWrite(false);
            int nStateFlags = 0;
            if (gcpRendD3D->m_wireframe_mode == R_SOLID_MODE)
            {
                nStateFlags = GS_DEPTHWRITE;
            }
            if IsCVarConstAccess(constexpr) (CV_r_shownormals == 2)
            {
                nStateFlags = GS_NODEPTHTEST;
            }
            gcpRendD3D->FX_SetState(nStateFlags);
            gcpRendD3D->D3DSetCull(eCULL_None);
            gcpRendD3D->FX_SetFPMode();
            gcpRendD3D->FX_SetVertexDeclaration(0, eVF_P3F_C4B_T2F);

            // We must limit number of vertices, because TempDynVB (see code below)
            // uses transient pool that has *limited* size. See DevBuffer.cpp for details.
            // Note that one source vertex produces *six* buffer vertices (three tangent space
            // vectors, two vertices per vector).
            const size_t maxBufferSize = (size_t)NextPower2(gRenDev->CV_r_transient_pool_size) << 20;
            const size_t maxVertexCount = maxBufferSize / (6 * sizeof(SVF_P3F_C4B_T2F));
            const int numVerts = (int)(AZStd::GetMin)((size_t)gcpRendD3D->m_RP.m_RendNumVerts, maxVertexCount);

            TempDynVB<SVF_P3F_C4B_T2F> vb(gcpRendD3D);
            vb.Allocate(numVerts * 6);
            SVF_P3F_C4B_T2F* Verts = vb.Lock();

            for (int v = 0; v < numVerts; v++, verts += StrVrt, tangents += StrTan)
            {
                uint32 col0 = 0xffff0000;
                uint32 col1 = 0xffffffff;
                const Vec3& vPos = *(const Vec3*)verts;
                Vec3 vTan, vBiTan, vNorm;

                if (StrTan == sizeof(SPipQTangents))
                {
                    const Quat q = ((const SPipQTangents*)tangents)->GetQ();
                    vTan = q.GetColumn0();
                    vBiTan = q.GetColumn1();
                    vNorm = ((const SPipQTangents*)tangents)->GetN();
                }
                else
                {
                    ((const SPipTangents*)tangents)->GetTBN(vTan, vBiTan, vNorm);
                }

                Verts[v * 6 + 0].xyz = vPos;
                Verts[v * 6 + 0].color.dcolor = col0;

                Verts[v * 6 + 1].xyz = Vec3(vPos[0] + vTan[0] * len, vPos[1] + vTan[1] * len, vPos[2] + vTan[2] * len);
                Verts[v * 6 + 1].color.dcolor = col1;

                col0 = 0x0000ff00;
                col1 = 0x00ffffff;

                Verts[v * 6 + 2].xyz = vPos;
                Verts[v * 6 + 2].color.dcolor = col0;

                Verts[v * 6 + 3].xyz = Vec3(vPos[0] + vBiTan[0] * len, vPos[1] + vBiTan[1] * len, vPos[2] + vBiTan[2] * len);
                Verts[v * 6 + 3].color.dcolor = col1;

                col0 = 0x000000ff;
                col1 = 0x00ffffff;

                Verts[v * 6 + 4].xyz = vPos;
                Verts[v * 6 + 4].color.dcolor = col0;

                Verts[v * 6 + 5].xyz = Vec3(vPos[0] + vNorm[0] * len, vPos[1] + vNorm[1] * len, vPos[2] + vNorm[2] * len);
                Verts[v * 6 + 5].color.dcolor = col1;
            }

            vb.Unlock();
            vb.Bind(0);
            vb.Release();

            if (gcpRendD3D->m_RP.m_pCurPass)
            {
                CHWShader_D3D* curVS = (CHWShader_D3D*)gcpRendD3D->m_RP.m_pCurPass->m_VShader;
                for (uint32 i = 0; i < gcpRendD3D->m_RP.m_RIs[nRE].Num(); i++)
                {
                    SRendItem* pRI = gcpRendD3D->m_RP.m_RIs[nRE][i];
                    gcpRendD3D->FX_SetObjectTransform(pRI->pObj, NULL, pRI->pObj->m_ObjFlags);
                    curVS->UpdatePerInstanceConstantBuffer();
                    gcpRendD3D->FX_Commit();

                    gcpRendD3D->FX_DrawPrimitive(eptLineList, 0, numVerts * 6);
                }
            }

            gcpRendD3D->m_RP.m_VertexStreams[0].pStream = NULL;
        }
    }
}

void CD3D9Renderer::FX_DrawFurBending()
{
    float len = CRenderer::CV_r_normalslength;
    for (int nRE = 0; nRE <= gcpRendD3D->m_RP.m_nLastRE; nRE++)
    {
        gcpRendD3D->m_RP.m_pRE = gcpRendD3D->m_RP.m_RIs[nRE][0]->pElem;
        if (gcpRendD3D->m_RP.m_pRE)
        {
            if (nRE)
            {
                gcpRendD3D->m_RP.m_pRE->mfPrepare(false);
            }
            gcpRendD3D->m_RP.m_pRE->mfCheckUpdate(-1, gcpRendD3D->m_RP.m_TI[gcpRendD3D->m_RP.m_nProcessThreadID].m_nFrameUpdateID);
        }

        int StrVrt, StrNorm, StrTan, StrCol;

        const byte* verts = (const byte*)gcpRendD3D->EF_GetPointer(eSrcPointer_Vert, &StrVrt, eType_FLOAT, eSrcPointer_Vert, FGP_SRC | FGP_REAL);
        const byte* normals = (const byte*)gcpRendD3D->EF_GetPointer(eSrcPointer_Normal, &StrNorm, eType_FLOAT, eSrcPointer_Normal, FGP_SRC | FGP_REAL);
        const byte* tangents = (const byte*)gcpRendD3D->EF_GetPointer(eSrcPointer_Tangent, &StrTan, eType_FLOAT, eSrcPointer_Tangent, FGP_SRC | FGP_REAL);
        const byte* colors = (const byte*)gcpRendD3D->EF_GetPointer(eSrcPointer_Color, &StrCol, eType_FLOAT, eSrcPointer_Color, FGP_SRC | FGP_REAL);

        verts = ((INT_PTR)verts > 256 && StrVrt >= sizeof(Vec3)) ? verts : 0;
        normals = ((INT_PTR)normals > 256 && StrNorm >= sizeof(SPipNormal)) ? normals : 0;
        tangents = ((INT_PTR)tangents > 256 && (StrTan == sizeof(SPipQTangents) || StrTan == sizeof(SPipTangents))) ? tangents : 0;
        colors = ((INT_PTR)colors > 256 && StrCol >= sizeof(UCol)) ? colors : 0;

        if (verts && (normals || tangents) && colors)
        {
            CTextureManager::Instance()->GetWhiteTexture()->Apply(0);
            gcpRendD3D->EF_SetColorOp(eCO_REPLACE, eCO_REPLACE, (eCA_Diffuse | (eCA_Diffuse << 3)), (eCA_Diffuse | (eCA_Diffuse << 3)));
            gcpRendD3D->EF_SetSrgbWrite(false);
            int nStateFlags = 0;
            if (gcpRendD3D->m_wireframe_mode == R_SOLID_MODE)
            {
                nStateFlags = GS_DEPTHWRITE;
            }
            if (CV_r_FurShowBending == 2)
            {
                nStateFlags = GS_NODEPTHTEST;
            }
            gcpRendD3D->FX_SetState(nStateFlags);
            gcpRendD3D->D3DSetCull(eCULL_None);
            gcpRendD3D->FX_SetFPMode();
            gcpRendD3D->FX_SetVertexDeclaration(0, eVF_P3F_C4B_T2F);

            // We must limit number of vertices, because TempDynVB (see code below)
            // uses transient pool that has *limited* size. See DevBuffer.cpp for details.
            // Note that one source vertex produces *four* buffer vertices.
            const uint32 c_vertsPerPrimitive = 4;
            const size_t maxBufferSize = (size_t)NextPower2(gRenDev->CV_r_transient_pool_size) << 20;
            const size_t maxVertexCount = maxBufferSize / (c_vertsPerPrimitive * sizeof(SVF_P3F_C4B_T2F));
            const int numVerts = (int)(std::min)((size_t)gcpRendD3D->m_RP.m_RendNumVerts, maxVertexCount);

            TempDynVB<SVF_P3F_C4B_T2F> vb(gcpRendD3D);
            vb.Allocate(numVerts * c_vertsPerPrimitive);
            SVF_P3F_C4B_T2F* Verts = vb.Lock();

            uint32 col0 = 0xffff0000;
            uint32 col1 = 0xff00ff00;
            uint32 col2 = 0xff0000ff;

            const bool bHasNormals = (normals != 0);

            for (int v = 0; v < numVerts; v++, verts += StrVrt, normals += StrNorm, colors += StrCol)
            {
                Vec3 vPos = *(const Vec3*)verts;
                Vec3 vNorm;
                if (bHasNormals)
                {
                    vNorm = ((const SPipNormal*)normals)->GetN();
                }
                else if (StrTan == sizeof(SPipQTangents))
                {
                    vNorm = ((const SPipQTangents*)tangents)->GetN();
                }
                else
                {
                    vNorm = ((const SPipTangents*)tangents)->GetN();
                }
                vNorm.Normalize();

                UCol color = *(const UCol*)colors;
                Vec3 vColor(color.r / 255.0f * 2 - 1, color.b / 255.0f * 2 - 1, color.g / 255.0f * 2 - 1);
                vColor.Normalize();

                float furLength = len * color.a / 255.0f;

                Verts[v * c_vertsPerPrimitive + 0].xyz = vPos;
                Verts[v * c_vertsPerPrimitive + 0].color.dcolor = col0;

                Verts[v * c_vertsPerPrimitive + 1].xyz = vPos + vNorm * furLength;
                Verts[v * c_vertsPerPrimitive + 1].color.dcolor = col1;

                Verts[v * c_vertsPerPrimitive + 2].xyz = vPos + vNorm * furLength;
                Verts[v * c_vertsPerPrimitive + 2].color.dcolor = col1;

                Verts[v * c_vertsPerPrimitive + 3].xyz = vPos + vColor * furLength;
                Verts[v * c_vertsPerPrimitive + 3].color.dcolor = col2;
            }

            vb.Unlock();
            vb.Bind(0);
            vb.Release();

            if (gcpRendD3D->m_RP.m_pCurPass)
            {
                CHWShader_D3D* curVS = (CHWShader_D3D*)gcpRendD3D->m_RP.m_pCurPass->m_VShader;
                for (uint32 i = 0; i < gcpRendD3D->m_RP.m_RIs[nRE].Num(); i++)
                {
                    SRendItem* pRI = gcpRendD3D->m_RP.m_RIs[nRE][i];
                    gcpRendD3D->FX_SetObjectTransform(pRI->pObj, NULL, pRI->pObj->m_ObjFlags);
                    curVS->UpdatePerInstanceConstantBuffer();
                    gcpRendD3D->FX_Commit();

                    gcpRendD3D->FX_DrawPrimitive(eptLineList, 0, numVerts * c_vertsPerPrimitive);
                }
            }

            gcpRendD3D->m_RP.m_VertexStreams[0].pStream = NULL;
        }
    }
}

// Draw light sources in debug mode
// Draw debug geometry/info
void CD3D9Renderer::EF_DrawDebugTools(SViewport& VP, const SRenderingPassInfo& passInfo)
{
    AZ_TRACE_METHOD();
    if IsCVarConstAccess(constexpr) (CV_r_showlines)
    {
        EF_ProcessRenderLists(FX_DrawWire, 0, VP, passInfo, false);
    }

    if IsCVarConstAccess(constexpr) (CV_r_shownormals)
    {
        EF_ProcessRenderLists(FX_DrawNormals, 0, VP, passInfo, false);
    }

    if IsCVarConstAccess(constexpr) (CV_r_showtangents)
    {
        EF_ProcessRenderLists(FX_DrawTangents, 0, VP, passInfo, false);
    }

    if (CV_r_FurShowBending)
    {
        m_RP.m_pRenderFunc = FX_DrawFurBending;
        FX_ProcessRenderList(FurPasses::GetInstance().GetFurRenderList(), FB_FUR, false /*bSetRenderFunc*/);
    }
}

#if !defined(_RELEASE)
static int __cdecl TimeProfCallback(const VOID * arg1, const VOID * arg2)
{
    SProfInfo* pi1 = (SProfInfo*)arg1;
    SProfInfo* pi2 = (SProfInfo*)arg2;
    if (pi1->pTechnique->m_fProfileTime > pi2->pTechnique->m_fProfileTime)
    {
        return -1;
    }
    if (pi1->pTechnique->m_fProfileTime < pi2->pTechnique->m_fProfileTime)
    {
        return 1;
    }
    return 0;
}

static int __cdecl Compare_SProfInfo(const VOID * arg1, const VOID * arg2)
{
    SProfInfo* pi1 = (SProfInfo*)arg1;
    SProfInfo* pi2 = (SProfInfo*)arg2;

#if defined(CONSOLE_CONST_CVAR_MODE)
    if constexpr (CRenderer::CV_r_ProfileShadersGroupByName == 1)
#else
    if (gRenDev->CV_r_ProfileShadersGroupByName == 1)
#endif
    {
        char str1[128];
        char str2[128];

        sprintf_s(str1, sizeof(str1), "%s.%s", pi1->pShader->GetName(), pi1->pTechnique->m_NameStr.c_str());
        sprintf_s(str2, sizeof(str2), "%s.%s", pi2->pShader->GetName(), pi2->pTechnique->m_NameStr.c_str());

        return azstricmp(str1, str2);
    }
#if defined(CONSOLE_CONST_CVAR_MODE)
    else if constexpr (CRenderer::CV_r_ProfileShadersGroupByName == 2)
#else
    else if (gRenDev->CV_r_ProfileShadersGroupByName == 2)
#endif
    {
        return azstricmp(pi1->pTechnique->m_NameStr.c_str(), pi2->pTechnique->m_NameStr.c_str());
    }

    if (pi1->pTechnique > pi2->pTechnique)
    {
        return -1;
    }
    if (pi1->pTechnique < pi2->pTechnique)
    {
        return 1;
    }

    return 0;
}
#endif

struct STimeStorage
{
    float fNumPolys;
    float fNumDips;
    double fTime;
    float fItems;
    uint32 nUsedFrameId;
    STimeStorage()
    {
        fNumPolys = 0;
        fNumDips = 0;
        fTime = 0;
        fItems = 0;
        nUsedFrameId = 0;
    }
};


// Print shaders profile info on the screen
void CD3D9Renderer::EF_PrintProfileInfo()
{
#ifndef _RELEASE
#if defined(ENABLE_PROFILING_CODE)
    TextToScreenColor(1, 14, 0, 2, 0, 1, "Instances: %d, MatBatches: %d, GeomBatches: %d, DrawCalls: %d, Text: %d, Stat: %d, PShad: %d, VShad: %d", m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumRendInstances, m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumRendMaterialBatches, m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumRendGeomBatches, GetCurrentNumberOfDrawCalls(), m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumTextChanges, m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumStateChanges, m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumPShadChanges, m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumVShadChanges);
#endif
    TextToScreenColor(1, 17, 0, 2, 0, 1, "VShad: %d, PShad: %d, Text: %d", m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumVShaders, m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumPShaders, m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumTextures);
    TextToScreenColor(1, 20, 0, 2, 0, 1, "Preprocess: %8.02f ms, OccmOut. queries: %8.02f ms", m_RP.m_PS[m_RP.m_nProcessThreadID].m_fPreprocessTime * 1000.f, m_RP.m_PS[m_RP.m_nProcessThreadID].m_fOcclusionTime * 1000.f);
    TextToScreenColor(1, 23, 0, 2, 0, 1, "Skinning:   %8.02f ms (Skinned Objects: %d)", m_RP.m_PS[m_RP.m_nProcessThreadID].m_fSkinningTime * 1000.f, m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumRendSkinnedObjects);

    // TODO: implement CV_r_profileDIPs=2 mode - draw only one triangle per draw call
    // merge items with same grouping factor into single item
    if (m_RP.m_Profile.Num())
    {
        qsort(&m_RP.m_Profile[0], m_RP.m_Profile.Num(), sizeof(SProfInfo), Compare_SProfInfo);

        for (uint32 i = 0; (i + 1) < m_RP.m_Profile.Num(); i++)
        {
            if (!Compare_SProfInfo(&m_RP.m_Profile[i], &m_RP.m_Profile[i + 1]))
            {
                m_RP.m_Profile[i].Time += m_RP.m_Profile[i + 1].Time;
                m_RP.m_Profile[i].m_nItems++;
                m_RP.m_Profile[i].NumPolys += m_RP.m_Profile[i + 1].NumPolys;
                m_RP.m_Profile[i].NumDips += m_RP.m_Profile[i + 1].NumDips;
                m_RP.m_Profile.DelElem(i + 1);
                i--;
            }
        }
    }

    // smooth values over time
    if (CV_r_ProfileShadersSmooth && (CV_r_ProfileShadersGroupByName == 1 || CV_r_ProfileShadersGroupByName == 2))
    {
        typedef std::map<string, STimeStorage*, stl::less_stricmp<string> > TimeStorageMap;
        static TimeStorageMap timeStorageMap;

        char strName[128] = "";

        for (uint32 i = 0; i < m_RP.m_Profile.Num(); i++)
        {
            SProfInfo* pi1 = &m_RP.m_Profile[i];

            if (CV_r_ProfileShadersGroupByName == 1)
            {
                azsnprintf(strName, sizeof(strName), "%s.%s", pi1->pShader->GetName(), pi1->pTechnique->m_NameStr.c_str());
            }
            else
            {
                cry_strcpy(strName, pi1->pTechnique->m_NameStr.c_str());
            }

            STimeStorage* pTimeStorage = stl::find_in_map(timeStorageMap, CONST_TEMP_STRING(strName), NULL);
            if (!pTimeStorage)
            {
                pTimeStorage = timeStorageMap[strName] = new STimeStorage();
            }

            float fSmooth = CV_r_ProfileShadersSmooth;
            m_RP.m_Profile[i].pTechnique->m_fProfileTime = pTimeStorage->fTime = (m_RP.m_Profile[i].Time + pTimeStorage->fTime * fSmooth) / (fSmooth + 1.f);
            m_RP.m_Profile[i].m_nItems = (int)(pTimeStorage->fItems = ((float)m_RP.m_Profile[i].m_nItems + pTimeStorage->fItems * fSmooth) / (fSmooth + 1.f));
            m_RP.m_Profile[i].NumDips = (int)(pTimeStorage->fNumDips = ((float)m_RP.m_Profile[i].NumDips + pTimeStorage->fNumDips * fSmooth) / (fSmooth + 1.f));
            m_RP.m_Profile[i].NumPolys = (int)(pTimeStorage->fNumPolys = ((float)m_RP.m_Profile[i].NumPolys + pTimeStorage->fNumPolys * fSmooth) / (fSmooth + 1.f));
            pTimeStorage->nUsedFrameId = GetFrameID(false);
        }

        // fade items not used in this frame, delete not important items
        TimeStorageMap::iterator next;
        for (TimeStorageMap::iterator it = timeStorageMap.begin(); it != timeStorageMap.end(); it = next)
        {
            next = it;
            next++;
            STimeStorage* pTimeStorage = (STimeStorage*)(it->second);
            if (pTimeStorage->nUsedFrameId != GetFrameID(false))
            {
                float fSmooth = CV_r_ProfileShadersSmooth;
                pTimeStorage->fTime = (0 + pTimeStorage->fTime * fSmooth) / (fSmooth + 1.f);
                pTimeStorage->fItems = (0 + pTimeStorage->fItems * fSmooth) / (fSmooth + 1.f);
                pTimeStorage->fNumDips = (0 + pTimeStorage->fNumDips * fSmooth) / (fSmooth + 1.f);
                pTimeStorage->fNumPolys = (0 + pTimeStorage->fNumPolys * fSmooth) / (fSmooth + 1.f);

                if (pTimeStorage->fTime < 0.0001f)
                {
                    timeStorageMap.erase(it);
                    delete pTimeStorage;
                }
            }
        }
    }
    else
    {
        for (uint32 i = 0; i < m_RP.m_Profile.Num(); i++)
        {
            m_RP.m_Profile[i].pTechnique->m_fProfileTime =
                (float)(m_RP.m_Profile[i].Time + m_RP.m_Profile[i].pTechnique->m_fProfileTime * (float)CV_r_ProfileShadersSmooth) / ((float)CV_r_ProfileShadersSmooth + 1);
        }
    }

    const uint32 nMaxLines = 18;

    // sort by final smoothed time
    if (m_RP.m_Profile.Num())
    {
        qsort(&m_RP.m_Profile[0], m_RP.m_Profile.Num(), sizeof(SProfInfo), TimeProfCallback);
    }

    float fTimeAll = 0;

    // print
    for (uint32 nLine = 0; nLine < m_RP.m_Profile.Num(); nLine++)
    {
        float fProfTime = m_RP.m_Profile[nLine].pTechnique->m_fProfileTime * 1000.f;

        fTimeAll += fProfTime;

        if (nLine >= nMaxLines)
        {
            continue;
        }

        if (CV_r_ProfileShadersGroupByName == 1)
        { // no RT flags
            TextToScreenColor(4, (27 + (nLine * 3)), 1, 0, 0, 1, "%8.2f ms, %6d tris, %4d DIPs, '%s.%s', %d item(s)",
                fProfTime,
                m_RP.m_Profile[nLine].NumPolys,
                m_RP.m_Profile[nLine].NumDips,
                m_RP.m_Profile[nLine].pShader->GetName(),
                m_RP.m_Profile[nLine].pTechnique->m_NameStr.c_str(),
                m_RP.m_Profile[nLine].m_nItems + 1);
        }
        else if (CV_r_ProfileShadersGroupByName == 2)
        { // only Technique name - no RT flag, no shader name
            TextToScreenColor(4, (27 + (nLine * 3)), 1, 0, 0, 1, "%8.2f ms, %6d tris, %4d DIPs, '%s', %d item(s)",
                fProfTime,
                m_RP.m_Profile[nLine].NumPolys,
                m_RP.m_Profile[nLine].NumDips,
                m_RP.m_Profile[nLine].pTechnique->m_NameStr.c_str(),
                m_RP.m_Profile[nLine].m_nItems + 1);
        }
        else
        { // with RT flags and all names
            TextToScreenColor(4, (27 + (nLine * 3)), 1, 0, 0, 1, "%8.2f ms, %6d tris, %4d DIPs, '%s.%s(0x%x)', %d item(s)",
                fProfTime,
                m_RP.m_Profile[nLine].NumPolys,
                m_RP.m_Profile[nLine].NumDips,
                m_RP.m_Profile[nLine].pShader->GetName(),
                m_RP.m_Profile[nLine].pTechnique->m_NameStr.c_str(),
                m_RP.m_Profile[nLine].pShader->m_nMaskGenFX,
                m_RP.m_Profile[nLine].m_nItems + 1);
        }
    }

    TextToScreenColor(1, (28 + (nMaxLines * 3)), 0, 2, 0, 1, "Total unique items:            %8d", m_RP.m_Profile.Num());
    TextToScreenColor(1, (31 + (nMaxLines * 3)), 0, 2, 0, 1, "Total flush time:              %8.2f ms", fTimeAll);
    TextToScreenColor(1, (34 + (nMaxLines * 3)), 0, 2, 0, 1, "Total scene rendering time (MT): %8.2f ms", m_RP.m_PS[m_RP.m_nProcessThreadID].m_fSceneTimeMT);
    TextToScreenColor(1, (34 + (nMaxLines * 3)), 0, 2, 0, 1, "Total scene rendering time (RT): %8.2f ms", m_RP.m_PS[m_RP.m_nProcessThreadID].m_fRenderTime);
#endif
}


struct SPreprocess
{
    int m_nPreprocess;
    int m_Num;
    CRenderObject* m_pObject;
    int m_nTech;
    CShader* m_Shader;
    CShaderResources* m_pRes;
    IRenderElement* m_RE;
};

struct Compare2
{
    bool operator()(const SPreprocess& a, const SPreprocess& b) const
    {
        return a.m_nPreprocess < b.m_nPreprocess;
    }
};

// Current scene preprocess operations (Rendering to RT, screen effects initializing, ...)
int CD3D9Renderer::EF_Preprocess(SRendItem* ri, uint32 nums, uint32 nume, RenderFunc pRenderFunc, [[maybe_unused]] const SRenderingPassInfo& passInfo) PREFAST_SUPPRESS_WARNING(6262)
{
    AZ_TRACE_METHOD();

    uint32 i, j;
    CShader* Shader;
    CShaderResources* Res;
    CRenderObject* pObject;
    int nTech;

    SPreprocess Procs[512];
    uint32 nProcs = 0;

    CTimeValue time0 = iTimer->GetAsyncTime();

    if (m_logFileHandle != AZ::IO::InvalidHandle)
    {
        Logv(SRendItem::m_RecurseLevel[m_RP.m_nFillThreadID], "*** Start preprocess frame ***\n");
    }

    int DLDFlags = 0;
    int nReturn = 0;

    for (i = nums; i < nume; i++)
    {
        if (nProcs >= 512)
        {
            break;
        }
        SRendItem::mfGet(ri[i].SortVal, nTech, Shader, Res);
        pObject = ri[i].pObj;
        if (!(ri[i].nBatchFlags & FSPR_MASK))
        {
            break;
        }
        nReturn++;
        if (nTech < 0)
        {
            nTech = 0;
        }
        if (nTech < (int)Shader->m_HWTechniques.Num())
        {
            SShaderTechnique* pTech = Shader->m_HWTechniques[nTech];
            for (j = SPRID_FIRST; j < 32; j++)
            {
                uint32 nMask = 1 << j;
                if (nMask >= FSPR_MAX || nMask > (ri[i].nBatchFlags & FSPR_MASK))
                {
                    break;
                }
                if (nMask & ri[i].nBatchFlags)
                {
                    Procs[nProcs].m_nPreprocess = j;
                    Procs[nProcs].m_Num = i;
                    Procs[nProcs].m_Shader = Shader;
                    Procs[nProcs].m_pRes = Res;
                    Procs[nProcs].m_RE = ri[i].pElem;
                    Procs[nProcs].m_pObject = pObject;
                    Procs[nProcs].m_nTech = nTech;
                    nProcs++;
                }
            }
        }
    }
    if (!nProcs)
    {
        return 0;
    }
    std::sort(&Procs[0], &Procs[nProcs], Compare2());

    if (pRenderFunc != FX_FlushShader_General)
    {
        return nReturn;
    }

    bool bRes = true;
    for (i = 0; i < nProcs; i++)
    {
        SPreprocess* pr = &Procs[i];
        if (!pr->m_Shader)
        {
            continue;
        }
        switch (pr->m_nPreprocess)
        {
        case SPRID_SCANTEX:
        case SPRID_SCANTEXWATER:
            if (!(m_RP.m_TI[m_RP.m_nFillThreadID].m_PersFlags & RBPF_DRAWTOTEXTURE))
            {
                CRenderObject* pObj = pr->m_pObject;
                int nT = pr->m_nTech;
                if (nT < 0)
                {
                    nT = 0;
                }
                SShaderTechnique* pTech = pr->m_Shader->m_HWTechniques[nT];
                CShaderResources* pRes = pr->m_pRes;
                for (j = 0; j < pTech->m_RTargets.Num(); j++)
                {
                    SHRenderTarget* pTarg = pTech->m_RTargets[j];
                    if (pTarg->m_eOrder == eRO_PreProcess)
                    {
                        bRes &= FX_DrawToRenderTarget(pr->m_Shader, pRes, pObj, pTech, pTarg, pr->m_nPreprocess, pr->m_RE);
                    }
                }
                if (pRes)
                {
                    for (j = 0; j < pRes->m_RTargets.Num(); j++)
                    {
                        SHRenderTarget* pTarg = pRes->m_RTargets[j];
                        if (pTarg->m_eOrder == eRO_PreProcess)
                        {
                            bRes &= FX_DrawToRenderTarget(pr->m_Shader, pRes, pObj, pTech, pTarg, pr->m_nPreprocess, pr->m_RE);
                        }
                    }
                }
            }
            break;

        case SPRID_CUSTOMTEXTURE:
            if (!(m_RP.m_TI[m_RP.m_nFillThreadID].m_PersFlags & RBPF_DRAWTOTEXTURE))
            {
                CRenderObject* pObj = pr->m_pObject;
                int nT = pr->m_nTech;
                if (nT < 0)
                {
                    nT = 0;
                }
                SShaderTechnique* pTech = pr->m_Shader->m_HWTechniques[nT];
                CShaderResources* pRes = pr->m_pRes;
                for (j = 0; j < pRes->m_RTargets.Num(); j++)
                {
                    SHRenderTarget* pTarg = pRes->m_RTargets[j];
                    if (pTarg->m_eOrder == eRO_PreProcess)
                    {
                        bRes &= FX_DrawToRenderTarget(pr->m_Shader, pRes, pObj, pTech, pTarg, pr->m_nPreprocess, pr->m_RE);
                    }
                }
            }
            break;
        case SPRID_GENCLOUDS:
            break;

        default:
            assert(0);
        }
    }

    if (m_logFileHandle != AZ::IO::InvalidHandle)
    {
        Logv(SRendItem::m_RecurseLevel[m_RP.m_nFillThreadID], "*** End preprocess frame ***\n");
    }

    m_RP.m_PS[m_RP.m_nFillThreadID].m_fPreprocessTime += iTimer->GetAsyncTime().GetDifferenceInSeconds(time0);

    return nReturn;
}


void CD3D9Renderer::EF_EndEf2D([[maybe_unused]] const bool bSort)
{
}

//========================================================================================================

bool CRenderer::FX_TryToMerge(CRenderObject* pObjN, CRenderObject* pObjO, IRenderElement* pRE, bool bResIdentical)
{
#if !defined(_RELEASE)
    if (!CV_r_Batching)
    {
        return false;
    }
#endif

    if ((pRE == nullptr) || pRE->mfGetType() != eDATA_Mesh)
    {
        return false;
    }

#if defined(FEATURE_SVO_GI)
    if (m_RP.m_nPassGroupID == EFSLIST_VOXELIZE)
    {
        return false;
    }
#endif
    if (!bResIdentical || pRE != m_RP.m_pRE)
    {
        if (m_RP.m_nLastRE + 1 >= MAX_REND_GEOMS_IN_BATCH)
        {
            return false;
        }
        if ((pObjN->m_ObjFlags ^ pObjO->m_ObjFlags) & FOB_MASK_AFFECTS_MERGING_GEOM)
        {
            return false;
        }
        if ((pObjN->m_ObjFlags | pObjO->m_ObjFlags) & (FOB_SKINNED | FOB_DECAL_TEXGEN_2D | FOB_REQUIRES_RESOLVE | FOB_DISSOLVE | FOB_LIGHTVOLUME))
        {
            return false;
        }

        if (pObjN->m_nClipVolumeStencilRef != pObjO->m_nClipVolumeStencilRef)
        {
            return false;
        }

        /* Confetti: David Srour
        * Following is important as well.
        * As an example... if 2 glass material objects use "nearest_cubemap"
        * textures, the chosen texture might be picked differently depending on the
        * camera position within the scene -- this'll cause jarring popping as the
        * camera moves. This issue was observed on iOS.
        */
        if (pObjN->m_nTextureID != pObjO->m_nTextureID)
        {
            return false;
        }

        m_RP.m_RIs[++m_RP.m_nLastRE].SetUse(0);
        m_RP.m_pRE = pRE;
        return true;
    }

    // Batching/Instancing case
    if ((pObjN->m_ObjFlags ^ pObjO->m_ObjFlags) & FOB_MASK_AFFECTS_MERGING)
    {
        return false;
    }

    if ((pObjN->m_ObjFlags | pObjO->m_ObjFlags) & (FOB_REQUIRES_RESOLVE | FOB_LIGHTVOLUME))
    {
        return false;
    }

    if (pObjN->m_nMaterialLayers != pObjO->m_nMaterialLayers)
    {
        return false;
    }

    if (pObjN->m_nTextureID != pObjO->m_nTextureID)
    {
        return false;
    }

    if (pObjN->m_nClipVolumeStencilRef != pObjO->m_nClipVolumeStencilRef)
    {
        return false;
    }

    m_RP.m_ObjFlags |= pObjN->m_ObjFlags & FOB_SELECTED;
    m_RP.m_fMinDistance = min(pObjN->m_fDistance, m_RP.m_fMinDistance);

    return true;
}

// Note: When adding/removing batch flags/techniques, make sure to update sDescList / sBatchList
static const char* sDescList[] =
{
    "NULL",
    "Preprocess",
    "General",
    "ShadowGen",
    "Decal",
    "WaterVolume",
    "Transparent",
    "Water",
    "HDRPostProcess",
    "AfterHDRPostProcess",
    "PostProcess",
    "AfterPostProcess",
    "ShadowPass",
    "DeferredPreprocess",
    "Skin",
    "HalfResParticles",
    "ParticlesThickness",
    "LensOptics",
    "Voxelize",
    "EyeOverlay",
    "FogVolume",
    "GPUParticleCollisionCubemap",
    "RefractiveSurface",
};

static const char* sBatchList[] =
{
    "FB_GENERAL",
    "FB_TRANSPARENT",
    "FB_SKIN",
    "FB_Z",
    "FB_FUR",
    "FB_ZPREPASS",
    "FB_PREPROCESS",
    "FB_MOTIONBLUR",
    "FB_POST_3D_RENDER",
    "FB_MULTILAYERS",
    "NULL"
    "FB_CUSTOM_RENDER",
    "FB_SOFTALPHATEST",
    "FB_LAYER_EFFECT",
    "FB_WATER_REFL",
    "FB_WATER_CAUSTIC",
    "FB_DEBUG",
    "FB_PARTICLES_THICKNESS",
    "FB_TRANSPARENT_AFTER_DOF",
    "FB_EYE_OVERLAY"
};

// Init states before rendering of the scene
void CD3D9Renderer::FX_PreRender(int Stage)
{
    uint32 i;

    if (Stage & 1)
    { // Before preprocess
        m_RP.m_pSunLight = NULL;

        m_RP.m_Flags = 0;
        m_RP.m_pPrevObject = NULL;

        RT_SetCameraInfo();

        for (i = 0; i < m_RP.m_DLights[m_RP.m_nProcessThreadID][SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID]].Num(); i++)
        {
            SRenderLight* dl = &m_RP.m_DLights[m_RP.m_nProcessThreadID][SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID]][i];
            if (dl->m_Flags & DLF_FAKE)
            {
                continue;
            }

            if (dl->m_Flags & DLF_SUN)
            {
                m_RP.m_pSunLight = dl;
            }
        }
    }

    CHWShader_D3D::mfSetGlobalParams();
    m_RP.m_nCommitFlags = FC_ALL;
    FX_PushVP();
}

// Restore states after rendering of the scene
void CD3D9Renderer::FX_PostRender()
{
    SThreadInfo* const pShaderThreadInfo = &(m_RP.m_TI[m_RP.m_nProcessThreadID]);

    //FrameProfiler f("CD3D9Renderer:EF_PostRender", iSystem );
    FX_ObjectChange(NULL, NULL, m_RP.m_pIdendityRenderObject, NULL);
    m_RP.m_pRE = NULL;

    FX_ResetPipe();
    FX_PopVP();

    m_RP.m_nCurrResolveBounds[0] = m_RP.m_nCurrResolveBounds[1] = m_RP.m_nCurrResolveBounds[2] = m_RP.m_nCurrResolveBounds[3] = 0;
    m_RP.m_FlagsShader_MD = 0;
    m_RP.m_FlagsShader_MDV = 0;
    m_RP.m_FlagsShader_LT = 0;
    m_RP.m_pCurObject = m_RP.m_pIdendityRenderObject;

    pShaderThreadInfo->m_PersFlags |= RBPF_FP_DIRTY;
    m_RP.m_nCommitFlags = FC_ALL;
}

// Object changing handling (skinning, shadow maps updating, initial states setting, ...)
bool CD3D9Renderer::FX_ObjectChange(CShader* Shader, [[maybe_unused]] CShaderResources* Res, CRenderObject* obj, [[maybe_unused]] IRenderElement* pRE)
{
    FUNCTION_PROFILER_RENDER_FLAT

    SThreadInfo* const pShaderThreadInfo = &(m_RP.m_TI[m_RP.m_nProcessThreadID]);

    if ((pShaderThreadInfo->m_PersFlags & RBPF_SHADOWGEN))
    {
        const bool bNearObjOnly = m_RP.m_ShadowInfo.m_pCurShadowFrustum->m_eFrustumType == ShadowMapFrustum::e_Nearest;
        if (bNearObjOnly && !(obj->m_ObjFlags & FOB_NEAREST))
        {
            return false;
        }
    }

    if ((obj->m_ObjFlags & FOB_NEAREST) && CV_r_nodrawnear)
    {
        return false;
    }

    if (Shader)
    {
        if (pShaderThreadInfo->m_pIgnoreObject && pShaderThreadInfo->m_pIgnoreObject->m_pRenderNode == obj->m_pRenderNode)
        {
            return false;
        }
    }

    if (obj == m_RP.m_pPrevObject)
    {
        return true;
    }

    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_RefractionPartialResolves == 2)
    {
        if (m_RP.m_pCurObject == NULL || obj->m_pRenderNode == NULL || obj->m_pRenderNode != m_RP.m_pCurObject->m_pRenderNode)
        {
            m_RP.m_nCurrResolveBounds[0] = m_RP.m_nCurrResolveBounds[1] = m_RP.m_nCurrResolveBounds[2] = m_RP.m_nCurrResolveBounds[3] = 0;
        }
    }

    m_RP.m_pCurObject = obj;

    int flags = 0;
    if (obj != m_RP.m_pIdendityRenderObject) // Non-default object
    {
        if (obj->m_ObjFlags & FOB_NEAREST)
        {
            flags |= RBF_NEAREST;
        }

        if ((flags ^ m_RP.m_Flags) & RBF_NEAREST)
        {
            UpdateNearestChange(flags);
        }
    }
    else
    {
        HandleDefaultObject();
    }

    const uint32 nPerfFlagsExcludeMask = (RBPF_SHADOWGEN | RBPF_ZPASS);
    const uint32 nPerfFlags2ExcludeMask = (RBPF2_MOTIONBLURPASS | RBPF2_CUSTOM_RENDER_PASS);

    if ((m_RP.m_nPassGroupID == EFSLIST_TRANSP)
        && (obj->m_ObjFlags & FOB_REQUIRES_RESOLVE)
        && !(pShaderThreadInfo->m_PersFlags & nPerfFlagsExcludeMask)
        && !(m_RP.m_PersFlags2 & nPerfFlags2ExcludeMask))
    {
        if IsCVarConstAccess(constexpr) (bool(CRenderer::CV_r_RefractionPartialResolves))
        {
            int recursiveLevel = SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID];
            if (!recursiveLevel)
            {
                gcpRendD3D->FX_RefractionPartialResolve();
            }
        }
    }

    m_RP.m_fMinDistance = obj->m_fDistance;
    m_RP.m_pPrevObject = obj;
    m_RP.m_CurPassBitMask = 0;

    return true;
}

void CD3D9Renderer::UpdateNearestChange(int flags)
{
    const int nProcessThread = m_RP.m_nProcessThreadID;
    SThreadInfo* const pShaderThreadInfo = &(m_RP.m_TI[nProcessThread]);

    const ShadowMapFrustum* pCurFrustum = m_RP.m_ShadowInfo.m_pCurShadowFrustum;
    const bool bNearObjOnly = pCurFrustum && (pCurFrustum->m_eFrustumType == ShadowMapFrustum::e_Nearest) && (m_RP.m_pCurObject->m_ObjFlags & FOB_NEAREST);
    if (bNearObjOnly && (pShaderThreadInfo->m_PersFlags & RBPF_SHADOWGEN)) //add additional flag
    {
        //set per-object camera view
        Matrix44A& mPrj = pShaderThreadInfo->m_matProj;
        Matrix44A& mView = pShaderThreadInfo->m_matView;
        ShadowMapFrustum& curFrust = *m_RP.m_ShadowInfo.m_pCurShadowFrustum;

        mPrj = curFrust.mLightProjMatrix;
        mView = curFrust.mLightViewMatrix;

        EF_SetCameraInfo();
    }

    if (!(pShaderThreadInfo->m_PersFlags & RBPF_SHADOWGEN) && (m_drawNearFov > 0.0f))
    {
        if (flags & RBF_NEAREST)
        {
            CCamera Cam = pShaderThreadInfo->m_cam;
            m_RP.m_PrevCamera = Cam;
            if (m_logFileHandle != AZ::IO::InvalidHandle)
            {
                Logv(SRendItem::m_RecurseLevel[nProcessThread], "*** Prepare nearest Z range ***\n");
            }
            // set nice fov for weapons

            float fFov = Cam.GetFov();
            if (m_drawNearFov > 1.0f && m_drawNearFov < 179.0f)
            {
                fFov = DEG2RAD(m_drawNearFov);
            }

            float fNearRatio = DRAW_NEAREST_MIN / Cam.GetNearPlane();
            Cam.SetAsymmetry(Cam.GetAsymL() * fNearRatio, Cam.GetAsymR() * fNearRatio, Cam.GetAsymB() * fNearRatio, Cam.GetAsymT() * fNearRatio);
            Cam.SetFrustum(Cam.GetViewSurfaceX(), Cam.GetViewSurfaceZ(), fFov, DRAW_NEAREST_MIN, CV_r_DrawNearFarPlane, Cam.GetPixelAspectRatio());

            SetCamera(Cam);
            m_NewViewport.fMaxZ = CV_r_DrawNearZRange;
            m_RP.m_Flags |= RBF_NEAREST;
        }
        else
        {
            if (m_logFileHandle != AZ::IO::InvalidHandle)
            {
                Logv(SRendItem::m_RecurseLevel[nProcessThread], "*** Restore Z range ***\n");
            }

            SetCamera(m_RP.m_PrevCamera);
            m_NewViewport.fMaxZ = m_RP.m_PrevCamera.GetZRangeMax();
            m_RP.m_Flags &= ~RBF_NEAREST;
        }

        m_bViewportDirty = true;
    }
    m_RP.m_nCurrResolveBounds[0] = m_RP.m_nCurrResolveBounds[1] = m_RP.m_nCurrResolveBounds[2] = m_RP.m_nCurrResolveBounds[3] = 0;
}

void CD3D9Renderer::HandleDefaultObject()
{
    if (m_RP.m_Flags & (RBF_NEAREST))
    {
        if (m_logFileHandle != AZ::IO::InvalidHandle)
        {
            Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], "*** Restore Z range/camera ***\n");
        }
        SetCamera(m_RP.m_PrevCamera);
        m_NewViewport.fMaxZ = 1.0f;
        m_bViewportDirty = true;
        m_RP.m_Flags &= ~(RBF_NEAREST);
    }
    m_ViewMatrix = m_CameraMatrix;
    // Restore transform
    SThreadInfo* const pShaderThreadInfo = &(m_RP.m_TI[m_RP.m_nProcessThreadID]);
    pShaderThreadInfo->m_matView = m_CameraMatrix;
}

//=================================================================================
// Check buffer overflow during geometry batching
void CRenderer::FX_CheckOverflow(int nVerts, int nInds, IRenderElement* re, int* nNewVerts, int* nNewInds)
{
    if (nNewVerts)
    {
        *nNewVerts = nVerts;
    }
    if (nNewInds)
    {
        *nNewInds = nInds;
    }

    if (m_RP.m_pRE || (m_RP.m_RendNumVerts + nVerts >= m_RP.m_MaxVerts || m_RP.m_RendNumIndices + nInds >= m_RP.m_MaxTris * 3))
    {
        m_RP.m_pRenderFunc();
        if (nVerts >= m_RP.m_MaxVerts)
        {
            // iLog->Log("CD3D9Renderer::EF_CheckOverflow: numVerts >= MAX (%d > %d)\n", nVerts, m_RP.m_MaxVerts);
            assert(nNewVerts);
            *nNewVerts = m_RP.m_MaxVerts;
        }
        if (nInds >= m_RP.m_MaxTris * 3)
        {
            // iLog->Log("CD3D9Renderer::EF_CheckOverflow: numIndices >= MAX (%d > %d)\n", nInds, m_RP.m_MaxTris*3);
            assert(nNewInds);
            *nNewInds = m_RP.m_MaxTris * 3;
        }
        FX_Start(m_RP.m_pShader, m_RP.m_nShaderTechnique, m_RP.m_pShaderResources, re);
        FX_StartMerging();
    }
}

// Start of the new shader pipeline (3D pipeline version)
void CRenderer::FX_Start(CShader* ef, int nTech, CShaderResources* Res, [[maybe_unused]] IRenderElement* re)
{
    FUNCTION_PROFILER_RENDER_FLAT
        assert(ef);

    PrefetchLine(&m_RP.m_pCurObject, 64);
    PrefetchLine(&m_RP.m_Frame, 0);

    if (!ef)        // should not be 0, check to prevent crash
    {
        return;
    }

    PrefetchLine(&ef->m_vertexFormat, 0);

    m_RP.m_nNumRendPasses = 0;
    m_RP.m_FirstIndex = 0;
    m_RP.m_FirstVertex = 0;
    m_RP.m_RendNumIndices = 0;
    m_RP.m_RendNumVerts = 0;
    m_RP.m_RendNumGroup = -1;
    m_RP.m_pShader = ef;
    m_RP.m_nShaderTechnique = nTech;
    m_RP.m_nShaderTechniqueType = -1;
    m_RP.m_pShaderResources = Res;
    m_RP.m_FlagsPerFlush = 0;

    m_RP.m_FlagsStreams_Decl = 0;
    m_RP.m_FlagsStreams_Stream = 0;
    m_RP.m_FlagsShader_RT = 0;
    m_RP.m_FlagsShader_MD = 0;
    m_RP.m_FlagsShader_MDV = 0;

    const uint64 hdrMode = g_HWSR_MaskBit[HWSR_HDR_MODE];
    const uint64 sample0 = g_HWSR_MaskBit[HWSR_SAMPLE0];
    const uint64 sample1 = g_HWSR_MaskBit[HWSR_SAMPLE1];
    const uint64 sample4 = g_HWSR_MaskBit[HWSR_SAMPLE4];
    const uint64 tiled = g_HWSR_MaskBit[HWSR_TILED_SHADING];

    FX_ApplyShaderQuality(ef->m_eShaderType);

    const uint32 nPersFlags2 = m_RP.m_PersFlags2;
    if ((nPersFlags2 & RBPF2_HDR_FP16) && !(m_RP.m_nBatchFilter & (FB_Z)))
    {
        m_RP.m_FlagsShader_RT |= hdrMode; // deprecated: redundant flag, will be dropped (rendering always HDR)
    }
    static const uint32 nPFlags2Mask = (RBPF2_WATERRIPPLES | RBPF2_RAINRIPPLES | RBPF2_SKIN);
    if (nPersFlags2 & nPFlags2Mask)
    {
        if (nPersFlags2 & RBPF2_SKIN)
        {
            m_RP.m_FlagsShader_RT |= sample0;
        }
        else
        if ((nPersFlags2 & (RBPF2_WATERRIPPLES | RBPF2_RAINRIPPLES)) && ef->m_eShaderType == eST_Water)
        {
            m_RP.m_FlagsShader_RT |= (nPersFlags2 & RBPF2_WATERRIPPLES) ? sample4 : 0;
            m_RP.m_FlagsShader_RT |= (nPersFlags2 & RBPF2_RAINRIPPLES) ? g_HWSR_MaskBit[HWSR_OCEAN_PARTICLE] : 0;
        }
    }

    // Set shader flag for tiled forward shading
    if (CV_r_DeferredShadingTiled > 0)
    {
        m_RP.m_FlagsShader_RT |= tiled;
    }
    if (CRenderer::CV_r_SlimGBuffer)
    {
        m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SLIM_GBUFFER];
    }

    SThreadInfo* const pShaderThreadInfo = &(m_RP.m_TI[m_RP.m_nProcessThreadID]);
    if (pShaderThreadInfo->m_PersFlags & RBPF_REVERSE_DEPTH)
    {
        m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_REVERSE_DEPTH];
    }

    m_RP.m_fCurOpacity = 1.0f;
    m_RP.m_CurVFormat = ef->m_vertexFormat;
    m_RP.m_ObjFlags = m_RP.m_pCurObject->m_ObjFlags;
    m_RP.m_RIs[0].SetUse(0);
    m_RP.m_nLastRE = 0;


    m_RP.m_pRE = NULL;
    m_RP.m_Frame++;
}

//==============================================================================================

static void sBatchFilter(uint32 nFilter, char* sFilt)
{
    STATIC_ASSERT((1 << ((sizeof(sBatchList) / sizeof(sBatchList[0])) - 1) <= FB_MASK), "Batch techniques/flags list mismatch");

AZ_PUSH_DISABLE_WARNING(4996, "-Wunknown-warning-option")
    sFilt[0] = 0;
    int n = 0;
    for (int i = 0; i < sizeof(sBatchList) / sizeof(sBatchList[0]); i++)
    {
        if (nFilter & (1 << i))
        {
            if (n)
            {
                strcat(sFilt, "|");
            }
            strcat(sFilt, sBatchList[i]);
            n++;
        }
    }
AZ_POP_DISABLE_WARNING
}

void CD3D9Renderer::FX_StartBatching()
{
    m_RP.m_nCommitFlags = FC_ALL;
}

void CD3D9Renderer::FX_ProcessBatchesList(int nums, int nume, uint32 nBatchFilter, uint32 nBatchExcludeFilter)
{
    PROFILE_FRAME(ProcessBatchesList);

    if (nume - nums == 0)
    {
        return;
    }
    SRenderPipeline& RESTRICT_REFERENCE rRP = m_RP;
    int nList = rRP.m_nPassGroupID;
    int nAW = rRP.m_nSortGroupID;
    int nThreadID = rRP.m_nProcessThreadID;

    auto& RESTRICT_REFERENCE RI = CRenderView::CurrentRenderView()->GetRenderItems(nAW, nList);
    assert(nums < RI.size());
    assert(nume <= RI.size());

    SRendItem* pPrefetchPlainPtr = &RI[0];

    rRP.m_nBatchFilter = nBatchFilter;

    // make sure all all jobs which are computing particle vertices/indices
    // have finished and their vertex/index buffers are unlocked
    // before starting rendering of those
    if (rRP.m_nPassGroupID == EFSLIST_TRANSP || rRP.m_nPassGroupID == EFSLIST_HALFRES_PARTICLES || rRP.m_nPassGroupID == EFSLIST_PARTICLES_THICKNESS)
    {
        m_ComputeVerticesJobExecutors[m_RP.m_nProcessThreadID].WaitForCompletion();
        UnLockParticleVideoMemory(gRenDev->m_nPoolIndexRT % SRenderPipeline::nNumParticleVertexIndexBuffer);
    }

#ifdef DO_RENDERLOG
    STATIC_ASSERT(((sizeof(sDescList) / sizeof(sDescList[0])) == EFSLIST_NUM), "Batch techniques/flags list mismatch");

    if (CV_r_log)
    {
        char sFilt[256];
        sBatchFilter(nBatchFilter, sFilt);
        Logv(SRendItem::m_RecurseLevel[nThreadID], "\n*** Start batch list %s (Filter: %s) (%s) ***\n", sDescList[nList], sFilt, nAW ? "After water" : "Before water");
    }
#endif

    uint32 prevSortVal = -1;
    CShader* pShader = NULL;
    CShaderResources* pCurRes = NULL;
    CRenderObject* pCurObject = NULL;
    CShader* pCurShader = NULL;
    int nTech;

    for (int i = nums; i < nume; i++)
    {
        SRendItem& ri = RI[i];
        if (!(ri.nBatchFlags & nBatchFilter))
        {
            continue;
        }
        if (ri.nBatchFlags & nBatchExcludeFilter)
        {
            continue;
        }

        CRenderObject* pObject = ri.pObj;
        IRenderElement* pRE = ri.pElem;
        bool bChangedShader = false;
        bool bResIdentical = true;
        if (prevSortVal != ri.SortVal)
        {
            CShaderResources* pRes;
            SRendItem::mfGet(ri.SortVal, nTech, pShader, pRes);
            if (pShader != pCurShader || !pRes || !pCurRes || pRes->m_IdGroup != pCurRes->m_IdGroup || (pObject->m_ObjFlags & (FOB_SKINNED | FOB_DECAL))) // Additional check for materials batching
            {
                bChangedShader = true;
            }
            bResIdentical = (pRes == pCurRes);
            pCurRes = pRes;
            prevSortVal = ri.SortVal;
        }
        if (!bChangedShader && FX_TryToMerge(pObject, pCurObject, pRE, bResIdentical))
        {
            rRP.m_RIs[rRP.m_nLastRE].AddElem(&ri);
            continue;
        }
        // when not doing main pass rendering, need to flush the shader for each data part since the external VMEM buffers are laid out only for the main pass
        if (pObject && pObject != pCurObject || (m_RP.m_FlagsPerFlush & RBSI_EXTERN_VMEM_BUFFERS))
        {
            if (pCurShader)
            {
                rRP.m_pRenderFunc();
                pCurShader = NULL;
                bChangedShader = true;
            }
            if (!FX_ObjectChange(pShader, pCurRes, pObject, pRE))
            {
                prevSortVal = ~0;
                continue;
            }
            pCurObject = pObject;
        }

        if (bChangedShader)
        {
            if (pCurShader)
            {
                rRP.m_pRenderFunc();
            }

            pCurShader = pShader;
            FX_Start(pShader, nTech, pCurRes, pRE);
        }

        if (pRE)
        {
            pRE->mfPrepare(true);
        }

        if (rRP.m_RIs[0].size() == 0)
        {
            rRP.m_RIs[0].AddElem(&ri);
        }
    }
    if (pCurShader)
    {
        rRP.m_pRenderFunc();
    }

#ifdef DO_RENDERLOG
    if (CV_r_log)
    {
        Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], "*** End batch list ***\n\n");
    }
#endif
}

//////////////////////////////////////////////////////////////////////////
// Only do expensive DX12 resource set building for PC DX12
#if defined(CRY_USE_DX12)
void CD3D9Renderer::PerFrameValidateResourceSets()
{
    AZ_TRACE_METHOD();
    int dirtyCount = CDeviceResourceSet::GetGlobalDirtyCount();
    if (dirtyCount != 0)
    {
        // Goes thou the list of all known resources and check if any of them needs to be re build.
        //////////////////////////////////////////////////////////////////////////
        for (unsigned int i = 0; i < CShader::s_ShaderResources_known.Num(); i++)
        {
            CShaderResources* pSR = CShader::s_ShaderResources_known[i];
            if (pSR && pSR->m_pCompiledResourceSet)
            {
                if (pSR->m_pCompiledResourceSet->IsDirty())
                {
                    pSR->m_pCompiledResourceSet->Build();
                }
            }
        }
        if (dirtyCount == CDeviceResourceSet::GetGlobalDirtyCount())
        {
            CDeviceResourceSet::ResetGlobalDirtyCount();
        }
    }
}
#endif

void CD3D9Renderer::FX_ProcessRenderList(int nums, int nume, int nList, int nAfterWater, void(* RenderFunc)(), bool bLighting, 
    uint32 nBatchFilter, uint32 nBatchExcludeFilter)
{
    if (nume - nums < 1)
    {
        return;
    }

    SThreadInfo* const pShaderThreadInfo = &(m_RP.m_TI[m_RP.m_nProcessThreadID]);

    const bool bTranspPass = (nList == EFSLIST_TRANSP) || (nList == EFSLIST_HALFRES_PARTICLES);
    if (bTranspPass && !CV_r_TransparentPasses)
    {
        return;
    }

    Matrix44A origMatView = pShaderThreadInfo->m_matView;
    Matrix44A origMatProj = pShaderThreadInfo->m_matProj;

    m_RP.m_pRenderFunc = RenderFunc;

    m_RP.m_pCurObject = m_RP.m_pIdendityRenderObject;
    m_RP.m_pPrevObject = m_RP.m_pCurObject;

    FX_PreRender(3);

    int nPrevGroup = m_RP.m_nPassGroupID;
    int nPrevGroup2 = m_RP.m_nPassGroupDIP;
    int nPrevSortGroupID = m_RP.m_nSortGroupID;

    m_RP.m_nPassGroupID = nList;
    m_RP.m_nPassGroupDIP = nList;
    m_RP.m_nSortGroupID = nAfterWater;

    FX_ProcessBatchesList(nums, nume, nBatchFilter, nBatchExcludeFilter);

    if (bLighting)
    {
        FX_ProcessPostGroups(nums, nume);
    }

    FX_PostRender();

    pShaderThreadInfo->m_matView = origMatView;
    pShaderThreadInfo->m_matProj = origMatProj;

    m_RP.m_nPassGroupID = nPrevGroup;
    m_RP.m_nPassGroupDIP = nPrevGroup2;
    m_RP.m_nSortGroupID = nPrevSortGroupID;
}

void CD3D9Renderer::FX_ProcessRenderList(int nList, uint32 nBatchFilter, bool bSetRenderFunc /* = true */)
{
    FX_PreRender(3);

    if (bSetRenderFunc)
    {
        m_RP.m_pRenderFunc = FX_FlushShader_General;
    }
    m_RP.m_nPassGroupID = nList;
    m_RP.m_nPassGroupDIP = nList;

    //PROFILE_DIPS_START;

    m_RP.m_nSortGroupID = 0;
    FX_ProcessBatchesList(m_RP.m_pRLD->m_nStartRI[m_RP.m_nSortGroupID][nList], m_RP.m_pRLD->m_nEndRI[m_RP.m_nSortGroupID][nList], nBatchFilter);

    m_RP.m_nSortGroupID = 1;
    FX_ProcessBatchesList(m_RP.m_pRLD->m_nStartRI[m_RP.m_nSortGroupID][nList], m_RP.m_pRLD->m_nEndRI[m_RP.m_nSortGroupID][nList], nBatchFilter);

    //PROFILE_DIPS_END(nList);

    FX_PostRender();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////


void CD3D9Renderer::FX_ProcessZPassRender_List(ERenderListID list, uint32 filter = FB_Z)
{
    m_RP.m_nPassGroupID = list;
    m_RP.m_nPassGroupDIP = list;

    m_RP.m_nSortGroupID = 0;
    FX_ProcessBatchesList(m_RP.m_pRLD->m_nStartRI[m_RP.m_nSortGroupID][list], m_RP.m_pRLD->m_nEndRI[m_RP.m_nSortGroupID][list], filter);
    m_RP.m_nSortGroupID = 1;
    FX_ProcessBatchesList(m_RP.m_pRLD->m_nStartRI[m_RP.m_nSortGroupID][list], m_RP.m_pRLD->m_nEndRI[m_RP.m_nSortGroupID][list], filter);
}

void CD3D9Renderer::FX_ProcessZPassRenderLists()
{
    PROFILE_LABEL_SCOPE("ZPASS");

    if (SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID] > 0)
    {
        return;
    }

    uint32 bfGeneral = SRendItem::BatchFlags(EFSLIST_GENERAL, m_RP.m_pRLD);
    uint32 bfSkin = SRendItem::BatchFlags(EFSLIST_SKIN, m_RP.m_pRLD);
    uint32 bfTransp = SRendItem::BatchFlags(EFSLIST_TRANSP, m_RP.m_pRLD);
    uint32 bfDecal = SRendItem::BatchFlags(EFSLIST_DECAL, m_RP.m_pRLD);
    bfGeneral |= FB_Z;


    if ((bfGeneral | bfSkin | bfTransp | bfDecal) & FB_Z)
    {
#ifdef DO_RENDERLOG
        if (CV_r_log)
        {
            Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], "*** Start z-pass ***\n");
        }
#endif

        FX_PreRender(3);

        m_RP.m_pRenderFunc = FX_FlushShader_ZPass;
        const bool isGmemEnabled = FX_GetEnabledGmemPath(nullptr) != CD3D9Renderer::eGT_REGULAR_PATH;
        bool bClearZBuffer = !(m_RP.m_nRendFlags & SHDF_DO_NOT_CLEAR_Z_BUFFER);

        // For GMEM paths, depth/stencil clear gets set in CD3D9Renderer::FX_GmemTransition(...).
        bClearZBuffer &= !isGmemEnabled;

        // For GMEM paths, velocity RT clear gets set in CD3D9Renderer::FX_GmemTransition(...).
        if (!FX_GetEnabledGmemPath(nullptr) && UseHalfFloatRenderTargets())
        {
            FX_ClearTarget(GetUtils().GetVelocityObjectRT(), Clr_White);
        }

        if (CRenderer::CV_r_usezpass == 2)
        {
            if (bfGeneral & FB_ZPREPASS)
            {
                PROFILE_LABEL_SCOPE("ZPREPASS");

                // Clear z target to prevent issues during reprojection.
                // Following would resolve GMEM paths.
                if (!FX_GetEnabledGmemPath(nullptr))
                {
                    FX_ClearTarget(CTexture::s_ptexZTarget, Clr_White);
                }

                FX_ZScene(true, bClearZBuffer, false, true);

                FX_ProcessZPassRender_List(EFSLIST_GENERAL, FB_ZPREPASS);

                FX_ZScene(false, false, false, true);
                bClearZBuffer = false;
            }
        }

        {
            PROFILE_LABEL_SCOPE("GBUFFER");

            FX_ZScene(true, bClearZBuffer);

            if (bfGeneral & FB_Z)
            {
                PROFILE_LABEL_SCOPE("GENERAL");
                FX_ProcessZPassRender_List(EFSLIST_GENERAL);
            }
            if (bfSkin    & FB_Z)
            {
                PROFILE_LABEL_SCOPE("SKIN");
                FX_ProcessZPassRender_List(EFSLIST_SKIN);
            }
            if (bfTransp  & FB_Z)
            {
                PROFILE_LABEL_SCOPE("TRANSPARENT");
                FX_ProcessZPassRender_List(EFSLIST_TRANSP);
            }

            // PC special case: render terrain/decals/roads normals separately - disable mrt rendering, on consoles we always use single rt for output
            FX_ZScene(false, false);
            FX_ZScene(true, false, true);

            m_RP.m_PersFlags2 &= ~RBPF2_NOALPHABLEND;
            m_RP.m_StateAnd |= GS_BLEND_MASK;

            if (bfDecal        & FB_Z)
            {
                PROFILE_LABEL_SCOPE("DECALS");
                FX_ProcessZPassRender_List(EFSLIST_DECAL);
            }

            FX_ZScene(false, false, true);
        }

        if (isGmemEnabled)
        {
            FX_GmemTransition(eGT_POST_GBUFFER);
        }

        // For some GMEM paths, the depth gets linearized right away during z-pass.
        // Depth downsampling gets done during transitions in CD3D9Renderer::FX_GmemTransition(...).
        if (!isGmemEnabled)
        {
            // Reset current object so we don't end up with RBF_NEAREST states in FX_LinearizeDepth
            FX_ObjectChange(NULL, NULL, m_RP.m_pIdendityRenderObject, NULL);

            FX_LinearizeDepth(CTexture::s_ptexZTarget);

            if (!CRenderer::CV_r_EnableComputeDownSampling)
            {
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DRENDPIPELINE_CPP_SECTION_5
    #include AZ_RESTRICTED_FILE(D3DRendPipeline_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
                GetUtils().DownsampleDepth(CTexture::s_ptexZTarget, CTexture::s_ptexZTargetScaled, true);
#endif
                GetUtils().DownsampleDepth(CTexture::s_ptexZTargetScaled, CTexture::s_ptexZTargetScaled2, false);
            }
            else
            {
                CTexture* UAVArr[2] =
                {
                    CTexture::s_ptexZTargetScaled,
                    CTexture::s_ptexZTargetScaled2
                };
                GetUtils().DownsampleDepthUsingCompute(CTexture::s_ptexZTarget, UAVArr, false);
            }
        }

        FurPasses::GetInstance().ExecuteZPostPass();

        FX_ZScene(true, false, true);
        m_RP.m_PersFlags2 &= ~RBPF2_NOALPHABLEND;
        m_RP.m_StateAnd |= GS_BLEND_MASK;

        FX_PostRender();
        RT_SetViewport(0, 0, GetWidth(), GetHeight());

        if (m_RP.m_PersFlags2 & RBPF2_ALLOW_DEFERREDSHADING)
        {
            m_bDeferredDecals = FX_DeferredDecals();
        }

        m_RP.m_PersFlags2 |= RBPF2_NOALPHABLEND;
        m_RP.m_StateAnd &= ~GS_BLEND_MASK;

        FX_ZScene(false, false, true);

        FX_ZTargetReadBack();

        m_RP.m_pRenderFunc = FX_FlushShader_General;

#ifdef DO_RENDERLOG
        if (CV_r_log)
        {
            Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], "*** End z-pass ***\n");
        }
#endif
    }
}

//////////////////////////////////////////////////////////////////////////

void CD3D9Renderer::FX_ProcessThicknessRenderLists()
{
    int recursiveLevel = SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID];
    if ((m_RP.m_nRendFlags & SHDF_ALLOWPOSTPROCESS) && recursiveLevel <= 0 && 0) // Thickness pass disabled temporarily
    {
        uint32 nBatchMask = SRendItem::BatchFlags(EFSLIST_TRANSP, m_RP.m_pRLD);
        if (nBatchMask & FB_PARTICLES_THICKNESS)
        {
            PROFILE_LABEL_SCOPE("PARTICLES_THICKNESS_PASS");

            CTexture* pThicknessTarget = CTexture::s_ptexBackBufferScaled[1];
            const uint32 nWidthRT = pThicknessTarget->GetWidth();
            const uint32 nHeightRT = pThicknessTarget->GetHeight();

            FX_PreRender(3);

            // Get current viewport
            int iTempX, iTempY, iWidth, iHeight;
            GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);

            FX_ClearTarget(pThicknessTarget, Clr_Median);
            FX_PushRenderTarget(0, pThicknessTarget, NULL);
            RT_SetViewport(0, 0, nWidthRT, nHeightRT);

            m_RP.m_nPassGroupID = EFSLIST_TRANSP;
            m_RP.m_nPassGroupDIP = EFSLIST_TRANSP;

            m_RP.m_nSortGroupID = 0;
            FX_ProcessBatchesList(m_RP.m_pRLD->m_nStartRI[m_RP.m_nSortGroupID][EFSLIST_TRANSP], m_RP.m_pRLD->m_nEndRI[m_RP.m_nSortGroupID][EFSLIST_TRANSP], FB_PARTICLES_THICKNESS);

            m_RP.m_nSortGroupID = 1;
            FX_ProcessBatchesList(m_RP.m_pRLD->m_nStartRI[m_RP.m_nSortGroupID][EFSLIST_TRANSP], m_RP.m_pRLD->m_nEndRI[m_RP.m_nSortGroupID][EFSLIST_TRANSP], FB_PARTICLES_THICKNESS);

            FX_PopRenderTarget(0);

            PostProcessUtils().TexBlurGaussian(pThicknessTarget, 1, 1, 1.0f, false);
            FX_SetActiveRenderTargets();
            RT_SetViewport(iTempX, iTempY, iWidth, iHeight);
            FX_PostRender();
        }
    }
}

//////////////////////////////////////////////////////////////////////////

void CD3D9Renderer::FX_ProcessSoftAlphaTestRenderLists()
{
    int32 nList = EFSLIST_GENERAL;

    int recursiveLevel = SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID];
    if ((m_RP.m_nRendFlags & SHDF_ALLOWPOSTPROCESS) && recursiveLevel <= 0)
    {
#ifdef DO_RENDERLOG
        if (CV_r_log)
        {
            Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], "*** Begin soft alpha test pass ***\n");
        }
#endif

        uint32 nBatchMask = SRendItem::BatchFlags(nList, m_RP.m_pRLD);
        if (nBatchMask & FB_SOFTALPHATEST)
        {
            m_RP.m_PersFlags2 |= RBPF2_NOALPHATEST;

            FX_PreRender(3);

            m_RP.m_nPassGroupID = nList;
            m_RP.m_nPassGroupDIP = nList;

            m_RP.m_nSortGroupID = 0;
            FX_ProcessBatchesList(m_RP.m_pRLD->m_nStartRI[m_RP.m_nSortGroupID][nList], m_RP.m_pRLD->m_nEndRI[m_RP.m_nSortGroupID][nList], FB_SOFTALPHATEST);
            m_RP.m_nSortGroupID = 1;
            FX_ProcessBatchesList(m_RP.m_pRLD->m_nStartRI[m_RP.m_nSortGroupID][nList], m_RP.m_pRLD->m_nEndRI[m_RP.m_nSortGroupID][nList], FB_SOFTALPHATEST);

            FX_PostRender();

            m_RP.m_PersFlags2 &= ~RBPF2_NOALPHATEST;
        }

#ifdef DO_RENDERLOG
        if (CV_r_log)
        {
            Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], "*** End soft alpha test pass ***\n");
        }
#endif
    }
}


void CD3D9Renderer::FX_ProcessPostRenderLists(uint32 nBatchFilter)
{
    int recursiveLevel = SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID];

    if ((m_RP.m_nRendFlags & SHDF_ALLOWPOSTPROCESS) && recursiveLevel <= 0)
    {
        int nList = EFSLIST_GENERAL;
        uint32 nBatchMask = SRendItem::BatchFlags(EFSLIST_GENERAL, m_RP.m_pRLD) | SRendItem::BatchFlags(EFSLIST_TRANSP, m_RP.m_pRLD);
        nBatchMask |= SRendItem::BatchFlags(EFSLIST_DECAL, m_RP.m_pRLD);
        nBatchMask |= SRendItem::BatchFlags(EFSLIST_SKIN, m_RP.m_pRLD);
        if (nBatchMask & nBatchFilter)
        {
            if (nBatchFilter == FB_CUSTOM_RENDER || nBatchFilter == FB_POST_3D_RENDER)
            {
                FX_CustomRenderScene(true);
            }

            FX_ProcessRenderList(EFSLIST_GENERAL, nBatchFilter);
            FX_ProcessRenderList(EFSLIST_SKIN, nBatchFilter);

            if (nBatchFilter != FB_MOTIONBLUR)
            {
                FX_ProcessRenderList(EFSLIST_DECAL, nBatchFilter);
            }

            FX_ProcessRenderList(EFSLIST_TRANSP, nBatchFilter);

            if (nBatchFilter == FB_CUSTOM_RENDER || nBatchFilter == FB_POST_3D_RENDER)
            {
                FX_CustomRenderScene(false);
            }
        }
    }
}

void CD3D9Renderer::FX_ProcessPostGroups(int nums, int nume)
{
    const uint32 nPrevPersFlags2 = m_RP.m_PersFlags2;
    m_RP.m_PersFlags2 &= ~RBPF2_FORWARD_SHADING_PASS;

    uint32 nBatchMask = m_RP.m_pRLD->m_nBatchFlags[m_RP.m_nSortGroupID][m_RP.m_nPassGroupID];
    AZ_PUSH_DISABLE_WARNING(, "-Wconstant-logical-operand")
    if (nBatchMask & FB_MULTILAYERS && CV_r_usemateriallayers)
    AZ_POP_DISABLE_WARNING
    {
        FX_ProcessBatchesList(nums, nume, FB_MULTILAYERS);
    }
    if (0 != (nBatchMask & FB_DEBUG))
    {
        FX_ProcessBatchesList(nums, nume, FB_DEBUG);
    }

    m_RP.m_PersFlags2 = nPrevPersFlags2;
}


void CD3D9Renderer::FX_ApplyThreadState(SThreadInfo& TI, SThreadInfo* pOldTI)
{
    if (pOldTI)
    {
        *pOldTI = m_RP.m_TI[m_RP.m_nProcessThreadID];
    }

    m_RP.m_TI[m_RP.m_nProcessThreadID] = TI;
}

CD3D9Renderer::OcclusionReadbackData::~OcclusionReadbackData()
{
    Destroy();
}

void CD3D9Renderer::OcclusionReadbackData::Destroy()
{
    azfree(m_occlusionReadbackBuffer);
    m_occlusionReadbackBuffer = nullptr;
}
void CD3D9Renderer::OcclusionReadbackData::Reset(bool reverseDepth)
{
    m_occlusionReadbackViewProj.SetIdentity();

    if (!m_occlusionReadbackBuffer)
    {
        m_occlusionReadbackBuffer = static_cast<float*>(azmalloc(CD3D9Renderer::s_occlusionBufferNumElements * sizeof(float), 16));
    }
    std::fill(m_occlusionReadbackBuffer, m_occlusionReadbackBuffer + CD3D9Renderer::s_occlusionBufferNumElements, (reverseDepth) ? 0.0f : 1.0f);
}

void CD3D9Renderer::InvalidateCoverageBufferData()
{
    static const char* occlusionDataTextureName[s_numOcclusionReadbackTextures] = {
        "$ZTargetReadBack0",
        "$ZTargetReadBack1",
        "$ZTargetReadBack2"
    };

    static_assert(s_numOcclusionReadbackTextures == 3, "Change the initialization of occlusionDataTextureName if you change s_numOcclusionReadbackTextures!");

    for (size_t i = 0; i < s_numOcclusionReadbackTextures; i++)
    {
        m_occlusionData[i].SetupOcclusionData(occlusionDataTextureName[i]);
    }
    m_cpuOcclusionReadIndex = 0;
    m_occlusionBufferIndex = 0;
}

void CD3D9Renderer::CPUOcclusionData::SetupOcclusionData(const char* textureName)
{
    m_occlusionDataState = CPUOcclusionData::OcclusionDataState::OcclusionDataInvalid;
    m_occlusionViewProj.SetIdentity();
    
    // Note: The macro Clr_FarPlane_R expects the variable name bReverseDepth to determine the clear value
    const bool bReverseDepth = CRenderer::CV_r_ReverseDepth > 0 ? true : false;
    if (!m_zTargetReadback)
    {
        unsigned int flags = FT_DONT_STREAM | FT_DONT_RELEASE | FT_STAGE_READBACK;
        m_zTargetReadback = CTexture::CreateTextureObject(textureName, s_occlusionBufferWidth, s_occlusionBufferHeight, 1, eTT_2D, flags, eTF_Unknown);
        //CPU reading code expects it to be 32 bit float. Changing this to 16bit would require "16bit to 32bitfloat" conversion in FX_ZTargetReadBackOnCPU.
        m_zTargetReadback->CreateRenderTarget(eTF_R32F, Clr_FarPlane_R); 
    }

    m_occlusionReadbackData.Reset(bReverseDepth);    
}

void CD3D9Renderer::CPUOcclusionData::Destroy()
{
    SAFE_RELEASE(m_zTargetReadback);
    m_occlusionReadbackData.Destroy();
}

int CD3D9Renderer::GetOcclusionBuffer(uint16* pOutOcclBuffer, Matrix44* pmCamBuffer)
{
    AZ_Assert(m_cpuOcclusionReadIndex < s_numOcclusionReadbackTextures, "m_cpuOcclusionReadIndex (%u) out of range (%u)", m_cpuOcclusionReadIndex.load(), s_numOcclusionReadbackTextures);
    const CPUOcclusionData& occlusionData = m_occlusionData[m_cpuOcclusionReadIndex];

    // Do not perform occlusion checks if our data is not ready or has been invalidated
    if (occlusionData.m_occlusionDataState == CPUOcclusionData::OcclusionDataState::OcclusionDataInvalid)
    {
        return 0;
    }

    const OcclusionReadbackData& readbackData = occlusionData.m_occlusionReadbackData;

    // Copy the data that was prepared by the render thread for use with the Coverage Buffer system
    float* outputBuffer = reinterpret_cast<float*>(pOutOcclBuffer);
    memcpy(pOutOcclBuffer, readbackData.m_occlusionReadbackBuffer, s_occlusionBufferNumElements * sizeof(float));
    
    *pmCamBuffer = readbackData.m_occlusionReadbackViewProj;

    return 1;
}

bool IsDepthReadbackOcclusionEnabled()
{
    static ICVar* pCVCheckOcclusion = gEnv->pConsole->GetCVar("e_CheckOcclusion");
    static ICVar* pCVStatObjBufferRenderTasks = gEnv->pConsole->GetCVar("e_StatObjBufferRenderTasks");
    static ICVar* pCVCoverageBufferReproj = gEnv->pConsole->GetCVar("e_CoverageBufferReproj");
    if ((pCVCheckOcclusion && pCVCheckOcclusion->GetIVal() == 0) ||
        (pCVStatObjBufferRenderTasks && pCVStatObjBufferRenderTasks->GetIVal() == 0) ||
        (pCVCoverageBufferReproj && pCVCoverageBufferReproj->GetIVal() == 4))
    {
        return false;
    }

    return true;
}

void CD3D9Renderer::UpdateOcclusionDataForCPU()
{
    const bool bReverseDepth = (m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & RBPF_REVERSE_DEPTH) != 0;
    
    // Copy to CPU accessible memory
    m_occlusionData[m_occlusionBufferIndex].m_zTargetReadback->GetDevTexture()->DownloadToStagingResource(0);
    
    Matrix44 mCurView, mCurProj;
    mCurView.SetIdentity();
    mCurProj.SetIdentity();
    GetModelViewMatrix(reinterpret_cast<f32*>(&mCurView));
    GetProjectionMatrix(reinterpret_cast<f32*>(&mCurProj));
    
    if (bReverseDepth)
    {
        mCurProj = ReverseDepthHelper::Convert(mCurProj);
    }
    
    m_occlusionData[m_occlusionBufferIndex].m_occlusionViewProj = mCurView * mCurProj;
    m_occlusionData[m_occlusionBufferIndex].m_occlusionDataState = CPUOcclusionData::OcclusionDataState::OcclusionDataOnGPU;
    
    m_occlusionBufferIndex = (m_occlusionBufferIndex + 1) % s_numOcclusionReadbackTextures;
}

void CD3D9Renderer::FX_ZTargetReadBackOnCPU()
{
    PROFILE_LABEL_SCOPE("DEPTH READBACK CPU");
    PROFILE_FRAME(FX_ZTargetReadBackOnCPU);

#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
    // ZTarget read back is used for occlusion culling and we don't want to pollute the main pass 
    // occlusion buffer with the render to texture pass
    if (IsRenderToTextureActive())
    {
        return;
    }
#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED

    if (!IsDepthReadbackOcclusionEnabled() || (SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID] > 0))
    {
        return;
    }

    const bool isGmemEnabled = FX_GetEnabledGmemPath(nullptr) != CD3D9Renderer::eGT_REGULAR_PATH;
    if (isGmemEnabled)
    {
        //Since FX_ZTargetReadBack can not be run for gmem path we update the
        //occlusion data for m_occlusionData here.
        UpdateOcclusionDataForCPU();
    }

    static ICVar* pCVCoverageBufferLatency = gEnv->pConsole->GetCVar("e_CoverageBufferNumberFramesLatency");    
    int latency = pCVCoverageBufferLatency->GetIVal();

    // Readback index for the depth buffer in our ring buffer
    AZ::u8 occlusionReadbackIndex = 0;
    static_assert(s_numOcclusionReadbackTextures <= 3, "Maximum of 3 occlusion readback textures currently supported");
    switch (latency)
    {
    case 0:
        // Do not perform any CPU readback
        return;
        break;

    case 1:
        // Readback the depth buffer that was written to this frame (CPU will stall on GPU, useful for debugging)
        occlusionReadbackIndex = (m_occlusionBufferIndex + 2) % s_numOcclusionReadbackTextures;
        break;

    case 2:
        // Readback previous frame.
        occlusionReadbackIndex = (m_occlusionBufferIndex + 1) % s_numOcclusionReadbackTextures;
        break;

    case 3:
        // Readback oldest frame
        occlusionReadbackIndex = m_occlusionBufferIndex;
        break;
    }
    CPUOcclusionData& occlusionData = m_occlusionData[occlusionReadbackIndex];

    // Do not perform readback if our occlusion data is not ready to be read back
    if (occlusionData.m_occlusionDataState != CPUOcclusionData::OcclusionDataState::OcclusionDataOnGPU)
    {
        return;
    }

    const bool bUseNativeDepth = CRenderer::CV_r_CBufferUseNativeDepth && !gEnv->IsEditor();
    const bool bReverseDepth = (m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & RBPF_REVERSE_DEPTH) != 0;
    
    int nCameraID = -1;

    // In stereo rendering, we want the coverage buffer to be a merge of both rendered eyes. Otherwise one eye may
    // cull out the geometry visible be the other eye.
    bool mergePreviousBuffer = (GetS3DRend().GetStatus() == IStereoRenderer::Status::kRenderingSecondEye);

    // Read data from the prepared frame
    occlusionData.m_zTargetReadback->GetDevTexture()->AccessCurrStagingResource(0, false, [=, &nCameraID](void* pData, [[maybe_unused]] uint32 rowPitch, [[maybe_unused]] uint32 slicePitch)
    {
        float* pDepths = reinterpret_cast<float*>(pData);
        const CameraViewParameters& rc = GetViewParameters();
        float zn = rc.fNear;
        float zf = rc.fFar;
        const float ProjRatioX  = zf / (zf - zn);
        const float ProjRatioY  = zn / (zn - zf);
                
        OcclusionReadbackData& readbackData = m_occlusionData[occlusionReadbackIndex].m_occlusionReadbackData;
        readbackData.m_occlusionReadbackViewProj = m_occlusionData[occlusionReadbackIndex].m_occlusionViewProj;
        float* readBuffer = readbackData.m_occlusionReadbackBuffer;

        if (bUseNativeDepth)
        {
            float x = floorf(pDepths[0] * 0.5f); // Decode the ID from the first pixel
            readBuffer[0] = pDepths[0] - (x * 2.0f);
            nCameraID  = (int)(x);

            for (uint32 idx = 1; idx < s_occlusionBufferNumElements; idx++)
            {
                const float fDepthVal = bReverseDepth ? 1.0f - pDepths[idx] : pDepths[idx];
                if (mergePreviousBuffer)
                {
                    if (readBuffer[idx] == FLT_EPSILON)
                    {
                        readBuffer[idx] = max(fDepthVal, FLT_EPSILON);
                    }
                    else
                    {
                        float maxDepth = max(fDepthVal, readBuffer[idx]);
                        readBuffer[idx] = max(maxDepth, FLT_EPSILON);
                    }
                }
                else
                {
                    readBuffer[idx] = max(fDepthVal, FLT_EPSILON);
                }
            }
        }
        else
        {
            for (uint32 idx = 0; idx < s_occlusionBufferNumElements; idx++)
            {
                if (!mergePreviousBuffer)
                {
                    readBuffer[idx] = max(ProjRatioY / max(pDepths[idx], FLT_EPSILON) + ProjRatioX, FLT_EPSILON);
                }
                else
                {
                    if (readBuffer[idx] == FLT_EPSILON)
                    {
                        readBuffer[idx] = max(ProjRatioY / max(pDepths[idx], FLT_EPSILON) + ProjRatioX, FLT_EPSILON);
                    }
                    else
                    {
                        float newDepth = ProjRatioY / max(pDepths[idx], FLT_EPSILON) + ProjRatioX;
                        float maxDepth = max(newDepth, readBuffer[idx]);
                        readBuffer[idx] = max(maxDepth, FLT_EPSILON);
                    }
                }
            }
        }

        return true;
    });

    occlusionData.m_occlusionDataState = CPUOcclusionData::OcclusionDataState::OcclusionDataOnCPU;
    m_cpuOcclusionReadIndex = occlusionReadbackIndex;
}

void CD3D9Renderer::FX_ZTargetReadBack()
{
    PROFILE_LABEL_SCOPE("DEPTH READBACK GPU");
    PROFILE_FRAME(FX_ZTargetReadBack);

#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
    // do not pollute the main occlusion buffer with contents from the render to texture camera
    if (IsRenderToTextureActive())
    {
        return;
    }
#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED

    //Check for gmem as this code runs after gbuffer breaking gmem path. Besides we can just use
    //downsampled linearized depth for occlusion.
    const bool isGmemEnabled = FX_GetEnabledGmemPath(nullptr) != CD3D9Renderer::eGT_REGULAR_PATH;

    if (!IsDepthReadbackOcclusionEnabled() || isGmemEnabled)
    {
        return;
    }
    
    const bool bUseNativeDepth = CRenderer::CV_r_CBufferUseNativeDepth && !gEnv->IsEditor();
    const bool bReverseDepth = (m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & RBPF_REVERSE_DEPTH) != 0;
    int sourceWidth = CTexture::s_ptexZTarget->GetWidth();
    int sourceHeight = CTexture::s_ptexZTarget->GetHeight();

    if (sourceWidth != m_occlusionSourceSizeX || sourceHeight != m_occlusionSourceSizeY)
    {
        m_occlusionSourceSizeX = sourceWidth;
        m_occlusionSourceSizeY = sourceHeight;

        int downSampleX = max(0, 1 + IntegerLog2((uint16)((m_occlusionSourceSizeX * m_RP.m_CurDownscaleFactor.x) / s_occlusionBufferWidth)));
        int downSampleY = max(0, 1 + IntegerLog2((uint16)((m_occlusionSourceSizeY * m_RP.m_CurDownscaleFactor.y) / s_occlusionBufferHeight)));
        m_numOcclusionDownsampleStages = min(4, max(downSampleX, downSampleY));
        
        const uint32 nFlags = FT_DONT_STREAM | FT_DONT_RELEASE | FT_STAGE_READBACK;
        for (int downsampleStage = 0; downsampleStage < m_numOcclusionDownsampleStages; downsampleStage++)
        {
            const int width = s_occlusionBufferWidth << (m_numOcclusionDownsampleStages - downsampleStage - 1);
            const int height = s_occlusionBufferHeight << (m_numOcclusionDownsampleStages - downsampleStage - 1);

            if (CTexture::s_ptexZTargetDownSample[downsampleStage])
            {
                CTexture::s_ptexZTargetDownSample[downsampleStage]->m_nFlags = nFlags;
                CTexture::s_ptexZTargetDownSample[downsampleStage]->m_nWidth = width;
                CTexture::s_ptexZTargetDownSample[downsampleStage]->m_nHeight = height;

                CTexture::s_ptexZTargetDownSample[downsampleStage]->CreateRenderTarget(CTexture::s_eTFZ, Clr_FarPlane_R);
            }
            else
            {
                assert(CTexture::s_ptexZTargetDownSample[downsampleStage]);
            }
        }

        InvalidateCoverageBufferData();
    }

    // downsample on GPU
    RECT srcRect;
    srcRect.top = srcRect.left = 0;
    srcRect.right = LONG(CTexture::s_ptexZTargetDownSample[0]->GetWidth() * m_RP.m_CurDownscaleFactor.x);
    srcRect.bottom = LONG(CTexture::s_ptexZTargetDownSample[0]->GetHeight() * m_RP.m_CurDownscaleFactor.y);

    RECT* srcRegion = &srcRect;

    bool bMSAA = m_RP.m_MSAAData.Type ? true : false;

    D3DShaderResourceView* pZTargetOrigSRV = (D3DShaderResourceView*)     CTexture::s_ptexZTarget->GetShaderResourceView(bMSAA ? SResourceView::DefaultViewMS : SResourceView::DefaultView);
    if (bUseNativeDepth)
    {
        CTexture::s_ptexZTarget->SetShaderResourceView(m_pZBufferDepthReadOnlySRV, bMSAA);   // Read native depth, rather than linear. TODO: Check me, this may be slow on ATI MSAA

        int vpX, vpY, vpWidth, vpHeight;
        GetViewport(&vpX, &vpY, &vpWidth, &vpHeight);

        srcRect.right  = LONG(srcRect.right * vpWidth  / float(m_width));
        srcRect.bottom = LONG(srcRect.bottom * vpHeight / float(m_height));
    }
    else
    {
        bMSAA = false;
    }

    CTexture* pSrc = CTexture::s_ptexZTarget;
    CTexture* pDst = CTexture::s_ptexZTarget;

    bool bUseMSAA = bMSAA;
    const SPostEffectsUtils::EDepthDownsample downsampleMode = (bUseNativeDepth && bReverseDepth)
        ? SPostEffectsUtils::eDepthDownsample_Min
        : SPostEffectsUtils::eDepthDownsample_Max;

    for (int i = 0; i < m_numOcclusionDownsampleStages; i++)
    {
        pDst = CTexture::s_ptexZTargetDownSample[i];
        GetUtils().StretchRect(pSrc, pDst, false, false, false, false, downsampleMode, false, srcRegion);
        pSrc = pDst;
        srcRegion = NULL;
        bUseMSAA = false;
    }

    pSrc = pDst;
    pDst = m_occlusionData[m_occlusionBufferIndex].m_zTargetReadback;
    PostProcessUtils().StretchRect(pSrc, pDst, false, false, false, false, downsampleMode);

    //  Blend ID into top left pixel of readback buffer
    gcpRendD3D->FX_PushRenderTarget(0, pDst, NULL);
    gcpRendD3D->RT_SetViewport(0, 0, 1, 1);

    CShader* pSH = CShaderMan::s_ShaderCommon;
    uint32 nPasses = 0;
    pSH->FXSetTechnique("ClearUniform");
    pSH->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
    pSH->FXBeginPass(0);

    static CCryNameR pClearParams("vClearParam");
    m_RP.m_nZOcclusionBufferID = ((m_RP.m_nZOcclusionBufferID + 1) < CULLER_MAX_CAMS) ? (m_RP.m_nZOcclusionBufferID + 1) : 0;
    Vec4 vFrameID = Vec4((float)(m_RP.m_nZOcclusionBufferID * 2.0f), 0, 0, 0);
    pSH->FXSetPSFloat(pClearParams, &vFrameID, 1);

    FX_SetState(GS_NODEPTHTEST | GS_BLSRC_ONE | GS_BLDST_ONE);
    D3DSetCull(eCULL_None);
    float fX = (float)m_CurViewport.nWidth;
    float fY = (float)m_CurViewport.nHeight;
    ColorF col = Col_Black;
    DrawQuad(-0.5f, -0.5f, fX - 0.5f, fY - 0.5f, col, 1.0f, fX, fY, fX, fY);

    gcpRendD3D->FX_PopRenderTarget(0);
    gcpRendD3D->RT_SetViewport(0, 0, GetWidth(), GetHeight());
    
    if (bUseNativeDepth)
    {
        CTexture::s_ptexZTarget->SetShaderResourceView(pZTargetOrigSRV, bMSAA);
    }
    
    UpdateOcclusionDataForCPU();
}

void CD3D9Renderer::FX_UpdateCharCBs()
{
    PROFILE_FRAME(FX_UpdateCharCBs);
    AZ_TRACE_METHOD();
    unsigned poolId = (m_nPoolIndexRT) % 3;
    for (size_t boneType = 0; boneType < eBoneType_Count; ++boneType)
    {
        for (util::list<SCharInstCB>* iter = m_CharCBActiveList[boneType][poolId].next; iter != &m_CharCBActiveList[boneType][poolId]; iter = iter->next)
        {
            SCharInstCB* cb = iter->item<&SCharInstCB::list>();
            if (cb->updated)
            {
                continue;
            }
            SSkinningData* pSkinningData = cb->m_pSD;

            // make sure all sync jobs filling the buffers have finished
            if (pSkinningData->pAsyncJobExecutor)
            {
                PROFILE_FRAME(FX_UpdateCharCBs_ASYNC_WAIT);
                pSkinningData->pAsyncJobExecutor->WaitForCompletion();
            }

            if (pSkinningData->nHWSkinningFlags & eHWS_Skinning_Matrix)
            {
                AZ_Assert(boneType == eBoneType_Matrix, "Skinning type is Matrix but bone type is not.");
                cb->m_buffer->UpdateBuffer(pSkinningData->pBoneMatrices, pSkinningData->nNumBones * sizeof(Matrix34));
            }
            else
            {
                AZ_Assert(boneType == eBoneType_DualQuat, "Copying DualQuat buffer but bone type is not DualQuat.");
                cb->m_buffer->UpdateBuffer(pSkinningData->pBoneQuatsS, pSkinningData->nNumBones * sizeof(DualQuat));
            }

            cb->updated = true;
        }
    }
    // free a buffer each frame if we have an over-comittment of more than 75% compared
    // to our last 2 frames of rendering
    {
        int committed = CryInterlockedCompareExchange((LONG*)&m_CharCBAllocated, 0, 0);
        int totalRequested = m_CharCBFrameRequired[poolId] + m_CharCBFrameRequired[(poolId - 1) % 3];
        WriteLock _lock(m_lockCharCB);

        for (size_t boneType = 0; boneType < eBoneType_Count; ++boneType)
        {
            if (totalRequested * 4 > committed * 3 && m_CharCBFreeList[boneType].empty() == false)
            {
                delete m_CharCBFreeList[boneType].prev->item<&SCharInstCB::list>();
                CryInterlockedDecrement(&m_CharCBAllocated);
                break;
            }
        }
    }
}

void* CD3D9Renderer::FX_AllocateCharInstCB(SSkinningData* pSkinningData, uint32 frameId)
{
    PROFILE_FRAME(FX_AllocateCharInstCB);
    SCharInstCB* cb = NULL;

    size_t boneType = eBoneType_DualQuat;
    size_t boneSize = sizeof(DualQuat);

    if (pSkinningData->nHWSkinningFlags & eHWS_Skinning_Matrix)
    {
        boneType = eBoneType_Matrix;
        boneSize = sizeof(Matrix34);
    }

    {
        WriteLock _lock(m_lockCharCB);
        if (m_CharCBFreeList[boneType].empty() == false)
        {
            cb = m_CharCBFreeList[boneType].next->item<& SCharInstCB::list>();
            cb->list.erase();
        }
    }
    if (cb == NULL)
    {
        cb = new SCharInstCB();
        cb->m_buffer = gcpRendD3D->m_DevBufMan.CreateConstantBuffer(
            "SkinningBones",
            768 * boneSize,
            AzRHI::ConstantBufferUsage::Static);
        CryInterlockedIncrement(&m_CharCBAllocated);
    }
    cb->updated = false;
    cb->m_pSD = pSkinningData;
    {
        WriteLock _lock(m_lockCharCB);
        cb->list.relink_tail(&m_CharCBActiveList[boneType][frameId % 3]);
    }
    CryInterlockedIncrement(&m_CharCBFrameRequired[frameId % 3]);
    return cb;
}

void CD3D9Renderer::FX_ClearCharInstCB(uint32 frameId)
{
    PROFILE_FRAME(FX_ClearCharInstCB);
    uint32 poolId = frameId % 3;
    WriteLock _lock(m_lockCharCB);
    m_CharCBFrameRequired[poolId] = 0;

    for (size_t boneType = 0; boneType < eBoneType_Count; ++boneType)
    {
        m_CharCBFreeList[boneType].splice_tail(&m_CharCBActiveList[boneType][poolId]);
    }
}

// Render thread only scene rendering
void CD3D9Renderer::RT_RenderScene(int nFlags, SThreadInfo& TI, void(* RenderFunc)())
{
    // We first ensure that CRenderer::CV_r_EnableGMEMPath is only used for iOS or Android. Required
    // for when running in the editor and selecting the ios or android .cfg file settings. Only need
    // to worry about this in non-release builds as the default value is 0 and the editor is not built
    // in release builds.
#if (!defined (ANDROID) && !defined(IOS))
    CRenderer::CV_r_EnableGMEMPath = 0;
#endif
    // We first ensure that CRenderer::r_EnableComputeDownSampling is only used for iOS Metal
#if (!defined (CRY_USE_METAL) || !defined(IOS))
    CRenderer::CV_r_EnableComputeDownSampling = 0;
#endif

    int const nCurrentRecurseLvl = SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID];

    PROFILE_LABEL_SCOPE(nCurrentRecurseLvl == 0 ? "SCENE" : "SCENE_REC");

    gcpRendD3D->SetCurDownscaleFactor(gcpRendD3D->m_CurViewportScale);

    // Skip scene rendering when device is lost
    if (m_bDeviceLost)
    {
        return;
    }

    SThreadInfo* const pShaderThreadInfo = &(m_RP.m_TI[m_RP.m_nProcessThreadID]);

    ////////////////////////////////////////////////
    // to non-thread safe remaing work for *::Render functions
    {
        PROFILE_FRAME(WaitForRendItems);
        m_finalizeRendItemsJobExecutor[m_RP.m_nProcessThreadID].WaitForCompletion();
        m_finalizeShadowRendItemsJobExecutor[m_RP.m_nProcessThreadID].WaitForCompletion();
    }

    CRenderMesh::FinalizeRendItems(m_RP.m_nProcessThreadID);
    CMotionBlur::InsertNewElements();
    FurBendData::Get().InsertNewElements();

    {
        PROFILE_LABEL_SCOPE("UpdateModifiedMeshes");
        CRenderMesh::UpdateModified();
    }

    // Once per frame, notify that the start of the render thread scene rendering has begun.
    if (nCurrentRecurseLvl == 0)
    {
        AZ::RenderThreadEventsBus::Broadcast(&AZ::RenderThreadEventsBus::Events::OnRenderThreadRenderSceneBegin);
    }

    ////////////////////////////////////////////////
#ifdef CRY_INTEGRATE_DX12
    GetGraphicsPipeline().Prepare();

    // Make sure all dirty device resource sets are rebuild.
    PerFrameValidateResourceSets();
#endif
    ////////////////////////////////////////////////

    const int recursiveLevel = SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID];
    const int currentFrameId = GetFrameID(false);
    CRenderView& currentView = *m_RP.m_pRenderViews[m_RP.m_nProcessThreadID];

    // set to use RenderList Description
    m_RP.m_pRLD = &m_RP.m_pRenderViews[m_RP.m_nProcessThreadID]->m_RenderListDesc[recursiveLevel];

    CTimeValue Time = iTimer->GetAsyncTime();

#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
    if (!recursiveLevel && !IsRenderToTextureActive())
#else
    if (!recursiveLevel)
#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
    {
        m_MainViewport.nX = 0;
        m_MainViewport.nY = 0;
        m_MainViewport.nWidth = m_width;
        m_MainViewport.nHeight = m_height;
    }

    // invalidate object pointers
    m_RP.m_pCurObject = m_RP.m_pPrevObject = m_RP.m_pIdendityRenderObject;

    RT_UpdateLightVolumes(nFlags, recursiveLevel);

    // Wait for shadow jobs before building constant buffers.
    {
        PROFILE_FRAME(WaitForShadowRendItems);
        m_finalizeShadowRendItemsJobExecutor[m_RP.m_nProcessThreadID].WaitForCompletion();
    }

    // Precompile constant buffers for the frame.
    {
        GetPerInstanceConstantBufferPool().Update(currentView, TI.m_RealTime);

        FX_UpdateCharCBs();

        CHWShader_D3D::UpdatePerFrameResourceGroup();
    }

    //
    // Process Shadow Maps
    //
    if (!recursiveLevel && !(nFlags & SHDF_ZPASS_ONLY))
    {
        if (nFlags & SHDF_NO_SHADOWGEN)
        {
            m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags |= RBPF_NO_SHADOWGEN;
        }
        else
        {
            m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags &= ~RBPF_NO_SHADOWGEN;
        }

        PROFILE_LABEL_SCOPE("SHADOWMAP PASSES");
        PROFILE_PS_TIME_SCOPE(fTimeDIPs[EFSLIST_SHADOW_GEN]);
        EF_PrepareAllDepthMaps();
    }

    if (FX_GetEnabledGmemPath(nullptr))
    {
        FX_GmemTransition(eGT_PRE_Z);
    }

    int nSaveDrawNear = CV_r_nodrawnear;
    int nSaveStreamSync = CV_r_texturesstreamingsync;
    if (nFlags & SHDF_NO_DRAWNEAR)
    {
        CV_r_nodrawnear = 1;
    }
    if (nFlags & SHDF_STREAM_SYNC)
    {
        CV_r_texturesstreamingsync = 1;
    }

    m_bDeferredDecals = false;
    uint32 nSaveRendFlags = m_RP.m_nRendFlags;
    m_RP.m_nRendFlags = nFlags;
    FX_ApplyThreadState(TI, &m_RP.m_OldTI[recursiveLevel]);

    //
    // VR Tracking updates
    //

    if (m_pStereoRenderer->IsRenderingToHMD())
    {
        if (gRenDev->m_CurRenderEye == STEREO_EYE_LEFT)
        {
            /*
                Update tracking states for VR:
                For OpenVR we need to tell the compositor (SteamVR) to retrieve up to date tracking info.
                This is a blocking call that will only return when the compositor allows us
                Calling this here allows the GPU work submitted above to get a head start while
                we wait for the compositor to free us.

                This only need to be done once per frame but must be done on the render thread.
                This cannot be done on the main thread or a job/side thread or else it will
                cause tracking to de-sync from rendering causing all frames to render with out
                of date tracking. Updating tracking here significantly reduces GPU bubbles.

                For Oculus, PSVR etc this is still the best place to request a tracking
                update in a multi-threaded scenario. It ensures that any prediction will be done
                for this frame that we want to render rather than the next frame.
            */
            RT_UpdateTrackingStates();
        }

        /*
            After tracking has updated we want to override the camera with the
            correct tracking information. If this is the Right eye's pass we don't
            need to update tracking info but we do need to set the correct camera.
        */
        RT_SetStereoCamera();
    }

    bool bHDRRendering = (nFlags & SHDF_ALLOWHDR) && IsHDRModeEnabled();
    //The HDR pass is in charge of doing the SRGB conversion. Since is
    //disabled, we need to push a render target to do the SRGB conversion before the post process.
    bool doSRGBConversionCopy = !IsHDRModeEnabled() && (m_RP.m_nRendFlags & SHDF_ALLOWHDR) &&
                                !recursiveLevel && !m_wireframe_mode && 
                                !FX_GetEnabledGmemPath(nullptr);

    if (!recursiveLevel && bHDRRendering)
    {
        m_RP.m_bUseHDR = true;
        if (FX_HDRScene(m_RP.m_bUseHDR, false))
        {
            m_RP.m_PersFlags2 |= RBPF2_HDR_FP16;
        }
    }
    else
    {
        m_RP.m_bUseHDR = false;
        FX_HDRScene(false);

        if ((pShaderThreadInfo->m_PersFlags & RBPF_DRAWTOTEXTURE) && bHDRRendering)
        {
            m_RP.m_PersFlags2 |= RBPF2_HDR_FP16;
        }
        else
        {
            m_RP.m_PersFlags2 &= ~RBPF2_HDR_FP16;
        }
    }

    if (doSRGBConversionCopy)
    {
        FX_PushRenderTarget(0, CTexture::s_ptexHDRTarget, &m_DepthBufferOrigMSAA, -1, true);
    }

    // Prepare post processing
    bool bAllowPostProcess = (nFlags & SHDF_ALLOWPOSTPROCESS) && !recursiveLevel && (CV_r_PostProcess) && !CV_r_measureoverdraw &&
        !(pShaderThreadInfo->m_PersFlags & (RBPF_SHADOWGEN));

    bool bAllowSubpixelShift = bAllowPostProcess
        && (gcpRendD3D->FX_GetAntialiasingType() & eAT_JITTER_MASK)
        && (!gEnv->IsEditing() || CRenderer::CV_r_AntialiasingModeEditor)
        && (GetWireframeMode() == R_SOLID_MODE)
#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
        && !IsRenderToTextureActive()
#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
        && (CRenderer::CV_r_DeferredShadingDebugGBuffer == 0);

    m_TemporalJitterClipSpace = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
    m_TemporalJitterMipBias = 0.0f;
    if (bAllowSubpixelShift)
    {
        SubpixelJitter::Sample sample = SubpixelJitter::EvaluateSample(SPostEffectsUtils::m_iFrameCounter, (SubpixelJitter::Pattern)CV_r_AntialiasingTAAJitterPattern);

        m_TemporalJitterClipSpace.x = ((sample.m_subpixelOffset.x * 2.0) / (float)m_width)  / m_RP.m_CurDownscaleFactor.x;
        m_TemporalJitterClipSpace.y = ((sample.m_subpixelOffset.y * 2.0) / (float)m_height) / m_RP.m_CurDownscaleFactor.y;
        m_TemporalJitterClipSpace.z = sample.m_subpixelOffset.x;
        m_TemporalJitterClipSpace.w = sample.m_subpixelOffset.y;

        if IsCVarConstAccess(constexpr) (CV_r_AntialiasingTAAUseJitterMipBias)
        {
            m_TemporalJitterMipBias = sample.m_mipBias;
        }
    }

    FX_PostProcessScene(bAllowPostProcess);
    bool bAllowDeferred = (nFlags & SHDF_ZPASS) && !recursiveLevel && !CV_r_measureoverdraw;
    if (bAllowDeferred)
    {
        PROFILE_PS_TIME_SCOPE(fTimeDIPs[EFSLIST_DEFERRED_PREPROCESS]);
        m_RP.m_PersFlags2 |= RBPF2_ALLOW_DEFERREDSHADING;
        FX_DeferredRendering(false, true);
    }
    else
    {
        m_RP.m_PersFlags2 &= ~RBPF2_ALLOW_DEFERREDSHADING;
    }

    {
        if (!recursiveLevel && (nFlags & SHDF_ALLOWHDR))
        {
            ETEX_Format eTF = (m_RP.m_bUseHDR && m_nHDRType == 1) ? eTF_R16G16B16A16F : eTF_R8G8B8A8;
            int nW = gcpRendD3D->GetWidth(); //m_d3dsdBackBuffem.Width;
            int nH = gcpRendD3D->GetHeight(); //m_d3dsdBackBuffem.Height;
            if (!CTexture::s_ptexSceneTarget || CTexture::s_ptexSceneTarget->GetDstFormat() != eTF || CTexture::s_ptexSceneTarget->GetWidth() != nW || CTexture::s_ptexSceneTarget->GetHeight() != nH)
            {
                CTexture::GenerateSceneMap(eTF);
            }
        }
    }

    if ((nFlags & SHDF_ALLOWPOSTPROCESS) && !recursiveLevel)
    {
        FX_DeferredRainPreprocess();
    }

    if (!(nFlags & SHDF_ZPASS_ONLY))
    {
        bool bLighting = (pShaderThreadInfo->m_PersFlags & RBPF_SHADOWGEN) == 0;
        if (!nFlags)
        {
            bLighting = false;
        }

        if ((nFlags & (SHDF_ALLOWHDR | SHDF_ALLOWPOSTPROCESS)) && CV_r_usezpass)
        {
            PROFILE_PS_TIME_SCOPE(fTimeDIPsZ);
            FX_ProcessZPassRenderLists();

            FX_DeferredRainGBuffer();
            FX_DeferredSnowLayer();

            const bool takingScreenShot = (m_screenShotType != 0);
            const bool bMotionVectorsEnabled = (CRenderer::CV_r_MotionBlur > 1 || (gRenDev->FX_GetAntialiasingType() & eAT_TEMPORAL_MASK) != 0) && CRenderer::CV_r_MotionVectors && (!takingScreenShot || CRenderer::CV_r_MotionBlurScreenShot);
            if (bMotionVectorsEnabled)
            {
                CMotionBlur* motionBlur = static_cast<CMotionBlur*>(PostEffectMgr()->GetEffect(ePFX_eMotionBlur));
                motionBlur->RenderObjectsVelocity();
            }

            // Restore per-batch sorting after zpass finished
            if (m_bUseGPUFriendlyBatching[m_RP.m_nProcessThreadID] && CRenderer::CV_r_ZPassDepthSorting)
            {
                for (int i = 0; i < MAX_LIST_ORDER; ++i)
                {
                    EF_SortRenderList(EFSLIST_GENERAL, i, m_RP.m_pRLD, m_RP.m_nProcessThreadID, false);
                }
            }
        }

#if defined(FEATURE_SVO_GI)
        if ((gEnv->pConsole->GetCVar("e_GI")->GetIVal()) && (nFlags & SHDF_ALLOWHDR) && !recursiveLevel && CSvoRenderer::GetInstance())
        {
            PROFILE_LABEL_SCOPE("SVOGI");
            CSvoRenderer::GetInstance()->Lock();
            CSvoRenderer::GetInstance()->UpdateCompute();
            CSvoRenderer::GetInstance()->UpdateRender();
            CSvoRenderer::GetInstance()->Unlock();
        }
#endif

        bool bEmpty = SRendItem::IsListEmpty(EFSLIST_GENERAL, m_RP.m_nProcessThreadID, m_RP.m_pRLD);
        bEmpty &= SRendItem::IsListEmpty(EFSLIST_DEFERRED_PREPROCESS, m_RP.m_nProcessThreadID, m_RP.m_pRLD);
        if (!recursiveLevel && !bEmpty && pShaderThreadInfo->m_FS.m_bEnable && CV_r_usezpass)
        {
            m_RP.m_PersFlags2 |= RBPF2_NOSHADERFOG;
        }

        if (bAllowDeferred && !bEmpty)
        {
            PROFILE_LABEL_SCOPE("DEFERRED_LIGHTING");
            PROFILE_PS_TIME_SCOPE(fTimeDIPs[EFSLIST_DEFERRED_PREPROCESS]);

            FX_ProcessRenderList(EFSLIST_DEFERRED_PREPROCESS, BEFORE_WATER, RenderFunc, false);       // Sorted list without preprocess of all deferred related passes and screen shaders
            FX_ProcessRenderList(EFSLIST_DEFERRED_PREPROCESS, AFTER_WATER , RenderFunc, false);       // Sorted list without preprocess of all deferred related passes and screen shaders
        }

        if (FX_GetEnabledGmemPath(nullptr))
        {
            FX_GmemTransition(eGT_POST_DEFERRED_PRE_FORWARD);
        }

        if (nCurrentRecurseLvl == 0 && FurPasses::GetInstance().GetFurRenderingMode() == FurPasses::RenderMode::AlphaTested)
        {
            // If using alpha tested fur, perform shell prepass before forward opaque
            FurPasses::GetInstance().ExecuteShellPrepass();
        }

        FX_RenderForwardOpaque(RenderFunc, bLighting, bAllowDeferred);

        FX_ProcessThicknessRenderLists();

        const bool bDeferredScenePasses = (nFlags & SHDF_ALLOWPOSTPROCESS) && !recursiveLevel && !bEmpty;
        if (bDeferredScenePasses)
        {
            FX_ResetPipe();

            FX_DeferredCaustics();
        }

        const bool bShadowGenSpritePasses = (pShaderThreadInfo->m_PersFlags & (RBPF_SHADOWGEN)) != 0;

        //Include this profile segment in the summary information for the quick GPU profiling display
        {
            PROFILE_LABEL_SCOPE(nCurrentRecurseLvl == 0 ? "TRANSPARENT_PASSES" : "TRANSPARENT_PASSES_REC");

            if (!FX_GetEnabledGmemPath(nullptr) && bAllowDeferred && bDeferredScenePasses)
            {
                // make sure all all jobs which are computing particle vertices/indices
                // have finished and their vertex/index buffers are unlocked
                // before starting rendering of those
                m_ComputeVerticesJobExecutors[m_RP.m_nProcessThreadID].WaitForCompletion();
                UnLockParticleVideoMemory(gRenDev->m_nPoolIndexRT % SRenderPipeline::nNumParticleVertexIndexBuffer);


                PROFILE_LABEL_SCOPE("VOLUMETRIC FOG");

                GetVolumetricFog().RenderVolumetricsToVolume(RenderFunc);
                GetVolumetricFog().RenderVolumetricFog();
            }

            if (bDeferredScenePasses && CV_r_measureoverdraw != 4)
            {
                FX_RenderFog();
            }

            if (nCurrentRecurseLvl == 0 && FurPasses::GetInstance().GetFurRenderingMode() == FurPasses::RenderMode::AlphaBlended)
            {
                // If using alpha blended fur, perform shell prepass after fog passes so that fog can influence fur shading
                FurPasses::GetInstance().ExecuteShellPrepass();
            }

            {
                PROFILE_LABEL_SCOPE("TRANSPARENT_BW");
                PROFILE_PS_TIME_SCOPE_COND(fTimeDIPs[EFSLIST_TRANSP], !bShadowGenSpritePasses);

                GetTiledShading().BindForwardShadingResources(NULL);

                FX_ProcessRenderList(EFSLIST_TRANSP, BEFORE_WATER, RenderFunc, bLighting); // Unsorted list

                // Highest quality we need to render twice for accuracy under water.\
                // So we render first, here,  all the transparent fragments that are under water.  
                // The fragments above water that are rendered here will be discarded when we render the refractive water surface later in the pipeline.
                // This has a huge cost, need optimization.
                if (nFlags & SHDF_ALLOW_WATER)
                {
                    const ICVar* renderTransparentUnderWaterCVar = iConsole->GetCVar("e_RenderTransparentUnderWater");
                    int renderTransparentUnderWater = renderTransparentUnderWaterCVar ? renderTransparentUnderWaterCVar->GetIVal() : 0;
                    if (renderTransparentUnderWater == 1)
                    {
                        FX_ProcessRenderList(EFSLIST_TRANSP, AFTER_WATER, RenderFunc, bLighting); // Unsorted list
                    }
                }

                GetTiledShading().UnbindForwardShadingResources();
            }

            if (nFlags & SHDF_ALLOW_WATER)
            {
                {
                    PROFILE_LABEL_SCOPE("WATER_VOLUME");
                    PROFILE_PS_TIME_SCOPE_COND(fTimeDIPs[EFSLIST_WATER_VOLUMES], !bShadowGenSpritePasses);
                    FX_ProcessRenderList(EFSLIST_WATER_VOLUMES, BEFORE_WATER, RenderFunc, false);    // Sorted list without preprocess
                }

                FX_RenderWater(RenderFunc);
            }

            {
                PROFILE_LABEL_SCOPE("TRANSPARENT_AW");
                PROFILE_PS_TIME_SCOPE_COND(fTimeDIPs[EFSLIST_TRANSP], !bShadowGenSpritePasses);

                if (bAllowPostProcess && CV_r_TranspDepthFixup)
                {
                    FX_DepthFixupPrepare();
                }

                GetTiledShading().BindForwardShadingResources(NULL);

                MultiLayerAlphaBlendPass::GetInstance().SetLayerCount(CD3D9Renderer::CV_r_AlphaBlendLayerCount);
                MultiLayerAlphaBlendPass::GetInstance().BindResources();

                //draw after water transparent list. exclude objects which skip depth of field (only particles enabled this option, but it
                //could be applied to any transparent object later)
                uint32 nBatchExcludeFilter = 0;
                if (m_RP.m_bUseHDR)
                {
                    nBatchExcludeFilter = FB_TRANSPARENT_AFTER_DOF;
                }
                FX_ProcessRenderList(EFSLIST_TRANSP, AFTER_WATER, RenderFunc, true, FB_GENERAL, nBatchExcludeFilter);

                MultiLayerAlphaBlendPass::GetInstance().UnBindResources();
                GetTiledShading().UnbindForwardShadingResources();

                MultiLayerAlphaBlendPass::GetInstance().Resolve(*this);

                if (bAllowPostProcess && CV_r_TranspDepthFixup)
                {
                    FX_DepthFixupMerge();
                }
            }

            FX_ProcessHalfResParticlesRenderList(EFSLIST_HALFRES_PARTICLES, RenderFunc, bLighting);

            if (nFlags & SHDF_ALLOW_WATER)
            {
                PROFILE_LABEL_SCOPE("WATER_VOLUME");
                PROFILE_PS_TIME_SCOPE_COND(fTimeDIPs[EFSLIST_WATER_VOLUMES], !bShadowGenSpritePasses);
                FX_ProcessRenderList(EFSLIST_WATER_VOLUMES, AFTER_WATER, RenderFunc, false);
            }

            // insert fence which is used on consoles to prevent overwriting VideoMemory
            InsertParticleVideoMemoryFence(gRenDev->m_nPoolIndexRT % SRenderPipeline::nNumParticleVertexIndexBuffer);
        }

#if defined(ENABLE_ART_RT_TIME_ESTIMATE)
        m_RP.m_PS[m_RP.m_nProcessThreadID].m_actualRenderTimeMinusPost += iTimer->GetAsyncTime().GetDifferenceInSeconds(Time);
#endif

        PROFILE_PS_TIME_SCOPE_COND(fTimeDIPs[EFSLIST_POSTPROCESS], !bShadowGenSpritePasses);

        if (bAllowDeferred && !recursiveLevel)
        {
            FX_DeferredSnowDisplacement();
        }

        if (FX_GetEnabledGmemPath(nullptr))
        {
            FX_GmemTransition(eGT_POST_AW_TRANS_PRE_POSTFX);
        }

        if (!recursiveLevel)
        {
            gcpRendD3D->m_RP.m_PersFlags1 &= ~RBPF1_SKIP_AFTER_POST_PROCESS;

            FX_ProcessRenderList(EFSLIST_HDRPOSTPROCESS, BEFORE_WATER, RenderFunc, false);       // Sorted list without preprocess of all fog passes and screen shaders
            FX_ProcessRenderList(EFSLIST_HDRPOSTPROCESS, AFTER_WATER , RenderFunc, false);       // Sorted list without preprocess of all fog passes and screen shaders
            if (doSRGBConversionCopy)
            {
                //The HDR pass is in charge of doing the SRGB conversion but it's disabled.
                FX_SRGBConversion();
            }
            FX_ProcessRenderList(EFSLIST_AFTER_HDRPOSTPROCESS, BEFORE_WATER, RenderFunc, false); // for specific cases where rendering after tone mapping is needed
            FX_ProcessRenderList(EFSLIST_AFTER_HDRPOSTPROCESS, AFTER_WATER , RenderFunc, false);
            FX_ProcessRenderList(EFSLIST_POSTPROCESS, BEFORE_WATER, RenderFunc, false);       // Sorted list without preprocess of all fog passes and screen shaders
            FX_ProcessRenderList(EFSLIST_POSTPROCESS, AFTER_WATER , RenderFunc, false);       // Sorted list without preprocess of all fog passes and screen shaders

#if defined(CRY_USE_METAL) || defined(ANDROID)
            //  If need upscale do it here.
            {
                const Vec2& vDownscaleFactor = gcpRendD3D->m_RP.m_CurDownscaleFactor;

                bool bDoUpscale = (vDownscaleFactor.x < .999999f) || (vDownscaleFactor.y < .999999f);

                if (bDoUpscale)
                {
                    PROFILE_LABEL_SCOPE("RT_UPSCALE");
                    CTexture* pCurrRT = CTexture::s_ptexSceneDiffuse;
                    GetUtils().CopyScreenToTexture(pCurrRT);

                    //  copy osm-guaided viewport rect. It will be destroyed soon.
                    RECT rcSrcRegion = gcpRendD3D->m_FullResRect;
                    //  Since now we render to a full RT.
                    gcpRendD3D->SetCurDownscaleFactor(Vec2(1, 1));
                    gcpRendD3D->RT_SetViewport(0, 0, gcpRendD3D->GetWidth(), gcpRendD3D->GetHeight());

                    SD3DPostEffectsUtils::GetInstance().CopyTextureToScreen(pCurrRT, &rcSrcRegion, FILTER_BILINEAR);
                }
            }
#endif
            bool bDrawAfterPostProcess = !(gcpRendD3D->m_RP.m_PersFlags1 & RBPF1_SKIP_AFTER_POST_PROCESS);

            RT_SetViewport(0, 0, GetWidth(), GetHeight());

            if (bDrawAfterPostProcess)
            {
                PROFILE_LABEL_SCOPE("AFTER_POSTPROCESS"); // for specific cases where rendering after all post effects is needed
                FX_ProcessRenderList(EFSLIST_AFTER_POSTPROCESS, BEFORE_WATER, RenderFunc, false);
                FX_ProcessRenderList(EFSLIST_AFTER_POSTPROCESS, AFTER_WATER , RenderFunc, false);
            }

            gcpRendD3D->m_RP.m_PersFlags2 &= ~RBPF2_NOPOSTAA;

            if IsCVarConstAccess(constexpr) (CV_r_DeferredShadingDebug && bAllowDeferred)
            {
                FX_DeferredRendering(true);
            }
        }
    }
    else
    {
        FX_ProcessRenderList(EFSLIST_GENERAL, BEFORE_WATER, RenderFunc, true);    // Sorted list without preprocess
        FX_ProcessRenderList(EFSLIST_DECAL, BEFORE_WATER, RenderFunc, true);    // Sorted list without preprocess
        FX_ProcessRenderList(EFSLIST_WATER_VOLUMES, BEFORE_WATER, RenderFunc, false);    // Sorted list without preprocess

        FX_ProcessRenderList(EFSLIST_GENERAL, AFTER_WATER, RenderFunc, true);    // Sorted list without preprocess
        FX_ProcessRenderList(EFSLIST_DECAL, AFTER_WATER, RenderFunc, true);    // Sorted list without preprocess
        FX_ProcessRenderList(EFSLIST_WATER_VOLUMES, AFTER_WATER, RenderFunc, false);    // Sorted list without preprocess
    }

    // Readback the downsampled z-buffer to the CPU for use by the Coverage Buffer system next frame.
    // This is performed at the end of the frame in order to help prevent a CPU/GPU sync point.
    FX_ZTargetReadBackOnCPU();

    FX_ApplyThreadState(m_RP.m_OldTI[recursiveLevel], NULL);

    m_RP.m_PS[m_RP.m_nProcessThreadID].m_fRenderTime += iTimer->GetAsyncTime().GetDifferenceInSeconds(Time);

    m_RP.m_nRendFlags = nSaveRendFlags;
    CV_r_nodrawnear = nSaveDrawNear;
    CV_r_texturesstreamingsync = nSaveStreamSync;
}

//======================================================================================================
// Process all render item lists (can be called recursively)
void CD3D9Renderer::EF_ProcessRenderLists(RenderFunc pRenderFunc, int nFlags, [[maybe_unused]] SViewport& VP, const SRenderingPassInfo& passInfo, bool bSync3DEngineJobs)
{
    AZ_TRACE_METHOD();
    ASSERT_IS_MAIN_THREAD(m_pRT)
    int nThreadID = passInfo.ThreadID();
    int nR = passInfo.GetRecursiveLevel();
#ifndef _RELEASE
    if (nR < 0)
    {
        __debugbreak();
    }
#endif

    bool bIsMultiThreadedRenderer = false;
    EF_Query(EFQ_RenderMultithreaded, bIsMultiThreadedRenderer);
    if (!nR)
    {
        if (bSync3DEngineJobs)
        {
            // wait for all RendItems which need preprocession
            // note: the PopCompletionFence here indicates that no new jobs for preprocessing are spawned
            // note: must be called before EndSpawningGeneratingRendItemJobs! in all constellations, else a race condition can uncoalesce the underlying memory
            AZ::LegacyJobExecutor* pJobExecutor = gEnv->pRenderer->GetGenerateRendItemJobExecutorPreProcess();
            if (pJobExecutor->IsRunning())
            {
                pJobExecutor->PopCompletionFence();
            }
            pJobExecutor->WaitForCompletion();

            // we need to prepare the render item lists here when we are using the editor(which doesn't have MT rendering)
            if (!bIsMultiThreadedRenderer)
            {
                if (m_generateRendItemJobExecutor.IsRunning())
                {
                    m_generateRendItemJobExecutor.PopCompletionFence();
                }

                if (m_generateShadowRendItemJobExecutor.IsRunning())
                {
                    m_generateShadowRendItemJobExecutor.PopCompletionFence();
                }

                ////////////////////////////////////////////////
                // wait till all SRendItems for this frame have finished preparing
                m_finalizeRendItemsJobExecutor[m_RP.m_nProcessThreadID].WaitForCompletion();
                m_finalizeShadowRendItemsJobExecutor[m_RP.m_nProcessThreadID].WaitForCompletion();
                gRenDev->GetGenerateRendItemJobExecutor()->ClearPostJob(); // clear post job to prevent invoking it twice when no MT Rendering is enabled, but recursive rendering is used
            }
        }

        assert(nThreadID == m_RP.m_nFillThreadID); // make sure this is main thread
        assert(nThreadID < RT_COMMAND_BUF_COUNT);
        if ((nFlags & SHDF_ALLOWPOSTPROCESS))
        {
            SRenderListDesc tmpRLD;
            int nPreProcessLists[] = { EFSLIST_PREPROCESS, EFSLIST_WATER, EFSLIST_WATER_VOLUMES };
            for (int i = 0; i < 3; ++i)
            {
                int nList = nPreProcessLists[i];
                FinalizeRendItems_ReorderRendItemList(0, nList, nThreadID);
                FinalizeRendItems_ReorderRendItemList(1, nList, nThreadID);

                // make sure the memory is continous before sorting
                auto& RESTRICT_REFERENCE renderItems = CRenderView::CurrentFillView()->GetRenderItems(0, nList);
                renderItems.CoalesceMemory();

                tmpRLD.m_nStartRI[0][nList] = 0;
                tmpRLD.m_nEndRI[0][nList] = renderItems.size();
                tmpRLD.m_nBatchFlags[0][nList] = passInfo.GetRenderView()->GetBatchFlags(0, 0, nList);
                EF_SortRenderList(nList, 0, &tmpRLD, nThreadID, (CRenderer::CV_r_ZPassDepthSorting != 0));
            }

            int nums = tmpRLD.m_nStartRI[0][EFSLIST_PREPROCESS];
            int nume = tmpRLD.m_nEndRI[0][EFSLIST_PREPROCESS];

            // Perform pre-process operations for the current frame
            auto& RESTRICT_REFERENCE postProcessRenderItems = CRenderView::CurrentFillView()->GetRenderItems(0, EFSLIST_PREPROCESS);

            if (nume - nums > 0 && postProcessRenderItems[nums].nBatchFlags & FSPR_MASK)
            {
                nums += EF_Preprocess(&postProcessRenderItems[0], nums, nume, pRenderFunc, passInfo);
            }
        }
    }

    // since we need to sync earlier if we don't have multithreaded rendering
    // we need to finalize the rend items again in a possible recursive pass
    if (!bIsMultiThreadedRenderer && nR)
    {
        m_generateRendItemJobExecutor.WaitForCompletion();
        m_finalizeRendItemsJobExecutor[nThreadID].PushCompletionFence();
        CRenderer::FinalizeRendItems(nThreadID);
    }
    m_pRT->RC_RenderScene(nFlags, pRenderFunc);
}

void CD3D9Renderer::EF_RenderScene(int nFlags, SViewport& VP, const SRenderingPassInfo& passInfo)
{
    AZ_TRACE_METHOD();
    int nThreadID = passInfo.ThreadID();
    int nRecurseLevel = passInfo.GetRecursiveLevel();

    CTimeValue time0 = iTimer->GetAsyncTime();
#ifndef _RELEASE
    if (nRecurseLevel < 0)
    {
        __debugbreak();
    }
    if (CV_r_excludeshader->GetString()[0] != '0')
    {
        char nm[256];
        azstrcpy(nm, AZ_ARRAY_SIZE(nm), CV_r_excludeshader->GetString());
        azstrlwr(nm, AZ_ARRAY_SIZE(nm));
        m_RP.m_sExcludeShader = nm;
    }
    else
#endif
    m_RP.m_sExcludeShader = "";

    if (nFlags & SHDF_ALLOWPOSTPROCESS && gRenDev->m_CurRenderEye == 0)
    {
        EF_AddClientPolys(passInfo);
    }

    EF_ProcessRenderLists(FX_FlushShader_General, nFlags, VP, passInfo, true);

    EF_DrawDebugTools(VP, passInfo);

    m_RP.m_PS[nThreadID].m_fSceneTimeMT += iTimer->GetAsyncTime().GetDifferenceInSeconds(time0);
}


// Process all render item lists
void CD3D9Renderer::EF_EndEf3D(const int nFlags, const int nPrecacheUpdateIdSlow, const int nPrecacheUpdateIdFast, const SRenderingPassInfo& passInfo)
{
    AZ_TRACE_METHOD();
    ASSERT_IS_MAIN_THREAD(m_pRT)
    int nThreadID = m_RP.m_nFillThreadID;

    int32 nRecurseLevel = SRendItem::m_RecurseLevel[nThreadID];
    assert(nRecurseLevel >= 0);
    if (nRecurseLevel < 0)
    {
        iLog->Log("Error: CRenderer::EF_EndEf3D without CRenderer::EF_StartEf");
        return;
    }

    m_RP.m_TI[m_RP.m_nFillThreadID].m_arrZonesRoundId[0] = max(m_RP.m_TI[m_RP.m_nFillThreadID].m_arrZonesRoundId[0], nPrecacheUpdateIdFast);
    m_RP.m_TI[m_RP.m_nFillThreadID].m_arrZonesRoundId[1] = max(m_RP.m_TI[m_RP.m_nFillThreadID].m_arrZonesRoundId[1], nPrecacheUpdateIdSlow);

    m_p3DEngineCommon.Update(nThreadID);

    if IsCVarConstAccess(constexpr) (CV_r_nodrawshaders == 1)
    {
        EF_ClearTargetsLater(FRT_CLEAR, Clr_Transparent);
        if (SRendItem::m_RecurseLevel[nThreadID] == 0)
        {

            if(m_generateRendItemPreProcessJobExecutor.IsRunning())
            {
                m_generateRendItemPreProcessJobExecutor.PopCompletionFence();
            }

            bool bIsMultiThreadedRenderer = false;
            gEnv->pRenderer->EF_Query(EFQ_RenderMultithreaded, bIsMultiThreadedRenderer);
            // Because of EndSpawningGeneratingRendItemJobs we need to skip this one while in multi-threaded renderer.
            if (!bIsMultiThreadedRenderer && m_generateRendItemJobExecutor.IsRunning())
            {
                m_generateRendItemJobExecutor.PopCompletionFence();
            }

            // The m_generateShadowRendItemJobExecutor was started in EF_PrepareShadowGenRenderList, need to end it.
            if (m_generateShadowRendItemJobExecutor.IsRunning())
            {
                m_generateShadowRendItemJobExecutor.PopCompletionFence();
            }
        }
        SRendItem::m_RecurseLevel[nThreadID]--;
        return;
    }

    int nAsyncShaders = CV_r_shadersasynccompiling;
    if (nFlags & SHDF_NOASYNC)
    {
        AZ_Assert(gRenDev->m_pRT->IsRenderThread(), "EF_EndEf3D: SHDF_NOASYNC may only be used with r_multithreading disabled.  This is because the render thread modifies r_shadersasynccompiling and can lead to race conditions.");
        CV_r_shadersasynccompiling = 0;
    }

    if (SRendItem::m_RecurseLevel[nThreadID] == 0 && !(nFlags & (SHDF_ZPASS_ONLY | SHDF_NO_SHADOWGEN))) //|SHDF_ALLOWPOSTPROCESS
    {
        PrepareShadowGenForFrustumNonJobs(nFlags);
    }

    if (GetS3DRend().IsStereoEnabled())
    {
        GetS3DRend().ProcessScene(nFlags, passInfo);
    }
    else
    {
        EF_Scene3D(m_MainRTViewport, nFlags, passInfo);
    }

    DynArray<SDeferredDecal>& deferredDecals = m_RP.m_DeferredDecals[nThreadID][nRecurseLevel];
    //deferredDecals.SetUse(0);
    bool bIsMultiThreadedRenderer = false;
    EF_Query(EFQ_RenderMultithreaded, bIsMultiThreadedRenderer);
    if (bIsMultiThreadedRenderer && SRendItem::m_RecurseLevel[nThreadID] == 0 && !(nFlags & (SHDF_ZPASS_ONLY | SHDF_NO_SHADOWGEN))) //|SHDF_ALLOWPOSTPROCESS
    {
        m_generateShadowRendItemJobExecutor.PopCompletionFence();
    }

    SRendItem::m_RecurseLevel[nThreadID]--;

    // Do not restore this variable unless this condition is valid, otherwise it can cause a race condition.
    // This variable is accessed and modified from both the render and main thread, so it is only valid to touch this variable on this thread when r_multithreaded=0
    if (nFlags & SHDF_NOASYNC)
    {
        CV_r_shadersasynccompiling = nAsyncShaders;
    }
}

void CD3D9Renderer::EF_InvokeShadowMapRenderJobs([[maybe_unused]] const int nFlags)
{
    int nThreadID = m_RP.m_nFillThreadID;
    if (SRendItem::m_RecurseLevel[nThreadID] == 0)
    {
        EF_PrepareShadowGenRenderList();
    }
}

void CD3D9Renderer::EF_Scene3D(SViewport& VP, int nFlags, const SRenderingPassInfo& passInfo)
{
    ASSERT_IS_MAIN_THREAD(m_pRT)
    AZ_TRACE_METHOD();
    int nThreadID = m_RP.m_nFillThreadID;
    assert(nThreadID >= 0 && nThreadID < RT_COMMAND_BUF_COUNT);

    bool bFullScreen = true;
    SDynTexture* pDT = NULL;
    int recursiveLevel = SRendItem::m_RecurseLevel[nThreadID];
    assert(recursiveLevel >= 0 && recursiveLevel < MAX_REND_RECURSION_LEVELS);

    if (!recursiveLevel && m_pStereoRenderer->GetStatus() != IStereoRenderer::Status::kRenderingSecondEye && !CV_r_measureoverdraw)
    {
        bool bAllowDeferred = (nFlags & SHDF_ZPASS) != 0;
        if (bAllowDeferred)
        {
            gRenDev->m_cEF.mfRefreshSystemShader("DeferredShading", CShaderMan::s_shDeferredShading);

            SShaderItem shItem(CShaderMan::s_shDeferredShading);
            CRenderObject* pObj = EF_GetObject_Temp(passInfo.ThreadID());
            if (pObj)
            {
                pObj->m_II.m_Matrix.SetIdentity();
                EF_AddEf(m_RP.m_pREDeferredShading, shItem, pObj, passInfo, EFSLIST_DEFERRED_PREPROCESS, 0, SRendItemSorter::CreateDeferredPreProcessRendItemSorter(passInfo, SRendItemSorter::eDeferredShadingPass));
            }
        }

        if ((nFlags & SHDF_ALLOWHDR) && IsHDRModeEnabled())
        {
            SShaderItem shItem(CShaderMan::s_shHDRPostProcess);
            CRenderObject* pObj = EF_GetObject_Temp(passInfo.ThreadID());
            if (pObj)
            {
                pObj->m_II.m_Matrix.SetIdentity();
                SRendItemSorter rendItemSorter = SRendItemSorter::CreateRendItemSorter(passInfo);
                EF_AddEf(m_RP.m_pREHDR, shItem, pObj, passInfo, EFSLIST_HDRPOSTPROCESS, 0, rendItemSorter);
            }
        }

        bool bAllowPostProcess = (nFlags & SHDF_ALLOWPOSTPROCESS) && (CV_r_PostProcess);
        bAllowPostProcess &= (m_RP.m_TI[nThreadID].m_PersFlags & RBPF_MIRRORCULL) == 0;
        if (bAllowPostProcess)
        {
            SShaderItem shItem(CShaderMan::s_shPostEffects);
            CRenderObject* pObj = EF_GetObject_Temp(passInfo.ThreadID());
            if (pObj)
            {
                pObj->m_II.m_Matrix.SetIdentity();
                SRendItemSorter rendItemSorter = SRendItemSorter::CreateRendItemSorter(passInfo);
                EF_AddEf(m_RP.m_pREPostProcess, shItem, pObj, passInfo, EFSLIST_POSTPROCESS, 0, rendItemSorter);
            }
        }
    }

    // Update per-frame params
    UpdatePerFrameParameters();

    EF_RenderScene(nFlags, VP, passInfo);

    //Re-apply stereo camera here just so that all rendering is done based off of the correct camera instead of
    //whatever the camera is currently set to
    if (gcpRendD3D->GetIStereoRenderer()->IsRenderingToHMD())
    {
        gcpRendD3D->RT_SetStereoCamera();
    }

    if (!passInfo.IsRecursivePass())
    {
        //Draw these Debug systems as part of the scene so that they render properly in VR

#ifdef ENABLE_RENDER_AUX_GEOM
#ifndef _RELEASE

        //Draws all aux geometry
        GetIRenderAuxGeom()->Flush();

        //Actually flushes and clears out aux geometry buffers
        //We need to do this so that geometry is re-processed for VR.
        //This is because the aux geometry buffers overwrite themselves
        //as they draw. By clearing them out it means we can just re-process
        //that geometry for the 2nd eye and not draw a mangled vertex buffer.
        GetIRenderAuxGeom()->Process();
#endif //_RELEASE
#endif //ENABLE_RENDER_AUX_GEOM

        //Only render the UI Canvas and the Console on the main window
        //If we're not in the editor, don't bother to check viewport
        if (!gEnv->IsEditor() || m_CurrContext->m_bMainViewport)
        {
            EBUS_EVENT(AZ::RenderNotificationsBus, OnScene3DEnd);
        }

        // For VR rendering, RenderTextMessages need to be called in EF_Scene3D to render into both eyes,
        // Some 2D rendering calls such as console rendering were moved from CSystem::RenderEnd or EndFrame into EF_Scene3D to work with it.
        // In this case we have to RenderTextMessage immediately instead of push RenderTextMessage to render thread.
        // EF_RenderTextMessages will render text messages into actual draw2d commands.
        // For the remaining 2D renderings that are still called at the end of frame(such as C3DEngine::DisplayInfo in CSystem::RenderEnd),
        // they are called after the TextMessage have already been rendered, so they will be eventually rendered 2 frames later.
        // This is not an ideal situation and we should find a better way to handle it later.
        EF_RenderTextMessages();
    }
}

void CD3D9Renderer::RT_PrepareStereo(int mode, int output)
{
    m_pStereoRenderer->PrepareStereo((EStereoMode)mode, (EStereoOutput)output);
}

void CD3D9Renderer::RT_CopyToStereoTex(int channel)
{
    m_pStereoRenderer->CopyToStereo(channel);
}

void CD3D9Renderer::RT_UpdateTrackingStates()
{
    if (m_pStereoRenderer->IsRenderingToHMD())
    {
        // Only allow tracking info to update once per frame
        // Do not allow recursion "frame" ID otherwise we will update tracking for both the water reflection pass as well as the main scene pass
        static int lastFrameId = 0;
        int frameId = GetFrameID(false); 
        if (lastFrameId != frameId)
        {
            EBUS_EVENT(AZ::VR::HMDDeviceRequestBus, UpdateTrackingStates);
            lastFrameId = frameId;
        }
    }
}

void CD3D9Renderer::RT_DisplayStereo()
{
    m_pStereoRenderer->DisplayStereo();
}

void CD3D9Renderer::EnablePipelineProfiler([[maybe_unused]] bool bEnable)
{
#if defined(ENABLE_PROFILING_GPU_TIMERS)
    if (m_pPipelineProfiler)
    {
        m_pPipelineProfiler->SetEnabled(bEnable);
    }
#endif
}

//========================================================================================================
