/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceShaderResourceGroupPool.h>
#include <Atom/RHI/DeviceShaderResourceGroup.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace UnitTest
{
    class ShaderResourceGroup
        : public AZ::RHI::DeviceShaderResourceGroup
    {
    public:
        AZ_CLASS_ALLOCATOR(ShaderResourceGroup, AZ::SystemAllocator);
    };

    class ShaderResourceGroupPool
        : public AZ::RHI::DeviceShaderResourceGroupPool
    {
    public:
        AZ_CLASS_ALLOCATOR(ShaderResourceGroupPool, AZ::SystemAllocator);

    private:
        AZ::RHI::ResultCode InitInternal(AZ::RHI::Device&, const AZ::RHI::ShaderResourceGroupPoolDescriptor&) override;

        void ShutdownInternal() override;

        AZ::RHI::ResultCode InitGroupInternal(AZ::RHI::DeviceShaderResourceGroup& shaderResourceGroupBase) override;

        AZ::RHI::ResultCode CompileGroupInternal(
            AZ::RHI::DeviceShaderResourceGroup& groupBase,
            const AZ::RHI::DeviceShaderResourceGroupData& groupData) override;

        void ShutdownResourceInternal(AZ::RHI::DeviceResource& resourceBase) override;
    };
}
