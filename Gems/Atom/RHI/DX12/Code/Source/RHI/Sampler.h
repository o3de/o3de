/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <RHI/Descriptor.h>
#include <Atom/RHI/DeviceObject.h>
#include <AzCore/Memory/PoolAllocator.h>

namespace AZ
{
    namespace RHI
    {
        class SamplerState;
    }

    namespace DX12
    {
        class Device;

        class Sampler final
            : public RHI::DeviceObject
        {
            using Base = RHI::DeviceObject;
        public:
            AZ_CLASS_ALLOCATOR(Sampler, AZ::ThreadPoolAllocator);
            AZ_RTTI(Sampler, "{58DC554B-8E8D-4533-AA9F-F9341BFC573B}", Base);

            static RHI::Ptr<Sampler> Create();

            ~Sampler() = default;

            void Init(Device& device, const RHI::SamplerState& samplerState);

            /// Returns the descriptor handle
            DescriptorHandle GetDescriptorHandle() const;

        private:
            Sampler() = default;
            void Shutdown() override;

            DescriptorHandle m_descriptor;
        };
    }
}
