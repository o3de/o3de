/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI.Reflect/Bits.h>
#include <Atom/RHI/BufferPool.h>
#include <Atom/RHI/ShaderResourceGroupData.h>
#include <Atom/RHI/ShaderResourceGroupPool.h>
#include <Atom/RHI/RHISystemInterface.h>

namespace AZ::RHI
{
    const ConstPtr<ImageView> ShaderResourceGroupData::s_nullImageView;
    const ConstPtr<BufferView> ShaderResourceGroupData::s_nullBufferView;
    const SamplerState ShaderResourceGroupData::s_nullSamplerState{};

    ShaderResourceGroupData::ShaderResourceGroupData(const ShaderResourceGroup& shaderResourceGroup)
        : ShaderResourceGroupData(*shaderResourceGroup.GetPool())
    {
    }

    ShaderResourceGroupData::ShaderResourceGroupData(
        const ShaderResourceGroupPool& shaderResourceGroupPool)
        : ShaderResourceGroupData(shaderResourceGroupPool.GetDeviceMask(), shaderResourceGroupPool.GetLayout())
    {
    }

    ShaderResourceGroupData::ShaderResourceGroupData(
        MultiDevice::DeviceMask deviceMask, const ShaderResourceGroupLayout* layout)
        : m_deviceMask(deviceMask)
        , m_shaderResourceGroupLayout(layout)
        , m_constantsData(layout->GetConstantsLayout())
    {
        m_imageViews.resize(layout->GetGroupSizeForImages());
        m_bufferViews.resize(layout->GetGroupSizeForBuffers());
        m_samplers.resize(layout->GetGroupSizeForSamplers());

        MultiDeviceObject::IterateDevices(
            m_deviceMask,
            [this, layout](int deviceIndex)
            {
                m_deviceShaderResourceGroupDatas[deviceIndex] = DeviceShaderResourceGroupData(layout);
                return true;
            });
    }

    void ShaderResourceGroupData::ResetViews()
    {
        for (auto& [deviceIndex, deviceShaderResourceGroupData] : m_deviceShaderResourceGroupDatas)
        {
            deviceShaderResourceGroupData.ResetViews();
        }
    }

    const ShaderResourceGroupLayout* ShaderResourceGroupData::GetLayout() const
    {
        return m_shaderResourceGroupLayout.get();
    }

    ShaderInputBufferIndex ShaderResourceGroupData::FindShaderInputBufferIndex(const Name& name) const
    {
        return m_shaderResourceGroupLayout->FindShaderInputBufferIndex(name);
    }

    ShaderInputImageIndex ShaderResourceGroupData::FindShaderInputImageIndex(const Name& name) const
    {
        return m_shaderResourceGroupLayout->FindShaderInputImageIndex(name);
    }

    ShaderInputSamplerIndex ShaderResourceGroupData::FindShaderInputSamplerIndex(const Name& name) const
    {
        return m_shaderResourceGroupLayout->FindShaderInputSamplerIndex(name);
    }

    ShaderInputConstantIndex ShaderResourceGroupData::FindShaderInputConstantIndex(const Name& name) const
    {
        return m_shaderResourceGroupLayout->FindShaderInputConstantIndex(name);
    }

    bool ShaderResourceGroupData::SetImageView(
        ShaderInputImageIndex inputIndex, const ImageView* imageView, uint32_t arrayIndex)
    {
        AZStd::array<const ImageView* const, 1> imageViews = { { imageView } };
        return SetImageViewArray(inputIndex, imageViews, arrayIndex);
    }

