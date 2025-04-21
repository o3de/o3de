/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <MultiplayerToolsSystemComponent.h>
#include <AzNetworking/Framework/NetworkingSystemComponent.h>
#include <Multiplayer/Components/NetBindComponent.h>
#include <Multiplayer/Components/SimplePlayerSpawnerComponent.h>
#include <Source/MultiplayerGem.h>
#include <Source/MultiplayerSystemComponent.h>
#include <Source/MultiplayerStatSystemComponent.h>
#include <Source/AutoGen/AutoComponentTypes.h>

namespace Multiplayer
{
    MultiplayerModule::MultiplayerModule()
        : AZ::Module()
    {
        m_descriptors.insert(
            m_descriptors.end(),
            {
                MultiplayerSystemComponent::CreateDescriptor(),
                MultiplayerStatSystemComponent::CreateDescriptor(),
                NetBindComponent::CreateDescriptor(),
                SimplePlayerSpawnerComponent::CreateDescriptor(),
#ifdef MULTIPLAYER_EDITOR
                MultiplayerToolsSystemComponent::CreateDescriptor(),
#endif
            });

        CreateComponentDescriptors(m_descriptors);
    }

    AZ::ComponentTypeList MultiplayerModule::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList{
            azrtti_typeid<MultiplayerSystemComponent>(),
            azrtti_typeid<MultiplayerStatSystemComponent>(),
#ifdef MULTIPLAYER_EDITOR
            azrtti_typeid<MultiplayerToolsSystemComponent>(),
#endif
        };
    }
} // namespace Multiplayer

#if !defined(MULTIPLAYER_EDITOR)
#if defined(AZ_MONOLITHIC_BUILD)
    #if defined(O3DE_GEM_NAME)
    AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME, _Client), Multiplayer::MultiplayerModule)
    #else
    AZ_DECLARE_MODULE_CLASS(Gem_Multiplayer_Client, Multiplayer::MultiplayerModule)
    #endif
#if defined(O3DE_GEM_NAME)
    AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME, _Server), Multiplayer::MultiplayerModule)
    #else
    AZ_DECLARE_MODULE_CLASS(Gem_Multiplayer_Server, Multiplayer::MultiplayerModule)
    #endif
#endif
#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), Multiplayer::MultiplayerModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_Multiplayer, Multiplayer::MultiplayerModule)
#endif
#endif
