/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/Core/GraphSerialization.h>
#include <ScriptCanvas/Grammar/ParsingUtilitiesScriptEventExtension.h>
#include <ScriptCanvas/Libraries/Core/FunctionDefinitionNode.h>
#include <ScriptCanvas/Variable/VariableBus.h>
#include <ScriptCanvas/Variable/VariableData.h>
#include <ScriptEvents/ScriptEventsMethod.h>

namespace ParsingUtilitiesScriptEventExtensionCpp
{
    AZ::Outcome<ScriptEvents::Method, AZStd::string> TranslateToScriptEventMethod
        ( const ScriptCanvas::Nodes::Core::FunctionDefinitionNode& node)
    {
        using namespace ScriptEvents;
        using namespace ScriptCanvas;
        using namespace ScriptCanvas::Nodes::Core;

        ScriptEvents::Method method;
        method.GetNameProperty().Set(node.GetDisplayName());

        auto parameterSlots = node.GetSlotsByType(CombinedSlotType::DataOut);
        for (auto& slot : parameterSlots)
        {
            auto& parameter = method.NewParameter();
            parameter.GetNameProperty().Set(slot->GetName());
            auto azType = slot->GetDataType().GetAZType();
            parameter.GetTypeProperty().Set(azType);
        }
                        
        auto methodValidateOutcome = method.Validate();
        if (!methodValidateOutcome.IsSuccess())
        {
            return AZ::Failure(methodValidateOutcome.GetError());
        }

        auto slots = node.GetSlotsByType(CombinedSlotType::ExecutionOut);
        if (slots.size() > 1)
        {
            return AZ::Failure(AZStd::string("Event nodes must have one or zero Execution Out Slots"));
        }

        if (!slots.empty())
        {
            auto resultNodes = node.GetConnectedNodes(*slots.front());
            if (!resultNodes.empty())
            {
                if (resultNodes.size() > 1)
                {
                    return AZ::Failure(AZStd::string("Event nodes can only have one connected return value Node"));
                }

                auto returnValueNode = azrtti_cast<const Nodes::Core::FunctionDefinitionNode*>(resultNodes.front().first);
                if (!returnValueNode || !returnValueNode->IsExecutionExit())
                {
                    return AZ::Failure(AZStd::string("Event nodes can only be connected to a FunctionDefinitionNode that defines a return value"));
                }

                auto returnValueSlots = returnValueNode->GetSlotsByType(CombinedSlotType::DataIn);
                if (returnValueSlots.size() != 1)
                {
                    return AZ::Failure(AZStd::string("Event nodes can only be connected to a FunctionDefinitionNode that defines a single return value slot."));
                }

                for (auto& slot : returnValueSlots)
                {
                    const auto azTypeReturn = slot->GetDataType().GetAZType();
                    method.GetReturnTypeProperty().Set(azTypeReturn);
                }
            }
        }

        return AZ::Success(method);
    }
}

namespace ScriptCanvas::ScriptEventGrammar
{
    GraphToScriptEventsResult ParseMinimumScriptEventArtifacts(const Graph& graph)
    {
        GraphToScriptEventsResult result;

        // get name, tool tip, category from variables
        auto variableData = graph.GetVariableDataConst();
        if (!variableData)
        {
            result.m_parseErrors.push_back("Missing variable data in graph. Parsing can not continue.");
            return result;
        }

        auto assignStringProperty = [&](AZStd::string_view variableName, ScriptEventData::VersionedProperty& property)
        {
            auto stringVariable = variableData->FindVariable(variableName);

            if (!stringVariable || stringVariable->GetDataType() != Data::Type::String())
            {
                result.m_parseErrors.push_back
                    ( AZStd::string::format
                    ( "Missing valid variable by name of '%.*s' and type 'String' in graph.", AZ_STRING_ARG(variableName)));
            }
            else
            {
                property.Set(*stringVariable->GetDatum()->GetAs<Data::StringType>());
            }
        };

        assignStringProperty("Name", result.m_event.GetNameProperty());
        assignStringProperty("Category", result.m_event.GetCategoryProperty());
        assignStringProperty("Tooltip", result.m_event.GetTooltipProperty());
        result.m_isScriptEvents = result.m_parseErrors.empty();
        return result;
    }

