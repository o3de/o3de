/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorRecastNavigationPhysXProviderComponent.h"

#include <AzCore/Serialization/EditContext.h>

namespace RecastNavigation
{
    void EditorRecastNavigationPhysXProviderComponent::Reflect(AZ::ReflectContext* context)
    {
        BaseClass::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorRecastNavigationPhysXProviderComponent, BaseClass>()
                ->Version(1);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorRecastNavigationPhysXProviderComponent>("Recast Navigation PhysX Provider",
                    "[Collects triangle geometry from PhysX scene for navigation mesh within the area defined by a shape component.]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);
            }
        }
    }

    EditorRecastNavigationPhysXProviderComponent::EditorRecastNavigationPhysXProviderComponent(const RecastNavigationPhysXProviderConfig& config)
        : BaseClass(config)
    {
    }

    void EditorRecastNavigationPhysXProviderComponent::Activate()
    {
        m_controller.m_config.m_useEditorScene = true;
        BaseClass::Activate();
    }

    void EditorRecastNavigationPhysXProviderComponent::Deactivate()
    {
        BaseClass::Deactivate();
    }

    void EditorRecastNavigationPhysXProviderComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        m_controller.m_config.m_useEditorScene = false;
        // The game entity must query the regular game PhysX scene, while the Editor component must query the Editor PhysX scene.
        BaseClass::BuildGameEntity(gameEntity);
        m_controller.m_config.m_useEditorScene = true;
    }
} // namespace RecastNavigation
