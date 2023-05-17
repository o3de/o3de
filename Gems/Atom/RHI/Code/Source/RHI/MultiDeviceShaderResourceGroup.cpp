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

namespace AZ
{
    namespace RHI
    {
        void MultiDeviceShaderResourceGroup::Compile(
            const MultiDeviceShaderResourceGroupData& groupData, CompileMode compileMode /*= CompileMode::Async*/)
        {
            for (auto& [deviceIndex, deviceShaderResourceGroup] : m_deviceShaderResourceGroups)
            {
                deviceShaderResourceGroup->Compile(groupData.GetDeviceShaderResourceGroupData(deviceIndex), compileMode);
            }
        }

        uint32_t MultiDeviceShaderResourceGroup::GetBindingSlot() const
        {
            return m_bindingSlot;
        }

        bool MultiDeviceShaderResourceGroup::IsQueuedForCompile() const
        {
            for (auto& [deviceIndex, deviceShaderResourceGroup] : m_deviceShaderResourceGroups)
            {
                if (deviceShaderResourceGroup->IsQueuedForCompile())
                    return true;
            }

            return false;
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
            return m_data;
        }

        void MultiDeviceShaderResourceGroup::DisableCompilationForAllResourceTypes()
        {
            for (auto& [deviceIndex, deviceShaderResourceGroup] : m_deviceShaderResourceGroups)
            {
                deviceShaderResourceGroup->DisableCompilationForAllResourceTypes();
            }
        }

        bool MultiDeviceShaderResourceGroup::IsResourceTypeEnabledForCompilation(uint32_t resourceTypeMask) const
        {
            for (auto& [deviceIndex, deviceShaderResourceGroup] : m_deviceShaderResourceGroups)
            {
                if (deviceShaderResourceGroup->IsResourceTypeEnabledForCompilation(resourceTypeMask))
                    return true;
            }

            return false;
        }

        bool MultiDeviceShaderResourceGroup::IsAnyResourceTypeUpdated() const
        {
            for (auto& [deviceIndex, deviceShaderResourceGroup] : m_deviceShaderResourceGroups)
            {
                if (deviceShaderResourceGroup->IsAnyResourceTypeUpdated())
                    return true;
            }

            return false;
        }

        void MultiDeviceShaderResourceGroup::EnableRhiResourceTypeCompilation(
            const MultiDeviceShaderResourceGroupData::ResourceTypeMask resourceTypeMask)
        {
            for (auto& [deviceIndex, deviceShaderResourceGroup] : m_deviceShaderResourceGroups)
            {
                deviceShaderResourceGroup->EnableRhiResourceTypeCompilation(resourceTypeMask);
            }
        }

        void MultiDeviceShaderResourceGroup::ResetResourceTypeIteration(const MultiDeviceShaderResourceGroupData::ResourceType resourceType)
        {
            for (auto& [deviceIndex, deviceShaderResourceGroup] : m_deviceShaderResourceGroups)
            {
                deviceShaderResourceGroup->ResetResourceTypeIteration(resourceType);
            }
        }

        HashValue64 MultiDeviceShaderResourceGroup::GetViewHash(const AZ::Name& viewName)
        {
            return m_viewHash[viewName];
        }

        void MultiDeviceShaderResourceGroup::UpdateViewHash(const AZ::Name& viewName, const HashValue64 viewHash)
        {
            for (auto& [deviceIndex, deviceShaderResourceGroup] : m_deviceShaderResourceGroups)
            {
                deviceShaderResourceGroup->UpdateViewHash(viewName, viewHash);
            }

            m_viewHash[viewName] = viewHash;
        }

        void MultiDeviceShaderResourceGroup::Shutdown()
        {
            for (auto& [_, deviceShaderResourceGroup] : m_deviceShaderResourceGroups)
                deviceShaderResourceGroup->Shutdown();
            MultiDeviceResource::Shutdown();
        }

        void MultiDeviceShaderResourceGroup::InvalidateViews()
        {
            for (auto& [_, deviceShaderResourceGroup] : m_deviceShaderResourceGroups)
                deviceShaderResourceGroup->InvalidateViews();
        }
    } // namespace RHI
} // namespace AZ
