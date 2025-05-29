/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "NodeableNodeOverloaded.h"

#include <AzCore/RTTI/AttributeReader.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Serialization/IdUtils.h>
#include <ScriptCanvas/Core/Attributes.h>
#include <ScriptCanvas/Libraries/Core/Method.h>

namespace NodeableNodeOverloadedCpp
{
    using namespace ScriptCanvas;

    size_t AdjustForHiddenNodeableThisPointer(const OverloadConfiguration& overloadConfiguration, size_t inputIndex)
    {
        if (overloadConfiguration.m_overloads.empty())
        {
            return inputIndex;
        }

        auto testMethod = overloadConfiguration.m_overloads[0].first;
        if (testMethod->GetNumArguments() > 0)
        {
            if (auto argument = testMethod->GetArgument(0))
            {
                if (argument->m_traits & AZ::BehaviorParameter::TR_THIS_PTR)
                {
                    return inputIndex + 1;
                }
            }
        }

        return inputIndex;
    }
}

namespace ScriptCanvas
{
    namespace Nodes
    {
        ////////////////////////////////////////////
        // NodeableMethodOverloadContractInterface
        ////////////////////////////////////////////

        NodeableNodeOverloaded::NodeableMethodOverloadContractInterface::NodeableMethodOverloadContractInterface(NodeableNodeOverloaded& nodeableOverloaded, size_t methodIndex)
            : m_nodeableOverloaded(nodeableOverloaded)
            , m_methodIndex(methodIndex)
        {

        }

        AZ::Outcome<void, AZStd::string> NodeableNodeOverloaded::NodeableMethodOverloadContractInterface::IsValidInputType(size_t index, const Data::Type& dataType)
        {
            return m_nodeableOverloaded.IsValidInputType(m_methodIndex, index, dataType);
        }

        const DataTypeSet& NodeableNodeOverloaded::NodeableMethodOverloadContractInterface::FindPossibleInputTypes(size_t index) const
        {
            return m_nodeableOverloaded.FindPossibleInputTypes(m_methodIndex, index);
        }

        AZ::Outcome<void, AZStd::string> NodeableNodeOverloaded::NodeableMethodOverloadContractInterface::IsValidOutputType(size_t index, const Data::Type& dataType)
        {
            return m_nodeableOverloaded.IsValidOutputType(m_methodIndex, index, dataType);
        }

        const DataTypeSet& NodeableNodeOverloaded::NodeableMethodOverloadContractInterface::FindPossibleOutputTypes(size_t index) const
        {
            return m_nodeableOverloaded.FindPossibleOutputTypes(m_methodIndex, index);
        }

        ///////////////////////////
        // NodeableNodeOverloaded
        ///////////////////////////

        bool NodeableNodeOverloaded::VersionConverter(AZ::SerializeContext&, AZ::SerializeContext::DataElementNode& classElement)
        {
            if (classElement.GetVersion() <= DataDrivingOverloads)
            {
                classElement.RemoveElementByName(AZ_CRC_CE("activeIndex"));
                classElement.RemoveElementByName(AZ_CRC_CE("activePrototype"));
                classElement.RemoveElementByName(AZ_CRC_CE("overloadSelectionTriggerSlotIds"));
            }

            return true;
        }

        void NodeableNodeOverloaded::Reflect(AZ::ReflectContext* reflectContext)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
            {
                serializeContext->Class<NodeableNodeOverloaded, NodeableNode>()
                    ->Version(Version::Current, &NodeableNodeOverloaded::VersionConverter)
                    ;

                if (auto editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<NodeableNodeOverloaded>("NodeableNodeOverloaded", "NodeableNode")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;
                }
            }
        }

        NodeableNodeOverloaded::~NodeableNodeOverloaded()
        {
            for (NodeableMethodOverloadContractInterface* contractInterface : m_methodOverloadContractInterface)
            {
                delete contractInterface;
            }

            m_methodOverloadContractInterface.clear();
        }

