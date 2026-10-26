/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorRecastNavigationPhysXProviderComponent.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/Translation/TranslationDef.h>

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
                editContext->Class<EditorRecastNavigationPhysXProviderComponent>(
                    QT_TRANSLATE_NOOP("RecastNavigation", "Recast Navigation PhysX Provider"),
                    QT_TRANSLATE_NOOP("RecastNavigation", "[Collects triangle geometry from PhysX scene for navigation mesh within the area defined by a shape component.]"))
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                ;

                editContext->Class<RecastNavigationPhysXProviderComponentController>(
                    QT_TRANSLATE_NOOP("RecastNavigation", "RecastNavigationPhysXProviderComponentController"), "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &RecastNavigationPhysXProviderComponentController::m_config,
                        QT_TRANSLATE_NOOP("RecastNavigation", "Configuration"), "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;

                editContext->Class<RecastNavigationPhysXProviderConfig>(
                    QT_TRANSLATE_NOOP("RecastNavigation", "Recast Navigation PhysX Provider Config"),
                    QT_TRANSLATE_NOOP("RecastNavigation", "[Navigation PhysX Provider configuration]"))
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &RecastNavigationPhysXProviderConfig::m_collisionGroupId,
                        QT_TRANSLATE_NOOP("RecastNavigation", "Collision Group"),
                        QT_TRANSLATE_NOOP("RecastNavigation", "If set, only colliders from the specified collision group will be considered."))
                    ;
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

    AZ::u32 EditorRecastNavigationPhysXProviderComponent::OnConfigurationChanged()
    {
        m_controller.OnConfigurationChanged();
        return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
    }
} // namespace RecastNavigation
