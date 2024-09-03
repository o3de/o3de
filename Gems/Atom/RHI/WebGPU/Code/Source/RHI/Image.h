/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceImage.h>
#include <AzCore/Memory/PoolAllocator.h>

namespace AZ
{
    namespace WebGPU
    {
        class Image final
            : public RHI::DeviceImage
        {
            using Base = RHI::DeviceImage;
        public:
            AZ_CLASS_ALLOCATOR(Image, AZ::ThreadPoolAllocator);
            AZ_RTTI(Image, "{7D518841-59D5-4545-BFD9-9824ECD52664}", Base);
            ~Image() = default;
            
            static RHI::Ptr<Image> Create();
  
        private:
            Image() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::Object
            void SetNameInternal([[maybe_unused]] const AZStd::string_view& name) override {}
            //////////////////////////////////////////////////////////////////////////
            
            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceImage
            void GetSubresourceLayoutsInternal(
                [[maybe_unused]] const RHI::ImageSubresourceRange& subresourceRange,
                [[maybe_unused]] RHI::DeviceImageSubresourceLayout* subresourceLayouts,
                [[maybe_unused]] size_t* totalSizeInBytes) const override
            {
            }
            //////////////////////////////////////////////////////////////////////////
        };
    }
}
