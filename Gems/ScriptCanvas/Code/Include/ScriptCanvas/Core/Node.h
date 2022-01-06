/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Casting/lossy_cast.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/EntityBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/tuple.h>
#include <ScriptCanvas/CodeGen/NodeableCodegen.h>
#include <ScriptCanvas/Core/Contracts/TypeContract.h>
#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Core/DatumBus.h>
#include <ScriptCanvas/Core/Endpoint.h>
#include <ScriptCanvas/Core/ExecutionNotificationsBus.h>
#include <ScriptCanvas/Core/GraphBus.h>
#include <ScriptCanvas/Core/NodeBus.h>
#include <ScriptCanvas/Core/SerializationListener.h>
#include <ScriptCanvas/Core/Slot.h>
#include <ScriptCanvas/Core/SubgraphInterface.h>
#include <ScriptCanvas/Debugger/StatusBus.h>
#include <ScriptCanvas/Debugger/ValidationEvents/ValidationEvent.h>
#include <ScriptCanvas/Execution/ErrorBus.h>
#include <ScriptCanvas/Execution/ExecutionBus.h>
#include <ScriptCanvas/Grammar/Primitives.h>
#include <ScriptCanvas/Variable/GraphVariable.h>

#define SCRIPT_CANVAS_CALL_ON_INDEX_SEQUENCE(lambdaInterior)\
    int dummy[]{ 0, ( lambdaInterior , 0)... };\
    static_cast<void>(dummy); /* avoid warning for unused variable */

namespace ScriptCanvas
{
    namespace SlotExecution
    {
        class Map;
    }

    using ExecutionNameMap = AZStd::unordered_multimap<AZStd::string, AZStd::string>;

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

#if defined(OBJECT_STREAM_EDITOR_ASSET_LOADING_SUPPORT_ENABLED)////
    template<typename t_Class>
    class SerializeContextReadWriteHandler : public AZ::SerializeContext::IEventHandler
    {
    public:
        /// Called right before we start reading from the instance pointed by classPtr.
        void OnReadBegin(void* objectPtr) override
        {
            t_Class* deserializedObject = reinterpret_cast<t_Class*>(objectPtr);
            deserializedObject->OnReadBegin();
        }

        /// Called after we are done reading from the instance pointed by classPtr.
        void OnReadEnd(void* objectPtr) override
        {
            t_Class* deserializedObject = reinterpret_cast<t_Class*>(objectPtr);
            deserializedObject->OnReadEnd();
        }

        /// Called right before we start writing to the instance pointed by classPtr.
        void OnWriteBegin(void* objectPtr) override
        {
            t_Class* deserializedObject = reinterpret_cast<t_Class*>(objectPtr);
            deserializedObject->OnWriteBegin();
        }

        /// Called after we are done writing to the instance pointed by classPtr.
        void OnWriteEnd(void* objectPtr) override
        {
            t_Class* deserializedObject = reinterpret_cast<t_Class*>(objectPtr);
            deserializedObject->OnWriteEnd();
        }
    };

    template<typename t_Class>
    class SerializeContextOnWriteEndHandler : public AZ::SerializeContext::IEventHandler
    {
    public:
        /// Called after we are done writing to the instance pointed by classPtr.
        void OnWriteEnd(void* objectPtr) override
        {
            t_Class* deserializedObject = reinterpret_cast<t_Class*>(objectPtr);
            deserializedObject->OnWriteEnd();
        }
    };

    template<typename t_Class>
    class SerializeContextOnWriteHandler : public AZ::SerializeContext::IEventHandler
    {
    public:
        /// Called right before we start writing to the instance pointed by classPtr.
        void OnWriteBegin(void* objectPtr) override
        {
            t_Class* deserializedObject = reinterpret_cast<t_Class*>(objectPtr);
            deserializedObject->OnWriteBegin();
        }