    bool ShaderResourceGroupData::SetImageViewArray(
        ShaderInputImageIndex inputIndex, AZStd::span<const ImageView* const> imageViews, uint32_t arrayIndex)
    {
        if (GetLayout()->ValidateAccess(inputIndex, static_cast<uint32_t>(arrayIndex + imageViews.size() - 1)))
        {
            bool isValidAll = true;

            for (auto& [deviceIndex, deviceShaderResourceGroupData] : m_deviceShaderResourceGroupDatas)
            {
                AZStd::vector<const DeviceImageView*> deviceImageViews(imageViews.size());

                for (int imageIndex = 0; imageIndex < imageViews.size(); ++imageIndex)
                {
                    deviceImageViews[imageIndex] = imageViews[imageIndex]
                        ? (imageViews[imageIndex]->GetImage()->IsDeviceSet(deviceIndex)
                               ? imageViews[imageIndex]->GetDeviceImageView(deviceIndex).get()
                               : nullptr)
                        : nullptr;
                }

                isValidAll &= deviceShaderResourceGroupData.SetImageViewArray(inputIndex, deviceImageViews, arrayIndex);
            }

            if (!imageViews.empty())
            {
                EnableResourceTypeCompilation(ResourceTypeMask::ImageViewMask);
            }

            if (isValidAll)
            {
                const Interval interval = GetLayout()->GetGroupInterval(inputIndex);
                for (int imageIndex = 0; imageIndex < imageViews.size(); ++imageIndex)
                {
                    m_imageViews[interval.m_min + arrayIndex + imageIndex] = imageViews[imageIndex];
                }
            }

            return isValidAll;
        }
        return false;
    }

    bool ShaderResourceGroupData::SetImageViewUnboundedArray(
        ShaderInputImageUnboundedArrayIndex inputIndex, AZStd::span<const ImageView* const> imageViews)
    {
        if (GetLayout()->ValidateAccess(inputIndex))
        {
            m_imageViewsUnboundedArray.clear();
            bool isValidAll = true;

            for (auto& [deviceIndex, deviceShaderResourceGroupData] : m_deviceShaderResourceGroupDatas)
            {
                AZStd::vector<const DeviceImageView*> deviceImageViews(imageViews.size());

                for (int imageIndex = 0; imageIndex < imageViews.size(); ++imageIndex)
                {
                    deviceImageViews[imageIndex] = imageViews[imageIndex]
                        ? (imageViews[imageIndex]->GetImage()->IsDeviceSet(deviceIndex)
                               ? imageViews[imageIndex]->GetDeviceImageView(deviceIndex).get()
                               : nullptr)
                        : nullptr;
                }

                isValidAll &= deviceShaderResourceGroupData.SetImageViewUnboundedArray(inputIndex, deviceImageViews);
            }

            if (!imageViews.empty())
            {
                EnableResourceTypeCompilation(ResourceTypeMask::ImageViewUnboundedArrayMask);
            }

            if (isValidAll)
            {
                for (int imageIndex = 0; imageIndex < imageViews.size(); ++imageIndex)
                {
                    m_imageViewsUnboundedArray.push_back(imageViews[imageIndex]);
                }
            }

            return isValidAll;
        }
        return false;
    }

    bool ShaderResourceGroupData::SetBufferView(
        ShaderInputBufferIndex inputIndex, const BufferView* bufferView, uint32_t arrayIndex)
    {
        AZStd::array<const BufferView* const, 1> bufferViews = { { bufferView } };
        return SetBufferViewArray(inputIndex, bufferViews, arrayIndex);
    }

    bool ShaderResourceGroupData::SetBufferViewArray(
        ShaderInputBufferIndex inputIndex, AZStd::span<const BufferView* const> bufferViews, uint32_t arrayIndex)
    {
        if (GetLayout()->ValidateAccess(inputIndex, static_cast<uint32_t>(arrayIndex + bufferViews.size() - 1)))
        {
            bool isValidAll = true;

            for (auto& [deviceIndex, deviceShaderResourceGroupData] : m_deviceShaderResourceGroupDatas)
            {
                AZStd::vector<const DeviceBufferView*> deviceBufferViews(bufferViews.size());

                for (int bufferIndex = 0; bufferIndex < bufferViews.size(); ++bufferIndex)
                {
                    deviceBufferViews[bufferIndex] = bufferViews[bufferIndex]
                        ? (bufferViews[bufferIndex]->GetBuffer()->IsDeviceSet(deviceIndex)
                               ? bufferViews[bufferIndex]->GetDeviceBufferView(deviceIndex).get()
                               : nullptr)
                        : nullptr;
                }

                isValidAll &= deviceShaderResourceGroupData.SetBufferViewArray(inputIndex, deviceBufferViews, arrayIndex);
            }

            if (!bufferViews.empty())
            {
                EnableResourceTypeCompilation(ResourceTypeMask::BufferViewMask);
            }

            if (isValidAll)
            {
                const Interval interval = GetLayout()->GetGroupInterval(inputIndex);
                for (int bufferIndex = 0; bufferIndex < bufferViews.size(); ++bufferIndex)
                {
                    m_bufferViews[interval.m_min + arrayIndex + bufferIndex] = bufferViews[bufferIndex];
                }
            }

            return isValidAll;
        }
        return false;
    }

