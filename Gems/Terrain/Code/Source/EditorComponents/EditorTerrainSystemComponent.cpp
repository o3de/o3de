/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <EditorComponents/EditorTerrainSystemComponent.h>
#include <EntityPresets/TerrainPreset.h>
#include <TerrainRenderer/EditorComponents/EditorTerrainMacroMaterialComponent.h>

namespace Terrain
{
    void EditorTerrainSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        GradientSignal::ImageCreatorUtils::
            PaintableImageAssetHelper<EditorTerrainMacroMaterialComponent, EditorTerrainMacroMaterialComponentMode>::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorTerrainSystemComponent, AZ::Component>()->Version(1);
        }
    }

    void EditorTerrainSystemComponent::Activate()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
        AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler::BusConnect();
    }

    void EditorTerrainSystemComponent::Deactivate()
    {
        AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
    }

    void EditorTerrainSystemComponent::OnMenuRegistrationHook()
    {
        TerrainPreset::RegisterMenus();
    }

    void EditorTerrainSystemComponent::OnActionRegistrationHook()
    {
        TerrainPreset::RegisterActions();
    }

    void EditorTerrainSystemComponent::OnMenuBindingHook()
    {
        TerrainPreset::BindMenus();
    }

} // namespace Terrain
