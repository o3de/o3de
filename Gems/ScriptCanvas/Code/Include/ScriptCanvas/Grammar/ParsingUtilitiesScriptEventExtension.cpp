/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/Grammar/ParsingUtilitiesScriptEventExtension.h>
#include <ScriptCanvas/Libraries/Core/FunctionDefinitionNode.h>
#include <ScriptCanvas/Variable/VariableData.h>
#include <ScriptEvents/ScriptEventsMethod.h>

namespace ParsingUtilitiesScriptEventExtensionCpp
{
    using namespace ScriptEvents;
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes::Core;

    AZ::Outcome<ScriptEvents::Method, AZStd::string> TranslateToScriptEventMethod(const FunctionDefinitionNode& node)
    {
        if (!node.IsExecutionEntry())
        {
            return AZ::Failure(AZStd::string("Only User-In nodes (not User-Out nodes) are allowed for ScriptEvents."));
        }

        ScriptEvents::Method method;

        auto parameterSlots = node.GetSlotsByType(CombinedSlotType::DataOut);
        for (auto& slot : parameterSlots)
        {
            auto& parameter = method.NewParameter();
            parameter.GetNameProperty().Set(slot->GetName());
            auto azType = slot->GetDataType().GetAZType();
            parameter.GetTypeProperty().Set(azType);
            // \todo add tool tip
        }
                
        auto methodValidateOutcome = method.Validate();
        if (!methodValidateOutcome.IsSuccess())
        {
            return AZ::Failure(methodValidateOutcome.GetError());
        }

        return AZ::Success(method);
    }
}

namespace ScriptCanvas::ScriptEventGrammar
{
    using namespace ScriptCanvas;
    using namespace ParsingUtilitiesScriptEventExtensionCpp;

    FunctionNodeToScriptEventResult ToScriptEvent(const Node& node)
    {
        FunctionNodeToScriptEventResult result{ false, &node, {}, {} };

        if (!azrtti_istypeof<Nodes::Core::FunctionDefinitionNode>(node))
        {
            result.m_parseErrors.push_back("All ScriptEvent graph nodes must be FunctionDefinitionNodes");
            return result;
        }

        auto functionDefinitionNode = azrtti_cast<const Nodes::Core::FunctionDefinitionNode*>(&node);
        auto outcome = TranslateToScriptEventMethod(*functionDefinitionNode);
        if (outcome.IsSuccess())
        {
            result.m_method = outcome.GetValue();
        }
        else
        {
            result.m_parseErrors.push_back(outcome.GetError());
        }

        result.m_isScriptEvent = result.m_parseErrors.empty();
        return result;
    }

    GraphToScriptEventsResult ToScriptEvents(Graph& graph)
    {
        GraphToScriptEventsResult result;
        
        {
            auto nodes = graph.GetNodesOfType<Node>();
            bool anyNodeWasInvalid = false;

            for (auto node : nodes)
            {
                result.m_nodeResults.push_back(ToScriptEvent(*node));
                anyNodeWasInvalid = anyNodeWasInvalid || !result.m_nodeResults.back().m_isScriptEvent;
            }

            if (anyNodeWasInvalid)
            {
                result.m_parseErrors.push_back("At least one node failed to parse as an event.");
            }
        }

        // get name, tool tip, address type from variables
        auto variableData = graph.GetVariableDataConst();
        if (!variableData)
        {
            result.m_parseErrors.push_back("Missing variable data in graph. Parsing can not continue.");
            return result;
        }

        auto assignStringProperty = [&](AZStd::string_view variableName, ScriptEventData::VersionedProperty& property)
        {
            auto stringVariable = variableData->FindVariable(variableName);

            if (!stringVariable
            || stringVariable->GetDataType() != Data::Type::String()
            || stringVariable->GetDatum()
            || !stringVariable->GetDatum()->GetAs<Data::StringType>())
            {
                result.m_parseErrors.push_back
                    ( AZStd::string::format
                        ( "Missing valid variable by name of '%.*s' and type 'String' in graph.", AZ_STRING_ARG(variableName)));
            }

            property.Set(*stringVariable->GetDatum()->GetAs<Data::StringType>());
        };

        assignStringProperty("Name", result.m_event.GetNameProperty());
        assignStringProperty("Category", result.m_event.GetCategoryProperty());
        assignStringProperty("Tooltip", result.m_event.GetTooltipProperty());

        if (auto addressTypeVariable = variableData->FindVariable("Address Type"))
        {
            auto azType = addressTypeVariable->GetDataType().GetAZType();
            result.m_event.GetAddressTypeProperty().Set(azType);
        }
        else
        {
            result.m_parseErrors.push_back
                ( AZStd::string::format
                    ("Missing variable by name of 'Address Type', which can be invalid or any allowable ScriptEvent data type."));
        }

        for (auto& nodeResult : result.m_nodeResults)
        {
            if (nodeResult.m_isScriptEvent)
            {
                result.m_event.NewMethod() = nodeResult.m_method;
            }
        }

        if (auto eventOutcome = result.m_event.Validate(); !eventOutcome.IsSuccess())
        {
            result.m_parseErrors.push_back(eventOutcome.GetError());
        }

        result.m_isScriptEvents = result.m_parseErrors.empty();
        return result;
    }
}
