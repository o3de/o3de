/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MethodOverloadContract.h"

#include <ScriptCanvas/Core/ContractBus.h>
#include <ScriptCanvas/Core/Slot.h>

#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Libraries/Core/Method.h>

namespace ScriptCanvas
{
    //////////////////////
    // OverloadSelection
    //////////////////////

    bool OverloadSelection::IsAmbiguous() const
    {
        return m_availableIndexes.size() > 1;
    }

    AZ::u32 OverloadSelection::GetActiveIndex() const
    {
        if (m_availableIndexes.empty())
        {
            return std::numeric_limits<AZ::u32>::max();
        }

        return (*m_availableIndexes.begin());
    }

    const DataTypeSet& OverloadSelection::FindPossibleInputTypes(size_t index) const
    {
        static DataTypeSet k_emptyDataTypeSet;

        auto inputDataIter = m_inputDataTypes.find(index);

        if (inputDataIter != m_inputDataTypes.end())
        {
            return inputDataIter->second;
        }

        return k_emptyDataTypeSet;
    }

    const DataTypeSet& OverloadSelection::FindPossibleOutputTypes(size_t index) const
    {
        static DataTypeSet k_emptyDataTypeSet;

        auto outputDataIter = m_outputDataTypes.find(index);

        if (outputDataIter != m_outputDataTypes.end())
        {
            return outputDataIter->second;
        }

        return k_emptyDataTypeSet;
    }

    ScriptCanvas::Data::Type OverloadSelection::GetInputDisplayType(size_t index) const
    {
        auto inputIter = m_inputDataTypes.find(index);

        if (inputIter == m_inputDataTypes.end()
            || inputIter->second.size() != 1)
        {
            return ScriptCanvas::Data::Type::Invalid();
        }

        return (*inputIter->second.begin());
    }

    ScriptCanvas::Data::Type OverloadSelection::GetOutputDisplayType(size_t index) const
    {
        auto outputIter = m_outputDataTypes.find(index);

        if (outputIter == m_outputDataTypes.end()
            || outputIter->second.size() != 1)
        {
            return ScriptCanvas::Data::Type::Invalid();
        }

        return (*outputIter->second.begin());
    }

    //////////////////////////
    // OverloadConfiguration
    //////////////////////////

    void OverloadConfiguration::Clear()
    {
        m_prototypes.clear();
        m_overloads.clear();

        m_inputDataTypes.clear();
        m_outputDataTypes.clear();

        m_overloadVariance.m_input.clear();
        m_overloadVariance.m_output.clear();
    }

    void OverloadConfiguration::CopyData(const OverloadConfiguration& overloadConfiguration)
    {
        m_prototypes.insert(m_prototypes.end(), overloadConfiguration.m_prototypes.begin(), overloadConfiguration.m_prototypes.end());

        m_overloads.insert(m_overloads.end(), overloadConfiguration.m_overloads.begin(), overloadConfiguration.m_overloads.end());

        for (const auto& inputPair : overloadConfiguration.m_overloadVariance.m_input)
        {
            auto insertResult = m_overloadVariance.m_input.insert_key(inputPair.first);
            auto inputIter = insertResult.first;

            inputIter->second.insert(inputIter->second.begin(), inputPair.second.begin(), inputPair.second.end());
        }

        m_overloadVariance.m_output.insert(m_overloadVariance.m_output.begin(), overloadConfiguration.m_overloadVariance.m_output.begin(), overloadConfiguration.m_overloadVariance.m_output.end());
    }

    void OverloadConfiguration::SetupOverloads(const AZ::BehaviorMethod* behaviorMethod, const AZ::BehaviorClass* behaviorClass, AZ::VariantOnThis variantOnThis)
    {
        // \todo move all of this into a context so that is cached somewhere, and doesn't need a per-node basis
        m_overloads = AZ::OverloadsToVector(*behaviorMethod, behaviorClass);
        m_overloadVariance = AZ::GetOverloadVariance(*behaviorMethod, m_overloads, variantOnThis);
        m_prototypes.clear();

        for (auto& overload : m_overloads)
        {
            m_prototypes.push_back(ToSignature(*overload.first));
        }

        DetermineInputOutputTypes();        
    }

