/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Multiplayer/Components/MultiplayerComponentRegistry.h>

namespace Multiplayer
{
    NetComponentId MultiplayerComponentRegistry::RegisterMultiplayerComponent(const ComponentData& componentData)
    {
        NetComponentId netComponentId = m_nextNetComponentId++;
        m_componentData[netComponentId] = componentData;
        return netComponentId;
    }

    AZStd::unique_ptr<IMultiplayerComponentInput> MultiplayerComponentRegistry::AllocateComponentInput(NetComponentId netComponentId)
    {
        const ComponentData& componentData = GetMultiplayerComponentData(netComponentId);
        return AZStd::move(componentData.m_allocComponentInputFunction());
    }

    const char* MultiplayerComponentRegistry::GetComponentGemName(NetComponentId netComponentId) const
    {
        const ComponentData& componentData = GetMultiplayerComponentData(netComponentId);
        return componentData.m_gemName.GetCStr();
    }

    const char* MultiplayerComponentRegistry::GetComponentName(NetComponentId netComponentId) const
    {
        const ComponentData& componentData = GetMultiplayerComponentData(netComponentId);
        return componentData.m_componentName.GetCStr();
    }

    const char* MultiplayerComponentRegistry::GetComponentPropertyName(NetComponentId netComponentId, PropertyIndex propertyIndex) const
    {
        const ComponentData& componentData = GetMultiplayerComponentData(netComponentId);
        return componentData.m_componentPropertyNameLookupFunction(propertyIndex);
    }

    const char* MultiplayerComponentRegistry::GetComponentRpcName(NetComponentId netComponentId, RpcIndex rpcIndex) const
    {
        const ComponentData& componentData = GetMultiplayerComponentData(netComponentId);
        return componentData.m_componentRpcNameLookupFunction(rpcIndex);
    }

    const MultiplayerComponentRegistry::ComponentData& MultiplayerComponentRegistry::GetMultiplayerComponentData(NetComponentId netComponentId) const
    {
        static ComponentData nullComponentData;
        auto it = m_componentData.find(netComponentId);
        if (it != m_componentData.end())
        {
            return it->second;
        }
        return nullComponentData;
    }

    void MultiplayerComponentRegistry::Reset()
    {
        m_componentData.clear();
    }
}
