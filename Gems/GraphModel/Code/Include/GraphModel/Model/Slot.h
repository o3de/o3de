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
#pragma once

// AZ
#include <AzCore/std/any.h>
#include <AzCore/std/hash.h>
#include <AzCore/std/containers/list.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/smart_ptr/enable_shared_from_this.h>

// Graph Model
#include <GraphModel/Model/Common.h>
#include <GraphModel/Model/GraphElement.h>
#include <GraphModel/Model/IGraphContext.h>

namespace GraphModel
{
    //!!! Start in Graph.h for high level GraphModel documentation !!!

    //! All slots have a SlotType and a SlotDirection. This combination determines the slot's available features.
    enum class SlotType
    {
        Invalid,
        Data,     //!< Has a DataType and can be connected to other Data slots. Input Data slot has a default value for when it is not connected.
        Event,    //!< Has no DataType. Represents an event sent or received. Can be connected to other Event slots.
        Property  //!< Has a DataType and a value. Cannot be connected to other slots. SlotDirection must be Input.
    };

    //! All slots have a SlotType and a SlotDirection. This combination determines the slot's available features.
    enum class SlotDirection
    {
        Invalid,
        Input,   //!< Represents information consumed by the node, usually appearing on the left side.
        Output   //!< Represents information produced by the node, usually appearing on the right side.
    };

    //! The sub ID is only used for extendable slots that can support
    //! multiple slots of the same definition, where the sub ID is a
    //! counter, not an index of the current slots
    using SlotName = AZStd::string;
    using SlotSubId = int;
    struct SlotIdData
    {
    public:
        AZ_TYPE_INFO(SlotIdData, "{D24130B9-89C4-4EAA-9A5D-3469B05C5065}");

        static void Reflect(AZ::ReflectContext* context);

        SlotIdData() = default;
        explicit SlotIdData(const SlotName& name);
        explicit SlotIdData(const SlotName& name, SlotSubId subId);
        ~SlotIdData() = default;

        bool IsValid() const;

        bool operator==(const SlotIdData& rhs) const;
        bool operator!=(const SlotIdData& rhs) const;
        bool operator<(const SlotIdData& rhs) const;
        bool operator>(const SlotIdData& rhs) const;

        AZStd::size_t GetHash() const;

        SlotName m_name;
        SlotSubId m_subId = 0;
    };

    struct ExtendableSlotConfiguration
    {
    public:
        AZ_TYPE_INFO(ExtendableSlotConfiguration, "{ACD18AD2-AD90-408C-9C11-920C2A8D77EC}");
        AZ_CLASS_ALLOCATOR(ExtendableSlotConfiguration, AZ::SystemAllocator, 0);

        ExtendableSlotConfiguration() = default;
        ~ExtendableSlotConfiguration() = default;

        bool m_isValid = false;             //!< Flag to determine if this extendable slot configuration is valid
        AZStd::string m_addButtonLabel;     //!< Label for the button for adding new extendable slots
        AZStd::string m_addButtonTooltip;   //!< Tooltip for the button for adding new extendable slots
        int m_minimumSlots = 1;
        int m_maximumSlots = 100;
    };
    
    //! Provides static information about a Slot, like its name and data type.
    //! The set of features provided by this slot is determined by the combination 
    //! of SlotDirection and SlotType, which is set depending on which Create* function
    //! is used to create the SlotDefinition.
    //!
    //! This information will either be hard-coded for each Node type, or
    //! reflected from some other source, so it does not need to be saved with
    //! the Node data (i.e. it isn't added to a SerializeContext). 
    //!
    //! See the Node class documentation for more.
    //!
    //! (We take the approach of using a single class with some features unused
    //! in specific configurations because it ends up being cleaner than a complex
    //! class hierarchy).
    class SlotDefinition
    {
    public:
        AZ_CLASS_ALLOCATOR(SlotDefinition, AZ::SystemAllocator, 0);
        AZ_RTTI(SlotDefinition, "{917F9C1A-1513-4694-B25A-D6404A4991ED}");

        SlotDefinition() = default;
        virtual ~SlotDefinition() = default;

