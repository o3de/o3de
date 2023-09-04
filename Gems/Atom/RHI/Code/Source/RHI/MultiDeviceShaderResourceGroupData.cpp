/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI.Reflect/Bits.h>
#include <Atom/RHI/MultiDeviceBufferPool.h>
#include <Atom/RHI/MultiDeviceShaderResourceGroupData.h>
#include <Atom/RHI/MultiDeviceShaderResourceGroupPool.h>
#include <Atom/RHI/RHISystemInterface.h>

namespace AZ::RHI
{
    MultiDeviceShaderResourceGroupData::MultiDeviceShaderResourceGroupData(const MultiDeviceShaderResourceGroup& shaderResourceGroup)
        : MultiDeviceShaderResourceGroupData(*shaderResourceGroup.GetPool())
    {
    }

    MultiDeviceShaderResourceGroupData::MultiDeviceShaderResourceGroupData(
        const MultiDeviceShaderResourceGroupPool& shaderResourceGroupPool)
        : MultiDeviceShaderResourceGroupData(shaderResourceGroupPool.GetDeviceMask(), shaderResourceGroupPool.GetLayout())
    {
    }

    MultiDeviceShaderResourceGroupData::MultiDeviceShaderResourceGroupData(
        MultiDevice::DeviceMask deviceMask, const ShaderResourceGroupLayout* layout)
        : m_shaderResourceGroupLayout(layout)
        , m_deviceMask(deviceMask)
    {
        auto deviceCount{ RHI::RHISystemInterface::Get()->GetDeviceCount() };

        for (int deviceIndex = 0; deviceIndex < deviceCount; ++deviceIndex)
        {
            if ((AZStd::to_underlying(m_deviceMask) >> deviceIndex) & 1)
            {
                m_deviceShaderResourceGroupDatas[deviceIndex] = ShaderResourceGroupData(layout);
            }
        }
    }

    const ShaderResourceGroupLayout* MultiDeviceShaderResourceGroupData::GetLayout() const
    {
        return m_shaderResourceGroupLayout.get();
    }

    bool MultiDeviceShaderResourceGroupData::ValidateSetImageView(
        ShaderInputImageIndex inputIndex, const MultiDeviceImageView* imageView, uint32_t arrayIndex) const
    {
        if (!Validation::IsEnabled())
        {
            return true;
        }
        if (!GetLayout()->ValidateAccess(inputIndex, arrayIndex))
        {
            return false;
        }

        if (imageView)
        {
            if (!ValidateImageViewAccess<ShaderInputImageIndex, ShaderInputImageDescriptor>(inputIndex, imageView, arrayIndex))
            {
                return false;
            }
        }

        return true;
    }

    bool MultiDeviceShaderResourceGroupData::ValidateSetBufferView(
        ShaderInputBufferIndex inputIndex, const MultiDeviceBufferView* bufferView, uint32_t arrayIndex) const
    {
        if (!Validation::IsEnabled())
        {
            return true;
        }
        if (!GetLayout()->ValidateAccess(inputIndex, arrayIndex))
        {
            return false;
        }

        if (bufferView)
        {
            if (!ValidateBufferViewAccess<ShaderInputBufferIndex, ShaderInputBufferDescriptor>(inputIndex, bufferView, arrayIndex))
            {
                return false;
            }
        }

        return true;
    }

    ShaderInputBufferIndex MultiDeviceShaderResourceGroupData::FindShaderInputBufferIndex(const Name& name) const
    {
        return m_shaderResourceGroupLayout->FindShaderInputBufferIndex(name);
    }

    ShaderInputImageIndex MultiDeviceShaderResourceGroupData::FindShaderInputImageIndex(const Name& name) const
    {
        return m_shaderResourceGroupLayout->FindShaderInputImageIndex(name);
    }

    ShaderInputSamplerIndex MultiDeviceShaderResourceGroupData::FindShaderInputSamplerIndex(const Name& name) const
    {
        return m_shaderResourceGroupLayout->FindShaderInputSamplerIndex(name);
    }

    ShaderInputConstantIndex MultiDeviceShaderResourceGroupData::FindShaderInputConstantIndex(const Name& name) const
    {
        return m_shaderResourceGroupLayout->FindShaderInputConstantIndex(name);
    }

    bool MultiDeviceShaderResourceGroupData::SetImageView(
        ShaderInputImageIndex inputIndex, const MultiDeviceImageView* imageView, uint32_t arrayIndex = 0)
    {
        AZStd::array<const MultiDeviceImageView* const, 1> imageViews = { { imageView } };
        return SetImageViewArray(inputIndex, imageViews, arrayIndex);
    }

