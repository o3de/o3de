/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <MultiplayerToolsModule.h>
#include <Pipeline/NetworkPrefabProcessor.h>

#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <Prefab/Instance/InstanceSerializer.h>

namespace Multiplayer
{

    void MultiplayerToolsSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        NetworkPrefabProcessor::Reflect(context);
    }

    void MultiplayerToolsSystemComponent::Activate()
    {
        AZ::Interface<IMultiplayerTools>::Register(this);
    }

    void MultiplayerToolsSystemComponent::Deactivate()
    {
        AZ::Interface<IMultiplayerTools>::Unregister(this);
    }

    bool MultiplayerToolsSystemComponent::DidProcessNetworkPrefabs()
    {
        return m_didProcessNetPrefabs;
    }

    void MultiplayerToolsSystemComponent::SetDidProcessNetworkPrefabs(bool didProcessNetPrefabs)
    {
        m_didProcessNetPrefabs = didProcessNetPrefabs;
    }

    MultiplayerToolsModule::MultiplayerToolsModule()
        : AZ::Module()
    {
        m_descriptors.insert(m_descriptors.end(), {
            MultiplayerToolsSystemComponent::CreateDescriptor(),
        });
    }

    AZ::ComponentTypeList MultiplayerToolsModule::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList
        {
            azrtti_typeid<MultiplayerToolsSystemComponent>(),
        };
    }
} // namespace Multiplayer

AZ_DECLARE_MODULE_CLASS(Gem_Multiplayer_Tools, Multiplayer::MultiplayerToolsModule);