    bool ShaderResourceGroupData::SetBufferViewUnboundedArray(
        ShaderInputBufferUnboundedArrayIndex inputIndex, AZStd::span<const BufferView* const> bufferViews)
    {
        if (GetLayout()->ValidateAccess(inputIndex))
        {
            m_bufferViewsUnboundedArray.clear();
            bool isValidAll = true;

            for (auto& [deviceIndex, deviceShaderResourceGroupData] : m_deviceShaderResourceGroupDatas)
            {
                AZStd::vector<const DeviceBufferView*> deviceBufferViews(bufferViews.size());

                for (int bufferIndex = 0; bufferIndex < bufferViews.size(); ++bufferIndex)
                {
                    deviceBufferViews[bufferIndex] = bufferViews[bufferIndex]
                        ? (bufferViews[bufferIndex]->GetBuffer()->IsDeviceSet(deviceIndex)
                               ? bufferViews[bufferIndex]->GetDeviceBufferView(deviceIndex).get()
                               : nullptr)
                        : nullptr;
                }

                isValidAll &= deviceShaderResourceGroupData.SetBufferViewUnboundedArray(inputIndex, deviceBufferViews);
            }

            if (!bufferViews.empty())
            {
                EnableResourceTypeCompilation(ResourceTypeMask::BufferViewUnboundedArrayMask);
            }

            if (isValidAll)
            {
                for (int bufferIndex = 0; bufferIndex < bufferViews.size(); ++bufferIndex)
                {
                    m_bufferViewsUnboundedArray.push_back(bufferViews[bufferIndex]);
                }
            }

            return isValidAll;
        }
        return false;
    }

    bool ShaderResourceGroupData::SetSampler(
        ShaderInputSamplerIndex inputIndex, const SamplerState& sampler, uint32_t arrayIndex)
    {
        return SetSamplerArray(inputIndex, AZStd::span<const SamplerState>(&sampler, 1), arrayIndex);
    }

    bool ShaderResourceGroupData::SetSamplerArray(
        ShaderInputSamplerIndex inputIndex, AZStd::span<const SamplerState> samplers, uint32_t arrayIndex)
    {
        if (GetLayout()->ValidateAccess(inputIndex, static_cast<uint32_t>(arrayIndex + samplers.size() - 1)))
        {
            bool isValidAll = true;

            for (auto& [deviceIndex, deviceShaderResourceGroupData] : m_deviceShaderResourceGroupDatas)
            {
                isValidAll &= deviceShaderResourceGroupData.SetSamplerArray(inputIndex, samplers, arrayIndex);
            }

            const Interval interval = GetLayout()->GetGroupInterval(inputIndex);
            for (size_t i = 0; i < samplers.size(); ++i)
            {
                m_samplers[interval.m_min + arrayIndex + i] = samplers[i];
            }

            if (!samplers.empty())
            {
                EnableResourceTypeCompilation(ResourceTypeMask::SamplerMask);
            }
            return isValidAll;
        }
        return false;
    }

    bool ShaderResourceGroupData::SetConstantRaw(ShaderInputConstantIndex inputIndex, const void* bytes, uint32_t byteCount)
    {
        return SetConstantRaw(inputIndex, bytes, 0, byteCount);
    }

