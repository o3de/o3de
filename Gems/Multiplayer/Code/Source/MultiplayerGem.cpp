/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/Multiplayer_precompiled.h>
#include <Source/MultiplayerGem.h>
#include <Source/MultiplayerSystemComponent.h>
#include <Source/AutoGen/AutoComponentTypes.h>
#include <Source/Pipeline/NetBindMarkerComponent.h>
#include <Source/Pipeline/NetworkSpawnableHolderComponent.h>
#include <Multiplayer/Components/NetBindComponent.h>
#include <AzNetworking/Framework/NetworkingSystemComponent.h>

namespace Multiplayer
{
    MultiplayerModule::MultiplayerModule()
        : AZ::Module()
    {
        m_descriptors.insert(m_descriptors.end(), {
            AzNetworking::NetworkingSystemComponent::CreateDescriptor(),
            MultiplayerSystemComponent::CreateDescriptor(),
            NetBindComponent::CreateDescriptor(),
            NetBindMarkerComponent::CreateDescriptor(),
            NetworkSpawnableHolderComponent::CreateDescriptor(),
        });

        CreateComponentDescriptors(m_descriptors);
    }

    AZ::ComponentTypeList MultiplayerModule::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList
        {
            azrtti_typeid<AzNetworking::NetworkingSystemComponent>(),
            azrtti_typeid<MultiplayerSystemComponent>(),
        };
    }
}

#if !defined(MULTIPLAYER_EDITOR)
AZ_DECLARE_MODULE_CLASS(Gem_Multiplayer, Multiplayer::MultiplayerModule);
#endif
