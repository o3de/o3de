/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/BufferView.h>
#include <Atom/RHI/ImageView.h>
#include <Atom/RHI/MultiDeviceShaderResourceGroup.h>
#include <Atom/RHI/MultiDeviceShaderResourceGroupPool.h>

namespace AZ::RHI
{
    void MultiDeviceShaderResourceGroup::Compile(
        const MultiDeviceShaderResourceGroupData& groupData, CompileMode compileMode /*= CompileMode::Async*/)
    {
        IterateObjects<ShaderResourceGroup>([&groupData, compileMode](auto deviceIndex, auto deviceShaderResourceGroup)
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
        return IterateObjects<ShaderResourceGroup>([]([[maybe_unused]] auto deviceIndex, auto deviceShaderResourceGroup)
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

    void MultiDeviceShaderResourceGroup::DisableCompilationForAllResourceTypes()
    {
        IterateObjects<ShaderResourceGroup>([]([[maybe_unused]] auto deviceIndex, auto deviceShaderResourceGroup)
        {
            deviceShaderResourceGroup->DisableCompilationForAllResourceTypes();
        });
    }

    bool MultiDeviceShaderResourceGroup::IsResourceTypeEnabledForCompilation(uint32_t resourceTypeMask) const
    {
        return IterateObjects<ShaderResourceGroup>([resourceTypeMask]([[maybe_unused]] auto deviceIndex, auto deviceShaderResourceGroup)
        {
            if (deviceShaderResourceGroup->IsResourceTypeEnabledForCompilation(resourceTypeMask))
            {
                return ResultCode::Fail;
            }

            return ResultCode::Success;
        }) == ResultCode::Fail;
    }

    bool MultiDeviceShaderResourceGroup::IsAnyResourceTypeUpdated() const
    {
        return IterateObjects<ShaderResourceGroup>([]([[maybe_unused]] auto deviceIndex, auto deviceShaderResourceGroup)
        {
            if (deviceShaderResourceGroup->IsAnyResourceTypeUpdated())
            {
                return ResultCode::Fail;
            }

            return ResultCode::Success;
        }) == ResultCode::Fail;
    }

    void MultiDeviceShaderResourceGroup::EnableRhiResourceTypeCompilation(
        const MultiDeviceShaderResourceGroupData::ResourceTypeMask resourceTypeMask)
    {
        IterateObjects<ShaderResourceGroup>([resourceTypeMask]([[maybe_unused]] auto deviceIndex, auto deviceShaderResourceGroup)
        {
            deviceShaderResourceGroup->EnableRhiResourceTypeCompilation(resourceTypeMask);
        });
    }

    void MultiDeviceShaderResourceGroup::ResetResourceTypeIteration(const MultiDeviceShaderResourceGroupData::ResourceType resourceType)
    {
        IterateObjects<ShaderResourceGroup>([resourceType]([[maybe_unused]] auto deviceIndex, auto deviceShaderResourceGroup)
        {
            deviceShaderResourceGroup->ResetResourceTypeIteration(resourceType);
        });
    }

    HashValue64 MultiDeviceShaderResourceGroup::GetViewHash(const AZ::Name& viewName)
    {
        return m_viewHash[viewName];
    }

    void MultiDeviceShaderResourceGroup::UpdateViewHash(const AZ::Name& viewName, const HashValue64 viewHash)
    {
        IterateObjects<ShaderResourceGroup>([&viewName, viewHash]([[maybe_unused]] auto deviceIndex, auto deviceShaderResourceGroup)
        {
            deviceShaderResourceGroup->UpdateViewHash(viewName, viewHash);
        });

        m_viewHash[viewName] = viewHash;
    }

    void MultiDeviceShaderResourceGroup::Shutdown()
    {
        IterateObjects<ShaderResourceGroup>([]([[maybe_unused]] auto deviceIndex, auto deviceShaderResourceGroup)
        {
            deviceShaderResourceGroup->Shutdown();
        });

        MultiDeviceResource::Shutdown();
    }

    void MultiDeviceShaderResourceGroup::InvalidateViews()
    {
        IterateObjects<ShaderResourceGroup>([]([[maybe_unused]] auto deviceIndex, auto deviceShaderResourceGroup)
        {
            deviceShaderResourceGroup->InvalidateViews();
        });
    }
} // namespace AZ::RHI