    FunctionNodeToScriptEventResult ParseScriptEvent(const Node& node)
    {
        using namespace ParsingUtilitiesScriptEventExtensionCpp;

        FunctionNodeToScriptEventResult result{ false, &node, {}, {} };

        auto functionDefinitionNode = azrtti_cast<const Nodes::Core::FunctionDefinitionNode*>(&node);

        if (!functionDefinitionNode)
        {
            return result;
        }

        if (!functionDefinitionNode->IsExecutionEntry())
        {
            return result;
        }

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

    GraphToScriptEventsResult ParseScriptEventsDefinition(const Graph& graph)
    {
        // get name, tool tip, category from variables
        GraphToScriptEventsResult result = ParseMinimumScriptEventArtifacts(graph);
        
        {
            auto nodes = graph.GetNodesOfType<Node>();
            bool anyNodeWasInvalid = false;

            for (auto node : nodes)
            {
                result.m_nodeResults.push_back(ParseScriptEvent(*node));
                anyNodeWasInvalid = anyNodeWasInvalid || !result.m_nodeResults.back().m_parseErrors.empty();
            }

            if (anyNodeWasInvalid)
            {
                result.m_parseErrors.push_back("At least one node failed to parse as an event.");
            }
        }

        // get address type from variables
        auto variableData = graph.GetVariableDataConst();
        if (!variableData)
        {
            result.m_parseErrors.push_back("Missing variable data in graph. Parsing can not continue.");
            return result;
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

        AZ::IO::ByteContainerStream stream(&result.m_event.ModScriptCanvasSerializationData());
        auto graphSerializationResult = ScriptCanvas::Serialize(*graph.GetOwnership(), stream);

        if (!graphSerializationResult.m_isSuccessful)
        {
            result.m_parseErrors.push_back(graphSerializationResult.m_errors);
        }

        result.m_isScriptEvents = result.m_parseErrors.empty();
        return result;
    }
}

namespace ScriptEvents
{
    void AddScriptEventHelpers(ScriptCanvas::Graph& graph)
    {
        using namespace ScriptCanvas;

        if (auto variableData = graph.GetVariableData())
        {
            auto confirmStringVariable = [&](AZStd::string_view name, AZStd::string description)
            {
                auto nameVar = variableData->FindVariable(name);
                if (!nameVar || nameVar->GetDataType() == Data::Type::String())
                {
                    Datum datum(Data::Type::String(), Datum::eOriginality::Original, &description, azrtti_typeid<AZStd::string>());
                    AZ::Outcome<ScriptCanvas::VariableId, AZStd::string> addOutcome;
                    GraphVariableManagerRequestBus::EventResult
                        ( addOutcome, graph.GetScriptCanvasId(), &GraphVariableManagerRequests::AddVariable, name, datum, false);
                    AZ_Warning
                        ( "ScriptEvents"
                        , addOutcome.IsSuccess()
                        , "Failed to add helper variable: '%.*s', error: %s", AZ_STRING_ARG(name), addOutcome.GetError().c_str());
                }
            };

            confirmStringVariable("Name", "<name of the ScriptEvent definition>");
            confirmStringVariable("Tooltip", "<helpful explanation for the ScriptEvent definition>");
            confirmStringVariable("Category", "<category for the ScriptEvent definition>");

            // make sure and extenalize string to "Address Type"
            // only add Address if no variable of any type by that name exists
            if (!variableData->FindVariable("Address"))
            {
                confirmStringVariable("Address", "<delete for ScriptEvents with no address, or change type if desired>");
            }
        }
    }
}
