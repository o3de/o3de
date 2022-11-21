/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

// AZ
#include <AzCore/Math/Crc.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/smart_ptr/enable_shared_from_this.h>

// Graph Model
#include <GraphModel/Model/Common.h>
#include <GraphModel/Model/GraphElement.h>
#include <GraphModel/Model/Slot.h>

namespace AZ
{
    class ReflectContext;
}

namespace GraphModel
{
    //!!! Start in Graph.h for high level GraphModel documentation !!!

    enum class NodeType
    {
        GeneralNode = 0,
        WrapperNode
    };

    //! The abstract base class for every type of node in a Graph. It consists primarily of a set of
    //! Slots. There is no functionality here beyond managing the Slots and finding Connections. Any 
    //! useful functionality must be provided by subclasses in the client context where the GraphModel 
    //! framework is used.
    //!
    //! Slots are divided into two main objects: a SlotDefinition and the actual Slot.
    //! The SlotDefinition contains the predefined description of each slot that the Node contains. This 
    //! information is not saved with the Node data in the Graph because it is provided by the Node subclass
    //! itself (either hard-coded or reflected from some other source). The Slot is the functional part, 
    //! and contains any data related to specific instance of the Node (for example, the default value
    //! of an Input Data Slot). This data is serialized with the Node. Whenever a Node is created, either 
    //! directly or by serializing in, the Node class ensures that Slots are created for each SlotDefinition 
    //! defined by the Node subclass.
    //!
    //! Every Slot in the Node has a SlotId, which is unique within the context of the Node. 
    //! A specific Slot in a spectific Node is called an Endpoint and is identified by a pairing of
    //! NodeId and SlotId.
    //!
    //! Subclasses must call RegisterSlot() to define the Node's inputs and outputs, but it shouldn't need 
    //! to serialize any of its own data. This base class's Slot lists are reflected for serialization, and 
    //! that's all that should be needed in most cases.
    class Node : public GraphElement, public AZStd::enable_shared_from_this<Node>
    {
        friend class Graph; // Because the Graph needs to set the ID, but no one else should be able to.

    public:
        AZ_CLASS_ALLOCATOR(Node, AZ::SystemAllocator, 0);
        AZ_RTTI(Node, "{274B4495-FDBF-45A9-9BAD-9E90269F2B73}", GraphElement);
        static void Reflect(AZ::ReflectContext* context);

        static const int INVALID_NODE_ID = 0;

        using SlotDefinitionList = AZStd::vector<SlotDefinitionPtr>;

        // We use a map instead of unordered_map to get consistent order.
        using SlotMap = AZStd::map<SlotId, SlotPtr>;
        using ConstSlotMap = AZStd::map<SlotId, const SlotPtr>;

        // Special case mapping for holding the extendable slots so we can keep track of their indexed
        // order as well.
        struct SortSlotsBySubId
        {
            AZ_TYPE_INFO(SortSlotsBySubId, "{01ED3FF5-0DE4-4B25-84FA-8763EB05FAFE}");
            
            SortSlotsBySubId() = default;

            bool operator()(const SlotPtr& left, const SlotPtr& right) const
            {
                return left->GetSlotSubId() < right->GetSlotSubId();
            }
        };
        using ExtendableSlotSet = AZStd::set<SlotPtr, SortSlotsBySubId>;
        using ExtendableSlotMap = AZStd::map<SlotName, ExtendableSlotSet>;

        Node() = default; // Needed by SerializeContext
        
        //! Constructor
        //! \param graph  The Graph that will own this Node (though constructing the Node doesn't actually add it to this graph yet).
        explicit Node(GraphPtr graph);

        //! Initializion after the Node has been serialized in.
        //! This must be called whenever the default constructor is used.
        //! Sets the m_graph pointer and caches pointers to other GraphElements.
        //! It also ensures the loaded Slot data aligns with the defined SlotDefinitions.
        virtual void PostLoadSetup(GraphPtr graph, NodeId id);

        //! An alternative to the above PostLoadSetup when the
        //! nodeId isn't already known (e.g. a deserialized node that has been copy/pasted)
        virtual void PostLoadSetup();

        //! Returns the name that will be displayed as the title of the Node in the UI
        virtual const char* GetTitle() const = 0;

        //! Returns the name that will be displayed as the sub-title of the Node in the UI
        virtual const char* GetSubTitle() const;

