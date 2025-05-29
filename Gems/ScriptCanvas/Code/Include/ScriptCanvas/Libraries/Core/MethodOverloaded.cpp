/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MethodOverloaded.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/RTTI/BehaviorContextUtilities.h>
#include <ScriptCanvas/Core/Contracts/MethodOverloadContract.h>
#include <ScriptCanvas/Core/SlotConfigurations.h>
#include <ScriptCanvas/Utils/BehaviorContextUtils.h>

namespace MethodOverloadedCpp
{
    enum Version
    {
        Original = 0,
        OrderedInputIds,
        OverloadTriggerInfo,
        OverloadTriggerInfo2,
        SwapIndexLookupForPrototype,
        DataDrivingOverloads,

        // add version label above
        Current,
    };
}    

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            /////////////////////
            // MethodOverloaded
            /////////////////////

            static bool MethodOverloadedVersionConverter(AZ::SerializeContext& , AZ::SerializeContext::DataElementNode& classElement)
            {
                if (classElement.GetVersion() <= MethodOverloadedCpp::DataDrivingOverloads)
                {
                    classElement.RemoveElementByName(AZ_CRC_CE("activeIndex"));
                    classElement.RemoveElementByName(AZ_CRC_CE("activePrototype"));
                    classElement.RemoveElementByName(AZ_CRC_CE("overloadSelectionTriggerSlotIds"));
                    classElement.RemoveElementByName(AZ_CRC_CE("overloadSelectionTriggerIndices"));
                }

                return true;
            }

            void MethodOverloaded::Reflect(AZ::ReflectContext* reflectContext)
            {
                if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
                {
                    serializeContext->Class<MethodOverloaded, Method>()
                        ->Version(MethodOverloadedCpp::Version::Current, &MethodOverloadedVersionConverter)
#if defined(OBJECT_STREAM_EDITOR_ASSET_LOADING_SUPPORT_ENABLED)////
                        ->EventHandler<SerializeContextReadWriteHandler<MethodOverloaded>>()
#endif//defined(OBJECT_STREAM_EDITOR_ASSET_LOADING_SUPPORT_ENABLED)
                        ->Field("orderedInputSlotIds", &MethodOverloaded::m_orderedInputSlotIds)
                        ->Field("outputSlotIds", &MethodOverloaded::m_outputSlotIds)
                        ;

                    if (auto editContext = serializeContext->GetEditContext())
                    {
                        editContext->Class<MethodOverloaded>("MethodOverloaded", "MethodOverloaded")
                            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                            ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                            ;
                    }
                }
            }

            void MethodOverloaded::OnInit()
            {
                ConfigureContracts();
            }

            void MethodOverloaded::OnPostActivate()
            {
                RefreshActiveIndexes();
            }

            void MethodOverloaded::OnSlotDisplayTypeChanged(const SlotId&, const ScriptCanvas::Data::Type&)
            {
                if (!m_updatingDisplay)
                {
                    RefreshActiveIndexes(true, true);
                    UpdateSlotDisplay();
                }
            }

            Data::Type MethodOverloaded::FindFixedDataTypeForSlot(const Slot& slot) const
            {
                if (!m_isCheckingForDataTypes)
                {
                    for (int inputIndex = 0; inputIndex < m_orderedInputSlotIds.size(); ++inputIndex)
                    {
                        if (slot.GetId() == m_orderedInputSlotIds[inputIndex])
                        {
                            auto inputDataTypesIter = m_overloadSelection.m_inputDataTypes.find(inputIndex);

                            if (inputDataTypesIter != m_overloadSelection.m_inputDataTypes.end()
                                && inputDataTypesIter->second.size() == 1)
                            {
                                return (*inputDataTypesIter->second.begin());
                            }
                        }
                    }

                    for (int outputIndex = 0; outputIndex < m_outputSlotIds.size(); ++outputIndex)
                    {
                        if (slot.GetId() == m_outputSlotIds[outputIndex])
                        {
                            auto outputDataTypesIter = m_overloadSelection.m_outputDataTypes.find(outputIndex);

                            if (outputDataTypesIter != m_overloadSelection.m_outputDataTypes.end()
                                && outputDataTypesIter->second.size() == 1)
                            {
                                return (*outputDataTypesIter->second.begin());
                            }
                        }
                    }
                }

                return Data::Type::Invalid();
            }

            AZ::Outcome<AZStd::string, void> MethodOverloaded::GetFunctionCallName([[maybe_unused]] const Slot* slot) const
            {
                AZStd::string overloadName;

                int activeIndex = GetActiveIndex();

                if (activeIndex >= 0 && activeIndex < m_overloadConfiguration.m_overloads.size())
                {
                    const auto& behaviorPair = m_overloadConfiguration.m_overloads[activeIndex];

                    const AZ::BehaviorMethod* behaviorMethod = behaviorPair.first;
                    const AZ::BehaviorClass* behaviorClass = behaviorPair.second;

                    if (behaviorPair.first->IsMember()
                        || AZ::FindAttribute(AZ::Script::Attributes::TreatAsMemberFunction, behaviorMethod->m_attributes))
                    {
                        overloadName = BehaviorContextUtils::FindExposedMethodName(*behaviorMethod, behaviorClass);
                    }

                    if (overloadName.empty())
                    {
                        overloadName = AZ::GetOverloadName((*behaviorMethod), activeIndex, m_overloadConfiguration.m_overloadVariance, Method::GetName());
                    }
                }

                if (!overloadName.empty())
                {
                    return AZ::Success(overloadName);
                }
                else
                {
                    return AZ::Failure();
                }
            }

            void MethodOverloaded::OnEndpointDisconnected(const Endpoint& targetEndpoint)
            {
                m_updatingDisplay = true;
                Method::OnEndpointDisconnected(targetEndpoint);
                m_updatingDisplay = false;

                RefreshActiveIndexes();
                UpdateSlotDisplay();
            }

            void MethodOverloaded::InitializeMethod(const MethodConfiguration& config)
            {
                SetupMethodData(&config.m_method, config.m_class);

                // this prevents repeated updates based on changes to slots
                Method::InitializeMethod(config);
                SetClassNamePretty("");
                RefreshActiveIndexes();

                ConfigureContracts();
                SetWarnOnMissingFunction(true);
            }

            SlotId MethodOverloaded::AddMethodInputSlot(const MethodConfiguration& config, size_t argumentIndex)
            {
                const AZ::BehaviorParameter* argumentPtr = config.m_method.GetArgument(argumentIndex);

                if (!argumentPtr)
                {
                    return SlotId{};
                }

                const auto& argument = *argumentPtr;
                auto nameAndToolTip = MethodHelper::GetArgumentNameAndToolTip(config, argumentIndex);

                SlotId slotId;

                if (m_overloadConfiguration.m_overloadVariance.m_input.find(argumentIndex) != m_overloadConfiguration.m_overloadVariance.m_input.end())
                {
                    DynamicDataSlotConfiguration slotConfig;
                    slotConfig.m_name = nameAndToolTip.first;
                    slotConfig.m_toolTip = nameAndToolTip.second;
                    slotConfig.m_addUniqueSlotByNameAndType = true;

                    DynamicDataType dynamicDataType = DynamicDataType::Any;

                    auto inputTypeIter = m_overloadConfiguration.m_inputDataTypes.find(argumentIndex);

                    if (inputTypeIter != m_overloadConfiguration.m_inputDataTypes.end())
                    {
                        dynamicDataType = inputTypeIter->second;
                    }

                    slotConfig.m_dynamicDataType = dynamicDataType;
                    slotConfig.SetConnectionType(ConnectionType::Input);

                    slotConfig.m_contractDescs = AZStd::vector<ScriptCanvas::ContractDescriptor>
                    {
                        // Restricted Type Contract
                        { []() { return aznew ScriptCanvas::OverloadContract(); } }
                    };

                    slotId = AddSlot(slotConfig);
                }
                else
                {
                    AZ::ScriptCanvasAttributes::HiddenIndices hiddenIndices;
                    AZ::ReadAttribute(hiddenIndices, AZ::ScriptCanvasAttributes::HiddenParameterIndex, config.m_method.m_attributes);

                    DataSlotConfiguration slotConfig(!AZ::BehaviorContextHelper::IsStringParameter(argument) ? Data::FromAZType(argument.m_typeId) : Data::Type::String());
                    slotConfig.m_name = nameAndToolTip.first;
                    slotConfig.m_toolTip = nameAndToolTip.second;
                    slotConfig.m_addUniqueSlotByNameAndType = true;
                    slotConfig.m_isVisible = AZStd::find(hiddenIndices.begin(), hiddenIndices.end(), argumentIndex) == hiddenIndices.end();
                    slotConfig.SetConnectionType(ConnectionType::Input);
                    slotId = AddSlot(slotConfig);
                }
                
                EndpointNotificationBus::MultiHandler::BusConnect({ GetEntityId(), slotId });
                m_orderedInputSlotIds.push_back(slotId);
                return slotId;
            }

            void MethodOverloaded::OnInitializeOutputPost(const MethodOutputConfig& config)
            {
                if (config.resultSlotIdsOut)
                {
                    m_outputSlotIds = *config.resultSlotIdsOut;
                }
            }

            void MethodOverloaded::OnInitializeOutputPre(MethodOutputConfig& config)
            {
                config.isReturnValueOverloaded = !m_overloadConfiguration.m_overloadVariance.m_output.empty();
            }

            DynamicDataType MethodOverloaded::GetOverloadedOutputType(size_t resultIndex) const
            {
                auto returnIter = m_overloadConfiguration.m_outputDataTypes.find(resultIndex);

                if (returnIter != m_overloadConfiguration.m_outputDataTypes.end())
                {
                    return returnIter->second;
                }

                return DynamicDataType::Any;
            }

            AZ::Outcome<void, AZStd::string> MethodOverloaded::IsValidInputType(size_t index, const Data::Type& dataType)
            {
                // If we are type checking we don't want to recurse in here. Just return success, since we know we triggered this so the type is valid.
                if (m_isTypeChecking)
                {
                    return AZ::Success();
                }

                m_isTypeChecking = true;
                AZ::Outcome<void, AZStd::string> resultOutcome = AZ::Failure(AZStd::string::format("Method Overload(%s) does not support the type %s in its current coniguration", GetName().c_str(), ScriptCanvas::Data::GetName(dataType).c_str()));

                auto inputDataIter = m_overloadSelection.m_inputDataTypes.find(index);

                if (inputDataIter != m_overloadSelection.m_inputDataTypes.end())
                {
                    if (inputDataIter->second.contains(dataType))
                    {
                        DataIndexMapping inputMapping;
                        DataIndexMapping outputMapping;

                        // Only care about display types. Not where it gets the information for this.
                        const bool checkForConnections = false;
                        FindDataIndexMappings(inputMapping, outputMapping, checkForConnections);

                        inputMapping[index] = dataType;

                        resultOutcome = IsValidConfiguration(inputMapping, outputMapping);
                    }
                }

                m_isTypeChecking = false;
                return resultOutcome;
            }

            const DataTypeSet& MethodOverloaded::FindPossibleInputTypes(size_t index) const
            {
                return m_overloadSelection.FindPossibleInputTypes(index);
            }

            AZ::Outcome<void, AZStd::string> MethodOverloaded::IsValidOutputType(size_t index, const Data::Type& dataType)
            {
                // If we are type checking we don't want to recurse in here. Just return success, since we know we triggered this so the type is valid.
                if (m_isTypeChecking)
                {
                    return AZ::Success();
                }

                m_isTypeChecking = true;

                AZ::Outcome<void, AZStd::string> resultOutcome = AZ::Failure(AZStd::string::format("Method Overload(%s) does not support the type %s in its current coniguration", GetName().c_str(), ScriptCanvas::Data::GetName(dataType).c_str()));

                auto outputDataIter = m_overloadSelection.m_outputDataTypes.find(index);

                if (outputDataIter != m_overloadSelection.m_outputDataTypes.end())
                {
                    if (outputDataIter->second.contains(dataType))
                    {
                        DataIndexMapping inputMapping;
                        DataIndexMapping outputMapping;

                        // Only care about display types. Not where it gets the information for this.
                        const bool checkForConnections = false;
                        FindDataIndexMappings(inputMapping, outputMapping, checkForConnections);

                        outputMapping[index] = dataType;

                        resultOutcome = IsValidConfiguration(inputMapping, outputMapping);
                    }
                }

                m_isTypeChecking = false;
                return resultOutcome;
            }

            const DataTypeSet& MethodOverloaded::FindPossibleOutputTypes(size_t index) const
            {
                return m_overloadSelection.FindPossibleOutputTypes(index);
            }

            void MethodOverloaded::OnReconfigurationBegin()
            {
                m_updatingDisplay = true;
            }

            void MethodOverloaded::OnReconfigurationEnd()
            {
                m_updatingDisplay = false;

                const bool checkForConnections = false;
                RefreshActiveIndexes(checkForConnections);
            }

            void MethodOverloaded::OnSanityCheckDisplay()
            {
                const bool checkForConnections = true;
                RefreshActiveIndexes(checkForConnections);
                UpdateSlotDisplay();
            }

            bool MethodOverloaded::IsAmbiguousOverload() const
            {
                return m_overloadSelection.IsAmbiguous();
            }

            int MethodOverloaded::GetActiveIndex() const
            {
                return m_overloadSelection.GetActiveIndex();
            }

            Grammar::FunctionPrototype MethodOverloaded::GetInputSignature() const
            {
                Grammar::FunctionPrototype signature;

                for (const auto& inputSlotId : m_orderedInputSlotIds)
                {
                    if (auto* slot = GetSlot(inputSlotId))
                    {
                        signature.m_inputs.emplace_back(aznew Grammar::Variable(Datum(slot->GetDataType(), Datum::eOriginality::Original)));
                    }
                }

                return signature;
            }

