/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzFramework/Spawnable/Spawnable.h>
#include <AzFramework/Spawnable/SpawnableEntitiesInterface.h>

namespace Multiplayer
{
    //! @class NetBindMarkerComponent
    //! @brief Component for tracking net entities in the original non-networked spawnable.
    class NetBindMarkerComponent final : public AZ::Component
    {
    public:
        AZ_COMPONENT(NetBindMarkerComponent, "{40612C1B-427D-45C6-A2F0-04E16DF5B718}");

        static void Reflect(AZ::ReflectContext* context);

        NetBindMarkerComponent() = default;
        ~NetBindMarkerComponent() override = default;

        //! AZ::Component overrides.
        //! @{
        void Activate() override;
        void Deactivate() override;
        //! @}

        size_t GetNetEntityIndex() const;
        void SetNetEntityIndex(size_t val);

        void SetNetworkSpawnableAsset(AZ::Data::Asset<AzFramework::Spawnable> networkSpawnableAsset);
        AZ::Data::Asset<AzFramework::Spawnable> GetNetworkSpawnableAsset() const;

    private:
        AZ::Data::Asset<AzFramework::Spawnable> m_networkSpawnableAsset{AZ::Data::AssetLoadBehavior::PreLoad};
        size_t m_netEntityIndex = 0;
        AzFramework::EntitySpawnTicket m_netSpawnTicket;
    };
} // namespace Multiplayer
