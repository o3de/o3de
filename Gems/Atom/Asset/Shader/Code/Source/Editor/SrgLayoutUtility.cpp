/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "SrgLayoutUtility.h"

#include <Atom/RHI.Edit/Utils.h>

namespace AZ
{
    namespace ShaderBuilder
    {
        namespace SrgLayoutUtility
        {
            RHI::ShaderInputImageType ToShaderInputImageType(TextureType textureType)
            {
                switch (textureType)
                {
                case TextureType::Texture1D:
                    return RHI::ShaderInputImageType::Image1D;
                case TextureType::Texture1DArray:
                    return RHI::ShaderInputImageType::Image1DArray;
                case TextureType::Texture2D:
                    return RHI::ShaderInputImageType::Image2D;
                case TextureType::Texture2DArray:
                    return RHI::ShaderInputImageType::Image2DArray;
                case TextureType::Texture2DMS:
                    return RHI::ShaderInputImageType::Image2DMultisample;
                case TextureType::Texture2DMSArray:
                    return RHI::ShaderInputImageType::Image2DMultisampleArray;
                case TextureType::Texture3D:
                    return RHI::ShaderInputImageType::Image3D;
                case TextureType::TextureCube:
                    return RHI::ShaderInputImageType::ImageCube;
                case TextureType::RwTexture1D:
                    return RHI::ShaderInputImageType::Image1D;
                case TextureType::RwTexture1DArray:
                    return RHI::ShaderInputImageType::Image1DArray;
                case TextureType::RwTexture2D:
                    return RHI::ShaderInputImageType::Image2D;
                case TextureType::RwTexture2DArray:
                    return RHI::ShaderInputImageType::Image2DArray;
                case TextureType::RwTexture3D:
                    return RHI::ShaderInputImageType::Image3D;
                case TextureType::RasterizerOrderedTexture1D:
                    return RHI::ShaderInputImageType::Image1D;
                case TextureType::RasterizerOrderedTexture1DArray:
                    return RHI::ShaderInputImageType::Image1DArray;
                case TextureType::RasterizerOrderedTexture2D:
                    return RHI::ShaderInputImageType::Image2D;
                case TextureType::RasterizerOrderedTexture2DArray:
                    return RHI::ShaderInputImageType::Image2DArray;
                case TextureType::RasterizerOrderedTexture3D:
                    return RHI::ShaderInputImageType::Image3D;
                case TextureType::SubpassInput:
                    return RHI::ShaderInputImageType::SubpassInput;
                default:
                    AZ_Assert(false, "Unhandled TextureType");
                    return RHI::ShaderInputImageType::Unknown;
                }
            }

            RHI::ShaderInputBufferType ToShaderInputBufferType(BufferType bufferType)
            {
                switch (bufferType)
                {
                case BufferType::Buffer:
                case BufferType::RwBuffer:
                case BufferType::RasterizerOrderedBuffer:
                    return RHI::ShaderInputBufferType::Typed;
                case BufferType::AppendStructuredBuffer:
                case BufferType::ConsumeStructuredBuffer:
                case BufferType::RasterizerOrderedStructuredBuffer:
                case BufferType::RwStructuredBuffer:
                case BufferType::StructuredBuffer:
                    return RHI::ShaderInputBufferType::Structured;
                case BufferType::RasterizerOrderedByteAddressBuffer:
                case BufferType::ByteAddressBuffer:
                case BufferType::RwByteAddressBuffer:
                    return RHI::ShaderInputBufferType::Raw;
                case BufferType::RaytracingAccelerationStructure:
                    return RHI::ShaderInputBufferType::AccelerationStructure;
                default:
                    AZ_Assert(false, "Unhandled BufferType");
                    return RHI::ShaderInputBufferType::Unknown;
                }
            }

