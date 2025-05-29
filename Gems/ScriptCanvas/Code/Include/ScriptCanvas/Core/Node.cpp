/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Node.h"

#include "Graph.h"

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/EntityUtils.h>
#include <AzCore/RTTI/AttributeReader.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Serialization/IdUtils.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <ScriptCanvas/Core/Attributes.h>
#include <ScriptCanvas/Core/Contract.h>
#include <ScriptCanvas/Core/Contracts/ConnectionLimitContract.h>
#include <ScriptCanvas/Core/Contracts/DynamicTypeContract.h>
#include <ScriptCanvas/Core/GraphBus.h>
#include <ScriptCanvas/Core/NodeableNode.h>
#include <ScriptCanvas/Data/DataRegistry.h>
#include <ScriptCanvas/Libraries/Core/EBusEventHandler.h>
#include <ScriptCanvas/Variable/VariableBus.h>
#include <ScriptCanvas/Debugger/API.h>
#include <ScriptCanvas/Utils/NodeUtils.h>
#include <ScriptCanvas/Grammar/Primitives.h>
#include <ScriptCanvas/Debugger/ValidationEvents/DataValidation/DataValidationEvents.h>

// Version Conversion includes
#include <ScriptCanvas/Deprecated/VariableDatum.h>
#include <ScriptCanvas/Deprecated/VariableHelpers.h>
////

AZ_DECLARE_BUDGET(ScriptCanvas);

namespace NodeCpp
{
    enum Version
    {
        MergeFromBackend2dotZero = 12,
        AddDisabledFlag = 13,
        AddName = 1,
        // AddName was changed to a lower value than the previous version instead of a higher value,
        // causing errors when processing assets generated with versions between 2 and 14.
        // This both causes the serialization system to emit an error message, because this is usually not intentional
        // and a symptom of other problems, and it causes the version converter used by this class to not work as expected.
        // This change resolves this error by setting the version higher than any previous version.
        FixedVersioningIssue = 14,

        // add your named version above
        Current,
    }; 
}

namespace ScriptCanvas
{
    //////////////////////////////////////
    // EnumComboBoxNodePropertyInterface
    //////////////////////////////////////

    const AZ::Uuid EnumComboBoxNodePropertyInterface::k_EnumUUID = AZ::Uuid("{5BF53F56-E744-471F-9A52-ECB47B42F454}");

    /////////
    // Node
    /////////

#if defined(OBJECT_STREAM_EDITOR_ASSET_LOADING_SUPPORT_ENABLED)////
    class NodeEventHandler
        : public AZ::SerializeContext::IEventHandler
    {
    public:
        void OnWriteEnd(void* objectPtr) override
        {
            auto node = reinterpret_cast<Node*>(objectPtr);
            node->OnDeserialize();
        }
    };
#endif//defined(OBJECT_STREAM_EDITOR_ASSET_LOADING_SUPPORT_ENABLED)

