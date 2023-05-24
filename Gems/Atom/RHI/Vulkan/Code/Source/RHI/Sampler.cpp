/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/std/hash.h>
#include <Atom/RHI.Reflect/Vulkan/Conversion.h>
#include <RHI/Device.h>
#include <RHI/Sampler.h>
#include <Atom/RHI.Reflect/VkAllocator.h>

namespace AZ
{
    namespace Vulkan
    {
        size_t Sampler::Descriptor::GetHash() const
        {
            return static_cast<size_t>(m_samplerState.GetHash());
        }

        RHI::Ptr<Sampler> Sampler::Create()
        {
            return aznew Sampler();
        }

        RHI::ResultCode Sampler::Init(const Descriptor& descriptor)
        {
            m_descriptor = descriptor;
            AZ_Assert(descriptor.m_device, "Device is null.");
            Base::Init(*descriptor.m_device);

            const RHI::ResultCode result = BuildNativeSampler();
            RETURN_RESULT_IF_UNSUCCESSFUL(result);

            SetName(GetName());
            return result;
        }

        VkSampler Sampler::GetNativeSampler() const
        {
            return m_nativeSampler;
        }

        void Sampler::SetNameInternal(const AZStd::string_view& name)
        {
            if (IsInitialized() && !name.empty())
            {
                Debug::SetNameToObject(reinterpret_cast<uint64_t>(m_nativeSampler), name.data(), VK_OBJECT_TYPE_SAMPLER, static_cast<Device&>(GetDevice()));
            }
        }

        void Sampler::Shutdown()
        {
            if (m_nativeSampler != VK_NULL_HANDLE)
            {
                auto& device = static_cast<Device&>(GetDevice());
                device.GetContext().DestroySampler(device.GetNativeDevice(), m_nativeSampler, VkSystemAllocator::Get());
                m_nativeSampler = VK_NULL_HANDLE;
            }
            Base::Shutdown();
        }

        RHI::ResultCode Sampler::BuildNativeSampler()
        {
            const RHI::SamplerState& samplerState = m_descriptor.m_samplerState;
            auto& device = static_cast<Device&>(GetDevice());
            const auto& physicalDevice = static_cast<const PhysicalDevice&>(device.GetPhysicalDevice());
            const VkPhysicalDeviceFeatures deviceFeatures = device.GetEnabledDevicesFeatures();
            const VkPhysicalDeviceLimits deviceLimits = physicalDevice.GetDeviceLimits();
            const float maxSamplerAnisotropy = deviceLimits.maxSamplerAnisotropy;

            VkSamplerCreateInfo createInfo{};
            RHI::FilterMode filterMin = samplerState.m_filterMin;
            RHI::FilterMode filterMag = samplerState.m_filterMag;
            RHI::FilterMode filterMip = samplerState.m_filterMip;
            if (samplerState.m_anisotropyEnable)
            {
                // This is to match dx12 behavior (linear min/mag/mip when anistropic is enabled)
                filterMag = RHI::FilterMode::Linear;
                filterMin = RHI::FilterMode::Linear;
                filterMip = RHI::FilterMode::Linear;
                createInfo.anisotropyEnable = deviceFeatures.samplerAnisotropy;
            }

            createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            createInfo.pNext = nullptr;
            createInfo.flags = 0;
            createInfo.magFilter = ConvertFilterMode(filterMag);
            createInfo.minFilter = ConvertFilterMode(filterMin);

            switch (filterMip)
            {
            case RHI::FilterMode::Point:
                createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
                break;
            case RHI::FilterMode::Linear:
                createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
                break;
            default:
                AZ_Assert(false, "FilterMip is illegal.");
            }

            createInfo.addressModeU = ConvertAddressMode(samplerState.m_addressU);
            createInfo.addressModeV = ConvertAddressMode(samplerState.m_addressV);
            createInfo.addressModeW = ConvertAddressMode(samplerState.m_addressW);
            createInfo.mipLodBias = samplerState.m_mipLodBias;
            createInfo.maxAnisotropy = AZStd::clamp(samplerState.m_anisotropyMax * 1.f, 1.f, maxSamplerAnisotropy);
            createInfo.compareEnable = samplerState.m_comparisonFunc != RHI::ComparisonFunc::Always;
            createInfo.compareOp = ConvertComparisonFunction(samplerState.m_comparisonFunc);
            createInfo.minLod = samplerState.m_mipLodMin;
            createInfo.maxLod = samplerState.m_mipLodMax;

            switch (samplerState.m_borderColor)
            {
            case RHI::BorderColor::OpaqueBlack:
                createInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
                break;
            case RHI::BorderColor::TransparentBlack:
                createInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
                break;
            case RHI::BorderColor::OpaqueWhite:
                createInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
                break;
            default:
                AZ_Assert(false, "BorderColor is illegal.");
            }

            createInfo.unnormalizedCoordinates = VK_FALSE;

            const VkResult result =
                device.GetContext().CreateSampler(device.GetNativeDevice(), &createInfo, VkSystemAllocator::Get(), &m_nativeSampler);
            AssertSuccess(result);

            return ConvertResult(result);
        }
    }
}
