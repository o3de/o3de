/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/Debug/MultiplayerDebugModule.h>
#include <Source/Debug/MultiplayerDebugSystemComponent.h>
#include <Source/Debug/MultiplayerConnectionViewportMessageSystemComponent.h>

namespace Multiplayer
{
    MultiplayerDebugModule::MultiplayerDebugModule()
        : AZ::Module()
    {
        m_descriptors.insert(m_descriptors.end(), {
            MultiplayerDebugSystemComponent::CreateDescriptor(),
            MultiplayerConnectionViewportMessageSystemComponent::CreateDescriptor()
        });
    }

    AZ::ComponentTypeList MultiplayerDebugModule::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList
        {
            azrtti_typeid<MultiplayerDebugSystemComponent>(),
            azrtti_typeid<MultiplayerConnectionViewportMessageSystemComponent>()
        };
    }
}

#if defined(AZ_MONOLITHIC_BUILD)
AZ_DECLARE_MODULE_CLASS(Gem_Multiplayer_Debug_Client, Multiplayer::MultiplayerDebugModule);
#endif
AZ_DECLARE_MODULE_CLASS(Gem_Multiplayer_Debug, Multiplayer::MultiplayerDebugModule);
