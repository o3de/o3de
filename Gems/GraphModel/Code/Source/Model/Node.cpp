/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// AZ
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>

// Graph Model
#include <GraphModel/Model/Connection.h>
#include <GraphModel/Model/Graph.h>
#include <GraphModel/Model/Node.h>
#include <GraphModel/Model/Slot.h>

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

        // Moving slots between input data slot and property slot containers in case the layout of the slot definitions change.
        for (const auto& def : m_inputDataSlotDefinitions)
        {
            for (const auto& slot : m_propertySlots)
            {
                if (def->GetName() == slot.first.m_name)
                {
                    m_inputDataSlots.insert(slot);
                    m_propertySlots.erase(slot.first);
                    break;
                }
            }
        }

        for (const auto& def : m_propertySlotDefinitions)
        {
            for (const auto& slot : m_inputDataSlots)
            {
                if (def->GetName() == slot.first.m_name)
                {
                    m_propertySlots.insert(slot);
                    m_inputDataSlots.erase(slot.first);
                    break;
                }
            }
        }

        // Make sure the loaded Slot data aligns with the Node's input slot descriptions
        SyncAndSetupSlots(m_propertySlots, m_propertySlotDefinitions);
        SyncAndSetupSlots(m_inputDataSlots, m_inputDataSlotDefinitions);
        SyncAndSetupExtendableSlots();

        // These slots types are a bit different, because they don't actually have any data to be serialized, so instead of SyncAndSetupSlots we need to create them.
        CreateSlotData(m_outputDataSlots, m_outputDataSlotDefinitions);
        CreateSlotData(m_inputEventSlots, m_inputEventSlotDefinitions);
        CreateSlotData(m_outputEventSlots, m_outputEventSlotDefinitions);

        [[maybe_unused]] int numExtendableSlots = 0;
        for (const auto& slotPair : m_extendableSlots)
        {
            numExtendableSlots += aznumeric_cast<int>(slotPair.second.size());
        }

        AZ_Assert(m_allSlots.size() == m_propertySlots.size() + m_inputDataSlots.size() + m_outputDataSlots.size() + m_inputEventSlots.size() + m_outputEventSlots.size() + numExtendableSlots, "Slot counts don't match");
        AZ_Assert(m_allSlotDefinitions.size() == m_propertySlotDefinitions.size() + m_inputDataSlotDefinitions.size() + m_outputDataSlotDefinitions.size() + m_inputEventSlotDefinitions.size() + m_outputEventSlotDefinitions.size() + m_extendableSlotDefinitions.size(), "SlotDefinition counts don't match");
    }

    //! Returns the name that will be displayed as the sub-title of the Node in the UI

    const char* Node::GetSubTitle() const
    {
        return "";
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

                m_allSlots.insert(AZStd::make_pair(slot->GetSlotId(), slot));
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

            auto slotDefinitionIter = AZStd::find_if(
                slotDefinitions.begin(),
                slotDefinitions.end(),
                [&slotName](SlotDefinitionPtr slotDefinition)
                {
                    return slotDefinition->GetName() == slotName;
                });

            if (slotDefinitionIter == slotDefinitions.end())
            {
                AZ_Warning(
                    GetGraph()->GetSystemName(), false, "Found data for unrecognized slot [%s]. It will be ignored.", slotName.c_str());
                slotDataIter = slotData.erase(slotDataIter);
            }
            else
            {
                // If PostLoadSetup fails (could be due to type mismatch)
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

            auto slotDefinitionIter = AZStd::find_if(
                m_extendableSlotDefinitions.begin(),
                m_extendableSlotDefinitions.end(),
                [&slotName](SlotDefinitionPtr slotDefinition)
                {
                    return slotDefinition->GetName() == slotName;
                });

            if (slotDefinitionIter == m_extendableSlotDefinitions.end())
            {
                AZ_Warning(
                    GetGraph()->GetSystemName(), false, "Found data for unrecognized slot [%s]. It will be ignored.", slotName.c_str());
                slotDataIter = m_extendableSlots.erase(slotDataIter);
            }
            else
            {
                for (SlotPtr slot : slotDataIter->second)
                {
                    // If PostLoadSetup fails (could be due to type mismatch)
                    slot->PostLoadSetup(GetGraph(), *slotDefinitionIter);
                    m_allSlots.insert(AZStd::make_pair(slot->GetSlotId(), slot));
                }

                ++slotDataIter;
            }
        }

        // Make sure all SlotDefinitions have slot data. This would normally happen when the code for a Node class has been changed to add a new slot.
        CreateExtendableSlotData();
    }

    //! Returns node type (general by default) which can be overriden for other types, such as wrapper nodes
    NodeType Node::GetNodeType() const
    {
        return NodeType::GeneralNode;
    }

    NodeId Node::GetId() const
    {
        return m_id;
    }

    uint32_t Node::GetMaxInputDepth() const
    {
        uint32_t maxInputDepth = 0;
        AZStd::for_each(m_allSlots.begin(), m_allSlots.end(), [&](const auto& slotPair) {
            const auto& slot = slotPair.second;
            if (slot->GetSlotDirection() == GraphModel::SlotDirection::Input)
            {
                const auto& connections = slot->GetConnections();
                AZStd::for_each(connections.begin(), connections.end(), [&](const auto& connection) {
                    AZ_Assert(connection->GetSourceNode().get() != this, "This should never be the source node on an input connection.");
                    AZ_Assert(connection->GetTargetNode().get() == this, "This should always be the target node on an input connection.");
                    maxInputDepth = AZStd::max(maxInputDepth, connection->GetSourceNode()->GetMaxInputDepth() + 1);
                });
            }
        });
        return maxInputDepth;
    }

    uint32_t Node::GetMaxOutputDepth() const
    {
        uint32_t maxOutputDepth = 0;
        AZStd::for_each(m_allSlots.begin(), m_allSlots.end(), [&](const auto& slotPair) {
            const auto& slot = slotPair.second;
            if (slot->GetSlotDirection() == GraphModel::SlotDirection::Output)
            {
                const auto& connections = slot->GetConnections();
                AZStd::for_each(connections.begin(), connections.end(), [&](const auto& connection) {
                    AZ_Assert(connection->GetSourceNode().get() == this, "This should always be the source node on an output connection.");
                    AZ_Assert(connection->GetTargetNode().get() != this, "This should never be the target node on an output connection.");
                    maxOutputDepth = AZStd::max(maxOutputDepth, connection->GetTargetNode()->GetMaxOutputDepth() + 1);
                });
            }
        });
        return maxOutputDepth;
    }

    bool Node::HasSlots() const
    {
        return !m_allSlots.empty();
    }

    bool Node::HasInputSlots() const
    {
        return AZStd::any_of(m_allSlots.begin(), m_allSlots.end(), [](const auto& slotPair) {
            const auto& slot = slotPair.second;
            return slot->GetSlotDirection() == GraphModel::SlotDirection::Input;
        });
    }

    bool Node::HasOutputSlots() const
    {
        return AZStd::any_of(m_allSlots.begin(), m_allSlots.end(), [](const auto& slotPair) {
            const auto& slot = slotPair.second;
            return slot->GetSlotDirection() == GraphModel::SlotDirection::Output;
        });
    }

    bool Node::HasConnections() const
    {
        return AZStd::any_of(m_allSlots.begin(), m_allSlots.end(), [](const auto& slotPair) {
            const auto& slot = slotPair.second;
            return !slot->GetConnections().empty();
        });
    }

    bool Node::HasInputConnections() const
    {
        return AZStd::any_of(m_allSlots.begin(), m_allSlots.end(), [](const auto& slotPair) {
            const auto& slot = slotPair.second;
            return slot->GetSlotDirection() == GraphModel::SlotDirection::Input && !slot->GetConnections().empty();
        });
    }

    bool Node::HasOutputConnections() const
    {
        return AZStd::any_of(m_allSlots.begin(), m_allSlots.end(), [](const auto& slotPair) {
            const auto& slot = slotPair.second;
            return slot->GetSlotDirection() == GraphModel::SlotDirection::Output && !slot->GetConnections().empty();
        });
    }

    bool Node::HasInputConnectionFromNode(ConstNodePtr node) const
    {
        return AZStd::any_of(m_allSlots.begin(), m_allSlots.end(), [&](const auto& slotPair) {
            const auto& slot = slotPair.second;
            const auto& connections = slot->GetConnections();
            return slot->GetSlotDirection() == GraphModel::SlotDirection::Input &&
                AZStd::any_of(connections.begin(), connections.end(), [&](const auto& connection) {
                    AZ_Assert(connection->GetSourceNode().get() != this, "This should never be the source node on an input connection.");
                    AZ_Assert(connection->GetTargetNode().get() == this, "This should always be the target node on an input connection.");
                    return connection->GetSourceNode() == node || connection->GetSourceNode()->HasInputConnectionFromNode(node);
                });
        });
    }

    bool Node::HasOutputConnectionToNode(ConstNodePtr node) const
    {
        return AZStd::any_of(m_allSlots.begin(), m_allSlots.end(), [&](const auto& slotPair) {
            const auto& slot = slotPair.second;
            const auto& connections = slot->GetConnections();
            return slot->GetSlotDirection() == GraphModel::SlotDirection::Output &&
                AZStd::any_of(connections.begin(), connections.end(), [&](const auto& connection) {
                    AZ_Assert(connection->GetSourceNode().get() == this, "This should always be the source node on an output connection.");
                    AZ_Assert(connection->GetTargetNode().get() != this, "This should never be the target node on an output connection.");
                    return connection->GetTargetNode() == node || connection->GetTargetNode()->HasOutputConnectionToNode(node);
                });
        });
    }

    bool Node::Contains(ConstSlotPtr slot) const
    {
        const auto slotItr = slot ? m_allSlots.find(slot->GetSlotId()) : m_allSlots.end();
        return slotItr != m_allSlots.end() && slotItr->second == slot;
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
        return ConstSlotMap(m_allSlots.begin(), m_allSlots.end());
    }

    ConstSlotPtr Node::GetSlot(const SlotId& slotId) const
    {
        const auto slotItr = m_allSlots.find(slotId);
        return slotItr != m_allSlots.end() ? slotItr->second : nullptr;
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
        static Node::ExtendableSlotSet defaultSet;
        const auto slotItr = m_extendableSlots.find(name);
        return slotItr != m_extendableSlots.end() ? slotItr->second : defaultSet;
    }

    int Node::GetExtendableSlotCount(const SlotName& name) const
    {
        const auto slotItr = m_extendableSlots.find(name);
        return slotItr != m_extendableSlots.end() ? aznumeric_cast<int>(slotItr->second.size()) : InvalidExtendableSlot;
    }

    void Node::DeleteSlot(SlotPtr slot)
    {
        if (CanDeleteSlot(slot))
        {
            // Remove this slot from our map tracking all slots on the node, as well as the extendable slots
            m_allSlots.erase(slot->GetSlotId());
            auto extendableSlotsIt = m_extendableSlots.find(slot->GetName());
            if (extendableSlotsIt != m_extendableSlots.end())
            {
                extendableSlotsIt->second.erase(slot);
            }
        }
    }

    bool Node::CanDeleteSlot(ConstSlotPtr slot) const
    {
        // Only extendable slots can be removed
        if (slot->SupportsExtendability())
        {
            // Only allow this slot to be deleted if there are more than the required minimum
            const int currentNumSlots = GetExtendableSlotCount(slot->GetName());
            return currentNumSlots >= 0 && currentNumSlots > slot->GetMinimumSlots();
        }

        return false;
    }

    bool Node::CanExtendSlot(SlotDefinitionPtr slotDefinition) const
    {
        if (slotDefinition->SupportsExtendability())
        {
            // Only allow this slot to extended if we haven't reached the maximum
            const int currentNumSlots = GetExtendableSlotCount(slotDefinition->GetName());
            return currentNumSlots >= 0 && currentNumSlots < slotDefinition->GetMaximumSlots();
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

        m_allSlots.insert(AZStd::make_pair(slot->GetSlotId(), slot));
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

    void Node::AssertPointerIsNew([[maybe_unused]] SlotDefinitionPtr newSlotDefinition, [[maybe_unused]] const SlotDefinitionList& existingSlotDefinitions) const
    {
#if defined(AZ_ENABLE_TRACING)
        auto slotItr = AZStd::find(existingSlotDefinitions.begin(), existingSlotDefinitions.end(), newSlotDefinition);
        AZ_Assert(slotItr == existingSlotDefinitions.end(), "This slot has already been registered");
#endif
    }

    void Node::AssertNameIsNew([[maybe_unused]] SlotDefinitionPtr newSlotDefinition, [[maybe_unused]] const SlotDefinitionList& existingSlotDefinitions) const
    {
#if defined(AZ_ENABLE_TRACING)
        auto slotItr = AZStd::find_if(existingSlotDefinitions.begin(), existingSlotDefinitions.end(),
            [newSlotDefinition](SlotDefinitionPtr existingSlotDefinition) { return newSlotDefinition->GetName() == existingSlotDefinition->GetName(); });
        AZ_Assert(slotItr == existingSlotDefinitions.end(), "Another slot with name [%s] already exists", newSlotDefinition->GetName().c_str());
#endif
    }

    void Node::AssertDisplayNameIsNew([[maybe_unused]] SlotDefinitionPtr newSlotDefinition, [[maybe_unused]] const SlotDefinitionList& existingSlotDefinitions) const
    {
#if defined(AZ_ENABLE_TRACING)
        auto slotItr = AZStd::find_if(existingSlotDefinitions.begin(), existingSlotDefinitions.end(),
            [newSlotDefinition](SlotDefinitionPtr existingSlotDefinition) { return newSlotDefinition->GetDisplayName() == existingSlotDefinition->GetDisplayName(); });
        AZ_Assert(slotItr == existingSlotDefinitions.end(), "Another slot with display name [%s] already exists", newSlotDefinition->GetDisplayName().c_str());
#endif
    }

} // namespace GraphModel
