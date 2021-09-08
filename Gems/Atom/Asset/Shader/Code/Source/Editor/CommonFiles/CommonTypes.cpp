/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CommonFiles/CommonTypes.h>

#include <cctype>

#include <AzFramework/StringFunc/StringFunc.h>

namespace AZ
{
    namespace ShaderBuilder
    {
        [[maybe_unused]] static const char* s_azslShaderCompilerName = "AZSL Compiler";

        AZ::RHI::Format StringToFormat(const char* format)
        {
            if (AzFramework::StringFunc::Equal(format, "R16G16B16A16_FLOAT"))
            {
                return AZ::RHI::Format::R16G16B16A16_FLOAT;
            }
            else if (AzFramework::StringFunc::Equal(format, "R32"))     // can be Float32, Uint32 or Sint32
            {
                return AZ::RHI::Format::R32_FLOAT;
            }
            else if (AzFramework::StringFunc::Equal(format, "R32G32"))  // can be Float32, Uint32 or Sint32
            {
                return AZ::RHI::Format::R32G32_FLOAT;
            }
            else if (AzFramework::StringFunc::Equal(format, "R32A32"))  // can be Float32, Uint32 or Sint32
            {
                return AZ::RHI::Format::R32G32B32A32_FLOAT;
            }
            else if (AzFramework::StringFunc::Equal(format, "R16G16B16A16_UNORM"))
            {
                return AZ::RHI::Format::R16G16B16A16_UNORM;
            }
            else if (AzFramework::StringFunc::Equal(format, "R16G16B16A16_SNORM"))
            {
                return AZ::RHI::Format::R16G16B16A16_SNORM;
            }
            else if (AzFramework::StringFunc::Equal(format, "R16G16B16A16_UINT"))
            {
                return AZ::RHI::Format::R16G16B16A16_UINT;
            }
            else if (AzFramework::StringFunc::Equal(format, "R16G16B16A16_SINT"))
            {
                return AZ::RHI::Format::R16G16B16A16_SINT;
            }
            else if (AzFramework::StringFunc::Equal(format, "R32G32B32A32"))    // can be Float32, Uint32 or Sint32
            {
                return AZ::RHI::Format::R32G32B32A32_FLOAT;
            }

            AZ_Warning(s_azslShaderCompilerName, false, "Unknown format %s ", format);
            return AZ::RHI::Format::R16G16B16A16_FLOAT;
        }

        MemberType StringToBaseType(const char* baseType)
        {
            if (AzFramework::StringFunc::Equal(baseType, "float"))
            {
                return MemberType::Float;
            }
            else if (AzFramework::StringFunc::Equal(baseType, "int"))
            {
                return MemberType::Int;
            }
            else if (AzFramework::StringFunc::Equal(baseType, "uint"))
            {
                return MemberType::Uint;
            }
            else if (AzFramework::StringFunc::Equal(baseType, "half"))
            {
                return MemberType::Half;
            }
            else if (AzFramework::StringFunc::Equal(baseType, "double"))
            {
                return MemberType::Double;
            }
            else if (AzFramework::StringFunc::Equal(baseType, "bool"))
            {
                return MemberType::Bool;
            }
            
            return MemberType::CustomType; // For structs etc
        }

        TextureType StringToTextureType(const char* textureType)
        {
            if (AzFramework::StringFunc::StartsWith(textureType, "Texture1DArray"))
            {
                return TextureType::Texture1DArray;
            }
            else if (AzFramework::StringFunc::StartsWith(textureType, "Texture1D"))
            {
                return TextureType::Texture1D;
            }
            else if (AzFramework::StringFunc::StartsWith(textureType, "Texture2DMSArray"))
            {
                return TextureType::Texture2DMSArray;
            }
            else if (AzFramework::StringFunc::StartsWith(textureType, "Texture2DMS"))
            {
                return TextureType::Texture2DMS;
            }
            else if (AzFramework::StringFunc::StartsWith(textureType, "Texture2DArray"))
            {
                return TextureType::Texture2DArray;
            }
            else if (AzFramework::StringFunc::StartsWith(textureType, "Texture2D"))
            {
                return TextureType::Texture2D;
            }
            else if (AzFramework::StringFunc::StartsWith(textureType, "Texture3D"))
            {
                return TextureType::Texture3D;
            }
            else if (AzFramework::StringFunc::StartsWith(textureType, "TextureCube"))
            {
                return TextureType::TextureCube;
            }
            else if (AzFramework::StringFunc::StartsWith(textureType, "RwTexture1DArray"))
            {
                return TextureType::RwTexture1DArray;
            }
            else if (AzFramework::StringFunc::StartsWith(textureType, "RwTexture1D"))
            {
                return TextureType::RwTexture1D;
            }
            else if (AzFramework::StringFunc::StartsWith(textureType, "RwTexture2DArray"))
            {
                return TextureType::RwTexture2DArray;
            }
            else if (AzFramework::StringFunc::StartsWith(textureType, "RwTexture2D"))
            {
                return TextureType::RwTexture2D;
            }
            else if (AzFramework::StringFunc::StartsWith(textureType, "RwTexture3D"))
            {
                return TextureType::RwTexture3D;
            }
            else if (AzFramework::StringFunc::StartsWith(textureType, "RasterizerOrderedTexture1DArray"))
            {
                return TextureType::RasterizerOrderedTexture1DArray;
            }
            else if (AzFramework::StringFunc::StartsWith(textureType, "RasterizerOrderedTexture1D"))
            {
                return TextureType::RasterizerOrderedTexture1D;
            }
            else if (AzFramework::StringFunc::StartsWith(textureType, "RasterizerOrderedTexture2DArray"))
            {
                return TextureType::RasterizerOrderedTexture2DArray;
            }
            else if (AzFramework::StringFunc::StartsWith(textureType, "RasterizerOrderedTexture2D"))
            {
                return TextureType::RasterizerOrderedTexture2D;
            }
            else if (AzFramework::StringFunc::StartsWith(textureType, "RasterizerOrderedTexture3D"))
            {
                return TextureType::RasterizerOrderedTexture3D;
            }
            else if (AzFramework::StringFunc::StartsWith(textureType, "SubpassInput"))
            {
                return TextureType::SubpassInput;
            }
            return TextureType::Unknown;
        }

