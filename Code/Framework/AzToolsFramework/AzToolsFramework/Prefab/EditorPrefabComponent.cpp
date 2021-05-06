/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#include <AzToolsFramework/Prefab/EditorPrefabComponent.h>

#include <AzCore/Interface/Interface.h>
#include <AzCore/Serialization/EditContext.h>
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
        }

        void EditorPrefabComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC("EditorPrefabInstanceContainerService"));
        }

        void EditorPrefabComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC("EditorPrefabInstanceContainerService"));
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
