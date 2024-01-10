/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/SingleDeviceShaderResourceGroupPool.h>
#include <Atom/RHI/SingleDeviceShaderResourceGroup.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace UnitTest
{
    class ShaderResourceGroup
        : public AZ::RHI::SingleDeviceShaderResourceGroup
    {
    public:
        AZ_CLASS_ALLOCATOR(ShaderResourceGroup, AZ::SystemAllocator);
    };

    class ShaderResourceGroupPool
        : public AZ::RHI::SingleDeviceShaderResourceGroupPool
    {
    public:
        AZ_CLASS_ALLOCATOR(ShaderResourceGroupPool, AZ::SystemAllocator);

    private:
        AZ::RHI::ResultCode InitInternal(AZ::RHI::Device&, const AZ::RHI::ShaderResourceGroupPoolDescriptor&) override;

        void ShutdownInternal() override;

        AZ::RHI::ResultCode InitGroupInternal(AZ::RHI::SingleDeviceShaderResourceGroup& shaderResourceGroupBase) override;

        AZ::RHI::ResultCode CompileGroupInternal(
            AZ::RHI::SingleDeviceShaderResourceGroup& groupBase,
            const AZ::RHI::SingleDeviceShaderResourceGroupData& groupData) override;

        void ShutdownResourceInternal(AZ::RHI::SingleDeviceResource& resourceBase) override;
    };
}
