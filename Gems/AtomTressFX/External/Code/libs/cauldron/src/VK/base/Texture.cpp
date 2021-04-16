// AMD AMDUtils code
// 
// Copyright(c) 2018 Advanced Micro Devices, Inc.All rights reserved.
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

#include "stdafx.h"
#include "ResourceViewHeaps.h"
#include "Texture.h"
#include "Misc/Misc.h"
#include "Misc/DDSLoader.h"
#include "Misc/DxgiFormatHelper.h"

namespace CAULDRON_VK
{
    VkFormat TranslateDxgiFormatIntoVulkans(DXGI_FORMAT format);

    //--------------------------------------------------------------------------------------
    // Constructor of the Texture class
    // initializes all members
    //--------------------------------------------------------------------------------------
    Texture::Texture() {}
    
    //--------------------------------------------------------------------------------------
    // Destructor of the Texture class
    //--------------------------------------------------------------------------------------
    Texture::~Texture() {}

    void Texture::OnDestroy()
    {
#ifdef USE_VMA
        if (m_pResource != VK_NULL_HANDLE)
        {
            vmaDestroyImage(m_pDevice->GetAllocator(), m_pResource, m_ImageAlloc);
            m_pResource = VK_NULL_HANDLE;
        }
#else
        if (m_deviceMemory != VK_NULL_HANDLE)
        {
            vkDestroyImage(m_pDevice->GetDevice(), m_pResource, nullptr);
            vkFreeMemory(m_pDevice->GetDevice(), m_deviceMemory, nullptr);
            m_deviceMemory = VK_NULL_HANDLE;
        }
#endif

    }

    bool Texture::isCubemap() const
    {
        return m_header.arraySize == 6;
    }

    INT32 Texture::Init(Device *pDevice, VkImageCreateInfo *pCreateInfo, char *name)
    {
        m_pDevice = pDevice;
        m_header.mipMapCount = pCreateInfo->mipLevels;
        m_header.width = pCreateInfo->extent.width;
        m_header.height = pCreateInfo->extent.height;
        m_header.depth = pCreateInfo->extent.depth;
        m_header.arraySize = pCreateInfo->arrayLayers;
        m_format = pCreateInfo->format;

#ifdef USE_VMA
        VmaAllocationCreateInfo imageAllocCreateInfo = {};
        imageAllocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        imageAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
        imageAllocCreateInfo.pUserData = name;
        VmaAllocationInfo gpuImageAllocInfo = {};
        vmaCreateImage(m_pDevice->GetAllocator(), pCreateInfo, &imageAllocCreateInfo, &m_pResource, &m_ImageAlloc, &gpuImageAllocInfo);
#else
        /* Create image */
        VkResult res = vkCreateImage(m_pDevice->GetDevice(), pCreateInfo, NULL, &m_pResource);
        assert(res == VK_SUCCESS);

        VkMemoryRequirements mem_reqs;
        vkGetImageMemoryRequirements(m_pDevice->GetDevice(), m_pResource, &mem_reqs);

        VkMemoryAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.pNext = NULL;
        alloc_info.allocationSize = 0;
        alloc_info.allocationSize = mem_reqs.size;
        alloc_info.memoryTypeIndex = 0;

        bool pass = memory_type_from_properties(m_pDevice->GetPhysicalDeviceMemoryProperties(), mem_reqs.memoryTypeBits,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            &alloc_info.memoryTypeIndex);
        assert(pass);

        /* Allocate memory */
        res = vkAllocateMemory(m_pDevice->GetDevice(), &alloc_info, NULL, &m_deviceMemory);
        assert(res == VK_SUCCESS);

        /* bind memory */
        res = vkBindImageMemory(m_pDevice->GetDevice(), m_pResource, m_deviceMemory, 0);
        assert(res == VK_SUCCESS);
#endif
        return 0;
    }

    INT32 Texture::InitRendertarget(Device *pDevice, uint32_t width, uint32_t height, VkFormat format, VkSampleCountFlagBits msaa, VkImageUsageFlags usage, bool bUAV, char *name)
    {
        VkImageCreateInfo image_info = {};
        image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_info.pNext = NULL;
        image_info.imageType = VK_IMAGE_TYPE_2D;
        image_info.format = format;
        image_info.extent.width = width;
        image_info.extent.height = height;
        image_info.extent.depth = 1;
        image_info.mipLevels = 1;
        image_info.arrayLayers = 1;
        image_info.samples = msaa;
        image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image_info.queueFamilyIndexCount = 0;
        image_info.pQueueFamilyIndices = NULL;
        image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        image_info.usage = usage; //TODO    
        image_info.flags = 0;
        image_info.tiling = VK_IMAGE_TILING_OPTIMAL;   // VK_IMAGE_TILING_LINEAR should never be used and will never be faster

        return Init(pDevice, &image_info, name);
    }

