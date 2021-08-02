/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>

namespace Multiplayer
{
    //! @class IMultiplayerTools
    //! @brief IMultiplayerTools provides interfacing between the Editor and Multiplayer Gem.
    //!
    //! IMultiplayerTools is an AZ::Interface<T> that provides information about
    //! O3DE Editor and Tools integrations with the Multiplayer Gem. 
    class IMultiplayerTools
    {
    public:
        // NetworkPrefabProcessor is the only class that should be setting process network prefab status
        friend class NetworkPrefabProcessor;

        AZ_RTTI(IMultiplayerTools, "{E8A80EAB-29CB-4E3B-A0B2-FFCB37060FB0}");

        virtual ~IMultiplayerTools() = default;

        //! @brief Whether or not network prefab processing has created active or pending spawnable prefabs.
        //! @return `true` if network prefab processing has created currently active or pending spawnables.
        virtual bool DidProcessNetworkPrefabs() = 0;

    private:
        //! Sets if network prefab processing has created currently active or pending spawnables
        //! @param  didProcessNetPrefabs if network prefab processing has created currently active or pending spawnables
        virtual void SetDidProcessNetworkPrefabs(bool didProcessNetPrefabs) = 0;
    };
}
