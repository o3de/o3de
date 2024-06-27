/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/RHIUtils.h>

#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Reflect/Buffer/BufferAssetView.h>

#include <MeshletsUtilities.h>

namespace AZ
{
    namespace Meshlets
    {

        //!=====================================================================================
        //!
        //!                                Utility Functions
        //!
        //!=====================================================================================

        // [To Do] examine if most of these functions can become global in RPI

        //! Utility function to generate the Srg given the shader and the desired Srg name to be associated to.
        //! If several shaders are sharing the same Srg (for example perView, perScene), it is enough to
        //! create the Srg by associating it with a single shader and since the GPU signature and the data
        //! are referring to the same shared description (preferable set in an [SrgDeclaration].aszli file)
        //! The association with all shaders will work properly.
        Data::Instance<RPI::ShaderResourceGroup> UtilityClass::CreateShaderResourceGroup(
            Data::Instance<RPI::Shader> shader,
            const char* shaderResourceGroupId,
            [[maybe_unused]] const char* moduleName)
        {
            Data::Instance<RPI::ShaderResourceGroup> srg = RPI::ShaderResourceGroup::Create(shader->GetAsset(), AZ::Name{ shaderResourceGroupId });
            if (!srg)
            {
                AZ_Error(moduleName, false, "Failed to create shader resource group [%s]", shaderResourceGroupId);
                return nullptr;
            }
            return srg;
        }

        //! Utility function to create a resource view of different type than the shared buffer data.
        //! Since this class is sub-buffer container, this method should be used after creating
        //!  a new allocation to be used as a sub-buffer.
        RHI::BufferViewDescriptor UtilityClass::CreateResourceViewWithDifferentFormat(
            uint32_t offsetInBytes, uint32_t elementCount, uint32_t elementSize,
            RHI::Format format, RHI::BufferBindFlags overrideBindFlags)
        {
            RHI::BufferViewDescriptor viewDescriptor;

            // In the following line I use the element size and not the size based of the
            // element format since in the more interesting case of structured buffer, the
            // size will result in an error.
            uint32_t elementOffset = offsetInBytes / elementSize;
            viewDescriptor.m_elementOffset = elementOffset;
            viewDescriptor.m_elementCount = elementCount;
            viewDescriptor.m_elementFormat = format;
            viewDescriptor.m_elementSize = elementSize;
            viewDescriptor.m_overrideBindFlags = overrideBindFlags;
            return viewDescriptor;
        }

        //! If srg is nullptr the index handle will NOT be set.
        //! This can be useful when creating a constant buffer or an image.
        Data::Instance<RPI::Buffer> UtilityClass::CreateBuffer(
            [[maybe_unused]] const char* warningHeader,
            SrgBufferDescriptor& bufferDesc,
            Data::Instance<RPI::ShaderResourceGroup> srg)
        {
            // If srg is provided, match the shader Srg bind index (returned via the descriptor)
            if (srg)
            {   // Not always do we want to associate Srg when creating a buffer
                bufferDesc.m_resourceShaderIndex = srg->FindShaderInputBufferIndex(bufferDesc.m_paramNameInSrg).GetIndex();
                if (bufferDesc.m_resourceShaderIndex == uint32_t(-1))
                {
                    AZ_Error(warningHeader, false, "Failed to find shader input index for [%s] in the SRG.",
                        bufferDesc.m_paramNameInSrg.GetCStr());
                    return nullptr;
                }
            }

            // Descriptor setting
            RPI::CommonBufferDescriptor desc;
            desc.m_elementFormat = bufferDesc.m_elementFormat;
            desc.m_poolType = bufferDesc.m_poolType;
            desc.m_elementSize = bufferDesc.m_elementSize;
            desc.m_bufferName = bufferDesc.m_bufferName.GetCStr();
            desc.m_byteCount = (uint64_t)bufferDesc.m_elementCount * bufferDesc.m_elementSize;
            desc.m_bufferData = nullptr;    // set during asset load - use Update

            // Buffer creation
            return RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);
        }

        bool UtilityClass::BindBufferToSrg(
            [[maybe_unused]] const char* warningHeader,
            Data::Instance<RPI::Buffer> buffer,
            SrgBufferDescriptor& bufferDesc,
            Data::Instance<RPI::ShaderResourceGroup> srg)
        {
            if (!buffer)
            {
                AZ_Error(warningHeader, false, "Trying to bind a null buffer");
                return false;
            }

            RHI::ShaderInputBufferIndex bufferIndex = srg->FindShaderInputBufferIndex(bufferDesc.m_paramNameInSrg);
            if (!bufferIndex.IsValid())
            {
                AZ_Error(warningHeader, false, "Failed to find shader input index for [%s] in the SRG.",
                    bufferDesc.m_paramNameInSrg.GetCStr());
                return false;
            }

            if (!srg->SetBufferView(bufferIndex, buffer->GetBufferView()))
            {
                AZ_Error(warningHeader, false, "Failed to bind buffer view for [%s]", bufferDesc.m_bufferName.GetCStr());
                return false;
            }
            return true;
        }

