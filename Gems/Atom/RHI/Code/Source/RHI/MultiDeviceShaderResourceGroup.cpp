/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/SingleDeviceBufferView.h>
#include <Atom/RHI/SingleDeviceImageView.h>
#include <Atom/RHI/MultiDeviceShaderResourceGroup.h>
#include <Atom/RHI/MultiDeviceShaderResourceGroupPool.h>

namespace AZ::RHI
{
    void MultiDeviceShaderResourceGroup::Compile(
        const MultiDeviceShaderResourceGroupData& groupData, CompileMode compileMode /*= CompileMode::Async*/)
    {
        m_mdData = groupData;

        IterateObjects<SingleDeviceShaderResourceGroup>([&groupData, compileMode](auto deviceIndex, auto deviceShaderResourceGroup)
        {
            deviceShaderResourceGroup->Compile(groupData.GetDeviceShaderResourceGroupData(deviceIndex), compileMode);
        });
    }

    uint32_t MultiDeviceShaderResourceGroup::GetBindingSlot() const
    {
        return m_bindingSlot;
    }

    bool MultiDeviceShaderResourceGroup::IsQueuedForCompile() const
    {
        return IterateObjects<SingleDeviceShaderResourceGroup>([]([[maybe_unused]] auto deviceIndex, auto deviceShaderResourceGroup)
        {
            if (deviceShaderResourceGroup->IsQueuedForCompile())
            {
                return ResultCode::Fail;
            }

            return ResultCode::Success;
        }) == ResultCode::Fail;
    }

    const MultiDeviceShaderResourceGroupPool* MultiDeviceShaderResourceGroup::GetPool() const
    {
        return static_cast<const MultiDeviceShaderResourceGroupPool*>(MultiDeviceResource::GetPool());
    }

    MultiDeviceShaderResourceGroupPool* MultiDeviceShaderResourceGroup::GetPool()
    {
        return static_cast<MultiDeviceShaderResourceGroupPool*>(MultiDeviceResource::GetPool());
    }

    const MultiDeviceShaderResourceGroupData& MultiDeviceShaderResourceGroup::GetData() const
    {
        return m_mdData;
    }

    void MultiDeviceShaderResourceGroup::Shutdown()
    {
        IterateObjects<SingleDeviceShaderResourceGroup>([]([[maybe_unused]] auto deviceIndex, auto deviceShaderResourceGroup)
        {
            deviceShaderResourceGroup->Shutdown();
        });

        MultiDeviceResource::Shutdown();
    }
} // namespace AZ::RHI
