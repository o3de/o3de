/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/Material/MaterialInstanceHandler.h>
#include <Atom/RPI.Public/Material/MaterialShaderParameterLayout.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Reflect/Image/Image.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Name/Name.h>
#include <AzCore/std/containers/vector.h>

namespace AZ
{
    namespace RPI
    {
        //! Places material parameters in a ByteAdressBuffer
        class MaterialShaderParameter : public AZStd::intrusive_base
        {
        public:
            MaterialShaderParameter(
                const int materialTypeIndex,
                const int materialInstanceIndex,
                const MaterialShaderParameterLayout* layout,
                Data::Instance<RPI::ShaderResourceGroup> srg);

            template<typename T>
            bool SetArrayParameter(const AZStd::string_view& name, const AZStd::vector<T>& values)
            {
                const auto* desc{ m_layout->GetDescriptor(m_layout->GetParameterIndex(name)) };
                AZ_Assert(desc != nullptr, "Member %s not found in MaterialShaderParameter", AZStd::string(name).c_str());
                if (desc)
                {
                    AZ_Assert(
                        desc->m_structuredBufferBinding.m_elementCount == values.size(),
                        "Member %s expects %d values in MaterialShaderParameter",
                        AZStd::string(name).c_str(),
                        desc->m_structuredBufferBinding.m_elementCount);
                    AZStd::span<uint8_t> data(values.data(), sizeof(T) * values.size());
                    SetStructuredBufferData(desc, data);
                    return true;
                }
                return false;
            }

            template<typename T>
            bool SetParameter(const AZ::Name& name, const T& value)
            {
                return SetParameter(name.GetStringView(), value);
            }
            template<typename T>
            bool SetParameter(const AZStd::string_view& name, const T& value)
            {
                auto index = m_layout->GetParameterIndex(name);
                if (index.IsValid())
                {
                    return SetParameter(index, value);
                }
                AZ_Assert(index.IsValid(), "Member index %s not found in MaterialShaderParameter", AZStd::string(name).c_str());
                return false;
            }

            template<typename T, AZStd::enable_if_t<!AZStd::is_same_v<T, Data::Asset<ImageAsset>>, bool> = true>
            bool SetParameter(const MaterialShaderParameterLayout::Index& index, const T& value)
            {
                const auto* desc{ m_layout->GetDescriptor(index) };
                if (desc)
                {
                    AZStd::span<const uint8_t> data(reinterpret_cast<const uint8_t*>(&value), sizeof(T));
                    SetStructuredBufferData(desc, data);
                    SetMaterialSrgData(desc, value);
                    return true;
                }
                return false;
            }

            template<>
            bool SetParameter(const MaterialShaderParameterLayout::Index& index, const bool& boolean)
            {
                const auto* desc{ m_layout->GetDescriptor(index) };
                if (desc)
                {
                    // bools are stored in 4 bytes
                    uint32_t value = boolean;
                    AZStd::span<const uint8_t> data(reinterpret_cast<const uint8_t*>(&value), sizeof(uint32_t));
                    SetStructuredBufferData(desc, data);
                    SetMaterialSrgData(desc, value);
                    return true;
                }
                return false;
            }

            template<typename T, unsigned int N, AZStd::enable_if_t<N <= 4, bool> = true>
            bool SetVectorParameter(const MaterialShaderParameterLayout::Index& index, const T value)
            {
                const auto* desc{ m_layout->GetDescriptor(index) };
                if (desc)
                {
                    // Vector3 has 4 floats for Simd reasons, so copy the data to a float[3]
                    AZStd::array<float, N> values;
                    values[0] = value.GetX();
                    if constexpr (N >= 2)
                    {
                        values[1] = value.GetY();
                    }
                    if constexpr (N >= 3)
                    {
                        values[2] = value.GetZ();
                    }
                    if constexpr (N == 4)
                    {
                        values[3] = value.GetW();
                    }
                    AZStd::span<const uint8_t> data(reinterpret_cast<const uint8_t*>(&values[0]), sizeof(float) * N);
                    SetStructuredBufferData(desc, data);
                    SetMaterialSrgData(desc, value);
                    return true;
                }
                return false;
            }

            template<>
            bool SetParameter(const MaterialShaderParameterLayout::Index& index, const Vector4& value)
            {
                return SetVectorParameter<Vector4, 4>(index, value);
            }
            template<>
            bool SetParameter(const MaterialShaderParameterLayout::Index& index, const Vector3& value)
            {
                return SetVectorParameter<Vector3, 3>(index, value);
            }
            template<>
            bool SetParameter(const MaterialShaderParameterLayout::Index& index, const Vector2& value)
            {
                return SetVectorParameter<Vector2, 2>(index, value);
            }

            template<>
            bool SetParameter(const MaterialShaderParameterLayout::Index& index, const Matrix3x3& matrix)
            {
                const auto* desc{ m_layout->GetDescriptor(index) };
                if (desc)
                {
                    // matrix3x3 has 3x Vector3, which store 4 floats for Simd reasons, so copy the data to a float[9]
                    AZStd::array<float, 9> values;
                    matrix.StoreToRowMajorFloat9(&values[0]);
                    AZStd::span<const uint8_t> data(reinterpret_cast<const uint8_t*>(&values[0]), sizeof(float) * 9);
                    SetStructuredBufferData(desc, data);
                    SetMaterialSrgData(desc, matrix);
                    return true;
                }
                return false;
            }

