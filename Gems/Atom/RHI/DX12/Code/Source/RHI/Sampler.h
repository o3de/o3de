/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
            AZ_CLASS_ALLOCATOR(Sampler, AZ::ThreadPoolAllocator, 0);
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