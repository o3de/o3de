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

#include "EngineInterface.h"

#include <DirectXMath.h>
using namespace DirectX;

#include <cassert>
#include "Base/ShaderCompilerHelper.h"
#include "Base/Helper.h"
#include "GLTF/GltfHelpers.h"
#include "GLTF/GltfCommon.h"
#include "GLTF/GLTFTexturesAndBuffers.h"
#include "GLTF/GltfPbrPass.h"
#include "GLTF/GltfDepthPass.h"
#include "TressFXLayouts.h"

// Inline operators for general state data
inline VkCompareOp operator* (EI_CompareFunc Enum)
{
    if (Enum == EI_CompareFunc::Never) return VK_COMPARE_OP_NEVER;
    if (Enum == EI_CompareFunc::Less) return VK_COMPARE_OP_LESS;
    if (Enum == EI_CompareFunc::Equal) return VK_COMPARE_OP_EQUAL;
    if (Enum == EI_CompareFunc::LessEqual) return VK_COMPARE_OP_LESS_OR_EQUAL;
    if (Enum == EI_CompareFunc::Greater) return VK_COMPARE_OP_GREATER;
    if (Enum == EI_CompareFunc::NotEqual) return VK_COMPARE_OP_NOT_EQUAL;
    if (Enum == EI_CompareFunc::GreaterEqual) return VK_COMPARE_OP_GREATER_OR_EQUAL;
    if (Enum == EI_CompareFunc::Always) return VK_COMPARE_OP_ALWAYS;
    assert(false && "Using an EI_CompareFunc that has not been mapped to Vulkan yet!");
    return VK_COMPARE_OP_NEVER;
}

inline VkBlendOp operator* (EI_BlendOp Enum)
{
    if (Enum == EI_BlendOp::Add) return VK_BLEND_OP_ADD;
    if (Enum == EI_BlendOp::Subtract) return VK_BLEND_OP_SUBTRACT;
    if (Enum == EI_BlendOp::ReverseSubtract) return VK_BLEND_OP_REVERSE_SUBTRACT;
    if (Enum == EI_BlendOp::Min) return VK_BLEND_OP_MIN;
    if (Enum == EI_BlendOp::Max) return VK_BLEND_OP_MAX;
    assert(false && "Using an EI_BlendOp that has not been mapped to Vulkan yet!");
    return VK_BLEND_OP_ADD;
};

inline VkStencilOp operator* (EI_StencilOp Enum)
{
    if (Enum == EI_StencilOp::Keep) return VK_STENCIL_OP_KEEP;
    if (Enum == EI_StencilOp::Zero) return VK_STENCIL_OP_ZERO;
    if (Enum == EI_StencilOp::Replace) return VK_STENCIL_OP_REPLACE;
    if (Enum == EI_StencilOp::IncrementClamp) return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
    if (Enum == EI_StencilOp::DecrementClamp) return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
    if (Enum == EI_StencilOp::Invert) return VK_STENCIL_OP_INVERT;
    if (Enum == EI_StencilOp::IncrementWrap) return VK_STENCIL_OP_INCREMENT_AND_WRAP;
    if (Enum == EI_StencilOp::DecrementWrap) return VK_STENCIL_OP_DECREMENT_AND_WRAP;
    assert(false && "Using an EI_StencilOp that has not been mapped to Vulkan yet!");
    return VK_STENCIL_OP_KEEP;
}

inline VkBlendFactor operator* (EI_BlendFactor Enum)
{
    if (Enum == EI_BlendFactor::Zero) return VK_BLEND_FACTOR_ZERO;
    if (Enum == EI_BlendFactor::One) return VK_BLEND_FACTOR_ONE;
    if (Enum == EI_BlendFactor::SrcColor) return VK_BLEND_FACTOR_SRC_COLOR;
    if (Enum == EI_BlendFactor::InvSrcColor) return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
    if (Enum == EI_BlendFactor::DstColor) return VK_BLEND_FACTOR_DST_COLOR;
    if (Enum == EI_BlendFactor::InvDstColor) return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
    if (Enum == EI_BlendFactor::SrcAlpha) return VK_BLEND_FACTOR_SRC_ALPHA;
    if (Enum == EI_BlendFactor::InvSrcAlpha) return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    if (Enum == EI_BlendFactor::DstAlpha) return VK_BLEND_FACTOR_DST_ALPHA;
    if (Enum == EI_BlendFactor::InvDstAlpha) return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
    assert(false && "Using an EI_BlendFactor that has not been mapped to Vulkan yet!");
    return VK_BLEND_FACTOR_ZERO;
};

inline VkPrimitiveTopology operator* (EI_Topology Enum)
{
    if (Enum == EI_Topology::TriangleList) return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    if (Enum == EI_Topology::TriangleStrip) return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    assert(false && "Using an EI_Topology that has not been mapped to Vulkan yet!");
    return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
}

inline VkImageLayout operator* (EI_LayoutState Enum)
{
    if (Enum == EI_LayoutState::Undefined) return VK_IMAGE_LAYOUT_UNDEFINED;
    if (Enum == EI_LayoutState::RenderColor) return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    if (Enum == EI_LayoutState::RenderDepth) return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    if (Enum == EI_LayoutState::ReadOnly) return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    if (Enum == EI_LayoutState::Present) return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    assert(false && "Using an EI_LayoutState that has not been mapped to Vulkan yet!");
    return VK_IMAGE_LAYOUT_UNDEFINED;
};

EI_Device::EI_Device() :
    m_currentImageIndex(0),
    m_pFullscreenIndexBuffer()
{
}

EI_Device::~EI_Device()
{
    
}

VkPipelineBindPoint VulkanBindPoint(EI_BindPoint bp) {
    return (bp == EI_BP_COMPUTE) ? VK_PIPELINE_BIND_POINT_COMPUTE : VK_PIPELINE_BIND_POINT_GRAPHICS;
}

struct VulkanBuffer
{
    VulkanBuffer(CAULDRON_VK::Device * pDevice) : pDevice(pDevice) {}

    void Create(const int structSize, const int structCount, const unsigned int flags, const char* name) {

        totalMemSize = structSize * structCount;

        VkBufferUsageFlags usage = 0;
        if (flags & EI_BF_UNIFORMBUFFER)
            usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        //if ( flags & EI_BF_NEEDSUAV)
        usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        if (flags & EI_BF_VERTEXBUFFER)
            usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        if (flags & EI_BF_INDEXBUFFER)
            usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

        if (flags & EI_BF_NEEDSCPUMEMORY) {
            VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
            bufferInfo.size = totalMemSize;
            bufferInfo.usage = usage | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

            VmaAllocationCreateInfo allocInfo = {};
            allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
            allocInfo.flags = VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
            allocInfo.pUserData = (void*)name;

            vmaCreateBuffer(pDevice->GetAllocator(), &bufferInfo, &allocInfo, &cpuBuffer, &cpuBufferAlloc, nullptr);
            vmaMapMemory(pDevice->GetAllocator(), cpuBufferAlloc, &cpuMappedMemory);

        }

        VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bufferInfo.size = totalMemSize;
        bufferInfo.usage = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        allocInfo.flags = VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
        allocInfo.pUserData = (void*)name;

        vmaCreateBuffer(pDevice->GetAllocator(), &bufferInfo, &allocInfo, &gpuBuffer, &gpuBufferAlloc, nullptr);

        // fill descriptor info
        info.buffer = gpuBuffer;
        info.offset = 0;
        info.range = totalMemSize;
    }

    void FreeCPUMemory()
    {
        vmaUnmapMemory(pDevice->GetAllocator(), cpuBufferAlloc);
        if (cpuBuffer != VK_NULL_HANDLE)
            vmaDestroyBuffer(pDevice->GetAllocator(), cpuBuffer, cpuBufferAlloc);
    }

    void Free() {
        FreeCPUMemory();
        if ( gpuBuffer != VK_NULL_HANDLE)
            vmaDestroyBuffer(pDevice->GetAllocator(), gpuBuffer, gpuBufferAlloc);
    }

    int totalMemSize = 0;
    CAULDRON_VK::Device          *pDevice = nullptr;
    VkDescriptorBufferInfo info = {};
    VkBuffer         cpuBuffer = VK_NULL_HANDLE;
    VkBuffer         gpuBuffer = VK_NULL_HANDLE;
    VmaAllocation    cpuBufferAlloc = VK_NULL_HANDLE;
    VmaAllocation    gpuBufferAlloc = VK_NULL_HANDLE;

    void *  cpuMappedMemory = nullptr;
};

