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

namespace AZ
{
    namespace RHI
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

        bool ShaderResourceGroupData::SetImageViewArray(ShaderInputImageIndex inputIndex, AZStd::array_view<const ImageView*> imageViews, uint32_t arrayIndex)
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
                    EnableResourceTypeCompilation(ResourceTypeMask::ImageViewMask, ResourceType::ImageView);
                }
                
                return isValidAll;
            }
            return false;
        }

        bool ShaderResourceGroupData::SetImageViewUnboundedArray(ShaderInputImageUnboundedArrayIndex inputIndex, AZStd::array_view<const ImageView*> imageViews)
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
                    EnableResourceTypeCompilation(ResourceTypeMask::ImageViewUnboundedArrayMask, ResourceType::ImageViewUnboundedArray);
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

        bool ShaderResourceGroupData::SetBufferViewArray(ShaderInputBufferIndex inputIndex, AZStd::array_view<const BufferView*> bufferViews, uint32_t arrayIndex)
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
                    EnableResourceTypeCompilation(ResourceTypeMask::BufferViewMask, ResourceType::BufferView);
                }
                return isValidAll;
            }
            return false;
        }

        bool ShaderResourceGroupData::SetBufferViewUnboundedArray(ShaderInputBufferUnboundedArrayIndex inputIndex, AZStd::array_view<const BufferView*> bufferViews)
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
                    EnableResourceTypeCompilation(ResourceTypeMask::BufferViewUnboundedArrayMask, ResourceType::BufferViewUnboundedArray);
                }
                return isValidAll;
            }
            return false;
        }

        bool ShaderResourceGroupData::SetSampler(ShaderInputSamplerIndex inputIndex, const SamplerState& sampler, uint32_t arrayIndex)
        {
            return SetSamplerArray(inputIndex, AZStd::array_view<SamplerState>(&sampler, 1), arrayIndex);
        }

        bool ShaderResourceGroupData::SetSamplerArray(ShaderInputSamplerIndex inputIndex, AZStd::array_view<SamplerState> samplers, uint32_t arrayIndex)
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
                    EnableResourceTypeCompilation(ResourceTypeMask::SamplerMask, ResourceType::Sampler);
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
            EnableResourceTypeCompilation(ResourceTypeMask::ConstantDataMask, ResourceType::ConstantData);
            return m_constantsData.SetConstantRaw(inputIndex, bytes, byteOffset, byteCount);
        }

        bool ShaderResourceGroupData::SetConstantData(const void* bytes, uint32_t byteCount)
        {
            EnableResourceTypeCompilation(ResourceTypeMask::ConstantDataMask, ResourceType::ConstantData);
            return m_constantsData.SetConstantData(bytes, byteCount);
        }

        bool ShaderResourceGroupData::SetConstantData(const void* bytes, uint32_t byteOffset, uint32_t byteCount)
        {
            EnableResourceTypeCompilation(ResourceTypeMask::ConstantDataMask, ResourceType::ConstantData);
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

        AZStd::array_view<RHI::ConstPtr<RHI::ImageView>> ShaderResourceGroupData::GetImageViewArray(RHI::ShaderInputImageIndex inputIndex) const
        {
            if (GetLayout()->ValidateAccess(inputIndex, 0))
            {
                const Interval interval = GetLayout()->GetGroupInterval(inputIndex);
                return AZStd::array_view<RHI::ConstPtr<RHI::ImageView>>(&m_imageViews[interval.m_min], interval.m_max - interval.m_min);
            }
            return {};
        }

        AZStd::array_view<RHI::ConstPtr<RHI::ImageView>> ShaderResourceGroupData::GetImageViewUnboundedArray(RHI::ShaderInputImageUnboundedArrayIndex inputIndex) const
        {
            if (GetLayout()->ValidateAccess(inputIndex))
            {
                return AZStd::array_view<RHI::ConstPtr<RHI::ImageView>>(m_imageViewsUnboundedArray.data(), m_imageViewsUnboundedArray.size());
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

        AZStd::array_view<RHI::ConstPtr<RHI::BufferView>> ShaderResourceGroupData::GetBufferViewArray(RHI::ShaderInputBufferIndex inputIndex) const
        {
            if (GetLayout()->ValidateAccess(inputIndex, 0))
            {
                const Interval interval = GetLayout()->GetGroupInterval(inputIndex);
                return AZStd::array_view<RHI::ConstPtr<RHI::BufferView>>(&m_bufferViews[interval.m_min], interval.m_max - interval.m_min);
            }
            return {};
        }

        AZStd::array_view<RHI::ConstPtr<RHI::BufferView>> ShaderResourceGroupData::GetBufferViewUnboundedArray(RHI::ShaderInputBufferUnboundedArrayIndex inputIndex) const
        {
            if (GetLayout()->ValidateAccess(inputIndex))
            {
                return AZStd::array_view<RHI::ConstPtr<RHI::BufferView>>(m_bufferViewsUnboundedArray.data(), m_bufferViewsUnboundedArray.size());
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

        AZStd::array_view<RHI::SamplerState> ShaderResourceGroupData::GetSamplerArray(RHI::ShaderInputSamplerIndex inputIndex) const
        {
            const Interval interval = GetLayout()->GetGroupInterval(inputIndex);
            return AZStd::array_view<RHI::SamplerState>(&m_samplers[interval.m_min], interval.m_max - interval.m_min);
        }

        AZStd::array_view<uint8_t> ShaderResourceGroupData::GetConstantRaw(ShaderInputConstantIndex inputIndex) const
        {
            return m_constantsData.GetConstantRaw(inputIndex);
        }

        AZStd::array_view<ConstPtr<ImageView>> ShaderResourceGroupData::GetImageGroup() const
        {
            return m_imageViews;
        }

        AZStd::array_view<ConstPtr<BufferView>> ShaderResourceGroupData::GetBufferGroup() const
        {
            return m_bufferViews;
        }

        AZStd::array_view<SamplerState> ShaderResourceGroupData::GetSamplerGroup() const
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

        AZStd::array_view<uint8_t> ShaderResourceGroupData::GetConstantData() const
        {
            return m_constantsData.GetConstantData();
        }

        const ConstantsData& ShaderResourceGroupData::GetConstantsData() const
        {
            return m_constantsData;
        }

        bool ShaderResourceGroupData::IsResourceTypeEnabledForCompilation(uint32_t resourceTypeMask) const
        {
            return RHI::CheckBitsAny(m_updateMask, resourceTypeMask);
        }

        bool ShaderResourceGroupData::IsAnyResourceTypeUpdated() const
        {
            return m_updateMask != 0;
        }

        void ShaderResourceGroupData::EnableResourceTypeCompilation(ResourceTypeMask resourceTypeMask, ResourceType resourceType)
        {
            AZ_Assert(static_cast<uint32_t>(resourceTypeMask) == AZ_BIT(static_cast<uint32_t>(resourceType)), "resourceType and resourceTypeMask should point to the same ResourceType");
            m_updateMask = RHI::SetBits(m_updateMask, static_cast<uint32_t>(resourceTypeMask));
            m_resourceTypeIteration[static_cast<uint32_t>(resourceType)] = 0;
        }

        void ShaderResourceGroupData::DisableCompilationForAllResourceTypes()
        {
            for (uint32_t i = 0; i < static_cast<uint32_t>(ResourceType::Count); i++)
            {
                if (m_resourceTypeIteration[i] == m_updateMaskResetLatency)
                {
                    m_updateMask = RHI::ResetBits(m_updateMask, AZ_BIT(i));
                }
                m_resourceTypeIteration[i]++;
            }
        }
    } // namespace RHI
} // namespace AZ
