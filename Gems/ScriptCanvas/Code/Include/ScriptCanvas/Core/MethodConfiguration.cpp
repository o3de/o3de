/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MethodConfiguration.h"

#include <AzCore/RTTI/AttributeReader.h>
#include <ScriptCanvas/Utils/BehaviorContextUtils.h>

#include <ScriptCanvas/Core/Contracts/MethodOverloadContract.h>
#include <ScriptCanvas/Libraries/Core/Method.h>
#include "../../GraphCanvas/Code/Source/Translation/TranslationBus.h"

namespace ScriptCanvas
{
    namespace MethodHelper
    {
        AZStd::string GetArgumentName(size_t argIndex, const AZ::BehaviorMethod& method, const AZ::BehaviorClass* /*bcClass*/, AZStd::string_view replaceTypeName)
        {
            if (const AZ::BehaviorParameter * argument = method.GetArgument(argIndex))
            {
                const AZStd::string argumentTypeName = replaceTypeName.empty()
                    ? AZ::BehaviorContextHelper::IsStringParameter(*argument)
                    ? Data::GetName(Data::Type::String())
                    : Data::GetName(Data::FromAZType(argument->m_typeId))
                    : AZStd::string(replaceTypeName);

                const AZStd::string* argumentNamePtr = method.GetArgumentName(argIndex);

                return argumentNamePtr && !argumentNamePtr->empty()
                    ? *argumentNamePtr
                    : (AZStd::string::format("%s:%2zu", argumentTypeName.data(), argIndex));
            }

            return {};
        }

        AZStd::pair<AZStd::string, AZStd::string> GetArgumentNameAndToolTip(const MethodConfiguration& config, size_t argumentIndex)
        {
            const AZStd::string argName = GetArgumentName(argumentIndex, config.m_method, config.m_class, "");
            const AZStd::string* argumentTooltipPtr = config.m_method.GetArgumentToolTip(argumentIndex);
            return { argName, (argumentTooltipPtr ? *argumentTooltipPtr : AZStd::string{}) };
        }

        void AddDataOutputSlot(const MethodOutputConfig& outputConfig)
        {
            const AZ::BehaviorMethod& method = outputConfig.config.m_method;
            const AZ::BehaviorParameter* result = method.GetResult();
            AZStd::vector<AZ::TypeId> unpackedTypes = BehaviorContextUtils::GetUnpackedTypes(result->m_typeId);

            for (size_t resultIndex = 0; resultIndex < unpackedTypes.size(); ++resultIndex)
            {
                const Data::Type outputType = (unpackedTypes.size() == 1 && AZ::BehaviorContextHelper::IsStringParameter(*result)) ? Data::Type::String() : Data::FromAZType(unpackedTypes[resultIndex]);

                AZStd::string resultSlotName(Data::GetName(outputType));

                AZStd::string className = outputConfig.config.m_className ? *outputConfig.config.m_className : "";
                if (className.empty())
                {
                    className = outputConfig.config.m_prettyClassName;
                }

                GraphCanvas::TranslationKey key;
                key << "BehaviorClass" << className << "methods" << *outputConfig.config.m_lookupName << "results" << resultIndex << "details";

                GraphCanvas::TranslationRequests::Details details;
                GraphCanvas::TranslationRequestBus::BroadcastResult(details, &GraphCanvas::TranslationRequests::GetDetails, key, details);

                if (!details.m_name.empty())
                {
                    resultSlotName = details.m_name;
                }

                SlotId addedSlotId;

                if (outputConfig.isReturnValueOverloaded)
                {
                    DynamicDataSlotConfiguration slotConfiguration;

                    slotConfiguration.m_dynamicDataType = outputConfig.methodNode->GetOverloadedOutputType(resultIndex);

                    switch (slotConfiguration.m_dynamicDataType)
                    {
                    case DynamicDataType::Container:
                        slotConfiguration.m_name = outputConfig.outputNamePrefix + "Container";
                        break;
                    case DynamicDataType::Any:
                    case DynamicDataType::Value:
                        slotConfiguration.m_name = outputConfig.outputNamePrefix + "Value";
                        break;
                    default:
                        break;
                    }

                    slotConfiguration.m_contractDescs = AZStd::vector<ScriptCanvas::ContractDescriptor>
                    {
                        // Restricted Type Contract
                        { []() { return aznew ScriptCanvas::OverloadContract(); } }
                    };

                    slotConfiguration.SetConnectionType(ConnectionType::Output);

                    addedSlotId = outputConfig.methodNode->AddSlot(slotConfiguration);
                    AZ_Error("ScriptCanvas", addedSlotId.IsValid(), "Failed to add method data output slot");
                }
                else
                {
                    DataSlotConfiguration slotConfiguration;
                    slotConfiguration.m_name = outputConfig.outputNamePrefix + resultSlotName;
                    slotConfiguration.SetType(outputType);
                    slotConfiguration.SetConnectionType(ConnectionType::Output);
                    AZ::BranchOnResultInfo branchOnResultInfo;
                    if (AZ::ReadAttribute(branchOnResultInfo, AZ::ScriptCanvasAttributes::BranchOnResult, method.m_attributes))
                    {
                        slotConfiguration.m_isVisible = branchOnResultInfo.m_returnResultInBranches;
                    }

                    addedSlotId = outputConfig.methodNode->AddSlot(slotConfiguration);
                    AZ_Error("ScriptCanvas", addedSlotId.IsValid(), "Failed to add method data output slot");
                }

                if (outputConfig.resultSlotIdsOut)
                {
                    outputConfig.resultSlotIdsOut->push_back(addedSlotId);
                }
            }
        }

