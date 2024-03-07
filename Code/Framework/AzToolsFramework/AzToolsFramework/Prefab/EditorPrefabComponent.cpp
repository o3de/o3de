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
#include <AzCore/IO/Path/Path.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/Prefab/Instance/InstanceEntityMapperInterface.h>
#include <AzToolsFramework/Prefab/PrefabPublicInterface.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <AzToolsFramework/UI/EditorEntityUi/EditorEntityUiInterface.h>
#include <AzToolsFramework/UI/Prefab/PrefabIntegrationBus.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        EditorPrefabOverride::EditorPrefabOverride(AZStd::string label, AZStd::string value)
            : m_label(AZStd::move(label))
            , m_value(AZStd::move(value))
        {
        }

        void EditorPrefabOverride::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorPrefabOverride>()
                    ->Field("label", &EditorPrefabOverride::m_label)
                    ->Field("value", &EditorPrefabOverride::m_value)
                    ;

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<EditorPrefabOverride>("Prefab Override", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        //->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &EditorPrefabOverride::m_value, "Patch", "")
                        ->Attribute(AZ::Edit::Attributes::NameLabelOverride, &EditorPrefabOverride::m_label)
                        //->Attribute(AZ::Edit::Attributes::ReadOnly, true)
                        ;
                }
            }
        }

        void EditorPrefabComponent::Reflect(AZ::ReflectContext* context)
        {
            EditorPrefabOverride::Reflect(context);

            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorPrefabComponent, EditorComponentBase>()
                    ->Field("patches", &EditorPrefabComponent::m_patches);

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<EditorPrefabComponent>("Prefab Component", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->Attribute(
                            AZ::Edit::Attributes::SliceFlags,
                            AZ::Edit::SliceFlags::HideOnAdd | AZ::Edit::SliceFlags::PushWhenHidden |
                                AZ::Edit::SliceFlags::DontGatherReference)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &EditorPrefabComponent::m_patches, "Patches", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                        ->Attribute(AZ::Edit::Attributes::ContainerReorderAllow, false)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true);
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

            // Fetch all patches from direct parent (for now)
            AZ::EntityId entityId = m_entity->GetId();

            // Find LinkId to direct parent
            auto instanceEntityMapperInterface = AZ::Interface<Prefab::InstanceEntityMapperInterface>::Get();
            if (auto owningInstance = instanceEntityMapperInterface->FindOwningInstance(entityId); owningInstance.has_value())
            {
                auto linkId = owningInstance->get().GetLinkId();

                // Get Link
                auto prefabSystemComponentInterface = AZ::Interface<AzToolsFramework::Prefab::PrefabSystemComponentInterface>::Get();
                auto findLinkResult = prefabSystemComponentInterface->FindLink(linkId);

                if (findLinkResult.has_value())
                {
                    [[maybe_unused]] PrefabDom linkPatches;
                    findLinkResult->get().GetLinkPatches(linkPatches, linkPatches.GetAllocator());

                    for (const auto& patch : linkPatches.GetArray())
                    {
                        AZStd::string pathString = patch.GetObject().FindMember("path")->value.GetString();
                        AZ::IO::Path path (pathString.c_str(), '/');

                        m_patches.push_back({ path.RootName().String(), "123" });
                    }
                }
            }
        }

        void EditorPrefabComponent::Deactivate()
        {
            PrefabInstanceContainerNotificationBus::Broadcast(
                &PrefabInstanceContainerNotifications::OnPrefabComponentDeactivate, GetEntityId());
        }

    } // namespace Prefab
} // namespace AzToolsFramework