        BufferType StringToBufferType(const char* bufferType)
        {
            if (AzFramework::StringFunc::StartsWith(bufferType, "Buffer"))
            {
                return BufferType::Buffer;
            }
            else if (AzFramework::StringFunc::StartsWith(bufferType, "RWBuffer"))
            {
                return BufferType::RwBuffer;
            }
            else if (AzFramework::StringFunc::StartsWith(bufferType, "RasterizerOrderedBuffer"))
            {
                return BufferType::RasterizerOrderedBuffer;
            }
            else if (AzFramework::StringFunc::StartsWith(bufferType, "AppendStructuredBuffer"))
            {
                return BufferType::AppendStructuredBuffer;
            }
            else if (AzFramework::StringFunc::StartsWith(bufferType, "ByteAddressBuffer"))
            {
                return BufferType::ByteAddressBuffer;
            }
            else if (AzFramework::StringFunc::StartsWith(bufferType, "ConsumeStructuredBuffer"))
            {
                return BufferType::ConsumeStructuredBuffer;
            }
            else if (AzFramework::StringFunc::StartsWith(bufferType, "RasterizerOrderedByteAddressBuffer"))
            {
                return BufferType::RasterizerOrderedByteAddressBuffer;
            }
            else if (AzFramework::StringFunc::StartsWith(bufferType, "RasterizerOrderedStructuredBuffer"))
            {
                return BufferType::RasterizerOrderedStructuredBuffer;
            }
            else if (AzFramework::StringFunc::StartsWith(bufferType, "RwByteAddressBuffer"))
            {
                return BufferType::RwByteAddressBuffer;
            }
            else if (AzFramework::StringFunc::StartsWith(bufferType, "RwStructuredBuffer"))
            {
                return BufferType::RwStructuredBuffer;
            }
            else if (AzFramework::StringFunc::StartsWith(bufferType, "StructuredBuffer"))
            {
                return BufferType::StructuredBuffer;
            }
            else if (AzFramework::StringFunc::StartsWith(bufferType, "RaytracingAccelerationStructure"))
            {
                return BufferType::RaytracingAccelerationStructure;
            }
            return BufferType::Unknown;
        }

        AZ::RHI::AddressMode StringToTextureAddressMode(const char* addressMode)
        {
            if (AzFramework::StringFunc::Equal(addressMode, "TEXTURE_ADDRESS_WRAP"))
            {
                return RHI::AddressMode::Wrap;
            }
            else if (AzFramework::StringFunc::Equal(addressMode, "TEXTURE_ADDRESS_CLAMP"))
            {
                return RHI::AddressMode::Clamp;
            }
            else if (AzFramework::StringFunc::Equal(addressMode, "TEXTURE_ADDRESS_BORDER"))
            {
                return RHI::AddressMode::Border;
            }
            else if (AzFramework::StringFunc::Equal(addressMode, "TEXTURE_ADDRESS_MIRROR"))
            {
                return RHI::AddressMode::Mirror;
            }
            else if (AzFramework::StringFunc::Equal(addressMode, "TEXTURE_ADDRESS_MIRRORONCE"))
            {
                return RHI::AddressMode::MirrorOnce;
            }
            // If failed to match, set default address mode as WRAP
            return RHI::AddressMode::Wrap;
        }

