/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Material/MaterialInstanceData.h>
#include <Atom/RPI.Public/Material/MaterialInstanceHandler.h>
#include <Atom/RPI.Public/Material/MaterialShaderParameter.h>
#include <Atom/RPI.Public/Material/SharedSamplerState.h>
#include <AzCore/Utils/Utils.h>


#include <Atom_RPI_Traits_Platform.h>

namespace AZ::RPI
{
    // utility class to reduce compile time by keeping some of the template functions away from the header
    // while still reducing code duplication a bit
    class TypedParameterHelper
    {
    public:
        MaterialShaderParameter* params;

        template<typename T>
        bool SetBasicParameter(const MaterialShaderParameterLayout::Index& index, const T& value)
        {
            const auto* desc{ params->m_layout->GetDescriptor(index) };
            if (desc)
            {
                AZStd::span<const uint8_t> data(reinterpret_cast<const uint8_t*>(&value), sizeof(T));
                params->SetStructuredBufferData(desc, data);
                SetMaterialSrgConstant(desc, value);
                return true;
            }
            return false;
        }

        template<typename T, unsigned int N, AZStd::enable_if_t<N <= 4, bool> = true>
        bool SetVectorParameter(const MaterialShaderParameterLayout::Index& index, const T value)
        {
            const auto* desc{ params->m_layout->GetDescriptor(index) };
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
                params->SetStructuredBufferData(desc, data);
                SetMaterialSrgConstant(desc, value);
                return true;
            }
            return false;
        }

        template<typename T>
        bool SetMaterialSrgConstant(const MaterialShaderParameterDescriptor* desc, const T& value)
        {
            if (!params->m_shaderResourceGroup)
            {
                return false;
            }
            auto* index = AZStd::get_if<RHI::ShaderInputConstantIndex>(&desc->m_srgInputIndex);
            if (index && index->IsValid())
            {
                return params->m_shaderResourceGroup->SetConstant(*index, value);
            }
            return false;
        }

        template<typename T>
        bool SetMaterialSrgMatrix(const MaterialShaderParameterDescriptor* desc, const T& value)
        {
            if (!params->m_shaderResourceGroup)
            {
                return false;
            }
            auto* index = AZStd::get_if<RHI::ShaderInputConstantIndex>(&desc->m_srgInputIndex);
            if (index && index->IsValid())
            {
                return params->m_shaderResourceGroup->SetConstantMatrixRows(*index, value, T::RowCount);
            }
            return false;
        }

