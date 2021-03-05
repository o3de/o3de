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
#include <Atom/RHI/ShaderResourceGroupInvalidateRegistry.h>
#include <Atom/RHI/ShaderResourceGroup.h>
#include <AzCore/Debug/EventTrace.h>

namespace AZ
{
    namespace RHI
    {
        void ShaderResourceGroupInvalidateRegistry::SetCompileGroupFunction(CompileGroupFunction compileGroupFunction)
        {
            m_compileGroupFunction = compileGroupFunction;
        }

        void ShaderResourceGroupInvalidateRegistry::OnAttach(const Resource* resource, ShaderResourceGroup* shaderResourceGroup)
        {
            RefCountType &refcount = m_resourceToRegistryMap[resource][shaderResourceGroup];
            if (refcount == 0)
            {
                ResourceInvalidateBus::MultiHandler::BusConnect(resource);
            }
            ++refcount;
        }

        void ShaderResourceGroupInvalidateRegistry::OnDetach(const Resource* resource, ShaderResourceGroup* shaderResourceGroup)
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
            AZ_TRACE_METHOD();
            AZ_Assert(m_compileGroupFunction, "No compile function set");
            const Resource* resource = *ResourceInvalidateBus::GetCurrentBusId();

            Registry& registry = m_resourceToRegistryMap[resource];
            AZ_Assert(registry.empty() == false, "Registry should not be empty.");
            for (const auto& keyValue : registry)
            {
                m_compileGroupFunction(*keyValue.first);
            }

            return ResultCode::Success;
        }
    }
}