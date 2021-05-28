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