        AZ::RHI::BorderColor StringToTextureBorderColor(const char* borderColor)
        {
            if (AzFramework::StringFunc::Equal(borderColor, "STATIC_BORDER_COLOR_OPAQUE_BLACK"))
            {
                return RHI::BorderColor::OpaqueBlack;
            }
            else if (AzFramework::StringFunc::Equal(borderColor, "STATIC_BORDER_COLOR_OPAQUE_WHITE"))
            {
                return RHI::BorderColor::OpaqueWhite;
            }
            else if (AzFramework::StringFunc::Equal(borderColor, "STATIC_BORDER_COLOR_TRANSPARENT_BLACK"))
            {
                return RHI::BorderColor::TransparentBlack;
            }
            // If failed to match, set default border color as OpaqueBlack
            return RHI::BorderColor::OpaqueBlack;
        }

        AZ::RHI::ComparisonFunc StringToComparisonFunc(const char* comparison)
        {
            if (AzFramework::StringFunc::Equal(comparison, "COMPARISON_NEVER"))
            {
                return RHI::ComparisonFunc::Never;
            }
            else if (AzFramework::StringFunc::Equal(comparison, "COMPARISON_LESS"))
            {
                return RHI::ComparisonFunc::Less;
            }
            else if (AzFramework::StringFunc::Equal(comparison, "COMPARISON_EQUAL"))
            {
                return RHI::ComparisonFunc::Equal;
            }
            else if (AzFramework::StringFunc::Equal(comparison, "COMPARISON_LESS_EQUAL"))
            {
                return RHI::ComparisonFunc::LessEqual;
            }
            else if (AzFramework::StringFunc::Equal(comparison, "COMPARISON_GREATER"))
            {
                return RHI::ComparisonFunc::Greater;
            }
            else if (AzFramework::StringFunc::Equal(comparison, "COMPARISON_NOT_EQUAL"))
            {
                return RHI::ComparisonFunc::NotEqual;
            }
            else if (AzFramework::StringFunc::Equal(comparison, "COMPARISON_GREATER_EQUAL"))
            {
                return RHI::ComparisonFunc::GreaterEqual;
            }
            else if (AzFramework::StringFunc::Equal(comparison, "COMPARISON_ALWAYS"))
            {
                return RHI::ComparisonFunc::Always;
            }

            // If failed to match, set default ComparisonFunc as Never
            return RHI::ComparisonFunc::Never;
        }

        AZ::RHI::FilterMode StringToFilterMode(const char* filterMode)
        {
            if (AzFramework::StringFunc::Equal(filterMode, "Point"))
            {
                return RHI::FilterMode::Point;
            }
            else if (AzFramework::StringFunc::Equal(filterMode, "Linear"))
            {
                return RHI::FilterMode::Linear;
            }

            // If failed to match, set default FilterMode as Point
            return RHI::FilterMode::Point;
        }

        AZ::RHI::ReductionType StringToReductionType(const char* reductionType)
        {
            if (AzFramework::StringFunc::Equal(reductionType, "Comparison"))
            {
                return RHI::ReductionType::Comparison;
            }
            else if (AzFramework::StringFunc::Equal(reductionType, "Filter"))
            {
                return RHI::ReductionType::Filter;
            }
            else if (AzFramework::StringFunc::Equal(reductionType, "Minimum"))
            {
                return RHI::ReductionType::Minimum;
            }
            else if (AzFramework::StringFunc::Equal(reductionType, "Maximum"))
            {
                return RHI::ReductionType::Maximum;
            }

            // If failed to match, set default ReductionType as Comparison
            return RHI::ReductionType::Comparison;
        }

        const BindingDependencies::Resource* BindingDependencies::SrgResources::GetResource(AZStd::string_view resourceName) const
        {
            if (m_srgConstantsDependencies.m_binding.m_selfName == resourceName)
            {
                return &m_srgConstantsDependencies.m_binding;
            }
            auto lookupIterator = m_resources.find(resourceName);
            if (lookupIterator != m_resources.end())
            {
                return &lookupIterator->second;
            }
            // last chance: the querier could be asking about an individual constant name. that's misguided but let's support it for convenience
            auto participantLookupIterator = AZStd::find_if(
                m_srgConstantsDependencies.m_partipicantConstants.begin(),
                m_srgConstantsDependencies.m_partipicantConstants.end(),
                [resourceName](const AZStd::string& candidate) { return candidate == resourceName; });
            if (participantLookupIterator != m_srgConstantsDependencies.m_partipicantConstants.end())
            {
                return &m_srgConstantsDependencies.m_binding;
            }
            return nullptr;
        }

        const BindingDependencies::SrgResources* BindingDependencies::GetSrg(AZStd::string_view srgName) const
        {
            auto iterator = m_srgNameToVectorIndex.find(srgName);
            return iterator == m_srgNameToVectorIndex.end() ? nullptr : &m_orderedSrgs[iterator->second];
        }
    } // namespace ShaderBuilder
} // namespace AZ
