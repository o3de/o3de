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

#include <AzCore/Casting/lossy_cast.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/EntityBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/tuple.h>

#include <ScriptCanvas/Core/Contracts/TypeContract.h>
#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Core/DatumBus.h>
#include <ScriptCanvas/Core/Endpoint.h>
#include <ScriptCanvas/Core/ExecutionNotificationsBus.h>
#include <ScriptCanvas/Core/GraphBus.h>
#include <ScriptCanvas/Core/NodeBus.h>
#include <ScriptCanvas/Core/SignalBus.h>
#include <ScriptCanvas/Core/Slot.h>
#include <ScriptCanvas/Debugger/StatusBus.h>
#include <ScriptCanvas/Execution/ErrorBus.h>
#include <ScriptCanvas/Execution/ExecutionBus.h>
#include <ScriptCanvas/Execution/RuntimeBus.h>
#include <ScriptCanvas/Variable/GraphVariable.h>

#define SCRIPT_CANVAS_CALL_ON_INDEX_SEQUENCE(lambdaInterior)\
    int dummy[]{ 0, ( lambdaInterior , 0)... };\
    static_cast<void>(dummy); /* avoid warning for unused variable */

namespace ScriptCanvasTests
{
    class NodeAccessor;
}

namespace ScriptCanvas
{
    namespace Internal
    {
        template<typename t_Func, t_Func function, typename t_Traits, size_t NumOutputs>
        struct MultipleOutputHelper;

        template<typename ResultType, typename t_Traits, typename>
        struct OutputSlotHelper;

        template<typename ResultType, typename t_Traits, typename>
        struct PushOutputHelper;

        template<typename ResultType, typename ResultIndexSequence, typename t_Func, t_Func function, typename t_Traits>
        struct CallHelper;
    }
    class Graph;    

    struct BehaviorContextMethodHelper;

    template<typename t_Class>
    class SerializeContextEventHandlerDefault : public AZ::SerializeContext::IEventHandler
    {
    public:
        /// Called after we are done writing to the instance pointed by classPtr.
        void OnWriteEnd(void* objectPtr) override
        {
            t_Class* deserializedObject = reinterpret_cast<t_Class*>(objectPtr);
            deserializedObject->OnWriteEnd();
        }
    };

    // List of slots that will be create visual only slots on the nodes.
    // Useful for special configurations or editor only concepts.
    struct VisualExtensionSlotConfiguration
    {
    public:
        AZ_TYPE_INFO(VisualExtensionSlotConfiguration, "{3EA2D6DB-1B8F-451B-A6CE-D5779E56F4A8}");
        AZ_CLASS_ALLOCATOR(VisualExtensionSlotConfiguration, AZ::SystemAllocator, 0);

        enum class VisualExtensionType
        {
            ExtenderSlot,
            PropertySlot,

            Unknown
        };        

        VisualExtensionSlotConfiguration() = default;
        VisualExtensionSlotConfiguration(VisualExtensionType extensionType)
            : m_extensionType(extensionType)
        {
        }

        ~VisualExtensionSlotConfiguration() = default;

        AZStd::string m_name;
        AZStd::string m_tooltip;

        AZStd::string m_displayGroup;

        AZ::Crc32     m_identifier;
        ConnectionType m_connectionType = ConnectionType::Unknown;

        VisualExtensionType m_extensionType = VisualExtensionType::Unknown;
    };

    class NodePropertyInterfaceListener
    {
    public:
        virtual void OnPropertyChanged() = 0;
    };
    
    // Dummy Wrapper Class to streamline the interface.
    // Should always be the TypeNodePropertyInterface
    class NodePropertyInterface
    {
    protected:
        NodePropertyInterface() = default;

    public:

        AZ_RTTI(NodePropertyInterface, "{265A2163-D3AE-4C4E-BDCC-37BA0084BF88}");

        virtual Data::Type GetDataType() = 0;

        void RegisterListener(NodePropertyInterfaceListener* listener)
        {
            m_listeners.insert(listener);
        }

        void RemoveListener(NodePropertyInterfaceListener* listener)
        {
            m_listeners.erase(listener);
        }

        void SignalDataChanged()
        {
            for (auto listener : m_listeners)
            {
                listener->OnPropertyChanged();
            }
        }

        virtual void ResetToDefault() = 0;

    private:

        AZStd::unordered_set< NodePropertyInterfaceListener* > m_listeners;
    };

    template<typename DataType>
    class TypedNodePropertyInterface
        : public NodePropertyInterface
    {
    public:

        AZ_RTTI((TypedNodePropertyInterface<DataType>, "{24248937-86FB-406C-8DD5-023B10BD0B60}", DataType), NodePropertyInterface);

        TypedNodePropertyInterface() = default;
        ~TypedNodePropertyInterface() = default;

        void SetPropertyReference(DataType* dataReference)
        {
            m_dataType = dataReference;
        }

        virtual Data::Type GetDataType() override
        {
            return Data::FromAZType(azrtti_typeid<DataType>());
        }

        const DataType* GetPropertyData() const
        {
            return m_dataType;
        }

        void SetPropertyData(DataType dataType)
        {
            if ((*m_dataType != dataType))
            {
                (*m_dataType) = dataType;

                SignalDataChanged();
            }
        }

        void ResetToDefault() override
        {
            SetPropertyData(DataType());
        }

    private:

        DataType* m_dataType;
    };

