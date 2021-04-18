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

#include <Source/Multiplayer_precompiled.h>
#include <Source/Imgui/MultiplayerImguiModule.h>
#include <Source/Imgui/MultiplayerImguiSystemComponent.h>

namespace Multiplayer
{
    MultiplayerImguiModule::MultiplayerImguiModule()
        : AZ::Module()
    {
        m_descriptors.insert(m_descriptors.end(), {
            MultiplayerImguiSystemComponent::CreateDescriptor(),
        });
    }

    AZ::ComponentTypeList MultiplayerImguiModule::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList
        {
            azrtti_typeid<MultiplayerImguiSystemComponent>(),
        };
    }
}

AZ_DECLARE_MODULE_CLASS(Gem_Multiplayer_Imgui, Multiplayer::MultiplayerImguiModule);
