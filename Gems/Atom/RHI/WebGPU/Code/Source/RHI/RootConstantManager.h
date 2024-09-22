/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceObject.h>

namespace AZ::WebGPU
{
    class Device;
    class BufferPool;
    class BindGroup;

    // Manager for handling root constants for a shader.
    // Root constant are not yet supported by WebGPU, so we use uniform buffers to pass
    // the values to the shader. The same buffer can be used for multiple draw/submit calls, since
    // we offset the access to it when binding the resource.
    class RootConstantManager
        : public RHI::DeviceObject
    {
    public:
        struct Allocation
        {
            RHI::Ptr<BindGroup> m_bindGroup;    // The binding group containing the buffer that needs to be bind
            RHI::Ptr<Buffer> m_buffer;          // The buffer that will be used to populate the root constants
            uint32_t m_bufferOffset = 0;        // The offset that needs to be used for the root constants values
        };

        AZ_CLASS_ALLOCATOR(RootConstantManager, SystemAllocator);

        static RHI::Ptr<RootConstantManager> Create();

        virtual ~RootConstantManager() = default;

        RHI::ResultCode Init(Device& device);

        void Shutdown() override;

        //! Allocates space in a buffer for root constants
        Allocation Allocate(uint32_t size);
        //! Removes unused resource at the end of the frame
        void Collect();
        //! Returns the layout of the group containing the buffer
        const BindGroupLayout& GetBindGroupLayout() const;

    protected:
        RootConstantManager() = default;

        //! Buffer pool used for creating the buffers
        RHI::Ptr<BufferPool> m_bufferPool;
        //! Allocations of buffers that are being used for root constants
        AZStd::vector<Allocation> m_allocations;
        //! Layout of the group that contains the uniform buffer used for holding root constants
        RHI::Ptr<BindGroupLayout> m_bindGroupLayout;
    };
}