    class ComboBoxPropertyInterface
    {
    public:

        AZ_RTTI(ComboBoxPropertyInterface, "{6CA5B611-59EA-4EAF-8A55-E7E74D7C1E53}");

        virtual int GetSelectedIndex() const = 0;
        virtual void SetSelectedIndex(int index) = 0;
    };

    template<typename DataType>
    class TypedComboBoxNodePropertyInterface
        : public TypedNodePropertyInterface<DataType>
        , public ComboBoxPropertyInterface
        
    {
    public:

        // The this-> method calls are here to deal with clang quirkiness with dependent template classes. Don't remove them.

        AZ_RTTI((TypedComboBoxNodePropertyInterface<DataType>, "{24248937-86FB-406C-8DD5-023B10BD0B60}", DataType), TypedNodePropertyInterface<DataType>, ComboBoxPropertyInterface);

        TypedComboBoxNodePropertyInterface() = default;
        ~TypedComboBoxNodePropertyInterface() = default;

        // TypedNodePropertyInterface
        void ResetToDefault() override
        {
            if (m_displaySet.empty())
            {
                this->SetPropertyData(DataType());
            }
            else
            {
                this->SetPropertyData(m_displaySet.front().second);
            }
        }
        ////

        void RegisterValueType(const AZStd::string& displayString, DataType value)
        {
            if (m_keySet.find(displayString) != m_keySet.end())
            {
                return;
            }

            m_displaySet.emplace_back(AZStd::make_pair(displayString, value));
        }

        // ComboBoxPropertyInterface
        int GetSelectedIndex() const
        {
            int counter = -1;

            const DataType* value = this->GetPropertyData();

            for (int i = 0; i < m_displaySet.size(); ++i)
            {
                if (m_displaySet[i].second == (*value))
                {
                    counter = i;
                    break;
                }
            }

            return counter;
        }

        void SetSelectedIndex(int index)
        {
            if (index >= 0 || index < m_displaySet.size())
            {
                this->SetPropertyData(m_displaySet[index].second);
            }
        }
        ////

        const AZStd::vector<AZStd::pair<AZStd::string, DataType>>& GetValueSet() const
        {
            return m_displaySet;
        }

    private:

        AZStd::unordered_set<AZStd::string> m_keySet;
        AZStd::vector<AZStd::pair<AZStd::string, DataType>> m_displaySet;
    };

    
    class EnumComboBoxNodePropertyInterface
        : public TypedComboBoxNodePropertyInterface<int>
    {
    public:
        AZ_RTTI(EnumComboBoxNodePropertyInterface, "{7D46B998-9E05-401A-AC92-37A90BAF8F60}", TypedComboBoxNodePropertyInterface<int32_t>);

        // No way of identifying Enum types properly yet. Going to fake a BCO object type for now.
        static const AZ::Uuid k_EnumUUID;

        Data::Type GetDataType() override
        {
            return ScriptCanvas::Data::Type::BehaviorContextObject(k_EnumUUID);
        }
    };


    // Signifies what sort of environment the Execution correlates to.
    // Mainly useful for knowing what busses to register for.
    enum class ExecutionType
    {
        Runtime,
        Editor
    };

    enum class UpdateResult
    {
        DirtyGraph,
        DeleteNode,

        Unknown
    };

    struct SlotVersionCache
    {
        ScriptCanvas::SlotId        m_slotId;
        ScriptCanvas::Datum         m_slotDatum;
        ScriptCanvas::VariableId    m_variableId;
        AZStd::string               m_originalName;
    };

