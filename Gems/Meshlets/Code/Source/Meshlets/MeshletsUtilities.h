#pragma once

#include <AtomCore/Instance/InstanceData.h>

#include <Atom/RHI/ImagePool.h>

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

    namespace Data
    {
        /**
        * This is an alias of intrusive_ptr designed for any class which inherits from
        * InstanceData. You're not required to use Instance<> over AZStd::intrusive_ptr<>,
        * but it provides symmetry with Asset<>.
        */
        template <typename T>
        using Instance = AZStd::intrusive_ptr<T>;
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

            static RHI::BufferViewDescriptor CreateResourceViewWithDifferentFormat(
                uint32_t offsetInBytes, uint32_t elementCount, uint32_t elementSize,
                RHI::Format format, RHI::BufferBindFlags overrideBindFlags
            );
        };

    } // namespace Meshlets
} // namespace AZ
