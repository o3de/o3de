/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDetourNavigationComponent.h"

#include <AzCore/Serialization/EditContext.h>
#include <Components/DetourNavigationComponent.h>

namespace RecastNavigation
{
    void EditorDetourNavigationComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorDetourNavigationComponent, AZ::Component>()
                ->Field("Navigation Mesh", &EditorDetourNavigationComponent::m_navQueryEntityId)
                ->Field("Nearest Distance", &EditorDetourNavigationComponent::m_nearestDistance)
                ->Version(1)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorDetourNavigationComponent>("Detour Navigation Component",
                    "[Calculates paths within an associated navigation mesh.]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorDetourNavigationComponent::m_navQueryEntityId,
                        "Navigation Mesh", "Entity with Recast Navigation Mesh component")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorDetourNavigationComponent::m_nearestDistance,
                        "Nearest Distance", "If FindPath APIs are given points that are outside the navigation mesh, then "
                        "look for the nearest point on the navigation mesh within this distance from the specified positions.")
                    ;
            }
        }
    }

    void EditorDetourNavigationComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("DetourNavigationComponent"));
    }

    void EditorDetourNavigationComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("DetourNavigationComponent"));
    }

    void EditorDetourNavigationComponent::Activate()
    {
        EditorComponentBase::Activate();
    }

    void EditorDetourNavigationComponent::Deactivate()
    {
        EditorComponentBase::Deactivate();
    }

    void EditorDetourNavigationComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        gameEntity->CreateComponent<DetourNavigationComponent>(m_navQueryEntityId, m_nearestDistance);
    }
} // namespace RecastNavigation