        bool NodeableNodeOverloaded::IsNodeableListEmpty() const
        {
            return m_nodeables.empty();
        }

        void NodeableNodeOverloaded::SetNodeables(AZStd::vector<AZStd::unique_ptr<Nodeable>>&& nodeables)
        {
            m_nodeables = AZStd::move(nodeables);
        }

        void NodeableNodeOverloaded::ConfigureNodeableOverloadConfigurations()
        {
            AZStd::vector<const AZ::BehaviorClass*> behaviorClasses;

            // get all the behavior context classes
            for (auto& nodeableUniquePtr : m_nodeables)
            {
                if (auto nodeableRawPtr = nodeableUniquePtr.get() ? nodeableUniquePtr.get() : GetNodeable())
                {
                    const AZ::TypeId typeId = azrtti_typeid(nodeableRawPtr);

                    if (auto behaviorClass = AZ::BehaviorContextHelper::GetClass(typeId))
                    {
                        behaviorClasses.push_back(behaviorClass);
                    }
                    else
                    {
                        AZ_Error("ScriptCanvas", false, "Nodeable %s missing behavior context reflection for TypdId %s", GetDebugName().data(), typeId.ToString<AZStd::string>().data());
                    }
                }
                else
                {
                    AZ_Error("ScriptCanvas", false, "Missing nodeable in nodeable node %s", GetDebugName().data());
                }
            }

            if (!behaviorClasses.empty())
            {
                auto referenceClass = behaviorClasses[0];

                // for each method in the first one
                for (auto nameMethodPair : referenceClass->m_methods)
                {
                    OverloadConfiguration overloadConfiguration;

                    overloadConfiguration.m_overloads.push_back(AZStd::make_pair(nameMethodPair.second, referenceClass));
                    overloadConfiguration.m_prototypes.push_back(ToSignature(*nameMethodPair.second));

                    for (size_t index(1); index < behaviorClasses.size(); ++index)
                    {
                        auto overloadClass = behaviorClasses[index];
                        if (auto method = overloadClass->FindMethodByReflectedName(nameMethodPair.first))
                        {
                            overloadConfiguration.m_overloads.push_back(AZStd::make_pair(method, overloadClass));
                            overloadConfiguration.m_prototypes.push_back(ToSignature((*method)));
                        }
                        else
                        {
                            AZ_Error("ScriptCanvas", false, "can't find method in overloaded class");
                        }
                    }

                    overloadConfiguration.m_overloadVariance = AZ::GetOverloadVariance(*nameMethodPair.second, overloadConfiguration.m_overloads, AZ::VariantOnThis::No);
                    m_methodConfigurations.emplace_back(AZStd::move(overloadConfiguration));
                }

                m_methodSelections.resize(m_methodConfigurations.size());
            }
        }

        Grammar::FunctionPrototype NodeableNodeOverloaded::GetCurrentInputPrototype(const SlotExecution::In& in) const
        {
            Grammar::FunctionPrototype signature;
            // add an (invalid) input for the nodeable this pointer, which always has to be presentJust 
            signature.m_inputs.emplace_back(aznew Grammar::Variable(Datum()));

            for (auto& inputSlotId : SlotExecution::ToInputSlotIds(in.inputs))
            {
                if (auto* slot = GetSlot(inputSlotId))
                {
                    signature.m_inputs.emplace_back(aznew Grammar::Variable(Datum(slot->GetDataType(), Datum::eOriginality::Original)));
                }
                else
                {
                    AZ_Error("ScriptCanvas", false, "missing input slot in NodeableNodeOverloaded: %s", GetNodeName().data());
                }
            }

            return signature;
        }

        AZStd::vector<AZStd::unique_ptr<Nodeable>> NodeableNodeOverloaded::GetInitializationNodeables() const
        {
            return AZStd::vector<AZStd::unique_ptr<Nodeable>>();
        }

        void NodeableNodeOverloaded::OnInit()
        {
            NodeableNode::OnInit();

            SetNodeables(GetInitializationNodeables());
            ConfigureNodeableOverloadConfigurations();

            ConfigureContracts();
        }

