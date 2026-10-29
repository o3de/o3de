/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Translation/TranslationDef.h>

#include "EditorDebugDrawOctreeComponent.h"

namespace DebugDraw
{
    void EditorDebugDrawOctreeComponent::Reflect(AZ::ReflectContext* context)
    {
        // NOTE: EditorDebugDrawComponentSettings is reflected by DebugDrawSystemComponent::Reflect();
        // do not reflect it here as well or the duplicate Uuid registration will assert on module load.

        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<EditorDebugDrawOctreeComponent, AzToolsFramework::Components::EditorComponentBase>()
                ->Version(0)
                ->Field("Settings", &EditorDebugDrawOctreeComponent::m_settings)
                ;

            if (auto* editContext = serialize->GetEditContext())
            {
                editContext->Class<EditorDebugDrawOctreeComponent>(
                    QT_TRANSLATE_NOOP("DebugDraw", "DebugDraw Octree"),
                    QT_TRANSLATE_NOOP(
                        "DebugDraw",
                        "Draws the visibility octree node bounds (orange) and the bounding volume of every visibility "
                        "entry (cyan) every frame, also while the game is running. Toggle with the "
                        "bg_octreeRuntimeDebugDraw console variable."))
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Debugging")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Level"))
                    ->DataElement(nullptr, &EditorDebugDrawOctreeComponent::m_settings,
                        QT_TRANSLATE_NOOP("DebugDraw", "Visibility settings"),
                        QT_TRANSLATE_NOOP("DebugDraw", "Common settings for DebugDraw components."))
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorDebugDrawOctreeComponent::OnPropertyUpdate)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void EditorDebugDrawOctreeComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        DebugDrawOctreeComponent::GetProvidedServices(provided);
    }

    void EditorDebugDrawOctreeComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        DebugDrawOctreeComponent::GetIncompatibleServices(incompatible);
    }

    void EditorDebugDrawOctreeComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        DebugDrawOctreeComponent::GetRequiredServices(required);
    }

    void EditorDebugDrawOctreeComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        DebugDrawOctreeComponent::GetDependentServices(dependent);
    }

    void EditorDebugDrawOctreeComponent::Activate()
    {
        if (m_settings.m_visibleInEditor)
        {
            AZ::TickBus::Handler::BusConnect();
        }
    }

    void EditorDebugDrawOctreeComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
    }

    void EditorDebugDrawOctreeComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        DebugDrawOctreeComponent::DrawVisibilityOctree();
    }

    void EditorDebugDrawOctreeComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        if (m_settings.m_visibleInGame)
        {
            gameEntity->CreateComponent<DebugDrawOctreeComponent>();
        }
    }

    void EditorDebugDrawOctreeComponent::OnPropertyUpdate()
    {
        AZ::TickBus::Handler::BusDisconnect();

        if (m_settings.m_visibleInEditor)
        {
            AZ::TickBus::Handler::BusConnect();
        }
    }
} // namespace DebugDraw