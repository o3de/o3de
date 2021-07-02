/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "RHI/Atom_RHI_DX12_precompiled.h"
#include <RHI/Sampler.h>
#include <RHI/Conversions.h>
#include <RHI/DescriptorContext.h>
#include <RHI/Device.h>

namespace AZ
{
    namespace DX12
    {
        RHI::Ptr<Sampler> Sampler::Create()
        {
            return aznew Sampler();
        }

        void Sampler::Init(Device& device, const RHI::SamplerState& samplerState)
        {
            Base::Init(device);
            DescriptorContext& context = device.GetDescriptorContext();
            context.CreateSampler(samplerState, m_descriptor);
        }

        DescriptorHandle Sampler::GetDescriptorHandle() const
        {
            return m_descriptor;
        }

        void Sampler::Shutdown()
        {
            auto& device = static_cast<Device&>(GetDevice());
            DescriptorContext& context = device.GetDescriptorContext();
            context.ReleaseDescriptor(m_descriptor);
            m_descriptor = DescriptorHandle();
            Base::Shutdown();
        }
    }
}
