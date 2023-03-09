/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom_RHI_Vulkan_Platform.h>
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
            AZ_CLASS_ALLOCATOR(Sampler, AZ::SystemAllocator);
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
