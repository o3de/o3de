/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorRecastNavigationMeshComponent.h"

#include <AzCore/Serialization/EditContext.h>
#include <Components/RecastNavigationMeshComponent.h>

namespace RecastNavigation
{
    void EditorRecastNavigationMeshComponent::Reflect(AZ::ReflectContext* context)
    {
        BaseClass::Reflect(context);

        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<EditorRecastNavigationMeshComponent, BaseClass>()
                ->Version(1);

            if (AZ::EditContext* editContext = serialize->GetEditContext())
            {
                editContext->Class<EditorRecastNavigationMeshComponent>("Recast Navigation Mesh",
                    "[Calculates the walkable navigation mesh within a specified area.]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

                editContext->Class<RecastNavigationMeshComponentController>(
                    "MeshComponentController", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &RecastNavigationMeshComponentController::m_configuration, "Configuration", "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->ConstantProperty("EditorRecastNavigationMeshComponentTypeId",
                BehaviorConstant(AZ::Uuid(EditorRecastNavigationMeshComponentTypeId)))
                ->Attribute(AZ::Script::Attributes::Module, "navigation")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
        }
    }

    EditorRecastNavigationMeshComponent::EditorRecastNavigationMeshComponent(const RecastNavigationMeshConfig& config)
        : BaseClass(config)
    {
    }
} // namespace RecastNavigation
