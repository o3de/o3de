/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Name/Name.h>
#include <AzCore/std/containers/unordered_map.h>
#include <Multiplayer/Components/MultiplayerComponent.h>
#include <Multiplayer/NetworkInput/IMultiplayerComponentInput.h>

namespace Multiplayer
{
    class MultiplayerComponentRegistry
    {
    public:
        using PropertyNameLookupFunction = AZStd::function<const char*(PropertyIndex index)>;
        using RpcNameLookupFunction = AZStd::function<const char*(RpcIndex index)>;
        using AllocComponentInputFunction = AZStd::function<AZStd::unique_ptr<IMultiplayerComponentInput>()>;
        struct ComponentData
        {
            AZ::Name m_gemName;
            AZ::Name m_componentName;
            PropertyNameLookupFunction m_componentPropertyNameLookupFunction;
            RpcNameLookupFunction m_componentRpcNameLookupFunction;
            AllocComponentInputFunction m_allocComponentInputFunction;
        };

        //! Registers a multiplayer component with the multiplayer system.
        //! @param  componentData the data associated with the component being registered
        //! @return the NetComponentId assigned to this particular component
        NetComponentId RegisterMultiplayerComponent(const ComponentData& componentData);

        //! Allocates a new component input for the provided netComponentId.
        //! @param  netComponentId the NetComponentId to allocate a component input for
        //! @return pointer to the allocated component input, caller assumes ownership
        AZStd::unique_ptr<IMultiplayerComponentInput> AllocateComponentInput(NetComponentId netComponentId);

        //! Returns the gem name associated with the provided NetComponentId.
        //! @param  netComponentId the NetComponentId to return the gem name of
        //! @return the name of the gem that contains the requested component
        const char* GetComponentGemName(NetComponentId netComponentId) const;

        //! Returns the component name associated with the provided NetComponentId.
        //! @param  netComponentId the NetComponentId to return the component name of
        //! @return the name of the component
        const char* GetComponentName(NetComponentId netComponentId) const;

        //! Returns the property name associated with the provided NetComponentId and propertyIndex.
        //! @param  netComponentId the NetComponentId to return the property name of
        //! @param  propertyIndex  the index off the network property to return the property name of
        //! @return the name of the network property
        const char* GetComponentPropertyName(NetComponentId netComponentId, PropertyIndex propertyIndex) const;

        //! Returns the Rpc name associated with the provided NetComponentId and rpcId.
        //! @param  netComponentId the NetComponentId to return the property name of
        //! @param  rpcIndex       the index of the rpc to return the rpc name of
        //! @return the name of the requested rpc
        const char* GetComponentRpcName(NetComponentId netComponentId, RpcIndex rpcIndex) const;

        //! Retrieves the stored component data for a given NetComponentId.
        //! @param  netComponentId the NetComponentId to return component data for
        //! @return reference to the requested component data, an empty container will be returned if the NetComponentId does not exist
        const ComponentData& GetMultiplayerComponentData(NetComponentId netComponentId) const;

        //! This releases all owned memory, should only be called during multiplayer shutdown.
        void Reset();

    private:
        NetComponentId m_nextNetComponentId = NetComponentId{ 0 };
        AZStd::unordered_map<NetComponentId, ComponentData> m_componentData;
    };
}