        void NodeableNodeOverloaded::OnConfigured()
        {
            ConfigureContracts();
        }

        void NodeableNodeOverloaded::OnPostActivate()
        {
            NodeableNode::OnPostActivate();

            CheckHasSingleOuts();
            RefreshAvailableNodeables();
        }

        void NodeableNodeOverloaded::OnSlotDisplayTypeChanged(const SlotId&, const ScriptCanvas::Data::Type&)
        {
            if (!m_updatingDisplayTypes && !m_updatingDynamicGroups)
            {
                RefreshAvailableNodeables();
                UpdateSlotDisplay();
            }
            else
            {
                m_slotTypeChange = true;
            }
        }

        void NodeableNodeOverloaded::OnDynamicGroupTypeChangeBegin(const AZ::Crc32&)
        {
            if (!m_updatingDisplayTypes)
            {
                m_updatingDynamicGroups = true;
            }
        }

        void NodeableNodeOverloaded::OnDynamicGroupDisplayTypeChanged(const AZ::Crc32&, const Data::Type&)
        {
            if (!m_updatingDisplayTypes && m_updatingDynamicGroups)
            {
                m_updatingDisplayTypes = true;

                RefreshAvailableNodeables();
                UpdateSlotDisplay();

                m_updatingDisplayTypes = false;

                m_updatingDynamicGroups = false;
                m_slotTypeChange = false;
            }
        }

        Data::Type NodeableNodeOverloaded::FindFixedDataTypeForSlot(const Slot& slot) const
        {
            if (!m_isCheckingForDataTypes)
            {
                // go over the map, and update all the slots based on the indices...
                auto& slotExecutionMap = *GetSlotExecutionMap();

                auto& ins = slotExecutionMap.GetIns();
                for (size_t methodIndex = 0; methodIndex < ins.size(); ++methodIndex)
                {
                    const OverloadConfiguration& overloadConfiguration = m_methodConfigurations[methodIndex];
                    const OverloadSelection& overloadSelection = m_methodSelections[methodIndex];

                    auto& in = ins[methodIndex];
                    size_t startIndex = NodeableNodeOverloadedCpp::AdjustForHiddenNodeableThisPointer(overloadConfiguration, 0);
                    const auto& inputSlotIds = SlotExecution::ToInputSlotIds(in.inputs);

                    for (size_t inputIndex = 0; inputIndex < inputSlotIds.size(); ++inputIndex)
                    {
                        if (inputSlotIds[inputIndex] == slot.GetId())
                        {
                            auto inputTypeIter = overloadSelection.m_inputDataTypes.find(startIndex + inputIndex);

                            if (inputTypeIter->second.size() == 1)
                            {
                                return (*inputTypeIter->second.begin());
                            }
                        }
                    }

                    if (in.outs.size() == 1)
                    {
                        const auto& outputSlotIds = SlotExecution::ToOutputSlotIds(in.outs[0].outputs);

                        for (size_t outputIndex = 0; outputIndex < outputSlotIds.size(); ++outputIndex)
                        {
                            if (outputSlotIds[outputIndex] == slot.GetId())
                            {
                                auto outputTypeIter = overloadSelection.m_outputDataTypes.find(outputIndex);

                                if (outputTypeIter->second.size() == 1)
                                {
                                    return (*outputTypeIter->second.begin());
                                }
                            }
                        }
                    }
                }
            }

            return Data::Type::Invalid();
        }

        void NodeableNodeOverloaded::OnEndpointConnected(const Endpoint& targetEndpoint)
        {
            m_updatingDisplayTypes = true;
            NodeableNode::OnEndpointConnected(targetEndpoint);
            m_updatingDisplayTypes = false;

            if (m_slotTypeChange)
            {
                RefreshAvailableNodeables();
                UpdateSlotDisplay();
            }

            m_slotTypeChange = false;
        }

