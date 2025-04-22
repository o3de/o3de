/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/any.h>
#include <AzCore/std/containers/stack.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/Math/Crc.h>

namespace ExpressionEvaluation
{
    using ExpressionResult = AZStd::any;
    using ExpressionParserId = AZ::u32;
    using ExpressionVariable = AZStd::any;

    // Information that describes how the element should be handled on the stack.
    struct ElementInformation
    {
        AZ_TYPE_INFO(ElementInformation, "{50C64349-5534-453F-8831-D6C125B4FB2C}");

        enum class OperatorAssociativity
        {
            Left,
            Right
        };

        // Manages whether or not this is an operator or a value type.
        bool m_allowOnOperatorStack = true;

        // The id the parsing inteface has assigned to the operator. Will be passed back in to aid in evaluation.
        int m_id = -1;

        // The priority of this operator. Used to handle pushing/popping of elements.
        int m_priority = 0;

        // Siginfies the associativity of the operators.
        OperatorAssociativity m_associativity = OperatorAssociativity::Left;

        // Any extra chunk of data this Element needs in order to be evaluated.
        AZStd::any m_extraStore;
    };

    // All of the information required to
    struct ExpressionToken
    {
        AZ_TYPE_INFO(ExpressionToken, "{7E6DF1F4-97AC-4553-B839-9A3C88DF1C50}");

        // The interface id that produced the information
        ExpressionParserId m_parserId;

        // The information to be executed upon.
        ElementInformation m_information;
    };
    
    struct ParsingError
    {
        size_t m_offsetIndex = 0;
        AZStd::string m_errorString;

        bool IsValidExpression() const
        {
            return m_offsetIndex == 0 && m_errorString.empty();
        }

        void Clear()
        {
            m_offsetIndex = 0;
            m_errorString.clear();
        }
    };

    // Class to get the various symbol sets that can be added to the parsing steps.
    namespace Interfaces
    {
        static const ExpressionParserId NumericPrimitives = AZ_CRC_CE("ExpressionEngine::NumericPrimitive");
        static const ExpressionParserId BooleanPrimitives = AZ_CRC_CE("ExpressionEngine::BooleanPrimitive");

        static const ExpressionParserId MathOperators = AZ_CRC_CE("ExpressionEngine::BasicMath");
    }
}