    void OverloadConfiguration::DetermineInputOutputTypes()
    {
        // Iterate over our possible input types and determine what class of objects we accept.
        for (const auto& paramTypes : m_overloadVariance.m_input)
        {
            bool isContainerType = false;
            bool isValueType = false;

            for (const AZ::BehaviorParameter* behaviorParameter : paramTypes.second)
            {
                if (behaviorParameter)
                {
                    ScriptCanvas::Data::Type dataType = ScriptCanvas::Data::FromAZType(behaviorParameter->m_typeId);
                    if (ScriptCanvas::Data::IsValueType(dataType))
                    {
                        isValueType = true;
                    }
                    else if (ScriptCanvas::Data::IsContainerType(dataType))
                    {
                        isContainerType = true;
                    }

                    if (isValueType && isContainerType)
                    {
                        break;
                    }
                }
            }

            if (isContainerType && isValueType)
            {
                m_inputDataTypes[paramTypes.first] = DynamicDataType::Any;
            }
            else if (isContainerType)
            {
                m_inputDataTypes[paramTypes.first] = DynamicDataType::Container;
            }
            else if (isValueType)
            {
                m_inputDataTypes[paramTypes.first] = DynamicDataType::Value;
            }
        }

        {
            // Iterate over our possible input types and determine what class of objects we accept.
            //
            // Outputs are treated differently then inputs, since they could be a touple, so this requires a fancier iteration.
            bool hasOutputs = !m_overloadVariance.m_output.empty();
            size_t returnIndex = 0;

            while (hasOutputs)
            {
                bool isValueType = false;
                bool isContainerType = false;

                for (const auto& methodSignature : m_prototypes)
                {
                    if (returnIndex >= methodSignature.m_outputs.size())
                    {
                        hasOutputs = false;
                        break;
                    }

                    const auto& variableData = methodSignature.m_outputs[returnIndex];

                    ScriptCanvas::Data::Type dataType = variableData->m_datum.GetType();

                    if (ScriptCanvas::Data::IsValueType(dataType))
                    {
                        isValueType = true;
                    }
                    else if (ScriptCanvas::Data::IsContainerType(dataType))
                    {
                        isContainerType = true;
                    }

                    if (isValueType && isContainerType)
                    {
                        break;
                    }
                }

                if (isContainerType && isValueType)
                {
                    m_outputDataTypes[returnIndex] = DynamicDataType::Any;
                }
                else if (isContainerType)
                {
                    m_outputDataTypes[returnIndex] = DynamicDataType::Container;
                }
                else if (isValueType)
                {
                    m_outputDataTypes[returnIndex] = DynamicDataType::Value;
                }

                ++returnIndex;
            }
        }
    }

    void OverloadConfiguration::PopulateOverloadSelection(OverloadSelection& overloadSelection, const DataIndexMapping& inputMapping, const DataIndexMapping& outputMapping) const
    {
        overloadSelection.m_availableIndexes = GenerateAvailableIndexes(inputMapping, outputMapping);

        PopulateOverloadSelection(overloadSelection, overloadSelection.m_availableIndexes);
    }

    void OverloadConfiguration::PopulateOverloadSelection(OverloadSelection& overloadSelection, const AZStd::unordered_set<AZ::u32>& availableIndexes) const
    {
        overloadSelection.m_availableIndexes = availableIndexes;

        overloadSelection.m_inputDataTypes.clear();
        PopulateDataIndexMapping(overloadSelection.m_availableIndexes, ConnectionType::Input, overloadSelection.m_inputDataTypes);

        overloadSelection.m_outputDataTypes.clear();
        PopulateDataIndexMapping(overloadSelection.m_availableIndexes, ConnectionType::Output, overloadSelection.m_outputDataTypes);
    }

    void OverloadConfiguration::PopulateDataIndexMapping(const AZStd::unordered_set<AZ::u32>& availableIndexes, ConnectionType connectionType, DataSetIndexMapping& dataIndexMapping) const
    {
        dataIndexMapping.clear();

        for (const AZ::u32& activeIndex : availableIndexes)
        {
            if (activeIndex >= m_prototypes.size())
            {
                continue;
            }

            const Grammar::FunctionPrototype& prototype = m_prototypes[activeIndex];

            const AZStd::vector<Grammar::VariableConstPtr>* dataSets = &prototype.m_inputs;

            if (connectionType == ConnectionType::Output)
            {
                dataSets = &prototype.m_outputs;
            }

            for (size_t slotIndex = 0; slotIndex < dataSets->size(); ++slotIndex)
            {
                const Data::Type inputType = (*dataSets)[slotIndex]->m_datum.GetType();
                dataIndexMapping[slotIndex].insert(inputType);
            }
        }
    }