    class Node
        : public AZ::Component
        , public DatumNotificationBus::Handler
        , private SignalBus::Handler
        , public NodeRequestBus::Handler
        , public EndpointNotificationBus::MultiHandler
    {
        friend class Graph;
        friend class RuntimeComponent;
        friend struct BehaviorContextMethodHelper;
        friend class NodeEventHandler;
        friend class Slot;
        friend class ExecutionContext;
        friend class ScriptCanvasTests::NodeAccessor;

    public:
        using SlotList = AZStd::list<Slot>;
        using SlotIterator = SlotList::iterator;

        using DatumList = AZStd::list<Datum>;
        using DatumIterator = DatumList::iterator;

        using DatumVector = AZStd::vector<const Datum*>;

        enum class OutputStorage { Optional, Required };

        using ExploredDynamicGroupCache = AZStd::unordered_map<AZ::EntityId, AZStd::unordered_set< AZ::Crc32 >>;

    private:

        struct IteratorCache
        {
        public:
            SlotIterator m_slotIterator;

            bool HasDatum() const
            {
                return m_hasDatum;
            }

            void SetDatumIterator(DatumIterator datumIterator)
            {
                if (!m_hasDatum)
                {
                    m_hasDatum = true;
                    m_datumIterator = datumIterator;
                }
            }

            DatumIterator GetDatumIter() const
            {
                return m_datumIterator;
            }

            void ClearIterator()
            {
                m_hasDatum = false;
                m_datumIterator = DatumIterator();
            }

            Datum* GetDatum()
            {
                return m_hasDatum ? &(*m_datumIterator) : nullptr;
            }

            const Datum* GetDatum() const
            {
                return m_hasDatum ? &(*m_datumIterator) : nullptr;
            }

        private:
            bool m_hasDatum = false;
            DatumIterator m_datumIterator;
        };

        using SlotIdIteratorMap = AZStd::unordered_map< SlotId, IteratorCache >;

    public:

        AZ_COMPONENT(Node, "{52B454AE-FA7E-4FE9-87D3-A1CAB235C691}");
        static void Reflect(AZ::ReflectContext* reflection);

        Node();
        ~Node() override;
        Node(const Node&); // Needed just for DLL linkage. Does not perform a copy
        Node& operator=(const Node&); // Needed just for DLL linkage. Does not perform a copy        

        virtual void CollectVariableReferences(AZStd::unordered_set< ScriptCanvas::VariableId >& variableIds) const;
        virtual bool ContainsReferencesToVariables(const AZStd::unordered_set< ScriptCanvas::VariableId >& variableIds) const;        

        Graph* GetGraph() const;
        AZ::EntityId GetGraphEntityId() const;
        AZ::Data::AssetId GetGraphAssetId() const;
        AZStd::string GetGraphAssetName() const;
        GraphIdentifier GetGraphIdentifier() const;

        void SanityCheckDynamicDisplay();
        void SanityCheckDynamicDisplay(ExploredDynamicGroupCache& exploredGroupCache);

        bool ConvertSlotToReference(const SlotId& slotId);
        bool ConvertSlotToValue(const SlotId& slotId);

        // CodeGen Overrides
        virtual bool IsEntryPoint() const;
        virtual bool RequiresDynamicSlotOrdering() const;
        ////

        void SetExecutionType(ExecutionType executionType);

        //! Node internal initialization, for custom init, use OnInit
        void Init() override final;

        //! Node internal activation and housekeeping, for custom activation configuration use OnActivate
        void Activate() override final;

        //! Node internal deactivation and housekeeping, for custom deactivation configuration use OnDeactivate
        void Deactivate() override final;

        void PostActivate();

        void SignalDeserialized();

        virtual AZStd::string GetDebugName() const;
        virtual AZStd::string GetNodeName() const;

        AZStd::string GetSlotName(const SlotId& slotId) const;

        //! returns a list of all slots, regardless of type
        const SlotList& GetSlots() const { return m_slots; }

        AZStd::vector< Slot* > GetSlotsWithDisplayGroup(AZStd::string_view displayGroup) const;
        AZStd::vector< Slot* > GetSlotsWithDynamicGroup(const AZ::Crc32& dynamicGroup) const;
        AZStd::vector<const Slot*> GetAllSlotsByDescriptor(const SlotDescriptor& slotDescriptor, bool allowLatentSlots = false) const;

        //! returns a const list of nodes connected to slot(s) of the specified type
        NodePtrConstList FindConnectedNodesByDescriptor(const SlotDescriptor& slotType, bool followLatentConnections = false) const;
        AZStd::vector<AZStd::pair<const Node*, SlotId>> FindConnectedNodesAndSlotsByDescriptor(const SlotDescriptor& slotType, bool followLatentConnections = false) const;

        bool IsConnected(const Slot& slot) const;
        bool IsConnected(const SlotId& slotId) const;
        bool HasConnectionForDescriptor(const SlotDescriptor& slotDescriptor) const;

        bool IsPureData() const;
        bool IsActivated() const;

        //////////////////////////////////////////////////////////////////////////
        // NodeRequestBus::Handler
        Slot* GetSlot(const SlotId& slotId) const override;
        size_t GetSlotIndex(const SlotId& slotId) const override;
        AZStd::vector<const Slot*> GetAllSlots() const override;
        SlotId GetSlotId(AZStd::string_view slotName) const override;
        SlotId FindSlotIdForDescriptor(AZStd::string_view slotName, const SlotDescriptor& descriptor) const override;

        const Datum* FindDatum(const SlotId& slotId) const override;
        void FindModifiableDatumView(const SlotId& slotId, ModifiableDatumView& datumView) override;

        AZStd::vector<SlotId> GetSlotIds(AZStd::string_view slotName) const override;
        const ScriptCanvasId& GetOwningScriptCanvasId() const override { return m_scriptCanvasId; }
        bool SlotAcceptsType(const SlotId&, const Data::Type&) const override;
        Data::Type GetSlotDataType(const SlotId& slotId) const override;

        VariableId GetSlotVariableId(const SlotId& slotId) const override;
        void SetSlotVariableId(const SlotId& slotId, const VariableId& variableId) override;
        void ClearSlotVariableId(const SlotId& slotId) override;

        int FindSlotIndex(const SlotId& slotId) const override;
        bool IsOnPureDataThread(const SlotId& slotId) const override;

        AZ::Outcome<void, AZStd::string> IsValidTypeForGroup(const AZ::Crc32& dynamicGroup, const Data::Type& dataType) const override;

        void SignalBatchedConnectionManipulationBegin() override;
        void SignalBatchedConnectionManipulationEnd() override;

        void SetNodeEnabled(bool enabled) override;
        bool IsNodeEnabled() const override;

        bool RemoveVariableReferences(const AZStd::unordered_set< ScriptCanvas::VariableId >& variableIds);
        ////

        Slot* GetSlotByName(AZStd::string_view slotName) const;
        Slot* GetSlotByTransientId(TransientSlotIdentifier transientSlotId) const;

        // DatumNotificationBus::Handler
        void OnDatumEdited(const Datum* datum) override;
        ////

        ////
        // EndpointNotificationBus
        void OnEndpointConnected(const Endpoint& endpoint) override;
        void OnEndpointDisconnected(const Endpoint& endpoint) override;
        ////
        
        bool IsTargetInDataFlowPath(const Node* targetNode) const;
        bool IsInEventHandlingScope(const ID& possibleEventHandler) const;
        
        virtual void MarkDefaultableInput();
        
        AZStd::string CreateInputMapString(const SlotDataMap& map) const;

        bool IsNodeType(const NodeTypeIdentifier& nodeIdentifier) const;
        NodeTypeIdentifier GetNodeType() const;

        void ResetSlotToDefaultValue(const SlotId& slotId);
        void ResetProperty(const AZ::Crc32& propertyId);

        void DeleteSlot(const SlotId& slotId)
        {
            if (CanDeleteSlot(slotId))
            {
                RemoveSlot(slotId);
            }
        }
        
        bool HasExtensions() const;
        void RegisterExtension(const VisualExtensionSlotConfiguration& configuration);
        const AZStd::vector< VisualExtensionSlotConfiguration >& GetVisualExtensions() const;
        
        virtual bool CanDeleteSlot(const SlotId& slotId) const;

        virtual SlotId HandleExtension(AZ::Crc32 extensionId);        
        virtual void ExtensionCancelled(AZ::Crc32 extensionId);
        virtual void FinalizeExtension(AZ::Crc32 extensionId);
        virtual NodePropertyInterface* GetPropertyInterface(AZ::Crc32 propertyInterface);

        // Mainly here for EBus handlers which contain multiple 'events' which are differentiated by
        // Endpoint.
        virtual NodeTypeIdentifier GetOutputNodeType(const SlotId& slotId) const;
        virtual NodeTypeIdentifier GetInputNodeType(const SlotId& slotId) const;

        // Hook here to allow CodeGen to override this
        virtual bool IsDeprecated() const { return false; };

        // restored inputs to graph defaults, if necessary and possible
        void RefreshInput();

        GraphVariable* FindGraphVariable(const VariableId& variableId) const;        

        void OnSlotConvertedToValue(const SlotId& slotId);
        void OnSlotConvertedToReference(const SlotId& slotId);

        bool ValidateNode(ValidationResults& validationResults);

        // Versioning Overrides
        virtual bool IsOutOfDate() const;
        UpdateResult UpdateNode();

        virtual AZStd::string GetUpdateString() const;
        ////

    protected:

        virtual bool OnValidateNode(ValidationResults& validationResults);

        bool IsUpdating() const { return m_isUpdatingNode; }        

        // Versioning Overrides
        virtual UpdateResult OnUpdateNode();
        ////

        void SignalSlotsReordered();

        static void OnInputChanged(Node& node, const Datum& input, const SlotId& slotID);
        static void SetInput(Node& node, const SlotId& id, const Datum& input);
        static void SetInput(Node& node, const SlotId& id, Datum&& input);

        // Will ignore any references and return the Datum that the slot represents.
        void ModifyUnderlyingSlotDatum(const SlotId& id, ModifiableDatumView& datumView);

        bool HasSlots() const;

        //! returns a list of all slots, regardless of type
        SlotList& ModSlots() { return m_slots; }
        
        const Datum* FindDatumByIndex(size_t index) const;
        void FindModifiableDatumViewByIndex(size_t index, ModifiableDatumView& controller);

        const Slot* GetSlotByIndex(size_t index) const;
        
        SlotId AddSlot(const SlotConfiguration& slotConfiguration, bool isNewSlot = true);

        // Inserts a slot before the element found at @index. If the index < 0 or >= slots.size(), the slot will be will be added at the end
        SlotId InsertSlot(AZ::s64 index, const SlotConfiguration& slotConfiguration, bool isNewSlot = true);

        // Removes the slot on this node that matches the supplied slotId
        bool RemoveSlot(const SlotId& slotId, bool signalRemoval = true);
        void RemoveConnectionsForSlot(const SlotId& slotId, bool slotDeleted = false);
        void SignalSlotRemoved(const SlotId& slotId);

        void InitializeVariableReference(Slot& slot, const AZStd::unordered_set<VariableId>& blacklistIds);

        virtual void OnResetDatumToDefaultValue(ModifiableDatumView& datum);

    private:
        // insert or find a slot in the slot list and returns Success if a new slot was inserted.
        // The SlotIterator& parameter is populated with an iterator to the inserted or found slot within the slot list 
        AZ::Outcome<SlotIdIteratorMap::iterator, AZStd::string> FindOrInsertSlot(AZ::s64 index, const SlotConfiguration& slotConfig, SlotIterator& iterOut);

        // This function is only called once, when the node is added to a graph, as opposed to Init(), which will be called 
        // soon after construction, or after deserialization. So the functionality in configure does not need to be idempotent.
        void Configure();

    protected:

        TransientSlotIdentifier ConstructTransientIdentifier(const Slot& slot) const;

        DatumVector GatherDatumsForDescriptor(SlotDescriptor descriptor) const;

        SlotDataMap CreateInputMap() const;
        SlotDataMap CreateOutputMap() const;

        NamedEndpoint CreateNamedEndpoint(AZ::EntityId editorNodeId, SlotId slotId) const;

        Signal CreateNodeInputSignal(const SlotId& slotId) const;
        Signal CreateNodeOutputSignal(const SlotId& slotId) const;
        OutputDataSignal CreateNodeOutputDataSignal(const SlotId& slotId, const Datum& datum) const;
        
        NodeStateChange CreateNodeStateUpdate() const;
        VariableChange CreateVariableChange(const GraphVariable& graphVariable) const;
        VariableChange CreateVariableChange(const Datum& variableDatum, const VariableId& variableId) const;

        void ClearDisplayType(const AZ::Crc32& dynamicGroup)
        {
            ExploredDynamicGroupCache cache;
            ClearDisplayType(dynamicGroup, cache);
        }
        void ClearDisplayType(const AZ::Crc32& dynamicGroup, ExploredDynamicGroupCache& exploredCache);

        void SetDisplayType(const AZ::Crc32& dynamicGroup, const Data::Type& dataType)
        {
            ExploredDynamicGroupCache cache;
            SetDisplayType(dynamicGroup, dataType, cache);
        }
        void SetDisplayType(const AZ::Crc32& dynamicGroup, const Data::Type& dataType, ExploredDynamicGroupCache& exploredCache);

        Data::Type GetDisplayType(const AZ::Crc32& dynamicGroup) const;

        bool HasConcreteDisplayType(const AZ::Crc32& dynamicGroup) const
        {
            ExploredDynamicGroupCache cache;
            return HasConcreteDisplayType(dynamicGroup, cache);
        }
        bool HasConcreteDisplayType(const AZ::Crc32& dynamicGroup, ExploredDynamicGroupCache& exploredCache) const;

        bool HasDynamicGroup(const AZ::Crc32& dynamicGroup) const;

        void SetDynamicGroup(const SlotId& slotId, const AZ::Crc32& dynamicGroup);

        bool IsSlotConnectedToConcreteDisplayType(const Slot& slot)
        {
            ExploredDynamicGroupCache cache;
            return IsSlotConnectedToConcreteDisplayType(slot, cache);
        }

        bool IsSlotConnectedToConcreteDisplayType(const Slot& slot, ExploredDynamicGroupCache& exploredGroupCache) const;

        AZ::Outcome<void, AZStd::string> IsValidTypeForGroupInternal(const AZ::Crc32& dynamicGroup, const Data::Type& dataType, ExploredDynamicGroupCache& exploredCache) const;
        
        ////
        // Child Node Interfaces
        virtual bool IsEventHandler() const { return false; }

        //! This is a used by CodeGen to configure slots just prior to OnInit.
        virtual void ConfigureSlots() {}
        //! Entity level initialization, perform any resource allocation here that should be available throughout the node's existence.
        virtual void OnInit() {}

        //! Hook for populating the list of visual extensions to the node.
        virtual void ConfigureVisualExtensions() {}

        //! Entity level configuration, perform any post configuration actions on slots here.
        virtual void OnConfigured() {}

        //! Signalled when this entity is deserialized(like from an undo or a redo)
        virtual void OnDeserialized() {}

        //! Entity level activation, perform entity lifetime setup here, i.e. connect to EBuses
        virtual void OnActivate() {}
        //! Entity level deactivation, perform any entity lifetime release here, i.e disconnect from EBuses
        virtual void OnDeactivate() {}
        
        virtual void OnPostActivate() {}
        //! Any node that is not pure data will perform its operation assuming that all input has been pushed and is reasonably valid
        // perform your derived nodes work in OnInputSignal(const SlotId&)
        virtual void OnInputSignal([[maybe_unused]] const SlotId& slot) {}

        //! Signal sent once the OwningScriptCanvasId is set.
        virtual void OnGraphSet() {};

        //! Signal sent when a Dynamic Group Display type is changed
        void SignalSlotDisplayTypeChanged(const SlotId& slotId, const Data::Type& dataType);
        virtual void OnSlotDisplayTypeChanged([[maybe_unused]] const SlotId& slotId, [[maybe_unused]] const Data::Type& dataType) {};
        virtual void OnDynamicGroupDisplayTypeChanged([[maybe_unused]] const AZ::Crc32& dynamicGroup, [[maybe_unused]] const Data::Type& dataType) {};
        ////

        //! Signal when 
        virtual void OnSlotRemoved([[maybe_unused]] const SlotId& slotId) {};

        void ForEachConnectedNode(const Slot& slot, AZStd::function<void(Node&, const SlotId&)> callable) const;
        
        AZStd::vector<Endpoint> GetAllEndpointsByDescriptor(const SlotDescriptor& slotDescriptor, bool allowLatentEndpoints = false) const;

        AZStd::vector<AZStd::pair<const Node*, const SlotId>> GetConnectedNodes(const Slot& slot) const;
        AZStd::vector<AZStd::pair<Node*, const SlotId>> ModConnectedNodes(const Slot& slot) const;

        void ModConnectedNodes(const Slot& slot, AZStd::vector<AZStd::pair<Node*, const SlotId>>&) const;
        bool HasConnectedNodes(const Slot& slot) const;

        virtual void OnInputChanged([[maybe_unused]] const Datum& input, [[maybe_unused]] const SlotId& slotID) {}

        //////////////////////////////////////////////////////////////////////////
        //! The body of the node's execution, implement the node's work here.
        void PushOutput(const Datum& output, const Slot& slot) const;
        void SetOwningScriptCanvasId(ScriptCanvasId scriptCanvasId);
        void SetGraphEntityId(AZ::EntityId graphEntityId);

        // on activate, simple expressions need to push this data
        virtual void SetInput(const Datum& input, const SlotId& id);
        virtual void SetInput(Datum&& input, const SlotId& id);

        //! Any node that is not pure data will perform its operation assuming that all input has been pushed and is reasonably valid
        // perform your derived nodes work in OnInputSignal(const SlotId&)
        void SignalInput(const SlotId& slot) override final;

        //! This must be called manually to send execution out of a specific slot
        void SignalOutput(const SlotId& slot, ExecuteMode mode = ExecuteMode::Normal) override final;

        bool SlotExists(AZStd::string_view name, const SlotDescriptor& slotDescriptor) const;

        bool IsTargetInDataFlowPath(const ID& targetNodeId, AZStd::unordered_set<ID>& path) const;
        bool IsInEventHandlingScope(const ID& eventHandler, const AZStd::vector<SlotId>& eventSlots, const SlotId& connectionSlot, AZStd::unordered_set<ID>& path) const;
        bool IsOnPureDataThreadHelper(AZStd::unordered_set<ID>& path) const;

        void PopulateNodeType();

        ExecutionType GetExecutionType() const;

        RuntimeRequests* GetRuntimeBus() const { return m_runtimeBus; }
        ExecutionRequests* GetExecutionBus() const { return m_executionBus; }

        GraphScopedNodeId GetScopedNodeId() const { return GraphScopedNodeId(GetOwningScriptCanvasId(), GetEntityId()); }
        
    private:
        void SetToDefaultValueOfType(const SlotId& slotID);
        void RebuildInternalState();

        void ProcessDataSlot(Slot& slot);

        void OnNodeStateChanged();

        ExecutionType m_executionType = ExecutionType::Editor;

        ScriptCanvasId m_scriptCanvasId;
        NodeTypeIdentifier m_nodeType;

        bool m_isUpdatingNode = false;

        bool m_enabled = true;

        bool m_queueDisplayUpdates = false;
        AZStd::unordered_map<AZ::Crc32, Data::Type> m_queuedDisplayUpdates;

        SlotList m_slots;
        DatumList m_slotDatums;

        AZStd::unordered_set< SlotId > m_removingSlots;

        AZStd::unordered_map<SlotId, IteratorCache> m_slotIdIteratorCache;

        AZStd::unordered_multimap<AZStd::string, SlotIterator> m_slotNameMap;

        AZStd::unordered_set<SlotId> m_possiblyStaleInput;

        AZStd::unordered_multimap<AZ::Crc32, SlotId> m_dynamicGroups;
        AZStd::unordered_map<AZ::Crc32, Data::Type> m_dynamicGroupDisplayTypes;

        AZStd::vector< VisualExtensionSlotConfiguration > m_visualExtensions;

        // Cache pointer to the owning Graph
        RuntimeRequests* m_runtimeBus = nullptr;
        ExecutionRequests* m_executionBus = nullptr;

        ScriptCanvas::SlotId m_removingSlot;

    private:
        template<typename t_Func, t_Func, typename, size_t>
        friend struct Internal::MultipleOutputHelper;

        template<typename ResultType, typename t_Traits, typename>
        friend struct Internal::OutputSlotHelper;

        template<typename ResultType, typename t_Traits, typename>
        friend struct Internal::PushOutputHelper;

        template<typename ResultType, typename ResultIndexSequence, typename t_Func, t_Func function, typename t_Traits>
        friend struct Internal::CallHelper;

        template<size_t... inputDatumIndices>
        friend struct SetDefaultValuesByIndex;
    };

