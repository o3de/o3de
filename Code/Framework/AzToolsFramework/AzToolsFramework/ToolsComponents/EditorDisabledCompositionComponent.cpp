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
                    ->Version(1)
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
            for (auto const& pair : m_disabledComponents)
            {
                components.insert(components.end(), pair.second);
            }
        }

        void EditorDisabledCompositionComponent::AddDisabledComponent(AZ::Component* componentToAdd)
        {
            AZ_Assert(componentToAdd, "Unable to add a disabled component that is nullptr.");

            AZStd::string componentAlias = componentToAdd->GetSerializedIdentifier();
            AZ_Assert(!componentAlias.empty(), "Unable to add a disabled component that has an empty component alias.");

            bool found = m_disabledComponents.find(componentAlias) != m_disabledComponents.end();
            AZ_Assert(!found, "Unable to add a disabled component that was added already.");

            if (componentToAdd && !found)
            {
                m_disabledComponents.emplace(AZStd::move(componentAlias), componentToAdd);
            }
        }

        void EditorDisabledCompositionComponent::RemoveDisabledComponent(AZ::Component* componentToRemove)
        {
            AZ_Assert(componentToRemove, "Unable to remove a disabled component that is nullptr.");

            AZStd::string componentAlias = componentToRemove->GetSerializedIdentifier();
            AZ_Assert(!componentAlias.empty(), "Unable to add a disabled component that has an empty component alias.");

            bool found = m_disabledComponents.find(componentAlias) != m_disabledComponents.end();
            AZ_Assert(found, "Unable to remove a disabled component that has not been added.");

            if (componentToRemove && found)
            {
                m_disabledComponents.erase(componentAlias);
            }
        };

        bool EditorDisabledCompositionComponent::IsComponentDisabled(const AZ::Component* componentToCheck)
        {
            AZ_Assert(componentToCheck, "Unable to check a component that is nullptr.");

            AZStd::string componentAlias = componentToCheck->GetSerializedIdentifier();
            AZ_Assert(!componentAlias.empty(), "Unable to check a component that has an empty component alias.");

            auto componentIt = m_disabledComponents.find(componentAlias);

            if (componentIt != m_disabledComponents.end())
            {
                bool sameComponent = componentIt->second == componentToCheck;
                AZ_Assert(sameComponent, "The component to check shares the same alias but is a different object "
                    "compared to the one referenced in the composition component.");

                return sameComponent;
            }

            return false;
        }

        EditorDisabledCompositionComponent::~EditorDisabledCompositionComponent()
        {
            for (auto& pair : m_disabledComponents)
            {
                delete pair.second;
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

            // Set the entity for each disabled component.
            for (auto const& [componentAlias, disabledComponent] : m_disabledComponents)
            {
                // It's possible to get null components in the list if errors occur during serialization.
                // Guard against that case so that the code won't crash.
                if (disabledComponent)
                {
                    auto editorComponentBaseComponent = azrtti_cast<Components::EditorComponentBase*>(disabledComponent);
                    AZ_Assert(editorComponentBaseComponent, "Editor component does not derive from EditorComponentBase");

                    editorComponentBaseComponent->SetEntity(GetEntity());
                    editorComponentBaseComponent->SetSerializedIdentifier(componentAlias);
                }
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
