/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <RHI/ImagePool.h>
#include <RHI/CommandList.h>
#include <RHI/Device.h>
#include <RHI/ResourcePoolResolver.h>

namespace AZ::WebGPU
{
    class Image;

    //! Provides a way to update an image content using a staging buffer.
    //! The ImagePoolResolver is part of an ImagePool and the Resolve function must be called
    //! from a scope in order to execute the image updates.
    class ImagePoolResolver final
        : public ResourcePoolResolver
    {
        using Base = RHI::ResourcePoolResolver;

    public:
        AZ_RTTI(ImagePoolResolver, "{9CD9908E-4F2E-4C9D-912A-D6E451B116E3}", Base);
        AZ_CLASS_ALLOCATOR(ImagePoolResolver, AZ::SystemAllocator);

        ImagePoolResolver(Device& device);

        /// Uploads new content to an image subresource.
        RHI::ResultCode UpdateImage(const RHI::DeviceImageUpdateRequest& request, size_t& bytesTransferred);

        //////////////////////////////////////////////////////////////////////
        ///ResourcePoolResolver
        void Resolve(CommandList& commandList) override;
        void Deactivate() override;
        void OnResourceShutdown(const RHI::DeviceResource& resource) override;
        //////////////////////////////////////////////////////////////////////

    private:
        struct ImageUploadPacket
        {
            Image* m_destinationImage = nullptr;
            RHI::Ptr<Buffer> m_stagingBuffer;
            RHI::DeviceImageSubresourceLayout m_subresourceLayout;
            RHI::ImageSubresource m_subresource;
            RHI::Origin m_offset;
        };
        
        AZStd::mutex m_uploadPacketsLock;
        AZStd::vector<ImageUploadPacket> m_uploadPackets;
    };
}
