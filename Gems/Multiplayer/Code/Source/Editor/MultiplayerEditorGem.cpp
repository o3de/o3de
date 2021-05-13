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
#include <Source/MultiplayerGem.h>
#include <Source/MultiplayerSystemComponent.h>
#include <Source/Editor/MultiplayerEditorGem.h>
#include <AzNetworking/Framework/NetworkingSystemComponent.h>

#include <Source/Editor/MultiplayerEditorSystemComponent.h>

namespace Multiplayer
{
    AZ_CLASS_ALLOCATOR_IMPL(MultiplayerEditorModule, AZ::SystemAllocator, 0)

    MultiplayerEditorModule::MultiplayerEditorModule()
        : MultiplayerModule()
    {
        // Append Editor specific descriptors
        m_descriptors.insert(
            m_descriptors.end(),
            {
                MultiplayerEditorSystemComponent::CreateDescriptor(),
            });
    }

    MultiplayerEditorModule::~MultiplayerEditorModule() = default;

    AZ::ComponentTypeList MultiplayerEditorModule::GetRequiredSystemComponents() const
    {
        AZ::ComponentTypeList requiredComponents = MultiplayerModule::GetRequiredSystemComponents();
        requiredComponents.push_back(azrtti_typeid<MultiplayerEditorSystemComponent>());
        return requiredComponents;
    }
} // namespace Multiplayer

AZ_DECLARE_MODULE_CLASS(Gem_MultiplayerEditor, Multiplayer::MultiplayerEditorModule)