        //! Returns node type (general by default) which can be overriden for
        //! other types, such as wrapper nodes
        virtual NodeType GetNodeType() const;

        NodeId GetId() const;

        //! Return the greatest distance, number of connected nodes, between this node and other root nodes.
        uint32_t GetMaxInputDepth() const;

        //! Return the greatest distance, number of connected nodes, between this node and other leaf nodes.
        uint32_t GetMaxOutputDepth() const;

        //! Return true if this node contains any slots.
        bool HasSlots() const;

        //! Return true if this node contains any input slots.
        bool HasInputSlots() const;

        //! Return true if this node contains any output slots.
        bool HasOutputSlots() const;

        //! Returns true if the graph contains any connections referencing this node.
        bool HasConnections() const;

        //! Returns true if the graph has any connections to input slots on this node.
        bool HasInputConnections() const;

        //! Returns true if the graph has any connections to output slots on this node.
        bool HasOutputConnections() const;

        //! Returns true if any of the input slots on this node have direct or indirect connections to output slots on the specified node.
        bool HasInputConnectionFromNode(ConstNodePtr node) const;

        //! Returns true if any of the output slots on this node have direct or indirect connections to input slots on the specified node.
        bool HasOutputConnectionToNode(ConstNodePtr node) const;

        //! Returns true if this node contains the specified slot.
        bool Contains(ConstSlotPtr slot) const;

        //! Returns SlotDefinitions for all available Slots
        const SlotDefinitionList& GetSlotDefinitions() const;

        //! Returns the map of all available Slots.
        //! For the generic case, there will be one Slot for every SlotDefinition in GetSlotDefinitions().
        //! Additionally, for extendable slots there could be 0 or more Slots per SlotDefinition
        const SlotMap& GetSlots();
        ConstSlotMap GetSlots() const;

        //! Returns the slot with the given slotId, or nullptr if it doesn't exist
        SlotPtr GetSlot(const SlotId& slotId);
        ConstSlotPtr GetSlot(const SlotId& slotId) const;

        //! Returns the slot with the given SlotName, or nullptr if it doesn't exist.
        //! This is a simplified version for normal (non-extendable) slots. It is equivalent to calling GetSlot
        //! with the given SlotName and a subId of 0. If the slot is actually extendable, it
        //! will return the first indexed slot if it exists.
        SlotPtr GetSlot(const SlotName& name);
        ConstSlotPtr GetSlot(const SlotName& name) const;

        //! Returns an ordered set of the extendable slots for a given SlotName, or an empty set if there are none
        const ExtendableSlotSet& GetExtendableSlots(const SlotName& name);

        //! Returns the number of extendable slots for a given SlotName.
        //! Will return -1 if the specified slot is not extendable.
        int GetExtendableSlotCount(const SlotName& name) const;

        //! Delete the specified slot, which is only allowed on extendable slots.
        //! This method does nothing if the slot is not extendable.
        void DeleteSlot(SlotPtr slot);

        //! Check if the specified slot can be deleted, which is restricted by
        //!  - The slot must be extendable
        //!  - Deleting the slot can't reduce the number of extendable slots below the configured minimum
        //!    number of allowed slots for this definition
        //! This method is also virtual so the client can impose any custom limitations as needed.
        //! This method does nothing if the slot is not extendable.
        virtual bool CanDeleteSlot(ConstSlotPtr slot) const;

        //! Append a new slot to an extendable slot list.
        //! This is restricted such that
        //!   - The slot definition must be extendable
        //!   - Creating a new slot can't increase the number of extendable slots above the configured maximum
        //!    number of allowed slots for this definition
        //! This method does nothing if the slot is not extendable.
        virtual SlotPtr AddExtendedSlot(const SlotName& slotName);

    protected:

        //! Default implementation will prevent slots from being extended past the
        //! maximum allowed configuration, but the client could override this to impose
        //! additional restrictions
        virtual bool CanExtendSlot(SlotDefinitionPtr slotDefinition) const;

        //! Subclasses should call this function during construction to define its slots.
        //! The slot's name must be unique among all slots in this node.
        void RegisterSlot(SlotDefinitionPtr slotDefinition);