    void Texture::CreateRTV(VkImageView *pImageView, int mipLevel)
    {
        VkImageViewCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        info.image = m_pResource;
        info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        if (m_header.arraySize > 1)
        {
            info.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
            info.subresourceRange.layerCount = m_header.arraySize;
        }
        else
        {
            info.subresourceRange.layerCount = 1;
        }
        info.format = m_format;
        if (m_format == VK_FORMAT_D32_SFLOAT)
            info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        else
            info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

        if (mipLevel == -1)
        {
            info.subresourceRange.baseMipLevel = 0;
            info.subresourceRange.levelCount = m_header.mipMapCount;
        }
        else
        {
            info.subresourceRange.baseMipLevel = mipLevel;
            info.subresourceRange.levelCount = 1;
        }

        info.subresourceRange.baseArrayLayer = 0;
        VkResult res = vkCreateImageView(m_pDevice->GetDevice(), &info, NULL, pImageView);
        assert(res == VK_SUCCESS);
    }

    void Texture::CreateSRV(VkImageView *pImageView, int mipLevel)
    {
        VkImageViewCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        info.image = m_pResource;
        info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        if (m_header.arraySize > 1)
        {
            info.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
            info.subresourceRange.layerCount = m_header.arraySize;
        }
        else
        {
            info.subresourceRange.layerCount = 1;
        }
        info.format = m_format;
        if (m_format == VK_FORMAT_D32_SFLOAT)
            info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        else
            info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

        if (mipLevel == -1)
        {
            info.subresourceRange.baseMipLevel = 0;
            info.subresourceRange.levelCount = m_header.mipMapCount;
        }
        else
        {
            info.subresourceRange.baseMipLevel = mipLevel;
            info.subresourceRange.levelCount = 1;
        }

        info.subresourceRange.baseArrayLayer = 0;

        VkResult res = vkCreateImageView(m_pDevice->GetDevice(), &info, NULL, pImageView);
        assert(res == VK_SUCCESS);
    }

    void Texture::CreateCubeSRV(VkImageView *pImageView)
    {
        VkImageViewCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        info.image = m_pResource;
        info.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
        info.format = m_format;
        info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        info.subresourceRange.baseMipLevel = 0;
        info.subresourceRange.levelCount = m_header.mipMapCount;
        info.subresourceRange.baseArrayLayer = 0;
        info.subresourceRange.layerCount = m_header.arraySize;

        VkResult res = vkCreateImageView(m_pDevice->GetDevice(), &info, NULL, pImageView);
        assert(res == VK_SUCCESS);
    }

    void Texture::CreateDSV(VkImageView *pImageView)
    {
        VkImageViewCreateInfo view_info = {};
        view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.pNext = NULL;
        view_info.image = m_pResource;
        view_info.format = m_format;
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        view_info.subresourceRange.baseMipLevel = 0;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount = 1;
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;

        if (m_format == VK_FORMAT_D16_UNORM_S8_UINT || m_format == VK_FORMAT_D24_UNORM_S8_UINT || m_format == VK_FORMAT_D32_SFLOAT_S8_UINT)
        {
            view_info.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }

        m_header.mipMapCount = 1;

        VkResult res = vkCreateImageView(m_pDevice->GetDevice(), &view_info, NULL, pImageView);
        assert(res == VK_SUCCESS);
    }

    INT32 Texture::InitDepthStencil(Device *pDevice, uint32_t width, uint32_t height, VkFormat format, VkSampleCountFlagBits msaa, char *name)
    {
        VkImageCreateInfo image_info = {};
        image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_info.pNext = NULL;
        image_info.imageType = VK_IMAGE_TYPE_2D;
        image_info.format = format;
        image_info.extent.width = width;
        image_info.extent.height = height;
        image_info.extent.depth = 1;
        image_info.mipLevels = 1;
        image_info.arrayLayers = 1;
        image_info.samples = msaa;
        image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image_info.queueFamilyIndexCount = 0;
        image_info.pQueueFamilyIndices = NULL;
        image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT; //TODO    
        image_info.flags = 0;
        image_info.tiling = VK_IMAGE_TILING_OPTIMAL;   // VK_IMAGE_TILING_LINEAR should never be used and will never be faster

        return Init(pDevice, &image_info, name);
    }

