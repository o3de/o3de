/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RHI/StreamingImagePool.h>
#include <RHI/StreamingImagePoolResolver.h>

namespace AZ
{
    namespace Metal
    {
        RHI::ResultCode StreamingImagePoolResolver::UpdateImage(const RHI::DeviceStreamingImageExpandRequest& request)
        {
            AZ_PROFILE_FUNCTION(RHI);
            
            Image* image = static_cast<Image*>(request.m_image.get());
            const RHI::ImageDescriptor& imageDescriptor = image->GetDescriptor();
            
            for (uint32_t arraySlice = 0; arraySlice < imageDescriptor.m_arraySize; ++arraySlice)
            {
                for (uint32_t mipSliceLocal = 0; mipSliceLocal < static_cast<uint32_t>(request.m_mipSlices.size()); ++mipSliceLocal)
                {
                    const RHI::StreamingImageMipSlice& mipSliceData = request.m_mipSlices[mipSliceLocal];
                    const RHI::StreamingImageSubresourceData& subresourceDataSource = mipSliceData.m_subresources[arraySlice];
                    const RHI::DeviceImageSubresourceLayout& subImageLayout = request.m_mipSlices[mipSliceLocal].m_subresourceLayout;
                    
                    MTLRegion mtlRegion = MTLRegionMake3D(0, 0, 0, subImageLayout.m_size.m_width, subImageLayout.m_size.m_height, subImageLayout.m_size.m_depth);
                    
                    int newMipLevel = CalculateMipLevel(image->GetDescriptor().m_size.m_width, subImageLayout.m_size.m_width);
                    
                    uint32_t bytesPerRow  = subImageLayout.m_bytesPerRow;
                    uint32_t bytesPerImage  = subImageLayout.m_bytesPerImage;
                    
                    //Metal requires these to be 0 for PVR formats
                    if(imageDescriptor.m_format >= RHI::Format::PVRTC2_UNORM && imageDescriptor.m_format <= RHI::Format::PVRTC4_UNORM_SRGB )
                    {
                        bytesPerRow = 0;
                        bytesPerImage = 0;
                    }
                    
                    //Update the video memory with the data
                    [image->GetMemoryView().GetGpuAddress<id<MTLTexture>>() replaceRegion: mtlRegion
                                                                              mipmapLevel: newMipLevel
                                                                                    slice: arraySlice
                                                                                withBytes: subresourceDataSource.m_data
                                                                              bytesPerRow: bytesPerRow
                                                                            bytesPerImage: bytesPerImage];
                }
            }
            
            //GFX TODO][ATOM-436]: if the texture is created in the gpu only private memory add it to the staging queue
            return RHI::ResultCode::Success;
        }
        
        void StreamingImagePoolResolver::Compile()
        {
        }
        
        void StreamingImagePoolResolver::Resolve(CommandList& commandList) const
        {
            //GFX TODO][ATOM-436]: blit the data into a destination texture with a Private mode.
        }
        
        void StreamingImagePoolResolver::Deactivate()
        {
        }
        
        int StreamingImagePoolResolver::CalculateMipLevel(int lowestMipLength, int currentMipLength)
        {
            int mipNum = 0;
            while((lowestMipLength>>mipNum) > currentMipLength)
            {
                mipNum++;
            }
            return mipNum;
        }
    }
}
