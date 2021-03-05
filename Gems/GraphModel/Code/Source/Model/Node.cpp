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

// AZ
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

// Graph Model
#include <GraphModel/Model/Graph.h>
#include <GraphModel/Model/Node.h>

namespace GraphModel
{
    static constexpr int InvalidExtendableSlot = -1;

    void Node::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<Node>()
                ->Version(0)
                // m_id isn't reflected because this information is already stored in the Graph's node map
                // m_outputDataSlots isn't reflected because its Slot::m_value field is unused
                // m_inputEventSlots isn't reflected because its Slot::m_value field is unused
                // m_outputEventSlots isn't reflected because its Slot::m_value field is unused
                ->Field("m_propertySlots", &Node::m_propertySlots)
                ->Field("m_inputDataSlots", &Node::m_inputDataSlots)
                ->Field("m_extendableSlots", &Node::m_extendableSlots)
                ;
        }
    }

    Node::Node(GraphPtr graph)
        : GraphElement(graph)
    {
    }

    void Node::PostLoadSetup(GraphPtr graph, NodeId id)
    {
        AZ_Assert(nullptr == GetGraph(), "Node isn't freshly loaded.");
        AZ_Assert(m_id == INVALID_NODE_ID, "Node isn't freshly loaded.");

        m_graph = graph;
        m_id = id;

        PostLoadSetup();
    }

    void Node::PostLoadSetup()
    {
        RegisterSlots();

        // Make sure the loaded Slot data aligns with the Node's input slot descriptions
        SyncAndSetupSlots(m_propertySlots, m_propertySlotDefinitions);
        SyncAndSetupSlots(m_inputDataSlots, m_inputDataSlotDefinitions);
        SyncAndSetupExtendableSlots();

        // These slots types are a bit different, because they don't actually have any data to be serialized, so instead of SyncAndSetupSlots we need to create them.
        CreateSlotData(m_outputDataSlots, m_outputDataSlotDefinitions);
        CreateSlotData(m_inputEventSlots, m_inputEventSlotDefinitions);
        CreateSlotData(m_outputEventSlots, m_outputEventSlotDefinitions);

        int numExtendableSlots = 0;
        for (auto it = m_extendableSlots.begin(); it != m_extendableSlots.end(); it++)
        {
            numExtendableSlots += aznumeric_cast<int>(it->second.size());
        }

        AZ_Assert(m_allSlots.size() == m_propertySlots.size() + m_inputDataSlots.size() + m_outputDataSlots.size() + m_inputEventSlots.size() + m_outputEventSlots.size() + numExtendableSlots, "Slot counts don't match");
        AZ_Assert(m_allSlotDefinitions.size() == m_propertySlotDefinitions.size() + m_inputDataSlotDefinitions.size() + m_outputDataSlotDefinitions.size() + m_inputEventSlotDefinitions.size() + m_outputEventSlotDefinitions.size() + m_extendableSlotDefinitions.size(), "SlotDefinition counts don't match");
    }

    void Node::CreateSlotData()
    {
        AZ_Assert(m_allSlots.empty(), "CreateSlotData() should only be called once after creating a new node.");

        CreateSlotData(m_propertySlots, m_propertySlotDefinitions);
        CreateSlotData(m_inputDataSlots, m_inputDataSlotDefinitions);
        CreateSlotData(m_outputDataSlots, m_outputDataSlotDefinitions);
        CreateSlotData(m_inputEventSlots, m_inputEventSlotDefinitions);
        CreateSlotData(m_outputEventSlots, m_outputEventSlotDefinitions);

        CreateExtendableSlotData();
    }

    void Node::CreateSlotData(SlotMap& slotMap, const SlotDefinitionList& slotDefinitionList)
    {
        AZ_Assert(slotMap.empty(), "This node isn't freshly initialized");

        for (SlotDefinitionPtr slotDefinition : slotDefinitionList)
        {
            SlotPtr slot = AZStd::make_shared<Slot>(GetGraph(), slotDefinition);
            slot->SetValue(slotDefinition->GetDefaultValue());
            
            SlotId slotId(slotDefinition->GetName());
            auto newEntry = AZStd::make_pair(slotId, slot);
            
            slotMap.insert(newEntry);
            m_allSlots.insert(newEntry);
        }
    }

    void Node::CreateExtendableSlotData()
    {
        for (SlotDefinitionPtr slotDefinition : m_extendableSlotDefinitions)
        {
            // Skip creating slots for this definition if a set already exists in the map, since we
            // use this method to populate slots that have been loaded on existing nodes where
            // new slots have been added in addition to creating slots on nodes for the first time
            const SlotName& slotName = slotDefinition->GetName();
            if (m_extendableSlots.find(slotName) != m_extendableSlots.end())
            {
                continue;
            }

            ExtendableSlotSet extendableSet;

            // When creating a new node, we need to populate enough extendable slots to satisfy
            // the minimum requirement of the definition.
            int minimumSlots = slotDefinition->GetMinimumSlots();
            for (int i = 0; i < minimumSlots; ++i)
            {
                SlotPtr slot = AZStd::make_shared<Slot>(GetGraph(), slotDefinition, i);
                slot->SetValue(slotDefinition->GetDefaultValue());

                auto newEntry = AZStd::make_pair(slot->GetSlotId(), slot);
                m_allSlots.insert(newEntry);

                extendableSet.insert(slot);
            }

            m_extendableSlots.insert(AZStd::make_pair(slotName, extendableSet));
        }
    }

    void Node::SyncAndSetupSlots(SlotMap& slotData, Node::SlotDefinitionList& slotDefinitions)
    {
        // Do PostLoadSetup to attach each Slot to its SlotDefinition
        // Also remove any Slot that doesn't have a corresponding SlotDefinition
        for (auto slotDataIter = slotData.begin(); slotDataIter != slotData.end(); /* increment in loop */)
        {
            const SlotId& slotId = slotDataIter->first;
            const SlotName& slotName = slotId.m_name;
            SlotPtr slot = slotDataIter->second;

            auto slotDefinitionIter = AZStd::find_if(slotDefinitions.begin(), slotDefinitions.end(), [&slotName](SlotDefinitionPtr slotDefinition) { return slotDefinition->GetName() == slotName; });

            if (slotDefinitionIter == slotDefinitions.end())
            {
                AZ_Warning(GetGraph()->GetSystemName(), false, "Found data for unrecognized slot [%s]. It will be ignored.", slotName.c_str());
                slotDataIter = slotData.erase(slotDataIter);
            }
            else
            {
                // CJS TODO: Consider using AZ::Outcome for better error reporting, and if PostLoadSetup fails (could be due to type mismatch) 
                slot->PostLoadSetup(GetGraph(), *slotDefinitionIter);

                ++slotDataIter;
            }
        }

        // Make sure all SlotDefinitions have slot data. This would normally happen when the code for a Node class has been changed to add a new slot.
        for (SlotDefinitionPtr slotDefinition : slotDefinitions)
        {
            SlotId slotId(slotDefinition->GetName());
            if (slotData.find(slotId) == slotData.end())
            {
                AZ_Warning(GetGraph()->GetSystemName(), false, "No data found for slot [%s]. It will be filled with default values.", slotDefinition->GetName().c_str());
                SlotPtr slot = AZStd::make_shared<Slot>(GetGraph(), slotDefinition);
                slotData[slotId] = slot;
            }
        }

        m_allSlots.insert(slotData.begin(), slotData.end());
    }

    void Node::SyncAndSetupExtendableSlots()
    {
        // Do PostLoadSetup to attach each Slot to its SlotDefinition
        // Also remove any Slot that doesn't have a corresponding SlotDefinition
        for (auto slotDataIter = m_extendableSlots.begin(); slotDataIter != m_extendableSlots.end(); /* increment in loop */)
        {
            const SlotName& slotName = slotDataIter->first;

            auto slotDefinitionIter = AZStd::find_if(m_extendableSlotDefinitions.begin(), m_extendableSlotDefinitions.end(), [&slotName](SlotDefinitionPtr slotDefinition) { return slotDefinition->GetName() == slotName; });

            if (slotDefinitionIter == m_extendableSlotDefinitions.end())
            {
                AZ_Warning(GetGraph()->GetSystemName(), false, "Found data for unrecognized slot [%s]. It will be ignored.", slotName.c_str());
                slotDataIter = m_extendableSlots.erase(slotDataIter);
            }
            else
            {
                for (SlotPtr slot : slotDataIter->second)
                {
                    // CJS TODO: Consider using AZ::Outcome for better error reporting, and if PostLoadSetup fails (could be due to type mismatch) 
                    slot->PostLoadSetup(GetGraph(), *slotDefinitionIter);

                    auto newEntry = AZStd::make_pair(slot->GetSlotId(), slot);
                    m_allSlots.insert(newEntry);
                }

                ++slotDataIter;
            }
        }

        // Make sure all SlotDefinitions have slot data. This would normally happen when the code for a Node class has been changed to add a new slot.
        CreateExtendableSlotData();
    }

    NodeId Node::GetId() const
    {
        return m_id;
    }

    bool Node::Contains(ConstSlotPtr slot) const
    {
        if (!slot)
        {
            return false;
        }

        auto iter = m_allSlots.find(slot->GetSlotId());
        if (iter != m_allSlots.end() && iter->second == slot)
        {
            return true;
        }

        return false;
    }

    const Node::SlotDefinitionList& Node::GetSlotDefinitions() const
    {
        return m_allSlotDefinitions;
    }

    const Node::SlotMap& Node::GetSlots()
    {
        return m_allSlots;
    }

    Node::ConstSlotMap Node::GetSlots() const
    {
        Node::ConstSlotMap constSlots;
        AZStd::for_each(m_allSlots.begin(), m_allSlots.end(), [&](auto pair) { constSlots.insert(pair); });
        return constSlots;
    }

    ConstSlotPtr Node::GetSlot(const SlotId& slotId) const
    {
        auto slot = m_allSlots.find(slotId);
        if (slot != m_allSlots.end())
        {
            return slot->second;
        }

        return nullptr;
    }

    SlotPtr Node::GetSlot(const SlotId& slotId)
    {
        // Shared const/non-const overload implementation
        return AZStd::const_pointer_cast<Slot>(static_cast<const Node*>(this)->GetSlot(slotId));
    }

    SlotPtr Node::GetSlot(const SlotName& name)
    {
        SlotId slotId(name);
        return GetSlot(slotId);
    }
    ConstSlotPtr Node::GetSlot(const SlotName& name) const
    {
        SlotId slotId(name);
        return GetSlot(slotId);
    }

    const Node::ExtendableSlotSet& Node::GetExtendableSlots(const SlotName& name)
    {
        auto it = m_extendableSlots.find(name);
        if (it != m_extendableSlots.end())
        {
            return it->second;
        }
        else
        {
            static Node::ExtendableSlotSet defaultSet;
            return defaultSet;
        }
    }

    int Node::GetExtendableSlotCount(const SlotName& name)
    {
        auto it = m_extendableSlots.find(name);
        if (it != m_extendableSlots.end())
        {
            return aznumeric_cast<int>(it->second.size());
        }

        return InvalidExtendableSlot;
    }

    DataTypePtr Node::GetDataType(ConstSlotPtr slot) const
    {
        if (slot)
        {
            // TODO: This method allows Nodes to introduce extra logic when slots
            // ask what their data type should be depending on existing connections.
            // This has been partially implemented, so for now just return the first
            // possible data type.
            auto possibleDataTypes = slot->GetPossibleDataTypes();
            if (possibleDataTypes.size())
            {
                return possibleDataTypes[0];
            }
        }

        return nullptr;
    }

    void Node::DeleteSlot(SlotPtr slot)
    {
        if (CanDeleteSlot(slot))
        {
            SlotId slotId = slot->GetSlotId();

            // Remove this slot from our map tracking all slots on the node, as well as the extendable slots
            auto allSlotsIt = m_allSlots.find(slotId);
            if (allSlotsIt != m_allSlots.end())
            {
                m_allSlots.erase(allSlotsIt);
            }
            auto extendableSlotsIt = m_extendableSlots.find(slot->GetName());
            if (extendableSlotsIt != m_extendableSlots.end())
            {
                auto slotIt = extendableSlotsIt->second.find(slot);
                if (slotIt != extendableSlotsIt->second.end())
                {
                    extendableSlotsIt->second.erase(slotIt);
                }
            }
        }
    }

    bool Node::CanDeleteSlot(ConstSlotPtr slot) const
    {
        // Only extendable slots can be removed
        if (slot->SupportsExtendability())
        {
            int currentNumSlots = 0;
            auto it = m_extendableSlots.find(slot->GetName());
            if (it != m_extendableSlots.end())
            {
                currentNumSlots = aznumeric_cast<int>(it->second.size());
            }

            // Only allow this slot to be deleted if there are more than the required minimum
            int minimumSlots = slot->GetMinimumSlots();
            if (currentNumSlots > minimumSlots)
            {
                return true;
            }
        }

        return false;
    }

    bool Node::CanExtendSlot(SlotDefinitionPtr slotDefinition) const
    {
        if (slotDefinition->SupportsExtendability())
        {
            int currentNumSlots = 0;
            auto it = m_extendableSlots.find(slotDefinition->GetName());
            if (it != m_extendableSlots.end())
            {
                currentNumSlots = aznumeric_cast<int>(it->second.size());
            }

            // Only allow this slot to extended if we haven't reached the maximum
            int maximumSlots = slotDefinition->GetMaximumSlots();
            if (currentNumSlots < maximumSlots)
            {
                return true;
            }
        }

        return false;
    }

    SlotPtr Node::AddExtendedSlot(const SlotName& slotName)
    {
        SlotDefinitionPtr slotDefinition;
        for (auto definition : m_extendableSlotDefinitions)
        {
            if (definition->GetName() == slotName)
            {
                slotDefinition = definition;
                break;
            }
        }

        if (!slotDefinition)
        {
            AZ_Assert(false, "No slot definitions with registered slotName");
            return nullptr;
        }

        if (!CanExtendSlot(slotDefinition))
        {
            return nullptr;
        }

        // Find the existing slots (if any) for this definition, so that we can set the subId
        // for the newly created slot
        auto extendableIt = m_extendableSlots.find(slotName);
        AZ_Assert(extendableIt != m_extendableSlots.end(), "Extendable slot definition name should always exist in the mapping.");
        int newSubId = 0;
        ExtendableSlotSet& currentSlots = extendableIt->second;
        if (!currentSlots.empty())
        {
            newSubId = (*currentSlots.rbegin())->GetSlotId().m_subId + 1;
        }

        SlotPtr slot = AZStd::make_shared<Slot>(GetGraph(), slotDefinition, newSubId);
        slot->SetValue(slotDefinition->GetDefaultValue());

        auto newEntry = AZStd::make_pair(slot->GetSlotId(), slot);
        m_allSlots.insert(newEntry);

        currentSlots.insert(slot);

        return slot;
    }

    void Node::RegisterSlot(SlotDefinitionPtr slotDefinition, SlotDefinitionList& slotDefinitionList)
    {
        AssertPointerIsNew(slotDefinition, m_allSlotDefinitions);
        AssertPointerIsNew(slotDefinition, m_propertySlotDefinitions);
        AssertPointerIsNew(slotDefinition, m_inputDataSlotDefinitions);
        AssertPointerIsNew(slotDefinition, m_outputDataSlotDefinitions);
        AssertPointerIsNew(slotDefinition, m_extendableSlotDefinitions);

        AssertNameIsNew(slotDefinition, m_allSlotDefinitions);
        AssertNameIsNew(slotDefinition, m_propertySlotDefinitions);
        AssertNameIsNew(slotDefinition, m_inputDataSlotDefinitions);
        AssertNameIsNew(slotDefinition, m_outputDataSlotDefinitions);
        AssertNameIsNew(slotDefinition, m_extendableSlotDefinitions);

        // Only check the target list because we could allow the same display name for an input and an output.
        // Only check if DisplayName is not empty, because Name will be used for display in that case.
        if (!slotDefinition->GetDisplayName().empty())
        {
            AssertDisplayNameIsNew(slotDefinition, slotDefinitionList);
        }

        slotDefinitionList.push_back(slotDefinition);
        m_allSlotDefinitions.push_back(slotDefinition);
    }

    void Node::RegisterSlot(SlotDefinitionPtr slotDefinition)
    {
        // [GFX TODO] CJS Consider merging SlotDirection and SlotType into a single enum so we can use switch statements.
        if (slotDefinition->SupportsExtendability())
        {
            RegisterSlot(slotDefinition, m_extendableSlotDefinitions);
        }
        else if (slotDefinition->Is(SlotDirection::Input, SlotType::Data))
        {
            RegisterSlot(slotDefinition, m_inputDataSlotDefinitions);
        }
        else if (slotDefinition->Is(SlotDirection::Output, SlotType::Data))
        {
            RegisterSlot(slotDefinition, m_outputDataSlotDefinitions);
        }
        else if (slotDefinition->Is(SlotDirection::Input, SlotType::Property))
        {
            RegisterSlot(slotDefinition, m_propertySlotDefinitions);
        }
        else if (slotDefinition->Is(SlotDirection::Input, SlotType::Event))
        {
            RegisterSlot(slotDefinition, m_inputEventSlotDefinitions);
        }
        else if (slotDefinition->Is(SlotDirection::Output, SlotType::Event))
        {
            RegisterSlot(slotDefinition, m_outputEventSlotDefinitions);
        }
        else
        {
            AZ_Assert(false, "Unsupported slot configuration");
        }
    }

    void Node::AssertPointerIsNew(SlotDefinitionPtr newSlotDefinition, const SlotDefinitionList& existingSlotDefinitions) const
    {
        auto iter = AZStd::find(existingSlotDefinitions.begin(), existingSlotDefinitions.end(), newSlotDefinition);
        AZ_Assert(iter == existingSlotDefinitions.end(), "This slot has already been registered");
    }

    void Node::AssertNameIsNew(SlotDefinitionPtr newSlotDefinition, const SlotDefinitionList& existingSlotDefinitions) const
    {
        auto iter = AZStd::find_if(existingSlotDefinitions.begin(), existingSlotDefinitions.end(),
            [newSlotDefinition](SlotDefinitionPtr existingSlotDefinition) { return newSlotDefinition->GetName() == existingSlotDefinition->GetName(); });
        AZ_Assert(iter == existingSlotDefinitions.end(), "Another slot with name [%s] already exists", newSlotDefinition->GetName().c_str());
    }

    void Node::AssertDisplayNameIsNew(SlotDefinitionPtr newSlotDefinition, const SlotDefinitionList& existingSlotDefinitions) const
    {
        auto iter = AZStd::find_if(existingSlotDefinitions.begin(), existingSlotDefinitions.end(),
            [newSlotDefinition](SlotDefinitionPtr existingSlotDefinition) { return newSlotDefinition->GetDisplayName() == existingSlotDefinition->GetDisplayName(); });
        AZ_Assert(iter == existingSlotDefinitions.end(), "Another slot with display name [%s] already exists", newSlotDefinition->GetDisplayName().c_str());
    }

} // namespace GraphModel