    //--------------------------------------------------------------------------------------
    // create a comitted resource using m_header
    //--------------------------------------------------------------------------------------
    VkImage Texture::CreateTextureCommitted(Device* pDevice, UploadHeap* pUploadHeap, const char *pName, bool useSRGB)
    {
        m_header.format = SetFormatGamma(m_header.format, useSRGB);

        m_format = TranslateDxgiFormatIntoVulkans((DXGI_FORMAT)m_header.format);

        VkImage tex;

        // Create the Image:
        {
            VkImageCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            info.imageType = VK_IMAGE_TYPE_2D;
            info.format = m_format;
            info.extent.width = m_header.width;
            info.extent.height = m_header.height;
            info.extent.depth = 1;
            info.mipLevels = m_header.mipMapCount;
            info.arrayLayers = m_header.arraySize;
            if (m_header.arraySize == 6)
                info.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
            info.samples = VK_SAMPLE_COUNT_1_BIT;
            info.tiling = VK_IMAGE_TILING_OPTIMAL;
            info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            // allocate memory and bind the image to it
#ifdef USE_VMA
            VmaAllocationCreateInfo imageAllocCreateInfo = {};
            imageAllocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
            imageAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
            imageAllocCreateInfo.pUserData = (char *)pName;
            VmaAllocationInfo gpuImageAllocInfo = {};
            vmaCreateImage(pDevice->GetAllocator(), &info, &imageAllocCreateInfo, &tex, &m_ImageAlloc, &gpuImageAllocInfo);
#else
            VkResult res = vkCreateImage(pDevice->GetDevice(), &info, NULL, &tex);
            assert(res == VK_SUCCESS);

            VkMemoryRequirements mem_reqs;
            vkGetImageMemoryRequirements(pDevice->GetDevice(), tex, &mem_reqs);

            VkMemoryAllocateInfo alloc_info = {};
            alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            alloc_info.allocationSize = mem_reqs.size;
            alloc_info.memoryTypeIndex = 0;

            bool pass = memory_type_from_properties(pDevice->GetPhysicalDeviceMemoryProperties(), mem_reqs.memoryTypeBits,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                &alloc_info.memoryTypeIndex);
            assert(pass && "No mappable, coherent memory");

            res = vkAllocateMemory(pDevice->GetDevice(), &alloc_info, NULL, &m_deviceMemory);
            assert(res == VK_SUCCESS);

            res = vkBindImageMemory(pDevice->GetDevice(), tex, m_deviceMemory, 0);
            assert(res == VK_SUCCESS);
#endif
        }

        return tex;
    }

