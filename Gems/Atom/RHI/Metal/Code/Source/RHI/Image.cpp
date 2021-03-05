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
#include "Atom_RHI_Metal_precompiled.h"

#include <RHI/Conversions.h>
#include <RHI/Device.h>
#include <RHI/Image.h>

namespace AZ
{
    namespace Metal
    {
        RHI::Ptr<Image> Image::Create()
        {
            return aznew Image();
        }
        
        const MemoryView& Image::GetMemoryView() const
        {
            return m_memoryView;
        }
        
        MemoryView& Image::GetMemoryView()
        {
            return m_memoryView;
        }
        
        void Image::SetNameInternal(const AZStd::string_view& name)
        {
            m_memoryView.SetName(name);
        }
    
        void Image::GetSubresourceLayoutsInternal(const RHI::ImageSubresourceRange& subresourceRange,
                    RHI::ImageSubresourceLayoutPlaced* subresourceLayouts,
                    size_t* totalSizeInBytes) const
        {
            const RHI::ImageDescriptor& imageDescriptor = GetDescriptor();
            size_t subresourceByteCount = 0;
            const uint32_t offsetAligment = 4;
            for (uint16_t arraySlice = subresourceRange.m_arraySliceMin; arraySlice <= subresourceRange.m_mipSliceMax; ++arraySlice)
            {
                for (uint16_t mipSlice = subresourceRange.m_mipSliceMin; mipSlice <= subresourceRange.m_mipSliceMax; ++mipSlice)
                {
                    RHI::ImageSubresource subresource;
                    subresource.m_arraySlice = arraySlice;
                    subresource.m_mipSlice = mipSlice;
                    RHI::ImageSubresourceLayout subresourceLayout = RHI::GetImageSubresourceLayout(imageDescriptor, subresource);

                    if (subresourceLayouts)
                    {
                        const uint32_t subresourceIndex = RHI::GetImageSubresourceIndex(mipSlice, arraySlice, imageDescriptor.m_mipLevels);
                        RHI::ImageSubresourceLayoutPlaced& layout = subresourceLayouts[subresourceIndex];
                        layout.m_bytesPerRow = subresourceLayout.m_bytesPerRow;
                        layout.m_bytesPerImage = subresourceLayout.m_rowCount * subresourceLayout.m_bytesPerRow;
                        layout.m_offset = subresourceByteCount;
                        layout.m_rowCount = subresourceLayout.m_rowCount;
                        layout.m_size = subresourceLayout.m_size;
                    }

                    subresourceByteCount = RHI::AlignUp(subresourceByteCount + subresourceLayout.m_bytesPerImage * subresourceLayout.m_size.m_depth, offsetAligment);
                }
            }
            
            if (totalSizeInBytes)
            {
                *totalSizeInBytes = subresourceByteCount;
            }
        }
    
        uint32_t Image::GetStreamedMipLevel() const
        {
            return m_streamedMipLevel;
        }
        
        void Image::SetStreamedMipLevel(uint32_t streamedMipLevel)
        {
            m_streamedMipLevel = streamedMipLevel;
        }
    
        size_t Image::GetResidentSizeInBytes() const
        {
            return m_residentSizeInBytes;
        }

        void Image::SetResidentSizeInBytes(size_t sizeInBytes)
        {
            m_residentSizeInBytes = sizeInBytes;
        }
    
        void Image::SetUploadHandle(const RHI::AsyncWorkHandle& handle)
        {
           m_uploadHandle = handle;
        }

        const RHI::AsyncWorkHandle& Image::GetUploadHandle() const
        {
           return m_uploadHandle;
        }
    
        void Image::FinalizeAsyncUpload(uint32_t newStreamedMipLevels)
        {
            AZ_Assert(newStreamedMipLevels <= m_streamedMipLevel, "Expanded mip levels can't be more than streamed mip level");

            if (newStreamedMipLevels < m_streamedMipLevel)
            {
                m_streamedMipLevel = newStreamedMipLevels;
                InvalidateViews();
            }
        }
    
        bool Image::IsSwapChainTexture() const
        {
            return m_isSwapChainImage;
        }
    }
}
