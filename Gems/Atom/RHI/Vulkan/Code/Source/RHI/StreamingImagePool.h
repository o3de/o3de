/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceStreamingImagePool.h>
#include <Atom/RHI.Reflect/Vulkan/ImagePoolDescriptor.h>
#include <RHI/MemoryAllocator.h>
#include <AzCore/std/parallel/mutex.h>

namespace AZ
{
    namespace Vulkan
    {
        class Device;
        class Image;

        class StreamingImagePool final
            : public RHI::DeviceStreamingImagePool
        {
            using Base = RHI::DeviceStreamingImagePool;

        public:
            AZ_CLASS_ALLOCATOR(StreamingImagePool, AZ::SystemAllocator, 0);
            AZ_RTTI(StreamingImagePool, "0C123A3C-FBB7-4908-81A5-150D1DFE728A", Base);

            static RHI::Ptr<StreamingImagePool> Create();
            ~StreamingImagePool() = default;

        private:
            StreamingImagePool() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::StreamingImagePool
            RHI::ResultCode InitInternal(RHI::Device& deviceBase, const RHI::StreamingImagePoolDescriptor& descriptor) override;
            RHI::ResultCode InitImageInternal(const RHI::DeviceStreamingImageInitRequest& request) override;
            RHI::ResultCode ExpandImageInternal(const RHI::DeviceStreamingImageExpandRequest& request) override;
            RHI::ResultCode TrimImageInternal(RHI::DeviceImage& image, uint32_t targetMipLevel) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceResourcePool
            void ShutdownInternal() override;
            void ShutdownResourceInternal(RHI::DeviceResource& resourceBase) override;
            /// As streaming image tiles are allocated from a pool allocator, fragmentation will remain 0 for
            /// the streaming image pool
            void ComputeFragmentation() const override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // FrameSchedulerEventBus::Handler
            void OnFrameEnd() override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // RHI::Object
            void SetNameInternal(const AZStd::string_view& name) override;
            //////////////////////////////////////////////////////////////////////////

            VkMemoryRequirements GetMemoryRequirements(const RHI::ImageDescriptor& imageDescriptor, uint32_t residentMipLevel);
            void WaitFinishUploading(const Image& image);

            MemoryAllocator m_memoryAllocator;
            RHI::HeapMemoryUsage m_memoryAllocatorUsage;
        };
    }
}