            bool LoadShaderResourceGroupLayouts(
                [[maybe_unused]] const char* builderName,
                const SrgDataContainer& resourceGroups,
                RPI::ShaderResourceGroupLayoutList& srgLayoutList)
            {
                // Load all SRGs included in source file
                for (const SrgData& srgData : resourceGroups)
                {
                    RHI::Ptr<RHI::ShaderResourceGroupLayout> newSrgLayout = RHI::ShaderResourceGroupLayout::Create();
                    newSrgLayout->SetName(AZ::Name{srgData.m_name.c_str()});
                    newSrgLayout->SetUniqueId(srgData.m_containingFileName);
                    newSrgLayout->SetBindingSlot(srgData.m_bindingSlot.m_index);

                    // Samplers
                    for (const SamplerSrgData& samplerData : srgData.m_samplers)
                    {
                        if (samplerData.m_isDynamic)
                        {
                            newSrgLayout->AddShaderInput({ samplerData.m_nameId,
                                                           samplerData.m_count,
                                                           samplerData.m_registerId,
                                                           samplerData.m_spaceId });
                        }
                        else
                        {
                            newSrgLayout->AddStaticSampler({ samplerData.m_nameId,
                                                             samplerData.m_descriptor,
                                                             samplerData.m_registerId,
                                                             samplerData.m_spaceId });
                        }
                    }

                    // Images
                    for (const TextureSrgData& textureData : srgData.m_textures)
                    {
                        const RHI::ShaderInputImageAccess imageAccess =
                            textureData.m_isReadOnlyType ? RHI::ShaderInputImageAccess::Read : RHI::ShaderInputImageAccess::ReadWrite;

                        const RHI::ShaderInputImageType imageType = SrgLayoutUtility::ToShaderInputImageType(textureData.m_type);

                        if (imageType != RHI::ShaderInputImageType::Unknown)
                        {
                            if (textureData.m_count != aznumeric_cast<uint32_t>(-1))
                            {
                                newSrgLayout->AddShaderInput(RHI::ShaderInputImageDescriptor{
                                    textureData.m_nameId, imageAccess, imageType, textureData.m_count,
                                    textureData.m_registerId, textureData.m_spaceId });
                            }
                            else
                            {
                                // unbounded array
                                newSrgLayout->AddShaderInput(RHI::ShaderInputImageUnboundedArrayDescriptor{
                                    textureData.m_nameId, imageAccess, imageType,
                                    textureData.m_registerId, textureData.m_spaceId });
                            }
                        }
                        else
                        {
                            AZ_Error(
                                builderName, false, "Failed to build Shader Resource Group Asset: Image %s has an unknown type.",
                                textureData.m_nameId.GetCStr());
                            return false;
                        }
                    }

                    // Buffers
                    {
                        for (const ConstantBufferData& cbData : srgData.m_constantBuffers)
                        {
                            newSrgLayout->AddShaderInput(RHI::ShaderInputBufferDescriptor{
                                cbData.m_nameId, RHI::ShaderInputBufferAccess::Constant, RHI::ShaderInputBufferType::Constant,
                                cbData.m_count, cbData.m_strideSize, cbData.m_registerId, cbData.m_spaceId });
                        }

                        for (const BufferSrgData& bufferData : srgData.m_buffers)
                        {
                            const RHI::ShaderInputBufferAccess bufferAccess =
                                bufferData.m_isReadOnlyType ? RHI::ShaderInputBufferAccess::Read : RHI::ShaderInputBufferAccess::ReadWrite;

                            const RHI::ShaderInputBufferType bufferType = SrgLayoutUtility::ToShaderInputBufferType(bufferData.m_type);

                            if (bufferType != RHI::ShaderInputBufferType::Unknown)
                            {
                                if (bufferData.m_count != aznumeric_cast<uint32_t>(-1))
                                {
                                    newSrgLayout->AddShaderInput(RHI::ShaderInputBufferDescriptor{
                                        bufferData.m_nameId, bufferAccess, bufferType, bufferData.m_count, bufferData.m_strideSize,
                                        bufferData.m_registerId, bufferData.m_spaceId });
                                }
                                else
                                {
                                    // unbounded array
                                    newSrgLayout->AddShaderInput(RHI::ShaderInputBufferUnboundedArrayDescriptor{
                                        bufferData.m_nameId, bufferAccess, bufferType, bufferData.m_strideSize,
                                        bufferData.m_registerId, bufferData.m_spaceId });
                                }
                            }
                            else
                            {
                                AZ_Error(
                                    builderName, false,
                                    "Failed to build Shader Resource Group Asset: Buffer %s has un unknown type.",
                                    bufferData.m_nameId.GetCStr());
                                return false;
                            }
                        }
                    }

                    // SRG Constants
                    for (const SrgConstantData& srgConstants : srgData.m_srgConstantData)
                    {
                        newSrgLayout->AddShaderInput({ srgConstants.m_nameId, srgConstants.m_constantByteOffset,
                                                       srgConstants.m_constantByteSize, srgData.m_srgConstantDataRegisterId,
                                                       srgData.m_srgConstantDataSpaceId });
                    }

                    // Shader Variant Key fallback
                    if (srgData.m_fallbackSize > 0)
                    {
                        // Designates this SRG as a ShaderVariantKey fallback
                        newSrgLayout->SetShaderVariantKeyFallback(srgData.m_fallbackName, srgData.m_fallbackSize);
                    }

                    srgLayoutList.push_back(newSrgLayout);
                }

                return true;
            }

        }  // namespace SrgLayoutUtility
    } // namespace ShaderBuilder
} // AZ