        void NodeableNodeOverloaded::OnEndpointDisconnected(const Endpoint& targetEndpoint)
        {
            m_updatingDisplayTypes = true;
            NodeableNode::OnEndpointDisconnected(targetEndpoint);
            m_updatingDisplayTypes = false;
            
            RefreshAvailableNodeables();
            UpdateSlotDisplay();

            m_slotTypeChange = false;
        }

        void NodeableNodeOverloaded::CheckHasSingleOuts()
        {
#if defined(AZ_ENABLE_TRACING)
            auto& slotExecutionMap = *GetSlotExecutionMap();

            auto& ins = slotExecutionMap.GetIns();
            for (size_t methodIndex = 0; methodIndex < ins.size(); ++methodIndex)
            {
                auto& in = ins[methodIndex];

                // TODO: Append debugging information
                AZ_Error("ScriptCanvas", in.outs.size() <= 1, "Unable to resolve Overloaded Nodeable with multiple outs for a single method.");
            }
#endif
        }

        void NodeableNodeOverloaded::UpdateSlotDisplay()
        {
            m_updatingDisplayTypes = true;

            // go over the map, and update all the slots based on the indices...
            auto& slotExecutionMap = *GetSlotExecutionMap();

            auto& ins = slotExecutionMap.GetIns();
            for (size_t methodIndex = 0; methodIndex < ins.size(); ++methodIndex)
            {
                const OverloadConfiguration& overloadConfiguration = m_methodConfigurations[methodIndex];
                const OverloadSelection& overloadSelection = m_methodSelections[methodIndex];

                auto& in = ins[methodIndex];
                size_t startIndex = NodeableNodeOverloadedCpp::AdjustForHiddenNodeableThisPointer(overloadConfiguration, 0);
                const auto& inputSlotIds = SlotExecution::ToInputSlotIds(in.inputs);

                for (size_t inputIndex = 0; inputIndex < inputSlotIds.size(); ++inputIndex)
                {
                    auto inputTypeIter = overloadSelection.m_inputDataTypes.find(startIndex + inputIndex);

                    if (inputTypeIter->second.size() == 1)
                    {
                        SetDisplayType(inputSlotIds[inputIndex], (*inputTypeIter->second.begin()));
                    }
                    else
                    {
                        ClearDisplayType(inputSlotIds[inputIndex]);
                    }
                }

                if (in.outs.size() == 1)
                {
                    const auto& outputSlotIds = SlotExecution::ToOutputSlotIds(in.outs[0].outputs);

                    for (size_t outputIndex = 0; outputIndex < outputSlotIds.size(); ++outputIndex)
                    {
                        auto outputTypeIter = overloadSelection.m_outputDataTypes.find(outputIndex);

                        if (outputTypeIter->second.size() == 1)
                        {
                            SetDisplayType(outputSlotIds[outputIndex], (*outputTypeIter->second.begin()));
                        }
                        else
                        {
                            ClearDisplayType(outputSlotIds[outputIndex]);
                        }
                    }
                }
            }

            m_updatingDisplayTypes = false;
        }

