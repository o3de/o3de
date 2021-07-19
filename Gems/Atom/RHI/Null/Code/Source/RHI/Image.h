/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/Image.h>
#include <AzCore/Memory/PoolAllocator.h>

namespace AZ
{
    namespace Null
    {
        class Image final
            : public RHI::Image
        {
            using Base = RHI::Image;
        public:
            AZ_CLASS_ALLOCATOR(Image, AZ::ThreadPoolAllocator, 0);
            AZ_RTTI(Image, "{2AA22D3F-521B-4058-92F2-CEBBD2891D6C}", Base);
            ~Image() = default;
            
            static RHI::Ptr<Image> Create();
  
        private:
            Image() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::Object
            void SetNameInternal([[maybe_unused]] const AZStd::string_view& name) override {}
            //////////////////////////////////////////////////////////////////////////
            
            //////////////////////////////////////////////////////////////////////////
            // RHI::Image
            void GetSubresourceLayoutsInternal(
                [[maybe_unused]] const RHI::ImageSubresourceRange& subresourceRange,
                [[maybe_unused]] RHI::ImageSubresourceLayoutPlaced* subresourceLayouts,
                [[maybe_unused]] size_t* totalSizeInBytes) const override
            {
            }
            //////////////////////////////////////////////////////////////////////////
        };
    }
}
