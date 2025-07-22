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
        AZStd::string EditorPrefabComponent::GetLabelForIndex(int index) const
        {
            AZStd::string label = m_overrides[index].GetPropertyPath();

            // Resize to 128 or it will not fit the label for the DPE.
            // TODO - Find a more elegant way to do this (elision?) or at least show the full path in the tooltip.
            label.resize(128);

            return label;
        }

        void EditorPrefabComponent::Reflect(AZ::ReflectContext* context)
        {
            EditorPrefabOverride::Reflect(context);

            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorPrefabComponent, EditorComponentBase>()
                    ->Field("overrides", &EditorPrefabComponent::m_overrides);

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
                        ->DataElement(AZ::Edit::UIHandlers::Default, &EditorPrefabComponent::m_overrides, "Overrides", "Overrides applied by the currently focused prefab to this prefab instance.")
                        ->Attribute(AZ::Edit::Attributes::IndexedChildNameLabelOverride, &EditorPrefabComponent::GetLabelForIndex)
                        //->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
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

            // TODO - Fetch all patches applied to this prefab from the currently focused prefab.
            // If the prefab is accessible in the Inspector, the focused prefab should always be an ancestor.
            
            // TODO - This function should be run again whenever prefab focus changes.
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

                        if (pathString.starts_with("/ContainerEntity"))
                        {
                            continue;
                        }

                        AZ::IO::Path path (pathString.c_str(), '/');
                        AZStd::string value;

                        auto& valueObj = patch.GetObject().FindMember("value")->value;
                        if (valueObj.IsString())
                        {
                            value = valueObj.GetString();
                        }
                        else if (valueObj.IsNumber())
                        {
                            value = AZStd::to_string(valueObj.GetDouble());
                        }
                        else if (valueObj.IsBool())
                        {
                            if (valueObj.GetBool())
                            {
                                value = "True";
                            }
                            else
                            {
                                value = "False";
                            }
                        }

                        m_overrides.push_back({ AZStd::move(path), value });
                    }
                }
            }
        }

        void EditorPrefabComponent::Deactivate()
        {
            PrefabInstanceContainerNotificationBus::Broadcast(
                &PrefabInstanceContainerNotifications::OnPrefabComponentDeactivate, GetEntityId());
        }

        EditorPrefabComponent::EditorPrefabOverride::EditorPrefabOverride(AZ::IO::Path path, AZStd::string value)
            : m_path(AZStd::move(path))
            , m_value(AZStd::move(value))
        {
        }

        AZStd::string EditorPrefabComponent::EditorPrefabOverride::GetPropertyName() const
        {
            return m_path.Filename().String();
        }

        AZStd::string EditorPrefabComponent::EditorPrefabOverride::GetPropertyPath() const
        {
            return m_path.String();
        }

        void EditorPrefabComponent::EditorPrefabOverride::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorPrefabOverride>()
                    ->Field("path", &EditorPrefabOverride::m_path)
                    ->Field("value", &EditorPrefabOverride::m_value);

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<EditorPrefabOverride>("Prefab Override", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &EditorPrefabOverride::m_value, "Value", "")
                        ->Attribute(AZ::Edit::Attributes::NameLabelOverride, &EditorPrefabOverride::GetPropertyName)
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, true);
                }
            }
        }

    } // namespace Prefab
} // namespace AzToolsFramework