    bool MultiDeviceShaderResourceGroupData::SetImageViewArray(
        ShaderInputImageIndex inputIndex, AZStd::span<const MultiDeviceImageView* const> imageViews, uint32_t arrayIndex)
    {
        if (GetLayout()->ValidateAccess(inputIndex, static_cast<uint32_t>(arrayIndex + imageViews.size() - 1)))
        {
            bool isValidAll = true;

            for (size_t i = 0; i < imageViews.size(); ++i)
            {
                isValidAll &= ValidateSetImageView(inputIndex, imageViews[i], aznumeric_caster(arrayIndex + i));
            }

            for (auto& [deviceIndex, deviceShaderResourceGroupData] : m_deviceShaderResourceGroupDatas)
            {
                AZStd::vector<const ImageView*> deviceImageViews(imageViews.size());

                for (int imageIndex = 0; imageIndex < imageViews.size(); ++imageIndex)
                {
                    auto [image, imageViewDescriptor]{ *imageViews[imageIndex] };
                    deviceImageViews[imageIndex] = image->GetDeviceImage(deviceIndex)->GetImageView(imageViewDescriptor).get();
                }

                isValidAll &= deviceShaderResourceGroupData.SetImageViewArray(inputIndex, deviceImageViews, arrayIndex);
            }

            if (!imageViews.empty())
            {
                EnableResourceTypeCompilation(ResourceTypeMask::ImageViewMask);
            }

            return isValidAll;
        }
        return false;
    }

    bool MultiDeviceShaderResourceGroupData::SetImageViewUnboundedArray(
        ShaderInputImageUnboundedArrayIndex inputIndex, AZStd::span<const MultiDeviceImageView* const> imageViews)
    {
        if (GetLayout()->ValidateAccess(inputIndex))
        {
            bool isValidAll = true;

            for (auto& [deviceIndex, deviceShaderResourceGroupData] : m_deviceShaderResourceGroupDatas)
            {
                AZStd::vector<const ImageView*> deviceImageViews(imageViews.size());

                for (int i = 0; i < imageViews.size(); ++i)
                {
                    auto [image, imageViewDescriptor]{ *imageViews[i] };
                    deviceImageViews[i] = image->GetDeviceImage(deviceIndex)->GetImageView(imageViewDescriptor).get();
                }

                isValidAll &= deviceShaderResourceGroupData.SetImageViewUnboundedArray(inputIndex, deviceImageViews);
            }

            for (size_t i = 0; i < imageViews.size(); ++i)
            {
                const bool isValid = ValidateImageViewAccess<ShaderInputImageUnboundedArrayIndex, ShaderInputImageUnboundedArrayDescriptor>(
                    inputIndex, imageViews[i], static_cast<uint32_t>(i));
                isValidAll &= isValid;
            }

            if (!imageViews.empty())
            {
                EnableResourceTypeCompilation(ResourceTypeMask::ImageViewUnboundedArrayMask);
            }
            return isValidAll;
        }
        return false;
    }

    bool MultiDeviceShaderResourceGroupData::SetBufferView(
        ShaderInputBufferIndex inputIndex, const MultiDeviceBufferView* bufferView, uint32_t arrayIndex)
    {
        AZStd::array<const MultiDeviceBufferView* const, 1> bufferViews = { { bufferView } };
        return SetBufferViewArray(inputIndex, bufferViews, arrayIndex);
    }

    bool MultiDeviceShaderResourceGroupData::SetBufferViewArray(
        ShaderInputBufferIndex inputIndex, AZStd::span<const MultiDeviceBufferView* const> bufferViews, uint32_t arrayIndex)
    {
        if (GetLayout()->ValidateAccess(inputIndex, static_cast<uint32_t>(arrayIndex + bufferViews.size() - 1)))
        {
            bool isValidAll = true;

            for (size_t i = 0; i < bufferViews.size(); ++i)
            {
                isValidAll &= ValidateSetBufferView(inputIndex, bufferViews[i], arrayIndex);
            }

            for (auto& [deviceIndex, deviceShaderResourceGroupData] : m_deviceShaderResourceGroupDatas)
            {
                AZStd::vector<const BufferView*> deviceBufferViews(bufferViews.size());

                for (int bufferIndex = 0; bufferIndex < bufferViews.size(); ++bufferIndex)
                {
                    auto [buffer, bufferViewDescriptor]{ *bufferViews[bufferIndex] };
                    deviceBufferViews[bufferIndex] = buffer->GetDeviceBuffer(deviceIndex)->GetBufferView(bufferViewDescriptor).get();
                }

                isValidAll &= deviceShaderResourceGroupData.SetBufferViewArray(inputIndex, deviceBufferViews, arrayIndex);
            }

            if (!bufferViews.empty())
            {
                EnableResourceTypeCompilation(ResourceTypeMask::BufferViewMask);
            }
            return isValidAll;
        }
        return false;
    }

