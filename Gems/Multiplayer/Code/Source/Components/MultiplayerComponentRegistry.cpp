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

        // add all the component hashes together to create an system-wide hash
        m_componentVersionHashes[componentData.m_componentName] = componentData.m_versionHash;
        m_systemVersionHash += componentData.m_versionHash;

        return netComponentId;
    }

    AZStd::unique_ptr<IMultiplayerComponentInput> MultiplayerComponentRegistry::AllocateComponentInput(NetComponentId netComponentId)
    {
        const ComponentData& componentData = GetMultiplayerComponentData(netComponentId);
        if (componentData.m_allocComponentInputFunction)
        {
            return AZStd::move(componentData.m_allocComponentInputFunction());
        }
        return nullptr;
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
        if (componentData.m_componentPropertyNameLookupFunction)
        {
            return componentData.m_componentPropertyNameLookupFunction(propertyIndex);
        }
        return "Unknown component";
    }

    const char* MultiplayerComponentRegistry::GetComponentRpcName(NetComponentId netComponentId, RpcIndex rpcIndex) const
    {
        const ComponentData& componentData = GetMultiplayerComponentData(netComponentId);
        if (componentData.m_componentRpcNameLookupFunction)
        {
            return componentData.m_componentRpcNameLookupFunction(rpcIndex);
        }
        return "Unknown component";
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

    AZ::HashValue64 MultiplayerComponentRegistry::GetSystemVersionHash() const
    {
        return m_systemVersionHash;
    }

    bool MultiplayerComponentRegistry::FindComponentVersionHashByName(const AZ::Name& multiplayerComponentName, AZ::HashValue64& hash) const
    {
        const auto it = m_componentVersionHashes.find(multiplayerComponentName);
        if (it != m_componentVersionHashes.end())
        {
            hash = it->second;
            return true;
        }

        return false;
    }

    const Multiplayer::ComponentVersionMap& MultiplayerComponentRegistry::GetMultiplayerComponentVersionHashes() const
    {
        return m_componentVersionHashes;
    }

    void MultiplayerComponentRegistry::Reset()
    {
        m_componentData.clear();
        m_componentVersionHashes.clear();
        m_systemVersionHash = AZ::HashValue64{ 0 };
    }
}
