/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceStreamingImagePool.h>
#include <AzCore/std/parallel/mutex.h>

namespace AZ
{
    namespace Vulkan
    {
        class Device;

        class StreamingImagePool final
            : public RHI::DeviceStreamingImagePool
        {
            using Base = RHI::DeviceStreamingImagePool;
            friend class Image;

        public:
            AZ_CLASS_ALLOCATOR(StreamingImagePool, AZ::SystemAllocator);
            AZ_RTTI(StreamingImagePool, "0C123A3C-FBB7-4908-81A5-150D1DFE728A", Base);

            static RHI::Ptr<StreamingImagePool> Create();
            ~StreamingImagePool() = default;

        protected:
            //! Allocate multiple memory blocks.
            //! All allocations use the same memory requeriments
            RHI::ResultCode AllocateMemoryBlocks(
                uint32_t blockCount,
                const VkMemoryRequirements& memReq,
                AZStd::vector<RHI::Ptr<VulkanMemoryAllocation>>& outAllocatedBlocks);

            //! DeAllocate memory blocks
            void DeAllocateMemoryBlocks(AZStd::vector<RHI::Ptr<VulkanMemoryAllocation>>& blocks);

        private:
            StreamingImagePool() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceStreamingImagePool
            RHI::ResultCode InitInternal(RHI::Device& deviceBase, const RHI::StreamingImagePoolDescriptor& descriptor) override;
            RHI::ResultCode InitImageInternal(const RHI::DeviceStreamingImageInitRequest& request) override;
            RHI::ResultCode ExpandImageInternal(const RHI::DeviceStreamingImageExpandRequest& request) override;
            RHI::ResultCode TrimImageInternal(RHI::DeviceImage& image, uint32_t targetMipLevel) override;
            RHI::ResultCode SetMemoryBudgetInternal(size_t newBudget) override;
            bool SupportTiledImageInternal() const override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceResourcePool
            void ShutdownInternal() override;
            void ShutdownResourceInternal(RHI::DeviceResource& resourceBase) override;
            /// As streaming image tiles are allocated from a pool allocator, fragmentation will remain 0 for
            /// the streaming image pool
            void ComputeFragmentation() const override;
            //////////////////////////////////////////////////////////////////////////

            void WaitFinishUploading(const Image& image);

            bool m_enableTileResource = false;
        };
    }
}