            template<>
            bool SetParameter(const MaterialShaderParameterLayout::Index& index, const Data::Instance<Image>& image)
            {
                const auto* desc{ m_layout->GetDescriptor(index) };
                if (desc)
                {
#ifdef USE_BINDLESS_SRG
                    const auto deviceCount{ AZ::RHI::RHISystemInterface::Get()->GetDeviceCount() };
                    for (auto deviceIndex{ 0 }; deviceIndex < deviceCount; ++deviceIndex)
                    {
                        int deviceReadIndex{ -1 };
                        if (image)
                        {
                            deviceReadIndex =
                                static_cast<int32_t>(image->GetImageView()->GetDeviceImageView(deviceIndex)->GetBindlessReadIndex());
                        }
                        AZStd::span<const uint8_t> data(reinterpret_cast<const uint8_t*>(&deviceReadIndex), sizeof(int32_t));
                        SetStructuredBufferData(desc, data, deviceIndex);
                        // try to set the texture read index in the SRG
                        if (!SetMaterialSrgDeviceReadIndex(desc, deviceIndex, deviceReadIndex))
                        {
                            // try to set the Image input in the SRG. Only one of these calls can succeeed, since we use the same name for
                            // the srg member both times
                            SetMaterialSrgData(desc, image);
                        }
                    }
                    return true;
#else
                    if (!SetMaterialSrgData(desc, image))
                    {
                        int imageReadIndex{ -1 };
                        auto* instanceHandler = MaterialInstanceHandlerInterface::Get();
                        if (instanceHandler)
                        {
                            int imageIndex = instanceHandler->RegisterMaterialTexture(m_materialTypeIndex, m_materialInstanceIndex, image);
                        }
                        return SetMaterialSrgData(desc, imageReadIndex);
                    }
                    else
                    {
                        return true;
                    }
#endif
                }
                else
                {
                    return false;
                }
            }

            bool SetParameterRaw(const MaterialShaderParameterLayout::Index& index, const AZStd::span<uint8_t>& data)
            {
                const auto* desc{ m_layout->GetDescriptor(index) };
                if (desc)
                {
                    SetStructuredBufferData(desc, data);
                    return true;
                }
                return false;
            }

            AZStd::unordered_map<int, const void*> GetStructuredBufferData() const
            {
                AZStd::unordered_map<int, const void*> deviceData;
                const auto deviceCount{ AZ::RHI::RHISystemInterface::Get()->GetDeviceCount() };
                for (auto deviceIndex{ 0 }; deviceIndex < deviceCount; ++deviceIndex)
                {
                    deviceData[deviceIndex] = m_structuredBufferData.at(deviceIndex).data();
                }
                return deviceData;
            }

            template<typename T>
            T GetShaderParameterData(const MaterialShaderParameterLayout::Index& index, const uint32_t deviceIndex = 0) const
            {
                T result{};
                auto desc = m_layout->GetDescriptor(index);
                if (desc)
                {
                    auto rawData = GetRawBufferParameterData(desc, deviceIndex);
                    if (sizeof(result) != rawData.size_bytes())
                    {
                        AZ_Assert(
                            false,
                            "GetShaderParameterData for parameter %s expects wrong type size (expected: %llu, actual: %llu)",
                            desc->m_typeName.c_str(),
                            sizeof(result),
                            rawData.size());
                    }
                    else
                    {
                        result = *reinterpret_cast<const T*>(rawData.data());
                    }
                }
                return result;
            }

            template<>
            bool GetShaderParameterData(const MaterialShaderParameterLayout::Index& index, const uint32_t deviceIndex) const
            {
                auto result = GetShaderParameterData<uint32_t>(index, deviceIndex);
                AZ_Assert(result == 0 || result == 1, "GetShaderParameterData: GPU Boolean contains illegal value %d", result);
                if (result == 0)
                {
                    return false;
                }
                return true;
            }

            template<>
            Matrix3x3 GetShaderParameterData(const MaterialShaderParameterLayout::Index& index, const uint32_t deviceIndex) const
            {
                auto result = GetShaderParameterData<AZStd::array<float, 9>>(index, deviceIndex);
                return Matrix3x3::CreateFromRowMajorFloat9(result.data());
            }

            template<>
            Matrix4x4 GetShaderParameterData(const MaterialShaderParameterLayout::Index& index, const uint32_t deviceIndex) const
            {
                auto result = GetShaderParameterData<AZStd::array<float, 16>>(index, deviceIndex);
                return Matrix4x4::CreateFromRowMajorFloat16(result.data());
            }