        //! This set of factory functions create a SlotDefinition for each of the valid SlotDirection/SlotType combinations
        static SlotDefinitionPtr CreateInputData(AZStd::string_view name, AZStd::string_view displayName, DataTypePtr dataType, AZStd::any defaultValue, AZStd::string_view description, ExtendableSlotConfiguration* extendableSlotConfiguration = nullptr);
        static SlotDefinitionPtr CreateInputData(AZStd::string_view name, AZStd::string_view displayName, DataTypeList supportedDataTypes, AZStd::any defaultValue, AZStd::string_view description, ExtendableSlotConfiguration* extendableSlotConfiguration = nullptr);
        static SlotDefinitionPtr CreateOutputData(AZStd::string_view name, AZStd::string_view displayName, DataTypePtr dataType, AZStd::string_view description, ExtendableSlotConfiguration* extendableSlotConfiguration = nullptr);
        static SlotDefinitionPtr CreateInputEvent(AZStd::string_view name, AZStd::string_view displayName, AZStd::string_view description, ExtendableSlotConfiguration* extendableSlotConfiguration = nullptr);
        static SlotDefinitionPtr CreateOutputEvent(AZStd::string_view name, AZStd::string_view displayName, AZStd::string_view description, ExtendableSlotConfiguration* extendableSlotConfiguration = nullptr);
        static SlotDefinitionPtr CreateProperty(AZStd::string_view name, AZStd::string_view displayName, DataTypePtr dataType, AZStd::any defaultValue, AZStd::string_view description, ExtendableSlotConfiguration* extendableSlotConfiguration = nullptr);

        SlotDirection GetSlotDirection() const;  
        SlotType GetSlotType() const;        

        //! Returns whether slot value is relevent to this slot's configuration
        bool SupportsValue() const;

        //! Returns whether slot data type is relevent to this slot's configuration
        bool SupportsDataType() const;

        //! Returns whether this slot's configuration allows connections to other slots
        bool SupportsConnections() const;

        //! Returns whether this slot matches the given configuration
        bool Is(SlotDirection slotDirection, SlotType slotType) const;

        //! Returns whether or not this slot is configured to be extendable
        bool SupportsExtendability() const;

        const SlotName& GetName() const;                    //!< Valid for all slot configurations
        const AZStd::string& GetDisplayName() const;        //!< Valid for all slot configurations
        const AZStd::string& GetDescription() const;        //!< Valid for all slot configurations
        const DataTypeList& GetSupportedDataTypes() const;  //!< Valid for Data and Property slots. Otherwise returns an empty DataTypeList.
        AZStd::any GetDefaultValue() const;                 //!< Valid for Input Data and Property slots. Otherwise returns an empty AZStd::any.

        //! These methods are only pertinent for extendable slots
        const int GetMinimumSlots() const;                  //!< Retrieve the minimum configured number of extendable slots (returns a default value if not configured)
        const int GetMaximumSlots() const;                  //!< Retrieve the maximum configured number of extendable slots (returns a default value if not configured)
        const AZStd::string& GetExtensionLabel() const;     //!< Retrieve the text for the label with the '+' sign for adding extendable slots
        const AZStd::string& GetExtensionTooltip() const;   //!< Retrieve the hover tooltip for the label with the '+' sign for adding extendable slots

    private:
        //! Helper method for handling the assignment/registration of the extendable slot configuration for a definition.
        static void HandleExtendableSlotRegistration(AZStd::shared_ptr<SlotDefinition> slotDefinition, ExtendableSlotConfiguration* extendableSlotConfiguration);

        SlotDirection m_slotDirection = SlotDirection::Invalid;
        SlotType m_slotType = SlotType::Invalid;
        SlotName m_name;
        AZStd::string m_displayName;
        AZStd::string m_description;
        DataTypeList m_supportedDataTypes;
        AZStd::any m_defaultValue;
        ExtendableSlotConfiguration m_extendableSlotConfiguration;
    };


    //!!! Start in Graph.h for high level GraphModel documentation !!!

