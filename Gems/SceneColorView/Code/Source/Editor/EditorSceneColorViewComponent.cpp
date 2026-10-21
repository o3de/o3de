/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: MIT
 */

#include "EditorSceneColorViewComponent.h"
#include "SceneColorViewComponent.h"
#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace SceneColorView
{
    void EditorSceneColorViewComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto* sc = azrtti_cast<AZ::SerializeContext*>(context))
        {
            sc->Class<EditorSceneColorViewComponent, AzToolsFramework::Components::EditorComponentBase>()
                ->Version(0)->Field("Configuration", &EditorSceneColorViewComponent::m_configuration);
            if (auto* ec = sc->GetEditContext())
            {
                ec->Class<EditorSceneColorViewComponent>("Scene Color", "Optional scene-color capture through ViewSrg. Keep one settings entity per scene.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Rendering")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorSceneColorViewComponent::m_configuration,
                        "Capture", "Scene-wide capture settings")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);
            }
        }
    }
    void EditorSceneColorViewComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    { services.push_back(AZ_CRC_CE("SceneColorViewSettingsService")); }
    void EditorSceneColorViewComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    { services.push_back(AZ_CRC_CE("SceneColorViewSettingsService")); }
    void EditorSceneColorViewComponent::Activate()
    {
        EditorComponentBase::Activate();
        AZ::TickBus::Handler::BusConnect();
    }
    void EditorSceneColorViewComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
        ReleaseSettingsForEntity(GetEntityId());
        EditorComponentBase::Deactivate();
    }
    void EditorSceneColorViewComponent::OnTick(float, AZ::ScriptTimePoint)
    {
        if (!ApplySettingsForEntity(GetEntityId(), m_configuration) && !m_warned)
        {
            AZ_Warning("SceneColorView", false, "A different settings entity already controls scene-color capture in this scene.");
            m_warned = true;
        }
    }
    void EditorSceneColorViewComponent::BuildGameEntity(AZ::Entity* entity)
    {
        auto* component = entity->CreateComponent<SceneColorViewComponent>();
        component->SetConfiguration(m_configuration);
    }
}
