/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Entity/EditorEntityUtilityComponent.h>
#include <Entity/EditorEntityContextBus.h>

namespace AzToolsFramework
{
    AZ::EntityId CreateEditorEntity(AZStd::string_view entityName)
    {
        AZ::Entity* newEntity = aznew AZ::Entity(entityName);

        AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
            &AzToolsFramework::EditorEntityContextRequestBus::Events::AddRequiredComponents, *newEntity);

        newEntity->Init();
        newEntity->Activate();

        return newEntity->GetId();
    }

    void EditorEntityUtilityComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorEntityUtilityComponent, AZ::Component>();
        }

        if (auto* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Method("CreateEditorEntity", &CreateEditorEntity)
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "Entity")
                ->Attribute(AZ::Script::Attributes::Module, "entity");
        }
    }

    
}
