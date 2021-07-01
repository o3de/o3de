/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/Multiplayer_precompiled.h>
#include <Source/Debug/MultiplayerDebugModule.h>
#include <Source/Debug/MultiplayerDebugSystemComponent.h>

namespace Multiplayer
{
    MultiplayerDebugModule::MultiplayerDebugModule()
        : AZ::Module()
    {
        m_descriptors.insert(m_descriptors.end(), {
            MultiplayerDebugSystemComponent::CreateDescriptor(),
        });
    }

    AZ::ComponentTypeList MultiplayerDebugModule::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList
        {
            azrtti_typeid<MultiplayerDebugSystemComponent>(),
        };
    }
}

AZ_DECLARE_MODULE_CLASS(Gem_Multiplayer_Imgui, Multiplayer::MultiplayerDebugModule);
