/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorEntityModelComponent.h"

#include <AzToolsFramework/Entity/EditorEntityModel.h>

#include <AzCore/RTTI/BehaviorContext.h>

namespace AzToolsFramework
{
    namespace Components
    {
        EditorEntityModelComponent::EditorEntityModelComponent() = default;
        EditorEntityModelComponent::~EditorEntityModelComponent() = default;

        void EditorEntityModelComponent::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorEntityModelComponent, AZ::Component>(); // Empty class
            }

            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<EditorEntityInfoRequestBus>("EditorEntityInfoRequestBus")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Category, "Entity")
                    ->Attribute(AZ::Script::Attributes::Module, "editor")
                    ->Event("GetParent", &EditorEntityInfoRequests::GetParent)
                    ->Event("GetChildren", &EditorEntityInfoRequests::GetChildren)
                    ->Event("GetChild", &EditorEntityInfoRequests::GetChild)
                    ->Event("GetChildCount", &EditorEntityInfoRequests::GetChildCount)
                    ->Event("GetChildIndex", &EditorEntityInfoRequests::GetChildIndex)
                    ->Event("GetName", &EditorEntityInfoRequests::GetName)
                    ->Event("IsLocked", &EditorEntityInfoRequests::IsLocked)
                    ->Event("IsVisible", &EditorEntityInfoRequests::IsVisible)
                    ->Event("IsHidden", &EditorEntityInfoRequests::IsHidden)
                    ->Event("GetStartStatus", &EditorEntityInfoRequests::GetStartStatus)
                    ;

                behaviorContext->EBus<EditorEntityAPIBus>("EditorEntityAPIBus")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Category, "Entity")
                    ->Attribute(AZ::Script::Attributes::Module, "editor")
                    ->Event("SetName", &EditorEntityAPIRequests::SetName)
                    ->Event("SetParent", &EditorEntityAPIRequests::SetParent)
                    ->Event("SetLockState", &EditorEntityAPIRequests::SetLockState)
                    ->Event("SetVisibilityState", &EditorEntityAPIRequests::SetVisibilityState)
                    ->Event("SetStartStatus", &EditorEntityAPIRequests::SetStartStatus)
                    ;

                behaviorContext->EnumProperty<static_cast<int>(EditorEntityStartStatus::StartActive)>("EditorEntityStartStatus_StartActive")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
                behaviorContext->EnumProperty<static_cast<int>(EditorEntityStartStatus::StartInactive)>("EditorEntityStartStatus_StartInactive")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
                behaviorContext->EnumProperty<static_cast<int>(EditorEntityStartStatus::EditorOnly)>("EditorEntityStartStatus_EditorOnly")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            }
        }

        void EditorEntityModelComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC_CE("EditorEntityModelService"));
        }

        void EditorEntityModelComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC_CE("EditorEntityModelService"));
        }

        void EditorEntityModelComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC_CE("EditorEntityContextService"));
        }

        void EditorEntityModelComponent::Activate()
        {
            m_entityModel = AZStd::make_unique<EditorEntityModel>();
        }

        void EditorEntityModelComponent::Deactivate()
        {
            m_entityModel.reset();
        }

    } // namespace Components
} // namespace AzToolsFramework
