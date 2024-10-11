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

namespace AZ
{
    namespace RPI
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
            struct GpuBindlessReadIndex
            {
                static const bool value = false;
            };

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

            template<> struct StructuredBufferTypeSize<Data::Asset<Image>> { static const size_t value = sizeof(int32_t); };
            template<> struct GpuTypeName<Data::Asset<Image>> { static constexpr const char* value = "int"; };
            template<> struct GpuBindlessReadIndex<Data::Asset<Image>> { static const bool value = true; };
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
                    gpuTypesize,
                    count,
                    getStructuredBufferOffset(gpuTypesize * count),
                };
                m_descriptors.emplace_back(
                    MaterialShaderParameterDescriptor{ AZStd::string(name), typeName, bufferBinding, {}, false, isPseudoparam });
            }
            else
            {
                auto* desc = GetDescriptor(parameterIndex);
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
            }
            return parameterIndex;
        }

        template<typename T>
        MaterialShaderParameterLayout::Index MaterialShaderParameterLayout::AddMaterialParameter(
            const AZStd::string_view& name, bool isPseudoParam, const size_t count)
        {
            // Hack: AZSL complains about the layout of variables following a float3x3. So add a float4 as padding
            if (!m_descriptors.empty() && m_descriptors.back().m_typeName == "float3x3" && GpuTypeName<T>::value != "float3x3" &&
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
                    StructuredBufferTypeSize<T>::value,
                    count,
                    getStructuredBufferOffset(StructuredBufferTypeSize<T>::value * count),
                };

                m_descriptors.emplace_back(MaterialShaderParameterDescriptor{
                    AZStd::string(name), GpuTypeName<T>::value, bufferBinding, {}, GpuBindlessReadIndex<T>::value, isPseudoParam });
            }
            else
            {
                auto* desc = GetDescriptor(parameterIndex);
                AZ_Assert(
                    desc->m_structuredBufferBinding.m_elementCount == count,
                    "MaterialParameterBuffer: Redefinition of Buffer entry %s with element count (%d -> %d)",
                    AZStd::string(name).c_str(),
                    desc->m_structuredBufferBinding.m_elementCount,
                    count);
                AZ_Assert(
                    desc->m_typeName == GpuTypeName<T>::value,
                    "MaterialParameterBuffer: Redefinition of Buffer entry %s with new type: %s -> %s",
                    AZStd::string(name).c_str(),
                    desc->m_typeName.c_str(),
                    GpuTypeName<T>::value);
            }
            return parameterIndex;
        }
        template<typename T>
        MaterialShaderParameterLayout::Index MaterialShaderParameterLayout::AddMaterialParameter(
            const Name& name, const bool isPseudoParam, const size_t count)
        {
            return AddMaterialParameter<T>(name.GetStringView(), isPseudoParam, count);
        }

        // explicit instantiation
        template MaterialShaderParameterLayout::Index MaterialShaderParameterLayout::AddMaterialParameter<AZ::Matrix3x3>(
            const AZStd::string_view&, const bool isPseudoParam, const size_t count);
        template MaterialShaderParameterLayout::Index MaterialShaderParameterLayout::AddMaterialParameter<AZ::Matrix3x4>(
            const AZStd::string_view&, const bool isPseudoParam, const size_t count);
        template MaterialShaderParameterLayout::Index MaterialShaderParameterLayout::AddMaterialParameter<AZ::Matrix4x4>(
            const AZStd::string_view&, const bool isPseudoParam, const size_t count);
        template MaterialShaderParameterLayout::Index MaterialShaderParameterLayout::AddMaterialParameter<AZ::Matrix3x3>(
            const Name&, const bool isPseudoParam, const size_t count);
        template MaterialShaderParameterLayout::Index MaterialShaderParameterLayout::AddMaterialParameter<AZ::Matrix3x4>(
            const Name&, const bool isPseudoParam, const size_t count);
        template MaterialShaderParameterLayout::Index MaterialShaderParameterLayout::AddMaterialParameter<AZ::Matrix4x4>(
            const Name&, const bool isPseudoParam, const size_t count);

        void MaterialShaderParameterDescriptor::BufferBinding::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<MaterialShaderParameterDescriptor::BufferBinding>()
                    ->Version(0)
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

            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<MaterialShaderParameterLayout>()
                    ->Version(0)
                    ->Field("m_names", &MaterialShaderParameterLayout::m_names)
                    ->Field("m_descriptors", &MaterialShaderParameterLayout::m_descriptors);
            }
        }

        MaterialShaderParameterLayout::Index MaterialShaderParameterLayout::AddMaterialParameter(
            const Name& name, const MaterialPropertyDataType dataType, const bool isPseudoParam, const size_t count)
        {
            return AddMaterialParameter(name.GetStringView(), dataType, isPseudoParam, count);
        }

        MaterialShaderParameterLayout::Index MaterialShaderParameterLayout::AddMaterialParameter(
            const AZStd::string_view& name, const MaterialPropertyDataType dataType, const bool isPseudoParam, const size_t count)
        {
            MaterialShaderParameterLayout::Index parameterIndex{ MaterialShaderParameterLayout::Index::Null };
            switch (dataType)
            {
            case MaterialPropertyDataType::Bool:
                parameterIndex = AddMaterialParameter<bool>(name, isPseudoParam, count);
                break;
            case MaterialPropertyDataType::UInt:
                parameterIndex = AddMaterialParameter<uint32_t>(name, isPseudoParam, count);
                break;
            case MaterialPropertyDataType::Float:
                parameterIndex = AddMaterialParameter<float>(name, isPseudoParam, count);
                break;
            case MaterialPropertyDataType::Vector2:
                parameterIndex = AddMaterialParameter<Vector2>(name, isPseudoParam, count);
                break;
            case MaterialPropertyDataType::Vector3:
                parameterIndex = AddMaterialParameter<Vector3>(name, isPseudoParam, count);
                break;
            case MaterialPropertyDataType::Vector4:
                parameterIndex = AddMaterialParameter<Vector4>(name, isPseudoParam, count);
                break;
            case MaterialPropertyDataType::Color:
                parameterIndex = AddMaterialParameter<Color>(name, isPseudoParam, count);
                break;
            case MaterialPropertyDataType::Int:
            case MaterialPropertyDataType::Enum:
                parameterIndex = AddMaterialParameter<int32_t>(name, isPseudoParam, count);
                break;
            case MaterialPropertyDataType::Image:
                parameterIndex = AddMaterialParameter<Data::Asset<Image>>(name, isPseudoParam, count);
                break;
            default:
                AZ_Error(
                    "MaterialShaderParameterLayout",
                    false,
                    "Material property '%s': Properties of this type cannot be mapped to a ShaderResourceGroup input.",
                    AZStd::string(name).c_str());
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

        size_t MaterialShaderParameterLayout::getStructuredBufferOffset(size_t size) const
        {
            if (m_descriptors.empty())
            {
                return 0;
            }
            const auto& lastEntry{ m_descriptors.back().m_structuredBufferBinding };

            auto byteIndex = (lastEntry.m_offset + (lastEntry.m_elementSize * lastEntry.m_elementCount)) / 4;
            return byteIndex * 4;
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

            // Note: the shader material functions provided by the engine take all parameters from a MaterialParameter - struct.
            // Using the MaterialParameter-buffers from the global MaterialSRg only works if we know the exact layout of the struct on the
            // GPU, and we only know that if we generated the struct during the materialpipeline processing (when we turn an abstract
            // materialtype into a non-abstract materialtype). Shaders unaffected by the material pipeline (e.g. the SilhouetteGather -
            // shader) can manually define a MaterialParameter - struct which resides in a member with the name "m_params" inside the
            // Material-SRG.

            auto inputName = AZ::Name{ AZStd::string::format("m_params.%s", desc->m_name.c_str()) };
            if (!assignSrgIndex(inputName))
            {
                // Backwards compatability with old materials that aren't using the shader material functions from the engine:
                // look for the parameters in the srg directly
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

    } // namespace RPI
} // namespace AZ
