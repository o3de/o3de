/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/BufferView.h>
#include <Atom/RHI/DeviceShaderResourceGroup.h>
#include <Atom/RHI/ImageView.h>
#include <Atom/RHI/ShaderResourceGroup.h>
#include <Atom/RHI/ShaderResourceGroupPool.h>

namespace AZ
{
    namespace RHI
    {
        void ShaderResourceGroup::Compile(const ShaderResourceGroupData& groupData, CompileMode compileMode /*= CompileMode::Async*/)
        {
            switch (compileMode)
            {
            case CompileMode::Async:
                for (auto& [deviceIndex, deviceShaderResourceGroup] : m_deviceShaderResourceGroups)
                {
                    deviceShaderResourceGroup->Compile(
                        groupData.GetDeviceShaderResourceGroupData(deviceIndex), DeviceShaderResourceGroup::CompileMode::Async);
                }

                break;
            case CompileMode::Sync:
                for (auto& [deviceIndex, deviceShaderResourceGroup] : m_deviceShaderResourceGroups)
                {
                    deviceShaderResourceGroup->Compile(
                        groupData.GetDeviceShaderResourceGroupData(deviceIndex), DeviceShaderResourceGroup::CompileMode::Sync);
                }

                break;
            default:
                AZ_Assert(false, "Invalid SRG Compile mode %d", compileMode);
                break;
            }
        }

        uint32_t ShaderResourceGroup::GetBindingSlot() const
        {
            return m_bindingSlot;
        }

        bool ShaderResourceGroup::IsQueuedForCompile() const
        {
            for (auto& [deviceIndex, deviceShaderResourceGroup] : m_deviceShaderResourceGroups)
            {
                if (deviceShaderResourceGroup->IsQueuedForCompile())
                    return true;
            }

            return false;
        }

        const ShaderResourceGroupPool* ShaderResourceGroup::GetPool() const
        {
            return static_cast<const ShaderResourceGroupPool*>(Resource::GetPool());
        }

        ShaderResourceGroupPool* ShaderResourceGroup::GetPool()
        {
            return static_cast<ShaderResourceGroupPool*>(Resource::GetPool());
        }

        const ShaderResourceGroupData& ShaderResourceGroup::GetData() const
        {
            return m_data;
        }

        void ShaderResourceGroup::SetData(const ShaderResourceGroupData& data)
        {
            m_data = data;
        }

        void ShaderResourceGroup::DisableCompilationForAllResourceTypes()
        {
            for (auto& [deviceIndex, deviceShaderResourceGroup] : m_deviceShaderResourceGroups)
            {
                deviceShaderResourceGroup->DisableCompilationForAllResourceTypes();
            }
        }
    
        bool ShaderResourceGroup::IsResourceTypeEnabledForCompilation(uint32_t resourceTypeMask) const
        {
            for (auto& [deviceIndex, deviceShaderResourceGroup] : m_deviceShaderResourceGroups)
            {
                if (deviceShaderResourceGroup->IsResourceTypeEnabledForCompilation(resourceTypeMask))
                    return true;
            }

            return false;
        }

        bool ShaderResourceGroup::IsAnyResourceTypeUpdated() const
        {
            for (auto& [deviceIndex, deviceShaderResourceGroup] : m_deviceShaderResourceGroups)
            {
                if (deviceShaderResourceGroup->IsAnyResourceTypeUpdated())
                    return true;
            }

            return false;
        }

        void ShaderResourceGroup::EnableRhiResourceTypeCompilation(const ShaderResourceGroupData::ResourceTypeMask resourceTypeMask)
        {
            for (auto& [deviceIndex, deviceShaderResourceGroup] : m_deviceShaderResourceGroups)
            {
                deviceShaderResourceGroup->EnableRhiResourceTypeCompilation(resourceTypeMask);
            }
        }

        void ShaderResourceGroup::ResetResourceTypeIteration(const ShaderResourceGroupData::ResourceType resourceType)
        {
            for (auto& [deviceIndex, deviceShaderResourceGroup] : m_deviceShaderResourceGroups)
            {
                deviceShaderResourceGroup->ResetResourceTypeIteration(resourceType);
            }
        }

        HashValue64 ShaderResourceGroup::GetViewHash(const AZ::Name& viewName)
        {
            return m_viewHash[viewName];
        }

        void ShaderResourceGroup::UpdateViewHash(const AZ::Name& viewName, const HashValue64 viewHash)
        {
            for (auto& [deviceIndex, deviceShaderResourceGroup] : m_deviceShaderResourceGroups)
            {
                deviceShaderResourceGroup->UpdateViewHash(viewName, viewHash);
            }

            m_viewHash[viewName] = viewHash;
        }
    }
}