        void NodeableNodeOverloaded::ConfigureContracts()
        {
            const SlotExecution::Map* slotExecutionMap = GetSlotExecutionMap();
            const auto& executionIns = slotExecutionMap->GetIns();

            m_availableNodeables.clear();

            // Iterate over each method, scrape for the input/output data.
            // Find all of the matching nodeables for each set.
            // Then take the intersection of all the sets to get the set of activle nodeable indexes that represents all of the current known data
            for (size_t methodIndex = 0; methodIndex < executionIns.size(); ++methodIndex)
            {
                NodeableMethodOverloadContractInterface* contractInterface = aznew NodeableMethodOverloadContractInterface((*this), methodIndex);
                m_methodOverloadContractInterface.emplace_back(contractInterface);


                const auto& currentIn = executionIns[methodIndex];

                auto inputSlotIds = SlotExecution::ToInputSlotIds(currentIn.inputs);

                for (size_t inputIndex = 0; inputIndex < currentIn.inputs.size(); ++inputIndex)
                {
                    SlotId slotId = inputSlotIds[inputIndex];
                    Slot* slot = GetSlot(slotId);

                    if (slot)
                    {
                        OverloadContract* overloadContract = slot->FindContract<OverloadContract>();

                        if (overloadContract)
                        {
                            overloadContract->ConfigureContract(contractInterface, inputIndex, ConnectionType::Input);
                        }
                    }
                }

                if (!currentIn.outs.empty())
                {
                    const SlotExecution::Outputs& outputs = currentIn.outs[0].outputs;
                    auto outputSlotIds = SlotExecution::ToOutputSlotIds(outputs);

                    for (size_t outputIndex = 0; outputIndex < outputSlotIds.size(); ++outputIndex)
                    {
                        SlotId slotId = outputSlotIds[outputIndex];

                        Slot* slot = GetSlot(slotId);

                        if (slot)
                        {
                            OverloadContract* overloadContract = slot->FindContract<OverloadContract>();

                            if (overloadContract)
                            {
                                overloadContract->ConfigureContract(contractInterface, outputIndex, ConnectionType::Output);
                            }
                        }
                    }
                }
            }
        }

        void NodeableNodeOverloaded::RefreshAvailableNodeables(const bool checkForConnections)
        {
            const SlotExecution::Map* slotExecutionMap = GetSlotExecutionMap();
            const auto& executionIns = slotExecutionMap->GetIns();

            auto previousNodeable = ReleaseNodeable();

            if (previousNodeable)
            {
                if (m_availableNodeables.size() == 1)
                {
                    m_nodeables[(*m_availableNodeables.begin())] = AZStd::unique_ptr<Nodeable>(previousNodeable);                    
                }
                else
                {
                    delete previousNodeable;
                }
            }

            m_availableNodeables.clear();

            // Iterate over each method, scrape for the input/output data.
            // Find all of the matching nodeables for each set.
            // Then take the intersection of all the sets to get the set of activle nodeable indexes that represents all of the current known data
            for (size_t methodIndex = 0; methodIndex < executionIns.size(); ++methodIndex)
            {
                DataIndexMapping inputTypeMapping;
                DataIndexMapping outputTypeMapping;

                FindDataIndexMappings(methodIndex, inputTypeMapping, outputTypeMapping, checkForConnections);

                const OverloadConfiguration& overloadConfiguration = m_methodConfigurations[methodIndex];

                AZStd::unordered_set< AZ::u32 > nodeableSet = overloadConfiguration.GenerateAvailableIndexes(inputTypeMapping, outputTypeMapping);

                if (methodIndex == 0)
                {
                    m_availableNodeables = AZStd::move(nodeableSet);
                }
                else
                {
                    auto availableIter = m_availableNodeables.begin();

                    while (availableIter != m_availableNodeables.end())
                    {
                        if (!nodeableSet.contains((*availableIter)))
                        {
                            availableIter = m_availableNodeables.erase(availableIter);
                        }
                        else
                        {
                            ++availableIter;
                        }
                    }

                    // We've gotten into an invalid setup. No Beuno.
                    if (m_availableNodeables.empty())
                    {
                        break;
                    }
                }
            }

            for (size_t methodIndex = 0; methodIndex < m_methodConfigurations.size(); ++methodIndex)
            {
                OverloadConfiguration& overloadConfiguration = m_methodConfigurations[methodIndex];
                OverloadSelection& overloadSelection = m_methodSelections[methodIndex];

                overloadConfiguration.PopulateOverloadSelection(overloadSelection, m_availableNodeables);
            }

            if (m_availableNodeables.size() == 1)
            {
                auto newNodeable = m_nodeables[(*m_availableNodeables.begin())].release();

                SetNodeable(AZStd::unique_ptr<Nodeable>(newNodeable));
            }
        }

