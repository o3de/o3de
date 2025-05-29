/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Prefab/EditorPrefabComponent.h>

#include <AzCore/Interface/Interface.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/Prefab/PrefabPublicInterface.h>
#include <AzToolsFramework/UI/EditorEntityUi/EditorEntityUiInterface.h>
#include <AzToolsFramework/UI/Prefab/PrefabIntegrationBus.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        void EditorPrefabComponent::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorPrefabComponent, EditorComponentBase>();

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<EditorPrefabComponent>("Prefab Component", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Hide)
                        ->Attribute(AZ::Edit::Attributes::HideIcon, true)
                        ->Attribute(
                            AZ::Edit::Attributes::SliceFlags,
                            AZ::Edit::SliceFlags::HideOnAdd | AZ::Edit::SliceFlags::PushWhenHidden |
                                AZ::Edit::SliceFlags::DontGatherReference);
                }
            }

            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->ConstantProperty(
                    "EditorPrefabComponentTypeId", BehaviorConstant(AZ::Uuid(EditorPrefabComponent::EditorPrefabComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            }
        }

        void EditorPrefabComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC_CE("EditorPrefabInstanceContainerService"));
        }

        void EditorPrefabComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC_CE("EditorPrefabInstanceContainerService"));
        }

        void EditorPrefabComponent::Activate()
        {
            PrefabInstanceContainerNotificationBus::Broadcast(
                &PrefabInstanceContainerNotifications::OnPrefabComponentActivate, GetEntityId());
        }

        void EditorPrefabComponent::Deactivate()
        {
            PrefabInstanceContainerNotificationBus::Broadcast(
                &PrefabInstanceContainerNotifications::OnPrefabComponentDeactivate, GetEntityId());
        }

    } // namespace Prefab
} // namespace AzToolsFramework
