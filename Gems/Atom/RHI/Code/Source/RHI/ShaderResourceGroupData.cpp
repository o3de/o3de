/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/ShaderResourceGroupData.h>
#include <Atom/RHI/ShaderResourceGroupPool.h>
#include <Atom/RHI.Reflect/Bits.h>
#include <Atom/RHI/BufferPool.h>

namespace AZ::RHI
{
    const ConstPtr<ImageView> ShaderResourceGroupData::s_nullImageView;
    const ConstPtr<BufferView> ShaderResourceGroupData::s_nullBufferView;
    const SamplerState ShaderResourceGroupData::s_nullSamplerState{};

    ShaderResourceGroupData::ShaderResourceGroupData() = default;
    ShaderResourceGroupData::~ShaderResourceGroupData() = default;

    ShaderResourceGroupData::ShaderResourceGroupData(const ShaderResourceGroup& shaderResourceGroup)
        : ShaderResourceGroupData(*shaderResourceGroup.GetPool())
    {}

    ShaderResourceGroupData::ShaderResourceGroupData(const ShaderResourceGroupPool& shaderResourceGroupPool)
        : ShaderResourceGroupData(shaderResourceGroupPool.GetLayout())
    {}

    ShaderResourceGroupData::ShaderResourceGroupData(const ShaderResourceGroupLayout* layout)
        : m_shaderResourceGroupLayout(layout)
        , m_constantsData(layout->GetConstantsLayout())
    {
        m_imageViews.resize(layout->GetGroupSizeForImages());
        m_bufferViews.resize(layout->GetGroupSizeForBuffers());
        m_samplers.resize(layout->GetGroupSizeForSamplers());
    }

    const ShaderResourceGroupLayout* ShaderResourceGroupData::GetLayout() const
    {
        return m_shaderResourceGroupLayout.get();
    }

    bool ShaderResourceGroupData::ValidateSetImageView(ShaderInputImageIndex inputIndex, const ImageView* imageView, uint32_t arrayIndex) const
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

    bool ShaderResourceGroupData::ValidateSetBufferView(ShaderInputBufferIndex inputIndex, const BufferView* bufferView, uint32_t arrayIndex) const
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

    bool ShaderResourceGroupData::SetImageView(ShaderInputImageIndex inputIndex, const ImageView* imageView, uint32_t arrayIndex = 0)
    {
        AZStd::array<const ImageView*, 1> imageViews = {{imageView}};
        return SetImageViewArray(inputIndex, imageViews, arrayIndex);
    }

    bool ShaderResourceGroupData::SetImageViewArray(ShaderInputImageIndex inputIndex, AZStd::span<const ImageView* const> imageViews, uint32_t arrayIndex)
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

    bool ShaderResourceGroupData::SetImageViewUnboundedArray(ShaderInputImageUnboundedArrayIndex inputIndex, AZStd::span<const ImageView* const> imageViews)
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

    bool ShaderResourceGroupData::SetBufferView(ShaderInputBufferIndex inputIndex, const BufferView* bufferView, uint32_t arrayIndex)
    {
        AZStd::array<const BufferView*, 1> bufferViews = {{bufferView}};
        return SetBufferViewArray(inputIndex, bufferViews, arrayIndex);
    }

    bool ShaderResourceGroupData::SetBufferViewArray(ShaderInputBufferIndex inputIndex, AZStd::span<const BufferView* const> bufferViews, uint32_t arrayIndex)
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

    bool ShaderResourceGroupData::SetBufferViewUnboundedArray(ShaderInputBufferUnboundedArrayIndex inputIndex, AZStd::span<const BufferView* const> bufferViews)
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

    bool ShaderResourceGroupData::SetSampler(ShaderInputSamplerIndex inputIndex, const SamplerState& sampler, uint32_t arrayIndex)
    {
        return SetSamplerArray(inputIndex, AZStd::span<const SamplerState>(&sampler, 1), arrayIndex);
    }