        void NodeableNodeOverloaded::FindDataIndexMappings(size_t methodIndex, DataIndexMapping& inputMapping, DataIndexMapping& outputMapping, bool checkForConnections)
        {
            const SlotExecution::Map* slotExecutionMap = GetSlotExecutionMap();
            const auto& executionIns = slotExecutionMap->GetIns();

            if (methodIndex >= executionIns.size())
            {
                return;
            }

            if (m_methodConfigurations.empty())
            {
                return;
            }

            m_isCheckingForDataTypes = true;

            const OverloadConfiguration& baseConfiguration = (*m_methodConfigurations.begin());
            size_t startIndex = NodeableNodeOverloadedCpp::AdjustForHiddenNodeableThisPointer(baseConfiguration, 0);

            const auto& currentIn = executionIns[methodIndex];

            auto inputSlotIds = SlotExecution::ToInputSlotIds(currentIn.inputs);

            for (size_t inputIndex = 0; inputIndex < currentIn.inputs.size(); ++inputIndex)
            {
                SlotId slotId = inputSlotIds[inputIndex];

                Slot* slot = GetSlot(slotId);

                if (slot)
                {
                    if (!slot->IsDynamicSlot() || slot->HasDisplayType())
                    {
                        Data::Type displayType = slot->GetDisplayType();

                        if (slot->IsDynamicSlot() && checkForConnections)
                        {
                            displayType = FindConnectedConcreteDisplayType((*slot));
                        }

                        if (displayType.IsValid())
                        {
                            inputMapping[startIndex + inputIndex] = displayType;
                        }
                    }
                }
            }

            if (!currentIn.outs.empty())
            {
                const SlotExecution::Outputs& outputs = currentIn.outs[0].outputs;
                auto outputSlotIds = SlotExecution::ToOutputSlotIds(outputs);

                for (size_t outputIndex = 0; outputIndex < outputSlotIds.size(); ++outputIndex)
                {
                    SlotId slotId = outputSlotIds[outputIndex];

                    Slot* slot = GetSlot(slotId);

                    if (slot)
                    {
                        if (!slot->IsDynamicSlot() || slot->HasDisplayType())
                        {
                            Data::Type displayType = slot->GetDisplayType();

                            if (slot->IsDynamicSlot() && checkForConnections)
                            {
                                displayType = FindConnectedConcreteDisplayType((*slot));
                            }

                            if (displayType.IsValid())
                            {
                                outputMapping[outputIndex] = displayType;
                            }
                        }
                    }
                }
            }

            m_isCheckingForDataTypes = false;
        }

        AZ::Outcome<void, AZStd::string> NodeableNodeOverloaded::IsValidConfiguration(size_t methodIndex, const DataIndexMapping& inputMapping, const DataIndexMapping& outputMapping)
        {
            if (methodIndex >= m_methodConfigurations.size())
            {
                return AZ::Failure(AZStd::string("Trying to access unknown method index."));
            }

            const OverloadConfiguration& overloadConfiguration = m_methodConfigurations[methodIndex];

            // Generate the new list of available indexes for this new configuration.
            // Then we need to confirm that the other method selections would be fine with these nodeables being active.
            AZStd::unordered_set<AZ::u32> availableIndexes = overloadConfiguration.GenerateAvailableIndexes(inputMapping, outputMapping);

            // To check, take the intersections of this against all of the other selections and ensure the list isn't empty.
            for (size_t checkMethodIndex = 0; checkMethodIndex < m_methodConfigurations.size(); ++checkMethodIndex)
            {
                if (checkMethodIndex == methodIndex)
                {
                    continue;
                }

                const OverloadSelection& overloadSelection = m_methodSelections[checkMethodIndex];

                auto availableIter = availableIndexes.begin();

                while (availableIter != availableIndexes.end())
                {
                    if (overloadSelection.m_availableIndexes.count((*availableIter)) == 0)
                    {
                        availableIter = availableIndexes.erase(availableIter);
                    }
                    else
                    {
                        ++availableIter;
                    }
                }

                if (availableIndexes.empty())
                {
                    return AZ::Failure(AZStd::string("Unable to find any matchin overloads."));
                }
            }

            AZStd::unordered_set<AZ::Crc32> dynamicGroups;

            for (size_t testIndex = 0; testIndex < m_methodConfigurations.size(); ++testIndex)
            {
                auto checkResult = CheckOverloadDataTypes(availableIndexes, methodIndex);

                if (!checkResult)
                {
                    return checkResult;
                }
            }

            return AZ::Success();
        }

