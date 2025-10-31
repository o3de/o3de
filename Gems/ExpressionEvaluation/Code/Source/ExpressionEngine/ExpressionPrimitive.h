/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/string/regex.h>
#include <AzCore/RTTI/RTTI.h>

#include <ExpressionEvaluation/ExpressionEngine/ExpressionTree.h>

#include <ExpressionEngine/ExpressionElementParser.h>
#include <ExpressionEngine/InternalTypes.h>

namespace ExpressionEvaluation
{
    // Shared interface for pushing Primitives onto the evaluation stack. Does not handle parsing.
    class PrimitiveParser
        : public ExpressionElementParser
    {
    public:
        AZ_CLASS_ALLOCATOR(PrimitiveParser, AZ::SystemAllocator);

        PrimitiveParser() = default;

        void EvaluateToken(const ElementInformation& parseResult, ExpressionResultStack& evaluationStack) const override;
    };

    namespace Primitive
    {
        template<typename T>
        ElementInformation GetPrimitiveElement(const T& valueType)
        {
            ElementInformation primitiveInformation;

            primitiveInformation.m_allowOnOperatorStack = false;
            primitiveInformation.m_id = InternalTypes::Primitive;
            primitiveInformation.m_extraStore = valueType;

            return primitiveInformation;
        }
    }

    // Parser for basic numeric types
    class NumericPrimitiveParser
        : public PrimitiveParser
    {
    public:
        AZ_CLASS_ALLOCATOR(NumericPrimitiveParser, AZ::SystemAllocator);

        NumericPrimitiveParser();

        ExpressionParserId GetParserId() const override;
            
        ParseResult ParseElement(const AZStd::string& inputText, size_t offset) const override;            

    private:
        AZStd::regex m_regex;
    };
        
    // Parser for basic boolean types
    class BooleanPrimitiveParser
        : public PrimitiveParser
    {
    public:
        AZ_CLASS_ALLOCATOR(BooleanPrimitiveParser, AZ::SystemAllocator);

        BooleanPrimitiveParser();

        ExpressionParserId GetParserId() const override;

        ParseResult ParseElement(const AZStd::string& inputText, size_t offset) const override;

    private:
        AZStd::regex m_regex;
    };
}