        void AddMethodOutputSlot(const MethodOutputConfig& outputConfig)
        {
            const AZ::BehaviorMethod& method = outputConfig.config.m_method;

            // check for checked operation
            if (auto checkOpAttribute = AZ::FindAttribute(AZ::ScriptCanvasAttributes::CheckedOperation, method.m_attributes))
            {
                AZ_Error("ScriptCanvas"
                    , AZ::FindAttribute(AZ::ScriptCanvasAttributes::BranchOnResult, method.m_attributes) == nullptr
                    , "A method can be a checked operation or a branch on result expression but not currently both");

                AZ::CheckedOperationInfo checkedOpInfo;

                if (!AZ::AttributeReader(nullptr, checkOpAttribute).Read<AZ::CheckedOperationInfo>(checkedOpInfo))
                {
                    AZ_Error("ScriptCanvas", false, "Failed to read check operation info");
                    return;
                }

                outputConfig.methodNode->AddSlot(ExecutionSlotConfiguration(checkedOpInfo.m_successCaseName, ConnectionType::Output));
                outputConfig.methodNode->AddSlot(ExecutionSlotConfiguration(checkedOpInfo.m_failureCaseName, ConnectionType::Output));
            }
            else if (auto branchOpAttribute = AZ::FindAttribute(AZ::ScriptCanvasAttributes::BranchOnResult, method.m_attributes))
            {
                if (!method.HasResult())
                {
                    AZ_Error("ScriptCanvas", false, "Method must have result to get branched");
                    return;
                }
                AZ::BranchOnResultInfo branchOpInfo;
                if (!AZ::AttributeReader(nullptr, branchOpAttribute).Read<AZ::BranchOnResultInfo>(branchOpInfo))
                {
                    AZ_Error("ScriptCanvas", false, "Failed to read branch on result info");
                    return;
                }

                outputConfig.methodNode->AddSlot(ExecutionSlotConfiguration(branchOpInfo.m_trueName, ConnectionType::Output));
                outputConfig.methodNode->AddSlot(ExecutionSlotConfiguration(branchOpInfo.m_falseName, ConnectionType::Output));
            }
            else
            {
                outputConfig.methodNode->AddSlot(CommonSlots::GeneralOutSlot());
            }

            if (method.HasResult())
            {
                AddDataOutputSlot(outputConfig);
            }

            if (outputConfig.resultSlotIdsOut && outputConfig.resultSlotIdsOut->empty())
            {
                outputConfig.resultSlotIdsOut->push_back(SlotId{});
            }
        }

        void SetSlotToDefaultValue(Node& node, const SlotId& slotId, const MethodConfiguration& config, size_t argumentIndex)
        {
            if (slotId.IsValid())
            {
                if (const auto defaultValue = config.m_method.GetDefaultValue(argumentIndex))
                {
                    ModifiableDatumView datumView;
                    node.FindModifiableDatumView(slotId, datumView);

                    if (datumView.IsValid() && Data::IsValueType(datumView.GetDataType()))
                    {
                        datumView.AssignToDatum(AZStd::move(Datum(defaultValue->m_value)));
                    }
                }
            }
        }

        DataSlotConfiguration ToInputSlotConfig(const MethodConfiguration& config, size_t argumentIndex)
        {
            const AZ::BehaviorParameter* argumentPtr = config.m_method.GetArgument(argumentIndex);
            AZ_Assert(argumentPtr, "Method: %s had a null argument at index: %d", config.m_lookupName->data(), argumentIndex);
            const auto& argument = *argumentPtr;

            auto nameAndToolTip = GetArgumentNameAndToolTip(config, argumentIndex);

            DataSlotConfiguration slotConfiguration;
            slotConfiguration.m_name = nameAndToolTip.first;
            slotConfiguration.m_toolTip = nameAndToolTip.second;
            slotConfiguration.SetConnectionType(ConnectionType::Input);

            AZ::ScriptCanvasAttributes::HiddenIndices hiddenIndices;
            AZ::ReadAttribute(hiddenIndices, AZ::ScriptCanvasAttributes::HiddenParameterIndex, config.m_method.m_attributes);
            slotConfiguration.m_isVisible = AZStd::find(hiddenIndices.begin(), hiddenIndices.end(), argumentIndex) == hiddenIndices.end();

            // Create a slot with a default value
            if (argument.m_typeId == azrtti_typeid<AZ::EntityId>())
            {
                slotConfiguration.ConfigureDatum(Datum(Data::Type::EntityID(), Datum::eOriginality::Copy, &ScriptCanvas::GraphOwnerId, AZ::Uuid::CreateNull()));
            }
            else
            {
                Data::Type scType = !AZ::BehaviorContextHelper::IsStringParameter((argument)) ? Data::FromAZType(argument.m_typeId) : Data::Type::String();
                slotConfiguration.ConfigureDatum(Datum(scType, Datum::eOriginality::Copy, nullptr, AZ::Uuid::CreateNull()));
            }

            return slotConfiguration;
        }

    }
}


