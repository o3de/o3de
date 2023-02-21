/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/RTTI/RTTI.h>

#include <ExpressionEngine/ExpressionElementParser.h>
#include <ExpressionEngine/InternalTypes.h>

namespace ExpressionEvaluation
{
    struct VariableDescriptor
    {
        AZ_RTTI(VariableDescriptor, "{6D219DB1-3763-4408-A3E8-75E4AE66E9BD}");

        VariableDescriptor()
        {
        }

        VariableDescriptor(const AZStd::string& displayName)
            : m_displayName(displayName)
            , m_nameHash(AZ::Crc32(displayName))
        {
        }
        
        virtual ~VariableDescriptor() = default;

        AZStd::string m_displayName;
        AZ::Crc32 m_nameHash;
    };

    // Interface that adds in support for Variables into the Expression grammar.
    class VariableParser
        : public ExpressionElementParser
    {
    public:
        AZ_CLASS_ALLOCATOR(VariableParser, AZ::SystemAllocator);

        static int GetVariableOperatorId()
        {
            return 1;
        }

        static ElementInformation GetVariableInformation(const AZStd::string& displayName);

        VariableParser();

        ExpressionParserId GetParserId() const override
        {
            return InternalTypes::Interfaces::InternalParser;
        }

        ParseResult ParseElement(const AZStd::string& inputText, size_t offset) const override;
        void EvaluateToken(const ElementInformation& parseResult, ExpressionResultStack& evaluationStack) const override;

    private:
        AZStd::regex m_regex;
    };
}