        /// Called after we are done writing to the instance pointed by classPtr.
        void OnWriteEnd(void* objectPtr) override
        {
            t_Class* deserializedObject = reinterpret_cast<t_Class*>(objectPtr);
            deserializedObject->OnWriteEnd();
        }
    };
#endif//defined(OBJECT_STREAM_EDITOR_ASSET_LOADING_SUPPORT_ENABLED)

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
        virtual void OnPropertyChanged() {};
    };
    
    // Dummy Wrapper Class to streamline the interface.
    // Should always be the TypeNodePropertyInterface
    class NodePropertyInterface
    {
    protected:
        NodePropertyInterface() = default;

    public:
        AZ_RTTI(NodePropertyInterface, "{265A2163-D3AE-4C4E-BDCC-37BA0084BF88}");
        virtual ~NodePropertyInterface() = default;

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
        virtual ~TypedNodePropertyInterface() = default;

        void SetPropertyReference(DataType* dataReference)
        {
            m_dataType = dataReference;
        }

        Data::Type GetDataType() override
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

    using ConstSlotsOutcome = AZ::Outcome<AZStd::vector<const Slot*>, AZStd::string>;
    using SlotsOutcome = AZ::Outcome<AZStd::vector<Slot*>, AZStd::string>;

    using EndpointResolved = AZStd::pair<const Node*, const Slot*>;
    using EndpointsResolved = AZStd::vector<EndpointResolved>;

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
        virtual ~TypedComboBoxNodePropertyInterface() = default;

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
        int GetSelectedIndex() const override
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

        void SetSelectedIndex(int index) override
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
        virtual ~EnumComboBoxNodePropertyInterface() = default;

        // No way of identifying Enum types properly yet. Going to fake a BCO object type for now.
        static const AZ::Uuid k_EnumUUID;

        Data::Type GetDataType() override
        {
            return ScriptCanvas::Data::Type::BehaviorContextObject(k_EnumUUID);
        }
    };



    enum class UpdateResult
    {
        DirtyGraph,
        DeleteNode,
        DisableNode,

        Unknown
    };

    struct SlotVersionCache
    {
        ScriptCanvas::SlotId        m_slotId;
        ScriptCanvas::Datum         m_slotDatum;
        ScriptCanvas::VariableId    m_variableId;
        AZStd::string               m_originalName;
    };

    struct NodeConfiguration
    {
        AZ::Uuid m_type = AZ::Uuid::CreateNull();
        AZStd::string m_className;
        AZStd::string m_methodName;
        PropertyStatus m_propertyStatus = PropertyStatus::None;

        bool IsValid()
        {
            return !m_type.IsNull();
        }
    };

    class Node
        : public AZ::Component
        , public DatumNotificationBus::Handler
        , public NodeRequestBus::Handler
        , public EndpointNotificationBus::MultiHandler
        , public SerializationListener
    {
        friend class Graph;
        friend class RuntimeComponent;
        friend struct BehaviorContextMethodHelper;
        friend class NodeEventHandler;
        friend class Slot;
        
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

        AZ_COMPONENT(Node, "{52B454AE-FA7E-4FE9-87D3-A1CAB235C691}", SerializationListener);
        static void Reflect(AZ::ReflectContext* reflection);

        Node();
        ~Node() override;
        Node(const Node&); // Needed just for DLL linkage. Does not perform a copy
        Node& operator=(const Node&); // Needed just for DLL linkage. Does not perform a copy        

        virtual bool CanAcceptNullInput(const Slot& executionSlot, const Slot& inputSlot) const;

        virtual void CollectVariableReferences(AZStd::unordered_set< ScriptCanvas::VariableId >& variableIds) const;

        virtual bool ContainsReferencesToVariables(const AZStd::unordered_set< ScriptCanvas::VariableId >& variableIds) const;        

        Graph* GetGraph() const;
        AZ::EntityId GetGraphEntityId() const;
        AZ::Data::AssetId GetGraphAssetId() const;
        AZStd::string GetGraphAssetName() const;
        GraphIdentifier GetGraphIdentifier() const;

        bool IsSanityCheckRequired() const;
        void SanityCheckDynamicDisplay();
        void SanityCheckDynamicDisplay(ExploredDynamicGroupCache& exploredGroupCache);

        bool ConvertSlotToReference(const SlotId& slotId);
        bool ConvertSlotToValue(const SlotId& slotId);

        NamedEndpoint CreateNamedEndpoint(SlotId slotId) const;

        // Reconfiguration
        void SignalReconfigurationBegin();
        void SignalReconfigurationEnd();

        // CodeGen Overrides
        virtual bool IsEntryPoint() const;
        virtual bool RequiresDynamicSlotOrdering() const;


        //! Node internal initialization, for custom init, use OnInit
        void Init() final;

        //! Node internal activation and housekeeping, for custom activation configuration use OnActivate
        void Activate() final;

        //! Node internal deactivation and housekeeping, for custom deactivation configuration use OnDeactivate
        void Deactivate() final;

        void PostActivate();

        void SignalDeserialized();

        virtual AZStd::string GetNodeTypeName() const;
        virtual AZStd::string GetDebugName() const;
        virtual AZStd::string GetNodeName() const;

        AZStd::string GetSlotName(const SlotId& slotId) const;

        //! returns a list of all slots, regardless of type
        SlotList& GetSlots() { return m_slots; }
        const SlotList& GetSlots() const { return m_slots; }
        ConstSlotsOutcome GetSlots(const AZStd::vector<SlotId>& slotIds) const;
        SlotsOutcome GetSlots(const AZStd::vector<SlotId>& slotIds);

        AZStd::vector< Slot* > GetSlotsWithDisplayGroup(AZStd::string_view displayGroup) const;
        AZStd::vector< Slot* > GetSlotsWithDynamicGroup(const AZ::Crc32& dynamicGroup) const;
        AZStd::vector<const Slot*> GetAllSlotsByDescriptor(const SlotDescriptor& slotDescriptor, bool allowLatentSlots = false) const;

        //! returns a const list of nodes connected to slot(s) of the specified type
        NodePtrConstList FindConnectedNodesByDescriptor(const SlotDescriptor& slotType, bool followLatentConnections = false) const;
        AZStd::vector<AZStd::pair<const Node*, SlotId>> FindConnectedNodesAndSlotsByDescriptor(const SlotDescriptor& slotType, bool followLatentConnections = false) const;
        
        AZStd::vector<const Slot*> GetSlotsByType(CombinedSlotType slotType) const;

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
        AZStd::vector<Slot*> ModAllSlots() override;
        SlotId GetSlotId(AZStd::string_view slotName) const override;
        SlotId FindSlotIdForDescriptor(AZStd::string_view slotName, const SlotDescriptor& descriptor) const override;

        const Datum* FindDatum(const SlotId& slotId) const override;
        void FindModifiableDatumView(const SlotId& slotId, ModifiableDatumView& datumView) override;

        AZStd::vector<SlotId> GetSlotIds(AZStd::string_view slotName) const override;
        const ScriptCanvasId& GetOwningScriptCanvasId() const override { return m_scriptCanvasId; }
        AZ::Outcome<void, AZStd::string> SlotAcceptsType(const SlotId&, const Data::Type&) const override;
        Data::Type GetSlotDataType(const SlotId& slotId) const override;

        VariableId GetSlotVariableId(const SlotId& slotId) const override;
        void SetSlotVariableId(const SlotId& slotId, const VariableId& variableId) override;
        void ClearSlotVariableId(const SlotId& slotId) override;

        int FindSlotIndex(const SlotId& slotId) const override;
        bool IsOnPureDataThread(const SlotId& slotId) const override;

        AZ::Outcome<void, AZStd::string> IsValidTypeForSlot(const SlotId& slotId, const Data::Type& dataType) const override;
        AZ::Outcome<void, AZStd::string> IsValidTypeForGroup(const AZ::Crc32& dynamicGroup, const Data::Type& dataType) const override;

        void SignalBatchedConnectionManipulationBegin() override;
        void SignalBatchedConnectionManipulationEnd() override;

        void AddNodeDisabledFlag(NodeDisabledFlag disabledFlag) override;
        void RemoveNodeDisabledFlag(NodeDisabledFlag disabledFlag) override;

        bool IsNodeEnabled() const override;
        bool HasNodeDisabledFlag(NodeDisabledFlag disabledFlag) const override;
        NodeDisabledFlag GetNodeDisabledFlag() const;
        void SetNodeDisabledFlag(NodeDisabledFlag disabledFlag);

        bool RemoveVariableReferences(const AZStd::unordered_set< ScriptCanvas::VariableId >& variableIds) override;
        ////

        Slot* GetSlotByName(AZStd::string_view slotName) const;
        Slot* GetSlotByTransientId(TransientSlotIdentifier transientSlotId) const;
        const Slot* GetSlotByNameAndType(AZStd::string_view slotName, CombinedSlotType slotType) const;

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
        virtual size_t GenerateFingerprint() const { return 0; }
        virtual NodeConfiguration GetReplacementNodeConfiguration() const { return {}; };
        virtual AZStd::unordered_map<AZStd::string, AZStd::vector<AZStd::string>> GetReplacementSlotsMap() const { return {}; };

        // Use following function to customize node replacement
        virtual void CustomizeReplacementNode(Node* /*replacementNode*/, AZStd::unordered_map<SlotId, AZStd::vector<SlotId>>& /*outSlotIdMap*/) const {}

        // restored inputs to graph defaults, if necessary and possible
        void RefreshInput();

        GraphVariable* FindGraphVariable(const VariableId& variableId) const;

        EndpointsResolved GetConnectedNodes(const Slot& slot) const;

        AZStd::vector<const Slot*> GetEventSlots() const;

        bool ValidateNode(ValidationResults& validationResults);

        void OnSlotConvertedToValue(const SlotId& slotId);
        void OnSlotConvertedToReference(const SlotId& slotId);

        // Versioning Overrides
        virtual bool IsOutOfDate(const VersionData& graphVersion) const;
        UpdateResult UpdateNode();

        virtual AZStd::string GetUpdateString() const;

        //////////////////////////////////////////////////////////////////////////
        // Translation

        // \todo a hack that will disappear after conversions are properly done
        virtual bool ConvertsInputToStrings() const;

        // must be overridden
        virtual AZ::Outcome<DependencyReport, void> GetDependencies() const;

        // must be overridden
        virtual AZ::Outcome<AZStd::string, void> GetFunctionCallName(const Slot* /*slot*/) const;

        virtual EventType GetFunctionEventType(const Slot* slot) const;

        virtual AZ::Outcome<Grammar::FunctionPrototype> GetSimpleSignature() const;

        virtual AZ::Outcome<Grammar::FunctionPrototype> GetSignatureOfEexcutionIn(const Slot& executionSlot) const;

        // must be overridden
        virtual AZ::Outcome<Grammar::LexicalScope, void> GetFunctionCallLexicalScope(const Slot* /*slot*/) const;

        // override if necessary, usually only when the node's execution topology dramatically alters at edit-time in a way that is not generally parseable 
        ConstSlotsOutcome GetSlotsInExecutionThreadByType(const Slot& executionSlot, CombinedSlotType targetSlotType, const Slot* executionChildSlot = nullptr) const;

        size_t GetOutIndex(const Slot& slot) const;

        // override if necessary, only used by NodeableNodes which can hide branched outs and rename them later
        virtual AZ::Outcome<AZStd::string> GetInternalOutKey(const Slot& slot) const;

        // override if necessary, only used by NodeableNodes which can hide branched outs and rename them later
        virtual AZ::Outcome<AZStd::string> GetLatentOutKey(const Slot& slot) const;

        virtual SlotId GetLoopFinishSlotId() const { return {}; }

        virtual SlotId GetLoopSlotId() const { return {}; }

        virtual PropertyFields GetPropertyFields() const;

        virtual Grammar::MultipleFunctionCallFromSingleSlotInfo GetMultipleFunctionCallFromSingleSlotInfo(const Slot& slot) const;

        virtual VariableId GetVariableIdRead(const Slot*) const;

        virtual VariableId GetVariableIdWritten(const Slot*) const;

        virtual const Slot* GetVariableOutputSlot() const;

        virtual bool IsFormalLoop() const { return false; }

        virtual bool IsIfBranchPrefacedWithBooleanExpression() const { return false; }

        virtual bool IsIfBranch() const { return false; }

        virtual const Slot* GetIfBranchFalseOutSlot() const;

        virtual const Slot* GetIfBranchTrueOutSlot() const;

        virtual bool IsLogicalAND() const { return false; }

        virtual bool IsLogicalNOT() const { return false; }

        virtual bool IsLogicalOR() const { return false; }

        virtual bool IsNoOp() const { return false; }

        virtual bool IsNodeableNode() const { return false; }

        virtual bool IsSwitchStatement() const { return false; }

        // Handler translation
        virtual bool IsEventHandler() const { return false; }

        virtual const Slot* GetEBusConnectSlot() const;

        virtual const Slot* GetEBusConnectAddressSlot() const;

        virtual const Slot* GetEBusDisconnectSlot() const;

        virtual AZStd::string GetEBusName() const;

        virtual AZStd::optional<size_t> GetEventIndex(AZStd::string eventName) const;

        virtual AZStd::vector<SlotId> GetEventSlotIds() const;

        virtual AZStd::vector<SlotId> GetNonEventSlotIds() const;

        virtual AZStd::vector<const Slot*> GetOnVariableHandlingDataSlots() const;

        virtual AZStd::vector<const Slot*> GetOnVariableHandlingExecutionSlots() const;

        virtual bool IsAutoConnected() const;

        virtual bool IsEBusAddressed() const;

        virtual bool IsVariableWriteHandler() const { return false; }

        virtual const Datum* GetHandlerStartAddress() const;

        // Slot mapping.
        // The parser needs to know which data slots are associated with which execution slots,
        // and which execution-out slots are associated with which execution-in slots, or none
        // (in the case of latent outs).
        //
        // So far, the parser needs to map:
        //      In -> Out, Data In, Data Out
        //      Out -> Data Out (for the internal out case)
        //      Latent -> Data Out

        // if the child node returns the this map, all other topology questions are covered
        virtual const SlotExecution::Map* GetSlotExecutionMap() const { return nullptr; }

        virtual const Grammar::SubgraphInterface* GetSubgraphInterface() const { return nullptr; }

        // provides a simple map of execution in -> out by slot name, currently only used to help flow-of-control nodes identify cycle in the graph
        virtual ExecutionNameMap GetExecutionNameMap() const { return ExecutionNameMap(); }

        // override if necessary, usually only when the node's execution topology dramatically alters at edit-time in a way that is not generally parseable 
        virtual ConstSlotsOutcome GetSlotsInExecutionThreadByTypeImpl(const Slot& executionSlot, CombinedSlotType targetSlotType, const Slot* executionChildSlot) const;

        ConstSlotsOutcome GetSlotsFromMap(const SlotExecution::Map& map, const Slot& executionSlot, CombinedSlotType targetSlotType, const Slot* executionChildSlot) const;

        ConstSlotsOutcome GetDataInSlotsByExecutionIn(const SlotExecution::Map& map, const Slot& executionInSlot) const;

        ConstSlotsOutcome GetDataInSlotsByExecutionOut(const SlotExecution::Map& map, const Slot& executionOutSlot) const;

        ConstSlotsOutcome GetDataOutSlotsByExecutionIn(const SlotExecution::Map& map, const Slot& executionInSlot) const;

        ConstSlotsOutcome GetDataOutSlotsByExecutionOut(const SlotExecution::Map& map, const Slot& executionOutSlot) const;

        ConstSlotsOutcome GetExecutionOutSlotsByExecutionIn(const SlotExecution::Map& map, const Slot& executionInSlot) const;

        AZ::Outcome<AZStd::string> GetInternalOutKey(const SlotExecution::Map& map, const Slot& slot) const;

        AZ::Outcome<AZStd::string> GetLatentOutKey(const SlotExecution::Map& map, const Slot& slot) const;

        void ClearDisplayType(const SlotId& slotId);
        void SetDisplayType(const SlotId& slotId, const Data::Type& dataType);

        void SetDisplayType(const AZ::Crc32& dynamicGroup, const Data::Type& dataType, bool forceDisplaySet = false)
        {
            ExploredDynamicGroupCache cache;
            SetDisplayType(dynamicGroup, dataType, cache, forceDisplaySet);
        }

        Data::Type GetDisplayType(const AZ::Crc32& dynamicGroup) const;

        // Translation
        //////////////////////////////////////////////////////////////////////////

    protected:
        void OnDeserialize() override;

        virtual void OnReconfigurationBegin() {}
        virtual void OnReconfigurationEnd() {}

        virtual void OnSanityCheckDisplay() {}

        virtual bool OnValidateNode(ValidationResults& validationResults);

        bool IsUpdating() const { return m_isUpdatingNode; }

        // Versioning Overrides
        virtual UpdateResult OnUpdateNode() { return UpdateResult::DirtyGraph;  }
        ////

        void ConfigureSlotDisplayType(Slot* slot, const Data::Type& dataType)
        {
            ExploredDynamicGroupCache cache;
            ConfigureSlotDisplayType(slot, dataType, cache);
        }

        void ConfigureSlotDisplayType(Slot* slot, const Data::Type& dataType, ExploredDynamicGroupCache& exploredGroupCache);

        void SignalSlotsReordered();

        // Will ignore any references and return the Datum that the slot represents.
        void ModifyUnderlyingSlotDatum(const SlotId& id, ModifiableDatumView& datumView);

        bool HasSlots() const;

        //! returns a list of all slots, regardless of type
        SlotList& ModSlots() { return m_slots; }
        
        // \todo make fast query to the system debugger
        AZ_INLINE static bool IsGraphObserved(const AZ::EntityId& entityId, const GraphIdentifier& identifier);
        AZ_INLINE static bool IsVariableObserved(const VariableId& variableId);

        const Datum* FindDatumByIndex(size_t index) const;
        void FindModifiableDatumViewByIndex(size_t index, ModifiableDatumView& controller);

        const Slot* GetSlotByIndex(size_t index) const;
public:
        SlotId AddSlot(const SlotConfiguration& slotConfiguration, bool signalAddition = true);
protected:
        // Inserts a slot before the element found at @index. If the index < 0 or >= slots.size(), the slot will be will be added at the end
        SlotId InsertSlot(AZ::s64 index, const SlotConfiguration& slotConfiguration, bool isNewSlot = true);

        // Removes the slot on this node that matches the supplied slotId
        bool RemoveSlot(const SlotId& slotId, bool signalRemoval = true, const bool warnOnMissingSlots = true);
        void RemoveConnectionsForSlot(const SlotId& slotId, bool slotDeleted = false);
        void SignalSlotRemoved(const SlotId& slotId);

        void InitializeVariableReference(Slot& slot, const AZStd::unordered_set<VariableId>& excludedVariableIds);

        void RenameSlot(Slot& slot, const AZStd::string& slotName);

        virtual void OnResetDatumToDefaultValue(ModifiableDatumView& datum);

    private:
        // insert or find a slot in the slot list and returns Success if a new slot was inserted.
        // The SlotIterator& parameter is populated with an iterator to the inserted or found slot within the slot list 
        AZ::Outcome<SlotIdIteratorMap::iterator, AZStd::string> FindOrInsertSlot(AZ::s64 index, const SlotConfiguration& slotConfig, SlotIterator& iterOut);

    public:

        // This function is only called once, when the node is added to a graph, as opposed to Init(), which will be called 
        // soon after construction, or after deserialization. So the functionality in configure does not need to be idempotent.
        void Configure();

    protected:
        TransientSlotIdentifier ConstructTransientIdentifier(const Slot& slot) const;

        DatumVector GatherDatumsForDescriptor(SlotDescriptor descriptor) const;

        SlotDataMap CreateInputMap() const;
        SlotDataMap CreateOutputMap() const;

        Signal CreateNodeInputSignal(const SlotId& slotId) const;
        Signal CreateNodeOutputSignal(const SlotId& slotId) const;

        NodeStateChange CreateNodeStateUpdate() const;
        VariableChange CreateVariableChange(const GraphVariable& graphVariable) const;
        VariableChange CreateVariableChange(const Datum& variableDatum, const VariableId& variableId) const;

        void ClearDisplayType(const AZ::Crc32& dynamicGroup)
        {
            ExploredDynamicGroupCache cache;
            ClearDisplayType(dynamicGroup, cache);
        }
        void ClearDisplayType(const AZ::Crc32& dynamicGroup, ExploredDynamicGroupCache& exploredCache);

        void SetDisplayType(const AZ::Crc32& dynamicGroup, const Data::Type& dataType, ExploredDynamicGroupCache& exploredCache, bool forceDisplaySet = false);

        Data::Type FindConcreteDisplayType(const AZ::Crc32& dynamicGroup) const
        {
            ExploredDynamicGroupCache cache;
            return FindConcreteDisplayType(dynamicGroup, cache);
        }

        Data::Type FindConcreteDisplayType(const AZ::Crc32& dynamicGroup, ExploredDynamicGroupCache& exploredCache) const;

        bool HasDynamicGroup(const AZ::Crc32& dynamicGroup) const;

        void SetDynamicGroup(const SlotId& slotId, const AZ::Crc32& dynamicGroup);

        Data::Type FindConnectedConcreteDisplayType(const Slot& slot) const
        {
            AZ::Crc32 dynamicGroup = slot.GetDynamicGroup();

            if (dynamicGroup != AZ::Crc32())
            {
                return FindConcreteDisplayType(dynamicGroup);
            }
            else
            {
                ExploredDynamicGroupCache cache;
                return FindConnectedConcreteDisplayType(slot, cache);
            }
        }

        Data::Type FindConnectedConcreteDisplayType(const Slot& slot, ExploredDynamicGroupCache& exploredGroupCache) const;        

        AZ::Outcome<void, AZStd::string> IsValidTypeForSlotInternal(Slot* slot, const Data::Type& dataType, ExploredDynamicGroupCache& exploredCache) const;
        AZ::Outcome<void, AZStd::string> IsValidTypeForGroupInternal(const AZ::Crc32& dynamicGroup, const Data::Type& dataType, ExploredDynamicGroupCache& exploredCache) const;
        
        ////
        // Child Node Interfaces

        //! This is a used by CodeGen to configure slots just prior to OnInit.
        virtual void ConfigureSlots() {}

        //! Entity level initialization, perform any resource allocation here that should be available throughout the node's existence.
        virtual void OnInit() {}

        //! Hook for populating the list of visual extensions to the node.
        virtual void ConfigureVisualExtensions() {}

        //! Entity level configuration, perform any post configuration actions on slots here.
        virtual void OnConfigured() {}

        //! Signaled when this entity is deserialized(like from an undo or a redo)
        virtual void OnDeserialized() {}

        //! Entity level activation, perform entity lifetime setup here, i.e. connect to EBuses
        virtual void OnActivate() {}

        //! Entity level deactivation, perform any entity lifetime release here, i.e disconnect from EBuses
        virtual void OnDeactivate() {}
        
        virtual void OnPostActivate() {}

        //! Signal sent once the OwningScriptCanvasId is set.
        virtual void OnGraphSet() {}

        //! Signal sent when a Dynamic Group Display type is changed
        void SignalSlotDisplayTypeChanged(const SlotId& slotId, const Data::Type& dataType);
        virtual void OnSlotDisplayTypeChanged(const SlotId& /*slotId*/, const Data::Type& /*dataType*/) {}
        virtual void OnDynamicGroupTypeChangeBegin(const AZ::Crc32& /*dynamicGroup*/) {}
        virtual void OnDynamicGroupDisplayTypeChanged(const AZ::Crc32& /*dynamicGroup*/, const Data::Type& /*dataType*/) {}

        virtual Data::Type FindFixedDataTypeForSlot(const Slot& /*slot*/) const { return Data::Type::Invalid(); }
        ////

        //! Signal when 
        virtual void OnSlotRemoved(const SlotId& /*slotId*/) {}

        void ForEachConnectedNode(const Slot& slot, AZStd::function<void(Node&, const SlotId&)> callable) const;
        
        AZStd::vector<Endpoint> GetAllEndpointsByDescriptor(const SlotDescriptor& slotDescriptor, bool allowLatentEndpoints = false) const;

        AZStd::vector<AZStd::pair<Node*, const SlotId>> ModConnectedNodes(const Slot& slot) const;

        void ModConnectedNodes(const Slot& slot, AZStd::vector<AZStd::pair<Node*, const SlotId>>&) const;
        bool HasConnectedNodes(const Slot& slot) const;

        void SetOwningScriptCanvasId(ScriptCanvasId scriptCanvasId);
        void SetGraphEntityId(AZ::EntityId graphEntityId);

        bool SlotExists(AZStd::string_view name, const SlotDescriptor& slotDescriptor) const;

        bool IsTargetInDataFlowPath(const ID& targetNodeId, AZStd::unordered_set<ID>& path) const;
        bool IsInEventHandlingScope(const ID& eventHandler, const AZStd::vector<SlotId>& eventSlots, const SlotId& connectionSlot, AZStd::unordered_set<ID>& path) const;
        bool IsOnPureDataThreadHelper(AZStd::unordered_set<ID>& path) const;

        void PopulateNodeType();

        GraphScopedNodeId GetScopedNodeId() const { return GraphScopedNodeId(GetOwningScriptCanvasId(), GetEntityId()); }
        
    private:
        void SetToDefaultValueOfType(const SlotId& slotID);
        void RebuildInternalState();

        void ProcessDataSlot(Slot& slot);

        void OnNodeStateChanged();

        ScriptCanvasId m_scriptCanvasId;
        NodeTypeIdentifier m_nodeType;

        bool m_nodeReconfiguring = false;
        bool m_nodeReconfigured = false;
        bool m_isUpdatingNode = false;

        NodeDisabledFlag m_disabledFlag = NodeDisabledFlag::None;

        bool m_queueDisplayUpdates = false;
        AZStd::unordered_map<AZ::Crc32, Data::Type> m_queuedDisplayUpdates;

        SlotList m_slots;
        DatumList m_slotDatums;

        AZStd::unordered_set< SlotId > m_removingSlots;

        AZStd::unordered_map<SlotId, IteratorCache> m_slotIdIteratorCache;

        AZStd::unordered_multimap<AZStd::string, SlotIterator> m_slotNameMap;
        //////////////////////////////////////////////////////////////////////////
        AZStd::unordered_set<SlotId> m_possiblyStaleInput;

        AZStd::unordered_multimap<AZ::Crc32, SlotId> m_dynamicGroups;
        AZStd::unordered_map<AZ::Crc32, Data::Type> m_dynamicGroupDisplayTypes;

        AZStd::vector< VisualExtensionSlotConfiguration > m_visualExtensions;

        // Cache pointer to the owning Graph
        GraphRequests* m_graphRequestBus = nullptr;
        
        ScriptCanvas::SlotId m_removingSlot;

    public:
        template<typename t_Func, t_Func, typename, size_t>
        friend struct Internal::MultipleOutputHelper;

        template<typename ResultType, typename t_Traits, typename>
        friend struct Internal::OutputSlotHelper;

        template<size_t... inputDatumIndices>
        friend struct SetDefaultValuesByIndex;
    };

    bool Node::IsGraphObserved(const AZ::EntityId& entityId, const GraphIdentifier& identifier)
    {
        bool isObserved{};
        ExecutionNotificationsBus::BroadcastResult(isObserved, &ExecutionNotifications::IsGraphObserved, entityId, identifier);
        return isObserved;
    }

    bool Node::IsVariableObserved(const VariableId& variableId)
    {
        bool isObserved{};
        ExecutionNotificationsBus::BroadcastResult(isObserved, &ExecutionNotifications::IsVariableObserved, variableId);
        return isObserved;
    }

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

                slotConfiguration.m_name = t_Traits::GetResultName(0);
                slotConfiguration.SetType(Data::FromAZType<AZStd::decay_t<ResultType>>());
                slotConfiguration.SetConnectionType(ConnectionType::Output);

                node.AddSlot(slotConfiguration);
            }
        };

        template<typename t_Traits>
        struct OutputSlotHelper<void, t_Traits, void>
        {
            template<size_t... Is> static void AddOutputSlot(Node&, AZStd::index_sequence<Is...>) {}
        };

        template<typename ResultType, typename t_Traits>
        struct OutputSlotHelper<ResultType, t_Traits, AZStd::enable_if_t<IsTupleLike<ResultType>::value>>
        {
            template<size_t Index>
            static void CreateDataSlot(Node& node, ConnectionType connectionType)
            {
                DataSlotConfiguration slotConfiguration;

                slotConfiguration.m_name = t_Traits::GetResultName(Index);
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

        template<typename t_Func, t_Func function, typename t_Traits, size_t NumOutputs>
        struct MultipleOutputHelper
        {
            using ResultIndexSequence = AZStd::make_index_sequence<NumOutputs>;
            using ResultType = AZStd::function_traits_get_result_t<t_Func>;

            static void Add(Node& node)
            {
                OutputSlotHelper<ResultType, t_Traits>::AddOutputSlot(node, ResultIndexSequence());
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
