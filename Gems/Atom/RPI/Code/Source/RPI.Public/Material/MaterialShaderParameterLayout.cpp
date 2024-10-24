/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Material/MaterialShaderParameter.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Matrix3x4.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Utils/Utils.h>

namespace AZ::RPI
{
    namespace
    {
        template<typename T>
        struct StructuredBufferTypeSize
        {
            static const size_t value = sizeof(T);
        };
        template<typename T>
        struct GpuTypeName
        {
        };

        template<typename T>
        struct GpuIndirectIndex
        {
            static const bool value = false;
        };

        // clang-format doesn't like single line functions
        // clang-format off
        template<> struct GpuTypeName<int32_t> { static constexpr const char* value = "int"; };
        template<> struct GpuTypeName<uint32_t> { static constexpr const char* value = "uint"; };
        template<> struct GpuTypeName<float> { static constexpr const char* value = "float"; };
        
        template<> struct StructuredBufferTypeSize<Vector2> { static const size_t value = sizeof(float) * 2; };
        template<> struct GpuTypeName<Vector2> { static constexpr const char* value = "float2"; };
        
        template<> struct StructuredBufferTypeSize<Vector3> { static const size_t value = sizeof(float) * 3; };
        template<> struct GpuTypeName<Vector3> { static constexpr const char* value = "float3"; };
        
        template<> struct StructuredBufferTypeSize<Vector4> { static const size_t value = sizeof(float) * 4; };
        template<> struct GpuTypeName<Vector4> { static constexpr const char* value = "float4"; };
        
        template<> struct StructuredBufferTypeSize<Color> { static const size_t value = sizeof(float) * 4; };
        template<> struct GpuTypeName<Color> { static constexpr const char* value = "float4"; };
        
        template<> struct StructuredBufferTypeSize<bool> { static const size_t value = sizeof(int32_t); };
        template<> struct GpuTypeName<bool> { static constexpr const char* value = "bool"; };
        
        template<> struct StructuredBufferTypeSize<AZ::Matrix3x3> { static const size_t value = sizeof(float) * AZ::Matrix3x3::RowCount * AZ::Matrix3x3::ColCount; };
        template<> struct GpuTypeName<AZ::Matrix3x3> { static constexpr const char* value = "float3x3"; };
        
        template<> struct StructuredBufferTypeSize<AZ::Matrix3x4> { static const size_t value = sizeof(float) * AZ::Matrix3x4::RowCount * AZ::Matrix3x4::ColCount; };
        template<> struct GpuTypeName<AZ::Matrix3x4> { static constexpr const char* value = "float3x4"; };
        
        template<> struct StructuredBufferTypeSize<AZ::Matrix4x4> { static const size_t value = sizeof(float) * Matrix4x4::RowCount * AZ::Matrix4x4::ColCount; };
        template<> struct GpuTypeName<AZ::Matrix4x4> { static constexpr const char* value = "float4x4"; };
        
        // Sampler-State is stored in an array in the SRG and we ony store the index to it
        template<> struct StructuredBufferTypeSize<RHI::SamplerState> { static const size_t value = sizeof(uint32_t); };
        template<> struct GpuTypeName<RHI::SamplerState> { static constexpr const char* value = "uint"; };
        template<> struct GpuIndirectIndex<RHI::SamplerState> { static const bool value = true; };
        
        // Textures are stored either in an Array in the SRG or in the Bindless-SRG, and we only store the index to it
        template<> struct StructuredBufferTypeSize<Data::Asset<Image>> { static const size_t value = sizeof(int32_t); };
        template<> struct GpuTypeName<Data::Asset<Image>> { static constexpr const char* value = "int"; };
        template<> struct GpuIndirectIndex<Data::Asset<Image>> { static const bool value = true; };
        // clang-format on
    } // namespace