    bool ShaderResourceGroupData::SetConstantRaw(
        ShaderInputConstantIndex inputIndex, const void* bytes, uint32_t byteOffset, uint32_t byteCount)
    {
        EnableResourceTypeCompilation(ResourceTypeMask::ConstantDataMask);

        bool isValidAll = m_constantsData.SetConstantRaw(inputIndex, bytes, byteOffset, byteCount);;

        for (auto& [deviceIndex, deviceShaderResourceGroupData] : m_deviceShaderResourceGroupDatas)
        {
            isValidAll &= deviceShaderResourceGroupData.SetConstantRaw(inputIndex, bytes, byteOffset, byteCount);
        }

        return isValidAll;
    }

    bool ShaderResourceGroupData::SetConstantData(const void* bytes, uint32_t byteCount)
    {
        EnableResourceTypeCompilation(ResourceTypeMask::ConstantDataMask);

        bool isValidAll = m_constantsData.SetConstantData(bytes, byteCount);

        for (auto& [deviceIndex, deviceShaderResourceGroupData] : m_deviceShaderResourceGroupDatas)
        {
            isValidAll &= deviceShaderResourceGroupData.SetConstantData(bytes, byteCount);
        }

        return isValidAll;
    }

    bool ShaderResourceGroupData::SetConstantData(const void* bytes, uint32_t byteOffset, uint32_t byteCount)
    {
        EnableResourceTypeCompilation(ResourceTypeMask::ConstantDataMask);

        bool isValidAll = m_constantsData.SetConstantData(bytes, byteOffset, byteCount);

        for (auto& [deviceIndex, deviceShaderResourceGroupData] : m_deviceShaderResourceGroupDatas)
        {
            isValidAll &= deviceShaderResourceGroupData.SetConstantData(bytes, byteOffset, byteCount);
        }

        return isValidAll;
    }

    const RHI::ConstPtr<RHI::ImageView>& ShaderResourceGroupData::GetImageView(RHI::ShaderInputImageIndex inputIndex, uint32_t arrayIndex) const
    {
        if (GetLayout()->ValidateAccess(inputIndex, arrayIndex))
        {
            const Interval interval = GetLayout()->GetGroupInterval(inputIndex);
            return m_imageViews[interval.m_min + arrayIndex];
        }
        return s_nullImageView;
    }

    AZStd::span<const RHI::ConstPtr<RHI::ImageView>> ShaderResourceGroupData::GetImageViewArray(RHI::ShaderInputImageIndex inputIndex) const
    {
        if (GetLayout()->ValidateAccess(inputIndex, 0))
        {
            const Interval interval = GetLayout()->GetGroupInterval(inputIndex);
            return AZStd::span<const RHI::ConstPtr<RHI::ImageView>>(&m_imageViews[interval.m_min], interval.m_max - interval.m_min);
        }
        return {};
    }

    AZStd::span<const RHI::ConstPtr<RHI::ImageView>> ShaderResourceGroupData::GetImageViewUnboundedArray(RHI::ShaderInputImageUnboundedArrayIndex inputIndex) const
    {
        if (GetLayout()->ValidateAccess(inputIndex))
        {
            return AZStd::span<const RHI::ConstPtr<RHI::ImageView>>(m_imageViewsUnboundedArray.data(), m_imageViewsUnboundedArray.size());
        }
        return {};
    }

    const RHI::ConstPtr<RHI::BufferView>& ShaderResourceGroupData::GetBufferView(RHI::ShaderInputBufferIndex inputIndex, uint32_t arrayIndex) const
    {
        if (GetLayout()->ValidateAccess(inputIndex, arrayIndex))
        {
            const Interval interval = GetLayout()->GetGroupInterval(inputIndex);
            return m_bufferViews[interval.m_min + arrayIndex];
        }
        return s_nullBufferView;
    }

    AZStd::span<const RHI::ConstPtr<RHI::BufferView>> ShaderResourceGroupData::GetBufferViewArray(RHI::ShaderInputBufferIndex inputIndex) const
    {
        if (GetLayout()->ValidateAccess(inputIndex, 0))
        {
            const Interval interval = GetLayout()->GetGroupInterval(inputIndex);
            return AZStd::span<const RHI::ConstPtr<RHI::BufferView>>(&m_bufferViews[interval.m_min], interval.m_max - interval.m_min);
        }
        return {};
    }

