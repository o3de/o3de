/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
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
                    ->Version(1)
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
            for (auto const& pair : m_pendingComponents)
            {
                components.insert(components.end(), pair.second);
            }
        }

        void EditorPendingCompositionComponent::AddPendingComponent(AZ::Component* componentToAdd)
        {
            AZ_Assert(componentToAdd, "Unable to add a pending component that is nullptr.");

            AZStd::string componentAlias = componentToAdd->GetSerializedIdentifier();
            AZ_Assert(!componentAlias.empty(), "Unable to add a pending component that has an empty component alias.");

            bool found = m_pendingComponents.find(componentAlias) != m_pendingComponents.end();
            AZ_Assert(!found, "Unable to add a pending component that was added already.");

            if (componentToAdd && !found)
            {
                m_pendingComponents.emplace(AZStd::move(componentAlias), componentToAdd);

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
            AZ_Assert(componentToRemove, "Unable to remove a pending component that is nullptr.");

            AZStd::string componentAlias = componentToRemove->GetSerializedIdentifier();
            AZ_Assert(!componentAlias.empty(), "Unable to remove a pending component that has an empty component alias.");

            bool found = m_pendingComponents.find(componentAlias) != m_pendingComponents.end();
            AZ_Assert(found, "Unable to remove a pending component that has not been added.");

            if (componentToRemove && found)
            {
                m_pendingComponents.erase(componentAlias);

                bool isDuringUndo = false;
                AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(isDuringUndo, &AzToolsFramework::ToolsApplicationRequestBus::Events::IsDuringUndoRedo);
                if (isDuringUndo)
                {
                    SetDirty();
                }
            }
        };

        bool EditorPendingCompositionComponent::IsComponentPending(const AZ::Component* componentToCheck)
        {
            AZ_Assert(componentToCheck, "Unable to check a component that is nullptr.");

            AZStd::string componentAlias = componentToCheck->GetSerializedIdentifier();
            AZ_Assert(!componentAlias.empty(), "Unable to check a component that has an empty component alias.");

            auto componentIt = m_pendingComponents.find(componentAlias);

            if (componentIt != m_pendingComponents.end())
            {
                bool sameComponent = componentIt->second == componentToCheck;
                AZ_Assert(sameComponent, "The component to check shares the same alias but is a different object "
                    "compared to the one referenced in the composition component.");

                return sameComponent;
            }

            return false;
        }

        EditorPendingCompositionComponent::~EditorPendingCompositionComponent()
        {
            for (auto& pair : m_pendingComponents)
            {
                delete pair.second;
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

            // Set the entity for each pending component.
            for (auto const& [componentAlias, pendingComponent] : m_pendingComponents)
            {
                // It's possible to get null components in the list if errors occur during serialization.
                // Guard against that case so that the code won't crash.
                if (pendingComponent)
                {
                    auto editorComponentBaseComponent = azrtti_cast<Components::EditorComponentBase*>(pendingComponent);
                    AZ_Assert(editorComponentBaseComponent, "Editor component does not derive from EditorComponentBase");

                    editorComponentBaseComponent->SetEntity(GetEntity());
                    editorComponentBaseComponent->SetSerializedIdentifier(componentAlias);
                }
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