#if defined(OBJECT_STREAM_EDITOR_ASSET_LOADING_SUPPORT_ENABLED)////
            void MethodOverloaded::OnWriteEnd()
            {
                OnDeserialize();
            }
#endif//defined(OBJECT_STREAM_EDITOR_ASSET_LOADING_SUPPORT_ENABLED)

            void MethodOverloaded::OnDeserialize()
            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(GetMutex());
                Node::OnDeserialize();

                // look for a standard, class overload
                AZStd::tuple<const AZ::BehaviorMethod*, MethodType, EventType, const AZ::BehaviorClass*> methodLookup = LookupMethod();
                const AZ::BehaviorMethod* lookUpMethod = AZStd::get<0>(methodLookup);
                const AZ::BehaviorClass* lookUpClass = AZStd::get<3>(methodLookup);

                // check for an explicit overload
                if (!lookUpMethod)
                {
                    AZ::ExplicitOverloadInfo info = AZ::GetExplicitOverloads(Method::GetName());
                    if (!info.m_overloads.empty())
                    {
                        lookUpMethod = info.m_overloads[0].first;
                    }
                }
                
                if (lookUpMethod)
                {
                    SetupMethodData(lookUpMethod, lookUpClass);

                    const bool checkForConnections = false;
                    RefreshActiveIndexes(checkForConnections);
                    RefreshInput();

                    if (m_overloadSelection.m_availableIndexes.empty())
                    {
                        AZ_Warning(
                            "ScriptCanvas", false, "Method [%s] is overloaded with an invalid configuration.",
                            lookUpMethod->m_name.c_str());

                        /* Debug information to spew out the non-found configuration. Useful in debugging, and kind of tedious to write.
                           Keeping here as a quick ref commented out, since not useful in the general case.
                        DataIndexMapping concreteInputTypes;
                        DataIndexMapping concreteOutputTypes;

                        FindDataIndexMappings(concreteInputTypes, concreteOutputTypes, checkForConnections);

                        for (const auto& dataPair : concreteInputTypes)
                        {
                            AZ_TracePrintf("ScriptCanvas", "InputType: %s", Data::GetName(dataPair.second).c_str());
                        }

                        for (const auto& dataPair : concreteOutputTypes)
                        {
                            AZ_TracePrintf("ScriptCanvas", "OutputType: %s", Data::GetName(dataPair.second).c_str());
                        }
                        */
                    }
                }

                SetWarnOnMissingFunction(true);
            }

            void MethodOverloaded::SetupMethodData(const AZ::BehaviorMethod* behaviorMethod, const AZ::BehaviorClass* behaviorClass)
            {
                m_overloadConfiguration.SetupOverloads(behaviorMethod, behaviorClass);
            }

            void MethodOverloaded::ConfigureContracts()
            {
                for (size_t inputIndex = 0; inputIndex < m_orderedInputSlotIds.size(); ++inputIndex)
                {
                    SlotId slotId = m_orderedInputSlotIds[inputIndex];

                    Slot* inputSlot = GetSlot(slotId);

                    if (inputSlot)
                    {
                        OverloadContract* contract = inputSlot->FindContract<OverloadContract>();

                        if (contract)
                        {
                            contract->ConfigureContract(this, inputIndex, ConnectionType::Input);
                        }
                    }
                }

                for (size_t outputIndex = 0; outputIndex < m_outputSlotIds.size(); ++outputIndex)
                {
                    SlotId slotId = m_outputSlotIds[outputIndex];

                    Slot* inputSlot = GetSlot(slotId);

                    if (inputSlot)
                    {
                        OverloadContract* contract = inputSlot->FindContract<OverloadContract>();

                        if (contract)
                        {
                            contract->ConfigureContract(this, outputIndex, ConnectionType::Output);
                        }
                    }
                }
            }

            void MethodOverloaded::RefreshActiveIndexes(bool checkForConnections, bool adjustSlots)
            {
                DataIndexMapping concreteInputTypes;
                DataIndexMapping concreteOutputTypes;

                FindDataIndexMappings(concreteInputTypes, concreteOutputTypes, checkForConnections);

                m_overloadConfiguration.PopulateOverloadSelection(m_overloadSelection, concreteInputTypes, concreteOutputTypes);

                if (m_overloadSelection.m_availableIndexes.size() == 1)
                {
                    auto methodOverload = m_overloadConfiguration.m_overloads[(*m_overloadSelection.m_availableIndexes.begin())];

                    if (adjustSlots)
                    {
                        const size_t numArguments = methodOverload.first->GetNumArguments();
                        const size_t numInputSlots = m_orderedInputSlotIds.size();

                        if (numArguments > numInputSlots)
                        {
                            MethodConfiguration config(*methodOverload.first, GetMethodType());
                            AZStd::string_view lookupName = GetLookupName();
                            config.m_lookupName = &lookupName;

                            for (size_t index = numInputSlots; index != numArguments; ++index)
                            {
                                AddMethodInputSlot(config, index);
                            }
                        }
                        else if (numArguments < numInputSlots)
                        {
                            const size_t removeCount = numInputSlots - numArguments;
                            // remove extra slots, assuming remaining ones are of valid type (if not valid name)
                            for (size_t count = 0; count != removeCount; ++count)
                            {
                                RemoveSlot(m_orderedInputSlotIds.back());
                                m_orderedInputSlotIds.pop_back();
                            }
                        }
                    }

                    SetMethodUnchecked(methodOverload.first, methodOverload.second);
                }
            }

            void MethodOverloaded::FindDataIndexMappings(DataIndexMapping& inputMapping, DataIndexMapping& outputMapping, bool checkForConnections) const
            {
                const_cast<MethodOverloaded*>(this)->m_isCheckingForDataTypes = true;

                for (size_t inputIndex = 0; inputIndex < m_orderedInputSlotIds.size(); ++inputIndex)
                {
                    SlotId slotId = m_orderedInputSlotIds[inputIndex];
                    Slot* slot = GetSlot(slotId);

                    if (slot && (!checkForConnections || IsConnected((*slot))))
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
                                inputMapping[inputIndex] = displayType;
                            }
                        }
                    }
                }                

                for (size_t outputIndex = 0; outputIndex < m_outputSlotIds.size(); ++outputIndex)
                {
                    SlotId slotId = m_outputSlotIds[outputIndex];
                    Slot* slot = GetSlot(slotId);

                    if (slot && (!checkForConnections || IsConnected((*slot))))
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

                const_cast<MethodOverloaded*>(this)->m_isCheckingForDataTypes = false;
            }

            void MethodOverloaded::UpdateSlotDisplay()
            {
                m_updatingDisplay = true;

                for (size_t inputIndex = 0; inputIndex < m_orderedInputSlotIds.size(); ++inputIndex)
                {
                    SlotId inputSlotId = m_orderedInputSlotIds[inputIndex];

                    ScriptCanvas::Data::Type inputType = m_overloadSelection.GetInputDisplayType(inputIndex);

                    if (inputType.IsValid())
                    {
                        SetDisplayType(inputSlotId, inputType);
                    }
                    else
                    {
                        ClearDisplayType(inputSlotId);
                    }
                }

                for (size_t outputIndex = 0; outputIndex < m_outputSlotIds.size(); ++outputIndex)
                {
                    SlotId outputSlotId = m_outputSlotIds[outputIndex];

                    ScriptCanvas::Data::Type outputType = m_overloadSelection.GetOutputDisplayType(outputIndex);

                    if (outputType.IsValid())
                    {
                        SetDisplayType(outputSlotId, outputType);
                    }
                    else
                    {
                        ClearDisplayType(outputSlotId);
                    }
                }

                m_updatingDisplay = false;
            }

            AZ::Outcome<void, AZStd::string> MethodOverloaded::IsValidConfiguration(const DataIndexMapping& inputMapping, const DataIndexMapping& outputMapping) const
            {
                AZStd::unordered_set<AZ::u32> availableIndexes = m_overloadConfiguration.GenerateAvailableIndexes(inputMapping, outputMapping);

                DataSetIndexMapping inputDataTypes;
                m_overloadConfiguration.PopulateDataIndexMapping(availableIndexes, ScriptCanvas::ConnectionType::Input, inputDataTypes);

                DataSetIndexMapping outputDataTypes;
                m_overloadConfiguration.PopulateDataIndexMapping(availableIndexes, ScriptCanvas::ConnectionType::Output, outputDataTypes);

                for (size_t inputIndex = 0; inputIndex < m_orderedInputSlotIds.size(); ++inputIndex)
                {
                    auto inputMappingIter = inputMapping.find(inputIndex);

                    if (inputMappingIter != inputMapping.end())
                    {
                        continue;
                    }

                    auto inputDataTypeIter = inputDataTypes.find(inputIndex);

                    if (inputDataTypeIter->second.size() == 1)
                    {
                        ScriptCanvas::Data::Type dataType = (*inputDataTypeIter->second.begin());
                        SlotId slotId = m_orderedInputSlotIds[inputIndex];


                        auto isValidType = SlotAcceptsType(slotId, dataType);

                        if (!isValidType)
                        {
                            return isValidType;
                        }
                    }
                }

                for (size_t outputIndex = 0; outputIndex < m_outputSlotIds.size(); ++outputIndex)
                {
                    auto outputMappingIter = outputMapping.find(outputIndex);

                    if (outputMappingIter->second.IsValid())
                    {
                        continue;
                    }

                    auto outputDataTypeIter = inputDataTypes.find(outputIndex);

                    if (outputDataTypeIter->second.size() == 1)
                    {
                        ScriptCanvas::Data::Type dataType = (*outputDataTypeIter->second.begin());
                        SlotId slotId = m_outputSlotIds[outputIndex];


                        auto isValidType = SlotAcceptsType(slotId, dataType);

                        if (!isValidType)
                        {
                            return isValidType;
                        }
                    }
                }

                return AZ::Success();
            }
        } 
    } 
} 
