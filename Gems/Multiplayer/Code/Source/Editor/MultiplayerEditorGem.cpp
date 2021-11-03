/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <MultiplayerGem.h>
#include <MultiplayerSystemComponent.h>
#include <Editor/MultiplayerEditorGem.h>
#include <Editor/MultiplayerEditorSystemComponent.h>

#include <AzNetworking/Framework/NetworkingSystemComponent.h>

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
                PythonEditorFuncs::CreateDescriptor()
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
