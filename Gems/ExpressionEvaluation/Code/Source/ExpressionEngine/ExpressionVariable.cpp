/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzCore/std/string/regex.h>

#include <ExpressionEngine/ExpressionVariable.h>
#include <ExpressionEngine/InternalTypes.h>

namespace ExpressionEvaluation
{
    ///////////////////
    // VariableParser
    ///////////////////

    ElementInformation VariableParser::GetVariableInformation(const AZStd::string& displayName)
    {
        ElementInformation elementInformation;

        elementInformation.m_allowOnOperatorStack = false;

        elementInformation.m_id = InternalTypes::Variable;

        elementInformation.m_extraStore = VariableDescriptor(displayName);

        return elementInformation;
    }

    VariableParser::VariableParser()
        : m_regex(R"(^\{[^\}]*\})")
    {
    }

    VariableParser::ParseResult VariableParser::ParseElement(const AZStd::string& inputText, size_t offset) const
    {
        AZStd::smatch match;

        ParseResult result;

        if (AZStd::regex_search(&inputText.at(offset), match, m_regex))
        {
            AZStd::string matchedCharacters = match[0].str();

            result.m_charactersConsumed = matchedCharacters.length();

            AZStd::string variableName = matchedCharacters.substr(1, matchedCharacters.length() - 2);

            result.m_element = GetVariableInformation(variableName);
        }

        return result;
    }

    void VariableParser::EvaluateToken(const ElementInformation& parseResult, ExpressionResultStack& evaluationStack) const
    {
        AZ_UNUSED(parseResult);
        AZ_UNUSED(evaluationStack);

        AZ_Error("ExpressionParser", false, "VariableInterface should never be used to evaluate Variable information.");
    }
}