        //! Overridden by specific nodes to register their necessary slots
        //! This is called automatically by this Node base class when PostLoadSetup is called,
        //! which occurs after a Node has been deserialized. The derived classes are stil in charge
        //! of calling it in their Node(GraphPtr graph) constructor, since their overrides aren't
        //! accessible from the base class during construction.
        virtual void RegisterSlots() {}

        //! Once a subclass is done calling RegisterSlot(), it can call this function to 
        //! instantiate all the slot data. This should only be done when creating a new 
        //! Node, not when loading a Node from serialize data (in that case Slot creation 
        //! will be handled automatically by PostLoadSetup()).
        void CreateSlotData();

    private:

        //! Common implementation for RegisterSlot() to a specific SlotDefinitionList
        void RegisterSlot(SlotDefinitionPtr slotDefinition, SlotDefinitionList& slotDefinitionList);

        //! Common implementation for CreateSlotData() to a specific SlotMap
        void CreateSlotData(SlotMap& slotMap, const SlotDefinitionList& slotDefinitionList);

        //! Specific implementation for creating slot data for extendable slots
        void CreateExtendableSlotData();

        //! This is a substep in the PostLoadSetup() process. It ensures that the set of loaded Slot
        //! data aligns with the Node's pre-established SlotDefinitions, and calls PostLoadSetup()
        //! on each Slot.
        //! \param slotData  The collection of loaded Slot object data (m_inputDataSlots, m_outputDataSlots, etc)
        //! \param slotDefinitions  The set of estalished SlotDefinitions that defines the Node's set of available slots
        void SyncAndSetupSlots(SlotMap& slotData, Node::SlotDefinitionList& slotDefinitions);

        //! Special case of above method for the extendable slots, which are stored in a different mapping
        void SyncAndSetupExtendableSlots();

        //! Assertions for use during slot registration, to prevent duplicates
        void AssertPointerIsNew(SlotDefinitionPtr newSlotDefinition, const SlotDefinitionList& existingSlotDefinitions) const;
        void AssertNameIsNew(SlotDefinitionPtr newSlotDefinition, const SlotDefinitionList& existingSlotDefinitions) const;
        void AssertDisplayNameIsNew(SlotDefinitionPtr newSlotDefinition, const SlotDefinitionList& existingSlotDefinitions) const;

        NodeId m_id = INVALID_NODE_ID;

        // These are what will actually be serialized. That way we don't have to make every node sub-type 
        // provide its own reflection; it just puts the slots in these lists via RegisterSlot() and CreateSlotData().
        // These are stored in separate maps rather than just one because some of these slot types don't need to be
        // reflected to SerializeContext.
        SlotMap m_propertySlots;    //!< For slots with configuration = SlotDirection::Input  SlotType::Property
        SlotMap m_inputDataSlots;   //!< For slots with configuration = SlotDirection::Input  SlotType::Data
        SlotMap m_outputDataSlots;  //!< For slots with configuration = SlotDirection::Output SlotType::Data
        SlotMap m_inputEventSlots;  //!< For slots with configuration = SlotDirection::Input  SlotType::Event
        SlotMap m_outputEventSlots; //!< For slots with configuration = SlotDirection::Output SlotType::Event
        ExtendableSlotMap m_extendableSlots;  //!< For all extendable slots, regardless of configuration, since we need to serialize all of them
        SlotMap m_allSlots; //!< Provies a single list of all of the above SlotMaps for convenient looping over them all

        // These are not serialized; they're definitions of the slots that are part of the Node type 
        // definition so saving this data is unnecessary.
        SlotDefinitionList m_propertySlotDefinitions;    //!< For slots with configuration = SlotDirection::Input  SlotType::Property
        SlotDefinitionList m_inputDataSlotDefinitions;   //!< For slots with configuration = SlotDirection::Input  SlotType::Data
        SlotDefinitionList m_outputDataSlotDefinitions;  //!< For slots with configuration = SlotDirection::Output SlotType::Data
        SlotDefinitionList m_inputEventSlotDefinitions;  //!< For slots with configuration = SlotDirection::Input  SlotType::Event
        SlotDefinitionList m_outputEventSlotDefinitions; //!< For slots with configuration = SlotDirection::Output SlotType::Event
        SlotDefinitionList m_extendableSlotDefinitions;  //!< For all extendable slot configurations
        SlotDefinitionList m_allSlotDefinitions; //!< Provies a single list of all of the above SlotDefinitionLists for convenient looping over them all
    };

} // namespace GraphModel
