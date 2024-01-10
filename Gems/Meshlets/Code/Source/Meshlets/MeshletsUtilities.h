/*
* Modifications Copyright (c) Contributors to the Open 3D Engine Project. 
* For complete copyright and license terms please see the LICENSE at the root of this distribution.
* 
* SPDX-License-Identifier: Apache-2.0 OR MIT
*
*/

#pragma once

#include <AtomCore/Instance/InstanceData.h>
#include <AtomCore/Instance/Instance.h>

#include <Atom/RHI/SingleDeviceImagePool.h>

#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Public/Image/AttachmentImage.h>
#include <Atom/RPI.Public/Shader/Shader.h>

#include <SharedBuffer.h>

namespace AZ
{
    namespace RHI
    {
        class BufferAssetView;
    }

    namespace RPI
    {
        class Buffer;
    }

    namespace Meshlets
    {
        class UtilityClass
        {
        public:
            UtilityClass() = default;

            static Data::Instance<RPI::ShaderResourceGroup> CreateShaderResourceGroup(
                Data::Instance<RPI::Shader> shader,
                const char* shaderResourceGroupId,
                const char* moduleName
            );

            static Data::Instance<RPI::Buffer> CreateBuffer(
                const char* warningHeader,
                SrgBufferDescriptor& bufferDesc,
                Data::Instance<RPI::ShaderResourceGroup> srg = nullptr
            );

            static Data::Instance<RPI::Buffer> CreateBufferAndBindToSrg(
                const char* warningHeader,
                SrgBufferDescriptor& bufferDesc,
                Data::Instance<RPI::ShaderResourceGroup> srg
            );

            static bool BindBufferToSrg(
                const char* warningHeader,
                Data::Instance<RPI::Buffer> buffer,
                SrgBufferDescriptor& bufferDesc,
                Data::Instance<RPI::ShaderResourceGroup> srg = nullptr
            );

            static Data::Instance<RHI::BufferView> CreateSharedBufferView(
                const char* warningHeader,
                SrgBufferDescriptor& bufferDesc,
                Data::Instance<Meshlets::SharedBufferAllocation>& bufferAllocator
            );

            static bool BindBufferViewToSrg(
                [[maybe_unused]] const char* warningHeader,
                Data::Instance<RHI::BufferView> bufferView,
                SrgBufferDescriptor& bufferDesc,
                Data::Instance<RPI::ShaderResourceGroup> srg
            );

            static Data::Instance<RHI::BufferView> CreateSharedBufferViewAndBindToSrg(
                const char* warningHeader,
                SrgBufferDescriptor& bufferDesc,
                Data::Instance<Meshlets::SharedBufferAllocation>& outputBufferAllocator,
                Data::Instance<RPI::ShaderResourceGroup> srg
            );

            //! Utility function to create a resource view into the shared buffer memory area. This
            //! resource view can have a different type than the shared buffer data.
            //! Since the shared buffer class is used as a buffer allocation for many sub-buffers, this
            //! method should be used after creating a new allocation within the shared buffer.
            static RHI::BufferViewDescriptor CreateResourceViewWithDifferentFormat(
                uint32_t offsetInBytes, uint32_t elementCount, uint32_t elementSize,
                RHI::Format format, RHI::BufferBindFlags overrideBindFlags
            );
        };

    } // namespace Meshlets
} // namespace AZ
