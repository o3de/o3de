/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorRecastNavigationTiledSurveyorComponent.h"

#include <AzCore/Serialization/EditContext.h>
#include <Components/RecastNavigationTiledSurveyorComponent.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>

namespace RecastNavigation
{
    void EditorRecastNavigationTiledSurveyorComponent::Reflect(AZ::ReflectContext* context)
    {
        if (const auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<EditorRecastNavigationTiledSurveyorComponent, AZ::Component>()
                ->Field("Show Input Data", &EditorRecastNavigationTiledSurveyorComponent::m_debugDrawInputData)
                ->Version(1)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<EditorRecastNavigationTiledSurveyorComponent>("Recast Navigation Tiled Surveyor",
                    "[Collects triangle geometry for navigation mesh within the area defined by a shape component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(nullptr, &EditorRecastNavigationTiledSurveyorComponent::m_debugDrawInputData, "Show Input Data",
                        "If enabled, debug draw is enabled to show the triangles collected in the Editor scene for the navigation mesh")
                    ;
            }
        }
    }

    void EditorRecastNavigationTiledSurveyorComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("RecastNavigationSurveyorComponent"));
        provided.push_back(AZ_CRC_CE("RecastNavigationSurveyorService"));
    }

    void EditorRecastNavigationTiledSurveyorComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("RecastNavigationSurveyorComponent"));
        incompatible.push_back(AZ_CRC_CE("RecastNavigationSurveyorService"));
    }

    void EditorRecastNavigationTiledSurveyorComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("AxisAlignedBoxShapeService"));
    }

    void EditorRecastNavigationTiledSurveyorComponent::Activate()
    {
        EditorComponentBase::Activate();
    }

    void EditorRecastNavigationTiledSurveyorComponent::Deactivate()
    {
        EditorComponentBase::Deactivate();
    }

    void EditorRecastNavigationTiledSurveyorComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        gameEntity->CreateComponent<RecastNavigationTiledSurveyorComponent>();
    }
} // namespace RecastNavigation
