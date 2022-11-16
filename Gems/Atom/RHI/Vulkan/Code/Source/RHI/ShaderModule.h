/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceObject.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <Atom/RHI.Reflect/Vulkan/ShaderStageFunction.h>

namespace AZ
{
    namespace Vulkan
    {
        class Device;

        class ShaderModule final
            : public RHI::DeviceObject
        {
            using Base = RHI::DeviceObject;

        public:
            AZ_CLASS_ALLOCATOR(ShaderModule, AZ::ThreadPoolAllocator, 0);
            AZ_RTTI(ShaderModule, "FB682B11-D456-4151-AEE4-5D73A4C7B6F2", Base);

            struct Descriptor
            {
                Device* m_device = nullptr;
                ShaderByteCodeView m_bytecode;
                AZStd::string m_entryFunctionName;
                RHI::ShaderStage m_shaderStage = RHI::ShaderStage::Unknown;
                uint32_t m_shaderSubStage = 0;
            };

            static RHI::Ptr<ShaderModule> Create();
            RHI::ResultCode Init(const Descriptor& descriptor);
            ~ShaderModule() = default;

            VkShaderModule GetNativeShaderModule() const;
            const AZStd::string& GetEntryFunctionName() const;

        private:
            ShaderModule() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::Object
            void SetNameInternal(const AZStd::string_view& name) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceObject
            void Shutdown() override;
            //////////////////////////////////////////////////////////////////////////

            Descriptor m_descriptor;
            AZStd::vector<uint32_t> m_alignedByteCode;
            VkShaderModule m_nativeShaderModule = VK_NULL_HANDLE;
        };
    }
}
