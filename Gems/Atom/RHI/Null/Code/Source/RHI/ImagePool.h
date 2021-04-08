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