    void Texture::LoadAndUpload(Device* pDevice, UploadHeap* pUploadHeap, ImgLoader *pDds, VkImage pTexture2D)
    {
        // Upload Image
        {
            VkImageMemoryBarrier copy_barrier = {};
            copy_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            copy_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            copy_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            copy_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            copy_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            copy_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            copy_barrier.image = pTexture2D;
            copy_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copy_barrier.subresourceRange.baseMipLevel = 0;
            copy_barrier.subresourceRange.levelCount = m_header.mipMapCount;
            copy_barrier.subresourceRange.layerCount = m_header.arraySize;
            vkCmdPipelineBarrier(pUploadHeap->GetCommandList(), VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &copy_barrier);
        }

        //compute pixel size
        //
        UINT32 bytePP = m_header.bitCount / 8;
        if ((m_header.format >= DXGI_FORMAT_BC1_TYPELESS) && (m_header.format <= DXGI_FORMAT_BC5_SNORM))
        {
            bytePP = (UINT32)GetPixelByteSize((DXGI_FORMAT)m_header.format);
        }

        for (uint32_t a = 0; a < m_header.arraySize; a++)
        {
            // copy all the mip slices into the offsets specified by the footprint structure
            //
            for (uint32_t mip = 0; mip < m_header.mipMapCount; mip++)
            {
                uint32_t dwWidth = std::max<uint32_t>(m_header.width >> mip, 1);
                uint32_t dwHeight = std::max<uint32_t>(m_header.height >> mip, 1);

                UINT8* pixels = NULL;
                UINT64 UplHeapSize = dwWidth*dwHeight * 4;
                pixels = pUploadHeap->Suballocate(UplHeapSize, 512);
                if (pixels == NULL)
                {
                    // oh! We ran out of mem in the upload heap, flush it and try allocating mem from it again
                    pUploadHeap->FlushAndFinish();
                    pixels = pUploadHeap->Suballocate(SIZE_T(UplHeapSize), 512);
                    assert(pixels != NULL);
                }

                uint32_t offset = uint32_t(pixels - pUploadHeap->BasePtr());

                pDds->CopyPixels(pixels, dwWidth * bytePP, dwWidth * bytePP, dwHeight);

                {
                    VkBufferImageCopy region = {};
                    region.bufferOffset = offset;
                    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    region.imageSubresource.layerCount = 1;
                    region.imageSubresource.baseArrayLayer = a;
                    region.imageSubresource.mipLevel = mip;
                    region.imageExtent.width = dwWidth;
                    region.imageExtent.height = dwHeight;
                    region.imageExtent.depth = 1;
                    vkCmdCopyBufferToImage(pUploadHeap->GetCommandList(), pUploadHeap->GetResource(), pTexture2D, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
                }
            }
        }

        // prepare to shader read
        //
        {
            VkImageMemoryBarrier use_barrier = {};
            use_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            use_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            use_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            use_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            use_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            use_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            use_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            use_barrier.image = pTexture2D;
            use_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            use_barrier.subresourceRange.levelCount = m_header.mipMapCount;
            use_barrier.subresourceRange.layerCount = m_header.arraySize;
            vkCmdPipelineBarrier(pUploadHeap->GetCommandList(), VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &use_barrier);
        }
    }

    //--------------------------------------------------------------------------------------
    // entry function to initialize an image from a .DDS texture
    //--------------------------------------------------------------------------------------
    bool Texture::InitFromFile(Device* pDevice, UploadHeap* pUploadHeap, const char *pFilename, bool useSRGB, float cutOff)
    {
        m_pDevice = pDevice;
        assert(m_pResource == NULL);

        ImgLoader *img = GetImageLoader(pFilename);
        bool result = img->Load(pFilename, cutOff, &m_header);
        if (result)
        {
            m_pResource = CreateTextureCommitted(pDevice, pUploadHeap, pFilename, useSRGB);
            LoadAndUpload(pDevice, pUploadHeap, img, m_pResource);
        }

        delete(img);

        return result;
    }

    VkFormat TranslateDxgiFormatIntoVulkans(DXGI_FORMAT format)
    {
        switch (format)
        {
            case DXGI_FORMAT_B8G8R8A8_UNORM: return VK_FORMAT_B8G8R8A8_UNORM;
            case DXGI_FORMAT_R8G8B8A8_UNORM: return VK_FORMAT_R8G8B8A8_UNORM;
            case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return VK_FORMAT_R8G8B8A8_SRGB;
            case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: return VK_FORMAT_B8G8R8A8_SRGB;
            case DXGI_FORMAT_BC1_UNORM: return VK_FORMAT_BC1_RGB_UNORM_BLOCK;
            case DXGI_FORMAT_BC2_UNORM: return VK_FORMAT_BC2_UNORM_BLOCK;
            case DXGI_FORMAT_BC3_UNORM: return VK_FORMAT_BC3_UNORM_BLOCK;
            case DXGI_FORMAT_BC4_UNORM: return VK_FORMAT_BC4_UNORM_BLOCK;
            case DXGI_FORMAT_BC4_SNORM: return VK_FORMAT_BC4_UNORM_BLOCK;
            case DXGI_FORMAT_BC5_UNORM: return VK_FORMAT_BC5_UNORM_BLOCK;
            case DXGI_FORMAT_BC5_SNORM: return VK_FORMAT_BC5_UNORM_BLOCK;
            case DXGI_FORMAT_BC1_UNORM_SRGB: return VK_FORMAT_BC1_RGB_SRGB_BLOCK;
            case DXGI_FORMAT_BC2_UNORM_SRGB: return VK_FORMAT_BC2_SRGB_BLOCK;
            case DXGI_FORMAT_BC3_UNORM_SRGB: return VK_FORMAT_BC3_SRGB_BLOCK;
            case DXGI_FORMAT_R10G10B10A2_UNORM: return VK_FORMAT_A2R10G10B10_UNORM_PACK32;
            case DXGI_FORMAT_R16G16B16A16_FLOAT: return VK_FORMAT_R16G16B16A16_SFLOAT;            
            default: assert(false);  return VK_FORMAT_UNDEFINED;
        }
    }
}