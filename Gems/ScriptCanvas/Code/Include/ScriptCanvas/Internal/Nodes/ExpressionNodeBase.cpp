/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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

            bool ExpressionNodeBase::VersionConverter(
                [[maybe_unused]] AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& element)
            {
                // The expression text and existing slots are authoritative; discard the old parser cache.
                element.RemoveElementByName(AZ_CRC_CE("m_expressionTree"));
                return true;
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

                m_slotToVariableMap.clear();
                m_slotsByVariables.clear();
                if (!ParseExpression())
                {
                    return;
                }
                const auto& variableList = m_expression.m_variables;

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
                if (extensionId == GetExtensionId() && (!m_isInError || m_format.empty()))
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
                if (m_isInError && m_parseError.empty())
                {
                    const bool signalError = false;
                    ParseFormat(signalError);
                }

                if (m_isInError)
                {
                    InvalidExpressionEvent* invaildExpressionEvent = aznew InvalidExpressionEvent(GetEntityId(), m_parseError);

                    validationResults.AddValidationEvent(invaildExpressionEvent);

                    return false;
                }

                return true;
            }

            void ExpressionNodeBase::ParseFormat(bool signalError)
            {
                if (!ParseExpression())
                {
                    if (signalError && GetGraph())
                    {
                        GetGraph()->ReportError((*this), "Parsing Error", m_parseError);
                    }

                    return;
                }
                else
                {
                    AZStd::unordered_map< AZStd::string, SlotCacheSetup > variableSlotMapping;
                    const auto& variableList = m_expression.m_variables;

                    for (const auto& variable : variableList)
                    {
                        const AZStd::string variableString(variable.data(), variable.size());
                        Slot* slot = GetSlotByName(variableString);

                        if (slot)
                        {
                            SlotCacheSetup& cacheSetup = variableSlotMapping[variableString];

                            cacheSetup.m_previousId = slot->GetId();
                            cacheSetup.m_reference = slot->GetVariableReference();

                            const Datum* datum = FindDatum(slot->GetId());

                            if (datum)
                            {
                                cacheSetup.m_defaultValue.ReconfigureDatumTo((*datum));
                            }
                        }
                    }

                    m_parsingFormat = true;
                    AZStd::vector<const Slot*> eraseSlots = GetAllSlotsByDescriptor(SlotDescriptors::DataIn());

                    for (const Slot* eraseSlot : eraseSlots)
                    {
                        // If we have the name in our mapping. We want to keep its connections since we're going to
                        // recreate it.
                        size_t newMapping = variableSlotMapping.count(eraseSlot->GetName());
                        bool signalRemoval = newMapping == 0;

                        SlotId slotId = eraseSlot->GetId();

                        RemoveSlot(slotId, signalRemoval);
                    }

                    m_slotToVariableMap.clear();
                    m_slotsByVariables.clear();

                    // Start our counting from our raw variable position ignoring any other slots that might have been added.
                    int slotOrder = static_cast<int>(GetSlots().size()) -1;

                    for (const auto& variableName : variableList)
                    {
                        auto mappingIter = variableSlotMapping.find(variableName);

                        bool isNewSlot = (mappingIter == variableSlotMapping.end());

                        DataSlotConfiguration configuration(Data::Type::Number());
                        ConfigureSlot(variableName, configuration);
                        if (!isNewSlot)
                        {
                            configuration.m_slotId = mappingIter->second.m_previousId;
                            configuration.ConfigureDatum(AZStd::move(mappingIter->second.m_defaultValue));
                        }
                        SlotId slotId = InsertSlot(slotOrder, configuration, isNewSlot);

                        Slot* slot = GetSlot(slotId);

                        if (slot && mappingIter != variableSlotMapping.end())
                        {
                            if (mappingIter->second.m_reference.IsValid())
                            {
                                slot->SetVariableReference(mappingIter->second.m_reference);
                            }
                        }

                        m_slotToVariableMap[slotId] = variableName;
                        m_slotsByVariables[variableName] = slotId;

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

            bool ExpressionNodeBase::ParseExpression()
            {
                const auto tokens = ScriptCanvas::Expression::Tokenize(m_format, true);
                auto parsed = tokens ? ScriptCanvas::Expression::ShuntingYard(tokens.m_value)
                                     : ScriptCanvas::Expression::Result<ScriptCanvas::Expression::Program>{ {}, tokens.m_error };
                if (parsed)
                {
                    for (const auto& variable : parsed.m_value.m_variables)
                    {
                        const auto* slot = GetSlotByName(variable);
                        if (slot && (slot->IsExecution() || slot->IsOutput()))
                        {
                            parsed.m_error = "Expression uses a reserved slot name: " + variable;
                            break;
                        }
                    }
                }
                m_isInError = !parsed;
                m_parseError = parsed.m_error;
                if (parsed)
                {
                    m_expression = AZStd::move(parsed.m_value);
                }
                return !m_isInError;
            }

            AZStd::string ExpressionNodeBase::GetExpressionSeparator() const
            {
                return "";
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