    namespace Internal
    {
        template<typename, typename = AZStd::void_t<>>
        struct IsTupleLike : AZStd::false_type {};

        template<typename ResultType>
        struct IsTupleLike<ResultType, AZStd::void_t<decltype(AZStd::get<0>(AZStd::declval<ResultType>()))>> : AZStd::true_type {};

        template<typename ResultType, typename t_Traits, typename = AZStd::void_t<>>
        struct OutputSlotHelper
        {
            template<AZStd::size_t... Is>
            static void AddOutputSlot(Node& node, AZStd::index_sequence<Is...>)
            {
                DataSlotConfiguration slotConfiguration;

                slotConfiguration.m_name = AZStd::string::format("%s: %s", t_Traits::GetResultName(0), Data::GetName(Data::FromAZType<AZStd::decay_t<ResultType>>()).data());
                slotConfiguration.SetType(Data::FromAZType<AZStd::decay_t<ResultType>>());
                slotConfiguration.SetConnectionType(ConnectionType::Output);

                node.AddSlot(slotConfiguration);
            }
        };

        template<typename t_Traits>
        struct OutputSlotHelper<void, t_Traits, void>
        {
            template<size_t... Is> static void AddOutputSlot([[maybe_unused]] Node& node, AZStd::index_sequence<Is...>) {}
        };

