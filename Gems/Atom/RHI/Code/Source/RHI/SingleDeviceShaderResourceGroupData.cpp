/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/SingleDeviceShaderResourceGroupData.h>
#include <Atom/RHI/SingleDeviceShaderResourceGroupPool.h>
#include <Atom/RHI.Reflect/Bits.h>
#include <Atom/RHI/SingleDeviceBufferPool.h>

namespace AZ::RHI
{
    const ConstPtr<ImageView> SingleDeviceShaderResourceGroupData::s_nullImageView;
    const ConstPtr<BufferView> SingleDeviceShaderResourceGroupData::s_nullBufferView;
    const SamplerState SingleDeviceShaderResourceGroupData::s_nullSamplerState{};

    SingleDeviceShaderResourceGroupData::SingleDeviceShaderResourceGroupData() = default;
    SingleDeviceShaderResourceGroupData::~SingleDeviceShaderResourceGroupData() = default;

    SingleDeviceShaderResourceGroupData::SingleDeviceShaderResourceGroupData(const SingleDeviceShaderResourceGroup& shaderResourceGroup)
        : SingleDeviceShaderResourceGroupData(*shaderResourceGroup.GetPool())
    {}

    SingleDeviceShaderResourceGroupData::SingleDeviceShaderResourceGroupData(const SingleDeviceShaderResourceGroupPool& shaderResourceGroupPool)
        : SingleDeviceShaderResourceGroupData(shaderResourceGroupPool.GetLayout())
    {}

    SingleDeviceShaderResourceGroupData::SingleDeviceShaderResourceGroupData(const ShaderResourceGroupLayout* layout)
        : m_shaderResourceGroupLayout(layout)
        , m_constantsData(layout->GetConstantsLayout())
    {
        m_imageViews.resize(layout->GetGroupSizeForImages());
        m_bufferViews.resize(layout->GetGroupSizeForBuffers());
        m_samplers.resize(layout->GetGroupSizeForSamplers());
    }

    const ShaderResourceGroupLayout* SingleDeviceShaderResourceGroupData::GetLayout() const
    {
        return m_shaderResourceGroupLayout.get();
    }

    bool SingleDeviceShaderResourceGroupData::ValidateSetImageView(ShaderInputImageIndex inputIndex, const ImageView* imageView, uint32_t arrayIndex) const
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

    bool SingleDeviceShaderResourceGroupData::ValidateSetBufferView(ShaderInputBufferIndex inputIndex, const BufferView* bufferView, uint32_t arrayIndex) const
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

    ShaderInputBufferIndex SingleDeviceShaderResourceGroupData::FindShaderInputBufferIndex(const Name& name) const
    {
        return m_shaderResourceGroupLayout->FindShaderInputBufferIndex(name);
    }

    ShaderInputImageIndex SingleDeviceShaderResourceGroupData::FindShaderInputImageIndex(const Name& name) const
    {
        return m_shaderResourceGroupLayout->FindShaderInputImageIndex(name);
    }

    ShaderInputSamplerIndex SingleDeviceShaderResourceGroupData::FindShaderInputSamplerIndex(const Name& name) const
    {
        return m_shaderResourceGroupLayout->FindShaderInputSamplerIndex(name);
    }

    ShaderInputConstantIndex SingleDeviceShaderResourceGroupData::FindShaderInputConstantIndex(const Name& name) const
    {
        return m_shaderResourceGroupLayout->FindShaderInputConstantIndex(name);
    }

    bool SingleDeviceShaderResourceGroupData::SetImageView(ShaderInputImageIndex inputIndex, const ImageView* imageView, uint32_t arrayIndex = 0)
    {
        AZStd::array<const ImageView*, 1> imageViews = {{imageView}};
        return SetImageViewArray(inputIndex, imageViews, arrayIndex);
    }

    bool SingleDeviceShaderResourceGroupData::SetImageViewArray(ShaderInputImageIndex inputIndex, AZStd::span<const ImageView* const> imageViews, uint32_t arrayIndex)
    {
        if (GetLayout()->ValidateAccess(inputIndex, static_cast<uint32_t>(arrayIndex + imageViews.size() - 1)))
        {
            const Interval interval = GetLayout()->GetGroupInterval(inputIndex);
            bool isValidAll = true;
            for (size_t i = 0; i < imageViews.size(); ++i)
            {
                const bool isValid = ValidateSetImageView(inputIndex, imageViews[i], aznumeric_caster(arrayIndex + i));
                if (isValid)
                {
                    m_imageViews[interval.m_min + arrayIndex + i] = imageViews[i];
                }
                isValidAll &= isValid;
            }

            if(!imageViews.empty())
            {
                EnableResourceTypeCompilation(ResourceTypeMask::ImageViewMask);
            }

            return isValidAll;
        }
        return false;
    }