    MaterialShaderParameterLayout::Index MaterialShaderParameterLayout::AddTypedMaterialParameter(
        const AZStd::string_view& name,
        const AZStd::string_view& typeName,
        const size_t gpuTypesize,
        const bool isPseudoparam,
        const size_t count)
    {
        // Hack: AZSL complains about the layout of variables following a float3x3. So add a float4 as padding
        if (!m_descriptors.empty() && m_descriptors.back().m_typeName == "float3x3" && typeName != "float3x3" &&
            !name.starts_with("m_pad_matrix"))
        {
            AddMaterialParameter<Vector4>(AZStd::string::format("m_pad_matrix_%d", m_matrixPaddingIndex++), true);
        }

        MaterialShaderParameterLayout::Index parameterIndex{ GetParameterIndex(name) };
        if (!parameterIndex.IsValid())
        {
            parameterIndex = MaterialShaderParameterLayout::Index{ static_cast<uint32_t>(m_names.Size()) };
            m_names.Insert(AZ::Name(name), parameterIndex);
            MaterialShaderParameterDescriptor::BufferBinding bufferBinding{
                static_cast<uint32_t>(gpuTypesize),
                static_cast<uint32_t>(count),
                static_cast<uint32_t>(getStructuredBufferOffset()),
            };
            m_descriptors.emplace_back(
                MaterialShaderParameterDescriptor{ AZStd::string(name), typeName, bufferBinding, {}, false, isPseudoparam });
        }
        else
        {
            [[maybe_unused]] auto* desc = GetDescriptor(parameterIndex);
            AZ_Assert(
                desc->m_structuredBufferBinding.m_elementCount == count,
                "MaterialParameterBuffer: Redefinition of Buffer entry %s with element count (%d -> %d)",
                AZStd::string(name).c_str(),
                desc->m_structuredBufferBinding.m_elementCount,
                count);
            AZ_Assert(
                desc->m_typeName == typeName,
                "MaterialParameterBuffer: Redefinition of Buffer entry %s with new type: %s -> %s",
                AZStd::string(name).c_str(),
                desc->m_typeName.c_str(),
                AZStd::string(typeName).c_str());
            AZ_Warning(
                "MaterialParameterBuffer",
                false,
                "Parameter %s with type %s is defined more than once.",
                AZStd::string(name).c_str(),
                desc->m_typeName.c_str());
        }
        return parameterIndex;
    }

    template<typename T>
    MaterialShaderParameterLayout::Index MaterialShaderParameterLayout::AddMaterialParameter(
        const AZStd::string_view& name, bool isPseudoParam, const size_t count)
    {
        AZStd::string typeName{ GpuTypeName<T>::value };
        // Hack: AZSL complains about the layout of variables following a float3x3. So add a float4 as padding
        if (!m_descriptors.empty() && m_descriptors.back().m_typeName == "float3x3" && typeName != "float3x3" &&
            !name.starts_with("m_pad_matrix"))
        {
            AddMaterialParameter<Vector4>(AZStd::string::format("m_pad_matrix_%d", m_matrixPaddingIndex++), true);
        }

        MaterialShaderParameterLayout::Index parameterIndex{ GetParameterIndex(name) };
        if (!parameterIndex.IsValid())
        {
            parameterIndex = MaterialShaderParameterLayout::Index{ static_cast<uint32_t>(m_names.Size()) };
            m_names.Insert(AZ::Name(name), parameterIndex);
            MaterialShaderParameterDescriptor::BufferBinding bufferBinding{
                static_cast<uint32_t>(StructuredBufferTypeSize<T>::value),
                static_cast<uint32_t>(count),
                static_cast<uint32_t>(getStructuredBufferOffset()),
            };

            m_descriptors.emplace_back(MaterialShaderParameterDescriptor{
                AZStd::string(name), typeName, bufferBinding, {}, GpuIndirectIndex<T>::value, isPseudoParam });
        }
        else
        {
            [[maybe_unused]] auto* desc = GetDescriptor(parameterIndex);
            AZ_Assert(
                desc->m_structuredBufferBinding.m_elementCount == count,
                "MaterialParameterBuffer: Redefinition of Buffer entry %s with element count (%d -> %d)",
                AZStd::string(name).c_str(),
                desc->m_structuredBufferBinding.m_elementCount,
                count);
            AZ_Assert(
                desc->m_typeName == typeName,
                "MaterialParameterBuffer: Redefinition of Buffer entry %s with new type: %s -> %s",
                AZStd::string(name).c_str(),
                desc->m_typeName.c_str(),
                typeName.c_str());
        }
        return parameterIndex;
    }

    // explicit instantiation

#define INSTANTIATE_AddMaterialParameter(type)                                                                                             \
    template AZ_DLL_EXPORT MaterialShaderParameterLayout::Index MaterialShaderParameterLayout::AddMaterialParameter<type>(                 \
        const AZStd::string_view&, const bool isPseudoParam, const size_t count);

