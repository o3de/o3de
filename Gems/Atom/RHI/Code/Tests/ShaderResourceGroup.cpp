/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Tests/ShaderResourceGroup.h>

namespace UnitTest
{
    using namespace AZ;

    RHI::ResultCode ShaderResourceGroupPool::InitInternal(RHI::Device&, const RHI::ShaderResourceGroupPoolDescriptor&)
    {
        return RHI::ResultCode::Success;
    }

    void ShaderResourceGroupPool::ShutdownInternal()
    {
    }

    RHI::ResultCode ShaderResourceGroupPool::InitGroupInternal(RHI::DeviceShaderResourceGroup&)
    {
        return RHI::ResultCode::Success;
    }

    void ShaderResourceGroupPool::ShutdownResourceInternal(RHI::DeviceResource&)
    {
    }

    RHI::ResultCode ShaderResourceGroupPool::CompileGroupInternal(RHI::DeviceShaderResourceGroup&, const RHI::DeviceShaderResourceGroupData&)
    {
        return RHI::ResultCode::Success;
    }
}