        AZ::Outcome<void, AZStd::string> NodeableNodeOverloaded::CheckOverloadDataTypes(const AZStd::unordered_set<AZ::u32>& availableIndexes, size_t methodIndex)
        {
            const SlotExecution::Map* slotExecutionMap = GetSlotExecutionMap();
            const auto& executionIns = slotExecutionMap->GetIns();

            if (methodIndex >= executionIns.size())
            {
                return AZ::Failure(AZStd::string("Invalid method index given to Nodeable"));;
            }

            if (methodIndex >= m_methodConfigurations.size())
            {
                return AZ::Failure(AZStd::string("Invalid method index given to Nodeable"));;                
            }

            OverloadSelection overloadSelection;
            m_methodConfigurations[methodIndex].PopulateOverloadSelection(overloadSelection, availableIndexes);

            const OverloadConfiguration& baseConfiguration = (*m_methodConfigurations.begin());
            size_t startIndex = NodeableNodeOverloadedCpp::AdjustForHiddenNodeableThisPointer(baseConfiguration, 0);

            const auto& currentIn = executionIns[methodIndex];

            auto inputSlotIds = SlotExecution::ToInputSlotIds(currentIn.inputs);

            for (size_t inputIndex = 0; inputIndex < currentIn.inputs.size(); ++inputIndex)
            {
                if (overloadSelection.m_inputDataTypes[startIndex + inputIndex].size() == 1)
                {
                    Data::Type dataType = (*overloadSelection.m_inputDataTypes[startIndex + inputIndex].begin());
                    SlotId slotId = inputSlotIds[inputIndex];

                    auto validTypeForSlot = IsValidTypeForSlot(slotId, dataType);

                    if (!validTypeForSlot)
                    {
                        return validTypeForSlot;
                    }
                }
            }

            if (!currentIn.outs.empty())
            {
                const SlotExecution::Outputs& outputs = currentIn.outs[0].outputs;
                auto outputSlotIds = SlotExecution::ToOutputSlotIds(outputs);

                for (size_t outputIndex = 0; outputIndex < outputSlotIds.size(); ++outputIndex)
                {
                    if (overloadSelection.m_outputDataTypes[outputIndex].size() == 1)
                    {
                        Data::Type dataType = (*overloadSelection.m_outputDataTypes[outputIndex].begin());
                        SlotId slotId = outputSlotIds[outputIndex];

                        auto validTypeForSlot = IsValidTypeForSlot(slotId, dataType);

                        if (!validTypeForSlot)
                        {
                            return validTypeForSlot;
                        }
                    }
                }
            }

            return AZ::Success();
        }

        AZ::Outcome<void, AZStd::string> NodeableNodeOverloaded::IsValidInputType(size_t methodIndex, size_t index, const Data::Type& dataType)
        {
            // If we are type checking we don't want to recurse in here. Just return success, since we know we triggered this so the type is valid.
            if (m_isTypeChecking)
            {
                return AZ::Success();
            }

            if (methodIndex >= m_methodConfigurations.size())
            {
                return AZ::Failure(AZStd::string("Invalid Method index given to Nodeable Node Overloaded."));
            }

            m_isTypeChecking = true;

            AZ::Outcome<void, AZStd::string> resultOutcome = AZ::Failure(AZStd::string::format("%s does not support the type %s in its current coniguration", GetNodeName().c_str(), ScriptCanvas::Data::GetName(dataType).c_str()));

            const OverloadSelection& overloadSelection = m_methodSelections[methodIndex];

            const OverloadConfiguration& overloadConfiguration = m_methodConfigurations[methodIndex];
            size_t startIndex = NodeableNodeOverloadedCpp::AdjustForHiddenNodeableThisPointer(overloadConfiguration, 0);

            auto outputDataIter = overloadSelection.m_inputDataTypes.find(startIndex + index);

            if (outputDataIter != overloadSelection.m_inputDataTypes.end())
            {
                if (outputDataIter->second.contains(dataType))
                {
                    DataIndexMapping inputMapping;
                    DataIndexMapping outputMapping;

                    // Only care about display types. Not where it gets the information for this.
                    const bool checkForConnections = false;
                    FindDataIndexMappings(methodIndex, inputMapping, outputMapping, checkForConnections);

                    inputMapping[startIndex + index] = dataType;

                    resultOutcome = IsValidConfiguration(methodIndex, inputMapping, outputMapping);
                }
            }

            m_isTypeChecking = false;
            return resultOutcome;
        }

