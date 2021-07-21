/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/BufferView.h>
#include <AzCore/Memory/PoolAllocator.h>

namespace AZ
{
    namespace Vulkan
    {
        class Buffer;

        class BufferView final
            : public RHI::BufferView 
        {
            using Base = RHI::BufferView;

        public:
            using ResourceType = Buffer;

            AZ_CLASS_ALLOCATOR(BufferView, AZ::ThreadPoolAllocator, 0);
            AZ_RTTI(BufferView, "26BD4514-1D3B-4BDF-A7A5-AC689AEAEC42", Base);

            static RHI::Ptr<BufferView> Create();

            /// Only available if the underlaying buffer bind flags is ShaderRead or ShaderWrite
            VkBufferView GetNativeTexelBufferView() const;

            /// Only valid for buffers with the RayTracingAccelerationStructure bind flag
            VkAccelerationStructureKHR GetNativeAccelerationStructure() const;

        private:
            BufferView() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::Object
            void SetNameInternal(const AZStd::string_view& name) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            //  RHI::BufferView
            RHI::ResultCode InitInternal(RHI::Device& device, const RHI::Resource& resourceBase) override;
            RHI::ResultCode InvalidateInternal() override;
            void ShutdownInternal() override;
            //////////////////////////////////////////////////////////////////////////

            RHI::ResultCode BuildNativeBufferView(Device& device, const Buffer& buffer, const RHI::BufferViewDescriptor& descriptor);

            VkBufferView m_nativeBufferView = VK_NULL_HANDLE;
            VkAccelerationStructureKHR m_nativeAccelerationStructure = VK_NULL_HANDLE;
        };

    }
}
