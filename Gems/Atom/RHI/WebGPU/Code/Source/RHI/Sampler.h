/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceObject.h>

namespace AZ::RHI
{
    class SamplerState;
}

namespace AZ::WebGPU
{
    class Device;

    //! Encapsulates a WebGPU sampler object
    class Sampler final
        : public RHI::DeviceObject
    {
        using Base = RHI::DeviceObject;

    public:
        AZ_CLASS_ALLOCATOR(Sampler, AZ::SystemAllocator);
        AZ_RTTI(Sampler, "{A4CB9EBE-E423-48D5-B199-312D52C1CD98}", Base);

        struct Descriptor
        {
            size_t GetHash() const;
            RHI::SamplerState m_samplerState;
        };
        static RHI::Ptr<Sampler> Create();
        RHI::ResultCode Init(Device& device, const Descriptor& descriptor);
        ~Sampler() = default;

        //! Returns the native sampler object
        const wgpu::Sampler& GetNativeSampler() const;

    private:
        Sampler() = default;

        //////////////////////////////////////////////////////////////////////////
        // RHI::Object
        void SetNameInternal(const AZStd::string_view& name) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // RHI::DeviceObject
        void Shutdown() override;
        //////////////////////////////////////////////////////////////////////////

        RHI::ResultCode BuildNativeSampler();

        Descriptor m_descriptor;
        //! Native sampler
        wgpu::Sampler m_wgpuSampler = nullptr;
    };
}