    bool NodeVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& nodeElementNode)
    {
        // AddName was mistakenly set to version 1 instead of a higher version than the previous version.
        // This caused issues with version conversion and nodes, and caused asset processing errors.
        // This early out check skips conversion that may have previously failed because the version number was mistakenly
        // set backward and was previously triggering incorrect conversion logic.
        // This is +1 because the current value is always the latest +1, and when AddName was the most recent, the saved node versions were AddName+1 and not AddName.
        // The version enum wasn't created until version MergeFromBackend2dotZero, so there are many additional version checks below here that could get triggered
        // and cause the version converter to think it failed.
        if (nodeElementNode.GetVersion() == NodeCpp::Version::AddName + 1)
        {
            // To avoid triggering those other version conversion checks, skip them by returning early.
            // Return true so the system knows this was handled.
            // This does mean that if someone tried to convert data from the old version 2 instead of the new version 2, it will fail.
            // That's a narrow edge case and would require data that is many years old, from before O3DE.
            return true;
        }
        if (nodeElementNode.GetVersion() <= 5)
        {
            auto slotVectorElementNodes = AZ::Utils::FindDescendantElements(context, nodeElementNode, AZStd::vector<AZ::Crc32>{AZ_CRC_CE("Slots"), AZ_CRC_CE("m_slots")});
            if (slotVectorElementNodes.empty())
            {
                AZ_Error("Script Canvas", false, "Node version %u is missing SlotContainer container structure", nodeElementNode.GetVersion());
                return false;
            }

            auto& slotVectorElementNode = slotVectorElementNodes.front();
            AZStd::vector<Slot> oldSlots;
            if (!slotVectorElementNode->GetData(oldSlots))
            {
                AZ_Error("Script Canvas", false, "Unable to retrieve the SlotContainer AZStd::vector<Slot> structure from Node version %u. Node version conversion has failed", nodeElementNode.GetVersion());
                return false;
            }

            // Datum -> VarDatum
            int datumArrayElementIndex = nodeElementNode.FindElement(AZ_CRC_CE("m_inputData"));
            if (datumArrayElementIndex == -1)
            {
                AZ_Error("Script Canvas", false, "Unable to find the Datum array structure on Node class version %u", nodeElementNode.GetVersion());
                return false;
            }

            auto& datumArrayElementNode = nodeElementNode.GetSubElement(datumArrayElementIndex);
            AZStd::vector<Datum> oldData;
            if (!datumArrayElementNode.GetData(oldData))
            {
                AZ_Error("Script Canvas", false, "Unable to retrieve the Datum array structure from Node version %u. Node version conversion has failed", nodeElementNode.GetVersion());
                return false;
            }

            // Retrieve the old AZStd::vector<Data::Type>
            int dataTypeArrayElementIndex = nodeElementNode.FindElement(AZ_CRC_CE("m_outputTypes"));
            if (dataTypeArrayElementIndex == -1)
            {
                AZ_Error("Script Canvas", false, "Unable to find the Data::Type array structure on the Node class version %u", nodeElementNode.GetVersion());
                return false;
            }

            auto& dataTypeArrayElementNode = nodeElementNode.GetSubElement(dataTypeArrayElementIndex);
            AZStd::vector<Data::Type> oldDataTypes;
            if (!dataTypeArrayElementNode.GetData(oldDataTypes))
            {
                AZ_Error("Script Canvas", false, "Unable to retrieve the Data::Type array structure from Node version %u. Node version conversion has failed", nodeElementNode.GetVersion());
                return false;
            }

            // Retrieve the Slot index -> Datum index map
            int slotDatumIndexMapElementIndex = nodeElementNode.FindElement(AZ_CRC_CE("m_inputIndexBySlotIndex"));
            if (slotDatumIndexMapElementIndex == -1)
            {
                AZ_Error("Script Canvas", false, "Unable to find the Slot Index to Data::Type Index Map on the Node class version %u", nodeElementNode.GetVersion());
                return false;
            }

            auto& slotDatumIndexMapElementNode = nodeElementNode.GetSubElement(slotDatumIndexMapElementIndex);
            AZStd::unordered_map<int, int> slotIndexToDatumIndexMap;
            if (!slotDatumIndexMapElementNode.GetData(slotIndexToDatumIndexMap))
            {
                AZ_Error("Script Canvas", false, "Unable to retrieve the Slot Index to Data::Type Index Map from Node version %u. Node version conversion has failed", nodeElementNode.GetVersion());
                return false;
            }

            // Retrieve the Slot index -> Data::Type index map
            int slotDataTypeIndexMapElementIndex = nodeElementNode.FindElement(AZ_CRC_CE("m_outputTypeIndexBySlotIndex"));
            if (slotDataTypeIndexMapElementIndex == -1)
            {
                AZ_Error("Script Canvas", false, "Unable to find the Slot Index to Data::Type Index Map on the Node class version %u", nodeElementNode.GetVersion());
                return false;
            }

            auto& slotDataTypeIndexMapElementNode = nodeElementNode.GetSubElement(slotDataTypeIndexMapElementIndex);
            AZStd::unordered_map<int, int> slotIndexToDataTypeIndexMap;
            if (!slotDataTypeIndexMapElementNode.GetData(slotIndexToDataTypeIndexMap))
            {
                AZ_Error("Script Canvas", false, "Unable to retrieve the Slot Index to Data::Type Index Map from Node version %u. Node version conversion has failed", nodeElementNode.GetVersion());
                return false;
            }

            AZStd::vector<VariableDatum> newVariableData;
            newVariableData.reserve(oldData.size());
            for (const auto& oldDatum : oldData)
            {
                newVariableData.emplace_back(VariableDatum(oldDatum));
            }

            AZStd::unordered_map<SlotId, Deprecated::VariableInfo> slotIdVarInfoMap;
            for (const auto& slotIndexDatumIndexPair : slotIndexToDatumIndexMap)
            {
                const auto& varId = newVariableData[slotIndexDatumIndexPair.second].GetId();
                const auto& dataType = newVariableData[slotIndexDatumIndexPair.second].GetData().GetType();
                slotIdVarInfoMap[oldSlots[slotIndexDatumIndexPair.first].GetId()].m_ownedVariableId = varId;
                slotIdVarInfoMap[oldSlots[slotIndexDatumIndexPair.first].GetId()].m_currentVariableId = varId;
                slotIdVarInfoMap[oldSlots[slotIndexDatumIndexPair.first].GetId()].m_dataType = dataType;
            }

            for (const auto& slotIndexDataTypeIndexPair : slotIndexToDataTypeIndexMap)
            {
                slotIdVarInfoMap[oldSlots[slotIndexDataTypeIndexPair.first].GetId()].m_dataType = oldDataTypes[slotIndexDataTypeIndexPair.second];
            }

            // Remove all the version 5 and below DataElements
            nodeElementNode.RemoveElementByName(AZ_CRC_CE("Slots"));
            nodeElementNode.RemoveElementByName(AZ_CRC_CE("m_outputTypes"));
            nodeElementNode.RemoveElementByName(AZ_CRC_CE("m_inputData"));
            nodeElementNode.RemoveElementByName(AZ_CRC_CE("m_inputIndexBySlotIndex"));
            nodeElementNode.RemoveElementByName(AZ_CRC_CE("m_outputTypeIndexBySlotIndex"));

            // Move the old slots from the AZStd::vector to an AZStd::list
            Node::SlotList newSlots{ AZStd::make_move_iterator(oldSlots.begin()), AZStd::make_move_iterator(oldSlots.end()) };
            if (nodeElementNode.AddElementWithData(context, "Slots", newSlots) == -1)
            {
                AZ_Error("Script Canvas", false, "Failed to add Slot List container to the serialized node element");
                return false;
            }

            // The new variable datum structure is a AZStd::list
            AZStd::list<VariableDatum> newVarDatums{ AZStd::make_move_iterator(newVariableData.begin()), AZStd::make_move_iterator(newVariableData.end()) };
            if (nodeElementNode.AddElementWithData(context, "Variables", newVarDatums) == -1)
            {
                AZ_Error("Script Canvas", false, "Failed to add Variable List container to the serialized node element");
                return false;
            }

            // Add the SlotId, VariableId Pair array to the Node
            if (nodeElementNode.AddElementWithData(context, "SlotToVariableInfoMap", slotIdVarInfoMap) == -1)
            {
                AZ_Error("Script Canvas", false, "Failed to add SlotId, Variable Id Pair array to the serialized node element");
                return false;
            }
        }

        if (nodeElementNode.GetVersion() <= 6)
        {
            // Finds the AZStd::list<VariableDatum> and replaces that with an AZStd::list<VariableDatumBase> which does not have the exposure/or visibility options
            AZStd::list<VariableDatum> oldVarDatums;
            if (!nodeElementNode.GetChildData(AZ_CRC_CE("Variables"), oldVarDatums))
            {
                AZ_Error("Script Canvas", false, "Unable to retrieve the Variable Datum list structure from Node version %u. Node version conversion has failed", nodeElementNode.GetVersion());
                return false;
            }

            nodeElementNode.RemoveElementByName(AZ_CRC_CE("Variables"));

            AZStd::list<Deprecated::VariableDatumBase> newVarDatumBases;
            for (const auto& oldVarDatum : oldVarDatums)
            {
                newVarDatumBases.emplace_back(oldVarDatum);
            }

            if (nodeElementNode.AddElementWithData(context, "Variables", newVarDatumBases) == -1)
            {
                AZ_Error("Script Canvas", false, "Failed to add Variable Datum Base list to the node element");
                return false;
            }
        }

        // Converting away from Variable Datums
        if (nodeElementNode.GetVersion() <= 9)
        {
            Node::SlotList slots;

            if (!nodeElementNode.GetChildData(AZ_CRC_CE("Slots"), slots))
            {
                return false;
            }

            AZStd::list<Deprecated::VariableDatumBase> varDatums;

            if (!nodeElementNode.GetChildData(AZ_CRC_CE("Variables"), varDatums))
            {
                return false;
            }

            AZStd::unordered_map<SlotId, Deprecated::VariableInfo> slotIdVarInfoMap;

            if (!nodeElementNode.GetChildData(AZ_CRC_CE("SlotToVariableInfoMap"), slotIdVarInfoMap))
            {
                return false;
            }

            // Create a variable mapping to the previous datum iterators for easier lookup
            AZStd::unordered_map<VariableId, typename AZStd::list<Deprecated::VariableDatumBase>::iterator> variableIdMap;

            for (auto varIter = varDatums.begin(); varIter != varDatums.end(); ++varIter)            
            {
                variableIdMap[varIter->GetId()] = varIter;
            }

            Node::DatumList datumList;

            // Create a look-up map for the slot Id's so we can manipulate the slots.
            AZStd::unordered_map < SlotId, Node::SlotIterator > slotIdMap;

            for (auto slotIter = slots.begin(); slotIter != slots.end(); ++slotIter)
            {
                slotIdMap[slotIter->GetId()] = slotIter;

                // We want to size the datum list to be the right amount so we can manage the insertion order correctly.
                if (slotIter->IsData() && slotIter->IsInput())
                {
                    datumList.emplace_back();
                }
            }

            // Iterate over the old variable id slot mapping.
            for (auto mapPair : slotIdVarInfoMap)
            {
                auto slotIter = slotIdMap.find(mapPair.first);

                if (slotIter == slotIdMap.end())
                {
                    continue;
                }

                if (slotIter->second == slots.end())
                {
                    AZ_Error("ScriptCanvas", false, "Null Slot in slotId map when attempting to version a node");
                    return false;
                }

                Slot& slotReference = (*slotIter->second);

                Deprecated::VariableInfo variableInfo = mapPair.second;
                
                // If the slot reference is an output. We don't want to register a datum for it, we just want to set the display
                // type of the slot to the correct element.
                if (slotReference.IsOutput())
                {
                    slotReference.SetDisplayType(variableInfo.m_dataType);
                }
                // If it's an input. We need to setup the datum list in the correct order to ensure that our internal mapping remains consistent
                else
                {
                    auto datumIter = variableIdMap.find(variableInfo.m_ownedVariableId);

                    if (datumIter == variableIdMap.end())
                    {
                        continue;
                    }

                    if (datumIter->second == varDatums.end())
                    {
                        AZ_Error("ScriptCanvas", false, "Variable datum not found when attempting to version node");
                        return false;
                    }

                    if (datumList.empty())
                    {
                        AZ_Error("ScriptCanvas", false, "Datum list is empty when attempting to version node");
                        return false;
                    }

                    Node::DatumIterator copyIterator = datumList.begin();

                    for (auto offsetSlotIter = slots.begin(); offsetSlotIter != slotIter->second; ++offsetSlotIter)
                    {
                        if (offsetSlotIter->IsData() && offsetSlotIter->IsInput())
                        {
                            ++copyIterator;

                            if (copyIterator == datumList.end())
                            {
                                break;
                            }
                        }
                    }

                    (*copyIterator).ReconfigureDatumTo(datumIter->second->GetData());
                }
            }

            // Remove the old data.
            nodeElementNode.RemoveElementByName(AZ_CRC_CE("Slots"));
            nodeElementNode.RemoveElementByName(AZ_CRC_CE("Variables"));
            nodeElementNode.RemoveElementByName(AZ_CRC_CE("SlotToVariableInfoMap"));

            // Push in the new data.
            nodeElementNode.AddElementWithData(context, "Slots", slots);
            nodeElementNode.AddElementWithData(context, "Datums", datumList);
        }

        if (nodeElementNode.GetVersion() < 14)
        {
            bool enabled;
            if (nodeElementNode.GetChildData(AZ_CRC_CE("Enabled"), enabled))
            {
                nodeElementNode.RemoveElementByName(AZ_CRC_CE("Enabled"));
                NodeDisabledFlag disabledFlag = NodeDisabledFlag::None;
                if (!enabled)
                {
                    disabledFlag = NodeDisabledFlag::User;
                }
                if (nodeElementNode.AddElementWithData(context, "NodeDisabledFlag", disabledFlag) == -1)
                {
                    AZ_Assert(false, "Unable to add NodeState data in Node version %u.", nodeElementNode.GetVersion());
                    return false;
                }
            }
        }

        // Deprecated field, just remove without version check
        nodeElementNode.RemoveElementByName(AZ_CRC_CE("UniqueGraphID"));
        nodeElementNode.RemoveElementByName(AZ_CRC_CE("ExecutionType"));

        return true;
    }

    void Node::Reflect(AZ::ReflectContext* context)
    {
        Slot::Reflect(context);
        Nodes::NodeableNode::Reflect(context);

        // Version Conversion Reflection
        Deprecated::VariableInfo::Reflect(context);
        Deprecated::VariableDatumBase::Reflect(context);
        ////

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            // Needed to serialize in the AZStd::vector<Slot> from the old SlotContainer class 
            serializeContext->RegisterGenericType<AZStd::vector<Slot>>();

            // Needed to serialize in the AZStd::vector<Datum> from this class
            serializeContext->RegisterGenericType<AZStd::vector<Datum>>();

            // Needed to serialize in the AZStd::vector<Data::Type> from version 5 and below
            serializeContext->RegisterGenericType<AZStd::vector<Data::Type>>();

            // Needed to serialize in the AZStd::unordered<int, int> from version 5 and below
            serializeContext->RegisterGenericType<AZStd::unordered_map<int, int>>();            

            // Needed to serialize in the AZStd::list<Deprecated::VariableDatumBase> from version 6 and below
            serializeContext->RegisterGenericType<AZStd::list<Deprecated::VariableDatum>>();
            serializeContext->RegisterGenericType<AZStd::list<Deprecated::VariableDatumBase>>();
            serializeContext->RegisterGenericType<AZStd::unordered_map<SlotId, Deprecated::VariableInfo>>();

            serializeContext->Class<Node, AZ::Component>()
#if defined(OBJECT_STREAM_EDITOR_ASSET_LOADING_SUPPORT_ENABLED)////
                ->EventHandler<NodeEventHandler>()
#endif//defined(OBJECT_STREAM_EDITOR_ASSET_LOADING_SUPPORT_ENABLED)
                ->Version(NodeCpp::Version::Current, &NodeVersionConverter)
                ->Field("Slots", &Node::m_slots)
                ->Field("Datums", &Node::m_slotDatums)
                ->Field("NodeDisabledFlag", &Node::m_disabledFlag)
                ->Field("Name", &Node::m_name)
                ->Field("ToolTip", &Node::m_toolTip)
                ->Field("Style", &Node::m_nodeStyle)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<Node>("Node", "Node")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Node::m_slotDatums, "Input", "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;
            }
        }
    }

    int Node::GetNodeVersion()
    {
        return NodeCpp::Version::Current;
    }

    // Class Definition

    Node::Node()
        : AZ::Component()
    {
    }

    Node::Node(const Node&)
    {}

    Node& Node::operator=(const Node&)
    {
        return *this;
    }

    Node::~Node()
    {
        DatumNotificationBus::Handler::BusDisconnect();
        NodeRequestBus::Handler::BusDisconnect();
    }

    void Node::Init()
    {
        const auto& entityId = GetEntityId();

        NodeRequestBus::Handler::BusConnect(entityId);
        DatumNotificationBus::Handler::BusConnect(entityId);

        for (auto& slot : m_slots)
        {
            slot.SetNode(this);

            EndpointNotificationBus::MultiHandler::BusConnect(slot.GetEndpoint());
        }

        for (Datum& datum : m_slotDatums)
        {
            datum.SetNotificationsTarget(entityId);
        }

        OnInit();
        PopulateNodeType();
        ConfigureVisualExtensions();
    }

    void Node::Activate()
    {
        m_graphRequestBus = GraphRequestBus::FindFirstHandler(m_scriptCanvasId);
        AZ_Assert(m_graphRequestBus, "Invalid m_executionUniqueId given for RuntimeRequestBus");
        OnActivate();
        MarkDefaultableInput();
    }

    void Node::Deactivate()
    {
        OnDeactivate();
        m_graphRequestBus = GraphRequestBus::FindFirstHandler(m_scriptCanvasId);
    }

    void Node::PostActivate()
    {
        for (auto& currentSlot : m_slots)
        {
            currentSlot.InitializeVariables();
        }

        OnPostActivate();
    }

    void Node::SignalDeserialized()
    {
        for (auto& currentSlot : m_slots)
        {
            currentSlot.InitializeVariables();
        }

        OnDeserialized();
    }

    void Node::PopulateNodeType()
    {
        m_nodeType = NodeUtils::ConstructNodeType(this);
    }

    void Node::Configure()
    {
        ConfigureSlots();
        OnConfigured();
    }

    AZStd::string Node::GetSlotName(const SlotId& slotId) const
    {
        if (slotId.IsValid())
        {
            auto slot = GetSlot(slotId);
            if (slot)
            {
                return slot->GetName();
            }
        }
        return "";
    }

    SlotsOutcome Node::GetSlots(const AZStd::vector<SlotId>& slotIds)
    {
        AZStd::vector<Slot*> slots;
        slots.reserve(slotIds.size());

        for (const auto& slotId : slotIds)
        {
            if (const auto slot = GetSlot(slotId))
            {
                slots.push_back(slot);
            }
            else
            {
                return AZ::Failure(AZStd::string::format("No slot found for slotId %s", slotId.ToString().data()));
            }
        }

        return AZ::Success(slots);
    }

    ConstSlotsOutcome Node::GetSlots(const AZStd::vector<SlotId>& slotIds) const
    {
        AZStd::vector<const Slot*> slots;
        slots.reserve(slotIds.size());

        for (const auto& slotId : slotIds)
        {
            if (const auto slot = GetSlot(slotId))
            {
                slots.push_back(slot);
            }
            else
            {
                return AZ::Failure(AZStd::string::format("No slot found for slotId %s", slotId.ToString().data()));
            }
        }

        return AZ::Success(slots);
    }

    AZStd::vector< Slot* > Node::GetSlotsWithDisplayGroup(AZStd::string_view displayGroup) const
    {
        AZ::Crc32 displayGroupId = AZ::Crc32(displayGroup);
        AZStd::vector< Slot* > displayGroupSlots;

        for (const Slot& currentSlot : m_slots)
        {
            if (currentSlot.GetDisplayGroup() == displayGroupId)
            {
                displayGroupSlots.emplace_back(GetSlot(currentSlot.GetId()));
            }
        }

        return displayGroupSlots;
    }

    AZStd::vector< Slot* > Node::GetSlotsWithDynamicGroup(const AZ::Crc32& dynamicGroup) const
    {
        AZStd::vector< Slot* > dynamicGroupSlots;
        auto equalRange = m_dynamicGroups.equal_range(dynamicGroup);

        for (auto slotIter = equalRange.first; slotIter != equalRange.second; ++slotIter)
        {
            Slot* slot = GetSlot(slotIter->second);

            dynamicGroupSlots.emplace_back(slot);
        }

        return dynamicGroupSlots;
    }

    void Node::RebuildInternalState()
    {
        m_slotIdIteratorCache.clear();
        m_slotNameMap.clear();
        m_dynamicGroups.clear();
        m_dynamicGroupDisplayTypes.clear();

        auto datumIter = m_slotDatums.begin();

        for (auto slotIter = m_slots.begin(); slotIter != m_slots.end(); ++slotIter)
        {
            IteratorCache cache;

            cache.m_slotIterator = slotIter;            

            // Manage the datum iterator here as well
            if (slotIter->IsData() && slotIter->IsInput())
            {
                if (datumIter != m_slotDatums.end())
                {
                    cache.SetDatumIterator(datumIter);
                    ++datumIter;
                }
            }

            m_slotIdIteratorCache.emplace(slotIter->GetId(), cache);
            m_slotNameMap.emplace(slotIter->GetName(), slotIter);

            if (slotIter->IsDynamicSlot())
            {
                AZ::Crc32 dynamicGroup = slotIter->GetDynamicGroup();

                if (dynamicGroup != AZ::Crc32())
                {
                    m_dynamicGroups.insert(AZStd::make_pair(dynamicGroup, slotIter->GetId()));

                    if (slotIter->HasDisplayType())
                    {
                        m_dynamicGroupDisplayTypes[dynamicGroup] = slotIter->GetDisplayType();
                    }
                }
            }
        }
    }

    void Node::ProcessDataSlot(Slot& slot)
    {
        if (!slot.IsDynamicSlot())
        {
            return;
        }

        AZ::Crc32 dynamicGroup = slot.GetDynamicGroup();

        if (dynamicGroup != AZ::Crc32())
        {
            m_dynamicGroups.insert(AZStd::make_pair(dynamicGroup, slot.GetId()));

            auto displayTypeIter = m_dynamicGroupDisplayTypes.find(dynamicGroup);

            if (displayTypeIter != m_dynamicGroupDisplayTypes.end())
            {
                if (slot.IsTypeMatchFor(displayTypeIter->second))
                {
                    slot.SetDisplayType(displayTypeIter->second);
                }
                else
                {
                    ClearDisplayType(dynamicGroup);
                }
            }
            else if (slot.HasDisplayType())
            {
                m_dynamicGroupDisplayTypes[dynamicGroup] = slot.GetDisplayType();
            }
        }

        EndpointNotificationBus::MultiHandler::BusConnect(slot.GetEndpoint());
    }

    void Node::OnNodeStateChanged()
    {
        if (IsNodeEnabled())
        {
            NodeNotificationsBus::Event(GetEntityId(), &NodeNotifications::OnNodeEnabled);
        }
        else
        {
            NodeNotificationsBus::Event(GetEntityId(), &NodeNotifications::OnNodeDisabled);
        }
    }

    void Node::MarkDefaultableInput()
    {
        for (const auto& cachePair : m_slotIdIteratorCache)
        {
            const auto& inputSlot = (*cachePair.second.m_slotIterator);
            const auto& slotId = inputSlot.GetId();

            if (inputSlot.GetDescriptor() == SlotDescriptors::DataIn())
            {
                // for each output slot...
                // for each connected node...
                // remove the ability to default it...
                
                EndpointsResolved connections = GetConnectedNodes(inputSlot);
                if (!connections.empty())
                {
                    m_possiblyStaleInput.insert(slotId);
                }
            }
        }
    }

    bool Node::IsInEventHandlingScope(const ID& possibleEventHandler) const
    {
        Node* node = m_graphRequestBus->FindNode(possibleEventHandler);

        if (auto eventHandler = azrtti_cast<ScriptCanvas::Nodes::Core::EBusEventHandler*>(node))
        {
            auto eventSlots = eventHandler->GetEventSlotIds();
            AZStd::unordered_set<ID> path;
            return IsInEventHandlingScope(possibleEventHandler, eventSlots, {}, path);
        }

        return false;
    }

    bool Node::IsInEventHandlingScope(const ID& eventHandler, const AZStd::vector<SlotId>& eventSlots, const SlotId& connectionSlot, AZStd::unordered_set<ID>& path) const
    {
        const ID candidateNodeId = GetEntityId();

        if (candidateNodeId == eventHandler)
        {
            return AZStd::find(eventSlots.begin(), eventSlots.end(), connectionSlot) != eventSlots.end();
        }
        else if (path.find(candidateNodeId) != path.end())
        {
            return false;
        }

        // prevent loops in the search
        path.insert(candidateNodeId);

        // check all parents of the candidate for a path to the handler
        auto connectedNodes = FindConnectedNodesAndSlotsByDescriptor(SlotDescriptors::ExecutionIn());

        //  for each connected parent
        for (auto& node : connectedNodes)
        {
            // return true if that parent is the event handler we're looking for, and we're connected to an event handling execution slot
            if (node.first->IsInEventHandlingScope(eventHandler, eventSlots, node.second, path))
            {
                return true;
            }
        }

        return false;
    }

    bool Node::IsTargetInDataFlowPath(const Node* targetNode) const
    {
        AZStd::unordered_set<ID> path;
        return (targetNode && IsTargetInDataFlowPath(targetNode->GetEntityId(), path));
    }

    bool Node::IsTargetInDataFlowPath(const ID& targetNodeId, AZStd::unordered_set<ID>& path) const
    {
        const ID candidateNodeId = GetEntityId();

        if (!targetNodeId.IsValid() || !candidateNodeId.IsValid())
        {
            return false;
        }

        if (candidateNodeId == targetNodeId)
        {
            // an executable path from the source to the target has been found
            return true;
        }
        else if (IsInEventHandlingScope(targetNodeId)) // targetNodeId is handler, and this node resides in that event handlers event execution slots
        {
            // this node pushes data into handled event as results for that event
            return true;
        }
        else if (path.find(candidateNodeId) != path.end())
        {
            // a loop has been encountered, without yielding a path
            return false;
        }

        // If we are the first node in the chain, we want to explore our latent connections
        bool exploreLatentConnections = path.empty();

        // prevent loops in the search
        path.insert(candidateNodeId);

        // check all children of the candidate for a path to the target
        auto connectedNodes = FindConnectedNodesByDescriptor(SlotDescriptors::ExecutionOut(), exploreLatentConnections);

        //  for each connected child
        for (auto& node : connectedNodes)
        {
            // return true if that child is in the data flow path of target node
            if (node->IsTargetInDataFlowPath(targetNodeId, path))
            {
                return true;
            }
        }

        return false;
    }

    void Node::RefreshInput()
    {
        for (const auto& slotID : m_possiblyStaleInput)
        {
            SetToDefaultValueOfType(slotID);
        }
    }

    GraphVariable* Node::FindGraphVariable(const VariableId& variableId) const
    {
        return m_graphRequestBus->FindVariableById(variableId);      
    }

    void Node::OnSlotConvertedToValue(const SlotId& slotId)
    {
        SanityCheckDynamicDisplay();

        EndpointNotificationBus::Event(ScriptCanvas::Endpoint(GetEntityId(), slotId), &EndpointNotifications::OnEndpointConvertedToValue);
    }

    void Node::OnSlotConvertedToReference(const SlotId& slotId)
    {
        SanityCheckDynamicDisplay();

        EndpointNotificationBus::Event(ScriptCanvas::Endpoint(GetEntityId(), slotId), &EndpointNotifications::OnEndpointConvertedToReference);
    }

    bool Node::ValidateNode(ValidationResults& validationResults)
    {
        AZStd::vector<SlotId> untypedSlots;
        AZStd::vector<SlotId> invalidReferences;

        for (const ScriptCanvas::Slot& currentSlot : GetSlots())
        {
            if (currentSlot.IsDynamicSlot())
            {
                if (!currentSlot.HasDisplayType())
                {
                    untypedSlots.emplace_back(currentSlot.GetId());
                }
            }

            if (currentSlot.IsVariableReference())
            {
                if (!currentSlot.GetVariableReference().IsValid() || currentSlot.GetVariable() == nullptr)
                {
                    invalidReferences.emplace_back(currentSlot.GetId());
                }
            }
        }

        bool spawnedError = false;

        if (!untypedSlots.empty())
        {
            spawnedError = true;
            auto validationEvent = aznew UnspecifiedDynamicDataTypeEvent(GetEntityId(), untypedSlots);

            validationResults.AddValidationEvent(validationEvent);
        }

        if (!invalidReferences.empty())
        {
            spawnedError = true;
            auto validationEvent = aznew InvalidReferenceEvent(GetEntityId(), invalidReferences);

            validationResults.AddValidationEvent(validationEvent);
        }

        return OnValidateNode(validationResults) && spawnedError;
    }

    bool Node::IsOutOfDate(const VersionData& graphVersion) const
    {
        AZ_UNUSED(graphVersion);
        return false;
    }

    UpdateResult Node::UpdateNode()
    {
        UpdateResult result = UpdateResult::DirtyGraph;

        AZStd::unordered_map<SlotId, SlotVersionCache> versionCache;

        for (const ScriptCanvas::Slot& currentSlot : GetSlots())
        {
            SlotVersionCache cache;
            cache.m_slotId = currentSlot.GetId();
            cache.m_originalName = currentSlot.GetName();

            if (currentSlot.IsVariableReference())
            {
                cache.m_variableId = currentSlot.GetVariableReference();
            }
            else if (currentSlot.IsData())
            {
                ScriptCanvas::ModifiableDatumView datumView;
                FindModifiableDatumView(cache.m_slotId, datumView);

                if (datumView.IsValid())
                {
                    cache.m_slotDatum = AZStd::move(datumView.CloneDatum());
                }
            }

            versionCache[currentSlot.GetId()] = AZStd::move(cache);
        }

        m_isUpdatingNode = true;
        result = OnUpdateNode();
        m_isUpdatingNode = false;

        for (ScriptCanvas::Slot& currentSlot : m_slots)
        {
            const ScriptCanvas::SlotId& slotId = currentSlot.GetId();

            auto lookupIter = versionCache.find(slotId);

            if (lookupIter == versionCache.end())
            {
                continue;
            }

            // Update any previously cached data and signal out to keep elements in sync correctly.
            SlotVersionCache& slotCache = lookupIter->second;

            if (currentSlot.IsData())
            {
                if (slotCache.m_variableId.IsValid())
                {
                    if (currentSlot.ConvertToReference())
                    {
                        ScriptCanvas::GraphVariable* variable = FindGraphVariable(slotCache.m_variableId);

                        if (variable && IsValidTypeForSlot(currentSlot.GetId(), variable->GetDataType()))
                        {
                            currentSlot.SetVariableReference(slotCache.m_variableId);
                        }

                        if (currentSlot.GetVariableReference().IsValid())
                        {
                            currentSlot.InitializeVariables();
                        }
                    }
                }
                else
                {
                    ScriptCanvas::ModifiableDatumView datumView;
                    FindModifiableDatumView(slotId, datumView);

                    // If our types are the same. Maintain our Data.
                    if (datumView.GetDataType() == slotCache.m_slotDatum.GetType())
                    {
                        datumView.AssignToDatum(slotCache.m_slotDatum);
                    }
                    // Otherwise signal out types changing. Invalid connections will be removed
                    // once all versioning in complete
                    else if (!currentSlot.IsDynamicSlot())
                    {
                        currentSlot.SignalTypeChanged(datumView.GetDataType());
                    }

                    // If we are a dynamic slot. Update the display type.
                    if (currentSlot.IsDynamicSlot())
                    {
                        currentSlot.SetDisplayType(datumView.GetDataType());
                    }
                }
            }

            if (slotCache.m_originalName.compare(currentSlot.GetName()) != 0)
            {
                currentSlot.SignalRenamed();
            }
        }
        return result;
    }

    AZStd::string Node::GetUpdateString() const
    {
        return "Updated Node";
    }

    bool Node::OnValidateNode(ValidationResults& validationResults)
    {
        AZ_UNUSED(validationResults);
        return true;
    }

    void Node::SignalSlotsReordered()
    {
        NodeNotificationsBus::Event(GetEntityId(), &NodeNotifications::OnSlotsReordered);
    }

    void Node::SetToDefaultValueOfType(const SlotId& slotId)
    {
        AZ_PROFILE_SCOPE(ScriptCanvas, "ScriptCanvas::Node::SetToDefaultValueOfType");

        Slot* slot = GetSlot(slotId);

        if (slot)
        {
            // If the slot is a variable reference. Leave it's datum alone.
            if (slot->IsVariableReference())
            {
                return;
            }

            ModifiableDatumView datumView;
            FindModifiableDatumView(slotId, datumView);

            datumView.SetToDefaultValueOfType();
        }
    }

    TransientSlotIdentifier Node::ConstructTransientIdentifier(const Slot& slot) const
    {
        TransientSlotIdentifier slotIdentifier;
        slotIdentifier.m_name = slot.GetName();
        slotIdentifier.m_slotDescriptor = slot.GetDescriptor();

        auto iteratorCacheSlot = m_slotIdIteratorCache.find(slot.GetId());

        if (iteratorCacheSlot != m_slotIdIteratorCache.end())
        {
            SlotList::const_iterator constIter = iteratorCacheSlot->second.m_slotIterator;
            slotIdentifier.m_index = aznumeric_cast<int>(AZStd::distance(m_slots.begin(), constIter));
        }

        return slotIdentifier;
    }

    Node::DatumVector Node::GatherDatumsForDescriptor(SlotDescriptor descriptor) const
    {
        DatumVector datumVectors;

        datumVectors.reserve(m_slots.size());

        for (const Slot& slot : m_slots)
        {
            if (slot.GetDescriptor() == descriptor)
            {
                datumVectors.emplace_back(FindDatum(slot.GetId()));
            }
        }

        return datumVectors;
    }

    SlotDataMap Node::CreateInputMap() const
    {
        SlotDataMap map;

        for (auto& slot : m_slots)
        {
            if (slot.GetDescriptor() == SlotDescriptors::DataIn())
            {
                if (const Datum* datum = FindDatum(slot.GetId()))
                {
                    NamedSlotId namedSlotId(slot.GetId(), slot.GetName());

                    if (!datum->IS_A(ScriptCanvas::Data::Type::EntityID()))
                    {
                        map.emplace(namedSlotId, DatumValue(*datum));
                    }
                    else
                    {
                        const AZ::EntityId* entityId = datum->GetAs<AZ::EntityId>();
                        map.emplace(namedSlotId, DatumValue(Datum(AZ::NamedEntityId((*entityId)))));
                    }
                }
            }
        }

        return map;
    }

    SlotDataMap Node::CreateOutputMap() const
    {
        return SlotDataMap();
    }

    AZStd::string Node::CreateInputMapString(const SlotDataMap& map) const
    {
        AZStd::string result;

        for (auto& iter : map)
        {
            if (auto slot = GetSlot(iter.first))
            {
                result += slot->GetName();
            }
            else
            {
                result += iter.first.ToString();
            }

            result += ": ";
            result += iter.second.m_datum.ToString();
            result += ", ";
        }

        return result;
    }

    bool Node::IsNodeType(const NodeTypeIdentifier& nodeIdentifier) const
    {
        return nodeIdentifier == GetNodeType();
    }

    NodeTypeIdentifier Node::GetNodeType() const
    {
        return m_nodeType;
    }

    void Node::ResetSlotToDefaultValue(const SlotId& slotId)
    {
        Slot* slot = GetSlot(slotId);

        if (slot)
        {
            if (slot->IsVariableReference())
            {
                slot->ClearVariableReference();

                if (!slot->IsDynamicSlot() || slot->HasDisplayType())
                {
                    InitializeVariableReference((*slot), {});
                }
            }
            else
            {
                ModifiableDatumView datumView;
                FindModifiableDatumView(slotId, datumView);

                if (datumView.IsValid())
                {
                    OnResetDatumToDefaultValue(datumView);
                }
            }

            NodeNotificationsBus::Event(GetEntityId(), &NodeNotifications::OnSlotInputChanged, slotId);
        }
    }

    void Node::ResetProperty(const AZ::Crc32& propertyId)
    {
        NodePropertyInterface* propertyInterface = GetPropertyInterface(propertyId);

        if (propertyInterface)
        {
            propertyInterface->ResetToDefault();
        }
    }

    bool Node::HasExtensions() const
    {
        return !m_visualExtensions.empty();
    }

    void Node::RegisterExtension(const VisualExtensionSlotConfiguration& configuration)
    {
        m_visualExtensions.emplace_back(configuration);
    }

    const AZStd::vector< VisualExtensionSlotConfiguration >& Node::GetVisualExtensions() const
    {
        return m_visualExtensions;
    }

    bool Node::CanDeleteSlot([[maybe_unused]] const SlotId& slotId) const
    {
        return false;
    }    

    SlotId Node::HandleExtension(AZ::Crc32 extensionId)
    {
        AZ_UNUSED(extensionId);
        return SlotId();
    }

    void Node::ExtensionCancelled(AZ::Crc32 extensionId)
    {
        AZ_UNUSED(extensionId);
    }

    void Node::FinalizeExtension(AZ::Crc32 extensionId)
    {
        AZ_UNUSED(extensionId);
    }

    NodePropertyInterface* Node::GetPropertyInterface(AZ::Crc32 propertyInterface)
    {
        AZ_UNUSED(propertyInterface);
        return nullptr;
    }

    NodeTypeIdentifier Node::GetOutputNodeType(const SlotId& slotId) const
    {
        AZ_UNUSED(slotId);
        return GetNodeType();
    }

    NodeTypeIdentifier Node::GetInputNodeType(const SlotId& slotId) const
    {
        AZ_UNUSED(slotId);
        return GetNodeType();
    }

    NamedEndpoint Node::CreateNamedEndpoint(SlotId slotId) const
    {
        auto slot = GetSlot(slotId);
        return NamedEndpoint(GetEntityId(), GetNodeName(), slotId, slot ? slot->GetName() : "");
    }

    void Node::SignalReconfigurationBegin()
    {
        m_nodeReconfigured = true;
        m_nodeReconfiguring = true;
        OnReconfigurationBegin();
    }

    void Node::SignalReconfigurationEnd()
    {
        
        OnReconfigurationEnd();        
    }

    void Node::ClearDisplayType(const AZ::Crc32& dynamicGroup, ExploredDynamicGroupCache& exploredGroupCache)
    {
        SetDisplayType(dynamicGroup, Data::Type::Invalid(), exploredGroupCache);
    }

    void Node::SetDisplayType(const AZ::Crc32& dynamicGroup, const Data::Type& dataType, ExploredDynamicGroupCache& exploredGroupCache, bool forceDisplaySet)
    {
        if (m_queueDisplayUpdates)
        {
            m_queuedDisplayUpdates[dynamicGroup] = dataType;
            return;
        }

        // Ensure that we don't do anything if we are already displaying the specified data type.
        auto currentDisplayIter = m_dynamicGroupDisplayTypes.find(dynamicGroup);

        if (currentDisplayIter != m_dynamicGroupDisplayTypes.end() && (!forceDisplaySet && currentDisplayIter->second == dataType))
        {
            return;
        }

        auto range = m_dynamicGroups.equal_range(dynamicGroup);

        if (dataType.IsValid())
        {
            m_dynamicGroupDisplayTypes[dynamicGroup] = dataType;
        }
        else
        {
            m_dynamicGroupDisplayTypes.erase(dynamicGroup);
        }

        OnDynamicGroupTypeChangeBegin(dynamicGroup);

        exploredGroupCache[GetEntityId()].insert(dynamicGroup);

        for (auto iter = range.first; iter != range.second; ++iter)
        {
            Slot* slot = GetSlot(iter->second);

            ConfigureSlotDisplayType(slot, dataType, exploredGroupCache);
        }

        OnDynamicGroupDisplayTypeChanged(dynamicGroup, dataType);
    }

    void Node::ConfigureSlotDisplayType(Slot* slot, const Data::Type& dataType, ExploredDynamicGroupCache& exploredGroupCache)
    {
        if (slot)
        {
            slot->SetDisplayType(dataType);
        }

        auto connectedNodes = ModConnectedNodes((*slot));

        for (auto endpointPair : connectedNodes)
        {
            Slot* connectedSlot = endpointPair.first->GetSlot(endpointPair.second);

            // If the slot is dynamic, we want to update its display type as well.
            if (connectedSlot && connectedSlot->IsDynamicSlot())
            {
                AZ::Crc32 connectedDynamicGroup = connectedSlot->GetDynamicGroup();

                if (connectedDynamicGroup != AZ::Crc32())
                {
                    AZ::EntityId connectedNodeId = endpointPair.first->GetEntityId();

                    auto exploredIter = exploredGroupCache.find(connectedNodeId);

                    // If we've already explored a group for a node we don't want to do it again.
                    if (exploredIter != exploredGroupCache.end() && exploredIter->second.count(connectedDynamicGroup) != 0)
                    {
                        continue;
                    }

                    endpointPair.first->SetDisplayType(connectedDynamicGroup, dataType, exploredGroupCache);
                }
                else
                {
                    connectedSlot->SetDisplayType(dataType);
                }
            }
        }
    }

    void Node::ClearDisplayType(const SlotId& slotId)
    {
        Slot* slot = GetSlot(slotId);

        if (slot)
        {
            AZ::Crc32 dynamicGroup = slot->GetDynamicGroup();

            if (dynamicGroup == AZ::Crc32())
            {
                ConfigureSlotDisplayType(slot, Data::Type::Invalid());
            }
            else
            {
                ClearDisplayType(dynamicGroup);
            }
        }
    }

    void Node::SetDisplayType(const SlotId& slotId, const Data::Type& dataType)
    {
        Slot* slot = GetSlot(slotId);

        if (slot)
        {
            AZ::Crc32 dynamicGroup = slot->GetDynamicGroup();

            if (dynamicGroup == AZ::Crc32())
            {
                ConfigureSlotDisplayType(slot, dataType);
            }
            else
            {
                SetDisplayType(dynamicGroup, dataType);
            }
        }
    }

    Data::Type Node::GetDisplayType(const AZ::Crc32& dynamicGroup) const
    {
        auto groupIter = m_dynamicGroupDisplayTypes.find(dynamicGroup);

        if (groupIter != m_dynamicGroupDisplayTypes.end())
        {
            return groupIter->second;
        }

        return Data::Type::Invalid();
    }

    Data::Type Node::FindConcreteDisplayType(const AZ::Crc32& dynamicGroup, ExploredDynamicGroupCache& exploredGroupCache) const
    {
        auto range = m_dynamicGroups.equal_range(dynamicGroup);

        exploredGroupCache[GetEntityId()].insert(dynamicGroup);

        for (auto iter = range.first; iter != range.second; ++iter)
        {
            Slot* slot = GetSlot(iter->second);

            if (slot)
            {
                auto connectedDataType = FindConnectedConcreteDisplayType((*slot), exploredGroupCache);
                if (connectedDataType.IsValid())
                {
                    return connectedDataType;
                }
            }
        }

        return Data::Type::Invalid();
    }

    bool Node::HasDynamicGroup(const AZ::Crc32& dynamicGroup) const
    {
        return m_dynamicGroups.count(dynamicGroup) > 0;
    }

    void Node::SetDynamicGroup(const SlotId& slotId, const AZ::Crc32& dynamicGroup)
    {
        Slot* slot = GetSlot(slotId);

        if (slot)
        {
            slot->SetDynamicGroup(dynamicGroup);
            ProcessDataSlot((*slot));
        }
    }

    Data::Type Node::FindConnectedConcreteDisplayType(const Slot& slot, ExploredDynamicGroupCache& exploredGroupCache) const
    {
        if (slot.IsVariableReference() && slot.GetVariableReference().IsValid())
        {
            return slot.GetDataType();
        }

        auto connectedNodes = GetConnectedNodes(slot);

        for (auto endpointPair : connectedNodes)
        {
            const Slot* connectedSlot = endpointPair.second;

            if (connectedSlot == nullptr)
            {
                continue;
            }

            // If the slot isn't dynamic, this means it has a concrete type.
            if (!connectedSlot->IsDynamicSlot())
            {
                return connectedSlot->GetDataType();
            }
            else
            {
                AZ::Crc32 connectedDynamicGroup = connectedSlot->GetDynamicGroup();

                if (connectedDynamicGroup != AZ::Crc32())
                {
                    AZ::EntityId connectedNodeId = endpointPair.first->GetEntityId();

                    auto exploredIter = exploredGroupCache.find(connectedNodeId);

                    // If we've already explored a group for a node we don't want to do it again.
                    if (exploredIter != exploredGroupCache.end() && exploredIter->second.count(connectedDynamicGroup) != 0)
                    {
                        continue;
                    }

                    auto concreteDisplayType = endpointPair.first->FindConcreteDisplayType(connectedDynamicGroup, exploredGroupCache);
                    if (concreteDisplayType.IsValid())
                    {
                        return concreteDisplayType;
                    }
                }
                else if (connectedSlot->HasDisplayType())
                {
                    return connectedSlot->GetDisplayType();
                }
            }
        }

        return FindFixedDataTypeForSlot(slot);
    }

    AZ::Outcome<void, AZStd::string> Node::IsValidTypeForSlotInternal(Slot* slot, const Data::Type& dataType, ExploredDynamicGroupCache& exploredGroupCache) const
    {
        auto isValidTypeForSlot = slot->IsTypeMatchFor(dataType);
        if (!isValidTypeForSlot)
        {
            return isValidTypeForSlot;
        }

        auto connectedNodes = GetConnectedNodes((*slot));

        for (auto endpointPair : connectedNodes)
        {
            const Slot* connectedSlot = endpointPair.second;

            if (connectedSlot->IsDynamicSlot())
            {
                AZ::Crc32 connectedDynamicGroup = connectedSlot->GetDynamicGroup();

                if (connectedDynamicGroup != AZ::Crc32())
                {
                    AZ::EntityId connectedNodeId = endpointPair.first->GetEntityId();

                    auto exploredIter = exploredGroupCache.find(connectedNodeId);

                    // If we've already explored a group for a node we don't want to do it again.
                    if (exploredIter != exploredGroupCache.end() && exploredIter->second.count(connectedDynamicGroup) != 0)
                    {
                        continue;
                    }

                    isValidTypeForSlot = endpointPair.first->IsValidTypeForGroupInternal(connectedDynamicGroup, dataType, exploredGroupCache);
                }
                else
                {
                    isValidTypeForSlot = connectedSlot->IsTypeMatchFor(dataType);
                }

                if (!isValidTypeForSlot)
                {
                    return isValidTypeForSlot;
                }
            }
        }

        return AZ::Success();
    }

    AZ::Outcome<void, AZStd::string> Node::IsValidTypeForGroupInternal(const AZ::Crc32& dynamicGroup, const Data::Type& dataType, ExploredDynamicGroupCache& exploredGroupCache) const
    {
        AZ::Outcome<void, AZStd::string> isValidTypeForGroup;

        exploredGroupCache[GetEntityId()].insert(dynamicGroup);

        auto range = m_dynamicGroups.equal_range(dynamicGroup);

        for (auto iter = range.first; iter != range.second; ++iter)
        {
            Slot* slot = GetSlot(iter->second);

            if (slot)
            {
                auto isValidTypeForSlot = IsValidTypeForSlotInternal(slot, dataType, exploredGroupCache);

                if (!isValidTypeForSlot)
                {
                    return isValidTypeForSlot;
                }
            }
        }

        return AZ::Success();
    }

    void Node::SignalSlotDisplayTypeChanged(const SlotId& slotId, const Data::Type& dataType)
    {
        OnSlotDisplayTypeChanged(slotId, dataType);
        NodeNotificationsBus::Event(GetEntityId(), &NodeNotifications::OnSlotDisplayTypeChanged, slotId, dataType);
    }

    AZ::Outcome<void, AZStd::string> Node::SlotAcceptsType(const SlotId& slotID, const Data::Type& type) const
    {
        if (auto slot = GetSlot(slotID))
        {
            if (slot->IsData())
            {
                AZ::Crc32 dynamicGroup = slot->GetDynamicGroup();

                if (dynamicGroup == AZ::Crc32())
                {
                    ExploredDynamicGroupCache dynamicGroupCache;
                    return IsValidTypeForSlotInternal(slot, type, dynamicGroupCache);
                }
                else
                {
                    return IsValidTypeForGroup(dynamicGroup, type);
                }
            }
        }

        AZ_Error("ScriptCanvas", false, "SlotID not found in node");
        return AZ::Failure(AZStd::string("SlotID not found in Node"));
    }

    Data::Type Node::GetUnderlyingSlotDataType(const SlotId& slotId) const
    {
        // Return the slot's base data type, which is used to determine which types of variables or connectors can be hooked to the slot.

        auto slotIter = m_slotIdIteratorCache.find(slotId);
        if (slotIter != m_slotIdIteratorCache.end() && slotIter->second.HasDatum())
        {
            return slotIter->second.GetDatum()->GetType();
        }

        return Data::Type::Invalid();
    }


    Data::Type Node::GetSlotDataType(const SlotId& slotId) const
    {
        // Return the slot's current data type, which could be a subtype of the slot's defined data type, based on
        // whatever variable is currently hooked into the slot.

        AZ_PROFILE_SCOPE(ScriptCanvas, "ScriptCanvas::Node::GetSlotDataType");

        const auto* slot = GetSlot(slotId);

        if (slot && slot->HasDisplayType())
        {
            return slot->GetDisplayType();
        }

        const Datum* datum = FindDatum(slotId);
        return datum ? datum->GetType() : Data::Type::Invalid();
    }

    VariableId Node::GetSlotVariableId(const SlotId& slotId) const
    {
        AZ_PROFILE_SCOPE(ScriptCanvas, "ScriptCanvas::Node::GetSlotVariableId");

        Slot* slot = GetSlot(slotId);

        if (slot && slot->IsVariableReference())
        {
            return slot->GetVariableReference();
        }

        return VariableId();
    }

    void Node::SetSlotVariableId(const SlotId& slotId, const VariableId& variableId)
    {
        AZ_PROFILE_SCOPE(ScriptCanvas, "ScriptCanvas::Node::SetSlotVariableId");

        Slot* slot = GetSlot(slotId);

        if (slot)
        {
            if (slot->ConvertToReference())
            {
                slot->SetVariableReference(variableId);
            }
            else
            {
                AZ_Error("ScriptCanvas", slot->CanConvertToReference(), "Could not convert Slot into a reference. Aborting SetVariableId attempt.");
            }            
        }
    }

    void Node::ClearSlotVariableId(const SlotId& slotId)
    {
        AZ_PROFILE_SCOPE(ScriptCanvas, "ScriptCanvas::Node::ResetSlotVariableId");

        SetSlotVariableId(slotId, VariableId());
    }

    bool Node::IsOnPureDataThread(const SlotId& slotId) const
    {
        const auto slot = GetSlot(slotId);

        if (slot && slot->GetDescriptor() == SlotDescriptors::DataIn())
        {
            const auto& nodes = GetConnectedNodes(*slot);
            AZStd::unordered_set<ID> path;
            path.insert(GetEntityId());

            for (auto& node : nodes)
            {
                if (node.first->IsOnPureDataThreadHelper(path))
                {
                    return true;
                }
            }
        }

        return false;
    }

    AZ::Outcome<void, AZStd::string> Node::IsValidTypeForSlot(const SlotId& slotId, const Data::Type& dataType) const
    {
        Slot* slot = GetSlot(slotId);

        if (!slot)
        {
            return AZ::Failure(AZStd::string("Failed to find slot with specified Id"));
        }

        if (!slot->IsDynamicSlot())
        {
            return slot->IsTypeMatchFor(dataType);
        }

        AZ::Crc32 dynamicGroup = slot->GetDynamicGroup();

        if (dynamicGroup != AZ::Crc32())
        {
            return IsValidTypeForGroup(dynamicGroup, dataType);
        }
        else
        {
            ExploredDynamicGroupCache cache;
            return IsValidTypeForSlotInternal(slot, dataType, cache);
        }
    }

    AZ::Outcome<void, AZStd::string> Node::IsValidTypeForGroup(const AZ::Crc32& dynamicGroup, const Data::Type& dataType) const
    {
        ExploredDynamicGroupCache cache;
        return IsValidTypeForGroupInternal(dynamicGroup, dataType, cache);
    }

    void Node::SignalBatchedConnectionManipulationBegin()
    {
        if (!m_queueDisplayUpdates)
        {
            m_queuedDisplayUpdates.clear();
            m_queueDisplayUpdates = true;
        }
    }

    void Node::SignalBatchedConnectionManipulationEnd()
    {
        if (m_queueDisplayUpdates)
        {
            m_queueDisplayUpdates = false;

            for (const auto& updatePair : m_queuedDisplayUpdates)
            {
                SetDisplayType(updatePair.first, updatePair.second);
            }            
        }
    }

    void Node::AddNodeDisabledFlag(NodeDisabledFlag disabledFlag)
    {
        if (!HasNodeDisabledFlag(disabledFlag))
        {
            bool oldState = IsNodeEnabled();
            int newDisabledFlag = static_cast<int>(m_disabledFlag) | static_cast<int>(disabledFlag);
            m_disabledFlag = static_cast<NodeDisabledFlag>(newDisabledFlag);

            if (oldState != IsNodeEnabled())
            {
                OnNodeStateChanged();
            }
        }
    }

    void Node::RemoveNodeDisabledFlag(NodeDisabledFlag disabledFlag)
    {
        if (HasNodeDisabledFlag(disabledFlag))
        {
            bool oldState = IsNodeEnabled();
            int newDisabledFlag = static_cast<int>(m_disabledFlag) & ~static_cast<int>(disabledFlag);
            m_disabledFlag = static_cast<NodeDisabledFlag>(newDisabledFlag);

            if (oldState != IsNodeEnabled())
            {
                OnNodeStateChanged();
            }
        }
    }

    bool Node::IsNodeEnabled() const
    {
        return static_cast<int>(m_disabledFlag) == static_cast<int>(NodeDisabledFlag::None);
    }

    bool Node::HasNodeDisabledFlag(NodeDisabledFlag disabledFlag) const
    {
        return (static_cast<int>(m_disabledFlag) & static_cast<int>(disabledFlag)) == static_cast<int>(disabledFlag);
    }

    NodeDisabledFlag Node::GetNodeDisabledFlag() const
    {
        return m_disabledFlag;
    }

    void Node::SetNodeDisabledFlag(NodeDisabledFlag disabledFlag)
    {
        bool oldState = IsNodeEnabled();
        m_disabledFlag = disabledFlag;
        if (oldState != IsNodeEnabled())
        {
            OnNodeStateChanged();
        }
    }

    bool Node::IsOnPureDataThreadHelper(AZStd::unordered_set<ID>& path) const
    {
        if (path.find(GetEntityId()) != path.end())
        {
            return false;
        }

        path.insert(GetEntityId());

        if (IsEventHandler())
        {   // data could have been routed back is the input to an event handler with a return value
            return false;
        }
        else if (IsPureData())
        {
            return true;
        }
        else
        {
            const auto& nodes = FindConnectedNodesByDescriptor(SlotDescriptors::DataIn());

            for (auto& node : nodes)
            {
                if (node->IsOnPureDataThreadHelper(path))
                {
                    return true;
                }
            }
        }

        return false;
    }

    void Node::ModifyUnderlyingSlotDatum(const SlotId& slotId, ModifiableDatumView& datumView)
    {
        auto slotIter = m_slotIdIteratorCache.find(slotId);

        if (slotIter != m_slotIdIteratorCache.end())
        {
            
            if (slotIter->second.HasDatum())
            {
                datumView.ConfigureView((*slotIter->second.GetDatum()));
            }
        }
    }

    bool Node::HasSlots() const
    {
        return !m_slots.empty();
    }

    SlotId Node::GetSlotId(AZStd::string_view slotName) const
    {
        auto slotNameIter = m_slotNameMap.find(slotName);
        return slotNameIter != m_slotNameMap.end() ? slotNameIter->second->GetId() : SlotId{};
    }

    AZStd::vector<const Slot*> Node::GetAllSlotsByDescriptor(const SlotDescriptor& slotDescriptor, bool allowLatentSlots) const
    {
        AZ_PROFILE_SCOPE(ScriptCanvas, "ScriptCanvas::Node::GetSlotsByType");

        AZStd::vector<const Slot*> slots;

        for (const auto& slot : m_slots)
        {
            if (slot.GetDescriptor() == slotDescriptor
                && (allowLatentSlots || !slot.IsLatent()))
            {
                slots.emplace_back(&slot);
            }
        }

        return slots;
    }

    AZStd::vector<Endpoint> Node::GetAllEndpointsByDescriptor(const SlotDescriptor& slotDescriptor, bool allowLatentSlots) const
    {
        AZ_PROFILE_SCOPE(ScriptCanvas, "ScriptCanvas::Node::GetEndpointsByType");

        AZStd::vector<Endpoint> endpoints;

        for (auto& slot : m_slots)
        {
            if (slot.GetDescriptor() == slotDescriptor
                && (allowLatentSlots || !slot.IsLatent()))
            {
                AZStd::vector<Endpoint> connectedEndpoints = m_graphRequestBus->GetConnectedEndpoints(Endpoint{ GetEntityId(), slot.GetId() });
                endpoints.insert(endpoints.end(), connectedEndpoints.begin(), connectedEndpoints.end());
            }
        }

        return endpoints;
    }

    AZStd::vector<SlotId> Node::GetSlotIds(AZStd::string_view slotName) const
    {
        AZ_PROFILE_SCOPE(ScriptCanvas, "ScriptCanvas::Node::GetSlotIds");

        auto nameSlotRange = m_slotNameMap.equal_range(slotName);
        AZStd::vector<SlotId> result;
        for (auto nameSlotIt = nameSlotRange.first; nameSlotIt != nameSlotRange.second; ++nameSlotIt)
        {
            result.push_back(nameSlotIt->second->GetId());
        }
        return result;
    }

    Slot* Node::GetSlot(const SlotId& slotId) const
    {
        AZ_PROFILE_SCOPE(ScriptCanvas, "ScriptCanvas::Node::GetSlot");

        if (slotId.IsValid())
        {
            auto slotIter = m_slotIdIteratorCache.find(slotId);

            if (slotIter != m_slotIdIteratorCache.end())
            {
                return &(*slotIter->second.m_slotIterator);
            }

            AZ_Warning("Script Canvas", m_removingSlot == slotId, "SlotId %s is not a part of Node %s", slotId.ToString().c_str(), GetNodeName().c_str());
        }

        return nullptr;
    }

    Slot* Node::GetSlotByName(AZStd::string_view slotName) const
    {
        auto slotNameIter = m_slotNameMap.find(slotName);
        return slotNameIter != m_slotNameMap.end() ? &(*slotNameIter->second) : nullptr;
    }

    Slot* Node::GetSlotByTransientId(TransientSlotIdentifier transientSlotId) const
    {
        auto equalRange = m_slotNameMap.equal_range(transientSlotId.m_name);

        if (equalRange.first == equalRange.second)
        {
            const Slot* slot = GetSlotByIndex(transientSlotId.m_index);
            return GetSlot(slot->GetId());
        }
        else
        {
            return GetSlotByName(transientSlotId.m_name);
        }
    }

    const Slot* Node::GetSlotByNameAndType(AZStd::string_view slotName, CombinedSlotType slotType) const
    {
        auto slotNameRange = m_slotNameMap.equal_range(slotName);
        for (auto slotNameIter = slotNameRange.first; slotNameIter != slotNameRange.second; ++slotNameIter)
        {
            if (slotNameIter->second->GetType() == slotType)
            {
                return &(*slotNameIter->second);
            }
        }
        
        return nullptr;
    }

    size_t Node::GetSlotIndex(const SlotId& slotId) const
    {
        size_t retVal = 0;
        auto slotIter = m_slots.begin();

        while (slotIter != m_slots.end())
        {
            if (slotIter->GetId() == slotId)
            {
                break;
            }

            retVal++;
            slotIter++;
        }

        if (slotIter == m_slots.end())
        {
            retVal = std::numeric_limits<size_t>::max();
        }

        return retVal;
    }

    const Slot* Node::GetSlotByIndex(size_t index) const
    {
        return index < m_slots.size() ? &*AZStd::next(m_slots.begin(), index) : nullptr;
    }

    AZStd::vector<const Slot*> Node::GetAllSlots() const
    {
        AZ_PROFILE_SCOPE(ScriptCanvas, "ScriptCanvas::Node::GetAllSlots");

        const SlotList& slots = GetSlots();

        AZStd::vector<const Slot*> retVal;
        retVal.reserve(slots.size());

        for (const auto& slot : slots)
        {
            retVal.push_back(&slot);
        }

        return retVal;
    }

    AZStd::vector<Slot*> Node::ModAllSlots()
    {
        AZ_PROFILE_SCOPE(ScriptCanvas, "ScriptCanvas::Node::ModAllSlots");

        SlotList& slots = GetSlots();

        AZStd::vector<Slot*> retVal;
        retVal.reserve(slots.size());

        for (auto& slot : slots)
        {
            retVal.push_back(&slot);
        }

        return retVal;
    }

    bool Node::SlotExists(AZStd::string_view name, const SlotDescriptor& slotDescriptor) const
    {
        return FindSlotIdForDescriptor(name, slotDescriptor).IsValid();
    }

    SlotId Node::AddSlot(const SlotConfiguration& slotConfiguration, bool isNewSlot)
    {
        return InsertSlot(-1, slotConfiguration, isNewSlot);
    }

    SlotId Node::InsertSlot(AZ::s64 index, const SlotConfiguration& slotConfig, bool isNewSlot)
    {
        SlotIterator addSlotIter = m_slots.end();
        auto insertSlotOutcome = FindOrInsertSlot(index, slotConfig, addSlotIter);

        if (insertSlotOutcome)
        {
            // Signal out that a slot was recreated so that local updates can occur before any innate signals are fired.
            if (!isNewSlot)
            {
                EndpointNotificationBus::Event(Endpoint(GetEntityId(), addSlotIter->GetId()), &EndpointNotifications::OnSlotRecreated);
            }

            if (slotConfig.GetSlotDescriptor().IsData())
            {
                if (slotConfig.GetSlotDescriptor().IsInput())
                {
                    Datum storageDatum;

                    if (auto dataConfiguration = azrtti_cast<const DataSlotConfiguration*>(&slotConfig))
                    {
                        storageDatum.ReconfigureDatumTo(dataConfiguration->GetDatum());
                    }

                    storageDatum.SetLabel(slotConfig.m_name);
                    storageDatum.SetNotificationsTarget(GetEntityId());
                    
                    DatumIterator insertionPoint = m_slotDatums.end();

                    if (index >= 0)                    
                    {
                        insertionPoint = m_slotDatums.begin();

                        for (SlotIterator slotIter = m_slots.begin(); slotIter != insertSlotOutcome.GetValue()->second.m_slotIterator; ++slotIter)
                        {
                            if (slotIter->GetDescriptor() == SlotDescriptors::DataIn())
                            {
                                ++insertionPoint;

                                if (insertionPoint == m_slotDatums.end())
                                {
                                    break;
                                }
                            }
                        }
                    }

                    DatumIterator datumIterator = m_slotDatums.emplace(insertionPoint, AZStd::move(storageDatum));

                    insertSlotOutcome.GetValue()->second.SetDatumIterator(datumIterator);

                    AZ_Assert(FindDatum(slotConfig.m_slotId) != nullptr, "Failed to register datum to slot.");
                }
                else
                {
                    Slot& slot = (*addSlotIter);

                    if (auto dataConfiguration = azrtti_cast<const DataSlotConfiguration*>(&slotConfig))
                    {
                        Data::Type variableType = dataConfiguration->GetDatum().GetType();

                        if (variableType.IsValid())
                        {
                            slot.SetDisplayType(variableType);
                        }
                    }
                }

                ProcessDataSlot((*addSlotIter));

                if (auto dynamicConfiguration = azrtti_cast<const DynamicDataSlotConfiguration*>(&slotConfig))
                {
                    if (dynamicConfiguration->m_displayType.IsValid())
                    {
                        addSlotIter->SetDisplayType(dynamicConfiguration->m_displayType);
                    }
                }
            }

            if (isNewSlot)
            {
                NodeNotificationsBus::Event((GetEntity() != nullptr) ? GetEntityId() : AZ::EntityId(), &NodeNotifications::OnSlotAdded, addSlotIter->GetId());
            }

            EndpointNotificationBus::MultiHandler::BusConnect(addSlotIter->GetEndpoint());
        }

        return addSlotIter != m_slots.end() ? addSlotIter->GetId() : SlotId{};
    }

    bool Node::RemoveSlot(const SlotId& slotId, bool signalRemoval, [[maybe_unused]] const bool warnOnMissingSlots)
    {
        // If we are already removing the slot, early out with false since something else is doing the deleting.
        if (m_removingSlots.count(slotId) != 0)
        {
            return false;
        }

        auto slotIter = m_slotIdIteratorCache.find(slotId);

        if (slotIter != m_slotIdIteratorCache.end())
        {
            IteratorCache iteratorCache = slotIter->second;

            /// Disconnect connected endpoints
            if (signalRemoval && !m_isUpdatingNode)
            {
                // We want to avoid recursive calls into ourselves here(happens in the case of dynamically added slots)
                m_removingSlots.insert(slotId);
                RemoveConnectionsForSlot(slotId);
                m_removingSlots.erase(slotId);
            }

            if (iteratorCache.HasDatum())
            {
                m_slotDatums.erase(iteratorCache.GetDatumIter());
                iteratorCache.ClearIterator();
            }            

            m_slotIdIteratorCache.erase(slotIter);

            auto slotNameIter = AZStd::find_if(m_slotNameMap.begin(), m_slotNameMap.end(), [iteratorCache](const AZStd::pair<AZStd::string, SlotIterator>& nameSlotPair)
            {
                return nameSlotPair.second == iteratorCache.m_slotIterator;
            });

            if (slotNameIter != m_slotNameMap.end())
            {
                m_slotNameMap.erase(slotNameIter);
            }

            if (iteratorCache.m_slotIterator->IsDynamicSlot() && iteratorCache.m_slotIterator->GetDynamicGroup() != AZ::Crc32())
            {
                AZ::Crc32 dynamicGroup = iteratorCache.m_slotIterator->GetDynamicGroup();

                auto range = m_dynamicGroups.equal_range(dynamicGroup);

                for (auto iter = range.first; iter != range.second; ++iter)
                {
                    if (iter->second == slotId)
                    {
                        m_dynamicGroups.erase(iter);
                        break;
                    }
                }
            }

            m_slots.erase(iteratorCache.m_slotIterator);

            if (signalRemoval && !m_isUpdatingNode)
            {
                SanityCheckDynamicDisplay();
                SignalSlotRemoved(slotId);
            }

            return true;
        }

        AZ_Warning("Script Canvas", !warnOnMissingSlots, "Cannot remove slot that does not exist! %s", slotId.m_id.ToString<AZStd::string>().c_str());
        return false;
    }

    void Node::RemoveConnectionsForSlot(const SlotId& slotId, bool slotDeleted)
    {
        Graph* graph = GetGraph();

        if (graph)
        {
            if (slotDeleted)
            {
                m_removingSlot = slotId;
            }

            Endpoint baseEndpoint{ GetEntityId(), slotId };

            for (const auto& connectedEndpoint : graph->GetConnectedEndpoints(baseEndpoint))
            {
                graph->DisconnectByEndpoint(baseEndpoint, connectedEndpoint);
            }

            if (slotDeleted)
            {
                m_removingSlot = ScriptCanvas::SlotId();
            }
        }
    }

    void Node::SignalSlotRemoved(const SlotId& slotId)
    {
        OnSlotRemoved(slotId);
        NodeNotificationsBus::Event((GetEntity() != nullptr) ? GetEntityId() : AZ::EntityId(), &NodeNotifications::OnSlotRemoved, slotId);
    }

    void Node::InitializeVariableReference(Slot& slot, const AZStd::unordered_set<VariableId>& excludedVariableIds)
    {
        AZ_Assert(slot.IsVariableReference(), "Initializing a non-variable referenced slot.");

        ScriptCanvas::Data::Type dataType = slot.GetDataType();
        if (dataType.IsValid())
        {
            ScriptCanvas::GraphVariable* variable = nullptr;
            ScriptCanvas::GraphVariableManagerRequestBus::EventResult(variable, GetOwningScriptCanvasId(), &ScriptCanvas::GraphVariableManagerRequests::FindFirstVariableWithType, dataType, excludedVariableIds);

            if (variable)
            {
                slot.SetVariableReference(variable->GetVariableId());
            }
            else
            {
                slot.ClearVariableReference();
            }
        }
    }


    void Node::RenameSlot(Slot& slot, const AZStd::string& slotName)
    {
        const AZStd::string& oldName = slot.GetName();

        auto nameRange = m_slotNameMap.equal_range(oldName);

        for (auto nameIter = nameRange.first; nameIter != nameRange.second; ++nameIter)
        {
            if (nameIter->second->GetId() == slot.GetId())
            {
                auto slotIter = nameIter->second;
                m_slotNameMap.erase(nameIter);
                m_slotNameMap.emplace(slotName, slotIter);

                slot.Rename(slotName);
                break;
            }
        }
    }

    void Node::OnResetDatumToDefaultValue(ModifiableDatumView& datumView)
    {
        datumView.SetToDefaultValueOfType();
    }

    AZ::Outcome<Node::SlotIdIteratorMap::iterator, AZStd::string> Node::FindOrInsertSlot(AZ::s64 insertIndex, const SlotConfiguration& slotConfiguration, SlotIterator& iterOut)
    {
        if (slotConfiguration.m_name.empty())
        {
            return AZ::Failure(AZStd::string("attempting to add a slot with no name"));
        }

        if (!slotConfiguration.GetSlotDescriptor().IsValid())
        {
            return AZ::Failure(AZStd::string("Trying to add a slot with an Invalid Slot Descriptor"));
        }

        auto findResult = m_slotNameMap.equal_range(slotConfiguration.m_name);
        for (auto slotNameIter = findResult.first; slotNameIter != findResult.second; ++slotNameIter)
        {
            if (slotConfiguration.m_addUniqueSlotByNameAndType &&
                slotNameIter->second->GetDescriptor() == slotConfiguration.GetSlotDescriptor())
            {
                iterOut = slotNameIter->second;
                return AZ::Failure(AZStd::string::format("Slot with name %s already exist", slotConfiguration.m_name.data()));
            }
        }

        SlotIterator insertIter = (insertIndex < 0 || insertIndex >= azlossy_cast<AZ::s64>(m_slots.size())) ? m_slots.end() : AZStd::next(m_slots.begin(), insertIndex);
        iterOut = m_slots.emplace(insertIter, slotConfiguration);

        IteratorCache iteratorCache;

        iteratorCache.m_slotIterator = iterOut;

        auto insertResult = m_slotIdIteratorCache.emplace(iterOut->GetId(), iteratorCache);
        m_slotNameMap.emplace(iterOut->GetName(), iterOut);

        iterOut->SetNode(this);

        return AZ::Success(insertResult.first);
    }

    void Node::SetOwningScriptCanvasId(ScriptCanvasId scriptCanvasId)
    {
        m_scriptCanvasId = scriptCanvasId;
        m_graphRequestBus = GraphRequestBus::FindFirstHandler(m_scriptCanvasId);
        OnGraphSet();
    }

    void Node::SetGraphEntityId(AZ::EntityId graphEntityId)
    {
        const Data::Type entityIdType = Data::Type::EntityID();
        for (Datum& datum : m_slotDatums)
        {
            if (datum.GetType() == entityIdType)
            {
                Data::EntityIDType* entityId = datum.ModAs<Data::EntityIDType>();

                if ((*entityId) == ScriptCanvas::GraphOwnerId)
                {
                    (*entityId) = graphEntityId;
                }
            }
        }
    }

    bool Node::CanAcceptNullInput([[maybe_unused]] const Slot& executionSlot, [[maybe_unused]] const Slot& inputSlot) const
    {
        return true;
    }

    void Node::CollectVariableReferences(AZStd::unordered_set< ScriptCanvas::VariableId >& variableIds) const
    {
        for (const Slot& slot : m_slots)
        {
            if (!slot.IsVariableReference())
            {
                continue;
            }

            const ScriptCanvas::VariableId& variableId = slot.GetVariableReference();

            variableIds.insert(variableId);
        }
    }

    bool Node::ContainsReferencesToVariables(const AZStd::unordered_set< ScriptCanvas::VariableId >& variableIds) const
    {
        for (const Slot& slot : m_slots)
        {
            if (!slot.IsVariableReference())
            {
                continue;
            }

            const ScriptCanvas::VariableId& variableId = slot.GetVariableReference();

            if (variableIds.count(variableId) > 0)
            {
                return true;
            }
        }

        return false;
    }

    bool Node::RemoveVariableReferences(const AZStd::unordered_set< ScriptCanvas::VariableId >& variableIds)
    {
        for (Slot& slot : m_slots)
        {
            if (!slot.IsVariableReference())
            {
                continue;
            }

            const ScriptCanvas::VariableId& variableId = slot.GetVariableReference();

            if (variableIds.count(variableId) > 0)
            {
                slot.ClearVariableReference();
                NodeNotificationsBus::Event(GetEntityId(), &NodeNotifications::OnSlotInputChanged, slot.GetId());
            }
        }

        return true;
    }    

    Graph* Node::GetGraph() const
    {
        Graph* graph = nullptr;
        GraphRequestBus::EventResult(graph, GetOwningScriptCanvasId(), &GraphRequests::GetGraph);
        return graph;
    }

    AZ::EntityId Node::GetGraphEntityId() const
    {
        return m_graphRequestBus->GetRuntimeEntityId();
    }

    NodePtrConstList Node::FindConnectedNodesByDescriptor(const SlotDescriptor& slotDescriptor, bool followLatentConnections) const
    {
        AZ_PROFILE_SCOPE(ScriptCanvas, "ScriptCanvas::Node::GetConnectedNodesByType");

        NodePtrConstList connectedNodes;

        for (const auto& endpoint : GetAllEndpointsByDescriptor(slotDescriptor, followLatentConnections))
        {
            Node* connectedNode = m_graphRequestBus->FindNode(endpoint.GetNodeId());
            
            if (connectedNode)
            {
                connectedNodes.emplace_back(connectedNode);
            }
        }

        return connectedNodes;
    }

    AZStd::vector<AZStd::pair<const Node*, SlotId>> Node::FindConnectedNodesAndSlotsByDescriptor(const SlotDescriptor& slotDescriptor, bool followLatentConnections) const
    {
        AZ_PROFILE_SCOPE(ScriptCanvas, "ScriptCanvas::Node::GetConnectedNodesAndSlotsByType");

        AZStd::vector<AZStd::pair<const Node*, SlotId>> connectedNodes;

        for (const auto& endpoint : GetAllEndpointsByDescriptor(slotDescriptor, followLatentConnections))
        {
            Node* connectedNode = m_graphRequestBus->FindNode(endpoint.GetNodeId());
            
            if (connectedNode)
            {
                connectedNodes.emplace_back(AZStd::make_pair(connectedNode, endpoint.GetSlotId()));
            }
        }

        return connectedNodes;
    }

    AZ::Data::AssetId Node::GetGraphAssetId() const
    {
        return m_graphRequestBus->GetAssetId();
    }

    AZStd::string Node::GetGraphAssetName() const
    {
        AZ::Data::AssetId assetId = GetGraphAssetId();
        AZ::Data::AssetInfo assetInfo;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, assetId);
        return assetInfo.m_relativePath;
    }

    GraphIdentifier Node::GetGraphIdentifier() const
    {
        return m_graphRequestBus->GetGraphIdentifier();
    }

    bool Node::IsSanityCheckRequired() const
    {
        bool result = m_nodeReconfigured;
        for (const auto& slot : m_slots)
        {
            result |= slot.IsSanityCheckRequired();
        }
        return result;
    }

    void Node::SanityCheckDynamicDisplay()
    {
        // Don't sanity check displays while reconfiguring
        if (m_nodeReconfiguring)
        {
            return;
        }

        ExploredDynamicGroupCache exploredCache;
        SanityCheckDynamicDisplay(exploredCache);

        OnSanityCheckDisplay();

        // Some weird cases with the overloaded slots and data values.
        // Going to just signal out all the slots have changed their display type
        // in order to keep the visuals in sync.
        for (Slot& slot : m_slots)
        {
            if (slot.IsData())
            {
                NodeNotificationsBus::Event(GetEntityId(), &NodeNotifications::OnSlotDisplayTypeChanged, slot.GetId(), slot.GetDataType());
            }
        }
    }

    void Node::SanityCheckDynamicDisplay(ExploredDynamicGroupCache& exploredGroupCache)
    {
        // Don't sanity check displays while reconfiguring
        if (m_nodeReconfiguring)
        {
            return;
        }

        auto exploredIter = exploredGroupCache.find(GetEntityId());

        bool hasSet = exploredIter != exploredGroupCache.end();

        for (Slot& slot : m_slots)
        {
            if (!slot.IsUserAdded() && slot.IsDynamicSlot())
            {
                AZ::Crc32 group = slot.GetDynamicGroup();

                if (group == AZ::Crc32())
                {
                    auto connectedDisplayType = FindConnectedConcreteDisplayType(slot, exploredGroupCache);

                    if (!connectedDisplayType.IsValid())
                    {
                        slot.ClearDisplayType();
                    }
                    else if (slot.GetDataType() != connectedDisplayType)
                    {
                        slot.ClearDisplayType();
                        slot.SetDisplayType(connectedDisplayType);
                    }
                }
                else
                {
                    if (hasSet)
                    {
                        if (exploredIter->second.count(group) > 0)
                        {
                            continue;
                        }
                    }

                    // If we have a Display type sanity check our concrete connections.
                    auto connectedDisplayType = FindConcreteDisplayType(group);
                    if (slot.HasDisplayType())
                    {
                        if (!connectedDisplayType.IsValid())
                        {
                            ClearDisplayType(group);
                        }
                        else if (slot.GetDataType() != connectedDisplayType)
                        {
                            const bool forceDisplaySet = true;

                            ClearDisplayType(group);
                            SetDisplayType(group, connectedDisplayType, forceDisplaySet);
                        }
                    }
                    else
                    {
                        if (connectedDisplayType.IsValid())
                        {
                            const bool forceDisplaySet = true;
                            SetDisplayType(group, connectedDisplayType, forceDisplaySet);
                        }
                        else
                        {
                            ClearDisplayType(group);
                        }
                    }
                }
            }
        }
    }

    bool Node::ConvertSlotToReference(const SlotId& slotId, bool isNewSlot)
    {
        Slot* slot = GetSlot(slotId);

        if (slot && slot->ConvertToReference(isNewSlot))
        {
            InitializeVariableReference((*slot), {});
            return true;
        }

        return false;
    }

    bool Node::ConvertSlotToValue(const SlotId& slotId)
    {
        Slot* slot = GetSlot(slotId);
        return slot ? slot->ConvertToValue() : false;
    }

    void Node::OnDatumEdited(const Datum* datum)
    {
        AZ_PROFILE_SCOPE(ScriptCanvas, "ScriptCanvas::Node::OnDatumChanged");

        SlotId slotId;

        AZStd::find_if(m_slotIdIteratorCache.begin(), m_slotIdIteratorCache.end(), [&slotId, datum](const AZStd::pair<SlotId, IteratorCache>& cachePair)
        {
            if (cachePair.second.HasDatum())
            {
                if (cachePair.second.GetDatum() == datum)
                {
                    slotId = cachePair.first;
                }
            }

            return slotId.IsValid();
        });

        if (slotId.IsValid())
        {
            NodeNotificationsBus::Event((GetEntity() != nullptr) ? GetEntityId() : AZ::EntityId(), &NodeNotifications::OnSlotInputChanged, slotId);
        }        
    }

    void Node::OnDeserialize()
    {
        RebuildInternalState();
    }

    void Node::OnEndpointConnected(const Endpoint& endpoint)
    {
        const SlotId& currentSlotId = EndpointNotificationBus::GetCurrentBusId()->GetSlotId();

        Slot* slot = GetSlot(currentSlotId);

        if (slot && slot->IsDynamicSlot())
        {
            if (slot->HasDisplayType() && !m_queueDisplayUpdates)
            {
                return;
            }

            auto node = m_graphRequestBus->FindNode(endpoint.GetNodeId());

            if (node)
            {
                Slot* otherSlot = node->GetSlot(endpoint.GetSlotId());

                if (otherSlot && (!otherSlot->IsDynamicSlot() || otherSlot->HasDisplayType()))
                {
                    Data::Type displayType = otherSlot->GetDataType();

                    AZ::Crc32 dynamicGroup = slot->GetDynamicGroup();

                    if (dynamicGroup != AZ::Crc32())
                    {
                        SetDisplayType(dynamicGroup, displayType);
                    }
                    else
                    {
                        slot->SetDisplayType(displayType);
                    }
                }
            }
        }
    }

    void Node::OnEndpointDisconnected([[maybe_unused]] const Endpoint& endpoint)
    {
        const SlotId& currentSlotId = EndpointNotificationBus::GetCurrentBusId()->GetSlotId();

        Slot* slot = GetSlot(currentSlotId);

        if (slot && slot->IsDynamicSlot() && !slot->IsUserAdded())
        {
            AZ::Crc32 dynamicGroup = slot->GetDynamicGroup();

            if (dynamicGroup != AZ::Crc32())
            {
                if (!FindConcreteDisplayType(dynamicGroup).IsValid())
                {
                    ClearDisplayType(dynamicGroup);
                }
            }
            else
            {
                ExploredDynamicGroupCache exploredCache;
                if (!FindConnectedConcreteDisplayType((*slot), exploredCache).IsValid())
                {
                    slot->ClearDisplayType();
                }
            }
        }
    }

    void Node::FindModifiableDatumViewByIndex(size_t index, ModifiableDatumView& controller)
    {
        int foundIndex = 0;
        for (Slot& slot : m_slots)
        {
            // These are the requirements for having localized datum storage.
            if (slot.IsData() && slot.IsInput())
            {
                if (foundIndex == index)
                {
                    FindModifiableDatumView(slot.GetId(), controller);
                    return;
                }

                ++foundIndex;
            }
        }

        return;
    }

    const Datum* Node::FindDatumByIndex(size_t index) const
    {
        int foundIndex = 0;
        for (const Slot& slot : m_slots)
        {
            // These are the requirements for having localized datum storage.
            if (slot.IsData() && slot.IsInput())
            {
                if (foundIndex == index)
                {
                    return FindDatum(slot.GetId());
                }

                ++foundIndex;
            }
        }

        return nullptr;
    }

    const Datum* Node::FindDatum(const SlotId& slotId) const
    {
        const Datum* datum = nullptr;

        auto slotIter = m_slotIdIteratorCache.find(slotId);

        if (slotIter != m_slotIdIteratorCache.end())
        {
            Slot* slot = &(*slotIter->second.m_slotIterator);

            if (slot->IsVariableReference())
            {
                GraphVariable* variable = slot->GetVariable();

                if (variable)
                {
                    datum = variable->GetDatum();
                }
                else
                {
                    SCRIPTCANVAS_REPORT_ERROR((*this), "Node (%s) is attempting to execute using an invalid Variable Reference", GetNodeName().c_str());
                }
            }

            if (datum == nullptr && slotIter->second.HasDatum())
            {
                datum = slotIter->second.GetDatum();
            }
        }

        return datum;
    }

    bool Node::FindModifiableDatumView(const SlotId& slotId, ModifiableDatumView& datumView)
    {
        auto slotIter = m_slotIdIteratorCache.find(slotId);

        if (slotIter != m_slotIdIteratorCache.end())
        {
            Slot* slot = &(*slotIter->second.m_slotIterator);

            if (slot->IsVariableReference())
            {
                GraphVariable* variable = slot->GetVariable();

                if (variable)
                {
                    datumView.ConfigureView((*variable));
                    return true;
                }
                else
                {
                    SCRIPTCANVAS_REPORT_ERROR((*this), "Node (%s) is attempting to execute using an invalid Variable Reference", GetNodeName().c_str());
                }
            }
            else
            {
                if (slotIter->second.HasDatum())
                {
                    datumView.ConfigureView((*slotIter->second.GetDatum()));
                    return true;
                }
            }
        }

        return false;
    }

    SlotId Node::FindSlotIdForDescriptor(AZStd::string_view slotName, const SlotDescriptor& descriptor) const
    {
        AZ_PROFILE_SCOPE(ScriptCanvas, "ScriptCanvas::Node::FindSlotIdForDescriptor");

        auto slotNameRange = m_slotNameMap.equal_range(slotName);
        auto nameSlotIt = AZStd::find_if(slotNameRange.first, slotNameRange.second, [descriptor](const AZStd::pair<AZStd::string, SlotIterator>& nameSlotPair)
        {
            return descriptor == nameSlotPair.second->GetDescriptor();
        });

        return nameSlotIt != slotNameRange.second ? nameSlotIt->second->GetId() : SlotId{};
    }

    int Node::FindSlotIndex(const SlotId& slotId) const
    {
        AZ_PROFILE_SCOPE(ScriptCanvas, "ScriptCanvas::Node::FindSlotIndex");

        auto slotIdIter = m_slotIdIteratorCache.find(slotId);

        if (slotIdIter != m_slotIdIteratorCache.end())
    {
            int slotIndex = aznumeric_cast<int>(AZStd::distance(m_slots.begin(), SlotList::const_iterator(slotIdIter->second.m_slotIterator)));
            return slotIndex;
        }

        return -1;
    }

    bool Node::IsConnected(const Slot& slot) const
    {
        AZ_PROFILE_SCOPE(ScriptCanvas, "ScriptCanvas::Node::IsConnected");
        return slot.IsVariableReference() || m_graphRequestBus->IsEndpointConnected(slot.GetEndpoint());
    }

    bool Node::IsConnected(const SlotId& slotId) const
    {
        const Slot* slot = GetSlot(slotId);
        return slot && IsConnected((*slot));
    }
    
    bool Node::HasConnectionForDescriptor(const SlotDescriptor& slotDescriptor) const
    {        
        for (const auto& slot : m_slots)
        {
            if (slot.GetDescriptor() == slotDescriptor)
            {
                if (IsConnected(slot.GetId()))
                {
                    return true;
                }
            }
        }

        return false;
    }
    
    bool Node::IsPureData() const
    {
        for (const auto& slot : m_slots)
        {
            if (slot.GetDescriptor().IsExecution())
            {
                return false;
            }
        }

        return true;
    }

    bool Node::IsActivated() const
    {
        return m_graphRequestBus;
    }
    
    EndpointsResolved Node::GetConnectedNodes(const Slot& slot) const
    {
        AZ_PROFILE_SCOPE(ScriptCanvas, "ScriptCanvas::Node::GetConnectedNodes");

        EndpointsResolved connectedNodes;

        auto endpointIters = m_graphRequestBus->GetConnectedEndpointIterators(Endpoint{ GetEntityId(), slot.GetId() });

        for (auto endpointIter = endpointIters.first; endpointIter != endpointIters.second; ++endpointIter)
        {
            const Endpoint& endpoint = endpointIter->second;
            auto node = m_graphRequestBus->FindNode(endpoint.GetNodeId());

            if (node == nullptr)
            {                
                AZStd::string assetName = m_graphRequestBus->GetAssetName();
                [[maybe_unused]] AZ::EntityId assetNodeId = m_graphRequestBus->FindAssetNodeIdByRuntimeNodeId(endpoint.GetNodeId());
                AZ_Warning("Script Canvas", false, "Unable to find node with id (id: %s) in the graph '%s'. Most likely the node was serialized with a type that is no longer reflected",
                    assetNodeId.ToString().data(), assetName.data());

                continue;
            }
            else if (!node->IsNodeEnabled())
            {
                continue;
            }

            auto endpointSlot = node->GetSlot(endpoint.GetSlotId());
            if (!endpointSlot)
            {
                AZStd::string assetName = m_graphRequestBus->GetAssetName();
                [[maybe_unused]] AZ::EntityId assetNodeId = m_graphRequestBus->FindAssetNodeIdByRuntimeNodeId(endpoint.GetNodeId());
                AZ_Warning("Script Canvas", false, "Endpoint was missing slot. id (id: %s) in the graph '%s'.",
                    assetNodeId.ToString().data(), assetName.data());

                continue;
            }

            connectedNodes.emplace_back(node, endpointSlot);
        }

        return connectedNodes;
    }

    AZStd::vector<AZStd::pair<Node*, const SlotId>> Node::ModConnectedNodes(const Slot& slot) const
    {
        AZ_PROFILE_SCOPE(ScriptCanvas, "ScriptCanvas::Node::ModConnectedNodes");
        AZStd::vector<AZStd::pair<Node*, const SlotId>> connectedNodes;
        ModConnectedNodes(slot, connectedNodes);
        return connectedNodes;
    }

    void Node::ModConnectedNodes(const Slot& slot, AZStd::vector<AZStd::pair<Node*, const SlotId>>& connectedNodes) const
    {
        auto endpointIters = m_graphRequestBus->GetConnectedEndpointIterators(Endpoint{ GetEntityId(), slot.GetId() });

        for (auto endpointIter = endpointIters.first; endpointIter != endpointIters.second; ++endpointIter)
        {
            const Endpoint& endpoint = endpointIter->second;

            auto node = m_graphRequestBus->FindNode(endpoint.GetNodeId());

            if (node == nullptr)
            {
                AZStd::string assetName = m_graphRequestBus->GetAssetName();
                [[maybe_unused]] AZ::EntityId assetNodeId = m_graphRequestBus->FindAssetNodeIdByRuntimeNodeId(endpoint.GetNodeId());

                AZ_Error("Script Canvas", false, "Unable to find node with id (id: %s) in the graph '%s'. Most likely the node was serialized with a type that is no longer reflected",
                    assetNodeId.ToString().data(), assetName.data());
                continue;
            }

            connectedNodes.emplace_back(node, endpoint.GetSlotId());
        }
    }

    bool Node::HasConnectedNodes(const Slot& slot) const
    {
        return m_graphRequestBus->IsEndpointConnected(Endpoint{ GetEntityId(), slot.GetId() });
    }

    void Node::ForEachConnectedNode(const Slot& slot, AZStd::function<void(Node&, const SlotId&)> callable) const
    {
        auto connectedNodes = ModConnectedNodes(slot);
        for (auto& nodeSlotPair : connectedNodes)
        {
            if (nodeSlotPair.first)
            {
                callable(*nodeSlotPair.first, nodeSlotPair.second);
            }
        }
    }

    AZStd::string Node::GetNodeTypeName() const
    {
        return RTTI_GetTypeName();
    }

    AZStd::string Node::GetDebugName() const
    {
        if (GetEntityId().IsValid())
        {
            return AZStd::string::format("%s (%s)", GetEntity()->GetName().c_str(), TYPEINFO_Name());
        }
        return TYPEINFO_Name();
    }

    AZStd::string Node::GetNodeName() const
    {
        if (m_name.empty())
        {
            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

            if (serializeContext)
            {
                const AZ::SerializeContext::ClassData* classData = serializeContext->FindClassData(RTTI_GetType());

                if (classData)
                {
                    if (classData->m_editData)
                    {
                        return classData->m_editData->m_name;
                    }
                    else
                    {
                        return classData->m_name;
                    }
                }
            }
            return "<unknown>";
        }
        return m_name;
    }

    const AZStd::string& Node::GetNodeToolTip() const
    {
        return m_toolTip;
    }

    const AZStd::string& Node::GetNodeStyle() const
    {
        return m_nodeStyle;
    }

    void Node::SetNodeName(const AZStd::string& name)
    {
        m_name = name;
    }

    void Node::SetNodeToolTip(const AZStd::string& toolTip)
    {
        m_toolTip = toolTip;
    }

    void Node::SetNodeStyle(const AZStd::string& nodeStyle)
    {
        m_nodeStyle = nodeStyle;
    }

    void Node::SetNodeLexicalId(const AZ::Crc32& nodeLexicalId)
    {
        m_nodeLexicalId = nodeLexicalId;
    }

    bool Node::IsEntryPoint() const
    {
        return false;
    }

    bool Node::RequiresDynamicSlotOrdering() const
    {
        return false;
    }


    AZStd::vector<const Slot*> Node::GetEventSlots() const
    {
        AZStd::vector<const Slot*> slots;
        auto eventSlotIds = GetEventSlotIds();
        for (auto slotId : eventSlotIds)
        {
            slots.push_back(GetSlot(slotId));
        }

        return slots;
    }

    const Slot* Node::GetEBusConnectSlot() const
    {
        return nullptr;
    }

    const Slot* Node::GetEBusConnectAddressSlot() const
    {
        return nullptr;
    }

    const Slot* Node::GetEBusDisconnectSlot() const
    {
        return nullptr;
    }

    AZStd::string Node::GetEBusName() const
    {
        return {};
    }

    AZStd::optional<size_t> Node::GetEventIndex([[maybe_unused]] AZStd::string eventName) const
    {
        return AZStd::nullopt;
    }

    AZStd::vector<SlotId> Node::GetEventSlotIds() const
    {
        return {};
    }

    AZStd::vector<SlotId> Node::GetNonEventSlotIds() const
    {
        return {};
    }

    AZStd::vector<const Slot*> Node::GetOnVariableHandlingDataSlots() const
    {
        return {};
    }

    AZStd::vector<const Slot*> Node::GetOnVariableHandlingExecutionSlots() const
    {
        return {};
    }

    bool Node::IsAutoConnected() const
    {
        return {};
    }

    bool Node::IsEBusAddressed() const
    {
        return {};
    }

    const Datum* Node::GetHandlerStartAddress() const
    {
        return {};
    }

    bool Node::ConvertsInputToStrings() const
    {
        return false;
    }

    AZ::Outcome<DependencyReport, void> Node::GetDependencies() const
    {
        return AZ::Failure();
    }
    
    AZ::Outcome<AZStd::string, void> Node::GetFunctionCallName(const Slot* /*slot*/) const
    {
        return AZ::Failure();
    }

    EventType Node::GetFunctionEventType(const Slot*) const
    {
        return EventType::Count;
    }

    AZ::Outcome<Grammar::LexicalScope, void> Node::GetFunctionCallLexicalScope(const Slot* /*slot*/) const
    {
        return AZ::Failure();
    }

    ConstSlotsOutcome Node::GetSlotsInExecutionThreadByTypeImpl(const Slot& executionSlot, CombinedSlotType targetSlotType, const Slot* executionChildSlot) const
    {
        if (auto map = GetSlotExecutionMap())
        {
            return GetSlotsFromMap(*map, executionSlot, targetSlotType, executionChildSlot);
        }
        else if (executionSlot.GetType() == CombinedSlotType::ExecutionIn && targetSlotType == CombinedSlotType::ExecutionOut)
        {
            ExecutionNameMap nameMap = GetExecutionNameMap();

            if (!nameMap.empty())
            {
                AZStd::vector<const Slot*> slots;
                auto range = nameMap.equal_range(executionSlot.GetName());
                for (auto iter = range.first; iter != range.second; ++iter)
                {
                    if (auto slot = GetSlot(GetSlotId(iter->second)))
                    {
                        slots.push_back(slot);
                    }
                    else
                    {
                        AZ_Error("ScriptCanvas", false, "No slot by name %s in node %s", iter->second.data(), GetDebugName().data());
                        return AZ::Failure(AZStd::string::format("No slot by name %s in node %s", iter->second.data(), GetDebugName().data()));
                    }
                }

                return AZ::Success(slots);
            }
        }

        return AZ::Failure(AZStd::string("override Node::GetSlotsInExecutionThreadByTypeImpl to do things subvert the normal slot map assumptions"));
    }

    ConstSlotsOutcome Node::GetSlotsFromMap(const SlotExecution::Map& map, const Slot& executionSlot, CombinedSlotType targetSlotType, const Slot* /*executionChildSlot*/) const
    {
        // so far ACM, needs to map:
        //      In -> Out, Data In, Data Out
        //      Out -> Data Out (for the internal out case)
        //      Latent -> Data Out
        switch (executionSlot.GetType())
        {
        case CombinedSlotType::ExecutionIn:
            switch (targetSlotType)
            {
            case CombinedSlotType::DataIn:
                return GetDataInSlotsByExecutionIn(map, executionSlot);

            case CombinedSlotType::ExecutionOut:
                return GetExecutionOutSlotsByExecutionIn(map, executionSlot);

            case CombinedSlotType::DataOut:
                return GetDataOutSlotsByExecutionIn(map, executionSlot);

            default:
                break;
            }
            break;

        case CombinedSlotType::LatentOut:
        case CombinedSlotType::ExecutionOut:
            switch (targetSlotType)
            {
            case CombinedSlotType::DataIn:
                return GetDataInSlotsByExecutionOut(map, executionSlot);

            case CombinedSlotType::DataOut:
                return GetDataOutSlotsByExecutionOut(map, executionSlot);
            default:
                break;
            }
            break;
        }

        return AZ::Failure(AZStd::string("no such mapping supported, yet"));
    }

    ConstSlotsOutcome Node::GetDataInSlotsByExecutionIn(const SlotExecution::Map& map, const Slot& executionInSlot) const
    {
        const auto in = map.GetIn(executionInSlot.GetId());

        if (!in)
        {
            return AZ::Failure(AZStd::string::format("%s-%s The execution in slot referenced by the slot id in the map was not found. SlotId: %s", GetNodeName().data(), executionInSlot.GetName().data(), executionInSlot.GetId().ToString().data()));
        }

        return GetSlots(SlotExecution::ToInputSlotIds(in->inputs));
    }

    ConstSlotsOutcome Node::GetDataOutSlotsByExecutionIn(const SlotExecution::Map& map, const Slot& executionInSlot) const
    {
        const auto outs = map.GetOuts(executionInSlot.GetId());

        if (!outs || outs->size() > 1)
        {
            return AZ::Failure(AZStd::string::format("%s-%s This function assumes the in slots have 0 or 1 outs", GetNodeName().data(), executionInSlot.GetName().data()));
        }

        AZStd::vector<const Slot*> outputSlots;

        for (const auto& out : (*outs))
        {
            for (const auto& outputSlotId : ToOutputSlotIds(out.outputs))
            {
                if (auto slot = GetSlot(outputSlotId))
                {
                    outputSlots.push_back(slot);
                }
                else
                {
                    return AZ::Failure(AZStd::string::format("%s-%s The slot referenced by the slot id in the map was not found. SlotId: %s", GetNodeName().data(), executionInSlot.GetName().data(), outputSlotId.ToString().data()));
                }
            }
        }

        return AZ::Success(outputSlots);
    }

    ConstSlotsOutcome Node::GetDataOutSlotsByExecutionOut(const SlotExecution::Map& map, const Slot& executionOutSlot) const
    {
        if (executionOutSlot.IsLatent())
        {
            if (const auto output = map.GetLatentOutput(executionOutSlot.GetId()))
            {
                return GetSlots(SlotExecution::ToOutputSlotIds(*output));
            }
            else
            {
                return AZ::Failure(AZStd::string::format("%s-%s not found in the execution map", GetNodeName().data(), executionOutSlot.GetName().data()));
            }
        }
        else
        {
            if (const auto output = map.GetOutput(executionOutSlot.GetId()))
            {
                return GetSlots(SlotExecution::ToOutputSlotIds(*output));
            }
            else
            {
                return AZ::Failure(AZStd::string::format("%s-%s not found in the execution map", GetNodeName().data(), executionOutSlot.GetName().data()));
            }
        }
    }

    ConstSlotsOutcome Node::GetExecutionOutSlotsByExecutionIn(const SlotExecution::Map& map, const Slot& executionInSlot) const
    {
        const auto outs = map.GetOuts(executionInSlot.GetId());
        if (!outs)
        {
            return AZ::Failure(AZStd::string::format("%s-%s no outs declared", GetNodeName().data(), executionInSlot.GetName().data()));
        }

        AZStd::vector<const Slot*> outSlots;

        for (const auto& out : (*outs))
        {
            if (auto slot = GetSlot(out.slotId))
            {
                outSlots.push_back(slot);
            }
            else
            {
                return AZ::Failure(AZStd::string::format("%s-%s The slot referenced by the slot id in the map was not found. SlotId: %s", GetNodeName().data(), executionInSlot.GetName().data(), out.slotId.ToString().data()));
            }
        }

        return AZ::Success(outSlots);
    }

    ConstSlotsOutcome Node::GetDataInSlotsByExecutionOut(const SlotExecution::Map& map, const Slot& executionOutSlot) const
    {
        if (auto returns = map.GetReturnValuesByOut(executionOutSlot.GetId()))
        {
            return GetSlots(SlotExecution::ToInputSlotIds(*returns));
        }

        return AZ::Failure(AZStd::string::format("%s-%s The slot referenced by the slot id in the map was not found. SlotId: %s", GetNodeName().data(), executionOutSlot.GetName().data(), executionOutSlot.GetId().ToString().data()));
    }

    const Slot* Node::GetCorrespondingExecutionSlot(const Slot* slot) const
    {
        if (!slot)
        {
            return nullptr;
        }

        if (slot->IsExecution())
        {
            return slot;
        }

        const ScriptCanvas::Slot* executionSlot = nullptr;

        const ScriptCanvas::SlotExecution::Map* map = GetSlotExecutionMap();

        if (map)
        {
            if (slot->IsInput())
            {
                // Find the corresponding execution input for the source
                if (const ScriptCanvas::SlotExecution::In* sourceIn = map->FindInFromInputSlot(slot->GetId()))
                {
                    const ScriptCanvas::SlotId inSlotId = sourceIn->slotId;
                    executionSlot = GetSlot(inSlotId);
                }
            }
            else
            {
                // Find the corresponding execution output for the source
                if (const ScriptCanvas::SlotExecution::Out* sourceOut = map->FindOutFromOutputSlot(slot->GetId()))
                {
                    const ScriptCanvas::SlotId outSlotId = sourceOut->slotId;
                    executionSlot = GetSlot(outSlotId);
                }
            }
        }
        else
        {
            // If the node doesn't have a slot execution map, we will need to just use whatever execution slot is there
            AZStd::vector<const ScriptCanvas::Slot*> executionSlots = slot->IsInput()
                ? GetAllSlotsByDescriptor(ScriptCanvas::SlotDescriptors::ExecutionIn())
                : GetAllSlotsByDescriptor(ScriptCanvas::SlotDescriptors::ExecutionOut());

            if (!executionSlots.empty())
            {
                executionSlot = executionSlots[0];
            }
        }

        return executionSlot;
    }

    AZStd::vector<const Slot*> Node::GetCorrespondingDataSlots(const Slot* slot) const
    {
        AZStd::vector<const Slot*> dataSlots;

        if (!slot)
        {
            return dataSlots;
        }

        const ScriptCanvas::SlotExecution::Map* map = GetSlotExecutionMap();

        if (map)
        {
            if (slot->IsExecution())
            {
                ConstSlotsOutcome slotOutcome = slot->IsInput()
                    ? GetSlotsFromMap(*map, *slot, CombinedSlotType::DataIn, nullptr)
                    : GetSlotsFromMap(*map, *slot, CombinedSlotType::DataOut, nullptr);

                if (slotOutcome.IsSuccess())
                {
                    dataSlots = slotOutcome.GetValue();
                }
            }
            else if (slot->IsData())
            {
                return GetCorrespondingDataSlots(GetCorrespondingExecutionSlot(slot));
            }
        }
        else
        {
            // If the node doens't have a slot execution map, we will need to just get whatever data slots are there
            dataSlots = slot->IsInput()
                ? GetAllSlotsByDescriptor(ScriptCanvas::SlotDescriptors::DataIn())
                : GetAllSlotsByDescriptor(ScriptCanvas::SlotDescriptors::DataOut());
        }

        return dataSlots;
    }

    const Slot* Node::GetIfBranchFalseOutSlot() const
    {
        return GetSlotByName("False");
    }

    const Slot* Node::GetIfBranchTrueOutSlot() const
    {
        return GetSlotByName("True");
    }

    size_t Node::GetOutIndex(const Slot& slot) const
    {
        size_t index = 0;
        auto slotId = slot.GetId();

        if (auto map = GetSlotExecutionMap())
        {
            auto& ins = map->GetIns();
            for (auto& in : ins)
            {
                for (auto& out : in.outs)
                {
                    // only count branches
                    if (in.outs.size() > 1)
                    {
                        if (out.slotId == slotId)
                        {
                            return index;
                        }

                        ++index;
                    }
                }
            }

            for (auto& latent : map->GetLatents())
            {
                if (latent.slotId == slotId)
                {
                    return index;
                }

                ++index;
            }
        }

        return std::numeric_limits<size_t>::max();
    }

    AZ::Outcome<AZStd::string> Node::GetInternalOutKey(const Slot& slot) const
    {
        if (auto map = GetSlotExecutionMap())
        {
            return GetInternalOutKey(*map, slot);
        }
        else
        {
            return AZ::Failure();
        }
    }

    AZ::Outcome<AZStd::string> Node::GetLatentOutKey(const Slot& slot) const
    {
        if (auto map = GetSlotExecutionMap())
        {
            return GetLatentOutKey(*map, slot);
        }
        else
        {
            return AZ::Failure();
        }
    }

    AZ::Outcome<AZStd::string> Node::GetInternalOutKey(const SlotExecution::Map& map, const Slot& slot) const
    {
        if (auto out = map.GetOut(slot.GetId()))
        {
            return AZ::Success(out->name);
        }
        else
        {
            return AZ::Failure();
        }
    }

    AZ::Outcome<AZStd::string> Node::GetLatentOutKey(const SlotExecution::Map& map, const Slot& slot) const
    {
        if (auto out = map.GetLatent(slot.GetId()))
        {
            return AZ::Success(out->name);
        }
        else
        {
            return AZ::Failure();
        }
    }

    AZ::Outcome<Grammar::FunctionPrototype> Node::GetSimpleSignature() const
    {
        auto executionSlots = GetSlotsByType(CombinedSlotType::ExecutionIn);
        if (executionSlots.size() != 1)
        {
            return AZ::Failure();
        }

        auto inSlot = executionSlots[0];
        if (!inSlot)
        {
            return AZ::Failure();
        }

        return GetSignatureOfEexcutionIn(*inSlot);
    }

    AZ::Outcome<Grammar::FunctionPrototype> Node::GetSignatureOfEexcutionIn(const Slot& executionInSlot) const
    {
        auto slotsOutcome = GetSlotsInExecutionThreadByType(executionInSlot, CombinedSlotType::ExecutionOut);
        if (!slotsOutcome.IsSuccess())
        {
            return AZ::Failure();
        }

        if (slotsOutcome.GetValue().size() != 1)
        {
            return AZ::Failure();
        }

        auto inputSlotsOutcome = GetSlotsInExecutionThreadByType(executionInSlot, CombinedSlotType::DataIn);
        if (!inputSlotsOutcome.IsSuccess())
        {
            return AZ::Failure();
        }

        auto outputSlotsOutcome = GetSlotsInExecutionThreadByType(executionInSlot, CombinedSlotType::DataOut);
        if (!outputSlotsOutcome.IsSuccess())
        {
            return AZ::Failure();
        }

        auto& inputSlots = inputSlotsOutcome.GetValue();
        auto& outputSlots = outputSlotsOutcome.GetValue();

        Grammar::FunctionPrototype signature;

        for (auto inputSlot : inputSlots)
        {
            signature.m_inputs.push_back(AZStd::make_shared<Grammar::Variable>(Datum(inputSlot->GetDataType(), Datum::eOriginality::Original)));
        }

        for (auto outputSlot : outputSlots)
        {
            signature.m_outputs.push_back(AZStd::make_shared<Grammar::Variable>(Datum(outputSlot->GetDataType(), Datum::eOriginality::Original)));
        }

        return AZ::Success(signature);
    }

    ConstSlotsOutcome Node::GetSlotsInExecutionThreadByType(const Slot& executionSlot, CombinedSlotType targetSlotType, const Slot* executionChildSlot) const
    {
        auto targetSlotsOutcome = GetSlotsInExecutionThreadByTypeImpl(executionSlot, targetSlotType, executionChildSlot);
        if (targetSlotsOutcome.IsSuccess())
        {
            return targetSlotsOutcome;
        }

        int executionInCount = 0;

        AZStd::vector<const Slot*> outSlots = GetSlotsByType(targetSlotType);

        for (auto& slot : outSlots)
        {
            if (slot->GetType() == CombinedSlotType::ExecutionIn)
            {
                ++executionInCount;
            }
        }

        if (targetSlotType == CombinedSlotType::DataOut
        && executionSlot.GetType() == CombinedSlotType::ExecutionIn
        && executionInCount > 1)
        {
            if (!executionChildSlot || executionChildSlot->GetType() != CombinedSlotType::ExecutionOut)
            {
                return AZ::Failure(AZStd::string("Data out by ExecutionIn must have child out slot"));
            }
        }

        if (executionInCount <= 1 && (!IsExecutionOut(targetSlotType) || outSlots.size() <= 1))
        {
            return AZ::Success(outSlots);
        }

        if (executionInCount > 1)
        {
            return AZ::Failure(AZStd::string("Define an execution map to to process a node with more than one input."));
        }

        if (IsExecutionOut(targetSlotType) && outSlots.size() > 1)
        {
            return AZ::Failure(AZStd::string("Define an execution map to associate Out slots with multiple In slots within a single node."));
        }

        return AZ::Success(outSlots);
    }

    AZStd::vector<const Slot*> Node::GetSlotsByType(CombinedSlotType slotType) const
    {
        AZ_PROFILE_SCOPE(ScriptCanvas, "ScriptCanvas::Node::GetSlotsByType");

        AZStd::vector<const Slot*> slots;

        for (const auto& slot : m_slots)
        {
            if (slot.GetType() == slotType)
            {
                slots.emplace_back(&slot);
            }
        }

        return slots;
    }

    PropertyFields Node::GetPropertyFields() const
    {
        return {};
    }

    Grammar::MultipleFunctionCallFromSingleSlotInfo Node::GetMultipleFunctionCallFromSingleSlotInfo([[maybe_unused]] const Slot& slot) const
    {
        return {};
    }

    VariableId Node::GetVariableIdRead(const Slot*) const
    {
        return {};
    }

    VariableId Node::GetVariableIdWritten(const Slot*) const
    {
        return {};
    }

    const Slot* Node::GetVariableInputSlot() const
    {
        return nullptr;
    }

    const Slot* Node::GetVariableOutputSlot() const
    {
        return nullptr;
    }
}
