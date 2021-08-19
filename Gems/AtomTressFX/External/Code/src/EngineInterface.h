// Copyright(c) 2019 Advanced Micro Devices, Inc.All rights reserved.
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#pragma once

#include "AMD_Types.h"
#include <vector>

// tressfx gpu interface implementation
class EI_Device;              // ID3D11Device
class EI_CommandContext;      // ID3D11DeviceContext
class EI_Resource;            // ID3D11Texture2D + UAV + SRV + RTV   or    ID3D11Buffer + UAV + SRV  (see details below)
class EI_LayoutManager;       // Anything you might use to assign shader slots.
struct EI_BindLayout;         // Array of resources to bind.
class EI_PSO;                 // Pipeline state (raster state, etc)
struct EI_RenderTargetSet;          // Render set

enum EI_ShaderStage
{
    EI_UNINITIALIZED = 0,  // we will always specify shader stage.  "all" is never used.
    EI_VS,
    EI_PS,
    EI_CS,
    EI_ALL
    };

enum EI_ResourceState
{
    EI_STATE_UNDEFINED,
    EI_STATE_SRV,
    EI_STATE_UAV,
    EI_STATE_COPY_DEST,
    EI_STATE_COPY_SOURCE,
    EI_STATE_RENDER_TARGET,
    EI_STATE_DEPTH_STENCIL,
    EI_STATE_INDEX_BUFFER,
    EI_STATE_CONSTANT_BUFFER,
};

enum EI_ResourceTypeEnum
{
    EI_RESOURCETYPE_UNDEFINED = 0,
    EI_RESOURCETYPE_BUFFER_RW = 0x01,
    EI_RESOURCETYPE_BUFFER_RO = 0x02,
    EI_RESOURCETYPE_IMAGE_RW = 0x03,
    EI_RESOURCETYPE_IMAGE_RO = 0x04,
    EI_RESOURCETYPE_UNIFORM = 0x05,
    EI_RESOURCETYPE_SAMPLER = 0x06,
};

struct EI_ResourceDescription {
    const char * name;
    int binding;
    EI_ResourceTypeEnum type;
};

struct EI_LayoutDescription
{
    std::vector<EI_ResourceDescription> resources;
    EI_ShaderStage stage;
};

struct EI_IndexedDrawParams
{
    EI_Resource* pIndexBuffer;
    int    numIndices;
    int    numInstances;
};

struct EI_DrawParams
{
    int    numVertices;
    int    numInstances;
};

struct EI_Barrier
{
    EI_Resource* pResource;
    EI_ResourceState from;
    EI_ResourceState to;
};

struct EI_BindSetDescription
{
    std::vector<EI_Resource *> resources;
};

// Add more pso control enums as necessary
enum class EI_CompareFunc
{
    Never = 0,
    Less,
    Equal,
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual,
    Always,
};

enum class EI_BlendOp
{
    Add = 0,
    Subtract,
    ReverseSubtract,
    Min,
    Max
};

enum class EI_StencilOp
{
    Keep = 0,
    Zero,
    Replace,
    IncrementClamp,
    DecrementClamp,
    Invert,
    IncrementWrap,
    DecrementWrap,
};

enum class EI_BlendFactor
{
    Zero = 0,
    One,
    SrcColor,
    InvSrcColor,    // 1 - SrcColor
    DstColor,
    InvDstColor,    // 1 - DstColor
    SrcAlpha,
    InvSrcAlpha,    // 1 - SrcAlpha
    DstAlpha,
    InvDstAlpha,    // 1 - DstAlpha
};

enum class EI_Topology
{
    TriangleList = 0,
    TriangleStrip,
};

