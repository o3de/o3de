/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ParsingMetaData.h"

#include <AzCore/std/sort.h>
#include <ScriptCanvas/Debugger/ValidationEvents/ParsingValidation/ParsingValidations.h>
#include <ScriptCanvas/Libraries/Core/ExtractProperty.h>
#include <ScriptCanvas/Libraries/Core/GetVariable.h>
#include <ScriptCanvas/Libraries/Core/Method.h>
#include <ScriptCanvas/Libraries/Core/SetVariable.h>
#include <ScriptCanvas/Libraries/Logic/Break.h>
#include <ScriptCanvas/Libraries/Logic/While.h>
#include <ScriptCanvas/Libraries/Math/MathExpression.h>
#include <ScriptCanvas/Libraries/String/Format.h>
#include <ScriptCanvas/Libraries/String/Print.h>
#include <ScriptCanvas/Utils/BehaviorContextUtils.h>

#include "AbstractCodeModel.h"
#include "Primitives.h"
#include "PrimitivesExecution.h"

namespace ParsingMetaDataCPP
{
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Grammar;

    AZStd::vector<size_t> FindAllPositionsOf(const AZStd::string& targetString, const AZStd::string& searchSpace)
    {
        AZStd::vector<size_t> positions;
        size_t startPosition = 0;
        size_t foundPosition;

        while ((foundPosition = searchSpace.find(targetString, startPosition)) != AZStd::string::npos)
        {
            startPosition = foundPosition + targetString.length();
            positions.push_back(foundPosition);
       }

        return positions;
    }

    void ReplaceInPlace(AZStd::string& replaced, const AZStd::string& from, const AZStd::string& to)
    {
        size_t startPosition = 0;
        size_t foundPosition;
        
        while ((foundPosition = replaced.find(from, startPosition)) != AZStd::string::npos)
        {
            replaced.replace(foundPosition, from.length(), to);
            startPosition = foundPosition + to.length();
        }
    }

    template<typename SlotIdsByName>
    AZStd::vector<AZStd::pair<size_t, SlotId>> ParsePositionsAndSlotIds(const SlotIdsByName& slotIdsByName, const AZStd::string& searchSpace)
    {
        AZStd::vector<AZStd::pair<size_t, SlotId>> positionsAndSlotIds;

        // find and order the position of the variables
        for (const auto& nameAndSlot : slotIdsByName)
        {
            auto findName = AZStd::string::format("{%s}", nameAndSlot.first.c_str());
            auto positions = FindAllPositionsOf(findName, searchSpace);

            for (auto& position : positions)
            {
                positionsAndSlotIds.push_back(AZStd::make_pair(position, nameAndSlot.second));
            }
        }

        AZStd::sort
            ( positionsAndSlotIds.begin()
            , positionsAndSlotIds.end()
            , [](auto& lhs, auto& rhs) { return lhs.first < rhs.first; });

        return positionsAndSlotIds;
    }

    void PostParseExecutionTreeBodyFormatString(AbstractCodeModel& , ExecutionTreePtr format)
    {
        using namespace ParsingMetaDataCPP;

        format->SetNameLexicalScope(LexicalScope(Grammar::LexicalScopeType::Namespace, { k_stringFormatLexicalScopeName }));
        format->SetName(k_stringFormatName);

        auto formatted = azrtti_cast<const Nodes::Internal::StringFormatted*>(format->GetId().m_node);
        auto rawString = formatted->GetRawString();

        const auto& slotIdsByName = formatted->GetNamedSlotIdMap();
        AZStd::vector<AZStd::pair<size_t, SlotId>> positionsAndSlotIds = ParsePositionsAndSlotIds(slotIdsByName, rawString);
        
        // translate the formatted string to C++ / Lua friendly printf format
        AZStd::string formattedString(rawString);

        // escape any percent signs
        ReplaceInPlace(formattedString, "%", "%%");

        // replace script canvas format variables with C++/Lua format variables
        const AZStd::string numberFormat = "%." + AZStd::to_string(formatted->GetPostDecimalPrecision()) + "f";
        for (const auto& nameAndSlotId : slotIdsByName)
        {
            auto replaceString = AZStd::string::format("{%s}", nameAndSlotId.first.c_str());

            if (formatted->GetSlotDataType(nameAndSlotId.second) == Data::Type::Number())
            {
                ReplaceInPlace(formattedString, replaceString, numberFormat);
            }
            else
            {
                ReplaceInPlace(formattedString, replaceString, "%s");
            }
        }

        // store the old input by slot id
        AZStd::unordered_map<SlotId, ExecutionInput> oldInput;
        for (size_t index(0); index < format->GetInputCount(); ++index)
        {
            auto& input = format->GetInput(index);
            oldInput.emplace(input.m_slot->GetId(), input);
        }

        format->ClearInput();

        // use the formated string as the first input
        auto input = AZStd::make_shared<Variable>();
        input->m_source = format;
        input->m_datum = Datum(formattedString);
        format->AddInput({ nullptr, input, DebugDataSource::FromInternal(input->m_datum.GetType()) });

        // rewrite the remaining input so that repeated can appear, and they appear in format string order
        for (auto andSlotId : positionsAndSlotIds)
        {
            format->AddInput(oldInput[andSlotId.second]);
        }
    }
}