    AZStd::span<const RHI::ConstPtr<RHI::BufferView>> ShaderResourceGroupData::GetBufferViewUnboundedArray(RHI::ShaderInputBufferUnboundedArrayIndex inputIndex) const
    {
        if (GetLayout()->ValidateAccess(inputIndex))
        {
            return AZStd::span<const RHI::ConstPtr<RHI::BufferView>>(m_bufferViewsUnboundedArray.data(), m_bufferViewsUnboundedArray.size());
        }
        return {};
    }

    const RHI::SamplerState& ShaderResourceGroupData::GetSampler(RHI::ShaderInputSamplerIndex inputIndex, uint32_t arrayIndex) const
    {
        if (GetLayout()->ValidateAccess(inputIndex, arrayIndex))
        {
            const Interval interval = GetLayout()->GetGroupInterval(inputIndex);
            return m_samplers[interval.m_min + arrayIndex];
        }
        return s_nullSamplerState;
    }

    AZStd::span<const RHI::SamplerState> ShaderResourceGroupData::GetSamplerArray(RHI::ShaderInputSamplerIndex inputIndex) const
    {
        const Interval interval = GetLayout()->GetGroupInterval(inputIndex);
        return AZStd::span<const RHI::SamplerState>(&m_samplers[interval.m_min], interval.m_max - interval.m_min);
    }

    AZStd::span<const uint8_t> ShaderResourceGroupData::GetConstantRaw(ShaderInputConstantIndex inputIndex) const
    {
        return m_constantsData.GetConstantRaw(inputIndex);
    }

    AZStd::span<const ConstPtr<ImageView>> ShaderResourceGroupData::GetImageGroup() const
    {
        return m_imageViews;
    }

    AZStd::span<const ConstPtr<BufferView>> ShaderResourceGroupData::GetBufferGroup() const
    {
        return m_bufferViews;
    }

    AZStd::span<const SamplerState> ShaderResourceGroupData::GetSamplerGroup() const
    {
        return m_samplers;
    }

    void ShaderResourceGroupData::EnableResourceTypeCompilation(ResourceTypeMask resourceTypeMask)
    {
        for (auto& [deviceIndex, deviceShaderResourceGroupData] : m_deviceShaderResourceGroupDatas)
        {
            deviceShaderResourceGroupData.EnableResourceTypeCompilation(resourceTypeMask);
        }
    }

    void ShaderResourceGroupData::ResetUpdateMask()
    {
        for (auto& [deviceIndex, deviceShaderResourceGroupData] : m_deviceShaderResourceGroupDatas)
        {
            deviceShaderResourceGroupData.ResetUpdateMask();
        }
    }

    void ShaderResourceGroupData::SetBindlessViews(
        ShaderInputBufferIndex indirectResourceBufferIndex,
        const BufferView* indirectResourceBufferView,
        AZStd::span<const ImageView* const> imageViews,
        AZStd::unordered_map<int, uint32_t*> outIndices,
        AZStd::span<bool> isViewReadOnly,
        uint32_t arrayIndex)
    {
        for (auto& [deviceIndex, deviceShaderResourceGroupData] : m_deviceShaderResourceGroupDatas)
        {
            AZStd::vector<const DeviceImageView*> deviceImageViews(imageViews.size());

            for (int imageIndex = 0; imageIndex < imageViews.size(); ++imageIndex)
            {
                deviceImageViews[imageIndex] = imageViews[imageIndex] ? imageViews[imageIndex]->GetDeviceImageView(deviceIndex).get() : nullptr;
            }

            auto outIndicesIt = outIndices.find(deviceIndex);

            deviceShaderResourceGroupData.SetBindlessViews(
                indirectResourceBufferIndex,
                indirectResourceBufferView->GetDeviceBufferView(deviceIndex).get(),
                deviceImageViews,
                outIndicesIt != outIndices.end() ? outIndicesIt->second : nullptr,
                isViewReadOnly,
                arrayIndex);
        }

        BufferPoolDescriptor desc = static_cast<const BufferPool*>(indirectResourceBufferView->GetBuffer()->GetPool())->GetDescriptor();
        AZ_Assert(desc.m_heapMemoryLevel == HeapMemoryLevel::Device, "Indirect buffer that contains indices to the bindless resource views should be device as that is protected against triple buffering.");

        auto key = AZStd::make_pair(indirectResourceBufferIndex, arrayIndex);
        auto it = m_bindlessResourceViews.find(key);
        if (it == m_bindlessResourceViews.end())
        {
            it = m_bindlessResourceViews.try_emplace(key).first;
        }
        else
        {
            // Release existing views
            it->second.m_bindlessResources.clear();
        }

        AZ_Assert(imageViews.size() == isViewReadOnly.size(), "Mismatch sizes. For each view we need to know if it is read only or readwrite");
        size_t i = 0;
        for (const ImageView* imageView : imageViews)
        {
            it->second.m_bindlessResources.push_back(imageView);
            it->second.m_bindlessResourceType = isViewReadOnly[i] ? BindlessResourceType::m_Texture2D : BindlessResourceType::m_RWTexture2D;
            ++i;
        }

        SetBufferView(indirectResourceBufferIndex, indirectResourceBufferView);
    }