    bool ShaderResourceGroupData::SetSamplerArray(ShaderInputSamplerIndex inputIndex, AZStd::span<const SamplerState> samplers, uint32_t arrayIndex)
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

    bool ShaderResourceGroupData::SetConstantRaw(ShaderInputConstantIndex inputIndex, const void* bytes, uint32_t byteCount)
    {
        return SetConstantRaw(inputIndex, bytes, 0, byteCount);
    }

    bool ShaderResourceGroupData::SetConstantRaw(ShaderInputConstantIndex inputIndex, const void* bytes, uint32_t byteOffset, uint32_t byteCount)
    {
        EnableResourceTypeCompilation(ResourceTypeMask::ConstantDataMask);
        return m_constantsData.SetConstantRaw(inputIndex, bytes, byteOffset, byteCount);
    }

    bool ShaderResourceGroupData::SetConstantData(const void* bytes, uint32_t byteCount)
    {
        EnableResourceTypeCompilation(ResourceTypeMask::ConstantDataMask);
        return m_constantsData.SetConstantData(bytes, byteCount);
    }

    bool ShaderResourceGroupData::SetConstantData(const void* bytes, uint32_t byteOffset, uint32_t byteCount)
    {
        EnableResourceTypeCompilation(ResourceTypeMask::ConstantDataMask);
        return m_constantsData.SetConstantData(bytes, byteOffset, byteCount);
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

    void ShaderResourceGroupData::ResetViews()
    {
        m_imageViews.assign(m_imageViews.size(), nullptr);
        m_bufferViews.assign(m_bufferViews.size(), nullptr);
        m_imageViewsUnboundedArray.assign(m_imageViewsUnboundedArray.size(), nullptr);
        m_bufferViewsUnboundedArray.assign(m_bufferViewsUnboundedArray.size(), nullptr);
    }

    AZStd::span<const uint8_t> ShaderResourceGroupData::GetConstantData() const
    {
        return m_constantsData.GetConstantData();
    }

    const ConstantsData& ShaderResourceGroupData::GetConstantsData() const
    {
        return m_constantsData;
    }

    uint32_t ShaderResourceGroupData::GetUpdateMask() const
    {
        return m_updateMask;
    }
    
    void ShaderResourceGroupData::EnableResourceTypeCompilation(ResourceTypeMask resourceTypeMask)
    {
        m_updateMask = RHI::SetBits(m_updateMask, static_cast<uint32_t>(resourceTypeMask));
    }

    void ShaderResourceGroupData::ResetUpdateMask()
    {
        m_updateMask = 0;
    }
    
    void ShaderResourceGroupData::SetBindlessViews(
        ShaderInputBufferIndex indirectResourceBufferIndex,
        const RHI::BufferView* indirectResourceBuffer,
        AZStd::span<const ImageView* const> imageViews,
        uint32_t* outIndices,
        AZStd::span<bool> isViewReadOnly,
        uint32_t arrayIndex)
    {
        BufferPoolDescriptor desc = static_cast<const BufferPool*>(indirectResourceBuffer->GetBuffer().GetPool())->GetDescriptor();
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

    void ShaderResourceGroupData::SetBindlessViews(
        ShaderInputBufferIndex indirectResourceBufferIndex,
        const RHI::BufferView* indirectResourceBuffer,
        AZStd::span<const BufferView* const> bufferViews,
        uint32_t* outIndices,
        AZStd::span<bool> isViewReadOnly,
        uint32_t arrayIndex)
    {
        BufferPoolDescriptor desc = static_cast<const BufferPool*>(indirectResourceBuffer->GetBuffer().GetPool())->GetDescriptor();
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

    const uint32_t ShaderResourceGroupData::GetBindlessViewsSize() const
    {
        return aznumeric_cast<uint32_t>(m_bindlessResourceViews.size());
    }
 
    const AZStd::unordered_map<AZStd::pair<ShaderInputBufferIndex, uint32_t>, ShaderResourceGroupData::BindlessResourceViews>&
    ShaderResourceGroupData::GetBindlessResourceViews() const
    {
        return m_bindlessResourceViews;
    }
}
