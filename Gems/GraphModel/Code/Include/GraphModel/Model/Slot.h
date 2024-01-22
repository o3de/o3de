/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

// AZ
#include <AzCore/Serialization/Json/BaseJsonSerializer.h>
#include <AzCore/std/any.h>
#include <AzCore/std/hash.h>
#include <AzCore/std/containers/list.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/smart_ptr/enable_shared_from_this.h>

// Graph Model
#include <GraphModel/Model/Common.h>
#include <GraphModel/Model/GraphContext.h>
#include <GraphModel/Model/GraphElement.h>

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
    struct SlotId
    {
    public:
        AZ_TYPE_INFO(SlotId, "{D24130B9-89C4-4EAA-9A5D-3469B05C5065}");

        static void Reflect(AZ::ReflectContext* context);

        SlotId() = default;
        SlotId(const SlotName& name);
        SlotId(const SlotName& name, SlotSubId subId);
        ~SlotId() = default;

        bool IsValid() const;

        bool operator==(const SlotId& rhs) const;
        bool operator!=(const SlotId& rhs) const;
        bool operator<(const SlotId& rhs) const;
        bool operator>(const SlotId& rhs) const;

        AZStd::size_t GetHash() const;
        AZStd::string ToString() const;

        SlotName m_name;
        SlotSubId m_subId = 0;
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
        AZ_CLASS_ALLOCATOR(SlotDefinition, AZ::SystemAllocator);
        AZ_RTTI(SlotDefinition, "{917F9C1A-1513-4694-B25A-D6404A4991ED}");

        SlotDefinition() = default;
        virtual ~SlotDefinition() = default;

        SlotDefinition(
            SlotDirection slotDirection,
            SlotType slotType,
            const AZStd::string& name,
            const AZStd::string& displayName,
            const AZStd::string& description = {},
            const DataTypeList& supportedDataTypes = {},
            const AZStd::any& defaultValue = {},
            int minimumSlots = {},
            int maximumSlots = {},
            const AZStd::string& addButtonLabel = {},
            const AZStd::string& addButtonTooltip = {},
            const AZStd::vector<AZStd::string>& enumValues = {},
            bool visibleOnNode = true,
            bool editableOnNode = true);

        SlotDirection GetSlotDirection() const;
        SlotType GetSlotType() const;

        //! Returns true if this slot supports assigning values
        bool SupportsValues() const;

        //! Returns true if this slot supports data types
        bool SupportsDataTypes() const;

        //! Returns whether this slot's configuration allows connections to other slots
        bool SupportsConnections() const;

        //! Returns whether or not this slot is configured to be extendable
        bool SupportsExtendability() const;

        //! Return true if this slot is configured to appear on node UI, otherwise false
        bool IsVisibleOnNode() const;

        //! Returns true if the value of this slot should be editable on the node UI, otherwise false
        bool IsEditableOnNode() const;

        //! Returns whether this slot matches the given configuration
        bool Is(SlotDirection slotDirection, SlotType slotType) const;

        const SlotName& GetName() const;                    //!< Valid for all slot configurations
        const AZStd::string& GetDisplayName() const;        //!< Valid for all slot configurations
        const AZStd::string& GetDescription() const;        //!< Valid for all slot configurations
        const DataTypeList& GetSupportedDataTypes() const;  //!< Valid for Data and Property slots. Otherwise returns an empty DataTypeList.
        AZStd::any GetDefaultValue() const;                 //!< Valid for Input Data and Property slots. Otherwise returns an empty AZStd::any.

        const AZStd::vector<AZStd::string>& GetEnumValues() const; //!< Options exposed if this slot type is an enumeration with multiple values

        //! These methods are only pertinent for extendable slots
        const int GetMinimumSlots() const;                  //!< Retrieve the minimum configured number of extendable slots (returns a default value if not configured)
        const int GetMaximumSlots() const;                  //!< Retrieve the maximum configured number of extendable slots (returns a default value if not configured)
        const AZStd::string& GetExtensionLabel() const;     //!< Retrieve the text for the label with the '+' sign for adding extendable slots
        const AZStd::string& GetExtensionTooltip() const;   //!< Retrieve the hover tooltip for the label with the '+' sign for adding extendable slots

    private:
        SlotDirection m_slotDirection = SlotDirection::Invalid;
        SlotType m_slotType = SlotType::Invalid;
        SlotName m_name;
        AZStd::string m_displayName;
        AZStd::string m_description;
        AZStd::vector<AZStd::string> m_enumValues;
        DataTypeList m_supportedDataTypes;
        AZStd::any m_defaultValue;
        bool m_visibleOnNode = true;
        bool m_editableOnNode = true;
        AZStd::string m_addButtonLabel; //!< Label for the button for adding new extendable slots
        AZStd::string m_addButtonTooltip; //!< Tooltip for the button for adding new extendable slots
        int m_minimumSlots = 0;
        int m_maximumSlots = 0;
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
    public:
        AZ_CLASS_ALLOCATOR(Slot, AZ::SystemAllocator);
        AZ_RTTI(Slot, "{50494867-04F1-4785-BB9C-9D6C96DCBFC9}", GraphElement);
        static void Reflect(AZ::ReflectContext* context);

        using ConnectionList = AZStd::vector<AZStd::shared_ptr<Connection>>;

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
        bool SupportsValues() const;
        bool SupportsDataTypes() const;
        bool SupportsConnections() const;
        bool SupportsExtendability() const;
        bool IsVisibleOnNode() const;
        bool IsEditableOnNode() const;
        const SlotName& GetName() const;
        const AZStd::string& GetDisplayName() const;
        const AZStd::string& GetDescription() const;
        const AZStd::vector<AZStd::string>& GetEnumValues() const;
        DataTypePtr GetDataType() const; //!< Valid for Data and Property slots. Otherwise returns null.
        DataTypePtr GetDefaultDataType() const; //!< Valid for Data and Property slots. Otherwise returns null.
        AZStd::any GetDefaultValue() const; //!< Valid for Data and Property slots. Otherwise returns an empty AZStd::any.

        //! Valid for Data and Property slots. Otherwise returns an empty DataTypeList.
        //! If valid, this will return the full list of all data types this slot could support.
        const DataTypeList& GetSupportedDataTypes() const;

        //! Return true if the input data type is supported by this slot.
        bool IsSupportedDataType(DataTypePtr dataType) const;

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
        T GetValue() const;

        //! Sets the slot's value, which will be used if there are no input connections.
        //! Type template type T must match the slot's data type.
        //! Valid for Input Data and Property slots.
        template<typename T>
        void SetValue(const T& value);

        //! Sets the slot's value, which will be used if there are no input connections.
        //! AZStd::any type must match the slot's data type.
        //! Valid for Input Data and Property slots.
        void SetValue(const AZStd::any& value);

        //! Returns the list of connections to this Slot.
        //! (Property slots will never have connections)
        const ConnectionList& GetConnections() const;

        //! Reset any data that was cached for this slot
        void ClearCachedData();

    private:
#if defined(AZ_ENABLE_TRACING)
        void AssertWithTypeInfo(bool expression, DataTypePtr dataTypeRequested, const char* message) const;
#endif
        DataTypePtr GetDataTypeForTypeId(const AZ::Uuid& typeId) const;
        DataTypePtr GetDataTypeForValue(const AZStd::any& value) const;

        // Pointer to the SlotDefinition in the parent Node, that defines this slot.
        SlotDefinitionPtr m_slotDefinition;

        // This is the value that gets used for a Property slot or an Input Data slot that doesn't have any connection.
        AZStd::any m_value;

        // SubId to uniquely identify extendable slots of the same name (regular slots will always have a SubId of 0)
        SlotSubId m_subId = 0;

        // Storing the parent node so that it only needs to be looked up once unless the graph state and cached data is clear
        mutable AZStd::mutex m_parentNodeMutex;
        mutable bool m_parentNodeDirty = true;
        mutable NodePtr m_parentNode;

        // Storing a list of connections for this slot, copied From the owner graph object.
        mutable AZStd::mutex m_connectionsMutex;
        mutable bool m_connectionsDirty = true;
        mutable ConnectionList m_connections;
    };

    template<typename T>
    T Slot::GetValue() const
    {
        const AZStd::any& value = GetValue();
#if defined(AZ_ENABLE_TRACING)
        DataTypePtr dataTypeRequested = GetDataTypeForTypeId(azrtti_typeid<T>());
        AssertWithTypeInfo(SupportsValues(), dataTypeRequested, "This slot type does not support values");
        AssertWithTypeInfo(IsSupportedDataType(dataTypeRequested), dataTypeRequested, "Slot::GetValue used with the wrong type");
        AssertWithTypeInfo(value.is<T>(), dataTypeRequested, "value does not hold data of the appropriate type");
#endif
        return value.is<T>() ? AZStd::any_cast<T>(value) : T{};
    }

    template<typename T>
    void Slot::SetValue(const T& value)
    {
        SetValue(AZStd::any(value));
    }
} // namespace GraphModel

namespace AZStd
{
    // This must be defined so that our custom data type SlotId can be used as a key in AZStd::unordered_map
    template<>
    struct hash<GraphModel::SlotId>
    {
        typedef GraphModel::SlotId argument_type;
        typedef size_t result_type;
        AZ_FORCE_INLINE size_t operator()(const argument_type& id) const
        {
            return id.GetHash();
        }
    };
} // namespace AZStd
