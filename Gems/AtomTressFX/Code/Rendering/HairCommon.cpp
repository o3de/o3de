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
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Reflect/Buffer/BufferAssetView.h>

// Hair specific
#include <Rendering/HairCommon.h>

namespace AZ
{
    namespace Render
    {
        namespace Hair
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
                    AZ_Error(moduleName, false, "Failed to create shader resource group");
                    return nullptr;
                }
                return srg;
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
                desc.m_poolType = bufferDesc.m_poolType;;
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

            Data::Instance<RHI::ImagePool> UtilityClass::CreateImagePool(RHI::ImagePoolDescriptor& imagePoolDesc)
            {
                Data::Instance<RHI::ImagePool> imagePool = aznew RHI::ImagePool;
                RHI::ResultCode result = imagePool->Init(imagePoolDesc);
                if (result != RHI::ResultCode::Success)
                {
                    AZ_Error("CreateImagePool", false, "Failed to create or initialize image pool");
                    return nullptr;
                }
                return imagePool;
            }

            Data::Instance<RHI::Image> UtilityClass::CreateImage2D(
                RHI::ImagePool* imagePool, RHI::ImageDescriptor& imageDesc)
            {
                Data::Instance<RHI::Image> rhiImage = aznew RHI::Image;
                RHI::ImageInitRequest request;
                request.m_image = rhiImage.get();
                request.m_descriptor = imageDesc;
                RHI::ResultCode result = imagePool->InitImage(request);
                if (result != RHI::ResultCode::Success)
                {
                    AZ_Error("CreateImage2D", false, "Failed to create or initialize image");
                    return nullptr;
                }
                return rhiImage;
            }
        } // namespace Hair
    } // namespace Render
} // namespace AZ