    void ShaderResourceGroupData::SetBindlessViews(
        ShaderInputBufferIndex indirectResourceBufferIndex,
        const BufferView* indirectResourceBufferView,
        AZStd::span<const BufferView* const> bufferViews,
        AZStd::unordered_map<int, uint32_t*> outIndices,
        AZStd::span<bool> isViewReadOnly,
        uint32_t arrayIndex)
    {
        for (auto& [deviceIndex, deviceShaderResourceGroupData] : m_deviceShaderResourceGroupDatas)
        {
            AZStd::vector<const DeviceBufferView*> deviceBufferViews(bufferViews.size());

            for (int bufferIndex = 0; bufferIndex < bufferViews.size(); ++bufferIndex)
            {
                deviceBufferViews[bufferIndex] = bufferViews[bufferIndex] ? bufferViews[bufferIndex]->GetDeviceBufferView(deviceIndex).get() : nullptr;
            }

            auto outIndicesIt = outIndices.find(deviceIndex);

            deviceShaderResourceGroupData.SetBindlessViews(
                indirectResourceBufferIndex,
                indirectResourceBufferView->GetDeviceBufferView(deviceIndex).get(),
                deviceBufferViews,
                outIndicesIt != outIndices.end() ? outIndicesIt->second : nullptr,
                isViewReadOnly,
                arrayIndex);
        }

        BufferPoolDescriptor desc = static_cast<const BufferPool*>(indirectResourceBufferView->GetBuffer()->GetPool())->GetDescriptor();
        AZ_Assert(desc.m_heapMemoryLevel == HeapMemoryLevel::Device, "Indirect buffer that contains indices to the bindless resource views should be device as that is protected against triple buffering.");

        auto key = AZStd::make_pair(indirectResourceBufferIndex, arrayIndex);
        auto it = m_bindlessResourceViews.find(key);
        if (it == m_bindlessResourceViews.end())
        {
            it = m_bindlessResourceViews.try_emplace(key).first;
        }
        else
        {
            // Release existing views
            it->second.m_bindlessResources.clear();
        }

        AZ_Assert(bufferViews.size() == isViewReadOnly.size(), "Mismatch sizes. For each view we need to know if it is read only or readwrite");

        size_t i = 0;
        for (const BufferView* bufferView : bufferViews)
        {
            it->second.m_bindlessResources.push_back(bufferView);
            it->second.m_bindlessResourceType = isViewReadOnly[i] ? BindlessResourceType::m_ByteAddressBuffer : BindlessResourceType::m_RWByteAddressBuffer;
            ++i;
        }

        SetBufferView(indirectResourceBufferIndex, indirectResourceBufferView);
    }

    const uint32_t ShaderResourceGroupData::GetBindlessViewsSize() const
    {
        return aznumeric_cast<uint32_t>(m_bindlessResourceViews.size());
    }

    const AZStd::unordered_map<AZStd::pair<ShaderInputBufferIndex, uint32_t>, ShaderResourceGroupData::BindlessResourceViews>&
    ShaderResourceGroupData::GetBindlessResourceViews() const
    {
        return m_bindlessResourceViews;
    }
} // namespace AZ::RHI
