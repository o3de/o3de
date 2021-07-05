/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/ImagePool.h>
#include <Atom/RHI.Reflect/Vulkan/ImagePoolDescriptor.h>
#include <RHI/MemoryAllocator.h>

namespace AZ
{
    namespace Vulkan
    {
        class Device;

        class ImagePool final
            : public RHI::ImagePool
        {
            using Base = RHI::ImagePool;
        public:
            AZ_CLASS_ALLOCATOR(ImagePool, AZ::SystemAllocator, 0);
            AZ_RTTI(ImagePool, "35351DF3-823C-4042-A8BA-D6FE10FF6A8D", Base);

            static RHI::Ptr<ImagePool> Create();

            void GarbageCollect();

        private:
            ImagePool() = default;

            //////////////////////////////////////////////////////////////////////////
            // FrameSchedulerEventBus::Handler
            void OnFrameEnd() override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // RHI::ImagePool
            RHI::ResultCode InitInternal(RHI::Device&, const RHI::ImagePoolDescriptor&) override;
            RHI::ResultCode InitImageInternal(const RHI::ImageInitRequest& request) override;
            RHI::ResultCode UpdateImageContentsInternal(const RHI::ImageUpdateRequest& request) override;
            void ShutdownInternal() override;
            void ShutdownResourceInternal(RHI::Resource& resourceBase) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // RHI::Object
            void SetNameInternal(const AZStd::string_view& name) override;
            //////////////////////////////////////////////////////////////////////////

            MemoryAllocator m_memoryAllocator;
        };
    }
}