    AZStd::unordered_set<AZ::u32> OverloadConfiguration::GenerateAvailableIndexes(const DataIndexMapping& inputMapping, const DataIndexMapping& outputMapping) const
    {
        AZStd::unordered_set<AZ::u32> availableIndexes;

        for (AZ::u32 methodIndex = 0; methodIndex < aznumeric_cast<AZ::u32>(m_prototypes.size()); ++methodIndex)
        {
            const Grammar::FunctionPrototype& prototype = m_prototypes[methodIndex];
            bool acceptPrototype = true;

            for (size_t inputIndex = 0; inputIndex < prototype.m_inputs.size(); ++inputIndex)
            {
                auto concreteInputTypeIter = inputMapping.find(inputIndex);

                if (concreteInputTypeIter == inputMapping.end())
                {
                    continue;
                }

                if (!concreteInputTypeIter->second.IS_EXACTLY_A(prototype.m_inputs[inputIndex]->m_datum.GetType()))
                {
                    acceptPrototype = false;
                    break;
                }
            }

            if (acceptPrototype)
            {
                for (size_t outputIndex = 0; outputIndex < prototype.m_outputs.size(); ++outputIndex)
                {
                    auto concreteOutputTypeIter = outputMapping.find(outputIndex);

                    if (concreteOutputTypeIter == outputMapping.end())
                    {
                        continue;
                    }

                    if (!concreteOutputTypeIter->second.IS_EXACTLY_A(prototype.m_outputs[outputIndex]->m_datum.GetType()))
                    {
                        acceptPrototype = false;
                        break;
                    }
                }
            }

            if (acceptPrototype)
            {
                availableIndexes.insert(methodIndex);
            }
        }

        return availableIndexes;
    }

    ///////////////////////////
    // MethodOverloadContract
    ///////////////////////////

    void OverloadContract::Reflect(AZ::ReflectContext* reflection)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
        {
            serializeContext->Class<OverloadContract, Contract>()
                ->Version(0)
                ;
        }
    }

    void OverloadContract::ConfigureContract(OverloadContractInterface* overloadInterface, size_t index, ConnectionType connectionType)
    {
        m_overloadInterface = overloadInterface;
        m_methodIndex = index;
        m_connectionType = connectionType;
    }

    AZ::Outcome<void, AZStd::string> OverloadContract::OnEvaluate(const Slot& sourceSlot, const Slot& targetSlot) const
    {
        // Check that the type in the target slot is one of the built in math functions
        AZ::Entity* targetSlotEntity{};
        AZ::ComponentApplicationBus::BroadcastResult(targetSlotEntity, &AZ::ComponentApplicationRequests::FindEntity, targetSlot.GetNodeId());
        auto dataNode = targetSlotEntity ? AZ::EntityUtils::FindFirstDerivedComponent<Node>(targetSlotEntity) : nullptr;
        if (dataNode)
        {
            const Data::Type& dataType = dataNode->GetSlotDataType(targetSlot.GetId());

            if (dataType == Data::Type::Invalid())
            {
                if (targetSlot.IsDynamicSlot())
                {
                    if (m_overloadInterface)
                    {
                        const DataTypeSet* dataTypeSet = nullptr;

                        if (m_connectionType == ConnectionType::Input)
                        {
                            dataTypeSet = &(m_overloadInterface->FindPossibleInputTypes(m_methodIndex));
                        }
                        else if (m_connectionType == ConnectionType::Output)
                        {
                            dataTypeSet = &(m_overloadInterface->FindPossibleOutputTypes(m_methodIndex));
                        }

                        if (dataTypeSet)
                        {
                            for (const auto& supportedDataType : (*dataTypeSet))
                            {
                                if (targetSlot.IsTypeMatchFor(supportedDataType))
                                {
                                    return AZ::Success();
                                }
                            }

                            AZStd::string errorMessage = AZStd::string::format("Connection cannot be created between source slot \"%s\" and target slot \"%s\" as the types do not satisfy the type requirement. (%s)\n\rValid types are:\n\r"
                                , sourceSlot.GetName().data()
                                , targetSlot.GetName().data()
                                , RTTI_GetTypeName());

                            for (const auto& type : (*dataTypeSet))
                            {
                                if (Data::IsValueType(type))
                                {
                                    errorMessage.append(AZStd::string::format("%s\n", Data::GetName(type).data()));
                                }
                                else
                                {
                                    errorMessage.append(Data::GetBehaviorClassName(type.GetAZType()));
                                }
                            }

                            return AZ::Failure(errorMessage);
                        }
                        else
                        {
                            return AZ::Failure(AZStd::string("Method Overload Contract is misconfigured, and cannot accept connections"));
                        }
                    }

                    return AZ::Failure(AZStd::string("Method Overload Contract is misconfigured, and cannot accept connections"));
                }
            }
            else
            {
                return EvaluateForType(dataType);
            }
        }

        return AZ::Failure(AZStd::string("Unable to find Node for Target Slot"));
    }

    AZ::Outcome<void, AZStd::string> OverloadContract::OnEvaluateForType(const Data::Type& dataType) const
    {
        if (m_overloadInterface == nullptr)
        {
            return AZ::Failure(AZStd::string("Method Overload Contract is misconfigured, and cannot accept connections"));
        }

        if (dataType != Data::Type::Invalid())
        {
            if (m_connectionType == ConnectionType::Input)
            {
                return m_overloadInterface->IsValidInputType(m_methodIndex, dataType);
            }
            else
            {
                return m_overloadInterface->IsValidOutputType(m_methodIndex, dataType);
            }
        }
        else
        {
            return AZ::Failure(AZStd::string::format("Method Overload cannot type m atch on an Invalid type"));
        }
    }
}