namespace ScriptCanvas
{
    namespace Grammar
    {
        // \todo move this to a virtual creation function on the node itself
        MetaDataPtr CreateMetaData(ExecutionTreePtr execution)
        {
            if (execution->GetSymbol() != Symbol::FunctionDefinition)
            {
                if (azrtti_istypeof<Nodes::String::Format>(execution->GetId().m_node))
                {
                    return AZStd::make_shared<FormatStringMetaData>();
                }
                else if (azrtti_istypeof<Nodes::String::Print>(execution->GetId().m_node))
                {
                    return AZStd::make_shared<PrintMetaData>();
                }
                else if (azrtti_istypeof<Nodes::Math::MathExpression>(execution->GetId().m_node))
                {
                    return AZStd::make_shared<MathExpressionMetaData>();
                }
                else if (execution->GetSymbol() == Symbol::FunctionCall)
                {
                    return AZStd::make_shared<FunctionCallDefaultMetaData>();
                }
            }

            return nullptr;
        }

        void FormatStringMetaData::PostParseExecutionTreeBody(AbstractCodeModel& model, ExecutionTreePtr format)
        {
            ParsingMetaDataCPP::PostParseExecutionTreeBodyFormatString(model, format);
        }

        void FunctionCallDefaultMetaData::PostParseExecutionTreeBody(AbstractCodeModel&, ExecutionTreePtr execution)
        {
            execution->SetEventType(execution->GetId().m_node->GetFunctionEventType(execution->GetId().m_slot));

            const auto scopeOutcome = execution->GetId().m_node->GetFunctionCallLexicalScope(execution->GetId().m_slot);
            if (scopeOutcome.IsSuccess())
            {
                execution->SetNameLexicalScope(scopeOutcome.GetValue());
            }

            auto functionCallNameOutcome = execution->GetId().m_node->GetFunctionCallName(execution->GetId().m_slot);
            if (functionCallNameOutcome.IsSuccess())
            {
                execution->SetName(functionCallNameOutcome.GetValue().data());
            }

            auto methodNode = azrtti_cast<const Nodes::Core::Method*>(execution->GetId().m_node);
            if (methodNode && methodNode->HasResult())
            {
                auto typeId = methodNode->GetMethod()->GetResult()->m_typeId;
                if (ScriptCanvas::BehaviorContextUtils::GetUnpackedTypes(typeId).size() > 1)
                {
                    multiReturnType = typeId;
                }
            }
        }

        // turn the print node into a separate string format node and then a print node
        void PrintMetaData::PostParseExecutionTreeBody(AbstractCodeModel& model, ExecutionTreePtr print)
        {            
            // set function call names
            print->SetNameLexicalScope(LexicalScope(Grammar::LexicalScopeType::Namespace, { k_printLexicalScopeName }));
            print->SetName(k_printName);

            // create a the implicit format node
            ExecutionTreePtr format = AZStd::make_shared<ExecutionTree>();
            format->SetId(print->GetId());
            format->SetScope(print->ModScope());

            // fix tree
            auto parent = print->ModParent();
            if (!parent)
            {
                model.AddError(print, aznew Internal::ParseError(AZ::EntityId(), "print has no parent statement"));
                return;
            }

            format->SetParent(parent);

            auto removeOutcome = model.RemoveChild(parent, print);
            if (!removeOutcome.IsSuccess())
            {
                model.AddError(print, aznew Internal::ParseError(AZ::EntityId(), "failed modify print from parent statement"));
                return;
            }

            const auto indexAndChild = removeOutcome.TakeValue();
            
            parent->InsertChild(indexAndChild.first, {indexAndChild.second.m_slot, indexAndChild.second.m_output, format });
            print->SetParent(format);
            format->AddChild({ nullptr, {}, print });

            // move input to format
            format->CopyInput(print, ExecutionTree::RemapVariableSource::Yes);
            print->ClearInput();
            
            // add output to format
            auto output = AZStd::make_shared<Variable>();
            output->m_source = format;
            output->m_datum = AZStd::move(Datum(Data::Type::String(), Datum::eOriginality::Copy));
            output->m_name = print->ModScope()->AddVariableName("format", "output");
            format->ModChild(0).m_output.push_back({ nullptr, model.CreateOutputAssignment(output) });
            
            // make format output the print input
            print->AddInput({ nullptr, output, DebugDataSource::FromInternal(output->m_datum.GetType()) });

            // finish parse of format
            ParsingMetaDataCPP::PostParseExecutionTreeBodyFormatString(model, format);
        }

        void MathExpressionMetaData::PostParseExecutionTreeBody(AbstractCodeModel&, ExecutionTreePtr expression)
        {
            using namespace ParsingMetaDataCPP;

            auto mathNode = azrtti_cast<const Nodes::Math::MathExpression*>(expression->GetId().m_node);
            auto rawString = mathNode->GetRawFormat();
            const auto& slotIdsByName = mathNode->GetSlotsByName();
            AZStd::vector<AZStd::pair<size_t, SlotId>> positionsAndSlotIds = ParsePositionsAndSlotIds(slotIdsByName, rawString);

            // convert the expression string to C++ / Lua math friendly format
            AZStd::string expressionString(rawString);

            // replace script canvas format variables with a place holder for future input
            for (const auto& nameAndSlotId : slotIdsByName)
            {
                auto replaceString = AZStd::string::format("{%s}", nameAndSlotId.first.c_str());
                ReplaceInPlace(expressionString, replaceString, "@");
            }

            // store the old input by slot id
            AZStd::unordered_map<SlotId, ExecutionInput> oldInput;
            for (size_t index(0); index < expression->GetInputCount(); ++index)
            {
                auto& input = expression->GetInput(index);
                oldInput.emplace(input.m_slot->GetId(), input);
            }

            // rewrite the input so that it can appear with repeats and in usage order 
            expression->ClearInput();

            for (auto andSlotId : positionsAndSlotIds)
            {
                expression->AddInput(oldInput[andSlotId.second]);
            }

            m_expressionString = expressionString;
        }

    } 

} 