        Data::Instance<RPI::Buffer> UtilityClass::CreateBufferAndBindToSrg(
            const char* warningHeader,
            SrgBufferDescriptor& bufferDesc,
            Data::Instance<RPI::ShaderResourceGroup> srg)
        {
            // Buffer creation
            Data::Instance<RPI::Buffer> buffer = CreateBuffer(warningHeader, bufferDesc, srg);

            if (!BindBufferToSrg(warningHeader, buffer, bufferDesc, srg))
            {
                return nullptr;
            }
            return buffer;
        }

        // Returns the buffer view instance as well as the buffer allocator
        Data::Instance<RHI::BufferView> UtilityClass::CreateSharedBufferView(
            const char* warningHeader,
            SrgBufferDescriptor& bufferDesc,
            Data::Instance<Meshlets::SharedBufferAllocation>& outputBufferAllocator)
        {
            size_t requiredSize = (uint64_t)bufferDesc.m_elementCount * bufferDesc.m_elementSize;
            outputBufferAllocator = Meshlets::SharedBufferInterface::Get()->Allocate(requiredSize);
            if (!outputBufferAllocator)
            {
                AZ_Error(warningHeader, false, "Shared buffer out of memory for [%s]", bufferDesc.m_bufferName.GetCStr());
                return Data::Instance<RHI::BufferView>();
            }

            // Create the buffer view into the shared buffer - it will be used as a separate buffer
            // by the PerObject Srg.
            bufferDesc.m_viewOffsetInBytes = uint32_t(outputBufferAllocator->GetVirtualAddress().m_ptr);
            AZ_Assert(bufferDesc.m_viewOffsetInBytes % bufferDesc.m_elementSize == 0, "Offset of buffer within The SharedBuffer is NOT aligned.");

            // And here we create resource view from the shared buffer 
            RHI::BufferViewDescriptor viewDescriptor = UtilityClass::CreateResourceViewWithDifferentFormat(
                bufferDesc.m_viewOffsetInBytes, bufferDesc.m_elementCount, bufferDesc.m_elementSize,
                bufferDesc.m_elementFormat, bufferDesc.m_bindFlags
            );
            // Notice the following - this is crucial in order to pass the RHI validation
            // and force it not to fail the buffers views due to missing attachment.
            // The attachment itself is created for the PerPass shared buffer.
            viewDescriptor.m_ignoreFrameAttachmentValidation = true;

            RHI::Buffer* rhiBuffer = Meshlets::SharedBufferInterface::Get()->GetBuffer()->GetRHIBuffer();
            return rhiBuffer->BuildBufferView(viewDescriptor);
        }

        bool UtilityClass::BindBufferViewToSrg(
            [[maybe_unused]] const char* warningHeader,
            Data::Instance<RHI::BufferView> bufferView,
            SrgBufferDescriptor& bufferDesc,
            Data::Instance<RPI::ShaderResourceGroup> srg)
        {
            if (!bufferView || !srg)
            {
                AZ_Error(warningHeader, false, "Trying to bind a null buffer view or Srg");
                return false;
            }

            RHI::ShaderInputBufferIndex bufferIndex = srg->FindShaderInputBufferIndex(bufferDesc.m_paramNameInSrg);
            if (!bufferIndex.IsValid())
            {
                AZ_Error(warningHeader, false, "Failed to find shader input index for [%s] in the SRG.",
                    bufferDesc.m_paramNameInSrg.GetCStr());
                return false;
            }

            if (!srg->SetBufferView(bufferIndex, bufferView))
            {
                AZ_Error(warningHeader, false, "Failed to bind buffer view for [%s]", bufferDesc.m_bufferName.GetCStr());
                return false;
            }
            return true;
        }

        Data::Instance<RHI::BufferView> UtilityClass::CreateSharedBufferViewAndBindToSrg(
            const char* warningHeader,
            SrgBufferDescriptor& bufferDesc,
            Data::Instance<Meshlets::SharedBufferAllocation>& outputBufferAllocator,
            Data::Instance<RPI::ShaderResourceGroup> srg)
        {
            // BufferView creation
            Data::Instance<RHI::BufferView> bufferView = CreateSharedBufferView(warningHeader, bufferDesc, outputBufferAllocator);

            if (srg && !BindBufferViewToSrg(warningHeader, bufferView, bufferDesc, srg))
            {
               return nullptr;
            }
            return bufferView;
        }

    } // namespace Meshlets
} // namespace AZ