    bool MultiDeviceShaderResourceGroupData::SetBufferViewUnboundedArray(
        ShaderInputBufferUnboundedArrayIndex inputIndex, AZStd::span<const MultiDeviceBufferView* const> bufferViews)
    {
        if (GetLayout()->ValidateAccess(inputIndex))
        {
            bool isValidAll = true;

            for (auto& [deviceIndex, deviceShaderResourceGroupData] : m_deviceShaderResourceGroupDatas)
            {
                AZStd::vector<const BufferView*> deviceBufferViews(bufferViews.size());

                for (int bufferIndex = 0; bufferIndex < bufferViews.size(); ++bufferIndex)
                {
                    auto [buffer, bufferViewDescriptor]{ *bufferViews[bufferIndex] };
                    deviceBufferViews[bufferIndex] = buffer->GetDeviceBuffer(deviceIndex)->GetBufferView(bufferViewDescriptor).get();
                }

                isValidAll &= deviceShaderResourceGroupData.SetBufferViewUnboundedArray(inputIndex, deviceBufferViews);
            }

            for (size_t i = 0; i < bufferViews.size(); ++i)
            {
                const bool isValid =
                    ValidateBufferViewAccess<ShaderInputBufferUnboundedArrayIndex, ShaderInputBufferUnboundedArrayDescriptor>(
                        inputIndex, bufferViews[i], static_cast<uint32_t>(i));
                isValidAll &= isValid;
            }

            if (!bufferViews.empty())
            {
                EnableResourceTypeCompilation(ResourceTypeMask::BufferViewUnboundedArrayMask);
            }
            return isValidAll;
        }
        return false;
    }

    bool MultiDeviceShaderResourceGroupData::SetSampler(
        ShaderInputSamplerIndex inputIndex, const SamplerState& sampler, uint32_t arrayIndex)
    {
        return SetSamplerArray(inputIndex, AZStd::span<const SamplerState>(&sampler, 1), arrayIndex);
    }

    bool MultiDeviceShaderResourceGroupData::SetSamplerArray(
        ShaderInputSamplerIndex inputIndex, AZStd::span<const SamplerState> samplers, uint32_t arrayIndex)
    {
        if (GetLayout()->ValidateAccess(inputIndex, static_cast<uint32_t>(arrayIndex + samplers.size() - 1)))
        {
            bool isValidAll = true;

            for (auto& [deviceIndex, deviceShaderResourceGroupData] : m_deviceShaderResourceGroupDatas)
            {
                isValidAll &= deviceShaderResourceGroupData.SetSamplerArray(inputIndex, samplers, arrayIndex);
            }

            if (!samplers.empty())
            {
                EnableResourceTypeCompilation(ResourceTypeMask::SamplerMask);
            }
            return isValidAll;
        }
        return false;
    }

    bool MultiDeviceShaderResourceGroupData::SetConstantRaw(ShaderInputConstantIndex inputIndex, const void* bytes, uint32_t byteCount)
    {
        return SetConstantRaw(inputIndex, bytes, 0, byteCount);
    }

    bool MultiDeviceShaderResourceGroupData::SetConstantRaw(
        ShaderInputConstantIndex inputIndex, const void* bytes, uint32_t byteOffset, uint32_t byteCount)
    {
        EnableResourceTypeCompilation(ResourceTypeMask::ConstantDataMask);

        bool isValidAll = true;

        for (auto& [deviceIndex, deviceShaderResourceGroupData] : m_deviceShaderResourceGroupDatas)
        {
            isValidAll &= deviceShaderResourceGroupData.SetConstantRaw(inputIndex, bytes, byteOffset, byteCount);
        }

        return isValidAll;
    }

    bool MultiDeviceShaderResourceGroupData::SetConstantData(const void* bytes, uint32_t byteCount)
    {
        EnableResourceTypeCompilation(ResourceTypeMask::ConstantDataMask);

        bool isValidAll = true;

        for (auto& [deviceIndex, deviceShaderResourceGroupData] : m_deviceShaderResourceGroupDatas)
        {
            isValidAll &= deviceShaderResourceGroupData.SetConstantData(bytes, byteCount);
        }

        return isValidAll;
    }