        template<typename ResultType, typename t_Traits>
        struct OutputSlotHelper<ResultType, t_Traits, AZStd::enable_if_t<IsTupleLike<ResultType>::value>>
        {
            template<size_t Index>
            static void CreateDataSlot(Node& node, ConnectionType connectionType)
            {
                DataSlotConfiguration slotConfiguration;

                slotConfiguration.m_name = AZStd::string::format("%s: %s", t_Traits::GetResultName(Index), Data::GetName(Data::FromAZType<AZStd::decay_t<AZStd::tuple_element_t<Index, ResultType>>>()).data());
                slotConfiguration.SetType(Data::FromAZType<AZStd::decay_t<AZStd::tuple_element_t<Index, ResultType>>>());

                slotConfiguration.SetConnectionType(connectionType);                
                node.AddSlot(slotConfiguration);
            }

            template<AZStd::size_t... Is>
            static void AddOutputSlot(Node& node, AZStd::index_sequence<Is...>)                
            {
                SCRIPT_CANVAS_CALL_ON_INDEX_SEQUENCE(
                    (CreateDataSlot<Is>(node, ConnectionType::Output))
                );
            }
        };

        template<typename ResultType, typename t_Traits, typename = AZStd::void_t<>>
        struct PushOutputHelper
        {
            template<size_t... Is>
            static void PushOutput(Node& node, ResultType&& result, AZStd::index_sequence<Is...>)
            {
                // protect against changes to the function that generated the node
                if (auto slot = node.GetSlotByIndex(t_Traits::s_resultsSlotIndicesStart))
                {
                    node.PushOutput(Datum(AZStd::forward<ResultType>(result)), *slot);
                }
            }
        };

