/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/string/regex.h>
#include <AzCore/Casting/numeric_cast.h>

#include <AzFramework/StringFunc/StringFunc.h>
#include <ExpressionEngine/ExpressionPrimitive.h>

#include <ExpressionEngine/InternalTypes.h>

namespace ExpressionEvaluation
{
    ////////////////////
    // PrimitiveParser
    ////////////////////

    void PrimitiveParser::EvaluateToken(const ElementInformation& parseResult, ExpressionResultStack& evaluationStack) const
    {
        evaluationStack.emplace(parseResult.m_extraStore);
    }

    ///////////////////////////
    // NumericPrimitiveParser
    ///////////////////////////

    NumericPrimitiveParser::NumericPrimitiveParser()
        : m_regex(R"(^(0|([1-9][0-9]*))(\.[0-9]+)?)")
    {
    }

    ExpressionParserId NumericPrimitiveParser::GetParserId() const
    {
        return Interfaces::NumericPrimitives;
    }
        
    NumericPrimitiveParser::ParseResult NumericPrimitiveParser::ParseElement(const AZStd::string& inputText, size_t offset) const
    {
        AZStd::smatch match;

        ParseResult result;
            
        if (AZStd::regex_search(&inputText.at(offset), match, m_regex))
        {
            AZStd::string matchedCharacters = match[0].str();
                
            result.m_charactersConsumed = static_cast<int>(matchedCharacters.length());
                
            double numericValue = AzFramework::StringFunc::ToDouble(matchedCharacters.c_str());                

            result.m_element = Primitive::GetPrimitiveElement(numericValue);
        }
            
        return result;
    }

    ///////////////////////////
    // BooleanPrimitiveParser
    ///////////////////////////

    BooleanPrimitiveParser::BooleanPrimitiveParser()
        : m_regex(R"((true|false))", AZStd::regex::ECMAScript | AZStd::regex::icase)
    {
    }

    ExpressionParserId BooleanPrimitiveParser::GetParserId() const
    {
        return Interfaces::BooleanPrimitives;
    }

    BooleanPrimitiveParser::ParseResult BooleanPrimitiveParser::ParseElement(const AZStd::string& inputText, size_t offset) const
    {
        AZStd::smatch match;

        ParseResult result;

        if (AZStd::regex_search(&inputText.at(offset), match, m_regex))
        {
            AZStd::string matchedCharacters = match[0].str();
            AZStd::to_lower(matchedCharacters.begin(), matchedCharacters.end());

            result.m_charactersConsumed = matchedCharacters.length();

            bool booleanValue = AzFramework::StringFunc::ToBool(matchedCharacters.c_str());
            result.m_element = Primitive::GetPrimitiveElement(booleanValue);            
        }

        return result;
    }
}
