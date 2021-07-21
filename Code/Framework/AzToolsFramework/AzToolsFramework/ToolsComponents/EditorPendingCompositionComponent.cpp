/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "AzToolsFramework_precompiled.h"
#include "EditorPendingCompositionComponent.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

namespace AzToolsFramework
{
    namespace Components
    {
        void EditorPendingCompositionComponent::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorPendingCompositionComponent, EditorComponentBase>()
                    ->Field("PendingComponents", &EditorPendingCompositionComponent::m_pendingComponents)
                    ;

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<EditorPendingCompositionComponent>("Pending Components", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Hide)
                            ->Attribute(AZ::Edit::Attributes::HideIcon, true)
                            ->Attribute(AZ::Edit::Attributes::SliceFlags, AZ::Edit::SliceFlags::HideOnAdd | AZ::Edit::SliceFlags::PushWhenHidden)
                        ;
                }
            }
        }

        void EditorPendingCompositionComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC("EditorPendingCompositionService", 0x6b5b794f));
        }

        void EditorPendingCompositionComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC("EditorPendingCompositionService", 0x6b5b794f));
        }

        void EditorPendingCompositionComponent::GetPendingComponents(AZStd::vector<AZ::Component*>& components)
        {
            components.insert(components.end(), m_pendingComponents.begin(), m_pendingComponents.end());
        }

        void EditorPendingCompositionComponent::AddPendingComponent(AZ::Component* componentToAdd)
        {
            AZ_Assert(componentToAdd, "Unable to add a pending component that is nullptr");
            if (componentToAdd && AZStd::find(m_pendingComponents.begin(), m_pendingComponents.end(), componentToAdd) == m_pendingComponents.end())
            {
                m_pendingComponents.push_back(componentToAdd);
                bool isDuringUndo = false;
                AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(isDuringUndo, &AzToolsFramework::ToolsApplicationRequestBus::Events::IsDuringUndoRedo);
                if (isDuringUndo)
                {
                    SetDirty();
                }
            }
        }

        void EditorPendingCompositionComponent::RemovePendingComponent(AZ::Component* componentToRemove)
        {
            AZ_Assert(componentToRemove, "Unable to remove a pending component that is nullptr");
            if (componentToRemove)
            {
                m_pendingComponents.erase(AZStd::remove(m_pendingComponents.begin(), m_pendingComponents.end(), componentToRemove), m_pendingComponents.end());
                bool isDuringUndo = false;
                AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(isDuringUndo, &AzToolsFramework::ToolsApplicationRequestBus::Events::IsDuringUndoRedo);
                if (isDuringUndo)
                {
                    SetDirty();
                }
            }
        };

        EditorPendingCompositionComponent::~EditorPendingCompositionComponent()
        {
            for (auto pendingComponent : m_pendingComponents)
            {
                delete pendingComponent;
            }
            m_pendingComponents.clear();

            // We disconnect from the bus here because we need to be able to respond even if the entity and component are not active
            // This is a special case for certain EditorComponents only!
            EditorPendingCompositionRequestBus::Handler::BusDisconnect();
        }

        void EditorPendingCompositionComponent::Init()
        {
            EditorComponentBase::Init();

            // We connect to the bus here because we need to be able to respond even if the entity and component are not active
            // This is a special case for certain EditorComponents only!
            EditorPendingCompositionRequestBus::Handler::BusConnect(GetEntityId());

            // Set the entity* for each pending component
            for (auto pendingComponent : m_pendingComponents)
            {
                auto editorComponentBaseComponent = azrtti_cast<Components::EditorComponentBase*>(pendingComponent);
                AZ_Assert(editorComponentBaseComponent, "Editor component does not derive from EditorComponentBase");
                editorComponentBaseComponent->SetEntity(GetEntity());
            }
        }

        void EditorPendingCompositionComponent::Activate()
        {
            EditorComponentBase::Activate();
        }

        void EditorPendingCompositionComponent::Deactivate()
        {
            EditorComponentBase::Deactivate();
        }
    } // namespace Components
} // namespace AzToolsFramework
