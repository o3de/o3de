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
#pragma once

#include <AzCore/std/parallel/atomic.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <Atom/RHI/ImagePool.h>

namespace AZ
{
    namespace DX12
    {
        class Device;
        class ImagePoolResolver;

        class ImagePool final
            : public RHI::ImagePool
        {
            using Base = RHI::ImagePool;
        public:
            AZ_CLASS_ALLOCATOR(ImagePool, AZ::SystemAllocator, 0);
            AZ_RTTI(ImagePool, "{084A02C0-DBCB-4285-B79E-842B49292B5E}", RHI::ImagePool);

            static RHI::Ptr<ImagePool> Create();

            Device& GetDevice() const;

        private:
            ImagePool() = default;

            ImagePoolResolver* GetResolver();

            void AddMemoryUsage(size_t bytesToAdd);
            void SubtractMemoryUsage(size_t bytesToSubtract);

            //////////////////////////////////////////////////////////////////////////
            // RHI::ImagePool
            RHI::ResultCode InitInternal(RHI::Device&, const RHI::ImagePoolDescriptor&) override;
            RHI::ResultCode InitImageInternal(const RHI::ImageInitRequest& request) override;
            RHI::ResultCode UpdateImageContentsInternal(const RHI::ImageUpdateRequest& request) override;
            void ShutdownResourceInternal(RHI::Resource& resourceBase) override;
            //////////////////////////////////////////////////////////////////////////
        };
    }
}