struct EI_ColorBlendParams
{
    bool            colorBlendEnabled = false;
    EI_BlendOp      colorBlendOp = EI_BlendOp::Add;
    EI_BlendFactor  colorSrcBlend = EI_BlendFactor::One;
    EI_BlendFactor  colorDstBlend = EI_BlendFactor::Zero;
    EI_BlendOp      alphaBlendOp = EI_BlendOp::Add;
    EI_BlendFactor  alphaSrcBlend = EI_BlendFactor::Zero;
    EI_BlendFactor  alphaDstBlend = EI_BlendFactor::One;
};

// Add as needed
enum EI_LayoutState {
    Undefined = 0,
    RenderColor,
    RenderDepth,
    ReadOnly,
    Present,
};

enum EI_RenderPassFlags
{
    None = 0,
    Load = 0x01,
    Clear = 0x02,
    Store = 0x04,
    Depth = 0x08,
};

inline EI_RenderPassFlags operator| (EI_RenderPassFlags a, EI_RenderPassFlags b)
{
    return static_cast<EI_RenderPassFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

struct EI_AttachmentParams
{
    EI_RenderPassFlags  Flags = EI_RenderPassFlags::None;
};

enum class EI_ResourceType
{
    Undefined = 0,
    Buffer,
    Texture,
    Sampler,
};

enum class EI_Filter
{
    Point = 0,
    Linear,
    // Add more as needed
};

enum class EI_AddressMode
{
    Wrap = 0,
    ClampEdge,
    // Add more as needed
};

struct EI_PSOParams
{
    EI_Topology         primitiveTopology = EI_Topology::TriangleList;
    bool                colorWriteEnable = true;
    bool				depthTestEnable = false;
    bool				depthWriteEnable = false;
    EI_CompareFunc		depthCompareOp = EI_CompareFunc::Always;

    EI_ColorBlendParams colorBlendParams;

    bool                stencilTestEnable = false;
    EI_StencilOp        backFailOp = EI_StencilOp::Keep;
    EI_StencilOp        backPassOp = EI_StencilOp::Keep;
    EI_StencilOp        backDepthFailOp = EI_StencilOp::Keep;
    EI_CompareFunc      backCompareOp = EI_CompareFunc::Always;

    EI_StencilOp        frontFailOp = EI_StencilOp::Keep;
    EI_StencilOp        frontPassOp = EI_StencilOp::Keep;
    EI_StencilOp        frontDepthFailOp = EI_StencilOp::Keep;
    EI_CompareFunc      frontCompareOp = EI_CompareFunc::Always;

    uint32_t            stencilReadMask = 0x00;
    uint32_t            stencilWriteMask = 0x00;
    uint32_t            stencilReference = 0x00;

    EI_BindLayout**     layouts = nullptr;
    int					numLayouts = 0;

    EI_RenderTargetSet* renderTargetSet = nullptr;
};

enum EI_BindPoint {
    EI_BP_COMPUTE,
    EI_BP_GRAPHICS
};

enum EI_BufferFlags {
    EI_BF_NEEDSUAV = 1u << 0,
    EI_BF_NEEDSCPUMEMORY = 1u << 1,
    EI_BF_UNIFORMBUFFER = 1u << 2,
    EI_BF_VERTEXBUFFER = 1u << 3,
    EI_BF_INDEXBUFFER = 1u << 4
};

///////////////////////////////////////////////////////////////
// We use 4 kinds of resources.
//    2D RW buffer
//    Structured buffers.
//    Index buffer.
//    Constant buffers
//

#ifndef TRESSFX_ASSERT
#include <assert.h>
#define TRESSFX_ASSERT assert
#endif

#include <cstdio>

#define EI_Read(ptr, size, pFile) fread(ptr, size, 1, pFile)
#define EI_Seek(pFile, offset) fseek(pFile, offset, SEEK_SET)
#define EI_LogWarning(msg) printf("%s", msg)

#ifdef TRESSFX_VK
    #include "VK/VKEngineInterfaceImpl.h"
#else
    #include "DX12/DX12EngineInterfaceImpl.h"
#endif

#include "SceneGLTFImpl.h"