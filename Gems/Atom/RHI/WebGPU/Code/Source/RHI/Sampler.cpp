/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/WebGPU.h>
#include <RHI/Device.h>
#include <RHI/Sampler.h>

namespace AZ::WebGPU
{
    size_t Sampler::Descriptor::GetHash() const
    {
        return static_cast<size_t>(m_samplerState.GetHash());
    }

    RHI::Ptr<Sampler> Sampler::Create()
    {
        return aznew Sampler();
    }

    RHI::ResultCode Sampler::Init(Device& device, const Descriptor& descriptor)
    {
        m_descriptor = descriptor;
        Base::Init(device);

        const RHI::ResultCode result = BuildNativeSampler();
        RETURN_RESULT_IF_UNSUCCESSFUL(result);
        return result;
    }

    const wgpu::Sampler& Sampler::GetNativeSampler() const
    {
        return m_wgpuSampler;
    }

    void Sampler::SetNameInternal(const AZStd::string_view& name)
    {
        if (m_wgpuSampler && !name.empty())
        {
            m_wgpuSampler.SetLabel(name.data());
        }
    }

    void Sampler::Shutdown()
    {
        m_wgpuSampler = nullptr;
        Base::Shutdown();
    }

    RHI::ResultCode Sampler::BuildNativeSampler()
    {
        auto& device = static_cast<Device&>(GetDevice());
        const RHI::SamplerState& samplerState = m_descriptor.m_samplerState;
        RHI::FilterMode filterMin = samplerState.m_filterMin;
        RHI::FilterMode filterMag = samplerState.m_filterMag;
        RHI::FilterMode filterMip = samplerState.m_filterMip;
        if (samplerState.m_anisotropyEnable)
        {
            // This is to match dx12 behavior (linear min/mag/mip when anistropic is enabled)
            filterMag = RHI::FilterMode::Linear;
            filterMin = RHI::FilterMode::Linear;
            filterMip = RHI::FilterMode::Linear;
        }

        wgpu::SamplerDescriptor desc = {};
        desc.addressModeU = ConvertAddressMode(samplerState.m_addressU);
        desc.addressModeV = ConvertAddressMode(samplerState.m_addressV);
        desc.addressModeW = ConvertAddressMode(samplerState.m_addressW);
        desc.magFilter = ConvertFilterMode(filterMag);
        desc.minFilter = ConvertFilterMode(filterMin);
        desc.mipmapFilter = ConvertMipMapFilterMode(filterMip);
        desc.lodMinClamp = samplerState.m_mipLodMin;
        desc.lodMaxClamp = samplerState.m_mipLodMax;
        desc.compare = samplerState.m_reductionType == RHI::ReductionType::Comparison
            ? ConvertCompareFunction(samplerState.m_comparisonFunc)
            : wgpu::CompareFunction::Undefined;  // Must be "Undefined" if not being used, if not it causes an error.
        desc.maxAnisotropy = aznumeric_caster(AZStd::max(samplerState.m_anisotropyMax, 1u));
        desc.label = GetName().GetCStr();

        m_wgpuSampler = device.GetNativeDevice().CreateSampler(&desc);
        AZ_Assert(m_wgpuSampler, "Failed to create sampler");
        return m_wgpuSampler ? RHI::ResultCode::Success : RHI::ResultCode::Fail;
    }
}