    bool MultiDeviceShaderResourceGroupData::SetConstantData(const void* bytes, uint32_t byteOffset, uint32_t byteCount)
    {
        EnableResourceTypeCompilation(ResourceTypeMask::ConstantDataMask);

        bool isValidAll = true;

        for (auto& [deviceIndex, deviceShaderResourceGroupData] : m_deviceShaderResourceGroupDatas)
        {
            isValidAll &= deviceShaderResourceGroupData.SetConstantData(bytes, byteOffset, byteCount);
        }

        return isValidAll;
    }

    uint32_t MultiDeviceShaderResourceGroupData::GetUpdateMask() const
    {
        return m_updateMask;
    }

    void MultiDeviceShaderResourceGroupData::EnableResourceTypeCompilation(ResourceTypeMask resourceTypeMask)
    {
        for (auto& [deviceIndex, deviceShaderResourceGroupData] : m_deviceShaderResourceGroupDatas)
        {
            deviceShaderResourceGroupData.EnableResourceTypeCompilation(resourceTypeMask);
        }

        m_updateMask = RHI::SetBits(m_updateMask, static_cast<uint32_t>(resourceTypeMask));
    }

    void MultiDeviceShaderResourceGroupData::ResetUpdateMask()
    {
        for (auto& [deviceIndex, deviceShaderResourceGroupData] : m_deviceShaderResourceGroupDatas)
        {
            deviceShaderResourceGroupData.ResetUpdateMask();
        }

        m_updateMask = 0;
    }

    void MultiDeviceShaderResourceGroupData::SetBindlessViews(
        ShaderInputBufferIndex indirectResourceBufferIndex,
        const MultiDeviceBufferView* indirectResourceBufferView,
        AZStd::span<const MultiDeviceImageView* const> imageViews,
        uint32_t* outIndices,
        AZStd::span<bool> isViewReadOnly,
        uint32_t arrayIndex)
    {
        for (auto& [deviceIndex, deviceShaderResourceGroupData] : m_deviceShaderResourceGroupDatas)
        {
            AZStd::vector<const ImageView*> deviceImageViews(imageViews.size());

            for (int imageIndex = 0; imageIndex < imageViews.size(); ++imageIndex)
            {
                auto [image, imageViewDescriptor]{ *imageViews[imageIndex] };
                deviceImageViews[imageIndex] = image->GetDeviceImage(deviceIndex)->GetImageView(imageViewDescriptor).get();
            }

            auto [buffer, bufferViewDescriptor]{ *indirectResourceBufferView };

            deviceShaderResourceGroupData.SetBindlessViews(
                indirectResourceBufferIndex,
                buffer->GetDeviceBuffer(deviceIndex)->GetBufferView(bufferViewDescriptor).get(),
                deviceImageViews,
                outIndices,
                isViewReadOnly,
                arrayIndex);
        }

        SetBufferView(indirectResourceBufferIndex, indirectResourceBufferView);
    }

    void MultiDeviceShaderResourceGroupData::SetBindlessViews(
        ShaderInputBufferIndex indirectResourceBufferIndex,
        const MultiDeviceBufferView* indirectResourceBufferView,
        AZStd::span<const MultiDeviceBufferView* const> bufferViews,
        uint32_t* outIndices,
        AZStd::span<bool> isViewReadOnly,
        uint32_t arrayIndex)
    {
        for (auto& [deviceIndex, deviceShaderResourceGroupData] : m_deviceShaderResourceGroupDatas)
        {
            AZStd::vector<const BufferView*> deviceBufferViews(bufferViews.size());

            for (int bufferIndex = 0; bufferIndex < bufferViews.size(); ++bufferIndex)
            {
                auto [buffer, bufferViewDescriptor]{ *bufferViews[bufferIndex] };
                deviceBufferViews[bufferIndex] = buffer->GetDeviceBuffer(deviceIndex)->GetBufferView(bufferViewDescriptor).get();
            }

            auto [buffer, bufferViewDescriptor]{ *indirectResourceBufferView };

            deviceShaderResourceGroupData.SetBindlessViews(
                indirectResourceBufferIndex,
                buffer->GetDeviceBuffer(deviceIndex)->GetBufferView(bufferViewDescriptor).get(),
                deviceBufferViews,
                outIndices,
                isViewReadOnly,
                arrayIndex);
        }

        SetBufferView(indirectResourceBufferIndex, indirectResourceBufferView);
    }
} // namespace AZ::RHI