        const DataTypeSet& NodeableNodeOverloaded::FindPossibleInputTypes(size_t methodIndex, size_t index) const
        {
            static const DataTypeSet k_emptySet;

            if (methodIndex < m_methodSelections.size())
            {
                const OverloadConfiguration& overloadConfiguration = m_methodConfigurations[methodIndex];
                size_t startIndex = NodeableNodeOverloadedCpp::AdjustForHiddenNodeableThisPointer(overloadConfiguration, 0);

                return m_methodSelections[methodIndex].FindPossibleInputTypes(startIndex + index);
            }

            return k_emptySet;
        }

        AZ::Outcome<void, AZStd::string> NodeableNodeOverloaded::IsValidOutputType(size_t methodIndex, size_t index, const Data::Type& dataType)
        {
            // If we are type checking we don't want to recurse in here. Just return success, since we know we triggered this so the type is valid.
            if (m_isTypeChecking)
            {
                return AZ::Success();
            }

            if (methodIndex >= m_methodConfigurations.size())
            {
                return AZ::Failure(AZStd::string("Invalid Method index given to Nodeable Node Overloaded."));
            }

            m_isTypeChecking = true;

            AZ::Outcome<void, AZStd::string> resultOutcome = AZ::Failure(AZStd::string::format("Nodeable Node Overload does not support the type %s in its current coniguration", ScriptCanvas::Data::GetName(dataType).c_str()));

            const OverloadSelection& overloadSelection = m_methodSelections[methodIndex];

            auto outputDataIter = overloadSelection.m_outputDataTypes.find(index);

            if (outputDataIter != overloadSelection.m_outputDataTypes.end())
            {
                if (outputDataIter->second.contains(dataType))
                {
                    DataIndexMapping inputMapping;
                    DataIndexMapping outputMapping;

                    // Only care about display types. Not where it gets the information for this.
                    const bool checkForConnections = false;
                    FindDataIndexMappings(methodIndex, inputMapping, outputMapping, checkForConnections);

                    outputMapping[index] = dataType;

                    resultOutcome = IsValidConfiguration(methodIndex, inputMapping, outputMapping);
                }
            }

            m_isTypeChecking = false;
            return resultOutcome;
        }

        const DataTypeSet& NodeableNodeOverloaded::FindPossibleOutputTypes(size_t methodIndex, size_t index) const
        {
            static const DataTypeSet k_emptySet;

            if (methodIndex < m_methodSelections.size())
            {
                return m_methodSelections[methodIndex].FindPossibleInputTypes(index);
            }

            return k_emptySet;
        }

        void NodeableNodeOverloaded::OnReconfigurationBegin()
        {
            // Stop it from reparsing display updates
            m_updatingDisplayTypes = true;
        }

        void NodeableNodeOverloaded::OnReconfigurationEnd()
        {
            m_updatingDisplayTypes = false;
            m_slotTypeChange = false;

            bool checkForConnections = false;
            RefreshAvailableNodeables(checkForConnections);
        }

        void NodeableNodeOverloaded::OnSanityCheckDisplay()
        {
            RefreshAvailableNodeables(true);
            UpdateSlotDisplay();
        }

    }
}
