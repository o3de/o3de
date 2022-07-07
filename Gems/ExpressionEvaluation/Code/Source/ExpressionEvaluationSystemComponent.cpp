/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/Profiler.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <ExpressionEngine/InternalTypes.h>
#include <ExpressionEngine/MathOperators/MathExpressionOperators.h>
#include <ExpressionEngine/Utils.h>
#include <ExpressionEvaluationSystemComponent.h>
#include <ExpressionPrimitivesSerializers.inl>
#include <ElementInformationSerializer.inl>

AZ_DEFINE_BUDGET(ExpressionEvaluation);

namespace ExpressionEvaluation
{
    namespace StructuralParsers
    {
        class InternalExpressionElementParser
            : public ExpressionElementParser
        {
        public:
            AZ_CLASS_ALLOCATOR(InternalExpressionElementParser, AZ::SystemAllocator, 0);

            InternalExpressionElementParser()
                // Just consume spaces, tabs, or commas
                : m_whiteSpaceRegex(R"(^[  ,]+)")
            {

            }

            ExpressionParserId GetParserId() const override
            {
                return InternalTypes::Interfaces::InternalParser;
            }

            ParseResult ParseElement(const AZStd::string& inputText, size_t offset) const override
            {
                ParseResult result;
                AZStd::smatch match;

                if (AZStd::regex_search(&inputText.at(offset), match, m_whiteSpaceRegex))
                {
                    result.m_charactersConsumed = match[0].length();
                }
                else if (inputText.at(offset) == '(')
                {
                    result.m_charactersConsumed = 1;

                    result.m_element.m_id = InternalTypes::OpenParen;
                    result.m_element.m_priority = std::numeric_limits<int>::min();
                }
                else if (inputText.at(offset) == ')')
                {
                    result.m_charactersConsumed = 1;

                    result.m_element.m_id = InternalTypes::CloseParen;
                    result.m_element.m_priority = std::numeric_limits<int>::min();
                }

                return result;
            }

            void EvaluateToken(const ElementInformation& parseResult, ExpressionResultStack& evaluationStack) const override
            {
                AZ_UNUSED(parseResult);
                AZ_UNUSED(evaluationStack);

                AZ_Error("ExpressionEngine", false, "IgnoredSymbolParser should not be used to evaluate tokens.");
            }

        private:

            AZStd::regex m_whiteSpaceRegex;
        };
    }

    ////////////////////////////////////////
    // ExpressionEvaluationSystemComponent
    ////////////////////////////////////////

