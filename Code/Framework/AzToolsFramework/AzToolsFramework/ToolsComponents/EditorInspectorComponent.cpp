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
#include "EditorInspectorComponent.h"
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/sort.h>

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
            services.push_back(AZ_CRC("EditorInspectorService", 0xc7357f25));
        }

        void EditorInspectorComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC("EditorInspectorService", 0xc7357f25));
        }

        bool EditorInspectorComponent::SerializationConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            // Prior to version 2, we left the version unspecified so it was technically 0
            if (classElement.GetVersion() <= 1)
            {
                // Convert the vector of component ids to an array of order entries
                auto componentOrderArrayElement = classElement.FindSubElement(AZ_CRC("ComponentOrderArray", 0x22bd99f7));
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
                classElement.RemoveElementByName(AZ_CRC("ComponentOrderArray", 0x22bd99f7));

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
            // If we didn't dirty the component order, we do not need to regenerate the serialized entry array
            if (!m_componentOrderIsDirty)
            {
                return;
            }

            // Clear the actual persistent id storage to get rebuilt from the order entry array
            m_componentOrderEntryArray.clear();
            m_componentOrderEntryArray.reserve(m_componentOrderArray.size());

            // Write our vector data back to the order element array
            for (size_t componentIndex = 0; componentIndex < m_componentOrderArray.size(); ++componentIndex)
            {
                m_componentOrderEntryArray.push_back({ m_componentOrderArray[componentIndex], componentIndex });
            }

            m_componentOrderIsDirty = false;
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

            m_componentOrderIsDirty = false;
        }

        ComponentOrderArray EditorInspectorComponent::GetComponentOrderArray()
        {
            return m_componentOrderArray;
        }

        void EditorInspectorComponent::SetComponentOrderArray(const ComponentOrderArray& componentOrderArray)
        {
            if (m_componentOrderArray == componentOrderArray)
            {
                return;
            }

            m_componentOrderArray = componentOrderArray;

            SetDirty();

            // mark the order as dirty before sending the OnComponentOrderChanged event in order for PrepareSave to be properly handled in the case 
            // one of the event listeners needs to build the InstanceDataHierarchy
            m_componentOrderIsDirty = true;
            EditorInspectorComponentNotificationBus::Event(GetEntityId(), &EditorInspectorComponentNotificationBus::Events::OnComponentOrderChanged);
        }

    } // namespace Components
} // namespace AzToolsFramework