        template<typename t_Traits>
        struct PushOutputHelper<void, t_Traits, void>
        {
            template<size_t... Is> static void PushOutput(Node& node, AZStd::ignore_t result, AZStd::index_sequence<Is...>) {}
        };

        template<typename ResultType, typename t_Traits>
        struct PushOutputHelper<ResultType, t_Traits, AZStd::enable_if_t<IsTupleLike<ResultType>::value>>
        {
            template<size_t t_Index>
            static void PushOutputFold(Node& node, ResultType&& tupleResult)
            {
                // protect against changes to the function that generated the node
                if (auto slot = node.GetSlotByIndex(t_Traits::s_resultsSlotIndicesStart + t_Index))
                {
                    node.PushOutput(Datum(AZStd::get<t_Index>(AZStd::forward<ResultType>(tupleResult))), *slot);
                }
            }

            template<size_t... Is>
            static void PushOutput(Node& node, ResultType&& tupleResult, AZStd::index_sequence<Is...>)
            {
                (PushOutputFold<Is>(node, AZStd::forward<ResultType>(tupleResult)), ...);
            }
        };

        template<typename ResultType, typename ResultIndexSequence, typename t_Func, t_Func function, typename t_Traits>
        struct CallHelper
        {
            // protect against changes to the function that generated the node
            template<typename... t_Args, size_t... Indices>
            static bool IsIndexSafe(Node& node, const AZStd::index_sequence<Indices...>&)
            {
                return (true && ... && (node.FindDatumByIndex(Indices) != nullptr && node.FindDatumByIndex(Indices)->GetAs<AZStd::decay_t<t_Args>>() != nullptr));
            }

