/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <limits>

#include <Atom/RHI.Reflect/Format.h>
#include <Atom/RHI.Reflect/SamplerState.h>
#include <Atom/RHI.Reflect/ShaderResourceGroupLayoutDescriptor.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>

namespace AZ
{
    namespace ShaderBuilder
    {

        // Structures Info

        enum class MemberType
        {
            Bool,
            Int,
            Uint,
            Half,
            Float,
            Double,
            CustomType, // For structs etc.
        };

        struct ArrayItem
        {
            uint32_t      m_count; // For arrays where the count is defined inline foo[4];
            AZStd::string m_text; // For arrays counts that reference an identifier (like a const declared elsewhere) foo[count];

            AZStd::string GetText() const
            {
                if (!m_text.empty())
                {
                    return m_text;
                }
                return AZStd::to_string(m_count);
            }
        };

        /// This enum is just a mirror from the azlc enum (so we don't have the header dependency).
        enum class MatrixMajor : uint32_t
        {
            Default,
            ColumnMajor,
            RowMajor
        };

        struct Variable
        {
            AZStd::string m_name;
            AZStd::string m_typeString;
            AZStd::vector<ArrayItem> m_arrayDefinition;
            MemberType    m_type;
            MatrixMajor   m_typeModifier = MatrixMajor::ColumnMajor;
            bool          m_isMatrixType = false;
            uint8_t       m_rows = 1;
            uint8_t       m_cols = 1;
        };

        enum class Semantic
        {
            SV_Position,
            SV_Target,
            Custom,
            NoSemanticSet,
        };

        enum class InterpolationModifier
        {
            Linear,
            Centroid,
            NoInterpolation,
            NoPerspective,
            Sample,
            NoInterpolationSet,
        };

        const static int32_t s_noSemanticIndex= -1;

        struct StructParameter
        {
            Variable                     m_variable;
            AZStd::string                m_semanticText;
            AZ::RHI::Format              m_format = AZ::RHI::Format::Unknown;
            int32_t                      m_semanticIndex = s_noSemanticIndex;
            Semantic                     m_semanticType = Semantic::NoSemanticSet;
            InterpolationModifier        m_interpolation = InterpolationModifier::NoInterpolationSet;
        };

        struct StructData
        {
            AZStd::string                  m_id;
            AZStd::vector<StructParameter> m_members;

            AZStd::vector<Variable>        BuildTypeIdPairs() const;
        };

        // Textures

        enum class TextureType
        {
            Texture1D,
            Texture1DArray,
            Texture2D,
            Texture2DArray,
            Texture2DMS,
            Texture2DMSArray,
            Texture3D,
            TextureCube,
            RwTexture1D,
            RwTexture1DArray,
            RwTexture2D,
            RwTexture2DArray,
            RwTexture3D,
            RasterizerOrderedTexture1D,
            RasterizerOrderedTexture1DArray,
            RasterizerOrderedTexture2D,
            RasterizerOrderedTexture2DArray,
            RasterizerOrderedTexture3D,
            SubpassInput,
            Unknown
        };

        struct TextureSrgData
        {
            Variable      m_dataType;
            Name          m_nameId;
            AZStd::string m_description;
            AZStd::string m_resource;
            AZStd::string m_dependsOn;
            uint32_t      m_count = 1;
            TextureType   m_type = TextureType::Unknown;
            bool          m_isReadOnlyType = false;
            uint32_t      m_registerId = RHI::UndefinedRegisterSlot;
            uint32_t      m_spaceId = RHI::UndefinedRegisterSlot;
        };

        // Buffers

        enum class BufferType
        {
            AppendStructuredBuffer,
            ConsumeStructuredBuffer,
            Buffer,
            ByteAddressBuffer,
            StructuredBuffer,
            RwBuffer,
            RwByteAddressBuffer,
            RwStructuredBuffer,
            RasterizerOrderedBuffer,
            RasterizerOrderedByteAddressBuffer,
            RasterizerOrderedStructuredBuffer,
            RaytracingAccelerationStructure,
            Unknown
        };

        struct BufferSrgData
        {
            Variable      m_dataType;
            Name          m_nameId;
            AZStd::string m_description;
            uint32_t      m_count = 1;
            uint32_t      m_strideSize = 0;
            BufferType    m_type = BufferType::Unknown;
            bool          m_isReadOnlyType = false;
            uint32_t      m_registerId = RHI::UndefinedRegisterSlot;
            uint32_t      m_spaceId = RHI::UndefinedRegisterSlot;
        };

