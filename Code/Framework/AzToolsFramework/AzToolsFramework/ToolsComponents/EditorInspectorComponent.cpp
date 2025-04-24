/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorInspectorComponent.h"
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/sort.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>

namespace AzToolsFramework
{
    namespace Components
    {
        void EditorInspectorComponent::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<ComponentOrderEntry>()
                    // Persistent IDs for this are simply the component id
                    ->PersistentId([](const void* instance) -> AZ::u64
                    { 
                        return reinterpret_cast<const ComponentOrderEntry*>(instance)->m_componentId;
                    })
                    ->Version(1)
                    ->Field("ComponentId", &ComponentOrderEntry::m_componentId)
                    ->Field("SortIndex", &ComponentOrderEntry::m_sortIndex);

                serializeContext->Class<EditorInspectorComponent, EditorComponentBase>()
                    ->Version(2, &SerializationConverter)
                    ->EventHandler<ComponentOrderSerializationEvents>()
                    ->Field("ComponentOrderEntryArray", &EditorInspectorComponent::m_componentOrderEntryArray);

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<EditorInspectorComponent>("Inspector Component Order", "Edit-time entity inspector state")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Hide)
                            ->Attribute(AZ::Edit::Attributes::SliceFlags, AZ::Edit::SliceFlags::HideOnAdd | AZ::Edit::SliceFlags::PushWhenHidden)
                            ->Attribute(AZ::Edit::Attributes::HideIcon, true)
                        ;
                }
            }
        }

        void EditorInspectorComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC_CE("EditorInspectorService"));
        }

        void EditorInspectorComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC_CE("EditorInspectorService"));
        }

        bool EditorInspectorComponent::SerializationConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            // Prior to version 2, we left the version unspecified so it was technically 0
            if (classElement.GetVersion() <= 1)
            {
                // Convert the vector of component ids to an array of order entries
                auto componentOrderArrayElement = classElement.FindSubElement(AZ_CRC_CE("ComponentOrderArray"));
                if (!componentOrderArrayElement)
                {
                    return false;
                }

                ComponentOrderArray componentOrderArray;
                componentOrderArray.reserve(componentOrderArrayElement->GetNumSubElements());
                AZ::ComponentId componentId;
                for (int arrayElementIndex = 0; arrayElementIndex < componentOrderArrayElement->GetNumSubElements(); ++arrayElementIndex)
                {
                    AZ::SerializeContext::DataElementNode& elementNode = componentOrderArrayElement->GetSubElement(arrayElementIndex);
                    if (elementNode.GetData(componentId))
                    {
                        componentOrderArray.push_back(componentId);
                    }
                }

                AZ_Error("EditorInspectorComponent", componentOrderArray.size() == componentOrderArrayElement->GetNumSubElements(), "Unable to get all the expected elements for the old component order array");

                // Get rid of the old array
                classElement.RemoveElementByName(AZ_CRC_CE("ComponentOrderArray"));

                // Add a new empty array (unable to use AddElementWithData, fails stating that AZStd::vector is not registered)
                int newArrayElementIndex = classElement.AddElement<ComponentOrderEntryArray>(context, "ComponentOrderEntryArray");
                if (newArrayElementIndex == -1)
                {
                    return false;
                }

                auto& newArrayElement = classElement.GetSubElement(newArrayElementIndex);

                bool elementAddSucceeded = true;
                for (size_t componentIndex = 0; componentIndex < componentOrderArray.size(); ++componentIndex)
                {
                    int orderEntryElement = newArrayElement.AddElementWithData<ComponentOrderEntry>(context, "element", ComponentOrderEntry{componentOrderArray[componentIndex], componentIndex});
                    if (orderEntryElement == -1)
                    {
                        elementAddSucceeded = false;
                    }
                }

                return elementAddSucceeded;
            }
            return true;
        }

        EditorInspectorComponent::~EditorInspectorComponent()
        {
            // We disconnect from the bus here because we need to be able to respond even if the entity and component are not active
            // This is a special case for certain EditorComponents only!
            EditorInspectorComponentRequestBus::Handler::BusDisconnect();
        }

        void EditorInspectorComponent::Init()
        {
            // We connect to the bus here because we need to be able to respond even if the entity and component are not active
            // This is a special case for certain EditorComponents only!
            EditorInspectorComponentRequestBus::Handler::BusConnect(GetEntityId());
        }

        void EditorInspectorComponent::PrepareSave()
        {
            // Clear the actual persistent id storage to get rebuilt from the order entry array
            m_componentOrderEntryArray.clear();
            m_componentOrderEntryArray.reserve(m_componentOrderArray.size());

            // Write our vector data back to the order element array
            for (size_t componentIndex = 0; componentIndex < m_componentOrderArray.size(); ++componentIndex)
            {
                m_componentOrderEntryArray.push_back({ m_componentOrderArray[componentIndex], componentIndex });
            }
        }

        void EditorInspectorComponent::PostLoad()
        {
            // Clear out the vector to be rebuilt from persistent id
            m_componentOrderArray.clear();
            m_componentOrderArray.reserve(m_componentOrderEntryArray.size());

            // This will sort all the component order entries by sort index (primary) and component id (secondary) which should never result in any collisions
            // This is used since slice data patching may create duplicate entries for the same sort index, missing indices and the like.
            // It should never result in multiple component id entries since the serialization of this data uses a persistent id which is the component id
            AZStd::sort(m_componentOrderEntryArray.begin(), m_componentOrderEntryArray.end(), 
                [](const ComponentOrderEntry& lhs, const ComponentOrderEntry& rhs) -> bool
                {
                    return lhs.m_sortIndex < rhs.m_sortIndex || (lhs.m_sortIndex == rhs.m_sortIndex && lhs.m_componentId < rhs.m_componentId);
                }
            );

            for (auto& componentOrderEntry : m_componentOrderEntryArray)
            {
                m_componentOrderArray.push_back(componentOrderEntry.m_componentId);
            }
        }

        ComponentOrderArray EditorInspectorComponent::GetComponentOrderArray()
        {
            return m_componentOrderArray;
        }

        void EditorInspectorComponent::SetComponentOrderArray(const ComponentOrderArray& componentOrderArray)
        {
            if (m_componentOrderArray == componentOrderArray)
            {
                // the order is unchanged, not necessary to set or invoke callbacks.
                return;
            }

            // If we get here, the caller is requesting a new order, but, we only want to persist this if the order is actually
            // different from what default would be.
            AZ::Entity::ComponentArrayType components;
            components = GetEntity()->GetComponents();
            RemoveHiddenComponents(components);
            SortComponentsByPriority(components);

            ComponentOrderArray defaultOrderArray;
            defaultOrderArray.reserve(components.size());

            for (const auto& component : components)
            {
                defaultOrderArray.push_back(component->GetId());
            }

            if (defaultOrderArray == componentOrderArray)
            {
                // the new order is the same as the default order
                if (m_componentOrderArray.empty())
                {
                    // the new order is default and the previous order is default, nothing to do.
                    return;
                }

                m_componentOrderArray.clear();
            }
            else
            {
                // the new order is non-default and the previous order is different from it, we need to persist it.
                m_componentOrderArray = componentOrderArray;
            }

            // if we get here, we either went from a default order to a user-specified order
            // or vice versa.  Either way, the order has changed and needs to persist the change.
            // We should only get in here from user interaction - dragging things around, cut and paste, etc
            // so we are in an undo state.
            SetDirty();
            PrepareSave();

            EditorInspectorComponentNotificationBus::Event(GetEntityId(), &EditorInspectorComponentNotificationBus::Events::OnComponentOrderChanged);
        }

    } // namespace Components
} // namespace AzToolsFramework
