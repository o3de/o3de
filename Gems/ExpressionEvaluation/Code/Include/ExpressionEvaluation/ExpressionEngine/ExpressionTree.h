/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/unordered_map.h>

#include <ExpressionEvaluation/ExpressionEngine/ExpressionTypes.h>

namespace AZ
{
    class ExpressionTreeVariableDescriptorSerializer;
}

namespace ExpressionEvaluation
{
    // Holds all of the tokeniszed information from parsing an expression string.
    // 
    // Provides interfaces to accessing/manipulating variables that may be exposed.
    class ExpressionTree
    {
        // Friend class for reflection
        friend class ExpressionEvaluationSystemComponent;
        friend class ExpressionTreeVariableDescriptorSerializer;

    public:
        struct VariableDescriptor
        {
            AZ_TYPE_INFO(VariableDescriptor, "{5E1A0044-E0E7-46D3-8BC6-A22E226ADB83}");

            VariableDescriptor()
            {
                m_supportedTypes.push_back(azrtti_typeid<double>());
            }

            AZStd::vector< AZ::Uuid > m_supportedTypes;
            ExpressionVariable        m_value;
        };

        AZ_RTTI(ExpressionTree, "{4CCF3DFD-2EA8-47CB-AF25-353BC034EF42}");
        AZ_CLASS_ALLOCATOR(ExpressionTree, AZ::SystemAllocator);

        ExpressionTree() = default;
        
        virtual ~ExpressionTree()
        {
            ClearTree();
        }

        void ClearTree()
        {
            m_tokens.clear();
            m_variables.clear();
        }

        void PushElement(ExpressionToken&& expressionToken)
        {
            m_tokens.emplace_back(AZStd::move(expressionToken));
        }
        
        void RegisterVariable(const AZStd::string& displayName)
        {
            AZ::Crc32 nameHash = AZ::Crc32(displayName);

            auto insertResult = m_variables.insert_key(nameHash);
            if (insertResult.second)
            {
                m_orderedVariables.push_back(displayName);
            }
        }

        size_t GetTreeSize() const
        {
            return m_tokens.size();
        }

        const AZStd::vector<AZStd::string>& GetVariables() const
        {
            return m_orderedVariables;
        }
        
        const ExpressionVariable& GetVariable(const AZStd::string& name) const
        {
            return GetVariable(AZ::Crc32(name));
        }
        
        const ExpressionVariable& GetVariable(const AZ::Crc32& nameHash) const
        {
            static AZStd::any k_defaultAny;
            
            auto variableIter = m_variables.find(nameHash);
            
            if (variableIter != m_variables.end())
            {
                return variableIter->second.m_value;
            }
            
            return k_defaultAny;
        }

        ExpressionVariable& ModVariable(const AZStd::string& name)
        {
            return ModVariable(AZ::Crc32(name));
        }

        ExpressionVariable& ModVariable(const AZ::Crc32& nameHash)
        {
            static AZStd::any k_defaultAny;

            auto variableIter = m_variables.find(nameHash);

            if (variableIter != m_variables.end())
            {
                return variableIter->second.m_value;
            }

            return k_defaultAny;
        }

        template<typename T>
        void SetVariable(const AZStd::string& name, const T& value)
        {
            AZ::Crc32 nameHash = AZ::Crc32(name);
            SetVariable(nameHash, value);
        }

        template<typename T>
        void SetVariable(const AZ::Crc32& nameHash, const T& value)
        {
            auto variableIter = m_variables.find(nameHash);

            if (variableIter != m_variables.end())
            {
                variableIter->second.m_value = value;
            }
        }
        
        const AZStd::vector< ExpressionToken >& GetTokens() const
        {
            return m_tokens;
        }

        const AZStd::vector<AZ::Uuid>& GetSupportedTypes(const AZStd::string& variableName) const
        {
            AZ::Crc32 nameHash = AZ::Crc32(variableName);
            return GetSupportedTypes(nameHash);
        }

        const AZStd::vector<AZ::Uuid>& GetSupportedTypes(const AZ::Crc32& nameHash) const
        {
            static AZStd::vector<AZ::Uuid> k_defaultTypes;

            auto variableIter = m_variables.find(nameHash);

            if (variableIter != m_variables.end())
            {
                return variableIter->second.m_supportedTypes;
            }

            return k_defaultTypes;
        }

    private:

        
        AZStd::unordered_map< AZ::Crc32, VariableDescriptor > m_variables;
        
        // Signifies the temporal ordering that we encountered the variables in the parsing. Not a sorted order.
        AZStd::vector< AZStd::string > m_orderedVariables;

        AZStd::vector< ExpressionToken > m_tokens;
    };
}
