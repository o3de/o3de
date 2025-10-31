/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/ShaderResourceGroupInvalidateRegistry.h>
#include <Atom/RHI/DeviceShaderResourceGroup.h>
namespace AZ::RHI
{
    void ShaderResourceGroupInvalidateRegistry::SetCompileGroupFunction(CompileGroupFunction compileGroupFunction)
    {
        m_compileGroupFunction = compileGroupFunction;
    }

    void ShaderResourceGroupInvalidateRegistry::OnAttach(const DeviceResource* resource, DeviceShaderResourceGroup* shaderResourceGroup)
    {
        RefCountType &refcount = m_resourceToRegistryMap[resource][shaderResourceGroup];
        if (refcount == 0)
        {
            ResourceInvalidateBus::MultiHandler::BusConnect(resource);
        }
        ++refcount;
    }

    void ShaderResourceGroupInvalidateRegistry::OnDetach(const DeviceResource* resource, DeviceShaderResourceGroup* shaderResourceGroup)
    {
        auto outerIt = m_resourceToRegistryMap.find(resource);
        AZ_Assert(outerIt != m_resourceToRegistryMap.end(), "No SRG registry found.");

        Registry& registry = outerIt->second;
        auto innerIt = registry.find(shaderResourceGroup);
        AZ_Assert(innerIt != registry.end(), "No SRG found.");

        if (--innerIt->second == 0)
        {
            if (registry.size() > 1)
            {
                registry.erase(innerIt);
            }

            // Drop the whole registry if this is the last element.
            else
            {
                ResourceInvalidateBus::MultiHandler::BusDisconnect(resource);
                m_resourceToRegistryMap.erase(outerIt);
            }
        }
    }

    bool ShaderResourceGroupInvalidateRegistry::IsEmpty() const
    {
        return m_resourceToRegistryMap.empty();
    }

    ResultCode ShaderResourceGroupInvalidateRegistry::OnResourceInvalidate()
    {
        AZ_PROFILE_FUNCTION(RHI);
        AZ_Assert(m_compileGroupFunction, "No compile function set");
        const DeviceResource* resource = *ResourceInvalidateBus::GetCurrentBusId();

        Registry& registry = m_resourceToRegistryMap[resource];
        AZ_Assert(registry.empty() == false, "Registry should not be empty.");
        for (const auto& keyValue : registry)
        {
            m_compileGroupFunction(*keyValue.first);
        }

        return ResultCode::Success;
    }
}
