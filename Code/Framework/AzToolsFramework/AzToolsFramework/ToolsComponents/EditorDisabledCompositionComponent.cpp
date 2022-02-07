/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorDisabledCompositionComponent.h"

#include <AzCore/Serialization/EditContext.h>

namespace AzToolsFramework
{
    namespace Components
    {
        void EditorDisabledCompositionComponent::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorDisabledCompositionComponent, EditorComponentBase>()
                    ->Field("DisabledComponents", &EditorDisabledCompositionComponent::m_disabledComponents)
                    ;

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<EditorDisabledCompositionComponent>("Disabled Components", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Hide)
                            ->Attribute(AZ::Edit::Attributes::HideIcon, true)
                            ->Attribute(AZ::Edit::Attributes::SliceFlags, AZ::Edit::SliceFlags::HideOnAdd | AZ::Edit::SliceFlags::PushWhenHidden)
                        ;
                }
            }
        }

        void EditorDisabledCompositionComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC("EditorDisabledCompositionService", 0x277e3445));
        }

        void EditorDisabledCompositionComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC("EditorDisabledCompositionService", 0x277e3445));
        }

        void EditorDisabledCompositionComponent::GetDisabledComponents(AZStd::vector<AZ::Component*>& components)
        {
            components.insert(components.end(), m_disabledComponents.begin(), m_disabledComponents.end());
        }

        void EditorDisabledCompositionComponent::AddDisabledComponent(AZ::Component* componentToAdd)
        {
            AZ_Assert(componentToAdd, "Unable to add a disabled component that is nullptr");
            if (componentToAdd && AZStd::find(m_disabledComponents.begin(), m_disabledComponents.end(), componentToAdd) == m_disabledComponents.end())
            {
                m_disabledComponents.push_back(componentToAdd);
            }
        }

        void EditorDisabledCompositionComponent::RemoveDisabledComponent(AZ::Component* componentToRemove)
        {
            AZ_Assert(componentToRemove, "Unable to remove a disabled component that is nullptr");
            if (componentToRemove)
            {
                m_disabledComponents.erase(AZStd::remove(m_disabledComponents.begin(), m_disabledComponents.end(), componentToRemove), m_disabledComponents.end());
            }
        };

        EditorDisabledCompositionComponent::~EditorDisabledCompositionComponent()
        {
            for (auto disabledComponent : m_disabledComponents)
            {
                delete disabledComponent;
            }
            m_disabledComponents.clear();

            // We disconnect from the bus here because we need to be able to respond even if the entity and component are not active
            // This is a special case for certain EditorComponents only!
            EditorDisabledCompositionRequestBus::Handler::BusDisconnect();
        }

        void EditorDisabledCompositionComponent::Init()
        {
            EditorComponentBase::Init();

            // We connect to the bus here because we need to be able to respond even if the entity and component are not active
            // This is a special case for certain EditorComponents only!
            EditorDisabledCompositionRequestBus::Handler::BusConnect(GetEntityId());

            // Set the entity* for each disabled component
            for (auto disabledComponent : m_disabledComponents)
            {
                auto editorComponentBaseComponent = azrtti_cast<Components::EditorComponentBase*>(disabledComponent);
                AZ_Assert(editorComponentBaseComponent, "Editor component does not derive from EditorComponentBase");
                editorComponentBaseComponent->SetEntity(GetEntity());
            }
        }

        void EditorDisabledCompositionComponent::Activate()
        {
            EditorComponentBase::Activate();
        }

        void EditorDisabledCompositionComponent::Deactivate()
        {
            EditorComponentBase::Deactivate();
        }
    } // namespace Components
} // namespace AzToolsFramework
