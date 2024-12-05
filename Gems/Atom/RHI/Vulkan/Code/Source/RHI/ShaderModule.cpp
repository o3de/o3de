/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI.Reflect/Vulkan/Conversion.h>
#include <RHI/Device.h>
#include <RHI/ShaderModule.h>
#include <Atom/RHI.Reflect/VkAllocator.h>

namespace AZ
{
    namespace Vulkan
    {
        RHI::Ptr<ShaderModule> ShaderModule::Create()
        {
            return aznew ShaderModule();
        }

        RHI::ResultCode ShaderModule::Init(const Descriptor& descriptor)
        {
            AZ_Assert(descriptor.m_device, "Device is null.");
            AZ_Assert(!descriptor.m_bytecode.empty(), "Shader bytecode is empty.");
            Base::Init(*descriptor.m_device);

            m_descriptor = descriptor;
            const size_t alignedByteCodeSize = (descriptor.m_bytecode.size() + 3) / 4;
            // pCode is declared as uint32_t*, and remained bytes should be padded by 0.
            AZStd::vector<uint32_t> alignedByteCode(alignedByteCodeSize, 0);
            memcpy(alignedByteCode.data(), descriptor.m_bytecode.data(), descriptor.m_bytecode.size());

            VkShaderModuleCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            createInfo.pNext = nullptr;
            createInfo.flags = 0;
            createInfo.codeSize = descriptor.m_bytecode.size();
            createInfo.pCode = alignedByteCode.data();

            const VkResult result =
                static_cast<Device&>(GetDevice())
                    .GetContext()
                    .CreateShaderModule(
                        descriptor.m_device->GetNativeDevice(), &createInfo, VkSystemAllocator::Get(), &m_nativeShaderModule);
            AssertSuccess(result);

            RETURN_RESULT_IF_UNSUCCESSFUL(ConvertResult(result));

            SetName(GetName());
            return RHI::ResultCode::Success;
        }

        VkShaderModule ShaderModule::GetNativeShaderModule() const
        {
            return m_nativeShaderModule;
        }

        const AZStd::string& ShaderModule::GetEntryFunctionName() const
        {
            return m_descriptor.m_entryFunctionName;
        }

        void ShaderModule::SetNameInternal(const AZStd::string_view& name)
        {
            if (IsInitialized() && !name.empty())
            {
                Debug::SetNameToObject(reinterpret_cast<uint64_t>(m_nativeShaderModule), name.data(), VK_OBJECT_TYPE_SHADER_MODULE, static_cast<Device&>(GetDevice()));
            }
        }

        void ShaderModule::Shutdown()
        {
            if (m_nativeShaderModule != VK_NULL_HANDLE)
            {
                auto& device = static_cast<Device&>(GetDevice());
                device.GetContext().DestroyShaderModule(device.GetNativeDevice(), m_nativeShaderModule, VkSystemAllocator::Get());
                m_nativeShaderModule = VK_NULL_HANDLE;
            }

            Base::Shutdown();
        }
    }
}
