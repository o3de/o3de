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
#include <Atom/RHI.Reflect/WebGPU/ShaderStageFunction.h>

namespace AZ::WebGPU
{
    class Device;

    class ShaderModule final
        : public RHI::DeviceObject
    {
        using Base = RHI::DeviceObject;

    public:
        AZ_CLASS_ALLOCATOR(ShaderModule, AZ::ThreadPoolAllocator);
        AZ_RTTI(ShaderModule, "{340EC499-D68B-4E81-AB87-D336B1ECBA2C}", Base);

        struct Descriptor
        {
            const ShaderStageFunction* m_shaderFunction = nullptr;
            RHI::ShaderStage m_shaderStage = RHI::ShaderStage::Unknown;
        };

        static RHI::Ptr<ShaderModule> Create();
        RHI::ResultCode Init(Device& device, const Descriptor& descriptor);
        ~ShaderModule() = default;

        const wgpu::ShaderModule& GetNativeShaderModule() const;
        const AZStd::string& GetEntryFunctionName() const;
        const ShaderStageFunction* GetStageFunction() const;

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
        //! Native shader module
        wgpu::ShaderModule m_wgpuShaderModule = nullptr;
        //! Entry function name
        AZStd::string m_entryFunctionName;
    };
}
