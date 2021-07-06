/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ExpressionNodeBase.h"

#include <AzCore/std/algorithm.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Serialization/Utils.h>

#include <ScriptCanvas/Core/Graph.h>

#include <ScriptCanvas/Debugger/ValidationEvents/DataValidation/InvalidExpressionEvent.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Internal
        {
            ///////////////////////
            // ExpressionNodeBase
            ///////////////////////         

            ExpressionNodeBase::ExpressionNodeBase()
                : m_isInError(false)
            {

            }

            AZ::Outcome<DependencyReport, void> ExpressionNodeBase::GetDependencies() const
            {
                return AZ::Success(DependencyReport{});
            }

            AZStd::string ExpressionNodeBase::GetRawFormat() const
            {
                return m_format;
            }

            const AZStd::unordered_map<AZStd::string, SlotId>& ExpressionNodeBase::GetSlotsByName() const
            {
                return m_slotsByVariables;
            }

            void ExpressionNodeBase::OnInit()
            {
                m_stringInterface.SetPropertyReference(&m_format);
                m_stringInterface.RegisterListener(this);

                const auto& variableList = m_expressionTree.GetVariables();

                for (const auto& variableName : variableList)
                {
                    Slot* variableSlot = GetSlotByName(variableName);

                    if (variableSlot)
                    {
                        m_slotToVariableMap[variableSlot->GetId()] = variableName;
                        m_slotsByVariables[variableName] = variableSlot->GetId();
                    }
                }
            }

            void ExpressionNodeBase::OnPostActivate()
            {
                for (auto variableListing : m_slotToVariableMap)
                {
                    Slot* variableSlot = GetSlot(variableListing.first);

                    if (variableSlot && !HasConnectedNodes((*variableSlot)))
                    {
                        const Datum* datum = FindDatum(variableSlot->GetId());

                        if (datum)
                        {
                            PushVariable(variableListing.second, (*datum));
                        }
                    }
                }
            }

            void ExpressionNodeBase::ConfigureVisualExtensions()
            {
                {
                    VisualExtensionSlotConfiguration visualExtensions(VisualExtensionSlotConfiguration::VisualExtensionType::ExtenderSlot);

                    visualExtensions.m_name = "Add Input";
                    visualExtensions.m_tooltip = "Adds an input to the current expression format";
                    visualExtensions.m_displayGroup = GetDisplayGroup();
                    visualExtensions.m_connectionType = ConnectionType::Input;

                    visualExtensions.m_identifier = GetExtensionId();

                    RegisterExtension(visualExtensions);
                }

                {
                    VisualExtensionSlotConfiguration visualExtensions(VisualExtensionSlotConfiguration::VisualExtensionType::PropertySlot);

                    visualExtensions.m_name = "";
                    visualExtensions.m_tooltip = "";
                    visualExtensions.m_displayGroup = GetDisplayGroup();
                    visualExtensions.m_connectionType = ConnectionType::Input;

                    visualExtensions.m_identifier = GetPropertyId();

                    RegisterExtension(visualExtensions);
                }
            }

            bool ExpressionNodeBase::CanDeleteSlot([[maybe_unused]] const SlotId& slotId) const
            {
                return m_handlingExtension;
            }

            SlotId ExpressionNodeBase::HandleExtension(AZ::Crc32 extensionId)
            {
                if (extensionId == GetExtensionId() && !m_isInError)
                {
                    int value = 0;
                    AZStd::string name = "Value";

                    Slot* slot = GetSlotByName(name);

                    while (slot)
                    {
                        ++value;
                        name = AZStd::string::format("Value_%i", value);
                        slot = GetSlotByName(name);
                    }

                    if (!m_format.empty())
                    {
                        m_format.append(GetExpressionSeparator());
                    }

                    m_format.append("{");
                    m_format.append(name);
                    m_format.append("}");

                    m_stringInterface.SignalDataChanged();

                    slot = GetSlotByName(name);

                    if (slot)
                    {
                        m_handlingExtension = true;
                        return slot->GetId();
                    }
                }

                return SlotId();
            }

            void ExpressionNodeBase::ExtensionCancelled([[maybe_unused]] AZ::Crc32 extensionId)
            {
                // Remove the seperator operator if we added it.
                if (!m_format.empty())
                {
                    AZStd::string seperator = GetExpressionSeparator();

                    m_format = m_format.substr(0, m_format.length() - seperator.size());
                    m_stringInterface.SignalDataChanged();
                }

                m_handlingExtension = false;
            }

            void ExpressionNodeBase::FinalizeExtension([[maybe_unused]] AZ::Crc32 extensionId)
            {
                m_handlingExtension = false;
            }

            NodePropertyInterface* ExpressionNodeBase::GetPropertyInterface(AZ::Crc32 propertyId)
            {
                if (propertyId == GetPropertyId())
                {
                    return &m_stringInterface;
                }

                return nullptr;
            }

            void ExpressionNodeBase::OnSlotRemoved(const SlotId& slotId)
            {
                if (m_parsingFormat)
                {
                    return;
                }

                auto mapIter = m_slotToVariableMap.find(slotId);

                if (mapIter != m_slotToVariableMap.end())
                {
                    AZStd::string name = AZStd::string::format("{%s}", mapIter->second.c_str());
                    AZStd::size_t firstInstance = m_format.find(name);

                    if (firstInstance == AZStd::string::npos)
                    {
                        return;
                    }

                    m_format.erase(firstInstance, name.size());

                    if (!m_handlingExtension)
                    {
                        m_stringInterface.SignalDataChanged();
                    }
                }
            }

            bool ExpressionNodeBase::OnValidateNode(ValidationResults& validationResults)
            {
                // This means we were loaded up with an error, but we haven't re-parsed so we
                // don't know what the error is. Force a reparse here to ensure
                // we report errors correctly.
                if (m_isInError && m_parseError.IsValidExpression())
                {
                    const bool signalError = false;
                    ParseFormat(signalError);
                }

                if (!m_parseError.IsValidExpression())
                {
                    InvalidExpressionEvent* invaildExpressionEvent = aznew InvalidExpressionEvent(GetEntityId(), m_parseError.m_errorString);

                    validationResults.AddValidationEvent(invaildExpressionEvent);

                    return false;
                }

                return true;
            }

            void ExpressionNodeBase::ParseFormat(bool signalError)
            {
                ExpressionEvaluation::ParseOutcome parseOutcome = ParseExpression(m_format);

                if (!parseOutcome.IsSuccess())
                {
                    m_isInError = true;
                    m_parseError = parseOutcome.GetError();

                    if (signalError)
                    {
                        AZStd::string parseError = m_parseError.m_errorString;
                        GetGraph()->ReportError((*this), "Parsing Error", parseError);
                    }

                    return;
                }
                else
                {
                    m_isInError = false;
                    m_parseError.Clear();

                    m_parsingFormat = true;

                    AZStd::unordered_map< AZStd::string, SlotCacheSetup > variableSlotMapping;

                    m_expressionTree = parseOutcome.GetValue();

                    const AZStd::vector< AZStd::string >& variableList = m_expressionTree.GetVariables();

                    for (const AZStd::string& variableString : variableList)
                    {
                        Slot* slot = GetSlotByName(variableString);

                        if (slot)
                        {
                            if (slot->IsExecution() || slot->IsOutput())
                            {
                                m_isInError = true;
                                m_parseError = ExpressionEvaluation::ParsingError();
                                m_parseError.m_offsetIndex = m_format.find_first_of(variableString);

                                const auto& totalSlots = GetSlots();

                                AZStd::string reservedNames;

                                for (const auto& slot2 : totalSlots)
                                {
                                    if (slot2.IsExecution() || slot2.IsOutput())
                                    {
                                        if (!reservedNames.empty())
                                        {
                                            reservedNames.append(", ");
                                        }

                                        reservedNames.append(slot2.GetName());
                                    }
                                }

                                m_parseError.m_errorString = AZStd::string::format("Using one of the reserved slot names \"%s\" in expression at position %zu", reservedNames.c_str(), m_parseError.m_offsetIndex);

                                AZStd::string parseError = m_parseError.m_errorString;
                                GetGraph()->ReportError((*this), "Parsing Error", parseError);

                                return;
                            }

                            SlotCacheSetup& cacheSetup = variableSlotMapping[variableString];

                            cacheSetup.m_previousId = slot->GetId();
                            cacheSetup.m_displayType = slot->GetDisplayType();
                            cacheSetup.m_reference = slot->GetVariableReference();

                            const Datum* datum = FindDatum(slot->GetId());

                            if (datum)
                            {
                                cacheSetup.m_defaultValue.ReconfigureDatumTo((*datum));
                            }
                        }
                    }

                    AZStd::vector<const Slot*> eraseSlots = GetAllSlotsByDescriptor(SlotDescriptors::DataIn());

                    for (const Slot* eraseSlot : eraseSlots)
                    {
                        // If we have the name in our mapping. We want to keep its connections since we're going to
                        // recreate it.
                        size_t newMapping = variableSlotMapping.count(eraseSlot->GetName());
                        bool signalRemoval = newMapping == 0;

                        SlotId slotId = eraseSlot->GetId();

                        RemoveSlot(slotId, signalRemoval);

                        if (signalRemoval)
                        {
                            m_slotToVariableMap.erase(slotId);
                        }
                    }

                    // Start our counting from our raw variable position ignoring any other slots that might have been added.
                    int slotOrder = static_cast<int>(GetSlots().size()) -1;

                    for (const auto& variableName : variableList)
                    {
                        const AZStd::vector<AZ::Uuid>& supportedTypes = m_expressionTree.GetSupportedTypes(variableName);

                        if (supportedTypes.empty())
                        {
                            // Just bypass any slots that have no valid types for now. We can sort it out later.
                            continue;
                        }

                        auto mappingIter = variableSlotMapping.find(variableName);

                        bool isNewSlot = (mappingIter == variableSlotMapping.end());

                        SlotId slotId;

                        // If we have a single data type. Create a typed data slot.
                        if (supportedTypes.size() == 1)
                        {
                            ScriptCanvas::Data::Type dataType = Data::FromAZType(supportedTypes.front());

                            if (dataType.IsValid())
                            {
                                DataSlotConfiguration dataSlotConfiguration(dataType);

                                ConfigureSlot(variableName, dataSlotConfiguration);

                                if (mappingIter != variableSlotMapping.end())
                                {
                                    dataSlotConfiguration.m_slotId = mappingIter->second.m_previousId;                                    

                                    if (dataType != mappingIter->second.m_displayType && mappingIter->second.m_displayType.IsValid())
                                    {
                                        AZ_Error("ScriptCanvas", false, "Variable supported type changed. Need to invalidate all connections. Currently unsupported.");
                                    }

                                    dataSlotConfiguration.ConfigureDatum(AZStd::move(mappingIter->second.m_defaultValue));
                                }

                                slotId = InsertSlot(slotOrder, dataSlotConfiguration, isNewSlot);
                            }
                        }
                        // Otherwise if we can support multiple types make a dynamic slot.
                        else
                        {
                            DynamicDataSlotConfiguration dynamicSlotConfiguration;

                            ConfigureSlot(variableName, dynamicSlotConfiguration);

                            dynamicSlotConfiguration.m_dynamicDataType = DynamicDataType::Any;

                            AZStd::vector< ScriptCanvas::Data::Type > dataTypes;
                            dataTypes.reserve(supportedTypes.size());

                            for (const auto& supportedType : supportedTypes)
                            {
                                dataTypes.emplace_back(ScriptCanvas::Data::FromAZType(supportedType));
                            }

                            dynamicSlotConfiguration.m_contractDescs = AZStd::vector<ScriptCanvas::ContractDescriptor>
                            {
                                // Restricted Type Contract
                                { [dataTypes]() { return aznew ScriptCanvas::RestrictedTypeContract(dataTypes); } }
                            };

                            if (mappingIter != variableSlotMapping.end())
                            {
                                dynamicSlotConfiguration.m_slotId = mappingIter->second.m_previousId;
                                dynamicSlotConfiguration.m_displayType = mappingIter->second.m_displayType;
                            }

                            slotId = InsertSlot(slotOrder, dynamicSlotConfiguration, isNewSlot);
                        }

                        Slot* slot = GetSlot(slotId);

                        if (slot && mappingIter != variableSlotMapping.end())
                        {
                            if (mappingIter->second.m_reference.IsValid())
                            {
                                slot->SetVariableReference(mappingIter->second.m_reference);
                            }
                        }

                        m_slotToVariableMap[slotId] = variableName;

                        ++slotOrder;
                    }

                    m_parsingFormat = false;
                }
            }

            void ExpressionNodeBase::SignalFormatChanged()
            {
                // Adding and removing slots within the ChangeNotify handler causes problems due to the property grid's
                // rows changing. To get around that problem we queue the parsing of the string format to happen
                // on the next system tick.
                AZ::SystemTickBus::QueueFunction([this]() {
                    m_stringInterface.SignalDataChanged();
                });
            }

            ExpressionEvaluation::ParseOutcome ExpressionNodeBase::ParseExpression([[maybe_unused]] const AZStd::string& formatString)
            {
                AZ_Assert(false , "Implementing node must override OnResult.");

                ExpressionEvaluation::ParsingError error;
                error.m_errorString = "Unable to parse string due to unknown parsing parameters";

                return AZ::Failure(error);
            }

            AZStd::string ExpressionNodeBase::GetExpressionSeparator() const
            {
                return "";
            }

            void ExpressionNodeBase::PushVariable(const AZStd::string& variableName, const Datum& datum)
            {
                auto& variableValue = m_expressionTree.ModVariable(variableName);
                variableValue = datum.ToAny();
            }

            void ExpressionNodeBase::OnPropertyChanged()
            {
                ParseFormat();
            }

            void ExpressionNodeBase::ConfigureSlot(const AZStd::string& variableName, SlotConfiguration& slotConfiguration)
            {
                AZStd::string tooltip = AZStd::string::format("Value which replaces instances of {%s} in the resulting expression.", variableName.c_str());

                slotConfiguration.m_name = variableName;
                slotConfiguration.m_toolTip = tooltip;
                slotConfiguration.m_displayGroup = GetDisplayGroup();

                slotConfiguration.SetConnectionType(ConnectionType::Input);

                slotConfiguration.m_addUniqueSlotByNameAndType = true;
            }
        }
    }
}