void EI_Device::DrawFullScreenQuad(EI_CommandContext& commandContext, EI_PSO& pso, EI_BindSet** bindSets, uint32_t numBindSets)
{
    // Set everything
    commandContext.BindSets(&pso, numBindSets, bindSets);

    EI_IndexedDrawParams drawParams;
    drawParams.pIndexBuffer = m_pFullscreenIndexBuffer.get();
    drawParams.numIndices = 4;
    drawParams.numInstances = 1;

    commandContext.DrawIndexedInstanced(pso, drawParams);
}

std::unique_ptr<EI_Resource> EI_Device::CreateBufferResource(int structSize, const int structCount, const unsigned int flags, const char* name)
{
    EI_Resource* result = new EI_Resource;
    result->m_ResourceType = EI_ResourceType::Buffer;
    result->m_pBuffer = new VulkanBuffer(&m_device);
    result->m_pBuffer->Create(structSize, structCount, flags | EI_BF_NEEDSCPUMEMORY, name);

    return std::unique_ptr<EI_Resource>(result);
}

std::unique_ptr<EI_Resource> EI_Device::CreateUint32Resource(const int width, const int height, const size_t arraySize, const char* name, uint32_t ClearValue /* Ignored on Vulkan */)
{
    EI_Resource * res = new EI_Resource;
    res->m_ResourceType = EI_ResourceType::Texture;
    res->m_pTexture = new CAULDRON_VK::Texture();

    VkImageCreateInfo image_info = {};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.pNext = NULL;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.format = VK_FORMAT_R32_UINT;
    image_info.extent.width = width;
    image_info.extent.height = height;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = (uint32_t)arraySize;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.queueFamilyIndexCount = 0;
    image_info.pQueueFamilyIndices = NULL;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    image_info.flags = 0;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;


    res->m_pTexture->Init(GetDevice()->GetCauldronDevice(), &image_info, const_cast<char*>(name));	// Note, Init should really be changed to take a const char* instead of char.

    // Need to make my own CreateSRV & RTV that will take an array (current functions only assume 2D Textures - hard coded)
    res->m_pTexture->CreateSRV(&res->srv, 0);
    res->m_pTexture->CreateRTV(&res->rtv, 0);
    return std::unique_ptr<EI_Resource>(res);
}

std::unique_ptr<EI_Resource> EI_Device::CreateRenderTargetResource(const int width, const int height, const size_t channels, const size_t channelSize, const char* name, AMD::float4* ClearValues /*ignored in Vulkan*/)
{
    EI_Resource * res = new EI_Resource;
    res->m_ResourceType = EI_ResourceType::Texture;
    res->m_pTexture = new CAULDRON_VK::Texture();

    VkFormat format;
    if (channels == 1)
    {
        format = VK_FORMAT_R16_SFLOAT;
    }
    else if (channels == 2)
    {
        format = VK_FORMAT_R16G16_SFLOAT;
    }
    else if (channels == 4)
    {
        if (channelSize == 1)
            format = VK_FORMAT_R8G8B8A8_SRGB;
        else
            format = VK_FORMAT_R16G16B16A16_SFLOAT;
    }

    res->m_pTexture->InitRendertarget(&m_device, width, height, format, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, true, (char*)name);
    res->m_pTexture->CreateSRV(&res->srv, 0);
    res->m_pTexture->CreateRTV(&res->rtv, 0);

    EI_Barrier barrier = { res, EI_STATE_UNDEFINED, EI_STATE_RENDER_TARGET };
    GetDevice()->GetCurrentCommandContext().SubmitBarrier(1, &barrier);
    return std::unique_ptr<EI_Resource>(res);
}

std::unique_ptr<EI_Resource> EI_Device::CreateDepthResource(const int width, const int height, const char* name)
{
    EI_Resource* res = new EI_Resource;
    res->m_ResourceType = EI_ResourceType::Texture;
    res->m_pTexture = new CAULDRON_VK::Texture();

    VkImageCreateInfo image_info = {};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.pNext = NULL;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.format = VK_FORMAT_D32_SFLOAT;
    image_info.extent.width = width;
    image_info.extent.height = height;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.queueFamilyIndexCount = 0;
    image_info.pQueueFamilyIndices = NULL;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT; //TODO    
    image_info.flags = 0;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;   // VK_IMAGE_TILING_LINEAR should never be used and will never be faster

    res->m_pTexture->Init(GetDevice()->GetCauldronDevice(), &image_info, const_cast<char*>(name));	// Note, Init should really be changed to take a const char* instead of char.
    res->m_pTexture->CreateSRV(&res->srv);
    res->m_pTexture->CreateDSV(&res->rtv);

    EI_Barrier barrier = { res, EI_STATE_UNDEFINED, EI_STATE_DEPTH_STENCIL };
    GetDevice()->GetCurrentCommandContext().SubmitBarrier(1, &barrier);

    return std::unique_ptr<EI_Resource>(res);
}

std::unique_ptr<EI_Resource> EI_Device::CreateResourceFromFile(const char* szFilename, bool useSRGB/*= false*/)
{
    EI_Resource* res = new EI_Resource;
    res->m_ResourceType = EI_ResourceType::Texture;
    res->m_pTexture = new CAULDRON_VK::Texture();

    res->m_pTexture->InitFromFile(GetDevice()->GetCauldronDevice(), &m_uploadHeap, szFilename, useSRGB);
    res->m_pTexture->CreateSRV(&res->srv, 0);
    m_uploadHeap.FlushAndFinish();

    return std::unique_ptr<EI_Resource>(res);
}

std::unique_ptr<EI_Resource> EI_Device::CreateSampler(EI_Filter MinFilter, EI_Filter MaxFilter, EI_Filter MipFilter, EI_AddressMode AddressMode)
{
    EI_Resource* res = new EI_Resource;
    res->m_ResourceType = EI_ResourceType::Sampler;
    res->m_pSampler = new VkSampler;

    VkSamplerCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.magFilter = (MinFilter == EI_Filter::Linear)? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
    info.minFilter = (MaxFilter == EI_Filter::Linear)? VK_FILTER_LINEAR : VK_FILTER_NEAREST;;
    info.mipmapMode = (MipFilter == EI_Filter::Linear)? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
    info.addressModeU = (AddressMode == EI_AddressMode::Wrap)? VK_SAMPLER_ADDRESS_MODE_REPEAT : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    info.addressModeV = (AddressMode == EI_AddressMode::Wrap) ? VK_SAMPLER_ADDRESS_MODE_REPEAT : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;;
    info.addressModeW = (AddressMode == EI_AddressMode::Wrap) ? VK_SAMPLER_ADDRESS_MODE_REPEAT : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;;
    info.minLod = -1000;
    info.maxLod = 1000;
    info.maxAnisotropy = 1.0f;
    VkResult result = vkCreateSampler(GetCauldronDevice()->GetDevice(), &info, NULL, res->m_pSampler);
    assert(result == VK_SUCCESS);

    return std::unique_ptr<EI_Resource>(res);
}

std::unique_ptr<EI_BindSet> EI_Device::CreateBindSet(EI_BindLayout * layout, EI_BindSetDescription& bindSet)
{
    EI_BindSet * result = new EI_BindSet;
    m_resourceViewHeaps.AllocDescriptor(layout->m_descriptorSetLayout, &result->m_descriptorSet);
    
    std::vector<VkWriteDescriptorSet> writes;
    std::vector<std::unique_ptr<VkDescriptorImageInfo>> DescriptorImageInfos;

    int numResources = (int)bindSet.resources.size();
    for (int i = 0; i < numResources; ++i)
    {
        VkWriteDescriptorSet w = {};
        w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        w.dstSet = result->m_descriptorSet;
        w.dstBinding = layout->description.resources[i].binding;
        w.dstArrayElement = 0;
        w.descriptorCount = 1;

        switch (layout->description.resources[i].type)
        {
        case EI_RESOURCETYPE_BUFFER_RW:
        {
            assert(bindSet.resources[i]->m_ResourceType == EI_ResourceType::Buffer);
            w.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            w.pBufferInfo = &bindSet.resources[i]->m_pBuffer->info;
            break;
        }
        case EI_RESOURCETYPE_BUFFER_RO:
        {
            assert(bindSet.resources[i]->m_ResourceType == EI_ResourceType::Buffer);
            w.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            w.pBufferInfo = &bindSet.resources[i]->m_pBuffer->info;
            break;
        }
        case EI_RESOURCETYPE_IMAGE_RW:
        {
            w.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            DescriptorImageInfos.push_back(std::make_unique<VkDescriptorImageInfo>());
            VkDescriptorImageInfo* pImageInfo = DescriptorImageInfos[DescriptorImageInfos.size() - 1].get();
            assert(bindSet.resources[i]->m_ResourceType == EI_ResourceType::Texture);
            pImageInfo->imageView = bindSet.resources[i]->srv;
            pImageInfo->imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            w.pImageInfo = pImageInfo;
            break;
        }
        case EI_RESOURCETYPE_IMAGE_RO:
        {
            w.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            DescriptorImageInfos.push_back(std::make_unique<VkDescriptorImageInfo>());
            VkDescriptorImageInfo* pImageInfo = DescriptorImageInfos[DescriptorImageInfos.size() - 1].get();
            assert(bindSet.resources[i]->m_ResourceType == EI_ResourceType::Texture);
            pImageInfo->imageView = bindSet.resources[i]->srv;
            pImageInfo->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            w.pImageInfo = pImageInfo;
            break;
        }
        case EI_RESOURCETYPE_UNIFORM:
        {
            assert(bindSet.resources[i]->m_ResourceType == EI_ResourceType::Buffer);
            w.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            w.pBufferInfo = &bindSet.resources[i]->m_pBuffer->info;
            break;
        }
        case EI_RESOURCETYPE_SAMPLER:
        {
            assert(bindSet.resources[i]->m_ResourceType == EI_ResourceType::Sampler);
            w.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
            DescriptorImageInfos.push_back(std::make_unique<VkDescriptorImageInfo>());
            VkDescriptorImageInfo* pImageInfo = DescriptorImageInfos[DescriptorImageInfos.size() - 1].get();
            pImageInfo->sampler = *bindSet.resources[i]->m_pSampler;
            w.pImageInfo = pImageInfo;
            break;
        }
        default:
            assert(0);
            break;
        }
        writes.push_back(w);
    }

    assert(writes.size() == layout->layoutBindings.size());
 
    vkUpdateDescriptorSets(m_device.GetDevice(), (uint32_t)writes.size(), writes.data(), 0, nullptr);

    return std::unique_ptr<EI_BindSet>(result);
}