    static bool ExpressionTokenConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement)
    {
        if (rootElement.GetVersion() < 1)
        {
            AZ::Crc32 interfaceId;
            rootElement.GetChildData(AZ_CRC("InterfaceId", 0x221346a5), interfaceId);
            rootElement.RemoveElementByName(AZ_CRC("InterfaceId", 0x221346a5));

            rootElement.AddElementWithData<unsigned int>(serializeContext, "ParserId", static_cast<unsigned int>(interfaceId));
        }

        return true;
    }

    void ExpressionEvaluationSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<ExpressionEvaluationSystemComponent, AZ::Component>()
                ->Version(0);

            // Only Serializing the information we need to 
            serialize->Class<ElementInformation>()
                ->Version(0)
                ->Field("Id", &ElementInformation::m_id)
                ->Field("ExtraData", &ElementInformation::m_extraStore)
            ;

            serialize->Class<ExpressionToken>()
                ->Version(1, ExpressionTokenConverter)
                ->Field("ParserId", &ExpressionToken::m_parserId)
                ->Field("TokenInformation", &ExpressionToken::m_information)
            ;

            serialize->Class<VariableDescriptor>()
                ->Version(0)
                ->Field("DisplayName", &VariableDescriptor::m_displayName)
                ->Field("NameHash", &VariableDescriptor::m_nameHash)
            ;

            serialize->Class<ExpressionTree::VariableDescriptor>()
                ->Version(0)
                ->Field("SupportedTypes", &ExpressionTree::VariableDescriptor::m_supportedTypes)
                ->Field("Value", &ExpressionTree::VariableDescriptor::m_value)
            ;

            serialize->Class<ExpressionTree>()
                ->Version(0)
                ->Field("Variables", &ExpressionTree::m_variables)
                ->Field("VariableDisplayOrder", &ExpressionTree::m_orderedVariables)
                ->Field("Tokens", &ExpressionTree::m_tokens)
            ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<ExpressionEvaluationSystemComponent>("ExpressionEvaluationGem", "[Description of functionality provided by this System Component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        if (AZ::JsonRegistrationContext* jsonContext = azrtti_cast<AZ::JsonRegistrationContext*>(context))
        {
            jsonContext->Serializer<AZ::ExpressionTreeVariableDescriptorSerializer>()->HandlesType<ExpressionTree::VariableDescriptor>();
            jsonContext->Serializer<AZ::ElementInformationSerializer>()->HandlesType<ElementInformation>();
        }
    }

    void ExpressionEvaluationSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("ExpressionEvaluationGemService", 0xad59526b));
    }

    void ExpressionEvaluationSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("ExpressionEvaluationGemService", 0xad59526b));
    }

    void ExpressionEvaluationSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        AZ_UNUSED(required);
    }

    void ExpressionEvaluationSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }

    ExpressionEvaluationSystemComponent::~ExpressionEvaluationSystemComponent()
    {
        for (auto internalParser : m_internalParsers)
        {
            delete internalParser;
        }

        for (auto parserPair : m_elementInterfaces)
        {
            delete parserPair.second;
        }
    }

    void ExpressionEvaluationSystemComponent::Init()
    {
        m_internalParsers.emplace_back(aznew StructuralParsers::InternalExpressionElementParser());
        m_internalParsers.emplace_back(aznew VariableParser());

        RegisterExpressionInterface(aznew NumericPrimitiveParser());
        RegisterExpressionInterface(aznew MathExpressionOperators());

        RegisterExpressionInterface(aznew BooleanPrimitiveParser());
    }

    void ExpressionEvaluationSystemComponent::Activate()
    {
        ExpressionEvaluationRequestBus::Handler::BusConnect();
    }

    void ExpressionEvaluationSystemComponent::Deactivate()
    {
        ExpressionEvaluationRequestBus::Handler::BusDisconnect();
    }

    void ExpressionEvaluationSystemComponent::RegisterExpressionInterface(ExpressionElementParser* elementParser)
    {
        auto interfaceIter = m_elementInterfaces.find(elementParser->GetParserId());

        if (interfaceIter != m_elementInterfaces.end())
        {
            delete elementParser;
        }
        else
        {
            m_elementInterfaces[elementParser->GetParserId()] = elementParser;
        }
    }

    void ExpressionEvaluationSystemComponent::RemoveExpressionInterface(ExpressionParserId parserId)
    {
        auto interfaceIter = m_elementInterfaces.find(parserId);

        if (interfaceIter != m_elementInterfaces.end())
        {
            delete interfaceIter->second;
            m_elementInterfaces.erase(interfaceIter);
        }
    }

    ParseOutcome ExpressionEvaluationSystemComponent::ParseExpression(AZStd::string_view expressionString) const
    {
        return ParseRestrictedExpression({}, expressionString);
    }

    ParseInPlaceOutcome ExpressionEvaluationSystemComponent::ParseExpressionInPlace(AZStd::string_view expressionString, ExpressionTree& expressionTree) const
    {
        return ParseRestrictedExpressionInPlace({}, expressionString, expressionTree);
    }

    ParseOutcome ExpressionEvaluationSystemComponent::ParseRestrictedExpression(const AZStd::unordered_set<ExpressionParserId>& availableParsers, AZStd::string_view expressionString) const
    {
        ExpressionTree expressionTree;
        AZ::Outcome<void, ParsingError> result = ParseRestrictedExpressionInPlace(availableParsers, expressionString, expressionTree);

        if (result)
        {
            return AZ::Success(expressionTree);
        }

        return AZ::Failure(result.GetError());
    }

    AZ::Outcome<void, ParsingError> ExpressionEvaluationSystemComponent::ParseRestrictedExpressionInPlace(const AZStd::unordered_set<ExpressionParserId>& parsers, AZStd::string_view expressionString, ExpressionTree& expressionTree) const
    {
        AZ_PROFILE_FUNCTION(ExpressionEvaluation);

        expressionTree.ClearTree();

        size_t offset = 0;
        size_t lastOffset = 0;
        size_t endpoint = expressionString.length();

        AZStd::vector< ExpressionToken > operatorStack;

        // Pre-reserve a bunch of space using the size of the string as a rough metric.
        // Should likely be too large assuming variables are used.
        operatorStack.reserve(expressionString.size() / 2);

        AZStd::vector< ExpressionElementParser*> parserList;

        parserList.reserve(m_internalParsers.size() + parsers.size());
        parserList.insert(parserList.begin(), m_internalParsers.begin(), m_internalParsers.end());

        if (!parsers.empty())
        {
            for (const auto& interfaceId : parsers)
            {
                auto interfaceIter = m_elementInterfaces.find(interfaceId);

                if (interfaceIter != m_elementInterfaces.end())
                {
                    parserList.emplace_back(interfaceIter->second);
                }
            }
        }
        else
        {
            parserList.reserve(parserList.size() + m_elementInterfaces.size());

            for (auto interfacePair : m_elementInterfaces)
            {
                parserList.emplace_back(interfacePair.second);
            }
        }

        AZStd::stack<size_t> openParenOffsetStack;

        // We want to make sure our parsing makes logical sense(i.e. goes in the pattern of Value Operator Value Operator Value)
        // Otherwise elements might not function correctly         
        bool expectOperator = false;

        // This is using a ShuntingYard Algorithm to sort out the expression into Reverse Polish Notation.
        while (offset < endpoint)
        {
            for (ExpressionElementParser* parser : parserList)
            {
                ExpressionElementParser::ParseResult result = parser->ParseElement(expressionString, offset);

                // Handle any tree elements that might have been returned.
                if (result.m_element.m_id >= 0)
                {
                    ExpressionToken expressionToken;

                    expressionToken.m_parserId = parser->GetParserId();
                    expressionToken.m_information = AZStd::move(result.m_element);

                    // Handle all of the internal elements
                    if (expressionToken.m_parserId == InternalTypes::Interfaces::InternalParser)
                    {
                        if (result.m_element.m_id == InternalTypes::OpenParen)
                        {
                            if (expectOperator)
                            {
                                return ReportUnexpectedSymbol(expressionString, offset, result.m_charactersConsumed);
                            }

                            operatorStack.emplace_back(AZStd::move(expressionToken));
                            openParenOffsetStack.push(offset);
                        }
                        else if (result.m_element.m_id == InternalTypes::CloseParen)
                        {
                            // Handling the weird case of () being the first element in an expression. Silly, but valid.
                            // If nothing has been added to the tree, we don't want to error on a close paren.
                            if (!expectOperator && expressionTree.GetTreeSize() != 0)
                            {
                                return ReportUnexpectedSymbol(expressionString, offset, result.m_charactersConsumed);
                            }

                            bool foundOpenParen = false;

                            while (!operatorStack.empty())
                            {
                                auto searchExpressionToken = operatorStack.back();
                                operatorStack.pop_back();

                                if (searchExpressionToken.m_parserId == InternalTypes::Interfaces::InternalParser)
                                {
                                    if (searchExpressionToken.m_information.m_id == InternalTypes::OpenParen)
                                    {
                                        foundOpenParen = true;
                                        openParenOffsetStack.pop();
                                        break;
                                    }
                                }
                                else
                                {
                                    expressionTree.PushElement(AZStd::move(searchExpressionToken));
                                }
                            }

                            if (!foundOpenParen)
                            {
                                return ReportUnexpectedSymbol(expressionString, offset, result.m_charactersConsumed);
                            }
                        }
                        else if (expressionToken.m_information.m_id == InternalTypes::Variable)
                        {
                            if (expectOperator)
                            {
                                return ReportUnexpectedValue(expressionString, offset, result.m_charactersConsumed);
                            }

                            VariableDescriptor descriptor = Utils::GetAnyValue<VariableDescriptor>(expressionToken.m_information.m_extraStore);
                            expressionTree.RegisterVariable(descriptor.m_displayName);

                            expressionTree.PushElement(AZStd::move(expressionToken));

                            expectOperator = true;
                        }
                        else
                        {
                            ParsingError parsingError;

                            parsingError.m_offsetIndex = offset;
                            parsingError.m_errorString = AZStd::string::format("Unknown internal tree element with id %i", expressionToken.m_information.m_id);

                            return AZ::Failure(parsingError);
                        }
                    }
                    else if (expressionToken.m_information.m_allowOnOperatorStack)
                    {
                        if (!expectOperator)
                        {
                            return ReportUnexpectedOperator(expressionString, offset, result.m_charactersConsumed);
                        }

                        if (operatorStack.empty())
                        {
                            operatorStack.emplace_back(AZStd::move(expressionToken));
                        }
                        else
                        {
                            int currentPriority = expressionToken.m_information.m_priority;

                            while (!operatorStack.empty())
                            {
                                const ExpressionToken& lastExpressionToken = operatorStack.back();

                                int lastPriority = lastExpressionToken.m_information.m_priority;

                                if (lastPriority < currentPriority)
                                {
                                    break;
                                }
                                else if (lastExpressionToken.m_information.m_associativity == ElementInformation::OperatorAssociativity::Left)
                                {
                                    ExpressionToken tempToken = lastExpressionToken;
                                    operatorStack.pop_back();

                                    expressionTree.PushElement(AZStd::move(tempToken));
                                }
                            }

                            operatorStack.emplace_back(AZStd::move(expressionToken));
                        }

                        expectOperator = false;
                    }
                    else
                    {
                        if (expectOperator)
                        {
                            return ReportUnexpectedValue(expressionString, offset, result.m_charactersConsumed);
                        }

                        expressionTree.PushElement(AZStd::move(expressionToken));

                        expectOperator = true;
                    }
                }

                // Increment out character by the amount of space consumed.
                // Then restart the parsing loop
                if (result.m_charactersConsumed > 0)
                {
                    offset += result.m_charactersConsumed;
                    break;
                }
            }

            if (offset == lastOffset)
            {
                return ReportUnknownCharacter(expressionString, offset);
            }

            lastOffset = offset;
        }

        if (!expectOperator && lastOffset > 0)
        {
            return ReportMissingValue(offset);
        }

        if (!openParenOffsetStack.empty())
        {
            size_t initialOffset = openParenOffsetStack.top();

            AZStd::string unbalancedParensString;

            AZStd::vector<size_t> reversedList;

            while (!openParenOffsetStack.empty())
            {
                reversedList.push_back(openParenOffsetStack.top());
                openParenOffsetStack.pop();
            }

            for (auto reverseIter = reversedList.rbegin(); reverseIter != reversedList.rend(); ++reverseIter)
            {            
                if (!unbalancedParensString.empty())
                {
                    unbalancedParensString.append(", ");
                }

                unbalancedParensString.append(AZStd::to_string((*reverseIter)));
            }

            return ReportUnbalancedParen(initialOffset, unbalancedParensString);
        }

        while (!operatorStack.empty())
        {
            ExpressionToken token = operatorStack.back();
            operatorStack.pop_back();

            expressionTree.PushElement(AZStd::move(token));
        }

        return AZ::Success();
    }

    EvaluateStringOutcome ExpressionEvaluationSystemComponent::EvaluateExpression(AZStd::string_view expression) const
    {
        ParseOutcome treeOutcome = ParseExpression(expression);

        if (!treeOutcome.IsSuccess())
        {
            ParsingError parsingError = treeOutcome.GetError();
            return AZ::Failure(treeOutcome.GetError());
        }

        return AZ::Success(Evaluate(treeOutcome.GetValue()));
    }

    ExpressionResult ExpressionEvaluationSystemComponent::Evaluate(const ExpressionTree& expressionTree) const
    {
        AZ_PROFILE_FUNCTION(ExpressionEvaluation);

        ExpressionResultStack resultStack;

        for (auto expressionToken : expressionTree.GetTokens())
        {
            // Empty one is reserved for internal elements(literals and variables)
            if (expressionToken.m_parserId ==  InternalTypes::Interfaces::InternalParser)
            {
                if (expressionToken.m_information.m_id == InternalTypes::Variable)
                {
                    VariableDescriptor variableDescriptor = Utils::GetAnyValue<VariableDescriptor>(expressionToken.m_information.m_extraStore);

                    AZStd::any variable = expressionTree.GetVariable(variableDescriptor.m_nameHash);
                    resultStack.emplace(AZStd::move(variable));
                }
            }
            else
            {
                auto interfaceIter = m_elementInterfaces.find(expressionToken.m_parserId);

                if (interfaceIter != m_elementInterfaces.end())
                {
                    interfaceIter->second->EvaluateToken(expressionToken.m_information, resultStack);
                }
                else
                {
                    break;
                }
            }
        }

        AZ_Error("ExpressionEngine", resultStack.size() == 1, "Expression Tree should evaluate down to a single result. %i results found.", resultStack.size());

        return resultStack.PopAndReturn();
    }

    AZ::Outcome<void, ParsingError> ExpressionEvaluationSystemComponent::ReportMissingValue(size_t offset) const
    {
        ParsingError parsingError;

        parsingError.m_offsetIndex = offset;
        parsingError.m_errorString = "Parsing completed after processing an Operator and not upon a value, invalid expression.";

        return AZ::Failure(parsingError);
    }

    AZ::Outcome<void, ParsingError> ExpressionEvaluationSystemComponent::ReportUnexpectedOperator(const AZStd::string& parseString, size_t offset, size_t charactersConsumed) const
    {
        AZStd::string substring = parseString.substr(offset, charactersConsumed);

        ParsingError parsingError;

        parsingError.m_offsetIndex = offset;
        parsingError.m_errorString = AZStd::string::format("Unexpected Operator '%s' found at character %zu. Expected a Value.", substring.c_str(), offset);

        return AZ::Failure(parsingError);
    }

    AZ::Outcome<void, ParsingError> ExpressionEvaluationSystemComponent::ReportUnexpectedValue(const AZStd::string& parseString, size_t offset, size_t charactersConsumed) const
    {
        AZStd::string substring = parseString.substr(offset, charactersConsumed);

        ParsingError parsingError;

        parsingError.m_offsetIndex = offset;
        parsingError.m_errorString = AZStd::string::format("Unexpected Value '%s' found at character %zu. Expected an Operator or end of expression.", substring.c_str(), offset);

        return AZ::Failure(parsingError);
    }

    AZ::Outcome<void, ParsingError> ExpressionEvaluationSystemComponent::ReportUnexpectedSymbol(const AZStd::string& parseString, size_t offset, size_t charactersConsumed) const
    {
        AZStd::string substring = parseString.substr(offset, charactersConsumed);

        ParsingError parsingError;

        parsingError.m_offsetIndex = offset;
        parsingError.m_errorString = AZStd::string::format("Unexpected Symbol '%s' found at character %zu.", substring.c_str(), offset);

        return AZ::Failure(parsingError);
    }

    AZ::Outcome<void, ParsingError> ExpressionEvaluationSystemComponent::ReportUnknownCharacter(const AZStd::string& parseString, size_t offset) const
    {
        ParsingError parsingError;

        parsingError.m_offsetIndex = offset;
        parsingError.m_errorString = AZStd::string::format("Unknown character '%c' found in expression.", parseString.at(offset));

        return AZ::Failure(parsingError);
    }

    AZ::Outcome<void, ParsingError> ExpressionEvaluationSystemComponent::ReportUnbalancedParen(size_t offset, const AZStd::string& openParenOffsetString) const
    {
        ParsingError parsingError;

        parsingError.m_offsetIndex = offset;
        parsingError.m_errorString = AZStd::string::format("Unbalanced ( found at character(s) '%s' in expression.", openParenOffsetString.c_str());

        return AZ::Failure(parsingError);
    }
}
