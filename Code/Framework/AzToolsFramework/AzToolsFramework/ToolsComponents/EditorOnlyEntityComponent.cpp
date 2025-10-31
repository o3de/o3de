/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzToolsFramework/ToolsComponents/EditorOnlyEntityComponent.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace AzToolsFramework
{
    namespace Components
    {
        void EditorOnlyEntityComponent::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorOnlyEntityComponent, EditorComponentBase>()
                    ->Field("IsEditorOnly", &EditorOnlyEntityComponent::m_isEditorOnly)
                    ;

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<EditorOnlyEntityComponent>("Editor-Only Flag Handler", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Hide)
                            ->Attribute(AZ::Edit::Attributes::HideIcon, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &EditorOnlyEntityComponent::m_isEditorOnly, 
                            "Editor Only", 
                            "Marks the entity for editor-use only. If true, the entity will not be exported for use in runtime contexts (including dynamic slices).")
                        ;
                }
            }
        }

        void EditorOnlyEntityComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC_CE("EditorOnlyEntityService"));
        }

        void EditorOnlyEntityComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC_CE("EditorOnlyEntityService"));
        }

        EditorOnlyEntityComponent::EditorOnlyEntityComponent()
            : m_isEditorOnly(false)
        {

        }

        EditorOnlyEntityComponent::~EditorOnlyEntityComponent()
        {

        }

        void EditorOnlyEntityComponent::Init()
        {
            EditorComponentBase::Init();

            // Connect at Init()-time to allow slice compilation to query for editor only status.
            EditorOnlyEntityComponentRequestBus::Handler::BusConnect(GetEntityId());
        }

        void EditorOnlyEntityComponent::Activate()
        {
            EditorComponentBase::Activate();

            EditorOnlyEntityComponentRequestBus::Handler::BusConnect(GetEntityId());
        }

        void EditorOnlyEntityComponent::Deactivate()
        {
            EditorOnlyEntityComponentRequestBus::Handler::BusDisconnect();

            EditorComponentBase::Deactivate();
        }

        bool EditorOnlyEntityComponent::IsEditorOnlyEntity()
        {
            return m_isEditorOnly;
        }

        void EditorOnlyEntityComponent::SetIsEditorOnlyEntity(bool isEditorOnly)
        {
            if (isEditorOnly != m_isEditorOnly)
            {
                AzToolsFramework::ScopedUndoBatch undo("Set IsEditorOnly");
                m_isEditorOnly = isEditorOnly;
                EditorOnlyEntityComponentNotificationBus::Broadcast(&EditorOnlyEntityComponentNotificationBus::Events::OnEditorOnlyChanged, GetEntityId(), m_isEditorOnly);
                undo.MarkEntityDirty(GetEntityId());
            }
        }

    } // namespace Components

} // namespace AzToolsFramework
