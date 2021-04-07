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

#include <Atom/RHI/ImageView.h>
#include <AzCore/Memory/PoolAllocator.h>

namespace AZ
{
    namespace Null
    {
        class Image;

        class ImageView final
            : public RHI::ImageView
        {
            using Base = RHI::ImageView;
        public:
            AZ_CLASS_ALLOCATOR(ImageView, AZ::ThreadPoolAllocator, 0);
            AZ_RTTI(ImageView, "{4960F7E4-1B2D-4F35-97B3-7B21A5E1C516}", Base);

            static RHI::Ptr<ImageView> Create();

        private:
            ImageView() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::ImageView
            RHI::ResultCode InitInternal([[maybe_unused]] RHI::Device& device, [[maybe_unused]] const RHI::Resource& resourceBase) override { return RHI::ResultCode::Success;}
            RHI::ResultCode InvalidateInternal() override { return RHI::ResultCode::Success;}
            void ShutdownInternal() override {}
            //////////////////////////////////////////////////////////////////////////
        };
    }
}