    bool SingleDeviceShaderResourceGroupData::SetImageViewUnboundedArray(ShaderInputImageUnboundedArrayIndex inputIndex, AZStd::span<const ImageView* const> imageViews)
    {
        if (GetLayout()->ValidateAccess(inputIndex))
        {
            m_imageViewsUnboundedArray.clear();
            bool isValidAll = true;
            for (size_t i = 0; i < imageViews.size(); ++i)
            {
                const bool isValid = ValidateImageViewAccess<ShaderInputImageUnboundedArrayIndex, ShaderInputImageUnboundedArrayDescriptor>(inputIndex, imageViews[i], static_cast<uint32_t>(i));
                if (isValid)
                {
                    m_imageViewsUnboundedArray.push_back(imageViews[i]);
                }
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

    bool SingleDeviceShaderResourceGroupData::SetBufferView(ShaderInputBufferIndex inputIndex, const BufferView* bufferView, uint32_t arrayIndex)
    {
        AZStd::array<const BufferView*, 1> bufferViews = {{bufferView}};
        return SetBufferViewArray(inputIndex, bufferViews, arrayIndex);
    }

    bool SingleDeviceShaderResourceGroupData::SetBufferViewArray(ShaderInputBufferIndex inputIndex, AZStd::span<const BufferView* const> bufferViews, uint32_t arrayIndex)
    {
        if (GetLayout()->ValidateAccess(inputIndex, static_cast<uint32_t>(arrayIndex + bufferViews.size() - 1)))
        {
            const Interval interval = GetLayout()->GetGroupInterval(inputIndex);
            bool isValidAll = true;
            for (size_t i = 0; i < bufferViews.size(); ++i)
            {
                const bool isValid = ValidateSetBufferView(inputIndex, bufferViews[i], arrayIndex);
                if (isValid)
                {
                    m_bufferViews[interval.m_min + arrayIndex + i] = bufferViews[i];
                }
                isValidAll &= isValid;
            }

            if (!bufferViews.empty())
            {
                EnableResourceTypeCompilation(ResourceTypeMask::BufferViewMask);
            }
            return isValidAll;
        }
        return false;
    }

    bool SingleDeviceShaderResourceGroupData::SetBufferViewUnboundedArray(ShaderInputBufferUnboundedArrayIndex inputIndex, AZStd::span<const BufferView* const> bufferViews)
    {
        if (GetLayout()->ValidateAccess(inputIndex))
        {
            m_bufferViewsUnboundedArray.clear();
            bool isValidAll = true;
            for (size_t i = 0; i < bufferViews.size(); ++i)
            {
                const bool isValid = ValidateBufferViewAccess<ShaderInputBufferUnboundedArrayIndex, ShaderInputBufferUnboundedArrayDescriptor>(inputIndex, bufferViews[i], static_cast<uint32_t>(i));
                if (isValid)
                {
                    m_bufferViewsUnboundedArray.push_back(bufferViews[i]);
                }
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

    bool SingleDeviceShaderResourceGroupData::SetSampler(ShaderInputSamplerIndex inputIndex, const SamplerState& sampler, uint32_t arrayIndex)
    {
        return SetSamplerArray(inputIndex, AZStd::span<const SamplerState>(&sampler, 1), arrayIndex);
    }

    bool SingleDeviceShaderResourceGroupData::SetSamplerArray(ShaderInputSamplerIndex inputIndex, AZStd::span<const SamplerState> samplers, uint32_t arrayIndex)
    {
        if (GetLayout()->ValidateAccess(inputIndex, static_cast<uint32_t>(arrayIndex + samplers.size() - 1)))
        {
            const Interval interval = GetLayout()->GetGroupInterval(inputIndex);
            for (size_t i = 0; i < samplers.size(); ++i)
            {
                m_samplers[interval.m_min + arrayIndex + i] = samplers[i];
            }

            if (!samplers.empty())
            {
                EnableResourceTypeCompilation(ResourceTypeMask::SamplerMask);
            }
            return true;
        }
        return false;
    }

    bool SingleDeviceShaderResourceGroupData::SetConstantRaw(ShaderInputConstantIndex inputIndex, const void* bytes, uint32_t byteCount)
    {
        return SetConstantRaw(inputIndex, bytes, 0, byteCount);
    }

    bool SingleDeviceShaderResourceGroupData::SetConstantRaw(ShaderInputConstantIndex inputIndex, const void* bytes, uint32_t byteOffset, uint32_t byteCount)
    {
        EnableResourceTypeCompilation(ResourceTypeMask::ConstantDataMask);
        return m_constantsData.SetConstantRaw(inputIndex, bytes, byteOffset, byteCount);
    }

    bool SingleDeviceShaderResourceGroupData::SetConstantData(const void* bytes, uint32_t byteCount)
    {
        EnableResourceTypeCompilation(ResourceTypeMask::ConstantDataMask);
        return m_constantsData.SetConstantData(bytes, byteCount);
    }

    bool SingleDeviceShaderResourceGroupData::SetConstantData(const void* bytes, uint32_t byteOffset, uint32_t byteCount)
    {
        EnableResourceTypeCompilation(ResourceTypeMask::ConstantDataMask);
        return m_constantsData.SetConstantData(bytes, byteOffset, byteCount);
    }

    const RHI::ConstPtr<RHI::ImageView>& SingleDeviceShaderResourceGroupData::GetImageView(RHI::ShaderInputImageIndex inputIndex, uint32_t arrayIndex) const
    {
        if (GetLayout()->ValidateAccess(inputIndex, arrayIndex))
        {
            const Interval interval = GetLayout()->GetGroupInterval(inputIndex);
            return m_imageViews[interval.m_min + arrayIndex];
        }
        return s_nullImageView;
    }

    AZStd::span<const RHI::ConstPtr<RHI::ImageView>> SingleDeviceShaderResourceGroupData::GetImageViewArray(RHI::ShaderInputImageIndex inputIndex) const
    {
        if (GetLayout()->ValidateAccess(inputIndex, 0))
        {
            const Interval interval = GetLayout()->GetGroupInterval(inputIndex);
            return AZStd::span<const RHI::ConstPtr<RHI::ImageView>>(&m_imageViews[interval.m_min], interval.m_max - interval.m_min);
        }
        return {};
    }

    AZStd::span<const RHI::ConstPtr<RHI::ImageView>> SingleDeviceShaderResourceGroupData::GetImageViewUnboundedArray(RHI::ShaderInputImageUnboundedArrayIndex inputIndex) const
    {
        if (GetLayout()->ValidateAccess(inputIndex))
        {
            return AZStd::span<const RHI::ConstPtr<RHI::ImageView>>(m_imageViewsUnboundedArray.data(), m_imageViewsUnboundedArray.size());
        }
        return {};
    }

    const RHI::ConstPtr<RHI::BufferView>& SingleDeviceShaderResourceGroupData::GetBufferView(RHI::ShaderInputBufferIndex inputIndex, uint32_t arrayIndex) const
    {
        if (GetLayout()->ValidateAccess(inputIndex, arrayIndex))
        {
            const Interval interval = GetLayout()->GetGroupInterval(inputIndex);
            return m_bufferViews[interval.m_min + arrayIndex];
        }
        return s_nullBufferView;
    }

    AZStd::span<const RHI::ConstPtr<RHI::BufferView>> SingleDeviceShaderResourceGroupData::GetBufferViewArray(RHI::ShaderInputBufferIndex inputIndex) const
    {
        if (GetLayout()->ValidateAccess(inputIndex, 0))
        {
            const Interval interval = GetLayout()->GetGroupInterval(inputIndex);
            return AZStd::span<const RHI::ConstPtr<RHI::BufferView>>(&m_bufferViews[interval.m_min], interval.m_max - interval.m_min);
        }
        return {};
    }

    AZStd::span<const RHI::ConstPtr<RHI::BufferView>> SingleDeviceShaderResourceGroupData::GetBufferViewUnboundedArray(RHI::ShaderInputBufferUnboundedArrayIndex inputIndex) const
    {
        if (GetLayout()->ValidateAccess(inputIndex))
        {
            return AZStd::span<const RHI::ConstPtr<RHI::BufferView>>(m_bufferViewsUnboundedArray.data(), m_bufferViewsUnboundedArray.size());
        }
        return {};
    }

    const RHI::SamplerState& SingleDeviceShaderResourceGroupData::GetSampler(RHI::ShaderInputSamplerIndex inputIndex, uint32_t arrayIndex) const
    {
        if (GetLayout()->ValidateAccess(inputIndex, arrayIndex))
        {
            const Interval interval = GetLayout()->GetGroupInterval(inputIndex);
            return m_samplers[interval.m_min + arrayIndex];
        }
        return s_nullSamplerState;
    }

    AZStd::span<const RHI::SamplerState> SingleDeviceShaderResourceGroupData::GetSamplerArray(RHI::ShaderInputSamplerIndex inputIndex) const
    {
        const Interval interval = GetLayout()->GetGroupInterval(inputIndex);
        return AZStd::span<const RHI::SamplerState>(&m_samplers[interval.m_min], interval.m_max - interval.m_min);
    }

    AZStd::span<const uint8_t> SingleDeviceShaderResourceGroupData::GetConstantRaw(ShaderInputConstantIndex inputIndex) const
    {
        return m_constantsData.GetConstantRaw(inputIndex);
    }

    AZStd::span<const ConstPtr<ImageView>> SingleDeviceShaderResourceGroupData::GetImageGroup() const
    {
        return m_imageViews;
    }

    AZStd::span<const ConstPtr<BufferView>> SingleDeviceShaderResourceGroupData::GetBufferGroup() const
    {
        return m_bufferViews;
    }

    AZStd::span<const SamplerState> SingleDeviceShaderResourceGroupData::GetSamplerGroup() const
    {
        return m_samplers;
    }

    void SingleDeviceShaderResourceGroupData::ResetViews()
    {
        m_imageViews.assign(m_imageViews.size(), nullptr);
        m_bufferViews.assign(m_bufferViews.size(), nullptr);
        m_imageViewsUnboundedArray.assign(m_imageViewsUnboundedArray.size(), nullptr);
        m_bufferViewsUnboundedArray.assign(m_bufferViewsUnboundedArray.size(), nullptr);
    }

    AZStd::span<const uint8_t> SingleDeviceShaderResourceGroupData::GetConstantData() const
    {
        return m_constantsData.GetConstantData();
    }

    const ConstantsData& SingleDeviceShaderResourceGroupData::GetConstantsData() const
    {
        return m_constantsData;
    }

    uint32_t SingleDeviceShaderResourceGroupData::GetUpdateMask() const
    {
        return m_updateMask;
    }
    
    void SingleDeviceShaderResourceGroupData::EnableResourceTypeCompilation(ResourceTypeMask resourceTypeMask)
    {
        m_updateMask = RHI::SetBits(m_updateMask, static_cast<uint32_t>(resourceTypeMask));
    }

    void SingleDeviceShaderResourceGroupData::ResetUpdateMask()
    {
        m_updateMask = 0;
    }
    
    void SingleDeviceShaderResourceGroupData::SetBindlessViews(
        ShaderInputBufferIndex indirectResourceBufferIndex,
        const RHI::BufferView* indirectResourceBuffer,
        AZStd::span<const ImageView* const> imageViews,
        uint32_t* outIndices,
        AZStd::span<bool> isViewReadOnly,
        uint32_t arrayIndex)
    {
        BufferPoolDescriptor desc = static_cast<const SingleDeviceBufferPool*>(indirectResourceBuffer->GetBuffer().GetPool())->GetDescriptor();
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
            BindlessResourceType resourceType = BindlessResourceType::m_Texture2D;
            //Update the indirect buffer with view indices
            if (isViewReadOnly[i])
            {
                outIndices[i] = imageView->GetBindlessReadIndex();
            }
            else
            {
                resourceType = BindlessResourceType::m_RWTexture2D;
                outIndices[i] = imageView->GetBindlessReadWriteIndex();
            }
            it->second.m_bindlessResourceType = resourceType;
            ++i;
        }

        SetBufferView(indirectResourceBufferIndex, indirectResourceBuffer);
    }

    void SingleDeviceShaderResourceGroupData::SetBindlessViews(
        ShaderInputBufferIndex indirectResourceBufferIndex,
        const RHI::BufferView* indirectResourceBuffer,
        AZStd::span<const BufferView* const> bufferViews,
        uint32_t* outIndices,
        AZStd::span<bool> isViewReadOnly,
        uint32_t arrayIndex)
    {
        BufferPoolDescriptor desc = static_cast<const SingleDeviceBufferPool*>(indirectResourceBuffer->GetBuffer().GetPool())->GetDescriptor();
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
            BindlessResourceType resourceType = BindlessResourceType::m_ByteAddressBuffer;
            //Update the indirect buffer with view indices
            if (isViewReadOnly[i])
            {
                outIndices[i] = bufferView->GetBindlessReadIndex();
            }
            else
            {
                resourceType = BindlessResourceType::m_RWByteAddressBuffer;
                outIndices[i] = bufferView->GetBindlessReadWriteIndex();
            }
            it->second.m_bindlessResourceType = resourceType;
            ++i;
        }

        SetBufferView(indirectResourceBufferIndex, indirectResourceBuffer);
    }

    const uint32_t SingleDeviceShaderResourceGroupData::GetBindlessViewsSize() const
    {
        return aznumeric_cast<uint32_t>(m_bindlessResourceViews.size());
    }
 
    const AZStd::unordered_map<AZStd::pair<ShaderInputBufferIndex, uint32_t>, SingleDeviceShaderResourceGroupData::BindlessResourceViews>&
    SingleDeviceShaderResourceGroupData::GetBindlessResourceViews() const
    {
        return m_bindlessResourceViews;
    }
}