    //! Represents the instance of a slot, based on a specific SlotDefinition.
    //! If you think of the SlotDefinition as a class declaration, then a Slot is like an
    //! instance of that class. Slots may contain data like default values and connections
    //! to other Slots. The specific set of supported features is determined by the 
    //! SlotDefinition's combination of SlotType and SlotDirection.
    //!
    //! (We take the approach of using a single class with some features unused
    //! in specific configurations because it ends up being cleaner than a complex
    //! class hierarchy).
    class Slot : public GraphElement, public AZStd::enable_shared_from_this<Slot>
    {
        friend class Graph; // So the Graph can update the Slot's cache of Connection pointers

    public:
        AZ_CLASS_ALLOCATOR(Slot, AZ::SystemAllocator, 0);
        AZ_RTTI(Slot, "{50494867-04F1-4785-BB9C-9D6C96DCBFC9}", GraphElement);
        static void Reflect(AZ::ReflectContext* context);

        using ConnectionList = AZStd::set<AZStd::shared_ptr<Connection>>; // AZStd::unordered_set doesn't work with shared_ptr so use set
        using WeakConnectionList = AZStd::list<AZStd::weak_ptr<Connection>>; // AZStd::set doesn't work with weak_ptr so use list

        Slot() = default; // Needed by SerializeContext
        ~Slot() override = default;

        //! Constructor
        //! \param graph  The Graph that will own this Slot
        //! \param slotDefinition  The descriptor that defines this Slot
        //! \param subId  The subId that is used to identify extendable slots
        Slot(GraphPtr graph, SlotDefinitionPtr slotDefinition, SlotSubId subId = 0);

        //! Initializion after the Slot has been serialized in.
        //! This must be called whenever the default constructor is used.
        //! Sets the m_graph pointer and caches pointers to other GraphElements.
        virtual void PostLoadSetup(GraphPtr graph, SlotDefinitionPtr slotDefinition);

        //! Return the SlotDefinition that defines this Slot
        SlotDefinitionPtr GetDefinition() const; 

        //! Convenience functions that wrap SlotDefinition accessors
        bool Is(SlotDirection slotDirection, SlotType slotType) const;
        SlotDirection GetSlotDirection() const;
        SlotType GetSlotType() const;
        bool SupportsValue() const;
        bool SupportsDataType() const;
        bool SupportsConnections() const;
        bool SupportsExtendability() const;
        const SlotName& GetName() const;                    //!< Valid for all slot configurations
        const AZStd::string& GetDisplayName() const;        //!< Valid for all slot configurations
        const AZStd::string& GetDescription() const;        //!< Valid for all slot configurations
        DataTypePtr GetDataType() const;                    //!< Valid for Data and Property slots. Otherwise returns null.
        AZStd::any GetDefaultValue() const;                 //!< Valid for Input Data and Property slots. Otherwise returns an empty AZStd::any.

        //! Valid for Data and Property slots. Otherwise returns an empty DataTypeList.
        //! If valid, this will return the full list of all data types this slot could support.
        const DataTypeList& GetSupportedDataTypes() const;

        //! Valid for Data and Property slots. Otherwise returns an empty DataTypeList.
        //! If valid, this will return the subset of data types that this slot can currently accept
        //! based on the configuration of the node this slot belongs to and/or connections to other slots on the node.
        const DataTypeList& GetPossibleDataTypes() const;

        //! Convenience functions that wrap SlotDefinition accessors (specific to definitions that support extendable slots)
        const int GetMinimumSlots() const;
        const int GetMaximumSlots() const;
        SlotId GetSlotId() const;
        SlotSubId GetSlotSubId() const;

        //! Get the Node that contains this Slot. 
        //! This function cannot be called until this Slot is added to a Node and
        //! that Node is added to the Graph.
        NodePtr GetParentNode() const;

        //! Return the slot's value, which will be used if there are no input connections.
        //! Valid for Input Data and Property slots.
        AZStd::any GetValue() const;

        //! Return the slot's value, which will be used if there are no input connections. Returns 0 if the type doesn't match.
        //! Type template type T must match the slot's data type.
        //! Valid for Input Data and Property slots.
        template<typename T>
        const T& GetValue() const;

