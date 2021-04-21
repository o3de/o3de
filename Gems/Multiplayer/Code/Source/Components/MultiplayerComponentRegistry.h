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

#pragma once

#include <AzCore/Name/Name.h>
#include <AzCore/std/containers/unordered_map.h>
#include <Source/Components/MultiplayerComponent.h>

namespace Multiplayer
{
    class MultiplayerComponentRegistry
    {
    public:
        using NameLookupFunction = AZStd::function<const char*(uint16_t index)>;
        struct ComponentData
        {
            AZ::Name m_gemName;
            AZ::Name m_componentName;
            NameLookupFunction m_componentPropertyNameLookupFunction;
            NameLookupFunction m_componentRpcNameLookupFunction;
        };

        //! Registers a multiplayer component with the multiplayer system.
        //! @param  componentData the data associated with the component being registered
        //! @return the NetComponentId assigned to this particular component
        NetComponentId RegisterMultiplayerComponent(const ComponentData& componentData);

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
        const char* GetComponentPropertyName(NetComponentId netComponentId, uint16_t propertyIndex) const;

        //! Returns the Rpc name associated with the provided NetComponentId and rpcId.
        //! @param  netComponentId the NetComponentId to return the property name of
        //! @param  rpcId          the index off the rpc to return the rpc name of
        //! @return the name of the requested rpc
        const char* GetComponentRpcName(NetComponentId netComponentId, uint16_t rpcId) const;

        //! Retrieves the stored component data for a given NetComponentId.
        //! @param  netComponentId the NetComponentId to return component data for
        //! @return reference to the requested component data, an empty container will be returned if the NetComponentId does not exist
        const ComponentData& GetMultiplayerComponentData(NetComponentId netComponentId) const;

    private:
        NetComponentId m_nextNetComponentId = NetComponentId{ 0 };
        AZStd::unordered_map<NetComponentId, ComponentData> m_componentData;
    };
}
