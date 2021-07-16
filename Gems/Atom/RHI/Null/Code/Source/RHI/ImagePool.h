/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/ImagePool.h>

namespace AZ
{
    namespace Null
    {
        class ImagePool final
            : public RHI::ImagePool
        {
            using Base = RHI::ImagePool;
        public:
            AZ_CLASS_ALLOCATOR(ImagePool, AZ::SystemAllocator, 0);
            AZ_RTTI(ImagePool, "{40DC04EA-890C-4D99-9F9C-DEC0038D7451}", RHI::ImagePool);
            
            static RHI::Ptr<ImagePool> Create();
            
        private:
            ImagePool() = default;
                        
            //////////////////////////////////////////////////////////////////////////
            // RHI::ImagePool
            RHI::ResultCode InitInternal([[maybe_unused]] RHI::Device&, [[maybe_unused]] const RHI::ImagePoolDescriptor&) override { return RHI::ResultCode::Success;}
            RHI::ResultCode InitImageInternal([[maybe_unused]] const RHI::ImageInitRequest& request) override { return RHI::ResultCode::Success;}
            RHI::ResultCode UpdateImageContentsInternal([[maybe_unused]] const RHI::ImageUpdateRequest& request) override { return RHI::ResultCode::Success;}
            void ShutdownResourceInternal([[maybe_unused]] RHI::Resource& resourceBase) override {}
            //////////////////////////////////////////////////////////////////////////
        };
    }
}
