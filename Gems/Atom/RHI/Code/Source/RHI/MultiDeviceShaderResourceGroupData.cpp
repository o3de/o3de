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
    const ConstPtr<MultiDeviceImageView> MultiDeviceShaderResourceGroupData::s_nullImageView;
    const ConstPtr<MultiDeviceBufferView> MultiDeviceShaderResourceGroupData::s_nullBufferView;
    const SamplerState MultiDeviceShaderResourceGroupData::s_nullSamplerState{};

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
        : m_deviceMask(deviceMask)
        , m_shaderResourceGroupLayout(layout)
        , m_constantsData(layout->GetConstantsLayout())
    {
        m_imageViews.resize(layout->GetGroupSizeForImages());
        m_bufferViews.resize(layout->GetGroupSizeForBuffers());
        m_samplers.resize(layout->GetGroupSizeForSamplers());

        auto deviceCount{ RHI::RHISystemInterface::Get()->GetDeviceCount() };

        for (int deviceIndex = 0; deviceIndex < deviceCount; ++deviceIndex)
        {
            if (CheckBitsAll((AZStd::to_underlying(m_deviceMask) >> deviceIndex), 1u))
            {
                m_deviceShaderResourceGroupDatas[deviceIndex] = SingleDeviceShaderResourceGroupData(layout);
            }
        }
    }

    const ShaderResourceGroupLayout* MultiDeviceShaderResourceGroupData::GetLayout() const
    {
        return m_shaderResourceGroupLayout.get();
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
        ShaderInputImageIndex inputIndex, const MultiDeviceImageView* imageView, uint32_t arrayIndex)
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

            for (auto& [deviceIndex, deviceShaderResourceGroupData] : m_deviceShaderResourceGroupDatas)
            {
                AZStd::vector<const SingleDeviceImageView*> deviceImageViews(imageViews.size());

                for (int imageIndex = 0; imageIndex < imageViews.size(); ++imageIndex)
                {
                    deviceImageViews[imageIndex] = imageViews[imageIndex]->GetDeviceImageView(deviceIndex).get();
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

    bool MultiDeviceShaderResourceGroupData::SetImageViewUnboundedArray(
        ShaderInputImageUnboundedArrayIndex inputIndex, AZStd::span<const MultiDeviceImageView* const> imageViews)
    {
        if (GetLayout()->ValidateAccess(inputIndex))
        {
            m_imageViewsUnboundedArray.clear();
            bool isValidAll = true;

            for (auto& [deviceIndex, deviceShaderResourceGroupData] : m_deviceShaderResourceGroupDatas)
            {
                AZStd::vector<const SingleDeviceImageView*> deviceImageViews(imageViews.size());

                for (int imageIndex = 0; imageIndex < imageViews.size(); ++imageIndex)
                {
                    deviceImageViews[imageIndex] = imageViews[imageIndex]->GetDeviceImageView(deviceIndex).get();
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

            for (auto& [deviceIndex, deviceShaderResourceGroupData] : m_deviceShaderResourceGroupDatas)
            {
                AZStd::vector<const SingleDeviceBufferView*> deviceBufferViews(bufferViews.size());

                for (int bufferIndex = 0; bufferIndex < bufferViews.size(); ++bufferIndex)
                {
                    deviceBufferViews[bufferIndex] = bufferViews[bufferIndex]->GetDeviceBufferView(deviceIndex).get();
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

    bool MultiDeviceShaderResourceGroupData::SetBufferViewUnboundedArray(
        ShaderInputBufferUnboundedArrayIndex inputIndex, AZStd::span<const MultiDeviceBufferView* const> bufferViews)
    {
        if (GetLayout()->ValidateAccess(inputIndex))
        {
            m_bufferViewsUnboundedArray.clear();
            bool isValidAll = true;

            for (auto& [deviceIndex, deviceShaderResourceGroupData] : m_deviceShaderResourceGroupDatas)
            {
                AZStd::vector<const SingleDeviceBufferView*> deviceBufferViews(bufferViews.size());

                for (int bufferIndex = 0; bufferIndex < bufferViews.size(); ++bufferIndex)
                {
                    deviceBufferViews[bufferIndex] = bufferViews[bufferIndex]->GetDeviceBufferView(deviceIndex).get();
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

    bool MultiDeviceShaderResourceGroupData::SetConstantRaw(ShaderInputConstantIndex inputIndex, const void* bytes, uint32_t byteCount)
    {
        return SetConstantRaw(inputIndex, bytes, 0, byteCount);
    }

    bool MultiDeviceShaderResourceGroupData::SetConstantRaw(
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

    bool MultiDeviceShaderResourceGroupData::SetConstantData(const void* bytes, uint32_t byteCount)
    {
        EnableResourceTypeCompilation(ResourceTypeMask::ConstantDataMask);

        bool isValidAll = m_constantsData.SetConstantData(bytes, byteCount);

        for (auto& [deviceIndex, deviceShaderResourceGroupData] : m_deviceShaderResourceGroupDatas)
        {
            isValidAll &= deviceShaderResourceGroupData.SetConstantData(bytes, byteCount);
        }

        return isValidAll;
    }

    bool MultiDeviceShaderResourceGroupData::SetConstantData(const void* bytes, uint32_t byteOffset, uint32_t byteCount)
    {
        EnableResourceTypeCompilation(ResourceTypeMask::ConstantDataMask);

        bool isValidAll = m_constantsData.SetConstantData(bytes, byteOffset, byteCount);

        for (auto& [deviceIndex, deviceShaderResourceGroupData] : m_deviceShaderResourceGroupDatas)
        {
            isValidAll &= deviceShaderResourceGroupData.SetConstantData(bytes, byteOffset, byteCount);
        }

        return isValidAll;
    }

    const RHI::ConstPtr<RHI::MultiDeviceImageView>& MultiDeviceShaderResourceGroupData::GetImageView(RHI::ShaderInputImageIndex inputIndex, uint32_t arrayIndex) const
    {
        if (GetLayout()->ValidateAccess(inputIndex, arrayIndex))
        {
            const Interval interval = GetLayout()->GetGroupInterval(inputIndex);
            return m_imageViews[interval.m_min + arrayIndex];
        }
        return s_nullImageView;
    }

    AZStd::span<const RHI::ConstPtr<RHI::MultiDeviceImageView>> MultiDeviceShaderResourceGroupData::GetImageViewArray(RHI::ShaderInputImageIndex inputIndex) const
    {
        if (GetLayout()->ValidateAccess(inputIndex, 0))
        {
            const Interval interval = GetLayout()->GetGroupInterval(inputIndex);
            return AZStd::span<const RHI::ConstPtr<RHI::MultiDeviceImageView>>(&m_imageViews[interval.m_min], interval.m_max - interval.m_min);
        }
        return {};
    }

    AZStd::span<const RHI::ConstPtr<RHI::MultiDeviceImageView>> MultiDeviceShaderResourceGroupData::GetImageViewUnboundedArray(RHI::ShaderInputImageUnboundedArrayIndex inputIndex) const
    {
        if (GetLayout()->ValidateAccess(inputIndex))
        {
            return AZStd::span<const RHI::ConstPtr<RHI::MultiDeviceImageView>>(m_imageViewsUnboundedArray.data(), m_imageViewsUnboundedArray.size());
        }
        return {};
    }

    const RHI::ConstPtr<RHI::MultiDeviceBufferView>& MultiDeviceShaderResourceGroupData::GetBufferView(RHI::ShaderInputBufferIndex inputIndex, uint32_t arrayIndex) const
    {
        if (GetLayout()->ValidateAccess(inputIndex, arrayIndex))
        {
            const Interval interval = GetLayout()->GetGroupInterval(inputIndex);
            return m_bufferViews[interval.m_min + arrayIndex];
        }
        return s_nullBufferView;
    }

    AZStd::span<const RHI::ConstPtr<RHI::MultiDeviceBufferView>> MultiDeviceShaderResourceGroupData::GetBufferViewArray(RHI::ShaderInputBufferIndex inputIndex) const
    {
        if (GetLayout()->ValidateAccess(inputIndex, 0))
        {
            const Interval interval = GetLayout()->GetGroupInterval(inputIndex);
            return AZStd::span<const RHI::ConstPtr<RHI::MultiDeviceBufferView>>(&m_bufferViews[interval.m_min], interval.m_max - interval.m_min);
        }
        return {};
    }

    AZStd::span<const RHI::ConstPtr<RHI::MultiDeviceBufferView>> MultiDeviceShaderResourceGroupData::GetBufferViewUnboundedArray(RHI::ShaderInputBufferUnboundedArrayIndex inputIndex) const
    {
        if (GetLayout()->ValidateAccess(inputIndex))
        {
            return AZStd::span<const RHI::ConstPtr<RHI::MultiDeviceBufferView>>(m_bufferViewsUnboundedArray.data(), m_bufferViewsUnboundedArray.size());
        }
        return {};
    }

    const RHI::SamplerState& MultiDeviceShaderResourceGroupData::GetSampler(RHI::ShaderInputSamplerIndex inputIndex, uint32_t arrayIndex) const
    {
        if (GetLayout()->ValidateAccess(inputIndex, arrayIndex))
        {
            const Interval interval = GetLayout()->GetGroupInterval(inputIndex);
            return m_samplers[interval.m_min + arrayIndex];
        }
        return s_nullSamplerState;
    }

    AZStd::span<const RHI::SamplerState> MultiDeviceShaderResourceGroupData::GetSamplerArray(RHI::ShaderInputSamplerIndex inputIndex) const
    {
        const Interval interval = GetLayout()->GetGroupInterval(inputIndex);
        return AZStd::span<const RHI::SamplerState>(&m_samplers[interval.m_min], interval.m_max - interval.m_min);
    }

    AZStd::span<const uint8_t> MultiDeviceShaderResourceGroupData::GetConstantRaw(ShaderInputConstantIndex inputIndex) const
    {
        return m_constantsData.GetConstantRaw(inputIndex);
    }

    AZStd::span<const ConstPtr<MultiDeviceImageView>> MultiDeviceShaderResourceGroupData::GetImageGroup() const
    {
        return m_imageViews;
    }

    AZStd::span<const ConstPtr<MultiDeviceBufferView>> MultiDeviceShaderResourceGroupData::GetBufferGroup() const
    {
        return m_bufferViews;
    }

    AZStd::span<const SamplerState> MultiDeviceShaderResourceGroupData::GetSamplerGroup() const
    {
        return m_samplers;
    }

    uint32_t MultiDeviceShaderResourceGroupData::GetUpdateMask() const
    {
        return m_updateMask;
    }

    void MultiDeviceShaderResourceGroupData::EnableResourceTypeCompilation(ResourceTypeMask resourceTypeMask)
    {
        m_updateMask = RHI::SetBits(m_updateMask, static_cast<uint32_t>(resourceTypeMask));

        for (auto& [deviceIndex, deviceShaderResourceGroupData] : m_deviceShaderResourceGroupDatas)
        {
            deviceShaderResourceGroupData.EnableResourceTypeCompilation(resourceTypeMask);
        }
    }

    void MultiDeviceShaderResourceGroupData::ResetUpdateMask()
    {
        m_updateMask = 0;

        for (auto& [deviceIndex, deviceShaderResourceGroupData] : m_deviceShaderResourceGroupDatas)
        {
            deviceShaderResourceGroupData.ResetUpdateMask();
        }
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
            AZStd::vector<const SingleDeviceImageView*> deviceImageViews(imageViews.size());

            for (int imageIndex = 0; imageIndex < imageViews.size(); ++imageIndex)
            {
                deviceImageViews[imageIndex] = imageViews[imageIndex]->GetDeviceImageView(deviceIndex).get();
            }

            deviceShaderResourceGroupData.SetBindlessViews(
                indirectResourceBufferIndex,
                indirectResourceBufferView->GetDeviceBufferView(deviceIndex).get(),
                deviceImageViews,
                outIndices,
                isViewReadOnly,
                arrayIndex);
        }

        BufferPoolDescriptor desc = static_cast<const MultiDeviceBufferPool*>(indirectResourceBufferView->GetBuffer()->GetPool())->GetDescriptor();
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
        for (const MultiDeviceImageView* imageView : imageViews)
        {
            it->second.m_bindlessResources.push_back(imageView);
            it->second.m_bindlessResourceType = isViewReadOnly[i] ? BindlessResourceType::m_Texture2D : BindlessResourceType::m_RWTexture2D;
            ++i;
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
            AZStd::vector<const SingleDeviceBufferView*> deviceBufferViews(bufferViews.size());

            for (int bufferIndex = 0; bufferIndex < bufferViews.size(); ++bufferIndex)
            {
                deviceBufferViews[bufferIndex] = bufferViews[bufferIndex]->GetDeviceBufferView(deviceIndex).get();
            }

            deviceShaderResourceGroupData.SetBindlessViews(
                indirectResourceBufferIndex,
                indirectResourceBufferView->GetDeviceBufferView(deviceIndex).get(),
                deviceBufferViews,
                outIndices,
                isViewReadOnly,
                arrayIndex);
        }

        BufferPoolDescriptor desc = static_cast<const MultiDeviceBufferPool*>(indirectResourceBufferView->GetBuffer()->GetPool())->GetDescriptor();
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
        for (const MultiDeviceBufferView* bufferView : bufferViews)
        {
            it->second.m_bindlessResources.push_back(bufferView);
            it->second.m_bindlessResourceType = isViewReadOnly[i] ? BindlessResourceType::m_ByteAddressBuffer : BindlessResourceType::m_RWByteAddressBuffer;
            ++i;
        }

        SetBufferView(indirectResourceBufferIndex, indirectResourceBufferView);
    }

    const uint32_t MultiDeviceShaderResourceGroupData::GetBindlessViewsSize() const
    {
        return aznumeric_cast<uint32_t>(m_bindlessResourceViews.size());
    }

    const AZStd::unordered_map<AZStd::pair<ShaderInputBufferIndex, uint32_t>, MultiDeviceShaderResourceGroupData::MultiDeviceBindlessResourceViews>&
    MultiDeviceShaderResourceGroupData::GetBindlessResourceViews() const
    {
        return m_bindlessResourceViews;
    }
} // namespace AZ::RHI
