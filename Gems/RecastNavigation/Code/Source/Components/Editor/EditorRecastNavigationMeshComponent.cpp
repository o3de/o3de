/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorRecastNavigationMeshComponent.h"
#include "EditorRecastNavigationMeshConfig.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Physics/Shape.h>
#include <AzQtComponents/Components/Widgets/FileDialog.h>
#include <Components/RecastNavigationMeshComponent.h>

namespace RecastNavigation
{
    void EditorRecastNavigationMeshComponent::Reflect(AZ::ReflectContext* context)
    {
        EditorRecastNavigationMeshConfig::Reflect(context);

        if (const auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<EditorRecastNavigationMeshComponent, AZ::Component>()
                ->Field("Configuration", &EditorRecastNavigationMeshComponent::m_meshConfig)
                ->Field("Debug Options", &EditorRecastNavigationMeshComponent::m_meshEditorConfig)
                ->Version(1)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<EditorRecastNavigationMeshComponent>("Recast Navigation Mesh",
                    "[Calculates the walkable navigation mesh within a specified area.]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(nullptr, &EditorRecastNavigationMeshComponent::m_meshConfig,
                        "Configuration", "Navigation Mesh configuration")
                    ->DataElement(nullptr, &EditorRecastNavigationMeshComponent::m_meshEditorConfig,
                        "Debug Options", "Various helper options for Editor viewport")
                    ;
            }
        }
    }

    void EditorRecastNavigationMeshComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("RecastNavigationMeshComponent"));
    }

    void EditorRecastNavigationMeshComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("RecastNavigationMeshComponent"));
    }

    void EditorRecastNavigationMeshComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("RecastNavigationSurveyorService"));
    }

    void EditorRecastNavigationMeshComponent::Activate()
    {
        EditorComponentBase::Activate();
    }

    void EditorRecastNavigationMeshComponent::Deactivate()
    {
        EditorComponentBase::Deactivate();
    }

    void EditorRecastNavigationMeshComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        gameEntity->CreateComponent<RecastNavigationMeshComponent>(m_meshConfig, m_meshEditorConfig.m_showNavigationMesh);
    }
} // namespace RecastNavigation
