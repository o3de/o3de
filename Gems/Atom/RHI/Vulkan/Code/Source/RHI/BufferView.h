/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom_RHI_Vulkan_Platform.h>
#include <Atom/RHI/DeviceBufferView.h>
#include <AzCore/Memory/PoolAllocator.h>

namespace AZ
{
    namespace Vulkan
    {
        class Buffer;

        class BufferView final
            : public RHI::DeviceBufferView 
        {
            using Base = RHI::DeviceBufferView;

        public:
            using ResourceType = Buffer;

            //Using SystemAllocator here instead of ThreadPoolAllocator as it gets slower when
            //we create thousands of buffer views related to SRGs
            AZ_CLASS_ALLOCATOR(BufferView, AZ::SystemAllocator);
            AZ_RTTI(BufferView, "26BD4514-1D3B-4BDF-A7A5-AC689AEAEC42", Base);

            static RHI::Ptr<BufferView> Create();

            /// Only available if the underlaying buffer bind flags is ShaderRead or ShaderWrite
            VkBufferView GetNativeTexelBufferView() const;

            /// Only valid for buffers with the RayTracingAccelerationStructure bind flag
            VkAccelerationStructureKHR GetNativeAccelerationStructure() const;

            uint32_t GetBindlessReadIndex() const override;

            uint32_t GetBindlessReadWriteIndex() const override;

        private:
            friend class BindlessDescriptorPool;

            BufferView() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::Object
            void SetNameInternal(const AZStd::string_view& name) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            //  RHI::DeviceBufferView
            RHI::ResultCode InitInternal(RHI::Device& device, const RHI::DeviceResource& resourceBase) override;
            RHI::ResultCode InvalidateInternal() override;
            void ShutdownInternal() override;
            //////////////////////////////////////////////////////////////////////////

            RHI::ResultCode BuildNativeBufferView(Device& device, const Buffer& buffer, const RHI::BufferViewDescriptor& descriptor);
            void ReleaseView();
            void ReleaseBindlessIndices();

            VkBufferView m_nativeBufferView = VK_NULL_HANDLE;
            VkAccelerationStructureKHR m_nativeAccelerationStructure = VK_NULL_HANDLE;

            uint32_t m_readIndex = InvalidBindlessIndex;
            uint32_t m_readWriteIndex = InvalidBindlessIndex;
        };

    }
}
