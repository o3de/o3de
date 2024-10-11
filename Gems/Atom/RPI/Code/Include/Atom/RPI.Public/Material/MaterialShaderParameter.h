/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/RHISystemInterface.h>
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
            MaterialShaderParameter(const MaterialShaderParameterLayout* layout);

            void SetShaderResourceGroup(Data::Instance<RPI::ShaderResourceGroup> srg)
            {
                m_shaderResourceGroup = AZStd::move(srg);
            }

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

            template<typename T>
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
                        SetMaterialSrgDeviceReadIndex(desc, deviceIndex, deviceReadIndex);
                        // try to set either the texture or the device - ReadIndex in the SRG, only one of these calls can succeed
                        SetMaterialSrgData(desc, image);
                    }
                    return true;
                }
                return false;
            }

            template<>
            bool SetParameter(const MaterialShaderParameterLayout::Index& index, const Data::Asset<ImageAsset>& imageAsset)
            {
                const auto* desc{ m_layout->GetDescriptor(index) };
                if (desc)
                {
                    int32_t readIndex = -1;
                    if (imageAsset)
                    {
                        // TODO: find out when this is used
                        // TODO: make this multi-device capable
                        // image->GetImageView()->GetDeviceImageView(RHI::MultiDevice::DefaultDeviceIndex)->GetBindlessReadIndex();
                        // SetMaterialSrgData(desc, imageAsset);
                    }
                    AZStd::span<const uint8_t> data(reinterpret_cast<const uint8_t*>(&readIndex), sizeof(int32_t));
                    SetStructuredBufferData(desc, data);
                    return true;
                }
                return false;
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

            uint64_t GetStructuredBufferDataSize() const
            {
                return m_structuredBufferData.at(RHI::MultiDevice::DefaultDeviceIndex).size();
            }

        private:
            void SetMaterialSrgDeviceReadIndex(
                const MaterialShaderParameterDescriptor* desc, const int deviceIndex, const int32_t readIndex)
            {
                if (!m_shaderResourceGroup)
                {
                    return;
                }
                auto* index = AZStd::get_if<RHI::ShaderInputConstantIndex>(&desc->m_srgInputIndex);
                if (index && index->IsValid())
                {
                    // TODO: this doesn't work, i can't set different constant values in different device-SRGs (yet)
                    // m_shaderResourceGroup->GetRHIShaderResourceGroup()
                    //     ->GetDeviceShaderResourceGroup(deviceIndex)
                    //     ->GetData()
                    //     ->SetConstant(index, readIndex);
                    // TODO: remove this as soon as the code above works
                    m_shaderResourceGroup->SetConstant(*index, readIndex);
                }
            }

            template<typename T>
            void SetMaterialSrgData(const MaterialShaderParameterDescriptor* desc, const T& value)
            {
                if (!m_shaderResourceGroup)
                {
                    return;
                }
                auto* index = AZStd::get_if<RHI::ShaderInputConstantIndex>(&desc->m_srgInputIndex);
                if (index && index->IsValid())
                {
                    m_shaderResourceGroup->SetConstant(*index, value);
                }
            }

            template<>
            void SetMaterialSrgData(const MaterialShaderParameterDescriptor* desc, const Data::Instance<Image>& value)
            {
                if (!m_shaderResourceGroup)
                {
                    return;
                }
                auto* index = AZStd::get_if<RHI::ShaderInputImageIndex>(&desc->m_srgInputIndex);
                if (index && index->IsValid())
                {
                    m_shaderResourceGroup->SetImage(*index, value);
                }
            }

            template<>
            void SetMaterialSrgData(const MaterialShaderParameterDescriptor* desc, const Data::Asset<ImageAsset>& value)
            {
                if (!m_shaderResourceGroup)
                {
                    return;
                }
                // TODO: figure out when this is called with an ImageAsset
                auto* index = AZStd::get_if<RHI::ShaderInputImageIndex>(&desc->m_srgInputIndex);
                if (index && index->IsValid())
                {
                    // m_shaderResourceGroup->SetImage(*index, value);
                }
            }

            void SetStructuredBufferData(const MaterialShaderParameterDescriptor* desc, const AZStd::span<const uint8_t>& data);
            void SetStructuredBufferData(
                const MaterialShaderParameterDescriptor* desc, const AZStd::span<const uint8_t>& deviceData, const int deviceIndex);

            const MaterialShaderParameterLayout* m_layout;
            AZStd::unordered_map<int, AZStd::vector<uint8_t>> m_structuredBufferData = {};
            Data::Instance<RPI::ShaderResourceGroup> m_shaderResourceGroup = nullptr;
        };

    } // namespace RPI
} // namespace AZ