        struct ConstantBufferData
        {
            Name                    m_nameId;
            uint32_t                m_count = 1;
            uint32_t                m_strideSize = 0;
            AZStd::string           m_templateId;
            AZStd::vector<Variable> m_members;
            uint32_t                m_registerId = RHI::UndefinedRegisterSlot;
            uint32_t                m_spaceId = RHI::UndefinedRegisterSlot;
        };

        // SRG Constants

        struct SrgConstantData
        {
            uint32_t m_constantByteOffset = 0;      /// The offset from the start of the constant buffer in bytes.
            uint32_t m_constantByteSize = 0;       /// The number of bytes 
            Name m_nameId;
            
            // Meta data
            AZStd::string m_qualifiedName;
            AZStd::vector<ArrayItem> m_typeDimensions;
            AZStd::string m_typeKind;
            AZStd::string m_typeName;
        };

        // Samplers

        struct SamplerSrgData
        {
            AZ::RHI::SamplerState m_descriptor;
            Name m_nameId;
            AZStd::string m_description;
            bool m_isComparison = false;
            bool m_isDynamic = false;
            uint32_t m_count = 1;
            uint32_t m_registerId = RHI::UndefinedRegisterSlot;
            uint32_t m_spaceId = RHI::UndefinedRegisterSlot;
        };

        MemberType StringToBaseType(const char* baseType);
        TextureType StringToTextureType(const char* textureType);
        BufferType StringToBufferType(const char* bufferType);
        AZ::RHI::AddressMode StringToTextureAddressMode(const char* addressMode);
        AZ::RHI::BorderColor StringToTextureBorderColor(const char* borderColor);
        AZ::RHI::ComparisonFunc StringToComparisonFunc(const char* comparison);
        AZ::RHI::FilterMode StringToFilterMode(const char* filterMode);
        AZ::RHI::ReductionType StringToReductionType(const char* reductionType);
        AZ::RHI::Format StringToFormat(const char* format);

        /// Reflection data about external resource usage
        /// This is fundamentally the reproduction of the JSON algebraic format output by AZSLc --bindingdep command
        struct BindingDependencies
        {
            using SrgName             = AZStd::string;
            using ResourceName        = AZStd::string;
            using NameVector          = AZStd::vector<AZStd::string>;
            using FunctionsNameVector = AZStd::vector<AZStd::string>;
            using BindingType         = AZStd::string;
            using Register            = uint32_t;

            /// Extended binding information for one resource
            struct Resource
            {
                ResourceName        m_selfName;  ///< variable name in the high level source
                FunctionsNameVector m_dependentFunctions;  ///< all global functions where this constant buffer is referenced
                Register            m_registerId    = ~static_cast<Register>(0x0);                
                Register            m_registerSpace = ~static_cast<Register>(0);
                uint32_t            m_registerSpan  = 0;
                BindingType         m_type;
            };

            /// All SRG constants of one SRG are in one constant buffer. This structure represents its dependencies.
            struct SrgConstantsConstantBuffer
            {
                Resource   m_binding;
                NameVector m_partipicantConstants; ///< informatory list of all individual SRGConstants names that this constant buffer holds
            };

            /// resource content of only one SRG
            struct SrgResources
            {
                const Resource* GetResource(AZStd::string_view resourceName) const;

                SrgConstantsConstantBuffer m_srgConstantsDependencies; ///< only 0 or 1 per SRG
                AZStd::unordered_map<ResourceName, Resource> m_resources;  ///< extended binding information for each resource
            };

            /// returns nullptr if not found
            const SrgResources* GetSrg(AZStd::string_view srgName) const;
            
            AZStd::vector<SrgResources> m_orderedSrgs; ///< convenient for iteration
            AZStd::unordered_map<SrgName, int> m_srgNameToVectorIndex;  ///< to index in m_orderedSrgs
        };

        struct RootConstantBinding
        {
            AZ::Name        m_nameId;
            uint32_t        m_sizeInBytes = 0;
            uint32_t        m_space = std::numeric_limits<uint32_t>::max();
            uint32_t        m_registerId = RHI::UndefinedRegisterSlot;
        };
    } // ShaderBuilder
} // AZ
