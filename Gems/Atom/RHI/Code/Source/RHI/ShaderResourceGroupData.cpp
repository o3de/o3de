/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI.Reflect/Bits.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI/ShaderResourceGroupData.h>
#include <Atom/RHI/ShaderResourceGroupPool.h>

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
            : ShaderResourceGroupData(shaderResourceGroupPool.GetDeviceMask(), shaderResourceGroupPool.GetLayout())
        {}

        ShaderResourceGroupData::ShaderResourceGroupData(DeviceMask deviceMask, const ShaderResourceGroupLayout* layout)
            : m_deviceMask{ deviceMask }
            , m_shaderResourceGroupLayout(layout)
            , m_constantsData(layout->GetConstantsLayout())
        {
            m_imageViews.resize(layout->GetGroupSizeForImages());
            m_bufferViews.resize(layout->GetGroupSizeForBuffers());
            m_samplers.resize(layout->GetGroupSizeForSamplers());

            int deviceCount = RHI::RHISystemInterface::Get()->GetDeviceCount();

            for (int deviceIndex = 0; deviceIndex < deviceCount; ++deviceIndex)
            {
                if ((m_deviceMask >> deviceIndex) & 1)
                {
                    m_deviceShaderResourceGroupDatas[deviceIndex] = DeviceShaderResourceGroupData(layout);
                }
            }
        }

        const ShaderResourceGroupLayout* ShaderResourceGroupData::GetLayout() const
        {
            return m_shaderResourceGroupLayout.get();
        }

        bool ShaderResourceGroupData::ValidateSetImageView(
            ShaderInputImageIndex inputIndex, const ImageView* imageView, uint32_t arrayIndex) const
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

        bool ShaderResourceGroupData::ValidateSetBufferView(
            ShaderInputBufferIndex inputIndex, const BufferView* bufferView, uint32_t arrayIndex) const
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
            AZStd::array<const ImageView*, 1> imageViews = { { imageView } };
            return SetImageViewArray(inputIndex, imageViews, arrayIndex);
        }

        bool ShaderResourceGroupData::SetImageViewArray(
            ShaderInputImageIndex inputIndex, AZStd::span<const ImageView* const> imageViews, uint32_t arrayIndex)
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

                        for (auto& [deviceIndex, deviceShaderResourceGroupData] : m_deviceShaderResourceGroupDatas)
                        {
                            AZStd::vector<const DeviceImageView*> deviceImageViews(imageViews.size());

                            for (int imageIndex = 0; imageIndex < imageViews.size(); ++imageIndex)
                            {
                                deviceImageViews[imageIndex] =
                                    imageViews[imageIndex] ? imageViews[imageIndex]->GetDeviceImageView(deviceIndex).get() : nullptr;
                            }

                            isValidAll &= deviceShaderResourceGroupData.SetImageViewArray(inputIndex, deviceImageViews, arrayIndex);
                        }
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

        bool ShaderResourceGroupData::SetImageViewUnboundedArray(
            ShaderInputImageUnboundedArrayIndex inputIndex, AZStd::span<const ImageView* const> imageViews)
        {
            if (GetLayout()->ValidateAccess(inputIndex))
            {
                bool isValidAll = true;

                for (auto& [deviceIndex, deviceShaderResourceGroupData] : m_deviceShaderResourceGroupDatas)
                {
                    AZStd::vector<const DeviceImageView*> deviceImageViews(imageViews.size());

                    for (int i = 0; i < imageViews.size(); ++i)
                    {
                        deviceImageViews[i] = imageViews[i]->GetDeviceImageView(deviceIndex).get();
                    }

                    isValidAll &= deviceShaderResourceGroupData.SetImageViewUnboundedArray(inputIndex, deviceImageViews);
                }

                m_imageViewsUnboundedArray.clear();
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
            AZStd::array<const BufferView*, 1> bufferViews = { { bufferView } };
            return SetBufferViewArray(inputIndex, bufferViews, arrayIndex);
        }

        bool ShaderResourceGroupData::SetBufferViewArray(
            ShaderInputBufferIndex inputIndex, AZStd::span<const BufferView* const> bufferViews, uint32_t arrayIndex)
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

                        for (auto& [deviceIndex, deviceShaderResourceGroupData] : m_deviceShaderResourceGroupDatas)
                        {
                            AZStd::vector<const DeviceBufferView*> deviceBufferViews(bufferViews.size());

                            for (int bufferIndex = 0; bufferIndex < bufferViews.size(); ++bufferIndex)
                            {
                                deviceBufferViews[bufferIndex] =
                                    bufferViews[bufferIndex] ? bufferViews[bufferIndex]->GetDeviceBufferView(deviceIndex).get() : nullptr;
                            }

                            isValidAll &= deviceShaderResourceGroupData.SetBufferViewArray(inputIndex, deviceBufferViews, arrayIndex);
                        }
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

        bool ShaderResourceGroupData::SetBufferViewUnboundedArray(
            ShaderInputBufferUnboundedArrayIndex inputIndex, AZStd::span<const BufferView* const> bufferViews)
        {
            if (GetLayout()->ValidateAccess(inputIndex))
            {
                bool isValidAll = true;

                for (auto& [deviceIndex, deviceShaderResourceGroupData] : m_deviceShaderResourceGroupDatas)
                {
                    AZStd::vector<const DeviceBufferView*> deviceBufferViews(bufferViews.size());

                    for (int i = 0; i < bufferViews.size(); ++i)
                    {
                        deviceBufferViews[i] = bufferViews[i]->GetDeviceBufferView(deviceIndex).get();
                    }

                    isValidAll &= deviceShaderResourceGroupData.SetBufferViewUnboundedArray(inputIndex, deviceBufferViews);
                }

                m_bufferViewsUnboundedArray.clear();
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

        bool ShaderResourceGroupData::SetSamplerArray(
            ShaderInputSamplerIndex inputIndex, AZStd::span<const SamplerState> samplers, uint32_t arrayIndex)
        {
            if (GetLayout()->ValidateAccess(inputIndex, static_cast<uint32_t>(arrayIndex + samplers.size() - 1)))
            {
                const Interval interval = GetLayout()->GetGroupInterval(inputIndex);
                for (size_t i = 0; i < samplers.size(); ++i)
                {
                    m_samplers[interval.m_min + arrayIndex + i] = samplers[i];
                }

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

        bool ShaderResourceGroupData::SetConstantRaw(ShaderInputConstantIndex inputIndex, const void* bytes, uint32_t byteCount)
        {
            return SetConstantRaw(inputIndex, bytes, 0, byteCount);
        }

        bool ShaderResourceGroupData::SetConstantRaw(
            ShaderInputConstantIndex inputIndex, const void* bytes, uint32_t byteOffset, uint32_t byteCount)
        {
            EnableResourceTypeCompilation(ResourceTypeMask::ConstantDataMask);

            bool isValidAll = true;

            for (auto& [deviceIndex, deviceShaderResourceGroupData] : m_deviceShaderResourceGroupDatas)
            {
                isValidAll &= deviceShaderResourceGroupData.SetConstantRaw(inputIndex, bytes, byteOffset, byteCount);
            }

            return isValidAll & m_constantsData.SetConstantRaw(inputIndex, bytes, byteOffset, byteCount);
        }

        bool ShaderResourceGroupData::SetConstantData(const void* bytes, uint32_t byteCount)
        {
            EnableResourceTypeCompilation(ResourceTypeMask::ConstantDataMask);

            bool isValidAll = true;

            for (auto& [deviceIndex, deviceShaderResourceGroupData] : m_deviceShaderResourceGroupDatas)
            {
                isValidAll &= deviceShaderResourceGroupData.SetConstantData(bytes, byteCount);
            }

            return isValidAll & m_constantsData.SetConstantData(bytes, byteCount);
        }

        bool ShaderResourceGroupData::SetConstantData(const void* bytes, uint32_t byteOffset, uint32_t byteCount)
        {
            EnableResourceTypeCompilation(ResourceTypeMask::ConstantDataMask);

            bool isValidAll = true;

            for (auto& [deviceIndex, deviceShaderResourceGroupData] : m_deviceShaderResourceGroupDatas)
            {
                isValidAll &= deviceShaderResourceGroupData.SetConstantData(bytes, byteOffset, byteCount);
            }

            return isValidAll & m_constantsData.SetConstantData(bytes, byteOffset, byteCount);
        }

        const DeviceShaderResourceGroupData& ShaderResourceGroupData::GetDeviceShaderResourceGroupData(int deviceIndex) const
        {
            AZ_Assert(
                m_deviceShaderResourceGroupDatas.find(deviceIndex) != m_deviceShaderResourceGroupDatas.end(),
                "No DeviceShaderResourceGroupData found for device index %d\n",
                deviceIndex);
            return m_deviceShaderResourceGroupDatas.at(deviceIndex);
        }

        const RHI::ConstPtr<RHI::ImageView>& ShaderResourceGroupData::GetImageView(
            RHI::ShaderInputImageIndex inputIndex, uint32_t arrayIndex) const
        {
            if (GetLayout()->ValidateAccess(inputIndex, arrayIndex))
            {
                const Interval interval = GetLayout()->GetGroupInterval(inputIndex);
                return m_imageViews[interval.m_min + arrayIndex];
            }
            return s_nullImageView;
        }

        AZStd::span<const RHI::ConstPtr<RHI::ImageView>> ShaderResourceGroupData::GetImageViewArray(
            RHI::ShaderInputImageIndex inputIndex) const
        {
            if (GetLayout()->ValidateAccess(inputIndex, 0))
            {
                const Interval interval = GetLayout()->GetGroupInterval(inputIndex);
                return AZStd::span<const RHI::ConstPtr<RHI::ImageView>>(&m_imageViews[interval.m_min], interval.m_max - interval.m_min);
            }
            return {};
        }

        AZStd::span<const RHI::ConstPtr<RHI::ImageView>> ShaderResourceGroupData::GetImageViewUnboundedArray(
            RHI::ShaderInputImageUnboundedArrayIndex inputIndex) const
        {
            if (GetLayout()->ValidateAccess(inputIndex))
            {
                return AZStd::span<const RHI::ConstPtr<RHI::ImageView>>(
                    m_imageViewsUnboundedArray.data(), m_imageViewsUnboundedArray.size());
            }
            return {};
        }

        const RHI::ConstPtr<RHI::BufferView>& ShaderResourceGroupData::GetBufferView(
            RHI::ShaderInputBufferIndex inputIndex, uint32_t arrayIndex) const
        {
            if (GetLayout()->ValidateAccess(inputIndex, arrayIndex))
            {
                const Interval interval = GetLayout()->GetGroupInterval(inputIndex);
                return m_bufferViews[interval.m_min + arrayIndex];
            }
            return s_nullBufferView;
        }

        AZStd::span<const RHI::ConstPtr<RHI::BufferView>> ShaderResourceGroupData::GetBufferViewArray(
            RHI::ShaderInputBufferIndex inputIndex) const
        {
            if (GetLayout()->ValidateAccess(inputIndex, 0))
            {
                const Interval interval = GetLayout()->GetGroupInterval(inputIndex);
                return AZStd::span<const RHI::ConstPtr<RHI::BufferView>>(&m_bufferViews[interval.m_min], interval.m_max - interval.m_min);
            }
            return {};
        }

        AZStd::span<const RHI::ConstPtr<RHI::BufferView>> ShaderResourceGroupData::GetBufferViewUnboundedArray(
            RHI::ShaderInputBufferUnboundedArrayIndex inputIndex) const
        {
            if (GetLayout()->ValidateAccess(inputIndex))
            {
                return AZStd::span<const RHI::ConstPtr<RHI::BufferView>>(
                    m_bufferViewsUnboundedArray.data(), m_bufferViewsUnboundedArray.size());
            }
            return {};
        }

        const RHI::SamplerState& ShaderResourceGroupData::GetSampler(
            RHI::ShaderInputSamplerIndex inputIndex, uint32_t arrayIndex) const
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
            for (auto& [deviceIndex, deviceShaderResourceGroupData] : m_deviceShaderResourceGroupDatas)
            {
                deviceShaderResourceGroupData.ResetViews();
            }

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
            for (auto& [deviceIndex, deviceShaderResourceGroupData] : m_deviceShaderResourceGroupDatas)
            {
                deviceShaderResourceGroupData.EnableResourceTypeCompilation(resourceTypeMask);
            }

            m_updateMask = RHI::SetBits(m_updateMask, static_cast<uint32_t>(resourceTypeMask));
        }

        void ShaderResourceGroupData::ResetUpdateMask()
        {
            for (auto& [deviceIndex, deviceShaderResourceGroupData] : m_deviceShaderResourceGroupDatas)
            {
                deviceShaderResourceGroupData.ResetUpdateMask();
            }

            m_updateMask = 0;
        }
 
    } // namespace RHI
} // namespace AZ