            template<typename... t_Args, size_t... ArgIndices>
            static void Call(Node& node, AZStd::Internal::pack_traits_arg_sequence<t_Args...>, [[maybe_unused]] AZStd::index_sequence<ArgIndices...> sequence)
            {
#if !defined(_RELEASE)
                if (IsIndexSafe<t_Args...>(node, sequence))
                {
                    PushOutputHelper<ResultType, t_Traits>::PushOutput(node, AZStd::invoke(function, *node.FindDatumByIndex(ArgIndices)->GetAs<AZStd::decay_t<t_Args>>()...), ResultIndexSequence());
                }
                else
                {
                    SCRIPTCANVAS_REPORT_ERROR((node), "Generic function Node (%s) could not be executed, due to missing slot for source definition."
                        "this is likely due to changing the function signature. See serialization log for potential errors & warnings.", node.GetNodeName().c_str());    
                }
#else
                PushOutputHelper<ResultType, t_Traits>::PushOutput(node, AZStd::invoke(function, *node.FindDatumByIndex(ArgIndices)->GetAs<AZStd::decay_t<t_Args>>()...), ResultIndexSequence());
#endif//!defined(_RELEASE)
            }
        };

        template<typename ResultIndexSequence, typename t_Func, t_Func function, typename t_Traits>
        struct CallHelper<void, ResultIndexSequence, t_Func, function, t_Traits>
        {
            // protect against changes to the function that generated the node
            template<typename... t_Args, size_t... Indices>
            static bool IsIndexSafe(Node& node, const AZStd::index_sequence<Indices...>&)
            {
                return true && (... && (node.FindDatumByIndex(Indices) != nullptr && node.FindDatumByIndex(Indices)->GetAs<AZStd::decay_t<t_Args>>() != nullptr));
            }