            template<typename T, unsigned int N, AZStd::enable_if_t<N <= 4, bool> = true>
            T GetVectorShaderParameterData(const MaterialShaderParameterLayout::Index& index, const uint32_t deviceIndex = 0) const
            {
                AZStd::array<float, N> result = GetShaderParameterData<AZStd::array<float, N>>(index, deviceIndex);
                T vectorResult;
                for (int ele = 0; ele < N; ++ele)
                {
                    vectorResult.SetElement(ele, result[ele]);
                }
                return vectorResult;
            }

            template<>
            Vector2 GetShaderParameterData(const MaterialShaderParameterLayout::Index& index, const uint32_t deviceIndex) const
            {
                return GetVectorShaderParameterData<Vector2, 2>(index, deviceIndex);
            }
            template<>
            Vector3 GetShaderParameterData(const MaterialShaderParameterLayout::Index& index, const uint32_t deviceIndex) const
            {
                return GetVectorShaderParameterData<Vector3, 3>(index, deviceIndex);
            }
            template<>
            Vector4 GetShaderParameterData(const MaterialShaderParameterLayout::Index& index, const uint32_t deviceIndex) const
            {
                return GetVectorShaderParameterData<Vector4, 4>(index, deviceIndex);
            }

            template<>
            Color GetShaderParameterData(const MaterialShaderParameterLayout::Index& index, const uint32_t deviceIndex) const
            {
                return GetVectorShaderParameterData<Color, 4>(index, deviceIndex);
            }

            AZStd::span<const uint8_t> GetRawBufferParameterData(
                const MaterialShaderParameterDescriptor* desc, const uint32_t deviceIndex = 0) const;

            uint64_t GetStructuredBufferDataSize() const
            {
                return m_structuredBufferData.at(RHI::MultiDevice::DefaultDeviceIndex).size();
            }

        private:
            bool SetMaterialSrgDeviceReadIndex(
                const MaterialShaderParameterDescriptor* desc, [[maybe_unused]] const int deviceIndex, const int32_t readIndex)
            {
                if (!m_shaderResourceGroup)
                {
                    return false;
                }
                auto* index = AZStd::get_if<RHI::ShaderInputConstantIndex>(&desc->m_srgInputIndex);
                if (index && index->IsValid())
                {
                    // TODO: this doesn't work, i can't set different constant values in different device-SRGs (yet)
                    // m_shaderResourceGroup->GetRHIShaderResourceGroup()
                    //     ->GetDeviceShaderResourceGroup(deviceIndex)
                    //     ->GetData()
                    //     ->SetConstant(index, readIndex);
                    return m_shaderResourceGroup->SetConstant(*index, readIndex);
                }
                return false;
            }

            template<typename T, AZStd::enable_if_t<!AZStd::is_same_v<T, Data::Asset<ImageAsset>>, bool> = true>
            bool SetMaterialSrgData(const MaterialShaderParameterDescriptor* desc, const T& value)
            {
                if (!m_shaderResourceGroup)
                {
                    return false;
                }
                auto* index = AZStd::get_if<RHI::ShaderInputConstantIndex>(&desc->m_srgInputIndex);
                if (index && index->IsValid())
                {
                    return m_shaderResourceGroup->SetConstant(*index, value);
                }
                return false;
            }

            template<>
            bool SetMaterialSrgData(const MaterialShaderParameterDescriptor* desc, const Color& value)
            {
                if (!m_shaderResourceGroup)
                {
                    return false;
                }
                auto* index = AZStd::get_if<RHI::ShaderInputConstantIndex>(&desc->m_srgInputIndex);
                if (index && index->IsValid())
                {
                    // Color is special because it could map to either a float3 or a float4
                    auto descriptor = m_shaderResourceGroup->GetLayout()->GetShaderInput(*index);
                    if (descriptor.m_constantByteCount == 3 * sizeof(float))
                    {
                        return m_shaderResourceGroup->SetConstant(*index, value.GetAsVector3());
                    }
                    else
                    {
                        return m_shaderResourceGroup->SetConstant(*index, value);
                    }
                }
                return false;
            }

            template<>
            bool SetMaterialSrgData(const MaterialShaderParameterDescriptor* desc, const Data::Instance<Image>& value)
            {
                if (!m_shaderResourceGroup)
                {
                    return false;
                }
                auto* index = AZStd::get_if<RHI::ShaderInputImageIndex>(&desc->m_srgInputIndex);
                if (index && index->IsValid())
                {
                    return m_shaderResourceGroup->SetImage(*index, value);
                }
                return false;
            }

            void SetStructuredBufferData(const MaterialShaderParameterDescriptor* desc, const AZStd::span<const uint8_t>& data);
            void SetStructuredBufferData(
                const MaterialShaderParameterDescriptor* desc, const AZStd::span<const uint8_t>& deviceData, const int deviceIndex);

            const MaterialShaderParameterLayout* m_layout;
            AZStd::unordered_map<uint32_t, AZStd::vector<uint8_t>> m_structuredBufferData = {};
            Data::Instance<RPI::ShaderResourceGroup> m_shaderResourceGroup = nullptr;
            int m_materialTypeIndex;
            int m_materialInstanceIndex;
        };

    } // namespace RPI
} // namespace AZ