        bool SetMaterialSrgImage(const MaterialShaderParameterDescriptor* desc, const Data::Instance<Image> value)
        {
            if (!params->m_shaderResourceGroup)
            {
                return false;
            }
            auto* index = AZStd::get_if<RHI::ShaderInputImageIndex>(&desc->m_srgInputIndex);
            if (index && index->IsValid())
            {
                return params->m_shaderResourceGroup->SetImage(*index, value);
            }
            return false;
        }
    };

    MaterialShaderParameter::MaterialShaderParameter(
        const int materialTypeIndex,
        const int materialInstanceIndex,
        const MaterialShaderParameterLayout* layout,
        Data::Instance<RPI::ShaderResourceGroup> srg)
        : m_layout(layout)
        , m_shaderResourceGroup(srg)
        , m_materialTypeIndex(materialTypeIndex)
        , m_materialInstanceIndex(materialInstanceIndex)
    {
        if (!m_layout->GetDescriptors().empty())
        {
            auto last{ m_layout->GetDescriptors().back().m_structuredBufferBinding };
            const auto deviceCount{ AZ::RHI::RHISystemInterface::Get()->GetDeviceCount() };
            for (auto deviceIndex{ 0 }; deviceIndex < deviceCount; ++deviceIndex)
            {
                m_structuredBufferData[deviceIndex].resize(last.m_offset + last.m_elementSize * last.m_elementCount, 0);
            }
        }
        else
        {
            AZ_Assert(false, "MaterialShaderParameter needs a layout");
        }
        SetParameter("m_materialType", m_materialTypeIndex);
        SetParameter("m_materialInstance", m_materialInstanceIndex);
    }

    void MaterialShaderParameter::SetStructuredBufferData(
        const MaterialShaderParameterDescriptor* desc, const AZStd::span<const uint8_t>& data)
    {
        const auto deviceCount{ AZ::RHI::RHISystemInterface::Get()->GetDeviceCount() };
        for (auto deviceIndex{ 0 }; deviceIndex < deviceCount; ++deviceIndex)
        {
            SetStructuredBufferData(desc, data, deviceIndex);
        }
    }

    void MaterialShaderParameter::SetStructuredBufferData(
        const MaterialShaderParameterDescriptor* desc, const AZStd::span<const uint8_t>& deviceData, const int deviceIndex)
    {
        auto& binding{ desc->m_structuredBufferBinding };

        AZ_Assert(
            binding.m_elementSize * binding.m_elementCount == deviceData.size_bytes(),
            "Size mismatch when setting the Material Shader Parameter data for %s %s: expected: %lld bytes, provided %lld bytes",
            desc->m_typeName.c_str(),
            desc->m_name.c_str(),
            binding.m_elementSize * binding.m_elementCount,
            deviceData.size_bytes());

        size_t minBufferSize = binding.m_offset + binding.m_elementSize * binding.m_elementCount;
        if (m_structuredBufferData[deviceIndex].size() < minBufferSize)
        {
            m_structuredBufferData[deviceIndex].resize(minBufferSize, 0);
        }
        for (int byteIndex = 0; byteIndex < deviceData.size(); ++byteIndex)
        {
            m_structuredBufferData[deviceIndex][binding.m_offset + byteIndex] = deviceData[byteIndex];
        }
    }

    AZStd::span<const uint8_t> MaterialShaderParameter::GetRawBufferParameterData(
        const MaterialShaderParameterDescriptor* desc, const uint32_t deviceIndex) const
    {
        AZ_Assert(desc, "GetRawBufferParameterData: desc is nullptr!");
        if (desc)
        {
            auto offset = desc->m_structuredBufferBinding.m_offset;
            auto size = desc->m_structuredBufferBinding.m_elementSize * desc->m_structuredBufferBinding.m_elementCount;
            for (auto& [device, structuredBufferData] : m_structuredBufferData)
            {
                if (device == deviceIndex)
                {
                    return AZStd::span<const uint8_t>{ &structuredBufferData[offset], size };
                }
            }
        }
        return {};
    }

    bool MaterialShaderParameter::SetMaterialSrgDeviceReadIndex(
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

    bool MaterialShaderParameter::SetParameter(const MaterialShaderParameterLayout::Index& index, const int value)
    {
        return TypedParameterHelper{ this }.SetBasicParameter(index, value);
    }
    bool MaterialShaderParameter::SetParameter(const MaterialShaderParameterLayout::Index& index, const uint32_t value)
    {
        return TypedParameterHelper{ this }.SetBasicParameter(index, value);
    }
    bool MaterialShaderParameter::SetParameter(const MaterialShaderParameterLayout::Index& index, const float value)
    {
        return TypedParameterHelper{ this }.SetBasicParameter(index, value);
    }
    bool MaterialShaderParameter::SetParameter(const MaterialShaderParameterLayout::Index& index, const bool value)
    {
        // booleans use 4 bytes on the GPU
        uint32_t boolean = value;
        return TypedParameterHelper{ this }.SetBasicParameter(index, boolean);
    }
    bool MaterialShaderParameter::SetParameter(const MaterialShaderParameterLayout::Index& index, const Vector2& value)
    {
        return TypedParameterHelper{ this }.SetVectorParameter<Vector2, 2>(index, value);
    }
    bool MaterialShaderParameter::SetParameter(const MaterialShaderParameterLayout::Index& index, const Vector3& value)
    {
        return TypedParameterHelper{ this }.SetVectorParameter<Vector3, 3>(index, value);
    }
    bool MaterialShaderParameter::SetParameter(const MaterialShaderParameterLayout::Index& index, const Vector4& value)
    {
        return TypedParameterHelper{ this }.SetVectorParameter<Vector4, 4>(index, value);
    }

    bool MaterialShaderParameter::SetParameter(const MaterialShaderParameterLayout::Index& index, const Color& value)
    {
        bool result = false;
        // first set the color as 4 floats in the parameter buffer
        const auto* desc{ m_layout->GetDescriptor(index) };
        if (desc)
        {
            AZStd::span<const uint8_t> data(reinterpret_cast<const uint8_t*>(&value), sizeof(Color));
            SetStructuredBufferData(desc, data);
            result = true;
        }

        if (!m_shaderResourceGroup)
        {
            return result;
        }
        // Color is special in the srg because it could map to either a float3 or a float4
        auto* srgIndex = AZStd::get_if<RHI::ShaderInputConstantIndex>(&desc->m_srgInputIndex);
        if (srgIndex && srgIndex->IsValid())
        {
            auto descriptor = m_shaderResourceGroup->GetLayout()->GetShaderInput(*srgIndex);
            if (descriptor.m_constantByteCount == 3 * sizeof(float))
            {
                result = m_shaderResourceGroup->SetConstant(*srgIndex, value.GetAsVector3());
            }
            else
            {
                result = m_shaderResourceGroup->SetConstant(*srgIndex, value);
            }
        }
        return result;
    }
    bool MaterialShaderParameter::SetParameter(const MaterialShaderParameterLayout::Index& index, const Matrix3x3& matrix)
    {
        const auto* desc{ m_layout->GetDescriptor(index) };
        if (desc)
        {
            // matrix3x3 has 3x Vector3, which store 4 floats for Simd reasons, so copy the data to a float[9]
            AZStd::array<float, 9> values;
            matrix.StoreToRowMajorFloat9(&values[0]);
            AZStd::span<const uint8_t> data(reinterpret_cast<const uint8_t*>(&values[0]), sizeof(float) * 9);
            SetStructuredBufferData(desc, data);
            TypedParameterHelper{ this }.SetMaterialSrgMatrix(desc, matrix);
            return true;
        }
        return false;
    }

    bool MaterialShaderParameter::SetParameter(const MaterialShaderParameterLayout::Index& index, const Matrix4x4& matrix)
    {
        const auto* desc{ m_layout->GetDescriptor(index) };
        if (desc)
        {
            AZStd::array<float, 16> values;
            matrix.StoreToRowMajorFloat16(&values[0]);
            AZStd::span<const uint8_t> data(reinterpret_cast<const uint8_t*>(&values[0]), sizeof(float) * 16);
            SetStructuredBufferData(desc, data);
            TypedParameterHelper{ this }.SetMaterialSrgMatrix(desc, matrix);
            return true;
        }
        return false;
    }

    bool MaterialShaderParameter::SetParameter(const MaterialShaderParameterLayout::Index& index, Data::Instance<Image> image)
    {
        const auto* desc{ m_layout->GetDescriptor(index) };
        if (desc)
        {
            const auto deviceCount{ AZ::RHI::RHISystemInterface::Get()->GetDeviceCount() };
            AZStd::unordered_map<int, uint32_t> deviceReadIndex;

            auto SameForAllDevices = [deviceCount](uint32_t value)
            {
                AZStd::unordered_map<int, uint32_t> result;
                for (auto deviceIndex{ 0 }; deviceIndex < deviceCount; ++deviceIndex)
                {
                    result[deviceIndex] = value;
                }
                return result;
            };
#ifdef AZ_TRAIT_REGISTER_TEXTURES_PER_MATERIAL
            int imageReadIndex{ -1 };
            auto* instanceHandler = MaterialInstanceHandlerInterface::Get();
            // register the texture for the material instance
            if (instanceHandler)
            {
                auto oldIndexIterator = m_materialTextureIndices.find(index);
                if (oldIndexIterator != m_materialTextureIndices.end())
                {
                    instanceHandler->ReleaseMaterialTexture(m_materialTypeIndex, m_materialInstanceIndex, oldIndexIterator->second);
                }
                if (image)
                {
                    imageReadIndex = instanceHandler->RegisterMaterialTexture(m_materialTypeIndex, m_materialInstanceIndex, image);
                }
                // keep track of which textures were already assigned
                m_materialTextureIndices[index] = imageReadIndex;
            }
            deviceReadIndex = SameForAllDevices(imageReadIndex);
#else
            if (image)
            {
                deviceReadIndex = image->GetImageView()->GetBindlessReadIndex();
            }
            else
            {
                deviceReadIndex = SameForAllDevices(RHI::DeviceImageView::InvalidBindlessIndex);
            }
#endif // AZ_TRAIT_REGISTER_TEXTURES_PER_MATERIAL

            for (auto deviceIndex{ 0 }; deviceIndex < deviceCount; ++deviceIndex)
            {
                AZStd::span<const uint8_t> data(reinterpret_cast<const uint8_t*>(&deviceReadIndex[deviceIndex]), sizeof(int32_t));
                SetStructuredBufferData(desc, data, deviceIndex);
                // try to set the texture read index in the SRG
                if (!SetMaterialSrgDeviceReadIndex(desc, deviceIndex, static_cast<int>(deviceReadIndex[deviceIndex])))
                {
                    // try to set the Image input in the SRG. Only one of these calls can succeeed, since we use the same name for
                    // the srg member both times
                    TypedParameterHelper{ this }.SetMaterialSrgImage(desc, image);
                }
            }
            return true;
        }
        return false;
    }

    bool MaterialShaderParameter::SetParameter(const MaterialShaderParameterLayout::Index& index, const RHI::SamplerState& samplerState)
    {
        const auto* desc{ m_layout->GetDescriptor(index) };
        if (desc)
        {
            uint32_t samplerIndex{ AZStd::numeric_limits<uint32_t>::max() };
            auto* instanceHandler = MaterialInstanceHandlerInterface::Get();
            if (instanceHandler)
            {
                auto sharedSampler = instanceHandler->RegisterTextureSampler(m_materialTypeIndex, m_materialInstanceIndex, samplerState);
                if (sharedSampler)
                {
                    samplerIndex = sharedSampler->m_samplerIndex;
                    m_sharedSamplerStates[index] = AZStd::move(sharedSampler);
                }
            }
            return TypedParameterHelper{ this }.SetBasicParameter(index, samplerIndex);
        }
        return false;
    }

    RHI::SamplerState MaterialShaderParameter::GetSharedSamplerState(const uint32_t samplerIndex) const
    {
        return MaterialInstanceHandlerInterface::Get()->GetRegisteredTextureSampler(
            m_materialTypeIndex, m_materialInstanceIndex, samplerIndex);
    }

    AZStd::unordered_map<int, const void*> MaterialShaderParameter::GetStructuredBufferData() const
    {
        AZStd::unordered_map<int, const void*> deviceData;
        const auto deviceCount{ AZ::RHI::RHISystemInterface::Get()->GetDeviceCount() };
        for (auto deviceIndex{ 0 }; deviceIndex < deviceCount; ++deviceIndex)
        {
            deviceData[deviceIndex] = m_structuredBufferData.at(deviceIndex).data();
        }
        return deviceData;
    }

} // namespace AZ::RPI