EI_BindSet::~EI_BindSet()
{
    GetDevice()->GetResourceViewHeaps()->FreeDescriptor(m_descriptorSet);
}

std::unique_ptr<EI_RenderTargetSet> EI_Device::CreateRenderTargetSet(const EI_ResourceFormat* pResourceFormats, const uint32_t numResources, const EI_AttachmentParams* AttachmentParams, float* clearValues)
{
    // Create the render pass set
    EI_RenderTargetSet* pNewRenderTargetSet = new EI_RenderTargetSet;

    int currentClearValueRef = 0;
    for (uint32_t i = 0; i < numResources; i++)
    {
        // Check size consistency
       assert(!(AttachmentParams[i].Flags & EI_RenderPassFlags::Depth && (i != (numResources - 1))) && "Only the last attachment can be specified as depth target");

        // Setup a clear value if needed
        if (AttachmentParams[i].Flags & EI_RenderPassFlags::Clear)
        {
            if (AttachmentParams[i].Flags & EI_RenderPassFlags::Depth)
            {
                pNewRenderTargetSet->m_ClearValues[i].depthStencil.depth = clearValues[currentClearValueRef++];
                pNewRenderTargetSet->m_ClearValues[i].depthStencil.stencil = (uint32_t)clearValues[currentClearValueRef++];
            }
            else
            {
                pNewRenderTargetSet->m_ClearValues[i].color.float32[0] = clearValues[currentClearValueRef++];
                pNewRenderTargetSet->m_ClearValues[i].color.float32[1] = clearValues[currentClearValueRef++];
                pNewRenderTargetSet->m_ClearValues[i].color.float32[2] = clearValues[currentClearValueRef++];
                pNewRenderTargetSet->m_ClearValues[i].color.float32[3] = clearValues[currentClearValueRef++];
            }
        }
    }

    // Tag the number of resources this render pass set is setting/clearing
    pNewRenderTargetSet->m_NumResources = numResources;

    // Setup the render pass
    pNewRenderTargetSet->m_RenderPass = CreateRenderPass(pResourceFormats, numResources, AttachmentParams);

    return std::unique_ptr<EI_RenderTargetSet>(pNewRenderTargetSet);
}

std::unique_ptr<EI_RenderTargetSet> EI_Device::CreateRenderTargetSet(const EI_Resource** pResourcesArray, const uint32_t numResources, const EI_AttachmentParams* AttachmentParams, float* clearValues)
{
    std::vector<EI_ResourceFormat> FormatArray(numResources);
    
    for (uint32_t i = 0; i < numResources; ++i)
    {
        assert(pResourcesArray[i]->m_ResourceType == EI_ResourceType::Texture);
        FormatArray[i] = pResourcesArray[i]->m_pTexture->GetFormat();
    }
    std::unique_ptr<EI_RenderTargetSet> result = CreateRenderTargetSet(FormatArray.data(), numResources, AttachmentParams, clearValues);
    result->SetResources(pResourcesArray);
    return result;
}

std::unique_ptr<EI_GLTFTexturesAndBuffers> EI_Device::CreateGLTFTexturesAndBuffers(GLTFCommon* pGLTFCommon)
{
    EI_GLTFTexturesAndBuffers* GLTFBuffersAndTextures = new CAULDRON_VK::GLTFTexturesAndBuffers();
    GLTFBuffersAndTextures->OnCreate(GetCauldronDevice(), pGLTFCommon, &m_uploadHeap, &m_vidMemBufferPool, &m_constantBufferRing);

    return std::unique_ptr<EI_GLTFTexturesAndBuffers>(GLTFBuffersAndTextures);
}

std::unique_ptr<EI_GltfPbrPass> EI_Device::CreateGLTFPbrPass(EI_GLTFTexturesAndBuffers* pGLTFTexturesAndBuffers, EI_RenderTargetSet* renderTargetSet)
{
    EI_GltfPbrPass* GLTFPbr = new CAULDRON_VK::GltfPbrPass();
    GLTFPbr->OnCreate(GetCauldronDevice(), renderTargetSet != nullptr ? renderTargetSet->m_RenderPass : GetSwapChainRenderPass(),
                        &m_uploadHeap, &m_resourceViewHeaps, &m_constantBufferRing, &m_vidMemBufferPool, pGLTFTexturesAndBuffers,
                        NULL, GetShadowBufferResource()->srv, VK_SAMPLE_COUNT_1_BIT);

    return std::unique_ptr<EI_GltfPbrPass>(GLTFPbr);
}

std::unique_ptr<EI_GltfDepthPass> EI_Device::CreateGLTFDepthPass(EI_GLTFTexturesAndBuffers* pGLTFTexturesAndBuffers, EI_RenderTargetSet* renderTargetSet)
{
    EI_GltfDepthPass* GLTFDepth = new CAULDRON_VK::GltfDepthPass();
    assert(renderTargetSet);

    GLTFDepth->OnCreate(GetCauldronDevice(), renderTargetSet->m_RenderPass, &m_uploadHeap, &m_resourceViewHeaps, 
                        &m_constantBufferRing, &m_vidMemBufferPool, pGLTFTexturesAndBuffers);

    return std::unique_ptr<EI_GltfDepthPass>(GLTFDepth);
}

void EI_RenderTargetSet::SetResources(const EI_Resource** pResourcesArray)
{
    if (m_FrameBuffer) {

        vkDestroyFramebuffer(GetDevice()->GetVulkanDevice(), m_FrameBuffer, nullptr);
    }

    // Now setup up the needed frame buffers
    VkImageView viewAttachments[MaxRenderAttachments];

    // We need the SRVs for all the things ...
    for (uint32_t i = 0; i < m_NumResources; ++i)
        viewAttachments[i] = pResourcesArray[i]->rtv;

    VkFramebufferCreateInfo fb_info = {};
    fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fb_info.pNext = NULL;
    fb_info.renderPass = m_RenderPass;
    fb_info.attachmentCount = m_NumResources;
    fb_info.pAttachments = viewAttachments;

    // Use the width and height from the first entry (they should ALL be the same)
    fb_info.width = pResourcesArray[0]->GetWidth();
    fb_info.height = pResourcesArray[0]->GetHeight();
    fb_info.layers = 1;

    vkCreateFramebuffer(GetDevice()->GetVulkanDevice(), &fb_info, nullptr, &m_FrameBuffer);
}

