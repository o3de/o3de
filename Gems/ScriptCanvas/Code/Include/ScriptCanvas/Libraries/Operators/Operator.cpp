/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Operator.h"

#include <AzCore/RTTI/BehaviorContext.h>

#include <ScriptCanvas/Core/Contracts/TypeContract.h>
#include <ScriptCanvas/Core/Contracts/ConnectionLimitContract.h>
#include <ScriptCanvas/Core/Contracts/SupportsMethodContract.h>

#include <ScriptCanvas/Data/Data.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Operators
        {
            OperatorBase::OperatorBase()
                : m_sourceType(ScriptCanvas::Data::Type::Invalid())
                , m_sourceDisplayType(ScriptCanvas::Data::Type::Invalid())
            {
            }

            OperatorBase::OperatorBase(const OperatorConfiguration& operatorConfiguration)
                : Node()
                , m_sourceType(ScriptCanvas::Data::Type::Invalid())
                , m_sourceDisplayType(ScriptCanvas::Data::Type::Invalid())
                , m_operatorConfiguration(operatorConfiguration)
            {
            }

            bool OperatorBase::IsSourceSlotId(const SlotId& slotId) const
            {
                return m_sourceSlots.find(slotId) != m_sourceSlots.end();
            }

            const OperatorBase::SlotSet& OperatorBase::GetSourceSlots() const
            {
                return m_sourceSlots;
            }

            ScriptCanvas::Data::Type OperatorBase::GetSourceType() const
            {
                return m_sourceType;
            }

            AZ::TypeId OperatorBase::GetSourceAZType() const
            {
                return m_sourceTypeId;
            }

            ScriptCanvas::Data::Type OperatorBase::GetDisplayType() const
            {
                return m_sourceDisplayType;
            }

            void OperatorBase::OnInit()
            {
                // If we don't have any source slots. Add at least one.
                if (m_sourceSlots.empty())
                {
                    for (const auto& sourceConfiguration : m_operatorConfiguration.m_sourceSlotConfigurations)
                    {
                        AddSourceSlot(sourceConfiguration);
                    }
                }
                else
                {
                    bool groupedSourceSlots = false;

                    for (auto sourceSlotId : m_sourceSlots)
                    {
                        ///////// Version Conversion to Dynamic Grouped based operators
                        Slot* slot = GetSlot(sourceSlotId);

                        if (slot->IsDynamicSlot() && slot->GetDynamicGroup() == AZ::Crc32())
                        {
                            groupedSourceSlots = true;
                            SetDynamicGroup(sourceSlotId, GetSourceDynamicTypeGroup());
                        }

                        if (slot->GetDisplayGroup() == AZ::Crc32())
                        {
                            slot->SetDisplayGroup(GetSourceDisplayGroup());
                        }
                        ////////

                        EndpointNotificationBus::MultiHandler::BusConnect({ GetEntityId(), sourceSlotId });
                    }

                    if (groupedSourceSlots && m_sourceDisplayType.IsValid())
                    {
                        SetDisplayType(GetSourceDynamicTypeGroup(), m_sourceDisplayType);
                    }
                }

                for (const SlotId& inputSlot : m_inputSlots)
                {
                    ///////// Version Conversion to Dynamic Grouped based operators
                    Slot* slot = GetSlot(inputSlot);

                    if (slot->IsDynamicSlot() && slot->GetDynamicGroup() == AZ::Crc32())
                    {
                        SetDynamicGroup(inputSlot, GetSourceDynamicTypeGroup());
                    }

                    if (slot->GetDisplayGroup() == AZ::Crc32())
                    {
                        slot->SetDisplayGroup(GetSourceDisplayGroup());
                    }
                    ////////

                    EndpointNotificationBus::MultiHandler::BusConnect({ GetEntityId(), inputSlot });
                }

                for (const SlotId& outputSlot : m_outputSlots)
                {
                    ///////// Version Conversion to Dynamic Grouped based operators
                    Slot* slot = GetSlot(outputSlot);

                    if (slot->IsDynamicSlot() && slot->GetDynamicGroup() == AZ::Crc32())
                    {
                        SetDynamicGroup(outputSlot, GetSourceDynamicTypeGroup());
                    }

                    if (slot->GetDisplayGroup() == AZ::Crc32())
                    {
                        slot->SetDisplayGroup(GetSourceDisplayGroup());
                    }
                    ////////

                    EndpointNotificationBus::MultiHandler::BusConnect({ GetEntityId(), outputSlot });
                }

                // Version Conversion for the differing source slots will be removed in a future revision
                // Certain container operators would dynamically add the container output pin and store that in the
                // m_input/m_output pins so we need to re-scour the input/output pins to see if theres a matching pin we
                // can pull over.
                if (m_operatorConfiguration.m_sourceSlotConfigurations.size() != m_sourceSlots.size())
                {
                    AZStd::vector<SourceSlotConfiguration> unhandledConfigurations = m_operatorConfiguration.m_sourceSlotConfigurations;

                    SlotSet explorableSourceSlots = m_sourceSlots;

                    auto configurationIter = unhandledConfigurations.begin();

                    while (configurationIter != unhandledConfigurations.end())
                    {
                        const SourceSlotConfiguration& configuration = (*configurationIter);
                        bool handledConfiguration = false;

                        for (SlotId slotId : explorableSourceSlots)
                        {
                            Slot* slot = GetSlot(slotId);

                            if (slot == nullptr)
                            {
                                continue;
                            }

                            if (configuration.m_sourceType == SourceType::SourceInput
                                && slot->IsInput())
                            {
                                if (!slot->IsDynamicSlot())
                                {
                                    AZ_Error("ScriptCanvas", false, "Operator Source Slot is not Dynamic Data Type");
                                    continue;
                                }

                                if (configuration.m_dynamicDataType == slot->GetDynamicDataType())
                                {
                                    handledConfiguration = true;
                                    explorableSourceSlots.erase(slotId);
                                    break;
                                }
                            }
                        }

                        if (!handledConfiguration)
                        {
                            if (configuration.m_sourceType == SourceType::SourceInput)
                            {
                                for (SlotId inputId : m_inputSlots)
                                {
                                    Slot* inputSlot = GetSlot(inputId);

                                    if (inputSlot)
                                    {
                                        // If its not a dynamic slot we can't do anything with it.
                                        if (!inputSlot->IsDynamicSlot())
                                        {
                                            continue;
                                        }

                                        // Pass the ownership into the source slots if we match the dynamic data types.
                                        if (inputSlot->GetDynamicDataType() == configuration.m_dynamicDataType)
                                        {
                                            handledConfiguration = true;
                                            m_sourceSlots.insert(inputId);
                                            m_inputSlots.erase(inputId);
                                            break;
                                        }
                                    }
                                }
                            }
                            else
                            {
                                for (SlotId outputId : m_outputSlots)
                                {
                                    Slot* outputSlot = GetSlot(outputId);

                                    if (outputSlot)
                                    {
                                        if (outputSlot->IsDynamicSlot())
                                        {
                                            // Pass the ownership into the source slots if we match the dynamic data types.
                                            if (outputSlot->GetDynamicDataType() == configuration.m_dynamicDataType)
                                            {
                                                handledConfiguration = true;
                                                m_sourceSlots.insert(outputId);
                                                m_outputSlots.erase(outputId);
                                                break;
                                            }
                                        }
                                        // Otherwise if we had a valid source type. We'll convert the output slot to a dynamic slot in an attempt
                                        // to maintain the old container nodes
                                        else if (m_sourceType.IsValid() && m_sourceType == outputSlot->GetDataType())
                                        {
                                            handledConfiguration = true;

                                            outputSlot->SetDynamicDataType(configuration.m_dynamicDataType);
                                            SetDynamicGroup(outputSlot->GetId(), GetSourceDynamicTypeGroup());

                                            m_sourceSlots.insert(outputId);
                                            m_outputSlots.erase(outputId);
                                            break;
                                        }
                                    }
                                }
                            }
                        }

                        if (handledConfiguration)
                        {
                            configurationIter = unhandledConfigurations.erase(configurationIter);
                        }
                        else
                        {
                            ++configurationIter;
                        }
                    }

                    if (!unhandledConfigurations.empty())
                    {
                        for (const auto& sourceConfiguration : unhandledConfigurations)
                        {
                            AddSourceSlot(sourceConfiguration);
                        }
                    }
                }
                ////

                if (m_sourceType.IsValid())
                {
                    PopulateAZTypes(m_sourceType);
                }
            }

            void OperatorBase::OnDynamicGroupDisplayTypeChanged(const AZ::Crc32& dynamicGroup, const Data::Type& dataType)
            {
                if (dynamicGroup == GetSourceDynamicTypeGroup())
                {
                    if (m_sourceType != dataType
                        && dataType.IsValid())
                    {
                        RemoveInputs();
                        RemoveOutputs();

                        m_sourceType = dataType;

                        PopulateAZTypes(m_sourceType);

                        OnSourceTypeChanged();
                    }

                    if (m_sourceDisplayType != dataType)
                    {
                        m_sourceDisplayType = dataType;
                        OnDisplayTypeChanged(dataType);
                    }
                }
            }

            void OperatorBase::OnSlotRemoved(const SlotId& slotId)
            {
                m_inputSlots.erase(slotId);
                m_outputSlots.erase(slotId);
            }

            Slot* OperatorBase::GetFirstInputSourceSlot() const
            {
                for (const SlotId& slotId : m_sourceSlots)
                {
                    Slot* slot = GetSlot(slotId);

                    if (slot && slot->IsInput())
                    {
                        return slot;
                    }
                }

                return nullptr;
            }

            Slot* OperatorBase::GetFirstOutputSourceSlot() const
            {
                for (const SlotId& slotId : m_sourceSlots)
                {
                    Slot* slot = GetSlot(slotId);

                    if (slot && slot->IsOutput())
                    {
                        return slot;
                    }
                }

                return nullptr;
            }

            ScriptCanvas::SlotId OperatorBase::AddSourceSlot(SourceSlotConfiguration sourceConfiguration)
            {
                SlotId sourceSlotId;
                DynamicDataSlotConfiguration slotConfiguration;

                if (sourceConfiguration.m_sourceType == SourceType::SourceInput)
                {
                    slotConfiguration.m_contractDescs = { { []() { return aznew ConnectionLimitContract(1); } } };
                }

                ConfigureContracts(sourceConfiguration.m_sourceType, slotConfiguration.m_contractDescs);

                slotConfiguration.m_name = sourceConfiguration.m_name;
                slotConfiguration.m_toolTip = sourceConfiguration.m_tooltip;
                slotConfiguration.m_addUniqueSlotByNameAndType = true;
                slotConfiguration.m_dynamicDataType = sourceConfiguration.m_dynamicDataType;
                slotConfiguration.m_dynamicGroup = GetSourceDynamicTypeGroup();
                slotConfiguration.m_displayGroup = GetSourceDisplayGroup();

                if (sourceConfiguration.m_sourceType == SourceType::SourceInput)
                {
                    slotConfiguration.SetConnectionType(ConnectionType::Input);
                }
                else if (sourceConfiguration.m_sourceType == SourceType::SourceOutput)
                {
                    slotConfiguration.SetConnectionType(ConnectionType::Output);
                }

                sourceSlotId = AddSlot(slotConfiguration);

                m_sourceSlots.insert(sourceSlotId);

                if (m_sourceType.IsValid())
                {
                    Slot* slot = GetSlot(sourceSlotId);

                    if (slot)
                    {
                        slot->SetDisplayType(m_sourceType);
                    }
                }

                EndpointNotificationBus::MultiHandler::BusConnect({ GetEntityId(), sourceSlotId });

                return sourceSlotId;
            }

            void OperatorBase::ConfigureContracts(SourceType sourceType, AZStd::vector<ContractDescriptor>& contractDescs)
            {
                AZ_UNUSED(contractDescs);
                AZ_UNUSED(sourceType);
            }

            void OperatorBase::CustomizeReplacementNode(Node* replacementNode, AZStd::unordered_map<SlotId, AZStd::vector<SlotId>>& outSlotIdMap) const
            {
                auto newDataInSlots = replacementNode->GetSlotsByType(ScriptCanvas::CombinedSlotType::DataIn);
                auto oldDataInSlots = this->GetSlotsByType(ScriptCanvas::CombinedSlotType::DataIn);
                if (newDataInSlots.size() == oldDataInSlots.size())
                {
                    for (size_t index = 0; index < newDataInSlots.size(); index++)
                    {
                        outSlotIdMap.emplace(oldDataInSlots[index]->GetId(), AZStd::vector<SlotId>{ newDataInSlots[index]->GetId() });
                    }
                }

                auto newDataOutSlots = replacementNode->GetSlotsByType(ScriptCanvas::CombinedSlotType::DataOut);
                auto oldDataOutSlots = this->GetSlotsByType(ScriptCanvas::CombinedSlotType::DataOut);
                if (newDataOutSlots.size() == oldDataOutSlots.size())
                {
                    for (size_t index = 0; index < newDataOutSlots.size(); index++)
                    {
                        outSlotIdMap.emplace(oldDataOutSlots[index]->GetId(), AZStd::vector<SlotId>{ newDataOutSlots[index]->GetId() });
                    }
                }
            }

            void OperatorBase::RemoveInputs()
            {
                SlotSet inputSlots = m_inputSlots;

                for (SlotId slotId : inputSlots)
                {
                    RemoveSlot(slotId);
                }
            }

            void OperatorBase::OnEndpointConnected(const Endpoint& endpoint)
            {
                Node::OnEndpointConnected(endpoint);

                const SlotId& currentSlotId = EndpointNotificationBus::GetCurrentBusId()->GetSlotId();

                bool isInBatchAdd = false;
                GraphRequestBus::EventResult(isInBatchAdd, GetOwningScriptCanvasId(), &GraphRequests::IsBatchAddingGraphData);

                if (IsSourceSlotId(currentSlotId))
                {
                    auto node = AZ::EntityUtils::FindFirstDerivedComponent<Node>(endpoint.GetNodeId());
                    if (node)
                    {
                        if (m_sourceType.IsValid())
                        {
                            OnSourceConnected(currentSlotId);
                        }
                    }
                }
                else if (!isInBatchAdd)
                {
                    Slot* slot = GetSlot(currentSlotId);
                    if (slot && slot->GetDescriptor() == SlotDescriptors::DataIn())
                    {
                        OnDataInputSlotConnected(currentSlotId, endpoint);
                        return;
                    }
                }
            }

            void OperatorBase::OnEndpointDisconnected(const Endpoint& endpoint)
            {
                Node::OnEndpointDisconnected(endpoint);

                const SlotId& currentSlotId = EndpointNotificationBus::GetCurrentBusId()->GetSlotId();

                if (IsSourceSlotId(currentSlotId))
                {
                    OnSourceDisconnected(currentSlotId);
                }
                else
                {
                    Slot* slot = GetSlot(currentSlotId);
                    if (slot && slot->GetDescriptor() == SlotDescriptors::DataIn())
                    {
                        OnDataInputSlotDisconnected(currentSlotId, endpoint);
                        EndpointNotificationBus::MultiHandler::BusDisconnect({ GetEntityId(), slot->GetId() });
                        return;
                    }
                }
            }

            AZ::BehaviorMethod* OperatorBase::GetOperatorMethod(const char* methodName)
            {
                AZ::BehaviorMethod* method = nullptr;

                const AZ::BehaviorClass* behaviorClass = AZ::BehaviorContextHelper::GetClass(m_sourceTypeId);
                if (behaviorClass)
                {
                    auto methodIt = behaviorClass->m_methods.find(methodName);

                    if (methodIt != behaviorClass->m_methods.end())
                    {
                        method = methodIt->second;
                    }
                }

                return method;
            }

            SlotId OperatorBase::AddSlotWithSourceType()
            {
                Data::Type type = Data::Type::Invalid();

                if (!m_sourceTypes.empty())
                {
                    type = Data::FromAZType(m_sourceTypes[0]);
                }

                SlotId inputSlotId;

                {
                    DynamicDataSlotConfiguration slotConfiguration;

                    if (type.IsValid())
                    {
                        slotConfiguration.m_name = Data::GetName(type);
                    }
                    else
                    {
                        slotConfiguration.m_name = "Value";
                    }

                    slotConfiguration.m_displayType = type;
                    slotConfiguration.m_dynamicDataType = DynamicDataType::Any;
                    slotConfiguration.SetConnectionType(ConnectionType::Input);
                    // consider reverting this
                    slotConfiguration.m_addUniqueSlotByNameAndType = false;
                    slotConfiguration.m_dynamicGroup = GetSourceDynamicTypeGroup();
                    slotConfiguration.m_displayGroup = GetSourceDisplayGroup();

                    inputSlotId = AddSlot(slotConfiguration);
                }

                if (inputSlotId.IsValid())
                {
                    m_inputSlots.insert(inputSlotId);
                    EndpointNotificationBus::MultiHandler::BusConnect({ GetEntityId(), inputSlotId });

                    OnInputSlotAdded(inputSlotId);
                }

                return inputSlotId;
            }

            bool OperatorBase::HasSourceConnection() const
            {
                for (auto sourceSlotId : m_sourceSlots)
                {
                    if (IsConnected(sourceSlotId))
                    {
                        return true;
                    }
                }

                return false;
            }

            bool OperatorBase::AreSourceSlotsFull(SourceType sourceType) const
            {
                bool isFull = true;
                for (auto sourceSlotId : m_sourceSlots)
                {
                    auto sourceSlot = GetSlot(sourceSlotId);

                    if (!sourceSlot->IsData())
                    {
                        continue;
                    }

                    switch (sourceType)
                    {
                    case SourceType::SourceInput:
                        if (sourceSlot->IsData() && sourceSlot->IsInput())
                        {
                            isFull = IsConnected(sourceSlotId);
                        }
                        break;
                    case SourceType::SourceOutput:
                        if (sourceSlot->GetDescriptor() == SlotDescriptors::DataOut())
                        {
                            isFull = IsConnected(sourceSlotId);
                        }
                        break;
                    default:
                        // No idea what this is. But it's not supported.
                        AZ_Error("ScriptCanvas", false, "Operator is operating upon an unknown SourceType");
                        isFull = false;
                        break;
                    }

                    if (!isFull)
                    {
                        break;
                    }
                }

                return isFull;
            }

            void OperatorBase::PopulateAZTypes(ScriptCanvas::Data::Type dataType)
            {
                m_sourceTypeId = ScriptCanvas::Data::ToAZType(dataType);

                m_sourceTypes.clear();

                if (ScriptCanvas::Data::IsContainerType(m_sourceTypeId))
                {
                    AZStd::vector<AZ::Uuid> types = ScriptCanvas::Data::GetContainedTypes(m_sourceTypeId);
                    m_sourceTypes.insert(m_sourceTypes.end(), types.begin(), types.end());
                }
                else
                {
                    // The data type is then a source type
                    m_sourceTypes.push_back(m_sourceTypeId);
                }
            }

            void OperatorBase::RemoveOutputs()
            {
                SlotSet outputSlots = m_outputSlots;
                for (SlotId slotId : outputSlots)
                {
                    RemoveSlot(slotId);
                }
            }

        }
    }
}