            template<typename... t_Args, size_t... ArgIndices>
            static void Call(Node& node, AZStd::Internal::pack_traits_arg_sequence<t_Args...>, [[maybe_unused]] AZStd::index_sequence<ArgIndices...> sequence)
            {
#if !defined(_RELEASE)
                if (IsIndexSafe<t_Args...>(node, sequence))
                {
                    AZStd::invoke(function, *node.FindDatumByIndex(ArgIndices)->GetAs<AZStd::decay_t<t_Args>>()...);
                }
                else
                {
                    SCRIPTCANVAS_REPORT_ERROR((node), "Generic function Node (%s) could not be executed, due to missing slot for source definition."
                        "this is likely due to changing the function signature. See serialization log for potential errors & warnings.", node.GetNodeName().c_str());
                }
#else
                AZStd::invoke(function, *node.FindDatumByIndex(ArgIndices)->GetAs<AZStd::decay_t<t_Args>>()...);
#endif//!defined(_RELEASE)
            }
        };

        template<typename t_Func, t_Func function, typename t_Traits, size_t NumOutputs>
        struct MultipleOutputHelper
        {
            using ResultIndexSequence = AZStd::make_index_sequence<NumOutputs>;
            using ResultType = AZStd::function_traits_get_result_t<t_Func>;

            static void Add(Node& node)
            {
                OutputSlotHelper<ResultType, t_Traits>::AddOutputSlot(node, ResultIndexSequence());
            }

            static void Call(Node& node)
            {
                CallHelper<ResultType, ResultIndexSequence, t_Func, function, t_Traits>::Call(node, typename AZStd::function_traits<t_Func>::arg_sequence{}, AZStd::make_index_sequence<AZStd::function_traits<t_Func>::arity>{});
            }
        };

        template<typename t_Func, t_Func function, typename t_Traits, typename t_Result, typename = AZStd::void_t<>>
        struct MultipleOutputInvokeHelper
            : MultipleOutputHelper<t_Func, function, t_Traits, 1> {};

        template<typename t_Func, t_Func function, typename t_Traits>
        struct MultipleOutputInvokeHelper<t_Func, function, t_Traits, void, void>
            : MultipleOutputHelper<t_Func, function, t_Traits, 0> {};

        template<typename t_Func, t_Func function, typename t_Traits, typename t_Result>
        struct MultipleOutputInvokeHelper< t_Func, function, t_Traits, t_Result, AZStd::enable_if_t<IsTupleLike<t_Result>::value>>
            : MultipleOutputHelper<t_Func, function, t_Traits, AZStd::tuple_size<t_Result>::value> {};
    }

    template<typename t_Func, t_Func function, typename t_Traits>
    struct MultipleOutputInvoker : Internal::MultipleOutputInvokeHelper<t_Func, function, t_Traits, AZStd::decay_t<AZStd::function_traits_get_result_t<t_Func>>> {};
}
