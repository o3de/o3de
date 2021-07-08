/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/ShaderResourceGroupPool.h>
#include <Atom/RHI/ShaderResourceGroup.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace UnitTest
{
    class ShaderResourceGroup
        : public AZ::RHI::ShaderResourceGroup
    {
    public:
        AZ_CLASS_ALLOCATOR(ShaderResourceGroup, AZ::SystemAllocator, 0);
    };

    class ShaderResourceGroupPool
        : public AZ::RHI::ShaderResourceGroupPool
    {
    public:
        AZ_CLASS_ALLOCATOR(ShaderResourceGroupPool, AZ::SystemAllocator, 0);

    private:
        AZ::RHI::ResultCode InitInternal(AZ::RHI::Device&, const AZ::RHI::ShaderResourceGroupPoolDescriptor&) override;

        void ShutdownInternal() override;

        AZ::RHI::ResultCode InitGroupInternal(AZ::RHI::ShaderResourceGroup& shaderResourceGroupBase) override;

        AZ::RHI::ResultCode CompileGroupInternal(
            AZ::RHI::ShaderResourceGroup& groupBase,
            const AZ::RHI::ShaderResourceGroupData& groupData) override;

        void ShutdownResourceInternal(AZ::RHI::Resource& resourceBase) override;
    };
}
