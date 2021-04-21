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

#include <Source/Components/MultiplayerComponentRegistry.h>

namespace Multiplayer
{
    NetComponentId MultiplayerComponentRegistry::RegisterMultiplayerComponent(const ComponentData& componentData)
    {
        NetComponentId netComponentId = m_nextNetComponentId++;
        m_componentData[netComponentId] = componentData;
        return netComponentId;
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

    const char* MultiplayerComponentRegistry::GetComponentPropertyName(NetComponentId netComponentId, uint16_t propertyIndex) const
    {
        const ComponentData& componentData = GetMultiplayerComponentData(netComponentId);
        return componentData.m_componentPropertyNameLookupFunction(propertyIndex);
    }

    const char* MultiplayerComponentRegistry::GetComponentRpcName(NetComponentId netComponentId, uint16_t rpcId) const
    {
        const ComponentData& componentData = GetMultiplayerComponentData(netComponentId);
        return componentData.m_componentRpcNameLookupFunction(rpcId);
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
}