VkRenderPass EI_Device::CreateRenderPass(const EI_ResourceFormat* pResourceFormats, const uint32_t numResources, const EI_AttachmentParams* AttachmentParams)
{
    VkAttachmentDescription attachments[MaxRenderAttachments];
    VkAttachmentReference colorRefs[MaxRenderAttachments];
    VkAttachmentReference depthRef;
    int numColorRefs = 0;

    assert(numResources < MaxRenderAttachments && "Creating a RenderPass with more attachments than currently supportable. Please increase MaxRenderAttachments static variable in VKEngineInterfaceImpl.cpp");

    // Start by figuring out render pass buffers
    for (uint32_t i = 0; i < numResources; ++i)
    {
        assert(!(AttachmentParams[i].Flags & EI_RenderPassFlags::Depth && (i != (numResources - 1))) && "Only the last attachment can be specified as depth target");

        attachments[i].format = pResourceFormats[i];
        attachments[i].samples = VK_SAMPLE_COUNT_1_BIT;	// We should probably find a better way to query/set this in the future
        attachments[i].storeOp = AttachmentParams[i].Flags & EI_RenderPassFlags::Store? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;

        VkImageLayout imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        if (AttachmentParams[i].Flags & EI_RenderPassFlags::Depth)
        {
            imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }
        attachments[i].initialLayout = imageLayout;
        attachments[i].finalLayout = imageLayout;
        assert(attachments[i].initialLayout == attachments[i].finalLayout);
        attachments[i].flags = 0;
        VkAttachmentLoadOp loadOp = AttachmentParams[i].Flags & EI_RenderPassFlags::Load ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[i].loadOp = AttachmentParams[i].Flags & EI_RenderPassFlags::Clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : loadOp;

        if (AttachmentParams[i].Flags & EI_RenderPassFlags::Depth)
        {
            loadOp = AttachmentParams[i].Flags & EI_RenderPassFlags::Load ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachments[i].stencilLoadOp = AttachmentParams[i].Flags & EI_RenderPassFlags::Clear? VK_ATTACHMENT_LOAD_OP_CLEAR : loadOp;
            attachments[i].stencilStoreOp = AttachmentParams[i].Flags & EI_RenderPassFlags::Store? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depthRef = { i, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
        }
        else
        {
            attachments[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachments[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            colorRefs[numColorRefs++] = { i, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
        }
    }

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.flags = 0;
    subpass.inputAttachmentCount = 0;
    subpass.pInputAttachments = NULL;
    subpass.colorAttachmentCount = numColorRefs;
    subpass.pColorAttachments = colorRefs;
    subpass.pResolveAttachments = NULL;
    subpass.pDepthStencilAttachment = numColorRefs != numResources? &depthRef : nullptr; // If we don't have the same number of color resources as total resources, one is depth
    subpass.preserveAttachmentCount = 0;
    subpass.pPreserveAttachments = NULL;

    VkRenderPassCreateInfo rp_info = {};
    rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rp_info.pNext = NULL;
    rp_info.attachmentCount = numResources;
    rp_info.pAttachments = attachments;
    rp_info.subpassCount = 1;
    rp_info.pSubpasses = &subpass;
    rp_info.dependencyCount = 0;
    rp_info.pDependencies = NULL;

    // Create the Vulkan render pass
    VkRenderPass pass;
    vkCreateRenderPass(m_device.GetDevice(), &rp_info, NULL, &pass);
    return pass;
}

EI_RenderTargetSet::~EI_RenderTargetSet()
{
    vkDestroyFramebuffer(GetDevice()->GetVulkanDevice(), m_FrameBuffer, nullptr);
    vkDestroyRenderPass(GetDevice()->GetVulkanDevice(), m_RenderPass, nullptr);
}

void EI_Device::BeginRenderPass(EI_CommandContext& commandContext, const EI_RenderTargetSet* pRenderTargetSet, const wchar_t* pPassName, uint32_t width /*= 0*/, uint32_t height /*= 0*/)
{
    VkRenderPassBeginInfo rp_begin = {};

    rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rp_begin.pNext = NULL;
    rp_begin.renderPass = pRenderTargetSet->m_RenderPass;
    rp_begin.framebuffer = pRenderTargetSet->m_FrameBuffer;
    rp_begin.renderArea.offset.x = 0;
    rp_begin.renderArea.offset.y = 0;
    rp_begin.renderArea.extent.width = width? width : m_width;
    rp_begin.renderArea.extent.height = height? height : m_height;
    rp_begin.clearValueCount = pRenderTargetSet->m_NumResources;
    rp_begin.pClearValues = pRenderTargetSet->m_ClearValues;

    vkCmdBeginRenderPass(commandContext.commandBuffer, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

    // NOTE: This should probably in it's own stand-alone call
    CAULDRON_VK::SetViewportAndScissor(commandContext.commandBuffer, 0, 0, width ? width : m_width, height ? height : m_height);
}

void EI_Device::EndRenderPass(EI_CommandContext& commandContext)
{
    vkCmdEndRenderPass(commandContext.commandBuffer);
}

void EI_Device::SetViewportAndScissor(EI_CommandContext& commandContext, uint32_t topX, uint32_t topY, uint32_t width, uint32_t height)
{
    CAULDRON_VK::SetViewportAndScissor(commandContext.commandBuffer, topX, topY, width, height);
}

void EI_Device::OnCreate(HWND hWnd, uint32_t numBackBuffers, bool enableValidation, const char* appName)
{
    // Create Device
    m_device.OnCreate(appName, "Cauldron", enableValidation, hWnd);
    m_device.CreatePipelineCache();

    //init the shader compiler
    CAULDRON_VK::CreateShaderCache();

    // Create Swapchain
    m_swapChain.OnCreate(&m_device, numBackBuffers, hWnd, CAULDRON_VK::DISPLAYMODE_SDR);

    m_resourceViewHeaps.OnCreate(&m_device, 256, 256, 256, 256);

    // Create a commandlist ring for the Direct queue
    m_commandListRing.OnCreate(&m_device, numBackBuffers, 8);
    // async compute
    m_computeCommandListRing.OnCreate(&m_device, numBackBuffers, 8, true);
    BeginNewCommandBuffer();

    VkFenceCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    info.flags = 0;
    vkCreateFence(GetVulkanDevice(), &info, nullptr, &m_computeDoneFence);


    // Create a 'dynamic' constant buffers ring
    m_constantBufferRing.OnCreate(&m_device, numBackBuffers, 20 * 1024 * 1024, "Uniforms");

    // Create a 'static' constant buffer pool
    m_vidMemBufferPool.OnCreate(&m_device, 128 * 1024 * 1024, USE_VID_MEM, "StaticGeom");
    m_sysMemBufferPool.OnCreate(&m_device, 32 * 1024, false, "PostProcGeom");

    // initialize the GPU time stamps module
    m_gpuTimer.OnCreate(&m_device, numBackBuffers);

    // Quick helper to upload resources, it has it's own commandList and uses suballocation.
    // for 4K textures we'll need 100Megs
    m_uploadHeap.OnCreate(&m_device, 100 * 1024 * 1024);

    // Create tonemapping pass
    m_toneMapping.OnCreate(&m_device, m_swapChain.GetRenderPass(), &m_resourceViewHeaps, &m_sysMemBufferPool, &m_constantBufferRing);

    // Initialize UI rendering resources
    m_ImGUI.OnCreate(&m_device, m_swapChain.GetRenderPass(), &m_uploadHeap, &m_constantBufferRing);

    // Create a render pass for our main buffer as it's needed earlier than other stuff is created
    VkFormat backbufferFormats[] = { VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_D32_SFLOAT };

    // Create index buffer for full screen passes
    m_pFullscreenIndexBuffer = CreateBufferResource(sizeof(uint32_t), 4, EI_BF_INDEXBUFFER, "FullScreenIndexBuffer");

    // Create shadow buffer. Because GLTF only allows us 1 buffer, we are going to create a HUGE one and divy it up as needed.
    m_shadowBuffer = CreateDepthResource(4096, 4096, "Shadow Buffer");

    // Create layout and PSO for resolve to swap chain
    EI_LayoutDescription desc = { { {"ColorTexture", 0, EI_RESOURCETYPE_IMAGE_RO }, }, EI_PS };
    m_pEndFrameResolveBindLayout = CreateLayout(desc);

    // Recreate a PSO for full screen resolve to swap chain
    //std::vector<VkVertexInputAttributeDescription> descs;
    EI_PSOParams psoParams;
    psoParams.primitiveTopology = EI_Topology::TriangleStrip;
    psoParams.colorWriteEnable = true;
    psoParams.depthTestEnable = false;
    psoParams.depthWriteEnable = false;
    psoParams.depthCompareOp = EI_CompareFunc::Always;

    psoParams.colorBlendParams.colorBlendEnabled = false;
    psoParams.colorBlendParams.colorBlendOp = EI_BlendOp::Add;
    psoParams.colorBlendParams.colorSrcBlend = EI_BlendFactor::Zero;
    psoParams.colorBlendParams.colorDstBlend = EI_BlendFactor::One;
    psoParams.colorBlendParams.alphaBlendOp = EI_BlendOp::Add;
    psoParams.colorBlendParams.alphaSrcBlend = EI_BlendFactor::One;
    psoParams.colorBlendParams.alphaDstBlend = EI_BlendFactor::Zero;

    EI_BindLayout* layouts[] = { m_pEndFrameResolveBindLayout.get() };
    psoParams.layouts = layouts;
    psoParams.numLayouts = 1;
    psoParams.renderTargetSet = nullptr; // Will go to swapchain
    m_pEndFrameResolvePSO = CreateGraphicsPSO("FullScreenRender.hlsl", "FullScreenVS", "FullScreenRender.hlsl", "FullScreenPS", psoParams);

    // Create default white texture to use
    m_DefaultWhiteTexture = CreateResourceFromFile("DefaultWhite.png", true);

    // Create some samplers to use
    m_LinearWrapSampler = CreateSampler(EI_Filter::Linear, EI_Filter::Linear, EI_Filter::Linear, EI_AddressMode::Wrap);

    // finish creating the index buffer
    uint32_t indexArray[] = { 0, 1, 2, 3 };
    m_currentCommandBuffer.UpdateBuffer(m_pFullscreenIndexBuffer.get(), indexArray);

    EI_Barrier copyToResource[] = {
        { m_pFullscreenIndexBuffer.get(), EI_STATE_COPY_DEST, EI_STATE_INDEX_BUFFER }
    };
    m_currentCommandBuffer.SubmitBarrier(AMD_ARRAY_SIZE(copyToResource), copyToResource);
}

void EI_Device::OnResize(uint32_t width, uint32_t height)
{
    m_width = width;
    m_height = height;

    // if a previous resize event from this frame hasnt already opened a command buffer
    if (!m_recording)
    {
        BeginNewCommandBuffer();
    }

    // If resizing but no minimizing
    if (width > 0 && height > 0)
    {
        // Re/Create color buffer
        m_colorBuffer = CreateRenderTargetResource(width, height, 4, 1, "Color Buffer");
        m_depthBuffer = CreateDepthResource(width, height, "Depth Buffer");
        m_swapChain.OnCreateWindowSizeDependentResources(width, height, m_vSync, CAULDRON_VK::DISPLAYMODE_SDR);

        // Create resources we need to resolve out render target back to swap chain
        EI_BindSetDescription bindSet = { { m_colorBuffer.get() } };
        m_pEndFrameResolveBindSet = CreateBindSet(m_pEndFrameResolveBindLayout.get(), bindSet);

        // Create a bind set for any samplers we need (Doing it here because the layouts aren't yet initialized during OnCreate() call)
        EI_BindSetDescription bindSetDesc = { { m_LinearWrapSampler.get(), } };
        m_pSamplerBindSet = CreateBindSet(GetSamplerLayout(), bindSetDesc);

        // update tonemapping
        m_toneMapping.UpdatePipelines(m_swapChain.GetRenderPass());
    }
}

void EI_Device::OnDestroy()
{
    m_device.GPUFlush();

    // Remove linear wrap sampler
    m_LinearWrapSampler.reset();

    // Remove default white texture
    m_DefaultWhiteTexture.reset();

    // Wipe all the local resources we were using
    m_pSamplerBindSet.reset();
    m_pEndFrameResolveBindSet.reset();
    m_pEndFrameResolvePSO.reset();
    m_pEndFrameResolveBindLayout.reset();

    m_pFullscreenIndexBuffer.reset();
    m_shadowBuffer.reset();
    m_depthBuffer.reset();
    m_colorBuffer.reset();

#if TRESSFX_DEBUG_UAV
    m_pDebugUAV.reset();
#endif // TRESSFX_DEBUG_UAV

    m_toneMapping.OnDestroy();
    m_ImGUI.OnDestroy();

    m_uploadHeap.OnDestroy();
    m_gpuTimer.OnDestroy();
    m_vidMemBufferPool.OnDestroy();
    m_sysMemBufferPool.OnDestroy();
    m_constantBufferRing.OnDestroy();
    m_resourceViewHeaps.OnDestroy();
    m_commandListRing.OnDestroy();
    m_computeCommandListRing.OnDestroy();

    // Fullscreen state should always be false before exiting the app.
    m_swapChain.SetFullScreen(false);
    m_swapChain.OnDestroyWindowSizeDependentResources();
    m_swapChain.OnDestroy();

    // shut down the shader compiler
    DestroyShaderCache(&m_device);
    m_device.DestroyPipelineCache();
    vkDestroyFence(GetVulkanDevice(), m_computeDoneFence, nullptr);
    
    m_device.OnDestroy();
}

std::unique_ptr<EI_PSO> EI_Device::CreateComputeShaderPSO(const char * shaderName, const char * entryPoint, EI_BindLayout ** layouts, int numLayouts)
{
    EI_PSO * result = new EI_PSO;

    DefineList defines;
    defines["AMD_TRESSFX_MAX_NUM_BONES"] = std::to_string(AMD_TRESSFX_MAX_NUM_BONES);
    defines["AMD_TRESSFX_MAX_HAIR_GROUP_RENDER"] = std::to_string(AMD_TRESSFX_MAX_HAIR_GROUP_RENDER);
    defines["AMD_TRESSFX_VULKAN"] = "1";

    VkPipelineShaderStageCreateInfo computeShader;
    CAULDRON_VK::VKCompileFromFile(m_device.GetDevice(), VK_SHADER_STAGE_COMPUTE_BIT, shaderName, entryPoint, &defines, &computeShader);

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};

    VkDescriptorSetLayout descSetLayouts[16];
    for (int i = 0; i < numLayouts; ++i)
    {
        descSetLayouts[i] = layouts[i]->m_descriptorSetLayout;
    }
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.pSetLayouts = descSetLayouts;
    pipelineLayoutCreateInfo.setLayoutCount = numLayouts;
    vkCreatePipelineLayout(m_device.GetDevice(), &pipelineLayoutCreateInfo, nullptr, &result->m_pipelineLayout);
    VkComputePipelineCreateInfo computePipelineCreateInfo = {};
    computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    computePipelineCreateInfo.layout = result->m_pipelineLayout;
    computePipelineCreateInfo.stage = computeShader;
    vkCreateComputePipelines(m_device.GetDevice(), m_device.GetPipelineCache(), 1, &computePipelineCreateInfo, NULL, &result->m_pipeline);

    result->m_bp = EI_BP_COMPUTE;
    return std::unique_ptr<EI_PSO>(result);
}

EI_PSO::~EI_PSO()
{
    vkDestroyPipeline(GetDevice()->GetVulkanDevice(), m_pipeline, NULL);
    vkDestroyPipelineLayout(GetDevice()->GetVulkanDevice(), m_pipelineLayout, NULL);
}

std::unique_ptr<EI_PSO> EI_Device::CreateGraphicsPSO(const char * vertexShaderName, const char * vertexEntryPoint, const char * fragmentShaderName, const char * fragmentEntryPoint, EI_PSOParams& psoParams)
{
    EI_PSO * result = new EI_PSO;

    DefineList defines;
    defines["AMD_TRESSFX_MAX_NUM_BONES"] = std::to_string(AMD_TRESSFX_MAX_NUM_BONES);
    defines["AMD_TRESSFX_MAX_HAIR_GROUP_RENDER"] = std::to_string(AMD_TRESSFX_MAX_HAIR_GROUP_RENDER);
    defines["TRESSFX_VULKAN"] = "1";

    // Compile and create shaders
    VkPipelineShaderStageCreateInfo vertexShader = {}, fragmentShader = {};

    CAULDRON_VK::VKCompileFromFile(m_device.GetDevice(), VK_SHADER_STAGE_VERTEX_BIT, vertexShaderName, vertexEntryPoint, &defines, &vertexShader);
    CAULDRON_VK::VKCompileFromFile(m_device.GetDevice(), VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShaderName, fragmentEntryPoint, &defines, &fragmentShader);

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};

    VkDescriptorSetLayout descSetLayouts[16];
    for (int i = 0; i < psoParams.numLayouts; ++i)
    {
        descSetLayouts[i] = psoParams.layouts[i]->m_descriptorSetLayout;
    }
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.pSetLayouts = descSetLayouts;
    pipelineLayoutCreateInfo.setLayoutCount = psoParams.numLayouts;
    vkCreatePipelineLayout(m_device.GetDevice(), &pipelineLayoutCreateInfo, nullptr, &result->m_pipelineLayout);

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages = { vertexShader, fragmentShader };

    // Create pipeline

    // vertex input state (never need any)
    std::vector<VkVertexInputBindingDescription> vi_binding(0);

    VkPipelineVertexInputStateCreateInfo vi = {};
    vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vi.pNext = NULL;
    vi.flags = 0;
    vi.vertexBindingDescriptionCount = (uint32_t)vi_binding.size();
    vi.pVertexBindingDescriptions = vi_binding.data();
    vi.vertexAttributeDescriptionCount = 0; 
    vi.pVertexAttributeDescriptions = nullptr;

    // input assembly state
    VkPipelineInputAssemblyStateCreateInfo ia;
    ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.pNext = NULL;
    ia.flags = 0;
    ia.primitiveRestartEnable = VK_FALSE;
    ia.topology = *psoParams.primitiveTopology;

    // rasterizer state
    VkPipelineRasterizationStateCreateInfo rs;
    rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs.pNext = NULL;
    rs.flags = 0;
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.cullMode = VK_CULL_MODE_BACK_BIT;
    rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rs.depthClampEnable = VK_FALSE;
    rs.rasterizerDiscardEnable = VK_FALSE;
    rs.depthBiasEnable = VK_FALSE;
    rs.depthBiasConstantFactor = 0;
    rs.depthBiasClamp = 0;
    rs.depthBiasSlopeFactor = 0;
    rs.lineWidth = 1.0f;

    VkPipelineColorBlendAttachmentState att_state[1];
    att_state[0].colorWriteMask = psoParams.colorWriteEnable ? VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT : 0;
    att_state[0].blendEnable = psoParams.colorBlendParams.colorBlendEnabled;
    att_state[0].colorBlendOp = *psoParams.colorBlendParams.colorBlendOp;
    att_state[0].srcColorBlendFactor = *psoParams.colorBlendParams.colorSrcBlend;
    att_state[0].dstColorBlendFactor = *psoParams.colorBlendParams.colorDstBlend;
    att_state[0].alphaBlendOp = *psoParams.colorBlendParams.alphaBlendOp;
    att_state[0].srcAlphaBlendFactor = *psoParams.colorBlendParams.alphaSrcBlend;
    att_state[0].dstAlphaBlendFactor = *psoParams.colorBlendParams.alphaDstBlend;

    // Color blend state
    VkPipelineColorBlendStateCreateInfo cb;
    cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cb.flags = 0;
    cb.pNext = NULL;
    cb.attachmentCount = 1;
    cb.pAttachments = att_state;
    cb.logicOpEnable = VK_FALSE;
    cb.logicOp = VK_LOGIC_OP_NO_OP;
    cb.blendConstants[0] = 0;
    cb.blendConstants[1] = 0;
    cb.blendConstants[2] = 0;
    cb.blendConstants[3] = 0;

    std::vector<VkDynamicState> dynamicStateEnables = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.pNext = NULL;
    dynamicState.pDynamicStates = dynamicStateEnables.data();
    dynamicState.dynamicStateCount = (uint32_t)dynamicStateEnables.size();

    // view port state
    VkPipelineViewportStateCreateInfo vp = {};
    vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp.pNext = NULL;
    vp.flags = 0;
    vp.viewportCount = 1;
    vp.scissorCount = 1;
    vp.pScissors = NULL;
    vp.pViewports = NULL;

    // depth stencil state
    VkPipelineDepthStencilStateCreateInfo ds;
    ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds.pNext = NULL;
    ds.flags = 0;
    ds.depthTestEnable = psoParams.depthTestEnable;
    ds.depthWriteEnable = psoParams.depthWriteEnable;
    ds.depthCompareOp = *psoParams.depthCompareOp;
    ds.stencilTestEnable = psoParams.stencilTestEnable;
    
    ds.back.failOp = *psoParams.backFailOp;
    ds.back.passOp = *psoParams.backPassOp;
    ds.back.depthFailOp = *psoParams.backDepthFailOp;
    ds.back.compareOp = *psoParams.backCompareOp;
    
    ds.front.failOp = *psoParams.frontFailOp;
    ds.front.passOp = *psoParams.frontPassOp;
    ds.front.depthFailOp = *psoParams.frontDepthFailOp;
    ds.front.compareOp = *psoParams.frontCompareOp;

    ds.back.compareMask = ds.front.compareMask = psoParams.stencilReadMask;
    ds.back.reference = ds.front.reference = psoParams.stencilReference;
    ds.back.writeMask = ds.front.writeMask = psoParams.stencilWriteMask;

    ds.depthBoundsTestEnable = VK_FALSE;
    ds.minDepthBounds = 0;
    ds.maxDepthBounds = 0;

    // multi sample state
    VkPipelineMultisampleStateCreateInfo ms;
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.pNext = NULL;
    ms.flags = 0;
    ms.pSampleMask = NULL;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    ms.sampleShadingEnable = VK_FALSE;
    ms.alphaToCoverageEnable = VK_FALSE;
    ms.alphaToOneEnable = VK_FALSE;
    ms.minSampleShading = 0.0;

    // create pipeline
    VkGraphicsPipelineCreateInfo pipeline = {};
    pipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline.pNext = NULL;
    pipeline.layout = result->m_pipelineLayout;
    pipeline.basePipelineHandle = VK_NULL_HANDLE;
    pipeline.basePipelineIndex = 0;
    pipeline.flags = 0;
    pipeline.pVertexInputState = &vi;
    pipeline.pInputAssemblyState = &ia;
    pipeline.pRasterizationState = &rs;
    pipeline.pColorBlendState = &cb;
    pipeline.pTessellationState = NULL;
    pipeline.pMultisampleState = &ms;
    pipeline.pDynamicState = &dynamicState;
    pipeline.pViewportState = &vp;
    pipeline.pDepthStencilState = &ds;
    pipeline.pStages = shaderStages.data();
    pipeline.stageCount = (uint32_t)shaderStages.size();
    pipeline.renderPass = psoParams.renderTargetSet != nullptr ? psoParams.renderTargetSet->m_RenderPass : GetSwapChainRenderPass();
    pipeline.subpass = 0;

    vkCreateGraphicsPipelines(m_device.GetDevice(), m_device.GetPipelineCache(), 1, &pipeline, NULL, &result->m_pipeline);

    result->m_bp = EI_BP_GRAPHICS;
    return std::unique_ptr<EI_PSO>(result);
}

void EI_Device::WaitForCompute()
{
    vkWaitForFences(GetVulkanDevice(), 1, &m_computeDoneFence, true, UINT64_MAX);
}

void EI_Device::SignalComputeStart()
{
    // TODO
    vkResetFences(GetVulkanDevice(), 1, &m_computeDoneFence);
}

void EI_Device::WaitForLastFrameGraphics()
{
    // TODO - not sure if this is necessary
    if ( m_lastFrameGraphicsCommandBufferFence != VK_NULL_HANDLE)
    {
        vkWaitForFences(GetVulkanDevice(), 1, &m_lastFrameGraphicsCommandBufferFence, true, UINT64_MAX);
    }
}

void EI_Device::SubmitComputeCommandList()
{
    vkEndCommandBuffer(m_currentComputeCommandBuffer.commandBuffer);

    VkPipelineStageFlags submitWaitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    const VkCommandBuffer cmd_bufs[] = { m_currentComputeCommandBuffer.commandBuffer };
    VkSubmitInfo submit_info;
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = NULL;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = nullptr;
    submit_info.pWaitDstStageMask = &submitWaitStage;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = cmd_bufs;
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = nullptr;

    vkQueueSubmit(m_device.GetComputeQueue(), 1, &submit_info, m_computeDoneFence);
}

void EI_CommandContext::DrawIndexedInstanced(EI_PSO& pso, EI_IndexedDrawParams& drawParams)
{
    assert(drawParams.pIndexBuffer->m_ResourceType == EI_ResourceType::Buffer);
    vkCmdBindIndexBuffer(commandBuffer, drawParams.pIndexBuffer->m_pBuffer->gpuBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pso.m_pipeline);
    vkCmdDrawIndexed(commandBuffer, drawParams.numIndices, 1, 0, 0, 0);
}

void EI_CommandContext::DrawInstanced(EI_PSO& pso, EI_DrawParams& drawParams)
{
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pso.m_pipeline);
    vkCmdDraw(commandBuffer, drawParams.numVertices, drawParams.numInstances, 0, 0);
}

void EI_CommandContext::PushConstants(EI_PSO * pso, int size, void * data)
{
    vkCmdPushConstants(commandBuffer, pso->m_pipelineLayout, VK_SHADER_STAGE_ALL, 0, size, data);
}

void EI_Device::BeginNewCommandBuffer()
{
    assert(m_recording == false);
    m_currentCommandBuffer.commandBuffer = m_commandListRing.GetNewCommandList();

    VkCommandBufferBeginInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(m_currentCommandBuffer.commandBuffer, &info);
    m_recording = true;
}

void EI_Device::BeginNewComputeCommandBuffer()
{
    m_currentComputeCommandBuffer.commandBuffer = m_computeCommandListRing.GetNewCommandList();

    VkCommandBufferBeginInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(m_currentComputeCommandBuffer.commandBuffer, &info);
}

void EI_Device::EndAndSubmitCommandBuffer()
{
    vkEndCommandBuffer(m_currentCommandBuffer.commandBuffer);

    // Close & Submit the command list
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence cmdBufExecutedFence;
    m_swapChain.GetSemaphores(&imageAvailableSemaphore, &renderFinishedSemaphore, &cmdBufExecutedFence);
    m_lastFrameGraphicsCommandBufferFence = cmdBufExecutedFence;

    VkPipelineStageFlags submitWaitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    const VkCommandBuffer cmd_bufs[] = { m_currentCommandBuffer.commandBuffer };
    VkSubmitInfo submit_info;
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = NULL;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = NULL;
    submit_info.pWaitDstStageMask = &submitWaitStage;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = cmd_bufs;
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = NULL;

    vkQueueSubmit(m_device.GetGraphicsQueue(), 1, &submit_info, VK_NULL_HANDLE);
    m_recording = false;
}

void EI_Device::EndAndSubmitCommandBufferWithFences()
{
    vkEndCommandBuffer(m_currentCommandBuffer.commandBuffer);

    // Close & Submit the command list
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence cmdBufExecutedFence;
    m_swapChain.GetSemaphores(&imageAvailableSemaphore, &renderFinishedSemaphore, &cmdBufExecutedFence);
    m_lastFrameGraphicsCommandBufferFence = cmdBufExecutedFence;

    VkPipelineStageFlags submitWaitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    const VkCommandBuffer cmd_bufs[] = { m_currentCommandBuffer.commandBuffer };
    VkSubmitInfo submit_info;
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = NULL;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &imageAvailableSemaphore;
    submit_info.pWaitDstStageMask = &submitWaitStage;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = cmd_bufs;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &renderFinishedSemaphore;

    vkQueueSubmit(m_device.GetGraphicsQueue(), 1, &submit_info, cmdBufExecutedFence);
    m_recording = false;
}

void EI_Device::BeginBackbufferRenderPass()
{
    // THIS FUNCTION SHOULD ONLY EVER BE CALLED ONCE PER FRAME AT THE END OF THE FRAME
    // AS IT RELIES ON A BUNCH OF HARDCODED THINGS SETUP IN THE SWAP CHAIN OF CAULDRON'S
    // INITIALIZATION
    VkClearValue ClearValues[2];

    // Setup clear values
    ClearValues[0].color.float32[0] = 0.0f;
    ClearValues[0].color.float32[1] = 0.0f;
    ClearValues[0].color.float32[2] = 0.0f;
    ClearValues[0].color.float32[3] = 0.0f;
    ClearValues[1].depthStencil.depth = 1.0f;
    ClearValues[1].depthStencil.stencil = 0;

    VkRenderPassBeginInfo rp_begin = {};
    rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rp_begin.pNext = NULL;
    rp_begin.renderPass = m_swapChain.GetRenderPass();
    rp_begin.framebuffer = m_swapChain.GetFramebuffer(m_currentImageIndex);
    rp_begin.renderArea.offset.x = 0;
    rp_begin.renderArea.offset.y = 0;
    rp_begin.renderArea.extent.width = m_width;
    rp_begin.renderArea.extent.height = m_height;
    rp_begin.pClearValues = ClearValues;
    rp_begin.clearValueCount = 2;
    vkCmdBeginRenderPass(m_currentCommandBuffer.commandBuffer, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

    CAULDRON_VK::SetViewportAndScissor(m_currentCommandBuffer.commandBuffer, 0, 0, m_width, m_height);
}

void EI_Device::EndRenderPass()
{
    vkCmdEndRenderPass(m_currentCommandBuffer.commandBuffer);
}

void EI_Device::GetTimeStamp(char * name)
{
    m_gpuTimer.GetTimeStamp(m_currentCommandBuffer.commandBuffer, name);
}

VkDescriptorSetLayoutBinding VulkanDescriptorSetBinding(int binding, EI_ShaderStage stage, EI_ResourceTypeEnum type) {

    VkDescriptorSetLayoutBinding b;

    b.stageFlags = VK_SHADER_STAGE_ALL;
    switch (stage) {
    case EI_VS:
        b.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        break;
    case EI_PS:
        b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        break;
    case EI_CS:
        b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        break;
    case EI_ALL:
        b.stageFlags = VK_SHADER_STAGE_ALL;
    default:
        b.stageFlags = VK_SHADER_STAGE_ALL;
    }
    b.binding = binding;
    if (type == EI_RESOURCETYPE_BUFFER_RO || type == EI_RESOURCETYPE_BUFFER_RW)
    {
        b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    }
    if (type == EI_RESOURCETYPE_IMAGE_RW)
    {
        b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    }
    if (type == EI_RESOURCETYPE_IMAGE_RO)
    {
        b.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    }
    if (type == EI_RESOURCETYPE_UNIFORM )
    {
        b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    }
    if (type == EI_RESOURCETYPE_SAMPLER)
    {
        b.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    }
    b.descriptorCount = 1;
    b.pImmutableSamplers = nullptr;
    return b;
}

std::unique_ptr<EI_BindLayout> EI_Device::CreateLayout(const EI_LayoutDescription& description)
{
    std::vector<VkDescriptorSetLayoutBinding> layoutBindings;

    EI_BindLayout* pLayout = new EI_BindLayout;

    int numResources = (int)description.resources.size();

    for (int i = 0; i < numResources; ++i)
    {
        int binding = description.resources[i].binding;
        if (binding >= 0) {
            layoutBindings.push_back(VulkanDescriptorSetBinding(binding, description.stage, description.resources[i].type));
        }
    }

    VkDescriptorSetLayoutCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.bindingCount = (uint32_t)layoutBindings.size();
    info.pBindings = layoutBindings.data();
    vkCreateDescriptorSetLayout(m_device.GetDevice(), &info, NULL, &pLayout->m_descriptorSetLayout);

    pLayout->layoutBindings = layoutBindings;
    pLayout->description = description;
    return std::unique_ptr<EI_BindLayout>(pLayout);
}

EI_BindLayout::~EI_BindLayout()
{
    vkDestroyDescriptorSetLayout(GetDevice()->GetVulkanDevice(), m_descriptorSetLayout, NULL);
}

void EI_Device::OnBeginFrame(bool bDoAsync)
{
    // Let our resource managers do some house keeping 
    m_computeCommandListRing.OnBeginFrame();
    m_constantBufferRing.OnBeginFrame();

    // if a resize event already started a the command buffer - we need to do it this way,
    // because multiple resizes in one frame could overflow the command buffer pool if we open a new
    // command buffer everytime we resize
    if (m_recording)
    {
        EndAndSubmitCommandBuffer();
        FlushGPU();
    }
    BeginNewCommandBuffer();

    if (bDoAsync)
        BeginNewComputeCommandBuffer();

    m_gpuTimer.OnBeginFrame(m_currentCommandBuffer.commandBuffer, &m_timeStamps);

    std::map<std::string, float> timeStampMap;
    for (int i = 0; i < (int)m_timeStamps.size() - 1; ++i)
    {
        timeStampMap[m_timeStamps[i + 1].m_label] += m_timeStamps[i + 1].m_microseconds - m_timeStamps[i].m_microseconds;
    }
    int i = 0;
    m_sortedTimeStamps.resize(timeStampMap.size());
    for (std::map<std::string, float>::iterator it = timeStampMap.begin(); it != timeStampMap.end(); ++it)
    {
        m_sortedTimeStamps[i].m_label = it->first;
        m_sortedTimeStamps[i].m_microseconds = it->second;
        ++i;
    }

    if (m_timeStamps.size() > 0)
    {
        //scrolling data and average computing
        static float values[128];
        values[127] = (float)(m_timeStamps.back().m_microseconds - m_timeStamps.front().m_microseconds);
        float average = values[0];
        for (uint32_t i = 0; i < 128 - 1; i++) { values[i] = values[i + 1]; average += values[i]; }
        average /= 128;
        m_averageGpuTime = average;
    }
}

void EI_Device::OnEndFrame()
{
    EI_Barrier barrier[] = {
        { m_colorBuffer.get(), EI_STATE_RENDER_TARGET, EI_STATE_SRV }
    };
    m_currentCommandBuffer.SubmitBarrier(AMD_ARRAY_SIZE(barrier), barrier);

    WaitForLastFrameGraphics();
    EndAndSubmitCommandBuffer();

    m_currentImageIndex = m_swapChain.WaitForSwapChain();

    m_commandListRing.OnBeginFrame();

    BeginNewCommandBuffer();
    BeginBackbufferRenderPass();

    // Tonemapping ------------------------------------------------------------------------
    {
        float exposure = 1.0f;
        int toneMapper = 0;
        m_toneMapping.Draw(GetCurrentCommandContext().commandBuffer, m_colorBuffer.get()->srv, exposure, toneMapper, true);
        GetTimeStamp("Tone Mapping");
    }

    // Start by resolving render to swap chain
    // Do UI render over top
    RenderUI();

    // Wrap up
    EndRenderPass();

    {
        EI_Barrier barrier[] = {
            { m_colorBuffer.get(), EI_STATE_SRV, EI_STATE_RENDER_TARGET }
        };
        m_currentCommandBuffer.SubmitBarrier(AMD_ARRAY_SIZE(barrier), barrier);
    }

    m_gpuTimer.OnEndFrame();
    EndAndSubmitCommandBufferWithFences();
    m_swapChain.Present();
}

void EI_Device::RenderUI()
{
    m_ImGUI.Draw(m_currentCommandBuffer.commandBuffer);
}

static EI_Device * g_device = NULL;

EI_Device * GetDevice() {
    if (g_device == NULL) {
        g_device = new EI_Device();
    }
    return g_device;
}

void EI_CommandContext::BindSets(EI_PSO * pso, int numBindSets, EI_BindSet ** bindSets)
{
    assert(numBindSets < 8);
    VkDescriptorSet descSets[8];
    for (int i = 0; i < numBindSets; ++i) {
        descSets[i] = bindSets[i]->m_descriptorSet;
    }
    
    vkCmdBindDescriptorSets(commandBuffer, VulkanBindPoint(pso->m_bp), pso->m_pipelineLayout, 0, numBindSets, descSets, 0, NULL);
}

VkAccessFlags VulkanAccessFlags(EI_ResourceState state)
{
    switch (state)
    {
    case EI_STATE_SRV:
        return VK_ACCESS_SHADER_READ_BIT;
    case EI_STATE_UAV:
        return VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    case EI_STATE_COPY_DEST:
        return VK_ACCESS_TRANSFER_WRITE_BIT;
    case EI_STATE_COPY_SOURCE:
        return VK_ACCESS_TRANSFER_READ_BIT;
    case EI_STATE_RENDER_TARGET:
        return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    case EI_STATE_DEPTH_STENCIL:
        return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    case EI_STATE_INDEX_BUFFER:
        return VK_ACCESS_INDEX_READ_BIT;
    case EI_STATE_CONSTANT_BUFFER:
        return VK_ACCESS_SHADER_READ_BIT;
    default:
        assert(0);
        return 0;
    }
}

EI_Resource::EI_Resource() : 
    m_ResourceType(EI_ResourceType::Undefined),
    m_pBuffer(nullptr)
{
}

EI_Resource::~EI_Resource()
{
    if (m_ResourceType == EI_ResourceType::Buffer && m_pBuffer != nullptr)
    {
        m_pBuffer->Free();
        delete m_pBuffer;
    }
    else if (m_ResourceType == EI_ResourceType::Texture)
    {
        m_pTexture->OnDestroy();
        delete m_pTexture;
    }
    else if (m_ResourceType == EI_ResourceType::Sampler)
    {
        vkDestroySampler(GetDevice()->GetCauldronDevice()->GetDevice(), *m_pSampler, nullptr);
        delete m_pSampler;
    }
    else
    {
        assert(false && "Trying to destroy an undefined resource");
    }
}

VkImageLayout VulkanImageLayout(EI_ResourceState state)
{
    switch (state)
    {
    case EI_STATE_SRV:
        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    case EI_STATE_UAV:
        return VK_IMAGE_LAYOUT_GENERAL;
    case EI_STATE_COPY_DEST:
        return  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    case EI_STATE_COPY_SOURCE:
        return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    case EI_STATE_RENDER_TARGET:
        return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    case EI_STATE_DEPTH_STENCIL:
        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    case EI_STATE_UNDEFINED:
    default:
        return VK_IMAGE_LAYOUT_UNDEFINED;
    }
}

void EI_CommandContext::SubmitBarrier(int numBarriers, EI_Barrier * barriers)
{
    assert(numBarriers < 16);
    VkBufferMemoryBarrier bb[16];
    VkImageMemoryBarrier ib[16];
    int numBufferBarriers = 0;
    int numImageBarriers = 0;
    for (int i = 0; i < numBarriers; ++i)
    {
        if (barriers[i].pResource->m_ResourceType == EI_ResourceType::Buffer)
        {
            bb[numBufferBarriers] = {};
            bb[numBufferBarriers].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            bb[numBufferBarriers].buffer = barriers[i].pResource->m_pBuffer->gpuBuffer;
            bb[numBufferBarriers].srcAccessMask = VulkanAccessFlags(barriers[i].from);
            bb[numBufferBarriers].dstAccessMask = VulkanAccessFlags(barriers[i].to);
            bb[numBufferBarriers].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            bb[numBufferBarriers].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            bb[numBufferBarriers].offset = 0;
            bb[numBufferBarriers].size = barriers[i].pResource->m_pBuffer->totalMemSize;
            numBufferBarriers++;
        }
        else {
            assert(barriers[i].pResource->m_ResourceType == EI_ResourceType::Texture);
            ib[numImageBarriers] = {};
            ib[numImageBarriers].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            ib[numImageBarriers].image = barriers[i].pResource->m_pTexture->Resource();

            // Resources NEED to be created as undefined, but we need to transition them out to actually use them
            if (barriers[i].from == EI_STATE_UNDEFINED)
                ib[numImageBarriers].srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT; // VulkanAccessFlags(barriers[i].from);
            else
                ib[numImageBarriers].srcAccessMask = VulkanAccessFlags(barriers[i].from);

            ib[numImageBarriers].dstAccessMask = VulkanAccessFlags(barriers[i].to);
            ib[numImageBarriers].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            ib[numImageBarriers].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            ib[numImageBarriers].oldLayout = VulkanImageLayout(barriers[i].from);
            ib[numImageBarriers].newLayout = VulkanImageLayout(barriers[i].to);

            bool isDepthImage = barriers[i].pResource->m_pTexture->GetFormat() == VK_FORMAT_D32_SFLOAT;

            VkImageSubresourceRange subresourceRange = {};
            subresourceRange.aspectMask = isDepthImage ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
            subresourceRange.baseMipLevel = 0;
            subresourceRange.levelCount = 1;
            subresourceRange.layerCount = barriers[i].pResource->m_pTexture->GetArraySize();

            ib[numImageBarriers].subresourceRange = subresourceRange;
            numImageBarriers++;
        }
    }
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, numBufferBarriers, bb, numImageBarriers, ib);
}

void EI_CommandContext::BindPSO( EI_PSO * pso)
{
    vkCmdBindPipeline(commandBuffer, VulkanBindPoint(pso->m_bp), pso->m_pipeline);
}

void EI_CommandContext::Dispatch(int numGroups)
{
    vkCmdDispatch(commandBuffer, numGroups, 1, 1);
}

void EI_CommandContext::UpdateBuffer(EI_Resource * res, void * data)
{
    assert(res->m_ResourceType == EI_ResourceType::Buffer);
    memcpy(res->m_pBuffer->cpuMappedMemory, data, res->m_pBuffer->totalMemSize);
    VkBufferCopy region;
    region.srcOffset = 0;
    region.dstOffset = 0;
    region.size = res->m_pBuffer->totalMemSize;

    vkCmdCopyBuffer(commandBuffer, res->m_pBuffer->cpuBuffer, res->m_pBuffer->gpuBuffer, 1, &region);
}

void EI_CommandContext::ClearUint32Image(EI_Resource* res, uint32_t value)
{
    assert(res->m_ResourceType == EI_ResourceType::Texture);
    VkClearColorValue clearValue;
    clearValue.uint32[0] = value;
    clearValue.uint32[1] = value;
    clearValue.uint32[2] = value;
    clearValue.uint32[3] = value;
    VkImageSubresourceRange range;
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseArrayLayer = 0;
    range.baseMipLevel = 0;
    range.layerCount = res->m_pTexture->GetArraySize();
    range.levelCount = 1;
    vkCmdClearColorImage(commandBuffer, res->m_pTexture->Resource(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearValue, 1, &range);
}

void EI_CommandContext::ClearFloat32Image(EI_Resource* res, float value)
{
    assert(res->m_ResourceType == EI_ResourceType::Texture);
    VkClearColorValue clearValue;
    clearValue.float32[0] = value;
    clearValue.float32[1] = value;
    clearValue.float32[2] = value;
    clearValue.float32[3] = value;
    VkImageSubresourceRange range;
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseArrayLayer = 0;
    range.baseMipLevel = 0;
    range.layerCount = res->m_pTexture->GetArraySize();
    range.levelCount = 1;
    vkCmdClearColorImage(commandBuffer, res->m_pTexture->Resource(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearValue, 1, &range);
}
