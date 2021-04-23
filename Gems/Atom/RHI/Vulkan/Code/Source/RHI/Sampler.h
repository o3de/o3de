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
#include <Atom/RHI/DeviceObject.h>

namespace AZ
{
    namespace RHI
    {
        class SamplerState;
    }

    namespace Vulkan
    {
        class Device;

        class Sampler final
            : public RHI::DeviceObject
        {
            using Base = RHI::DeviceObject;

        public:
            AZ_CLASS_ALLOCATOR(Sampler, AZ::SystemAllocator, 0);
            AZ_RTTI(Sampler, "1794C9F5-AC90-4483-8132-8B4949F78000", Base);

            struct Descriptor
            {
                size_t GetHash() const;

                Device* m_device = nullptr;
                RHI::SamplerState m_samplerState;
            };
            static RHI::Ptr<Sampler> Create();
            RHI::ResultCode Init(const Descriptor& descriptor);
            ~Sampler() = default;

            VkSampler GetNativeSampler() const;

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
            VkSampler m_nativeSampler = VK_NULL_HANDLE;
        };
    }
}
