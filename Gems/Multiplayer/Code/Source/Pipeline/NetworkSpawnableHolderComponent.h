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

#include <AzCore/Component/Component.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzFramework/Spawnable/Spawnable.h>

namespace Multiplayer
{
    //! @class NetworkSpawnableHolderComponent
    //! @brief Component for holding a reference to the network spawnable to make sure it is loaded with the original one.
    class NetworkSpawnableHolderComponent final : public AZ::Component
    {
    public:
        AZ_COMPONENT(NetworkSpawnableHolderComponent, "{B0E3ADEE-FCB4-4A32-8D4F-6920F1CB08E4}");

        static void Reflect(AZ::ReflectContext* context);

        NetworkSpawnableHolderComponent();;
        ~NetworkSpawnableHolderComponent() override = default;

        //! AZ::Component overrides.
        //! @{
        void Activate() override;
        void Deactivate() override;
        //! @}

        void SetNetworkSpawnableAsset(AZ::Data::Asset<AzFramework::Spawnable> networkSpawnableAsset);
        AZ::Data::Asset<AzFramework::Spawnable> GetNetworkSpawnableAsset();

    private:
        AZ::Data::Asset<AzFramework::Spawnable> m_networkSpawnableAsset{ AZ::Data::AssetLoadBehavior::PreLoad };
    };
} // namespace Multiplayer