        //! Sets the slot's value, which will be used if there are no input connections.
        //! Type template type T must match the slot's data type.
        //! Valid for Input Data and Property slots.
        template<typename T>
        void SetValue(const T& value);

        //! Sets the slot's value, which will be used if there are no input connections.
        //! AZStd::any type must match the slot's data type.
        //! Valid for Input Data and Property slots.
        void SetValue(const AZStd::any& value);


        // CJS TODO: More functions to add a bit later...
        // CJS TODO: Also cache connection information here so Slot doesn't have to search the Graph for connections

        //! Whether it's connected to other Slots in the Graph
        //! (Property slots will never have connections)
        // bool IsConnected() const;

        //! Returns the list of connections to this Slot.
        //! (Property slots will never have connections)
        ConnectionList GetConnections() const;

        //! Returns the list of other Slots that this Slot is connected to
        //! (Property slots will never have connections)
        // AZStd::vector<SlotPtr> GetConnectedSlots();    

        //! Returns the list of all Nodes that this Slot is connected to
        //! (Property slots will never have connections)
        //AZStd::vector<NodePtr> GetConnectedNodes();

        //! Returns the list of IDs for other Slots that this Slot is connected to
        //! (Property slots will never have connections)
        // AZStd::vector<Endpoint> GetConnectedEndpoints();

        //! Returns the list of IDs for all Nodes that this Slot is connected to
        //! (Property slots will never have connections)
        // AZStd::vector<NodeId> GetConnectedNodeIds();

    protected:

#if defined(AZ_ENABLE_TRACING)
        void AssertWithTypeInfo(bool expression, DataTypePtr dataTypeUsed, const char* message) const;
        void AssertTypeMatch(DataTypePtr dataTypeUsed, const char* message) const;
#endif

    private:

        mutable AZStd::weak_ptr<Node> m_parentNode; //!< This is a mutable because it is just-in-time initialized in a const accessor function. This is okay because it's just a cache.
        SlotDefinitionPtr m_slotDefinition;         //!< Pointer to the SlotDefinition in the parent Node, that defines this slot.
        AZStd::any m_value;                         //!< This is the value that gets used for a Property slot or an Input Data slot that doesn't have any connection.
        WeakConnectionList m_connections;           //!< List of connections to this Slot. Not reflected/serialized because this is just a cache of information owned by the Graph.
        SlotSubId m_subId = 0;                      //!< SubId to uniquely identify extendable slots of the same name (regular slots will always have a SubId of 0)
    };
        

    template<typename T>
    const T& Slot::GetValue() const
    {
        const T* pValue = AZStd::any_cast<T>(&m_value);

        #if defined(AZ_ENABLE_TRACING)
            DataTypePtr dataTypeUsed = GetGraphContext()->GetDataType<T>();
            AssertWithTypeInfo(SupportsValue(), dataTypeUsed, "This slot type does not support Value");
            AssertTypeMatch(dataTypeUsed, "Slot::GetValue used with the wrong type");
            AssertWithTypeInfo(nullptr != pValue, dataTypeUsed, "m_value does not hold data of the appropriate type");
        #endif

        return *pValue;
    }


    template<typename T>
    void Slot::SetValue(const T& value)
    {
        #if defined(AZ_ENABLE_TRACING)
            DataTypePtr dataTypeUsed = GetGraphContext()->GetDataType<T>();
            AssertWithTypeInfo(SupportsValue(), dataTypeUsed, "This slot type does not support Value");
            AssertTypeMatch(dataTypeUsed, "Slot::SetValue used with the wrong type");
        #endif

        m_value = value;
    }

} // namespace GraphModel

namespace AZStd
{
    // This must be defined so that our custom data type SlotId can be used as a key
    // in AZStd::unordered_map
    template <>
    struct hash<GraphModel::SlotId>
    {
        typedef GraphModel::SlotId argument_type;
        typedef size_t result_type;
        AZ_FORCE_INLINE size_t operator()(const argument_type& id) const
        {
            return id.GetHash();
        }
    };
}