    INSTANTIATE_AddMaterialParameter(AZ::Matrix4x4);
    INSTANTIATE_AddMaterialParameter(AZ::Matrix3x3);
    INSTANTIATE_AddMaterialParameter(AZ::Matrix3x4);
    INSTANTIATE_AddMaterialParameter(AZ::Color);
    INSTANTIATE_AddMaterialParameter(AZ::Vector2);
    INSTANTIATE_AddMaterialParameter(AZ::Vector3);
    INSTANTIATE_AddMaterialParameter(AZ::Vector4);
    INSTANTIATE_AddMaterialParameter(int32_t);
    INSTANTIATE_AddMaterialParameter(uint32_t);
    INSTANTIATE_AddMaterialParameter(float);
    INSTANTIATE_AddMaterialParameter(bool);
    INSTANTIATE_AddMaterialParameter(RHI::SamplerState);
    INSTANTIATE_AddMaterialParameter(Data::Asset<Image>);

    void MaterialShaderParameterDescriptor::BufferBinding::Reflect(ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<MaterialShaderParameterDescriptor::BufferBinding>()
                ->Version(1) // changed variable types to uint32_t
                ->Field("m_elementSize", &MaterialShaderParameterDescriptor::BufferBinding::m_elementSize)
                ->Field("m_elementCount", &MaterialShaderParameterDescriptor::BufferBinding::m_elementCount)
                ->Field("m_offset", &MaterialShaderParameterDescriptor::BufferBinding::m_offset);
        }
    }

        void MaterialShaderParameterDescriptor::Reflect(ReflectContext* context)
        {
            MaterialShaderParameterDescriptor::BufferBinding::Reflect(context);

            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<MaterialShaderParameterDescriptor>()
                    ->Version(0)
                    ->Field("m_name", &MaterialShaderParameterDescriptor::m_name)
                    ->Field("m_typeName", &MaterialShaderParameterDescriptor::m_typeName)
                    ->Field("m_structuredBufferBinding", &MaterialShaderParameterDescriptor::m_structuredBufferBinding)
                    ->Field("m_srgInputIndex", &MaterialShaderParameterDescriptor::m_srgInputIndex)
                    ->Field("m_isBindlessReadIndex", &MaterialShaderParameterDescriptor::m_isBindlessReadIndex)
                    ->Field("m_isPseudoparam", &MaterialShaderParameterDescriptor::m_isPseudoParam);
            }
        }

        void MaterialShaderParameterLayout::Reflect(ReflectContext* context)
        {
            Index::Reflect(context);
            RHI::NameIdReflectionMap<Index>::Reflect(context);
            MaterialShaderParameterDescriptor::Reflect(context);
            MaterialShaderParameterNameIndex::Reflect(context);

            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<MaterialShaderParameterLayout>()
                    ->Version(0)
                    ->Field("m_names", &MaterialShaderParameterLayout::m_names)
                    ->Field("m_descriptors", &MaterialShaderParameterLayout::m_descriptors);
            }
        }

        MaterialShaderParameterLayout::Index MaterialShaderParameterLayout::AddParameterFromFunctor(
            const AZStd::string& name, const AZStd::string& typeName, const size_t typeSize)
        {
            MaterialShaderParameterLayout::Index parameterIndex{ MaterialShaderParameterLayout::Index::Null };

            if (typeName == "float")
            {
                parameterIndex = AddMaterialParameter<float>(name);
            }
            else if (typeName == "int")
            {
                parameterIndex = AddMaterialParameter<int32_t>(name);
            }
            else if (typeName == "uint")
            {
                parameterIndex = AddMaterialParameter<uint32_t>(name);
            }
            else if (typeName == "float2")
            {
                parameterIndex = AddMaterialParameter<Vector2>(name);
            }
            else if (typeName == "float3")
            {
                parameterIndex = AddMaterialParameter<Vector3>(name);
            }
            else if (typeName == "float4")
            {
                parameterIndex = AddMaterialParameter<Vector4>(name);
            }
            else if (typeName == "float3x3")
            {
                parameterIndex = AddMaterialParameter<Matrix3x3>(name);
            }
            else if (typeName == "float3x4")
            {
                parameterIndex = AddMaterialParameter<Matrix3x4>(name);
            }
            else if (typeName == "float4x4")
            {
                parameterIndex = AddMaterialParameter<Matrix4x4>(name);
            }
            else
            {
                AZ_Assert(
                    typeSize > 0,
                    "CreateMaterialShaderParameterLayout: Type %s of Material-Functor shader parameter %s is unknown and needs a "
                    "valid size.",
                    typeName.c_str(),
                    name.c_str());
                parameterIndex = AddTypedMaterialParameter(name, typeName, typeSize);
            }
            return parameterIndex;
        }

        MaterialShaderParameterLayout::Index MaterialShaderParameterLayout::AddParameterFromPropertyConnection(
            const AZ::Name& name, const MaterialPropertyDataType dataType)
        {
            MaterialShaderParameterLayout::Index parameterIndex{ MaterialShaderParameterLayout::Index::Null };
            switch (dataType)
            {
            case MaterialPropertyDataType::Bool:
                parameterIndex = AddMaterialParameter<bool>(name);
                break;
            case MaterialPropertyDataType::UInt:
                parameterIndex = AddMaterialParameter<uint32_t>(name);
                break;
            case MaterialPropertyDataType::Float:
                parameterIndex = AddMaterialParameter<float>(name);
                break;
            case MaterialPropertyDataType::Vector2:
                parameterIndex = AddMaterialParameter<Vector2>(name);
                break;
            case MaterialPropertyDataType::Vector3:
                parameterIndex = AddMaterialParameter<Vector3>(name);
                break;
            case MaterialPropertyDataType::Vector4:
                parameterIndex = AddMaterialParameter<Vector4>(name);
                break;
            case MaterialPropertyDataType::Color:
                parameterIndex = AddMaterialParameter<Color>(name);
                break;
            case MaterialPropertyDataType::Int:
            case MaterialPropertyDataType::Enum:
                parameterIndex = AddMaterialParameter<int32_t>(name);
                break;
            case MaterialPropertyDataType::Image:
                parameterIndex = AddMaterialParameter<Data::Asset<Image>>(name);
                break;
            case MaterialPropertyDataType::SamplerState:
                parameterIndex = AddMaterialParameter<RHI::SamplerState>(name);
                break;
            default:
                AZ_Error(
                    "MaterialShaderParameterLayout",
                    false,
                    "Material property '%s': Properties of this type cannot be mapped to a Shader Parameter input.",
                    name.GetCStr());
                break;
            }
            return parameterIndex;
        }

        const MaterialShaderParameterLayout::Index MaterialShaderParameterLayout::GetParameterIndex(const AZStd::string_view& name) const
        {
            return m_names.Find(AZ::Name{ name });
        }

        MaterialShaderParameterDescriptor* MaterialShaderParameterLayout::GetDescriptor(const Index parameterIndex)
        {
            if (parameterIndex.IsValid())
            {
                return &m_descriptors[parameterIndex.GetIndex()];
            }
            return nullptr;
        }

        const MaterialShaderParameterDescriptor* const MaterialShaderParameterLayout::GetDescriptor(const Index parameterIndex) const
        {
            if (parameterIndex.IsValid())
            {
                return &m_descriptors[parameterIndex.GetIndex()];
            }
            return nullptr;
        }

        size_t MaterialShaderParameterLayout::getStructuredBufferOffset() const
        {
            if (m_descriptors.empty())
            {
                return 0;
            }
            const auto& lastEntry{ m_descriptors.back().m_structuredBufferBinding };

            auto byteIndex = (lastEntry.m_offset + (lastEntry.m_elementSize * lastEntry.m_elementCount)) / 4;
            return byteIndex * 4;
        }

        int MaterialShaderParameterLayout::ConnectParametersToSrg(const RHI::ShaderResourceGroupLayout* srgLayout)
        {
            int connected{ 0 };
            for (auto& desc : m_descriptors)
            {
                if (ConnectParameterToSrg(&desc, srgLayout))
                {
                    connected++;
                }
            }
            return connected;
        }

        bool MaterialShaderParameterLayout::ConnectParameterToSrg(
            MaterialShaderParameterDescriptor* desc, const RHI::ShaderResourceGroupLayout* srgLayout)
        {
            auto assignSrgIndex = [desc, srgLayout](AZ::Name name)
            {
                auto constantIndex = srgLayout->FindShaderInputConstantIndex(name);
                if (constantIndex.IsValid())
                {
                    desc->m_srgInputIndex = constantIndex;
                    return true;
                }
                auto imageIndex = srgLayout->FindShaderInputImageIndex(name);
                if (imageIndex.IsValid())
                {
                    desc->m_srgInputIndex = imageIndex;
                    return true;
                }
                return false;
            };

            // Note: the (new) shader material functions provided by the engine take all parameters from a MaterialParameter - struct.
            // The materials generally fetch the parameters from a Bindless ByteAdressBuffer via the SceneMaterialSrg before calling the
            // material functions, but we can fill these buffers if we know the exact layout of the Parameter struct on the
            // GPU, and we only know that if we generated the struct during the materialpipeline processing, when we turn an abstract
            // materialtype into a non-abstract materialtype.
            // Shaders that do not use an abstract material-type (e.g. the SilhouetteGather - shader) can still use the (new) shader
            // material functions by manually defining a MaterialParameter - struct. If that struct is part of the SRG with the name
            // "m_params", we set the parameter values directly in the SRG.

            auto inputName = AZ::Name{ AZStd::string::format("m_params.%s", desc->m_name.c_str()) };
            if (!assignSrgIndex(inputName))
            {
                // Backwards compatability with shaders that aren't using the (new) shader material functions from the engine:
                // look for the parameter name in the srg directly, so the data still arrives at the shader
                inputName = desc->m_name;
                return assignSrgIndex(inputName);
            }
            return true;
        }

        void MaterialShaderParameterLayout::FinalizeLayout()
        {
            auto& lastEntry{ m_descriptors.back().m_structuredBufferBinding };
            auto bufferSize = lastEntry.m_offset + lastEntry.m_elementSize * lastEntry.m_elementCount;

            // pad the struct with floats so the entire struct is 16 byte aligned in a structured buffer
            auto padding = (16 - bufferSize % 16) / 4;
            switch (padding)
            {
            case 1:
                AddMaterialParameter<float>("m_finalPadding", true);
                break;
            case 2:
                AddMaterialParameter<Vector2>("m_finalPadding", true);
                break;
            case 3:
                AddMaterialParameter<Vector3>("m_finalPadding", true);
                break;
            default:
                break;
            }
        }

        bool MaterialShaderParameterLayout::IsPropertyTypeCompatibleWithShaderParameter(
            const MaterialShaderParameterDescriptor* desc, const MaterialPropertyDataType dataType)
        {
            bool isValid = false;
            switch (dataType)
            {
            case MaterialPropertyDataType::Bool:
                isValid = GpuTypeName<bool>::value == desc->m_typeName;
                break;
            case AZ::RPI::MaterialPropertyDataType::Int:
            case AZ::RPI::MaterialPropertyDataType::UInt:
                // we allow connecting int to uint and vice versa
                isValid = GpuTypeName<int32_t>::value == desc->m_typeName;
                isValid |= GpuTypeName<uint32_t>::value == desc->m_typeName;
                break;
            case AZ::RPI::MaterialPropertyDataType::Float:
                isValid = GpuTypeName<float>::value == desc->m_typeName;
                break;
            case AZ::RPI::MaterialPropertyDataType::Vector2:
                isValid = GpuTypeName<Vector2>::value == desc->m_typeName;
                break;
            case AZ::RPI::MaterialPropertyDataType::Vector3:
                isValid = GpuTypeName<Vector3>::value == desc->m_typeName;
                break;
            case AZ::RPI::MaterialPropertyDataType::Vector4:
                isValid = GpuTypeName<Vector4>::value == desc->m_typeName;
                break;
            case AZ::RPI::MaterialPropertyDataType::Color:
                isValid = GpuTypeName<Color>::value == desc->m_typeName;
                break;
            case AZ::RPI::MaterialPropertyDataType::Image:
                isValid = GpuTypeName<Data::Asset<Image>>::value == desc->m_typeName;
                break;
            case AZ::RPI::MaterialPropertyDataType::SamplerState:
                isValid = GpuTypeName<RHI::SamplerState>::value == desc->m_typeName;
                break;
            case AZ::RPI::MaterialPropertyDataType::Enum:
                // enum can connect to int and uint
                isValid = GpuTypeName<uint32_t>::value == desc->m_typeName;
                isValid |= GpuTypeName<int32_t>::value == desc->m_typeName;
                break;
            default:
                break;
            }
            return isValid;
        }

        bool MaterialShaderParameterLayout::IsPropertyTypeCompatibleWithSrg(
            const MaterialShaderParameterDescriptor* desc, const MaterialPropertyDataType dataType)
        {
            bool isValid = false;

            // we can only differentiate between image and constant for the SRG. Checking the data-type doesn't really work
            if (dataType == AZ::RPI::MaterialPropertyDataType::Image)
            {
                if (AZStd::holds_alternative<RHI::ShaderInputImageIndex>(desc->m_srgInputIndex))
                {
                    isValid = AZStd::get<RHI::ShaderInputImageIndex>(desc->m_srgInputIndex).IsValid();
                }
                // if the property is an image this might still be a constant value with the bindless read index of the image
                else if (AZStd::holds_alternative<RHI::ShaderInputConstantIndex>(desc->m_srgInputIndex) && desc->m_isBindlessReadIndex)
                {
                    isValid = AZStd::get<RHI::ShaderInputConstantIndex>(desc->m_srgInputIndex).IsValid();
                }
            }
            else if (dataType != AZ::RPI::MaterialPropertyDataType::Invalid)
            {
                if (AZStd::holds_alternative<RHI::ShaderInputConstantIndex>(desc->m_srgInputIndex))
                {
                    isValid = AZStd::get<RHI::ShaderInputConstantIndex>(desc->m_srgInputIndex).IsValid();
                }
            }
            return isValid;
        }

        bool MaterialShaderParameterLayout::WriteMaterialParameterStructureAzsli(const AZ::IO::Path& filename) const
        {
            AZStd::string generatedAzsli;
            generatedAzsli += AZStd::string::format("// This code was generated from the MaterialShaderParameterLayout. Do not modify.\n");
            generatedAzsli += AZStd::string("#pragma once\n\n");
            generatedAzsli += AZStd::string("struct MaterialParameters {\n");

            auto typedMemberString = [](const MaterialShaderParameterDescriptor& entry, const size_t maxTypeNameLength = 6)
            {
                auto maxLength = AZStd::max(maxTypeNameLength + 1, entry.m_typeName.size() + 1);
                AZStd::string namePrefix = AZStd::string(maxLength - entry.m_typeName.size(), ' ');

                if (entry.m_structuredBufferBinding.m_elementCount > 1)
                {
                    return AZStd::string::format(
                        "    %s%s%s[%u];\n",
                        entry.m_typeName.c_str(),
                        namePrefix.c_str(),
                        entry.m_name.c_str(),
                        static_cast<uint32_t>(entry.m_structuredBufferBinding.m_elementCount));
                }
                else
                {
                    return AZStd::string::format("    %s%s%s;\n", entry.m_typeName.c_str(), namePrefix.c_str(), entry.m_name.c_str());
                }
            };

            if (!m_descriptors.empty())
            {
                size_t maxTypeNameLength = 0;
                for (const auto& entry : m_descriptors)
                {
                    maxTypeNameLength = AZStd::max(maxTypeNameLength, entry.m_typeName.size());
                }

                for (auto& entry : m_descriptors)
                {
                    generatedAzsli += typedMemberString(entry, maxTypeNameLength);
                }
            }
            generatedAzsli += AZStd::string("}; \n");
            generatedAzsli += AZStd::string("\n");

            if (!Utils::WriteFile(generatedAzsli, filename.c_str()).IsSuccess())
            {
                AZ_Assert(false, "Error writing MaterialParameterStruct to file %s", filename.c_str());
                return false;
            }
            return true;
        }

        void MaterialShaderParameterLayout::Reset()
        {
            m_names.Clear();
            m_descriptors.clear();
            m_matrixPaddingIndex = 0;
        }

        void MaterialShaderParameterNameIndex::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<MaterialShaderParameterNameIndex>()
                    ->Version(0)
                    ->Field("m_name", &MaterialShaderParameterNameIndex::m_name)
                    ->Field("m_index", &MaterialShaderParameterNameIndex::m_index);
            }
        }

        void MaterialShaderParameterNameIndex::ContextualizeName(const MaterialNameContext* context)
        {
            if (context && context->HasContextForSrgInputs())
            {
                context->ContextualizeSrgInput(m_name);
            }
        }

        bool MaterialShaderParameterNameIndex::ValidateOrFindIndex(const MaterialShaderParameterLayout* layout)
        {
            if (m_index.IsValid())
            {
                return true;
            }
            m_index = layout->GetParameterIndex(m_name.GetStringView());
            return m_index.IsValid();
        }

} // namespace AZ::RPI
