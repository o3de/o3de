/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorRecastNavigationPhysXProviderComponent.h"

#include <AzCore/Serialization/EditContext.h>
#include <Components/RecastNavigationPhysXProviderComponent.h>

namespace RecastNavigation
{
    void EditorRecastNavigationPhysXProviderComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<EditorRecastNavigationPhysXProviderComponent, AZ::Component>()
                ->Field("Show Input Data", &EditorRecastNavigationPhysXProviderComponent::m_debugDrawInputData)
                ->Version(1)
                ;

            if (AZ::EditContext* editContext = serialize->GetEditContext())
            {
                editContext->Class<EditorRecastNavigationPhysXProviderComponent>("Recast Navigation PhysX Provider",
                    "[Collects triangle geometry from PhysX scene for navigation mesh within the area defined by a shape component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(nullptr, &EditorRecastNavigationPhysXProviderComponent::m_debugDrawInputData, "Show Input Data",
                        "If enabled, debug draw is enabled to show the triangles collected in the Editor scene for the navigation mesh")
                    ;
            }
        }
    }

    void EditorRecastNavigationPhysXProviderComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        // This can be used to depend on this specific component.
        provided.push_back(AZ_CRC_CE("RecastNavigationPhysXProviderComponent"));
        // Or be able to satisfy requirements of @RecastNavigationMeshComponent, as one of geometry data providers for the navigation mesh.
        provided.push_back(AZ_CRC_CE("RecastNavigationProviderService"));
    }

    void EditorRecastNavigationPhysXProviderComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("RecastNavigationPhysXProviderComponent"));
        incompatible.push_back(AZ_CRC_CE("RecastNavigationProviderService"));
    }

    void EditorRecastNavigationPhysXProviderComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("AxisAlignedBoxShapeService"));
    }

    void EditorRecastNavigationPhysXProviderComponent::Activate()
    {
        EditorComponentBase::Activate();
    }

    void EditorRecastNavigationPhysXProviderComponent::Deactivate()
    {
        EditorComponentBase::Deactivate();
    }

    void EditorRecastNavigationPhysXProviderComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        gameEntity->CreateComponent<RecastNavigationPhysXProviderComponent>(m_debugDrawInputData);
    }
} // namespace RecastNavigation
